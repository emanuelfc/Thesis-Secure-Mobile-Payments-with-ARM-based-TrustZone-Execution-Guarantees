#pragma once

#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

#include <stdint.h>
#include <stddef.h>

TEE_Result init_snfc_ta();
void exit_snfc_ta();

TEE_Result create_snfc_packet(char *buf, size_t buf_size, char *write_buf, size_t *write_buf_size);
TEE_Result process_snfc_packet(char *payload, size_t payload_size, char *write_buf, size_t *write_buf_size);
