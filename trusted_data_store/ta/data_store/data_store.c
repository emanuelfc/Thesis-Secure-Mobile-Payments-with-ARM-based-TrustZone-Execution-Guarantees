#include <data_store.h>
#include <data.h>
#include <hashtable.h>
#include <hashtable_iterator.h>
#include <string.h>

data_store* new_data_store(size_t size)
{
	data_store *store = (data_store*)calloc(1, sizeof(data_store));
	if(store)
	{
		store->store = new_hashtable(size);
		if(store->store) return store;

		free(store);
	}

	return NULL;
}

data_store* create_data_store()
{
	return new_data_store(INITIAL_SIZE);
}

bool data_store_contains_key(const data_store *store, const char *key)
{
	hashtable *set = (hashtable*)hashtable_get(store->store, key);
	return set != NULL;
}

bool data_store_contains_field(const data_store *store, const char *key, const char *field)
{
	hashtable *set = (hashtable*)hashtable_get(store->store, key);
	if(set == NULL) return false;

	return (data*)hashtable_get(set, field) != NULL;
}

data* data_store_get(const data_store *store, const char *key, const char *field)
{
	hashtable *set = (hashtable*)hashtable_get(store->store, key);
	if(set == NULL) return NULL;

	return (data*)hashtable_get(set, field);
}

bool data_store_insert_new_entry(data_store *store, char *key, hashtable *ht)
{
	return hashtable_insert(store->store, key, (void*)ht, NULL);
}

bool data_store_insert(const data_store *store, const char *key, const char *field, const unsigned char *value, const size_t value_size)
{
	hashtable *set = (hashtable*)hashtable_get(store->store, key);
	// Set with key does not exist, create it
	if(set == NULL)
	{
		set = new_hashtable(INITIAL_SIZE);
		if(set)
		{
			if(!hashtable_insert(store->store, key, (void*)set, NULL))
			{
				delete_hashtable(set);
				return false;
			}
		}
	}

	data *ret_data = NULL;

	data *new_data = create_data_struct(value, value_size);
	if(hashtable_insert(set, field, (void*)new_data, (void*)&ret_data))
	{		
		if(ret_data) delete_data(ret_data);
		return true;
	}
	else
	{
		delete_data(new_data);
		return false;
	}
}

bool data_store_delete_field(const data_store *store, const char *key, const char *field)
{
	hashtable *set = (hashtable*)hashtable_get(store->store, key);
	if(set == NULL) return false;

	data *ret_data = NULL;
	bool ret_value = hashtable_delete(set, field, (void*)&ret_data);
	if(ret_data) delete_data(ret_data);
	if(set->n_entries == 0) hashtable_delete(store->store, key, NULL);

	return ret_value;
}

static void delete_set(hashtable *set)
{
	// Iterate through all the fields and delete them
	hashtable_iterator* ht_it = create_hashtable_iterator(set);
	while(has_next(ht_it))
	{
		data *field_data = (data*)(next(ht_it)->ptr_value);
		delete_data(field_data);
	}
	delete_hashtable_iterator(ht_it);

	delete_hashtable(set);
}

bool data_store_delete_key(const data_store *store, const char *key)
{
	entry *entry_set = NULL;
	if(hashtable_delete(store->store, key, (void*)&entry_set) && entry_set)
	{
		hashtable *set = (hashtable*)entry_set->ptr_value;
		delete_set(set);
		return true;
	}

	return false;
}

/*
bool data_store_delete(const data_store *store)
{
	bool res = false;
	// Iterate through all sets and delete them
	hashtable_iterator* store_it = create_hashtable_iterator(store->store);
	while(has_next(store_it))
	{
		res &= data_store_delete_key((hashtable*)(next(ht_it)->ptr_value));
	}
	delete_hashtable_iterator(store_it);

	if(res) delete_hashtable(store);

	return res;
}
*/
