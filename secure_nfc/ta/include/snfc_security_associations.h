#pragma once

#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

typedef struct security_association
{
	uint32_t cipher_alg;
	uint32_t key_size;
	uint32_t block_size;
	uint32_t mac_alg;
	uint32_t mac_key_size;
	uint32_t mac_len;
	
} security_association;

/*
static security_association INVALID_SA = 
{
	.cipher_alg = 0,
	.key_size = 0,
	.block_size = 0,
	.mac_alg = 0,
	.mac_len = 0,
};
*/

static security_association SECURITY_ASSOCIATIONS[256] = 
{
	{
		.cipher_alg = TEE_ALG_AES_GCM,
		.key_size = 128,
		.block_size = 128,
		.mac_alg = TEE_ALG_HMAC_SHA256,
		.mac_key_size = 256,
		.mac_len = 256,
	},

	{
		.cipher_alg = TEE_ALG_AES_GCM,
		.key_size = 128,
		.block_size = 128,
		.mac_alg = TEE_ALG_HMAC_MD5,
		.mac_key_size = 128,
		.mac_len = 128,
	},

	{
		.cipher_alg = TEE_ALG_AES_CTR,
		.key_size = 128,
		.block_size = 128,
		.mac_alg = TEE_ALG_HMAC_SHA256,
		.mac_key_size = 256,
		.mac_len = 256,
	},

	{
		.cipher_alg = TEE_ALG_AES_XTS,
		.key_size = 128,
		.block_size = 128,
		.mac_alg = TEE_ALG_HMAC_MD5,
		.mac_key_size = 128,
		.mac_len = 128,
	},

	{
		.cipher_alg = TEE_ALG_AES_GCM,
		.key_size = 256,
		.block_size = 256,
		.mac_alg = TEE_ALG_HMAC_SHA256,
		.mac_key_size = 256,
		.mac_len = 256,
	},

	{
		.cipher_alg = TEE_ALG_AES_CTR,
		.key_size = 256,
		.block_size = 256,
		.mac_alg = TEE_ALG_HMAC_SHA256,
		.mac_key_size = 256,
		.mac_len = 256,
	},
};
