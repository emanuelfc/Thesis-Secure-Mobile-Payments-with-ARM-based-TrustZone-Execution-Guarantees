#pragma once

#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>
#include <snfc_ta_context.h>

typedef struct handshake_request
{
	uint8_t spi;
	uint8_t kei;
	uint8_t flags;
	
} handshake_request;

void security_context_close(security_context *ctx);

TEE_Result set_security_parameters(security_context *ctx, uint8_t spi, uint8_t kei);

TEE_Result create_handshake_request(security_context *ctx, uint8_t spi, uint8_t kei, uint8_t flags, char *buf, size_t *buf_size);

TEE_Result process_handshake_request(security_context *ctx, char *buf, size_t buf_size, char *write_buf, size_t *write_buf_size);

TEE_Result process_handshake_reply(security_context *ctx, uint8_t flags, char *buf, size_t buf_size, char *write_buf, size_t *write_buf_size);
