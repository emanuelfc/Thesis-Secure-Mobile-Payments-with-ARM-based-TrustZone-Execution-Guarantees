#pragma once

#include <tee_client_api.h>
#include <stddef.h>
#include <stdint.h>

TEEC_Result init_snfc_ta();
void exit_snfc_ta();

TEEC_Result set_security_parameters(uint8_t spi, uint8_t kei);
TEEC_Result create_handshake_request(uint8_t spi, uint8_t kei, uint8_t flags, char *buf, size_t *buf_size);
TEEC_Result process_handshake_request(char *buf, size_t buf_size, char *reply_buf, size_t *reply_buf_size);
TEEC_Result process_handshake_reply(uint8_t flags, char *buf, size_t buf_size, char *write_buf, size_t *write_buf_size);
TEEC_Result process_authentication_proof(char *buf, size_t buf_size);

TEEC_Result create_snfc_packet(char *buf, size_t buf_size, char *write_buf, size_t *write_buf_size);
TEEC_Result process_snfc_packet(char *payload, size_t payload_size, char *write_buf, size_t *write_buf_size);
TEEC_Result security_context_close();
