#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>
#include <snfc_ta_handshake.h>
#include <snfc_ta_authentication.h>
#include <snfc_ta_communication.h>
#include <snfc_ta.h>

security_context ctx;

TEE_Result TA_CreateEntryPoint(void)
{
	return TEE_SUCCESS;
}

void TA_DestroyEntryPoint(void)
{	
	security_context_close(&ctx);
}

TEE_Result TA_OpenSessionEntryPoint(uint32_t param_types, TEE_Param __maybe_unused params[4], void __maybe_unused **sess_ctx)
{
	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_NONE,
							TEE_PARAM_TYPE_NONE,
							TEE_PARAM_TYPE_NONE,
							TEE_PARAM_TYPE_NONE);

	if(param_types != exp_param_types) return TEE_ERROR_BAD_PARAMETERS;

	return TEE_SUCCESS;
}

void TA_CloseSessionEntryPoint(void __maybe_unused *sess_ctx)
{
	(void)&sess_ctx;
}

static TEE_Result entry_set_security_parameters(uint32_t param_types, TEE_Param params[4])
{
	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT, 
							TEE_PARAM_TYPE_VALUE_INPUT,
							TEE_PARAM_TYPE_NONE,
							TEE_PARAM_TYPE_NONE);
	if(exp_param_types != param_types) return TEE_ERROR_BAD_PARAMETERS;

	return set_security_parameters(&ctx, (uint8_t)params[0].value.a, (uint8_t)params[1].value.a);
}

static TEE_Result entry_create_handshake_request(uint32_t param_types, TEE_Param params[4])
{
	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT, 
							TEE_PARAM_TYPE_VALUE_INPUT,
							TEE_PARAM_TYPE_VALUE_INPUT,
							TEE_PARAM_TYPE_MEMREF_INOUT);
	if(exp_param_types != param_types) return TEE_ERROR_BAD_PARAMETERS;
	
	return create_handshake_request(&ctx, params[0].value.a, params[1].value.a, params[2].value.a, params[3].memref.buffer, &params[3].memref.size);
}

static TEE_Result entry_process_handshake_request(uint32_t param_types, TEE_Param params[4])
{
	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
							TEE_PARAM_TYPE_MEMREF_OUTPUT,
							TEE_PARAM_TYPE_NONE,
							TEE_PARAM_TYPE_NONE);
	if(exp_param_types != param_types) return TEE_ERROR_BAD_PARAMETERS;

	return process_handshake_request(&ctx, params[0].memref.buffer, params[0].memref.size, params[1].memref.buffer, &params[1].memref.size);
}

static TEE_Result entry_process_handshake_reply(uint32_t param_types, TEE_Param params[4])
{
	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
							TEE_PARAM_TYPE_MEMREF_INPUT,
							TEE_PARAM_TYPE_MEMREF_OUTPUT,
							TEE_PARAM_TYPE_NONE);
	if(exp_param_types != param_types) return TEE_ERROR_BAD_PARAMETERS;

	return process_handshake_reply(&ctx, params[0].value.a, params[1].memref.buffer, params[1].memref.size, params[2].memref.buffer, &params[2].memref.size);
}

static TEE_Result entry_process_authentication_proof(uint32_t param_types, TEE_Param params[4])
{
	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
							TEE_PARAM_TYPE_NONE,
							TEE_PARAM_TYPE_NONE,
							TEE_PARAM_TYPE_NONE);
	if(exp_param_types != param_types) return TEE_ERROR_BAD_PARAMETERS;

	return process_authentication_proof(&ctx, params[0].memref.buffer);
}

static TEE_Result entry_create_snfc_packet(uint32_t param_types, TEE_Param params[4])
{
	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT, 
							TEE_PARAM_TYPE_MEMREF_OUTPUT,
							TEE_PARAM_TYPE_NONE,
							TEE_PARAM_TYPE_NONE);
	if(exp_param_types != param_types) return TEE_ERROR_BAD_PARAMETERS;
	
	return create_snfc_packet(&ctx, params[0].memref.buffer, params[0].memref.size, params[1].memref.buffer, &params[1].memref.size);
}

static TEE_Result entry_process_snfc_packet(uint32_t param_types, TEE_Param params[4])
{
	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT, 
							TEE_PARAM_TYPE_MEMREF_OUTPUT,
							TEE_PARAM_TYPE_NONE,
							TEE_PARAM_TYPE_NONE);
	if(exp_param_types != param_types) return TEE_ERROR_BAD_PARAMETERS;
	
	return process_snfc_packet(&ctx, params[0].memref.buffer, params[0].memref.size, params[1].memref.buffer, &params[1].memref.size);
}

static TEE_Result entry_security_context_close(uint32_t param_types, TEE_Param params[4])
{
	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_NONE, 
							TEE_PARAM_TYPE_NONE,
							TEE_PARAM_TYPE_NONE,
							TEE_PARAM_TYPE_NONE);
	if(exp_param_types != param_types) return TEE_ERROR_BAD_PARAMETERS;
	
	security_context_close(&ctx);

	return TEE_SUCCESS;
}

TEE_Result TA_InvokeCommandEntryPoint(void __maybe_unused *sess_ctx, uint32_t cmd_id, uint32_t param_types, TEE_Param params[4])
{
	(void)&sess_ctx; /* Unused parameter */

	switch(cmd_id)
	{
		case TA_SNFC_SET_SECURITY_PARAMETERS:
			return entry_set_security_parameters(param_types, params);	

		case TA_SNFC_CREATE_HANDSHAKE_REQUEST:
			return entry_create_handshake_request(param_types, params);
	
		case TA_SNFC_PROCESS_HANDSHAKE_REQUEST:
			return entry_process_handshake_request(param_types, params);

		case TA_SNFC_PROCESS_HANDSHAKE_REPLY:
			return entry_process_handshake_reply(param_types, params);

		case TA_SNFC_PROCESS_AUTHENTICATION_PROOF:
			return entry_process_authentication_proof(param_types, params);
	
		case TA_SNFC_CREATE_SNFC_PACKET:
			return entry_create_snfc_packet(param_types, params);

		case TA_SNFC_PROCESS_SNFC_PACKET:
			return entry_process_snfc_packet(param_types, params);

		case TA_SNFC_SECURITY_CONTEXT_CLOSE:
			return entry_security_context_close(param_types, params);

		default:
			return TEE_ERROR_BAD_PARAMETERS;
	}
}
