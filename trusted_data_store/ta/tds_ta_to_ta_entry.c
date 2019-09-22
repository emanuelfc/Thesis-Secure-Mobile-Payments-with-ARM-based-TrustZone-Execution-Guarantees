#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>
#include <tds_ta_to_ta_entry.h>
#include <trusted_data_store_ta.h>
#include <string.h>

static TEE_TASessionHandle tds_session;

TEE_Result init_tds_ta()
{
	TEE_Result res = TEE_ERROR_GENERIC;
	uint32_t err_origin = 0;

	TEE_Param params[4];
	memset(&params, 0, sizeof(params));
	uint32_t paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_NONE, TEE_PARAM_TYPE_NONE, TEE_PARAM_TYPE_NONE, TEE_PARAM_TYPE_NONE);

	TEE_UUID tds_uuid = TA_TRUSTED_DATA_STORE_UUID;		
	res = TEE_OpenTASession(&tds_uuid, 0, paramTypes, params, &tds_session, &err_origin);	
	if(res != TEE_SUCCESS) DMSG("%s:%d: Failed TEEC_OpenTASession, error = %x\n", __func__, __LINE__, res);

	return res;
}

void exit_tds_ta()
{
	TEE_CloseTASession(tds_session);
}

TEE_Result tds_contains_key(char *key)
{
	TEE_Result res = TEE_ERROR_GENERIC;
	uint32_t err_origin = 0;

	TEE_Param params[4];
	memset(&params, 0, sizeof(params));
	uint32_t paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
						TEE_PARAM_TYPE_NONE,
						TEE_PARAM_TYPE_NONE,
						TEE_PARAM_TYPE_NONE);
	
	// Set key parameter
	params[0].memref.buffer = key;
	params[0].memref.size = strlen(key) + 1;

	res = TEE_InvokeTACommand(tds_session, 0, TA_TDS_CONTAINS_KEY, paramTypes, params, &err_origin);
	if(res != TEE_SUCCESS) DMSG("%s: Failed tds_contains_key, error = %x\n", __func__, res);

	return res;
}

TEE_Result tds_contains_field(char *key, char *field)
{
	TEE_Result res = TEE_ERROR_GENERIC;
	uint32_t err_origin = 0;

	TEE_Param params[4];
	memset(&params, 0, sizeof(params));
	uint32_t paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
						TEE_PARAM_TYPE_MEMREF_INPUT,
						TEE_PARAM_TYPE_NONE,
						TEE_PARAM_TYPE_NONE);
	
	// Set key parameter
	params[0].memref.buffer = key;
	params[0].memref.size = strlen(key) + 1;

	// Set field parameter
	params[1].memref.buffer = field;
	params[1].memref.size = strlen(field) + 1;

	res = TEE_InvokeTACommand(tds_session, 0, TA_TDS_CONTAINS_FIELD, paramTypes, params, &err_origin);
	if(res != TEE_SUCCESS) DMSG("%s: Failed tds_contains_field, error = %x\n", __func__, res);

	return res;
}

TEE_Result tds_get(char *key, char *field, char* buf, size_t *buf_size)
{
	TEE_Result res = TEE_ERROR_GENERIC;
	uint32_t err_origin = 0;

	TEE_Param params[4];
	memset(&params, 0, sizeof(params));
	uint32_t paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
						TEE_PARAM_TYPE_MEMREF_INPUT,
						TEE_PARAM_TYPE_MEMREF_OUTPUT,
						TEE_PARAM_TYPE_NONE);
	
	// Set key parameter
	params[0].memref.buffer = key;
	params[0].memref.size = strlen(key) + 1;

	// Set field parameter
	params[1].memref.buffer = field;
	params[1].memref.size = strlen(field) + 1;

	// Set output buffer parameter
	params[2].memref.buffer = buf;
	params[2].memref.size = *buf_size;

	res = TEE_InvokeTACommand(tds_session, 0, TA_TDS_GET, paramTypes, params, &err_origin);
	if(res != TEE_SUCCESS) DMSG("%s: Failed tds_get, error = %x\n", __func__, res);

	*buf_size = params[2].memref.size;

	return res;
}

TEE_Result tds_insert(char *key, char *field, char *value, size_t value_size)
{
	TEE_Result res = TEE_ERROR_GENERIC;
	uint32_t err_origin = 0;

	TEE_Param params[4];
	memset(&params, 0, sizeof(params));
	uint32_t paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
						TEE_PARAM_TYPE_MEMREF_INPUT,
						TEE_PARAM_TYPE_MEMREF_INPUT,
						TEE_PARAM_TYPE_NONE);

	// Set key parameter
	params[0].memref.buffer = key;
	params[0].memref.size = strlen(key) + 1;

	// Set field parameter
	params[1].memref.buffer = field;
	params[1].memref.size = strlen(field) + 1;

	// Set value and value_size parameter
	params[2].memref.buffer = value;
	params[2].memref.size = value_size;

	res = TEE_InvokeTACommand(tds_session, 0, TA_TDS_INSERT, paramTypes, params, &err_origin);
	if(res != TEE_SUCCESS) DMSG("%s: Failed tds_insert, error = %x\n", __func__, res);

	return res;
}

TEE_Result tds_delete(char *key, char *field)
{
	TEE_Result res = TEE_ERROR_GENERIC;
	uint32_t err_origin = 0;

	TEE_Param params[4];
	memset(&params, 0, sizeof(params));
	uint32_t paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
						TEE_PARAM_TYPE_MEMREF_INPUT,
						TEE_PARAM_TYPE_NONE,
						TEE_PARAM_TYPE_NONE);

	// Set key parameter
	params[0].memref.buffer = key;
	params[0].memref.size = strlen(key) + 1;

	// Set field parameter
	params[1].memref.buffer = field;
	params[1].memref.size = strlen(field) + 1;

	res = TEE_InvokeTACommand(tds_session, 0, TA_TDS_DELETE_FIELD, paramTypes, params, &err_origin);
	if(res != TEE_SUCCESS) DMSG("%s: Failed tds_delete, error = %x\n", __func__, res);

	return res;
}
