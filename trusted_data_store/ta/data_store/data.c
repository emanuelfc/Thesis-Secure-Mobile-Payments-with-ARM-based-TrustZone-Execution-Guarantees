#include <data.h>
#include <string.h>
#include <stdbool.h>

bool set_data_value(data *data, const unsigned char *value, const size_t value_size)
{
	unsigned char *new_value = (unsigned char*)realloc(data->value, value_size);
	if(new_value != NULL)
	{
		memcpy(new_value, value, value_size);
		data->value = new_value;
		data->value_size = value_size;
		
		return true;
	}
	
	return false;
}

data* create_data_struct(const unsigned char *value, const size_t value_size)
{
	// Create entry
	data *new_data_struct = (data*)calloc(1, sizeof(data));
	if(new_data_struct != NULL)
	{
		if(set_data_value(new_data_struct, value, value_size))
		{
			return new_data_struct;
		}

		free(new_data_struct);
	}
	
	return NULL;
}

void delete_data(data *data)
{
	free(data->value);
	free(data);
}
