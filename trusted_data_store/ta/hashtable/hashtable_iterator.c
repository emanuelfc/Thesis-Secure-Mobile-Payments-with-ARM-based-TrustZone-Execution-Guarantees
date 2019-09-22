#include <hashtable_iterator.h>
#include <stdbool.h>
#include <hashtable.h>
#include <entry.h>

static entry DELETED = {NULL, NULL};

static bool is_valid_entry(entry* e)
{
	return e != NULL && e != &DELETED;
}

static void find_next(hashtable_iterator *ht_it)
{
	// Search for the next element
	ht_it->next = NULL;
	while((ht_it->index < ht_it->ht->size) && !is_valid_entry((ht_it->next = ht_it->ht->entries[ht_it->index++])));
}

hashtable_iterator* create_hashtable_iterator(hashtable *ht)
{
	hashtable_iterator *ht_it = calloc(1, sizeof(hashtable_iterator));
	if(ht_it)
	{
		ht_it->ht = ht;
		ht_it->next = NULL;
		ht_it->index = 0;
		find_next(ht_it);
	
		return ht_it;
	}

	return NULL;
}

bool has_next(hashtable_iterator *ht_it)
{
	return ht_it->next != NULL;
}

entry* next(hashtable_iterator *ht_it)
{	
	entry *e = ht_it->next;
	
	if(has_next(ht_it))
	{
		find_next(ht_it);
	}

	return e;
}

void delete_hashtable_iterator(hashtable_iterator *ht_it)
{
	free(ht_it);
}
