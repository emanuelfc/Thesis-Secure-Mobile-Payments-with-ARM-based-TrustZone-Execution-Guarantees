#include <data_store_storage.h>
#include <data_store.h>

#include <data.h>
#include <entry.h>
#include <hashtable.h>
#include <hashtable_iterator.h>

#include <string.h>
#include <stdio.h>

// READ

static data* read_data(FILE *file)
{
	data *d = calloc(1, sizeof(data));

	fread(&d->value_size, sizeof(d->value_size), 1, file);

	d->value = calloc(d->value_size, sizeof(char));
	fread(d->value, d->value_size, sizeof(char), file);

	return d;
}

static char* read_string(FILE *file)
{
	size_t len = 0;
	fread(&len, sizeof(len), 1, file);

	char *str = calloc(len, sizeof(char));
	fread(str, len, sizeof(char), file);

	return str;
}

static entry* read_hashtable_entry(FILE *file)
{
	char *key = read_string(file);
	data *d = read_data(file);

	entry *e = create_entry(key, (void*)d);

	free(key);

	return e;
}

static hashtable* read_hashtable(FILE *file)
{
	size_t size = 0;
	fread(&size, sizeof(size), 1, file);

	hashtable *ht = new_hashtable(size);

	size_t n_entries = 0;
	fread(&n_entries, sizeof(n_entries), 1, file);

	for(int i = 0; i < n_entries; i++)
	{
		entry *e = read_hashtable_entry(file);
		hashtable_insert_new_entry(ht, e);
	}

	return ht;
}

data_store* load_data_store()
{
	// Open data_store file
	FILE *file = fopen("data_store", "r");
	if(!file) return create_data_store();

	size_t size = 0;
	fread(&size, sizeof(size), 1, file);

	data_store *store = new_data_store(size);

	size_t n_entries = 0;
	fread(&n_entries, sizeof(n_entries), 1, file);

	for(int i = 0; i < n_entries; i++)
	{
		char *key = read_string(file);
		hashtable *ht = read_hashtable(file);
		data_store_insert_new_entry(store, key, ht);
	}

	return store;
}

// WRITE

static void save_data(char *buf, size_t buf_size, FILE *file)
{
	fwrite(&buf_size, sizeof(buf_size), 1, file);
	fwrite(buf, buf_size, sizeof(char), file);
}

static void save_string(char *str, FILE *file)
{
	save_data(str, strlen(str)+1, file);
}

static void save_hashtable_entry(entry *e, FILE *file)
{
	// Save key	
	save_string(e->key, file);

	data *d = (data*)(e->ptr_value);
	save_data(d->value, d->value_size, file);
}

static void save_hashtable_metadata(hashtable *ht, FILE *file)
{
	fwrite(&ht->size, sizeof(ht->size), 1, file);
	fwrite(&ht->n_entries, sizeof(ht->n_entries), 1, file);
}

static void save_data_store_entry(entry *e, FILE *file)
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

void save_data_store(data_store *store)
{
	// Create data_store file
	FILE *file = fopen("data_store", "w+");

	// Save store metadata
	save_hashtable_metadata(store->store, file);

	// Save entries
	hashtable_iterator *store_it = create_hashtable_iterator(store->store);
	while(has_next(store_it))
	{
		entry *e = next(store_it);
				
		// Save key
		save_string(e->key, file);
			
		save_data_store_entry(e, file);
	}
	delete_hashtable_iterator(store_it);

	fclose(file);
}
