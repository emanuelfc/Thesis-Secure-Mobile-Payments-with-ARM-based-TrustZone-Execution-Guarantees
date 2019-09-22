#include <entry.h>
#include <string.h>
#include <stdlib.h>

entry* create_entry(const char *key, void *ptr_value)
{
	// Create entry
	entry *new_entry = (entry*)calloc(1, sizeof(entry));
	if(new_entry != NULL)
	{
		new_entry->key = strdup(key);
		if(new_entry->key != NULL)
		{
			new_entry->ptr_value = ptr_value;
			return new_entry;
		}
		
		free(new_entry);
	}
	
	return NULL;
}

void delete_entry(entry *entry)
{
	free(entry->key);
	free(entry);
}
