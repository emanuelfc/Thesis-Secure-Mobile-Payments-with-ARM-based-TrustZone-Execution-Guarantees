#pragma once

#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>
#include <snfc_ta_context.h>

typedef struct snfc_packet
{
	uint8_t seq_n;
	char iv_check_value;
	char *payload;
	char *icv;

} snfc_packet;

TEE_Result create_snfc_packet(security_context *ctx, char *buf, size_t buf_size, char *write_buf, size_t *write_buf_size);

TEE_Result process_snfc_packet(security_context *ctx, char *packet, size_t packet_size, char *write_buf, size_t *write_buf_size);
