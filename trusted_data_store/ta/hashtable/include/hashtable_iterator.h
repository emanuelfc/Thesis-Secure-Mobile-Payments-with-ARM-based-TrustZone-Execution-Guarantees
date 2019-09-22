#pragma once

#include <stdbool.h>
#include <entry.h>
#include <hashtable.h>

typedef struct hashtable_iterator
{
	hashtable* ht;
	entry* next;
	size_t index;
} hashtable_iterator;

hashtable_iterator* create_hashtable_iterator(hashtable *ht);
bool has_next(hashtable_iterator *ht_it);
entry* next(hashtable_iterator *ht_it);
void delete_hashtable_iterator(hashtable_iterator *ht_it);


