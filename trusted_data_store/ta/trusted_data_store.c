#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>
#include <trusted_data_store_ta.h>
#include <data_store.h>
#include <data.h>
#include <tds_storage.h>
#include <string.h>

data_store *store = NULL;

/*
long elapsed;

long get_evaluation_time()
{
	return elapsed;
}

static long get_elapsed_time_millis(TEE_Time *begin, TEE_Time *end)
{
	return (end->seconds - begin->seconds) * 1000 + (end->millis - begin->millis);
}


TEE_Time begin, end;
TEE_GetREETime(&begin);

TEE_GetREETime(&end);
elapsed = get_elapsed_time_millis(&begin, &end);
*/

/*
 * Called when the instance of the TA is created. This is the first call in
 * the TA.
 */
TEE_Result TA_CreateEntryPoint(void)
{	
	return load_data_store(&store);
}

/*
 * Called when the instance of the TA is destroyed if the TA has not
 * crashed or panicked. This is the last call in the TA.
 */
void TA_DestroyEntryPoint(void)
{
	if(store) save_data_store(store);
}

/*
 * Called when a new session is opened to the TA. *sess_ctx can be updated
 * with a value to be able to identify this session in subsequent calls to the
 * TA. In this function you will normally do the global initialization for the
 * TA.
 */
TEE_Result TA_OpenSessionEntryPoint(uint32_t param_types, TEE_Param __maybe_unused params[4], void __maybe_unused **sess_ctx)
{
	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE);

	if(param_types != exp_param_types) return TEE_ERROR_BAD_PARAMETERS;

	/* If return value != TEE_SUCCESS the session will not be created. */
	return TEE_SUCCESS;
}

/*
 * Called when a session is closed, sess_ctx hold the value that was
 * assigned by TA_OpenSessionEntryPoint().
 */
void TA_CloseSessionEntryPoint(void __maybe_unused *sess_ctx)
{
	(void)&sess_ctx; /* Unused parameter */
}

static TEE_Result tds_contains_key(uint32_t param_types, TEE_Param params[4])
{
	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE);

	if(exp_param_types != param_types) return TEE_ERROR_BAD_PARAMETERS;

	if(data_store_contains_key(store, params[0].memref.buffer)) return TEE_SUCCESS;
	else return TEE_ERROR_ITEM_NOT_FOUND;
}

static TEE_Result tds_contains_field(uint32_t param_types, TEE_Param params[4])
{
	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
						   TEE_PARAM_TYPE_MEMREF_INPUT,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE);

	if(exp_param_types != param_types) return TEE_ERROR_BAD_PARAMETERS;

	if(data_store_contains_field(store, params[0].memref.buffer, params[1].memref.buffer)) return TEE_SUCCESS;
	else return TEE_ERROR_ITEM_NOT_FOUND;
}

static TEE_Result tds_get(uint32_t param_types, TEE_Param params[4])
{
	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
						   TEE_PARAM_TYPE_MEMREF_INPUT,
						   TEE_PARAM_TYPE_MEMREF_OUTPUT,
						   TEE_PARAM_TYPE_NONE);

	if(exp_param_types != param_types) return TEE_ERROR_BAD_PARAMETERS;

	data *data = data_store_get(store, params[0].memref.buffer, params[1].memref.buffer);
	if(data)
	{
		if(params[2].memref.size < data->value_size) return TEE_ERROR_SHORT_BUFFER;
		
		memcpy(params[2].memref.buffer, data->value, data->value_size);
		params[2].memref.size = data->value_size;

		return TEE_SUCCESS;
	}
	return TEE_ERROR_ITEM_NOT_FOUND;
}

static TEE_Result tds_insert(uint32_t param_types, TEE_Param params[4])
{
	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
						   TEE_PARAM_TYPE_MEMREF_INPUT,
						   TEE_PARAM_TYPE_MEMREF_INPUT,
						   TEE_PARAM_TYPE_NONE);

	if(exp_param_types != param_types) return TEE_ERROR_BAD_PARAMETERS;

	if(data_store_insert(store, params[0].memref.buffer, params[1].memref.buffer, params[2].memref.buffer, params[2].memref.size))
	{
		return TEE_SUCCESS;
	}
	else return TEE_ERROR_OUT_OF_MEMORY;
}

static TEE_Result tds_delete_field(uint32_t param_types, TEE_Param params[4])
{
	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
						   TEE_PARAM_TYPE_MEMREF_INPUT,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE);

	if(exp_param_types != param_types) return TEE_ERROR_BAD_PARAMETERS;

	data_store_insert(store, "00000000000000000000000000000000", "00000000000000000000000000000000", "00000000000000000000000000000000", sizeof("00000000000000000000000000000000"));

	if(data_store_delete_field(store, "00000000000000000000000000000000", "00000000000000000000000000000000"))
	{		
		return TEE_SUCCESS;
	}
	else return TEE_ERROR_ITEM_NOT_FOUND;
}

static TEE_Result tds_delete_key(uint32_t param_types, TEE_Param params[4])
{	
	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE);

	if(exp_param_types != param_types) return TEE_ERROR_BAD_PARAMETERS;

	if(data_store_delete_key(store, params[0].memref.buffer))
	{	
		return TEE_SUCCESS;
	}
	else return TEE_ERROR_ITEM_NOT_FOUND;
}

TEE_Result TA_InvokeCommandEntryPoint(void __maybe_unused *sess_ctx, uint32_t cmd_id, uint32_t param_types, TEE_Param params[4])
{
	(void)&sess_ctx; /* Unused parameter */

	switch(cmd_id)
	{
		case TA_TDS_CONTAINS_KEY:
			return tds_contains_key(param_types, params);

		case TA_TDS_CONTAINS_FIELD:
			return tds_contains_field(param_types, params);
		
		case TA_TDS_GET:
			return tds_get(param_types, params);

		case TA_TDS_INSERT:
			return tds_insert(param_types, params);

		case TA_TDS_DELETE_FIELD:
			return tds_delete_field(param_types, params);

		case TA_TDS_DELETE_KEY:
			return tds_delete_key(param_types, params);

		default:
			return TEE_ERROR_BAD_PARAMETERS;
	}
}
