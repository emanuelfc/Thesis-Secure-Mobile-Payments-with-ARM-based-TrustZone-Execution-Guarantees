#pragma once

#include <stdbool.h>
#include <stdlib.h>

typedef struct data
{
	unsigned char *value;
	size_t value_size;
	
} data;

bool set_data_value(data *data, const unsigned char *value, const size_t value_size);
data* create_data_struct(const unsigned char *value, const size_t value_size);
void delete_data(data *data);
