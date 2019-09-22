#pragma once

#include <stdbool.h>
#include <entry.h>
#include <stdlib.h>

#define MIN_LOAD 20
#define MAX_LOAD 60
#define RESIZE_FACTOR 2

typedef struct hashtable
{
	size_t size;
	size_t n_entries;
	entry **entries;

} hashtable;

hashtable* new_hashtable(const size_t size);
void delete_hashtable(hashtable *ht);
void hashtable_insert_new_entry(hashtable *ht, entry *entry);
bool hashtable_insert(hashtable *ht, const char *key, void *value_ptr, void **ret);
bool hashtable_delete(hashtable *ht, const char *key, void **ret);
entry* hashtable_get_entry(const hashtable *ht, const char *key);
void* hashtable_get(const hashtable *ht, const char *key);
