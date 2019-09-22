#include <testing_utils.h>

char* create_random_length_string(const size_t len)
{
	// There is no need for using a cryptographically secure RNG algorithm
	//srand(time(NULL));
	
	char* random_str = (char*)calloc(len, sizeof(char));
	if(random_str != NULL)
	{
		for(int i = 0; i < len - 1; i++)
		{
			random_str[i] = (char)(rand() % 94 + 32); // ASCII Table
		}

		random_str[len-1] = 0; // Sanity
		
		return random_str;
	}
		
	return NULL;
}

char* create_random_bounded_string(const size_t min, const size_t max)
{
	int len = rand() % min/2 + max/2;

	return create_random_length_string(len);
}

void test_buffer(const unsigned char *buffer, const size_t buffer_size, const unsigned char *expected_buffer, const size_t expected_buffer_size)
{	
	assert(expected_buffer_size == buffer_size);
	assert(memcmp(expected_buffer, buffer, expected_buffer_size) == 0);
}
