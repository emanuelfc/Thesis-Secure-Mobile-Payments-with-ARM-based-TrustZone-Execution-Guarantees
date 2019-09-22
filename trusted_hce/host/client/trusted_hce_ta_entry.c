#include <tee_client_api.h>
#include <trusted_hce_ta.h>
#include <trusted_hce_ta_entry.h>
#include <stdio.h>
#include <string.h>

static TEEC_Context thce_ctx;
static TEEC_Session thce_session;

TEEC_Result init_thce_ta()
{
	TEEC_Result res = TEEC_ERROR_GENERIC;
	uint32_t err_origin = 0;	
	if((res = TEEC_InitializeContext(NULL, &thce_ctx)) == TEEC_SUCCESS)
	{
		TEEC_UUID thce_uuid = TA_THCE_UUID;		
		res = TEEC_OpenSession(&thce_ctx, &thce_session, &thce_uuid, TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin);	
		if(res != TEEC_SUCCESS) printf("%s:%s:%d: Failed TEEC_OpenSession, error = %x\n", __FILE__, __func__, __LINE__, res);
	}
	else printf("%s:%s:%d: Failed TEEC_InitializeContext, error = %x\n", __FILE__, __func__, __LINE__, res);

	return res;
}

void exit_thce_ta()
{
	TEEC_CloseSession(&thce_session);
	TEEC_FinalizeContext(&thce_ctx);
}

TEEC_Result create_request(uint8_t op_code, uint32_t card_id, uint32_t value, char *write_buf, size_t *write_buf_size)
{
	TEEC_Result res = TEEC_ERROR_GENERIC;
	uint32_t err_origin = 0;

	TEEC_Operation op;
	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_VALUE_INPUT, TEEC_VALUE_INPUT, TEEC_MEMREF_TEMP_OUTPUT);

	op.params[0].value.a = op_code;

	op.params[1].value.a = card_id;

	op.params[2].value.a = value;

	op.params[3].tmpref.buffer = write_buf;
	op.params[3].tmpref.size = *write_buf_size;
	
	res = TEEC_InvokeCommand(&thce_session, TA_THCE_CREATE_REQUEST, &op, &err_origin);
	if(res != TEEC_SUCCESS) printf("%s:%s:%d: Failed TA_ENTRY create_request, error = %x\n", __FILE__, __func__, __LINE__, res);

	*write_buf_size = op.params[3].tmpref.size;

	return res;
}

TEEC_Result process_response(uint8_t op_code, char *buf, size_t buf_size)
{
	TEEC_Result res = TEEC_ERROR_GENERIC;
	uint32_t err_origin = 0;

	TEEC_Operation op;
	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_MEMREF_TEMP_INPUT, TEEC_NONE, TEEC_NONE);

	op.params[0].value.a = op_code;

	op.params[1].tmpref.buffer = buf;
	op.params[1].tmpref.size = buf_size;
	
	res = TEEC_InvokeCommand(&thce_session, TA_THCE_PROCESS_RESPONSE, &op, &err_origin);
	if(res != TEEC_SUCCESS) printf("%s:%s:%d: Failed TA_ENTRY process_response, error = %x\n", __FILE__, __func__, __LINE__, res);
	
	return res;
}
