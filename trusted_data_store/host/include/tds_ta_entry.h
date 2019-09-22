#pragma once

#include <tee_client_api.h>

TEEC_Result init_tds_ta();
void exit_tds_ta();

TEEC_Result tds_contains_key(char *key);
TEEC_Result tds_contains_field(char *key, char *field);
TEEC_Result tds_get(char *key, char *field, char* buf, size_t *buf_size);
TEEC_Result tds_insert(char *key, char *field, char *value, size_t value_size);
TEEC_Result tds_delete(char *key, char *field);
