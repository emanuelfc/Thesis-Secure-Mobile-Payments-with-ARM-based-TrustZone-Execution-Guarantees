#pragma once

#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>
#include <snfc_ta_context.h>

typedef struct authentication_id
{
	char *pub_key;
	char *ca_signature;

} authentication_id;

TEE_Result create_authentication_id(char *buf, size_t *write_bytes);
TEE_Result process_authentication_id(security_context *ctx, char *buf);

TEE_Result create_authentication_proof(security_context *ctx, char *kep, char *write_buf, size_t *write_bytes);
TEE_Result process_authentication_proof(security_context *ctx, char *buf);
