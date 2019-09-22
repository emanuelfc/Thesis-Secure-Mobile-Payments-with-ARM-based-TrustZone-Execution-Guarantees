#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>
#include <snfc_ta_authentication.h>
#include <cert.h>
#include <ca_cert.h>
#include <snfc_ta_utils.h>
#include <snfc_ta_context.h>
#include <string.h>
#include <stdlib.h>

static size_t write_authentication_id(authentication_id auth_id, char *buf)
{
	char *buf_start = buf;
		
	buf = write_buffer(auth_id.pub_key, buf, (CERT_KEY_SIZE / 8) * 2);
	buf = write_buffer(auth_id.ca_signature, buf, (CA_KEY_SIZE / 8) * 2);
	
	return buf - buf_start;
}

static authentication_id new_authentication_id()
{
	authentication_id auth_id;

	auth_id.pub_key = CERT_PUBLIC_KEY;
	auth_id.ca_signature = CERT_CA_SIGNATURE;

	return auth_id;
}

TEE_Result create_authentication_id(char *buf, size_t *write_bytes)
{
	authentication_id auth_id = new_authentication_id();
	*write_bytes = write_authentication_id(auth_id, buf);

	return TEE_SUCCESS;
}

static authentication_id read_authentication_id(char *buf, size_t pub_key_size)
{
	authentication_id auth_id;

	auth_id.pub_key = buf;
	auth_id.ca_signature = buf + (pub_key_size / 8) * 2;
	
	return auth_id;
}

static TEE_Result verify_public_key(char *pub_key, size_t key_size, char *pub_key_sign)
{
	TEE_Result res = TEE_ERROR_GENERIC;
	
	size_t pub_key_digest_len = (CA_SIGNATURE_HASH_LENGTH / 8) * 2;
	char *pub_key_digest = calloc(pub_key_digest_len, sizeof(char));

	if((res = create_digest(CA_SIGNATURE_HASH_ALG, pub_key, (key_size / 8) * 2, pub_key_digest, &pub_key_digest_len)) == TEE_SUCCESS)
	{	
		TEE_ObjectHandle ca_public_key;
		if((res = read_ecc_public_key(CA_PUBLIC_KEY, TEE_TYPE_ECDSA_PUBLIC_KEY, CA_KEY_SIZE, CA_ECC_CURVE, &ca_public_key)) == TEE_SUCCESS)
		{	
			res = verify_digest_signature(CA_SIGNATURE_ALG, ca_public_key, CA_KEY_SIZE, pub_key_digest, pub_key_digest_len, CERT_CA_SIGNATURE, (CA_KEY_SIZE / 8) * 2);
		}
	}

	free(pub_key_digest);

	return res;
}

TEE_Result process_authentication_id(security_context *ctx, char *buf)
{
	TEE_Result res = TEE_ERROR_GENERIC; 

	authentication_id auth_id = read_authentication_id(buf, CA_KEY_SIZE);

	if((res = verify_public_key(auth_id.pub_key, CERT_KEY_SIZE, auth_id.ca_signature)) == TEE_SUCCESS)
	{
		res = read_ecc_public_key(auth_id.pub_key, TEE_TYPE_ECDSA_PUBLIC_KEY, CERT_KEY_SIZE, CERT_ECC_CURVE, &ctx->peer_pub_key);
	}

	return res;
}

static TEE_Result create_kep_digest(security_context *ctx, char *digest, size_t *digest_len)
{
	TEE_Result res = TEE_ERROR_GENERIC;
	
	size_t key_values_buf_size = (ctx->kea.key_size / 8) * 2;
	char *key_values_buf = calloc(key_values_buf_size, sizeof(char));

	if((res = TEE_GetObjectBufferAttribute(ctx->ecdh_key, TEE_ATTR_ECC_PUBLIC_VALUE_X, key_values_buf, &key_values_buf_size)) == TEE_SUCCESS)
	{
		if((res = TEE_GetObjectBufferAttribute(ctx->ecdh_key, TEE_ATTR_ECC_PUBLIC_VALUE_Y, key_values_buf + (ctx->kea.key_size / 8), &key_values_buf_size)) == TEE_SUCCESS)
		{	
			res = create_digest(CERT_SIGNATURE_HASH_ALG, key_values_buf, key_values_buf_size * 2, digest, digest_len);
			
			free(key_values_buf);
		}
	}

	return res;
}

TEE_Result create_authentication_proof(security_context *ctx, char *kep, char *write_buf, size_t *write_bytes)
{
	TEE_Result res = TEE_ERROR_GENERIC;

	size_t digest_len = CERT_SIGNATURE_HASH_LENGTH / 8;
	char *digest = calloc(digest_len, sizeof(char));

	if(digest)
	{	

		if((res = create_digest(CERT_SIGNATURE_HASH_ALG, kep, (ctx->kea.key_size / 8) * 2, digest, &digest_len)) == TEE_SUCCESS)
		{			
			TEE_ObjectHandle cert_keypair;
			if((res = get_certificate_keypair(&cert_keypair, CERT_PUBLIC_KEY, CERT_KEY_SIZE, CERT_PRIVATE_KEY, CERT_ECC_CURVE)) == TEE_SUCCESS)
			{
				size_t kep_signature_size = (CERT_KEY_SIZE / 8) * 2;
				char *kep_signature = calloc(kep_signature_size, sizeof(char));

				if(kep_signature)
				{
					if((res = create_digest_signature(CERT_SIGNATURE_ALG, cert_keypair, CERT_KEY_SIZE, digest, digest_len, kep_signature, &kep_signature_size)) == TEE_SUCCESS)
					{				
						memcpy(write_buf, kep_signature, kep_signature_size);
						*write_bytes = kep_signature_size;
						res = TEE_SUCCESS;
					}

					TEE_Free(kep_signature);
				}

				TEE_FreeTransientObject(cert_keypair);
			}
		}
		
		TEE_Free(digest);
	}

	return res;
}

TEE_Result process_authentication_proof(security_context *ctx, char *kep_auth_proof)
{
	TEE_Result res = TEE_ERROR_GENERIC;	

	size_t kep_digest_size = CERT_SIGNATURE_HASH_LENGTH / 8;
	char *kep_digest = calloc(kep_digest_size, sizeof(char));
	
	if(kep_digest)
	{
		if((res = create_kep_digest(ctx, kep_digest, &kep_digest_size)) == TEE_SUCCESS)
		{
			res = verify_digest_signature(CERT_SIGNATURE_ALG, ctx->peer_pub_key, CERT_KEY_SIZE, kep_digest, kep_digest_size, kep_auth_proof, (CERT_KEY_SIZE / 8) * 2);
			if(res != TEE_SUCCESS) DMSG("Failed verify_digest_signature, res = %x\n", res);
		}
		else DMSG("Failed create_kep_digest, res = %x\n", res);

		TEE_Free(kep_digest);
	}

	return res;
}

