#include <snfc_ta_to_ta_entry.h>
#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>
#include <snfc_ta.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>

static TEE_TASessionHandle snfc_session;

TEE_Result init_snfc_ta()
{
	TEE_Result res = TEE_ERROR_GENERIC;
	uint32_t err_origin = 0;

	TEE_Param params[4];
	memset(&params, 0, sizeof(params));
	uint32_t paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_NONE, TEE_PARAM_TYPE_NONE, TEE_PARAM_TYPE_NONE, TEE_PARAM_TYPE_NONE);

	TEE_UUID snfc_uuid = TA_SNFC_UUID;		
	res = TEE_OpenTASession(&snfc_uuid, 0, paramTypes, params, &snfc_session, &err_origin);	
	if(res != TEE_SUCCESS) DMSG("%s:%d: Failed TEEC_OpenTASession, error = %x\n", __func__, __LINE__, res);

	return res;
}

void exit_snfc_ta()
{
	TEE_CloseTASession(snfc_session);
}

TEE_Result create_snfc_packet(char *buf, size_t buf_size, char *write_buf, size_t *write_buf_size)
{
	TEE_Result res = TEE_ERROR_GENERIC;
	uint32_t err_origin = 0;

	TEE_Param params[2];
	memset(&params, 0, sizeof(params));
	uint32_t paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT, TEE_PARAM_TYPE_MEMREF_OUTPUT, TEE_PARAM_TYPE_NONE, TEE_PARAM_TYPE_NONE);

	params[0].memref.buffer = buf;
	params[0].memref.size = buf_size;

	params[1].memref.buffer = write_buf;
	params[1].memref.size = *write_buf_size;
	
	res = TEE_InvokeTACommand(snfc_session, 0, TA_SNFC_CREATE_SNFC_PACKET, paramTypes, params, &err_origin);
	if(res != TEE_SUCCESS) DMSG("%s:%d: Failed TA_TO_TA_ENTRY create_snfc_packet, error = %x\n", __func__, __LINE__, res);
	
	*write_buf_size = params[1].memref.size;
	
	return res;
}

TEE_Result process_snfc_packet(char *payload, size_t payload_size, char *write_buf, size_t *write_buf_size)
{
	TEE_Result res = TEE_ERROR_GENERIC;
	uint32_t err_origin = 0;

	TEE_Param params[2];
	memset(&params, 0, sizeof(params));
	uint32_t paramTypes = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT, TEE_PARAM_TYPE_MEMREF_OUTPUT, TEE_PARAM_TYPE_NONE, TEE_PARAM_TYPE_NONE);

	params[0].memref.buffer = payload;
	params[0].memref.size = payload_size;

	params[1].memref.buffer = write_buf;
	params[1].memref.size = *write_buf_size;
	
	res = TEE_InvokeTACommand(snfc_session, 0, TA_SNFC_PROCESS_SNFC_PACKET, paramTypes, params, &err_origin);
	if(res != TEE_SUCCESS) DMSG("%s:%d: Failed TA_TO_TA_ENTRY process_snfc_packet, error = %x\n", __func__, __LINE__, res);
	
	*write_buf_size = params[1].memref.size;
	
	return res;
}
