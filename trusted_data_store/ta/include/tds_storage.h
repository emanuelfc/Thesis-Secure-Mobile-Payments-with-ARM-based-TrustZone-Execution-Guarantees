#pragma once

#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>
#include <data_store.h>

// LOAD

TEE_Result load_data_store(data_store **store);

// SAVE

TEE_Result save_data_store(data_store *store);
