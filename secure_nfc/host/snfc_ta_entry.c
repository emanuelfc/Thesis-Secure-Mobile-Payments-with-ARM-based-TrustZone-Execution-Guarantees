#include <snfc_ta_entry.h>
#include <tee_client_api.h>
#include <snfc_ta.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

static TEEC_Context snfc_ctx;
static TEEC_Session snfc_session;

TEEC_Result init_snfc_ta()
{
	TEEC_Result res = TEEC_ERROR_GENERIC;
	uint32_t err_origin = 0;	
	if((res = TEEC_InitializeContext(NULL, &snfc_ctx)) == TEEC_SUCCESS)
	{
		TEEC_UUID snfc_uuid = TA_SNFC_UUID;		
		res = TEEC_OpenSession(&snfc_ctx, &snfc_session, &snfc_uuid, TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin);	
		if(res != TEEC_SUCCESS) printf("%s:%s:%d: Failed TEEC_OpenSession, error = %x\n", __FILE__, __func__, __LINE__, res);
	}
	else printf("%s:%s:%d: Failed TEEC_InitializeContext, error = %x\n", __FILE__, __func__, __LINE__, res);

	return res;
}

void exit_snfc_ta()
{
	TEEC_CloseSession(&snfc_session);
	TEEC_FinalizeContext(&snfc_ctx);
}

TEEC_Result set_security_parameters(uint8_t spi, uint8_t kei)
{
	TEEC_Result res = TEEC_ERROR_GENERIC;	
	uint32_t err_origin = 0;
	
	TEEC_Operation op;
	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_VALUE_INPUT, TEEC_NONE, TEEC_NONE);

	op.params[0].value.a = spi;
	op.params[1].value.a = kei;
	
	res = TEEC_InvokeCommand(&snfc_session, TA_SNFC_SET_SECURITY_PARAMETERS, &op, &err_origin);
	if(res != TEEC_SUCCESS) printf("%s:%s:%d: Failed TA_ENTRY set_security_parameters, error = %x\n", __FILE__, __func__, __LINE__, res);
	
	return res;
}

TEEC_Result create_handshake_request(uint8_t spi, uint8_t kei, uint8_t flags, char *buf, size_t *buf_size)
{
	TEEC_Result res = TEEC_ERROR_GENERIC;
	uint32_t err_origin = 0;
	
	TEEC_Operation op;
	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_VALUE_INPUT, TEEC_VALUE_INPUT, TEEC_MEMREF_TEMP_INOUT);

	op.params[0].value.a = spi;
	op.params[1].value.a = kei;
	op.params[2].value.a = flags;

	op.params[3].tmpref.buffer = buf;
	op.params[3].tmpref.size = *buf_size;
	
	res = TEEC_InvokeCommand(&snfc_session, TA_SNFC_CREATE_HANDSHAKE_REQUEST, &op, &err_origin);
	if(res != TEEC_SUCCESS) printf("%s:%s:%d: Failed TA_ENTRY create_handshake_request, error = %x\n", __FILE__, __func__, __LINE__, res);
	
	*buf_size = op.params[3].tmpref.size;
	
	return res;
}

TEEC_Result process_handshake_request(char *buf, size_t buf_size, char *reply_buf, size_t *reply_buf_size)
{
	TEEC_Result res = TEEC_ERROR_GENERIC;
	uint32_t err_origin = 0;
	
	TEEC_Operation op;
	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT, TEEC_MEMREF_TEMP_OUTPUT, TEEC_NONE, TEEC_NONE);

	op.params[0].tmpref.buffer = buf;
	op.params[0].tmpref.size = buf_size;

	op.params[1].tmpref.buffer = reply_buf;
	op.params[1].tmpref.size = *reply_buf_size;
	
	res = TEEC_InvokeCommand(&snfc_session, TA_SNFC_PROCESS_HANDSHAKE_REQUEST, &op, &err_origin);
	if(res != TEEC_SUCCESS) printf("%s:%s:%d: Failed TA_ENTRY process_handshake_request, error = %x\n", __FILE__, __func__, __LINE__, res);
	
	*reply_buf_size = op.params[1].tmpref.size;
	
	return res;
}

TEEC_Result process_handshake_reply(uint8_t flags, char *buf, size_t buf_size, char *write_buf, size_t *write_buf_size)
{
	TEEC_Result res = TEEC_ERROR_GENERIC;
	uint32_t err_origin = 0;
	
	TEEC_Operation op;
	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_MEMREF_TEMP_INPUT, TEEC_MEMREF_TEMP_OUTPUT, TEEC_NONE);

	op.params[0].value.a = flags;

	op.params[1].tmpref.buffer = buf;
	op.params[1].tmpref.size = buf_size;

	op.params[2].tmpref.buffer = write_buf;
	op.params[2].tmpref.size = *write_buf_size;
	
	res = TEEC_InvokeCommand(&snfc_session, TA_SNFC_PROCESS_HANDSHAKE_REPLY, &op, &err_origin);
	if(res != TEEC_SUCCESS) printf("%s:%s:%d: Failed TA_ENTRY process_handshake_reply, error = %x\n", __FILE__, __func__, __LINE__, res);
	
	*write_buf_size = op.params[2].tmpref.size;
	
	return res;
}

TEEC_Result process_authentication_proof(char *buf, size_t buf_size)
{
	TEEC_Result res = TEEC_ERROR_GENERIC;
	uint32_t err_origin = 0;
	
	TEEC_Operation op;

	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);

	op.params[0].tmpref.buffer = buf;
	op.params[0].tmpref.size = buf_size;
	
	res = TEEC_InvokeCommand(&snfc_session, TA_SNFC_PROCESS_AUTHENTICATION_PROOF, &op, &err_origin);
	if(res != TEEC_SUCCESS) printf("%s:%s:%d: Failed TA_ENTRY process_authentication_proof, error = %x\n", __FILE__, __func__, __LINE__, res);
	
	return res;
}

TEEC_Result create_snfc_packet(char *buf, size_t buf_size, char *write_buf, size_t *write_buf_size)
{
	TEEC_Result res = TEEC_ERROR_GENERIC;
	uint32_t err_origin = 0;
	
	TEEC_Operation op;
	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT, TEEC_MEMREF_TEMP_OUTPUT, TEEC_NONE, TEEC_NONE);

	op.params[0].tmpref.buffer = buf;
	op.params[0].tmpref.size = buf_size;

	op.params[1].tmpref.buffer = write_buf;
	op.params[1].tmpref.size = *write_buf_size;
	
	res = TEEC_InvokeCommand(&snfc_session, TA_SNFC_CREATE_SNFC_PACKET, &op, &err_origin);
	if(res != TEEC_SUCCESS) printf("%s:%s:%d: Failed TA_ENTRY create_communication_packet, error = %x\n", __FILE__, __func__, __LINE__, res);
	
	*write_buf_size = op.params[1].tmpref.size;
	
	return res;
}

TEEC_Result process_snfc_packet(char *payload, size_t payload_size, char *write_buf, size_t *write_buf_size)
{
	TEEC_Result res = TEEC_ERROR_GENERIC;
	uint32_t err_origin = 0;

	TEEC_Operation op;
	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT, TEEC_MEMREF_TEMP_OUTPUT, TEEC_NONE, TEEC_NONE);

	op.params[0].tmpref.buffer = payload;
	op.params[0].tmpref.size = payload_size;

	op.params[1].tmpref.buffer = write_buf;
	op.params[1].tmpref.size = *write_buf_size;
	
	res = TEEC_InvokeCommand(&snfc_session, TA_SNFC_PROCESS_SNFC_PACKET, &op, &err_origin);
	if(res != TEEC_SUCCESS) printf("%s:%s:%d: Failed TA_ENTRY process_communication_packet, error = %x\n", __FILE__, __func__, __LINE__, res);
	
	*write_buf_size = op.params[1].tmpref.size;
	
	return res;
}

TEEC_Result security_context_close()
{
	TEEC_Result res = TEEC_ERROR_GENERIC;
	uint32_t err_origin = 0;
	
	TEEC_Operation op;
	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE, TEEC_NONE, TEEC_NONE, TEEC_NONE);
	
	res = TEEC_InvokeCommand(&snfc_session, TA_SNFC_SECURITY_CONTEXT_CLOSE, &op, &err_origin);
	if(res != TEEC_SUCCESS) printf("%s:%s:%d: Failed TA_ENTRY security_context_close, error = %x\n", __FILE__, __func__, __LINE__, res);
	
	return res;
}
