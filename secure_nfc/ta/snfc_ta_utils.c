#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>
#include <snfc_ta_utils.h>
#include <string.h>
#include <stdlib.h>

uint32_t alg_to_type(uint32_t alg)
{
	switch(alg)
	{
		case TEE_ALG_AES_GCM:
		case TEE_ALG_AES_CTR:
		case TEE_ALG_AES_XTS:
			return TEE_TYPE_AES;

		case TEE_ALG_HMAC_SHA256:
			return TEE_TYPE_HMAC_SHA256;	

		case TEE_ALG_HMAC_MD5:
			return TEE_TYPE_HMAC_MD5;

		default:
			return TEE_TYPE_DATA;
	}
}

void set_flag(uint16_t value, int pos, uint16_t *flags)
{
	// mask to set the lowest significant bit to 0 or 1
	value &= 1;
	
	*flags |= value << pos;
}

int get_flag(int pos, uint16_t flags)
{
	return (flags >> pos) & 1;
}

void* write_buffer(void *src, void *dst, const size_t num)
{
	memcpy(dst, src, num);
	return (void*)((char*)dst+num);
}

size_t write_tee_object_ref_attribute(TEE_ObjectHandle obj, uint32_t attribute, char *buf)
{
	size_t attr_size = 0;
	
	// Will write the attribute to the buffer, we must write the size before it
	TEE_GetObjectBufferAttribute(obj, attribute, (buf+sizeof(size_t)), &attr_size);
	// Write the size before the 
	buf = write_buffer(&attr_size, buf, sizeof(attr_size));

	return sizeof(size_t) + attr_size;
}

void print_ta_hex(char *buf, size_t buf_size)
{
	for(int i = 0; i < buf_size; i++)
	{
		DMSG("%02x ", buf[i]);
	}

	DMSG("\n");
}

TEE_Result create_digest_signature(int signature_alg, TEE_ObjectHandle key, size_t key_size, char *digest, size_t digest_len, char *signature, size_t *signature_len)
{
	TEE_Result res;

	TEE_OperationHandle sign_op;

	if((res = TEE_AllocateOperation(&sign_op, signature_alg, TEE_MODE_SIGN, key_size)) == TEE_SUCCESS)
	{
		if((res = TEE_SetOperationKey(sign_op, key)) == TEE_SUCCESS)
		{
			res = TEE_AsymmetricSignDigest(sign_op, NULL, 0, digest, digest_len, signature, signature_len);
			if(res != TEE_SUCCESS) DMSG("Failed TEE_AsymmetricSignDigest, error = %x\n", res);
		}

		TEE_FreeOperation(sign_op);
	}

	return res;
}

TEE_Result verify_digest_signature(int signature_alg, TEE_ObjectHandle key, size_t key_size, char *digest, size_t digest_len, char *signature, size_t signature_len)
{
	TEE_Result res = TEE_ERROR_GENERIC;

	TEE_OperationHandle verify_op;

	if((res = TEE_AllocateOperation(&verify_op, signature_alg, TEE_MODE_VERIFY, key_size)) == TEE_SUCCESS)
	{		
		if((res = TEE_SetOperationKey(verify_op, key)) == TEE_SUCCESS)
		{
			res = TEE_AsymmetricVerifyDigest(verify_op, NULL, 0, digest, digest_len, signature, signature_len);
			if(res != TEE_SUCCESS) DMSG("Failed TEE_AsymmetricVerifyDigest, error = %x\n", res);
		}

		TEE_FreeOperation(verify_op);
	}

	return res;
}

TEE_Result create_digest(int digest_alg, char *msg, size_t msg_len, char* digest, size_t *digest_len)
{
	TEE_Result res;
	
	TEE_OperationHandle digest_op;
	
	if((res = TEE_AllocateOperation(&digest_op, digest_alg, TEE_MODE_DIGEST, 0)) == TEE_SUCCESS)
	{
		TEE_ResetOperation(digest_op);	
		res = TEE_DigestDoFinal(digest_op, msg, msg_len, digest, digest_len);
		if(res != TEE_SUCCESS) DMSG("Failed TEE_DigestDoFinal, error %x\n", res);
		
		TEE_FreeOperation(digest_op);
	}

	return res;
}

TEE_Result read_ecc_public_key(char *buf, int key_type, size_t key_size, int ecc_curve, TEE_ObjectHandle *ecc_key)
{
	TEE_Result res;

	if((res = TEE_AllocateTransientObject(key_type, key_size, ecc_key)) == TEE_SUCCESS)
	{
		TEE_Attribute attrs[3];
		size_t key_size_bytes = key_size / 8;
		TEE_InitRefAttribute(&attrs[0], TEE_ATTR_ECC_PUBLIC_VALUE_X, buf, key_size_bytes);
		TEE_InitRefAttribute(&attrs[1], TEE_ATTR_ECC_PUBLIC_VALUE_Y, buf + key_size_bytes, key_size_bytes);
		TEE_InitValueAttribute(&attrs[2], TEE_ATTR_ECC_CURVE, ecc_curve, 0);
		res = TEE_PopulateTransientObject(*ecc_key, attrs, sizeof(attrs)/sizeof(TEE_Attribute));
	}

	return res;
}

TEE_Result get_certificate_keypair(TEE_ObjectHandle *keypair, char *pub_key, size_t key_size, char *priv_key, int curve)
{
	TEE_Result res;

	if((res = TEE_AllocateTransientObject(TEE_TYPE_ECDSA_KEYPAIR, key_size, keypair)) == TEE_SUCCESS)
	{
		TEE_Attribute attrs[4];
		size_t key_size_bytes = key_size / 8;
		TEE_InitRefAttribute(&attrs[0], TEE_ATTR_ECC_PUBLIC_VALUE_X, pub_key, key_size_bytes);
		TEE_InitRefAttribute(&attrs[1], TEE_ATTR_ECC_PUBLIC_VALUE_Y, pub_key + key_size_bytes, key_size_bytes);
		TEE_InitRefAttribute(&attrs[2], TEE_ATTR_ECC_PRIVATE_VALUE, priv_key, key_size_bytes);
		TEE_InitValueAttribute(&attrs[3], TEE_ATTR_ECC_CURVE, curve, 0);
		res = TEE_PopulateTransientObject(*keypair, attrs, sizeof(attrs)/sizeof(TEE_Attribute));
	}

	return res;
}

// Not used in normal execution, only during development phase to create the signatures
/*TEE_Result create_ca_signature(char *signature, size_t *signature_len)
{
	TEE_Result res = TEE_ERROR_GENERIC;

	size_t digest_len = CA_SIGNATURE_HASH_LENGTH / 8;
	char *digest = calloc(digest_len, sizeof(char));

	if(digest)
	{
		if((res = create_digest(CA_SIGNATURE_HASH_ALG, CERT_PUBLIC_KEY, (CERT_KEY_SIZE / 8) * 2, digest, &digest_len)) == TEE_SUCCESS)
		{
			TEE_ObjectHandle ca_keypair;

			if((res = get_certificate_keypair(&ca_keypair, CA_PUBLIC_KEY, CA_KEY_SIZE, CA_PRIVATE_KEY, CA_ECC_CURVE)) == TEE_SUCCESS)
			{
				res = create_digest_signature(CA_SIGNATURE_ALG, ca_keypair, CA_KEY_SIZE, digest, digest_len, signature, signature_len);
				TEE_FreeTransientObject(ca_keypair);
			}
		}
		
		TEE_Free(digest);
	}

	return res;
}*/
