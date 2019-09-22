#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

#include <data_store.h>
#include <tds_storage.h>

#include <data.h>
#include <entry.h>
#include <hashtable.h>
#include <hashtable_iterator.h>

#include <string.h>

static char *storage_id = "data_store_secure_storage";

// READ

static data* read_data(TEE_ObjectHandle file)
{
	uint32_t read_bytes = 0;
	
	data *d = calloc(1, sizeof(data));

	TEE_ReadObjectData(file, &d->value_size, sizeof(d->value_size), &read_bytes);

	d->value = calloc(d->value_size, sizeof(char));
	TEE_ReadObjectData(file, d->value, d->value_size, &read_bytes);

	return d;
}

static char* read_string(TEE_ObjectHandle file)
{
	uint32_t read_bytes = 0;	

	size_t len = 0;
	TEE_ReadObjectData(file, &len, sizeof(len), &read_bytes);

	char *str = calloc(len, sizeof(char));
	TEE_ReadObjectData(file, str, len, &read_bytes);

	return str;
}

static entry* read_hashtable_entry(TEE_ObjectHandle file)
{
	char *key = read_string(file);
	data *d = read_data(file);

	entry *e = create_entry(key, (void*)d);

	free(key);

	return e;
}

static hashtable* read_hashtable(TEE_ObjectHandle file)
{
	uint32_t read_bytes = 0;
	
	size_t size = 0;
	TEE_ReadObjectData(file, &size, sizeof(size), &read_bytes);

	hashtable *ht = new_hashtable(size);

	size_t n_entries = 0;
	TEE_ReadObjectData(file, &n_entries, sizeof(n_entries), &read_bytes);

	for(int i = 0; i < n_entries; i++)
	{
		entry *e = read_hashtable_entry(file);
		hashtable_insert_new_entry(ht, e);
	}

	return ht;
}

TEE_Result load_data_store(data_store **store)
{
	TEE_Result res = TEE_ERROR_GENERIC;
	
	// Open data_store file
	TEE_ObjectHandle file;
	res = TEE_OpenPersistentObject(TEE_STORAGE_PRIVATE,
					(void*)storage_id, sizeof(storage_id),
					TEE_DATA_FLAG_ACCESS_READ |
					TEE_DATA_FLAG_SHARE_READ,
					&file);

	*store = NULL;

	if(res == TEE_SUCCESS)
	{
		uint32_t read_bytes = 0;

		size_t size = 0;
		TEE_ReadObjectData(file, &size, sizeof(size), &read_bytes);

		*store = new_data_store(size);
		if(*store)
		{
			size_t n_entries = 0;
			TEE_ReadObjectData(file, &n_entries, sizeof(n_entries), &read_bytes);

			hashtable *ht = NULL;

			for(int i = 0; i < n_entries && res == TEE_SUCCESS; i++)
			{
				char *key = read_string(file);
				ht = read_hashtable(file);
				if(data_store_insert_new_entry(*store, key, ht)) res = TEE_SUCCESS;
				else res = TEE_ERROR_GENERIC;
			}
		}
		else res = TEE_ERROR_OUT_OF_MEMORY;

		TEE_CloseObject(file);
	}
	else
	{
		*store = create_data_store();
		if(*store) res = TEE_SUCCESS;
		else res = TEE_ERROR_OUT_OF_MEMORY;
	}

	return res;
}

// WRITE

static void save_data(char *buf, size_t buf_size, TEE_ObjectHandle file)
{
	TEE_WriteObjectData(file, (void*)&buf_size, sizeof(buf_size));
	TEE_WriteObjectData(file, (void*)buf, buf_size);
}

static void save_string(char *str, TEE_ObjectHandle file)
{
	save_data(str, strlen(str)+1, file);
}

static void save_hashtable_entry(entry *e, TEE_ObjectHandle file)
{
	// Save key	
	save_string(e->key, file);

	data *d = (data*)(e->ptr_value);
	save_data(d->value, d->value_size, file);
}

static void save_hashtable_metadata(hashtable *ht, TEE_ObjectHandle file)
{
	TEE_WriteObjectData(file, (void*)&ht->size, sizeof(ht->size));
	TEE_WriteObjectData(file, (void*)&ht->n_entries, sizeof(ht->n_entries));
}

static void save_data_store_entry(entry *e, TEE_ObjectHandle file)
{
	hashtable *ht = (hashtable*)e->ptr_value;

	// Save hashtable metadata
	save_hashtable_metadata(ht, file);
	
	// Save entries
	hashtable_iterator* ht_it = create_hashtable_iterator(ht);
	while(has_next(ht_it))
	{	
		save_hashtable_entry(next(ht_it), file);
	}
	delete_hashtable_iterator(ht_it);
}

TEE_Result save_data_store(data_store *store)
{
	TEE_Result res = TEE_ERROR_GENERIC;
	
	// Create data_store file
	TEE_ObjectHandle file;

	uint32_t obj_data_flag = TEE_DATA_FLAG_ACCESS_READ |		/* we can later read the oject */
				TEE_DATA_FLAG_ACCESS_WRITE |		/* we can later write into the object */
				TEE_DATA_FLAG_ACCESS_WRITE_META |	/* we can later destroy or rename the object */
				TEE_DATA_FLAG_OVERWRITE;		/* destroy existing object of same ID */

	res = TEE_CreatePersistentObject(TEE_STORAGE_PRIVATE,
					(void*)storage_id, sizeof(storage_id),
					obj_data_flag,
					TEE_HANDLE_NULL,
					NULL, 0,		/* we may not fill it right now */
					&file);

	if(res == TEE_SUCCESS)
	{
		// Save store metadata
		save_hashtable_metadata(store->store, file);

		// Save entries	
		hashtable_iterator* store_it = create_hashtable_iterator(store->store);
		while(has_next(store_it))
		{
			entry *e = next(store_it);

			// Save key
			save_string(e->key, file);
				
			save_data_store_entry(e, file);
		}
		delete_hashtable_iterator(store_it);

		TEE_CloseObject(file);
	}

	return res;
}
