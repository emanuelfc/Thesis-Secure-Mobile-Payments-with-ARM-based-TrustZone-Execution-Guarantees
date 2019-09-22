#include <tds_ta_entry.h>
#include <tee_client_api.h>
#include <trusted_data_store_ta.h>
#include <string.h>
#include <stdio.h>

static TEEC_Context tds_ctx;
static TEEC_Session tds_sess;

TEEC_Result init_tds_ta()
{
	TEEC_Result res = TEEC_ERROR_GENERIC;	
	uint32_t err_origin = 0;
		
	if((res = TEEC_InitializeContext(NULL, &tds_ctx)) == TEEC_SUCCESS)
	{
		TEEC_UUID tds_uuid = TA_TRUSTED_DATA_STORE_UUID;
		res = TEEC_OpenSession(&tds_ctx, &tds_sess, &tds_uuid, TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin);
		if(res != TEEC_SUCCESS) printf("%s:%s: Failed TEEC_OpenSession, error = %x\n", __FILE__, __func__, res);
	}
	else printf("%s:%s: Failed TEEC_InitializeContext, error = %x\n", __FILE__, __func__, res);

	return res;
}

void exit_tds_ta()
{
	TEEC_CloseSession(&tds_sess);
	TEEC_FinalizeContext(&tds_ctx);
}

TEEC_Result tds_contains_key(char *key)
{
	TEEC_Result res = TEEC_ERROR_GENERIC;	
	uint32_t err_origin = 0;

	TEEC_Operation op;	
	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);
	
	// Set key parameter
	op.params[0].tmpref.buffer = key;
	op.params[0].tmpref.size = strlen(key) + 1;

	res = TEEC_InvokeCommand(&tds_sess, TA_TDS_CONTAINS_KEY, &op, &err_origin);
	if(res != TEEC_SUCCESS) printf("%s:%s: Failed tds_contains_key, error = %x\n", __FILE__, __func__, res);

	return res;
}

TEEC_Result tds_contains_field(char *key, char *field)
{
	TEEC_Result res = TEEC_ERROR_GENERIC;	
	uint32_t err_origin = 0;

	TEEC_Operation op;	
	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT, TEEC_MEMREF_TEMP_INPUT, TEEC_NONE, TEEC_NONE);
	
	// Set key parameter
	op.params[0].tmpref.buffer = key;
	op.params[0].tmpref.size = strlen(key) + 1;

	// Set field parameter
	op.params[1].tmpref.buffer = field;
	op.params[1].tmpref.size = strlen(field) + 1;

	res = TEEC_InvokeCommand(&tds_sess, TA_TDS_CONTAINS_FIELD, &op, &err_origin);
	if(res != TEEC_SUCCESS) printf("%s:%s: Failed tds_contains_field, error = %x\n", __FILE__, __func__, res);

	return res;
}

TEEC_Result tds_get(char *key, char *field, char* buf, size_t *buf_size)
{
	TEEC_Result res = TEEC_ERROR_GENERIC;	
	uint32_t err_origin = 0;

	TEEC_Operation op;	
	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT, TEEC_MEMREF_TEMP_INPUT, TEEC_MEMREF_TEMP_OUTPUT, TEEC_NONE);
	
	// Set key parameter
	op.params[0].tmpref.buffer = key;
	op.params[0].tmpref.size = strlen(key) + 1;

	// Set field parameter
	op.params[1].tmpref.buffer = field;
	op.params[1].tmpref.size = strlen(field) + 1;

	// Set output buffer parameter
	op.params[2].tmpref.buffer = buf;
	op.params[2].tmpref.size = *buf_size;

	res = TEEC_InvokeCommand(&tds_sess, TA_TDS_GET, &op, &err_origin);
	if(res != TEEC_SUCCESS) printf("%s:%s: Failed tds_get, error = %x\n", __FILE__, __func__, res);

	*buf_size = op.params[2].tmpref.size;

	return res;
}

TEEC_Result tds_insert(char *key, char *field, char *value, size_t value_size)
{
	TEEC_Result res = TEEC_ERROR_GENERIC;	
	uint32_t err_origin = 0;

	TEEC_Operation op;	
	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT, TEEC_MEMREF_TEMP_INPUT, TEEC_MEMREF_TEMP_INPUT, TEEC_NONE);

	// Set key parameter
	op.params[0].tmpref.buffer = key;
	op.params[0].tmpref.size = strlen(key) + 1;

	// Set field parameter
	op.params[1].tmpref.buffer = field;
	op.params[1].tmpref.size = strlen(field) + 1;

	// Set value and value_size parameter
	op.params[2].tmpref.buffer = value;
	op.params[2].tmpref.size = value_size;

	res = TEEC_InvokeCommand(&tds_sess, TA_TDS_INSERT, &op, &err_origin);
	if(res != TEEC_SUCCESS) printf("%s:%s: Failed tds_insert, error = %x\n", __FILE__, __func__, res);

	return res;
}

TEEC_Result tds_delete(char *key, char *field)
{
	TEEC_Result res = TEEC_ERROR_GENERIC;	
	uint32_t err_origin = 0;	

	TEEC_Operation op;	
	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT, TEEC_MEMREF_TEMP_INPUT, TEEC_NONE, TEEC_NONE);

	// Set key parameter
	op.params[0].tmpref.buffer = key;
	op.params[0].tmpref.size = strlen(key) + 1;

	// Set field parameter
	op.params[1].tmpref.buffer = field;
	op.params[1].tmpref.size = strlen(field) + 1;

	res = TEEC_InvokeCommand(&tds_sess, TA_TDS_DELETE_FIELD, &op, &err_origin);
	if(res != TEEC_SUCCESS) printf("%s:%s: Failed tds_delete, error = %x\n", __FILE__, __func__, res);

	return res;
}



/*
long get_evaluation_time()
{
	long res = TEEC_ERROR_GENERIC;
	uint32_t err_origin = 0;
	
	TEEC_Operation op;
	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE, TEEC_NONE, TEEC_NONE, TEEC_NONE);
	
	res = TEEC_InvokeCommand(&tds_sess, 100, &op, &err_origin);
	if(res < 0) printf("%s:%s:%d: Failed TA_ENTRY evaluate, error = %x\n", __FILE__, __func__, __LINE__, res);
	
	return res;
}
*/
