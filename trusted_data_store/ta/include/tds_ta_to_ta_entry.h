#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

TEE_Result init_tds_ta();
void exit_tds_ta();

TEE_Result tds_contains_key(char *key);
TEE_Result tds_contains_field(char *key, char *field);
TEE_Result tds_get(char *key, char *field, char* buf, size_t *buf_size);
TEE_Result tds_insert(char *key, char *field, char *value, size_t value_size);
TEE_Result tds_delete(char *key, char *field);
