#pragma once

#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

typedef struct key_exchange_association
{
	uint32_t algorithm;
	uint32_t curve;
	uint32_t key_size;
	
} key_exchange_association;

/*
static key_exchange_association INVALID_KEA = 
{
	.algorithm = 0,
	.key_size = 0,
};
*/

static key_exchange_association KEY_EXCHANGE_ASSOCIATIONS[256] = 
{
	{
		.algorithm = TEE_ALG_ECDH_P224,
		.curve = TEE_ECC_CURVE_NIST_P224,
		.key_size = 224,
	},

	{
		.algorithm = TEE_ALG_ECDH_P256,
		.curve = TEE_ECC_CURVE_NIST_P256,
		.key_size = 256,
	},
};
