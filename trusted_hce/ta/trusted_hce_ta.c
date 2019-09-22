#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>
#include <trusted_hce_ta.h>
#include <trusted_hce.h>
#include <snfc_ta_to_ta_entry.h>
#include <tds_ta_to_ta_entry.h>

TEE_Result TA_CreateEntryPoint(void)
{
	TEE_Result res = init_tds_ta();

	if(res == TEE_SUCCESS) res = init_snfc_ta();

	return res;
}

void TA_DestroyEntryPoint(void)
{	
	exit_tds_ta();
	exit_snfc_ta();
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

static TEE_Result entry_create_request(uint32_t param_types, TEE_Param params[4])
{
	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT, 
							TEE_PARAM_TYPE_VALUE_INPUT,
							TEE_PARAM_TYPE_VALUE_INPUT,
							TEE_PARAM_TYPE_MEMREF_OUTPUT);
	if(exp_param_types != param_types) return TEE_ERROR_BAD_PARAMETERS;

	return create_request(params[0].value.a, params[1].value.a, params[2].value.a, params[3].memref.buffer, &params[3].memref.size);
}

static TEE_Result entry_process_response(uint32_t param_types, TEE_Param params[4])
{
	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT, 
							TEE_PARAM_TYPE_MEMREF_INPUT,
							TEE_PARAM_TYPE_NONE,
							TEE_PARAM_TYPE_NONE);
	if(exp_param_types != param_types) return TEE_ERROR_BAD_PARAMETERS;
	
	return process_response(params[0].value.a, params[1].memref.buffer, params[1].memref.size);
}

TEE_Result TA_InvokeCommandEntryPoint(void __maybe_unused *sess_ctx, uint32_t cmd_id, uint32_t param_types, TEE_Param params[4])
{
	(void)&sess_ctx; /* Unused parameter */

	switch(cmd_id)
	{
		case TA_THCE_CREATE_REQUEST:
			return entry_create_request(param_types, params);		

		case TA_THCE_PROCESS_RESPONSE:
			return entry_process_response(param_types, params);

		default:
			return TEE_ERROR_BAD_PARAMETERS;
	}
}
