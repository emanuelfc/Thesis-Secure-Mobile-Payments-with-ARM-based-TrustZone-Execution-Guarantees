#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>
#include <snfc_ta_authentication.h>
#include <snfc_ta_handshake.h>
#include <snfc_associations.h>
#include <snfc_ta_utils.h>
#include <snfc_ta_context.h>
#include <string.h>
#include <stdlib.h>

// Flags
#define SNFC_FLAGS_AUTHENTICATION 1

// Diffie Hellman Key Exchange

static TEE_Result create_ecdh_key(key_exchange_association kea, TEE_ObjectHandle *ecdh_key)
{
	TEE_Result res = TEE_ERROR_GENERIC;
	
	if((res = TEE_AllocateTransientObject(TEE_TYPE_ECDH_KEYPAIR, kea.key_size, ecdh_key)) == TEE_SUCCESS)
	{
		TEE_Attribute ecc_curve_attr;
		TEE_InitValueAttribute(&ecc_curve_attr, TEE_ATTR_ECC_CURVE, kea.curve, 0);
		res = TEE_GenerateKey(*ecdh_key, kea.key_size, &ecc_curve_attr, 1);

		if(res != TEE_SUCCESS) DMSG("Failed TEE_GenerateKey, error = %x\n", res);
	}
	else DMSG("Failed TEE_AllocateTransientObject, error = %x\n", res);
	
	return res;
}

static TEE_Result derive_shared_secret(TEE_ObjectHandle ecdh_key, key_exchange_association kea, const char *ecc_public_value_x, const char *ecc_public_value_y, TEE_ObjectHandle *shared_secret)
{
	TEE_Result res = TEE_ERROR_GENERIC;
	
	if((res = TEE_AllocateTransientObject(TEE_TYPE_GENERIC_SECRET, kea.key_size, shared_secret)) == TEE_SUCCESS)
	{
		TEE_Attribute attrs[2];
		TEE_InitRefAttribute(&attrs[0], TEE_ATTR_ECC_PUBLIC_VALUE_X, ecc_public_value_x, kea.key_size / 8);
		TEE_InitRefAttribute(&attrs[1], TEE_ATTR_ECC_PUBLIC_VALUE_Y, ecc_public_value_y, kea.key_size / 8);
		
		TEE_OperationHandle ecdh;
		
		if((res = TEE_AllocateOperation(&ecdh, kea.algorithm, TEE_MODE_DERIVE, kea.key_size)) == TEE_SUCCESS)
		{
			TEE_SetOperationKey(ecdh, ecdh_key);
			
			TEE_DeriveKey(ecdh, attrs, sizeof(attrs)/sizeof(TEE_Attribute), *shared_secret);
			
			TEE_FreeOperation(ecdh);
		
			res = TEE_SUCCESS;
		}
		else
		{
			TEE_FreeTransientObject(*shared_secret);
			DMSG("Failed TEE_AllocateOperation, error = %x\n", res);
		}
	}
	else DMSG("Failed TEE_AllocateTransientObject, error = %x\n", res);
	
	return res;
}

static TEE_Result create_key(TEE_ObjectHandle *key, size_t key_size, uint32_t alg, char *secret_value_buf)
{
	TEE_Result res;

	if((res = TEE_AllocateTransientObject(alg_to_type(alg), key_size, key)) == TEE_SUCCESS)
	{
		TEE_ResetTransientObject(*key);

		TEE_Attribute secret_value_attr;
		TEE_InitRefAttribute(&secret_value_attr, TEE_ATTR_SECRET_VALUE, secret_value_buf, key_size / 8);
		TEE_PopulateTransientObject(*key, &secret_value_attr, 1);


		res = TEE_SUCCESS;
	}

	return res;
}

static TEE_Result derive_session_keys(security_context *ctx, TEE_ObjectHandle *shared_secret, TEE_ObjectHandle *cipher_key, TEE_ObjectHandle *mac_key)

{
	TEE_Result res = TEE_ERROR_GENERIC;

	size_t shared_secret_size = ctx->kea.key_size / 8;			
	char *shared_secret_buf = calloc(shared_secret_size, sizeof(char));

	if((res = TEE_GetObjectBufferAttribute(*shared_secret, TEE_ATTR_SECRET_VALUE, shared_secret_buf, &shared_secret_size)) == TEE_SUCCESS)
	{		
		size_t hash_size = 512 / 8;
		char *hash_buf = calloc(hash_size, sizeof(hash_size));
		
		if(hash_buf)
		{
			if((res = create_digest(TEE_ALG_SHA512, shared_secret_buf, shared_secret_size, hash_buf, &hash_size)) == TEE_SUCCESS)
			{
				if((res = create_key(cipher_key, ctx->sa.key_size, ctx->sa.cipher_alg, hash_buf)) == TEE_SUCCESS)
				{
					if((res = create_key(mac_key, ctx->sa.mac_key_size, ctx->sa.mac_alg, hash_buf + (ctx->sa.key_size / 8))) == TEE_SUCCESS)
					{
						res = TEE_SUCCESS;
					}
					else DMSG("Failed create_key(mac_key), error = %x\n", res); 
				}
				else DMSG("Failed create_key(cipher_key), error = %x\n", res);
			}
			else DMSG("Failed create_digest, error = %x\n", res); 
			
			TEE_Free(hash_buf);
		}
		DMSG("Failed hash_buf allocation, error = %x\n", res); 
	}

	free(shared_secret_buf);

	return res;
}

static TEE_Result write_ecc_public_values(TEE_ObjectHandle ecdh_key, char *buf, size_t buf_size, uint32_t *write_bytes)
{
	TEE_Result res = TEE_ERROR_GENERIC;
	
	uint32_t key_size_bytes = buf_size;
	if((res = TEE_GetObjectBufferAttribute(ecdh_key, TEE_ATTR_ECC_PUBLIC_VALUE_X, buf, &key_size_bytes)) == TEE_SUCCESS)
	{
		*write_bytes = key_size_bytes;
		res = TEE_GetObjectBufferAttribute(ecdh_key, TEE_ATTR_ECC_PUBLIC_VALUE_Y, buf + key_size_bytes, &key_size_bytes);
		if(res != TEE_SUCCESS) DMSG("Failed TEE_GetObjectBufferAttribute, error = %x\n", res);
		*write_bytes += key_size_bytes;	
	}
	else DMSG("Failed TEE_GetObjectBufferAttribute, error = %x\n", res);

	return res;
}

static TEE_Result write_key_exchange_parameters(TEE_ObjectHandle ecdh_key, char *buf, size_t buf_size, uint32_t *write_bytes)
{
	return write_ecc_public_values(ecdh_key, buf, buf_size, write_bytes);
}

// Security Context

static TEE_Result iv_init(security_context *ctx, char *buf)
{
	TEE_Result res = TEE_ERROR_GENERIC;
	
	size_t iv_len = ctx->sa.block_size / 8;
	if(!ctx->iv) ctx->iv = calloc(iv_len, sizeof(char));

	if(ctx->iv)
	{
		res = create_digest(TEE_ALG_MD5, buf, (ctx->kea.key_size / 8) * 2, ctx->iv, &iv_len);
		if(res != TEE_SUCCESS) DMSG("Failed create_digest, error %x\n", res);
	}
	
	return res;
}

static TEE_Result security_context_init(security_context *ctx, TEE_ObjectHandle *cipher_key, TEE_ObjectHandle *mac_key)
{
	TEE_Result res;	
	
	// Allocate cipher encrypt operation
	if((res = TEE_AllocateOperation(&ctx->encrypt_handle, ctx->sa.cipher_alg, TEE_MODE_ENCRYPT, ctx->sa.key_size)) == TEE_SUCCESS)
	{
		// Allocate cipher decrypt operation
		if((res = TEE_AllocateOperation(&ctx->decrypt_handle, ctx->sa.cipher_alg, TEE_MODE_DECRYPT, ctx->sa.key_size)) == TEE_SUCCESS)
		{
			if((res = TEE_AllocateOperation(&ctx->mac_handle, ctx->sa.mac_alg, TEE_MODE_MAC, ctx->sa.mac_key_size)) == TEE_SUCCESS)
			{
				// Set key for the operations
				if((res = TEE_SetOperationKey(ctx->encrypt_handle, *cipher_key)) == TEE_SUCCESS &&
					(res = TEE_SetOperationKey(ctx->decrypt_handle, *cipher_key)) == TEE_SUCCESS &&
					(res = TEE_SetOperationKey(ctx->mac_handle, *mac_key)) == TEE_SUCCESS)
				{
					// Set iv	
					ctx->seq_n = 0;				
					return res;
				}

				DMSG("Failed TEE_SetOperationKey, error = %x\n", res);
			}
			
			DMSG("Failed TEE_AllocateOperation, error = %x\n", res);
			
			TEE_FreeOperation(ctx->decrypt_handle);
		}
		
		DMSG("Failed TEE_AllocateOperation, error = %x\n", res);
		
		TEE_FreeOperation(ctx->encrypt_handle);
	}
	
	DMSG("Failed TEE_AllocateOperation, error = %x\n", res);
	
	return res;
}

void security_context_close(security_context *ctx)
{
	if(ctx->encrypt_handle) TEE_FreeOperation(ctx->encrypt_handle);
	if(ctx->decrypt_handle) TEE_FreeOperation(ctx->decrypt_handle);
	if(ctx->mac_handle) TEE_FreeOperation(ctx->mac_handle);
	// No need to free the key object, FreeOperation upon encrypt and decrypt handles does that for each operation key
	if(ctx->ecdh_key) TEE_FreeTransientObject(ctx->ecdh_key);
	if(ctx->iv) TEE_Free(ctx->iv);
	if(ctx->peer_pub_key) TEE_FreeTransientObject(ctx->peer_pub_key);
	ctx->seq_n = 0;
}

TEE_Result set_security_parameters(security_context *ctx, uint8_t spi, uint8_t kei)
{
	ctx->sa = SECURITY_ASSOCIATIONS[spi];
	ctx->kea = KEY_EXCHANGE_ASSOCIATIONS[kei];

	return create_ecdh_key(ctx->kea, &ctx->ecdh_key);
}

// Packet processing functions

static TEE_Result process_key_exchange(security_context *ctx, char *buf)
{
	TEE_Result res = TEE_ERROR_GENERIC;	

	// We have the peer's public key, we can create a session key and initialize the security context
	
	TEE_ObjectHandle shared_secret;
	if((res = derive_shared_secret(ctx->ecdh_key, ctx->kea, buf, buf + ctx->kea.key_size / 8, &shared_secret)) == TEE_SUCCESS)
	{
		TEE_ObjectHandle cipher_key, mac_key;
		if((res = derive_session_keys(ctx, &shared_secret, &cipher_key, &mac_key)) == TEE_SUCCESS)
		{
			res = security_context_init(ctx, &cipher_key, &mac_key);
			TEE_FreeTransientObject(cipher_key);
			TEE_FreeTransientObject(mac_key);	
		}
		else DMSG("Failed derive_session_keys, error = %x\n", res);

		TEE_FreeTransientObject(shared_secret);
	}
	else DMSG("Failed derive_shared_secret, error = %x\n", res);
	
	return res;
}

// Handshake request

static handshake_request new_handshake_request(uint8_t spi, uint8_t kei, uint8_t flags)
{
	handshake_request hreq;

	hreq.spi = spi;
	hreq.kei = kei;
	hreq.flags = flags;
	
	return hreq;
}

static handshake_request read_handshake_request(char *buf)
{
	handshake_request hreq;

	hreq.spi = (uint8_t)*buf;
	buf += sizeof(hreq.spi);

	hreq.kei = (uint8_t)*buf;
	buf += sizeof(hreq.kei);

	hreq.flags = (uint8_t)*buf;
	buf += sizeof(hreq.flags);

	return hreq;
}

static size_t write_handshake_request(handshake_request hreq, char *buf)
{
	char *buf_start = buf;	

	buf = write_buffer(&hreq, buf, sizeof(hreq));

	return buf - buf_start;
}

TEE_Result create_handshake_request(security_context *ctx, uint8_t spi, uint8_t kei, uint8_t flags, char *buf, size_t *buf_size)
{
	TEE_Result res = TEE_ERROR_GENERIC;

	size_t write_bytes = 0;
	size_t total_write_bytes = 0;
	
	// Write the handshake parameters in the message
	handshake_request hreq = new_handshake_request(spi, kei, flags);
	write_bytes = write_handshake_request(hreq, buf);
	total_write_bytes += write_bytes;
	
	// Write key exchange parameters in the message
	if((res = write_key_exchange_parameters(ctx->ecdh_key, buf + total_write_bytes, *buf_size - total_write_bytes, &write_bytes)) == TEE_SUCCESS)
	{
		if((res = iv_init(ctx, buf + total_write_bytes)) == TEE_SUCCESS)
		{		
			total_write_bytes += write_bytes;

			// If authentication is enabled, write authentication parameters
			if(flags && SNFC_FLAGS_AUTHENTICATION)
			{
				res = create_authentication_id(buf + total_write_bytes, &write_bytes);
				total_write_bytes += write_bytes;
			}

			*buf_size = total_write_bytes;
		}
	}
	
	return res;
}

// Handshake reply

TEE_Result process_handshake_request(security_context *ctx, char *buf, size_t buf_size, char *reply_buf, size_t *reply_buf_size)
{
	TEE_Result res = TEE_ERROR_GENERIC;

	handshake_request hreq = read_handshake_request(buf);
	buf += sizeof(hreq);

	char *kep = buf;
	
	if((res = set_security_parameters(ctx, hreq.spi, hreq.kei)) == TEE_SUCCESS)
	{
		if((res = process_key_exchange(ctx, kep)) == TEE_SUCCESS)
		{			
			if((res = iv_init(ctx, kep)) == TEE_SUCCESS)
			{
				size_t total_write_bytes = 0;			
				size_t write_bytes = 0;
				
				// Write key exchange parameters in the message
				if((res = write_key_exchange_parameters(ctx->ecdh_key, reply_buf, reply_buf_size, &write_bytes)) == TEE_SUCCESS)
				{
					total_write_bytes += write_bytes;
		
					// If authentication is enabled, perform authentication
					if(SNFC_FLAGS_AUTHENTICATION && hreq.flags)
					{
						buf += (ctx->kea.key_size / 8) * 2;
						if((res = process_authentication_id(ctx, buf)) == TEE_SUCCESS)
						{
							if((res = create_authentication_id(reply_buf + total_write_bytes, &write_bytes)) == TEE_SUCCESS)
							{
								total_write_bytes += write_bytes;
								res = create_authentication_proof(ctx, kep, reply_buf + total_write_bytes, &write_bytes);
								total_write_bytes += write_bytes;
							}
							else DMSG("Failed create_authentication_id, error = %x\n", res);
						}
						else DMSG("Failed process_authentication_id, error = %x\n", res);
					}

					*reply_buf_size = total_write_bytes;
				}
			}
			else DMSG("Failed iv_init, error = %x\n", res);
		}
		else DMSG("Failed process_key_exchange, error = %x\n", res);
	}
	else DMSG("Failed set_security_parameters, error = %x\n", res);

	return res;
}

TEE_Result process_handshake_reply(security_context *ctx, uint8_t flags, char *buf, size_t buf_size, char *write_buf, size_t *write_buf_size)
{
	TEE_Result res = TEE_ERROR_GENERIC;

	char *kep = buf;

	if((res = process_key_exchange(ctx, kep)) == TEE_SUCCESS)
	{		
		buf += (ctx->kea.key_size / 8) * 2;

		// If authentication is enabled, perform authentication
		if(SNFC_FLAGS_AUTHENTICATION && flags)
		{
			if((res = process_authentication_id(ctx, buf)) == TEE_SUCCESS)
			{
				// first 256 = CERT_KEY_SIZE, second = CA_KEY_SIZE
				// quick fix since compiler keeps missing the declaration even after the error being fixed
				buf += (224 / 8) * 2 + (224 / 8) * 2;
				if((res = process_authentication_proof(ctx, buf)) == TEE_SUCCESS)
				{
					res = create_authentication_proof(ctx, kep, write_buf, write_buf_size);
				}
			}
			else DMSG("Failed process_authentication_id, error = %x\n");
		}
	}
	else DMSG("Failed process_key_exchange, error = %x\n");

	return res;
}

