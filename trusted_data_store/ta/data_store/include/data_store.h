#pragma once

#include <data_store.h>
#include <data.h>
#include <hashtable.h>
#include <stdbool.h>
#include <stdlib.h>

#define INITIAL_SIZE 15

typedef struct data_store
{
	hashtable *store;

} data_store;

data_store* create_data_store();
data_store* new_data_store(size_t size);
bool data_store_contains_key(const data_store *store, const char *key);
bool data_store_contains_field(const data_store *store, const char *key, const char *field);
data* data_store_get(const data_store *store, const char *key, const char *field);
bool data_store_insert_new_entry(data_store *store, char *key, hashtable *ht);
bool data_store_insert(const data_store *store, const char *key, const char *field, const unsigned char *value, const size_t value_size);
bool data_store_delete_field(const data_store *store, const char *key, const char *field);
bool data_store_delete_key(const data_store *store, const char *key);
bool data_store_delete(const data_store *store);
