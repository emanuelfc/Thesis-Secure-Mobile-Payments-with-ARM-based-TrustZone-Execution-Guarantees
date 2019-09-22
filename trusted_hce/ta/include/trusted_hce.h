#pragma once

#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

TEE_Result create_request(uint8_t op_code, uint32_t card_id, uint32_t value, char *write_buf, size_t *write_buf_size);
TEE_Result process_response(uint8_t op_code, char *response, size_t response_size);
