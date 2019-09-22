#pragma once

#include <tee_client_api.h>

TEEC_Result create_request(uint8_t op_code, uint32_t card_id, uint32_t value, char *write_buf, size_t *write_buf_size);
TEEC_Result process_response(uint8_t op_code, char *buf, size_t buf_size);
