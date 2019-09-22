#pragma once

typedef struct entry
{
	char *key;
	void *ptr_value;
	
} entry;

entry* create_entry(const char *key, void *value);

// Releases entry's memory, does not destroy its value
void delete_entry(entry *entry);
