/*

Credit for the inspiration (namely implementing it using open addressing
which was unknown to me at the time, and hash functions for the hashtable)
 of the implementation goes to: (links below)

https://gist.github.com/tonious/1377667
https://github.com/jamesroutley/write-a-hash-table

Other credits are placed before the code snippets
*/


#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <hashtable.h>
#include <entry.h>
#include <prime.h>

static entry DELETED = {NULL, NULL};

static size_t basic_hash(const char *key, size_t size)
{
	/*unsigned long long hash = 0;
	size_t i = 0;

	// source: https://gist.github.com/tonious/1377667
	// Convert our string to an integer
	while(hash < ULONG_MAX && i < strlen(key))
	{
		hash = hash << 8;
		hash += key[i];
		i++;
	}

	return hash % size;*/

	// The hash function below was created by: modified version of Fowler–Noll–Vo hash function (could not find the original source upon subsequent searches for the source)
	uint32_t hash = 2166136261ul;
	const uint8_t *bytes = (uint8_t *)key;

	while (*bytes != 0) {
		hash ^= *bytes;
		hash = hash * 0x01000193;
		bytes++;
	}

	return hash % size;
}

// Double hashing
static size_t hashtable_hash(const char *str, size_t size, size_t attempt)
{
	//const size_t hash_a = basic_hash(str, size);
	//const size_t hash_b = basic_hash(str, size);

	//return (hash_a + (attempt * (hash_b + 1)) % size);
	return (basic_hash(str, size) + attempt) % size;
}

hashtable* new_hashtable(const size_t size)
{
	// Structure should already be zeroed out
	hashtable *new_hashtable = (hashtable*)calloc(1, sizeof(hashtable));
	if(new_hashtable)
	{
		// Structure should already be zeroed out
		new_hashtable->size = next_prime(size);
		new_hashtable->entries = (entry**)calloc(new_hashtable->size, sizeof(entry*));
		if(new_hashtable->entries)
		{
			return new_hashtable;
		}
		
		free(new_hashtable);
	}
	
	return NULL;
}

void delete_hashtable(hashtable *ht)
{
	for(size_t i = 0; i < ht->size; i++)
	{
		if(ht->entries[i]) delete_entry(ht->entries[i]);
	}
	
	free(ht->entries);
	free(ht);
}

void hashtable_insert_new_entry(hashtable *ht, entry *entry)
{
	// Implementation uses open addressing
	// Search place to insert new entry, handle collisions if encountered
	size_t index = 0;
	size_t i = 0;
	do
	{
		index = hashtable_hash(entry->key, ht->size, i);
		i++;
	}
	while(ht->entries[index] && ht->entries[index] != &DELETED);
	
	// Insert entry
	ht->entries[index] = entry;
	ht->n_entries++;
}

static bool hashtable_resize(hashtable *ht, const size_t new_size)
{	
	hashtable *resized_ht = new_hashtable(new_size);
	if(resized_ht)
	{
		for(size_t i = 0; i < ht->size; i++)
		{
			entry *entry = ht->entries[i];
			if(entry != NULL && entry != &DELETED)
			{			
				hashtable_insert_new_entry(resized_ht, entry);
			}
		}

		free(ht->entries);

		ht->entries = resized_ht->entries;
		ht->n_entries = resized_ht->n_entries;
		ht->size = resized_ht->size;

		free(resized_ht);

		return true;
	}
	
	return false;
}

static bool hashtable_resize_up(hashtable *ht)
{
	return hashtable_resize(ht, next_prime(ht->size * RESIZE_FACTOR));
}

static bool hashtable_resize_down(hashtable *ht)
{
	return hashtable_resize(ht, next_prime(ht->size / RESIZE_FACTOR));
}

bool hashtable_insert(hashtable *ht, const char *key, void *ptr_value, void **ret)
{
	// Adjust hashtable size	
	const unsigned int load = (ht->n_entries * 100) / ht->size;
	if(load > MAX_LOAD)
	{
		if(!hashtable_resize_up(ht))
		{
			// Failed to resize, cannot perform operation			
			return false;
		}
	}

	// Search for already existing entry or an index to store the new entry
	entry *existing_entry = hashtable_get_entry(ht, key);
	if(existing_entry)
	{
		if(ret) *ret = existing_entry->ptr_value;
		existing_entry->ptr_value = ptr_value;
		return true;
	}

	if(ret) *ret = NULL;
	
	entry *new_entry = create_entry(key, ptr_value);
	if(new_entry)
	{
		hashtable_insert_new_entry(ht, new_entry);
		return true;
	}

	return false;
}

static int hashtable_get_entry_index(const hashtable *ht, const char *key)
{
	size_t i = 0;
	size_t index = hashtable_hash(key, ht->size, i);
	entry *cur_entry = ht->entries[index];
	while(cur_entry != NULL)
	{	
		if(cur_entry != &DELETED && (strcmp(cur_entry->key, key) == 0))
		{
			return index;
		}
		
		i++;
		index = hashtable_hash(key, ht->size, i);
		cur_entry = ht->entries[index];
	}

	return -1;
}

bool hashtable_delete(hashtable *ht, const char *key, void **ret)
{
	// Adjust hashtable size	
	const unsigned int load = (ht->n_entries * 100) / ht->size;
	if(load < MIN_LOAD)
	{
		if(!hashtable_resize_down(ht))
		{
			// Failed to resize, cannot perform operation
			return false;
		}
	}

	if(ht->n_entries > 0)
	{		
		int index = hashtable_get_entry_index(ht, key);
		if(index != -1)
		{		
			if(ret) *ret = ht->entries[index]->ptr_value;
			delete_entry(ht->entries[index]);
			ht->entries[index] = &DELETED;
			ht->n_entries--;
			return true;
		}
	}

	return false;
}

entry* hashtable_get_entry(const hashtable *ht, const char *key)
{
	if(ht->n_entries > 0)
	{
		int index = hashtable_get_entry_index(ht, key);
		if(index != -1) return ht->entries[index];
	}

	return NULL;
}

void* hashtable_get(const hashtable *ht, const char *key)
{
	entry* entry = hashtable_get_entry(ht, key);
	return entry ? entry->ptr_value : NULL;
}
