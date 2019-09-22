// Standard libraries
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <stdbool.h>

#include <tds_ta_entry.h>
#include <tee_client_api.h>

static int key_size = 32;
static int field_size = 32;
static int data_size = 32;

char* create_random_length_string(const size_t len)
{
	char* random_str = (char*)calloc(len, sizeof(char));
	if(random_str != NULL)
	{
		for(int i = 0; i < len - 1; i++)
		{	
			random_str[i] = (char)(rand() % 94 + 32); // ASCII Table
		}

		random_str[len] = 0; // Sanity
		
		return random_str;
	}
		
	return NULL;
}

static char* create_key(int i)
{
	char *str = calloc(key_size, sizeof(char));
	memset(str, 1, key_size);
	snprintf(str, key_size, "%032d", i);
	return str;
}

#define N_RUNS 50

const int NANOS_IN_MILLIS = 1000000;
static long nano_to_millis(long nano)
{
	return nano / NANOS_IN_MILLIS;
}

const int SECS_IN_MILLIS = 1000;
static long secs_to_milli(long secs)
{
	return secs * SECS_IN_MILLIS;
}

const int NANOS_IN_SECS = 1000000000;
static long secs_to_nano(long secs)
{
	return secs * NANOS_IN_SECS;
}

static long get_elapsed_time_nanos(struct timespec begin, struct timespec end)
{
	return secs_to_nano(end.tv_sec - begin.tv_sec) + (end.tv_nsec - begin.tv_nsec);
}

static void evaluate_read_write(int n)
{
	int res = 0;
	
	struct timespec begin, end;
	long elapsed = 0;

	size_t buf_size = 64;
	char *buf = calloc(buf_size, sizeof(char));
	size_t out_size = 0;

	char *key = create_key(0);
	char *field = key;

	// Reads
	long total_read_time = 0;
	for(int i = 0; i < n; i++)
	{
		out_size = buf_size;

		clock_gettime(CLOCK_MONOTONIC, &begin);
		if((res = tds_get(key, field, buf, &out_size)) != TEEC_SUCCESS)
		{
			printf("%s:%d: Failed evaluate_read_write - tds_get, error = %x, dec = %d\n", __func__, __LINE__, res, res);
			exit(-1);
		}
		clock_gettime(CLOCK_MONOTONIC, &end);
		elapsed = get_elapsed_time_nanos(begin, end);
		total_read_time += elapsed;
	}

	// Data that will be used in the bulk write test
	char *test_value = calloc(data_size, sizeof(char));
	memset(test_value, 2, data_size);

	// Write (or rather update)
	long total_write_time = 0;
	for(int i = 0; i < 100 - n; i++)
	{
		clock_gettime(CLOCK_MONOTONIC, &begin);		
		if((res = tds_insert(key, field, test_value, data_size)) != TEEC_SUCCESS)
		{
			printf("%s:%d: Failed evaluate_read_write - tds_insert, error = %x, dec = %d\n", __func__, __LINE__, res, res);
			exit(-1);
		}
		clock_gettime(CLOCK_MONOTONIC, &end);
		elapsed = get_elapsed_time_nanos(begin, end);
		total_write_time += elapsed;
	}
	printf("%ld, %ld\n", total_read_time, total_write_time);
}

int main(int argc, char **argv)
{
	int res = 0;

	if((res = init_tds_ta()) != TEEC_SUCCESS)
	{
		printf("%s:%d: Failed init_tds_ta, error = %x, dec = %d\n", __func__, __LINE__, res, res);
		exit(-1);
	}

	size_t buf_size = 1024;
	char *buf = calloc(buf_size, sizeof(char));
	size_t out_size = 0;

	long seed = 0;
	srand(time(seed));

	// Generate initial data
	char *data = create_random_length_string(data_size);

	srand(time(seed));

	struct timespec begin, end;
	long elapsed = 0;

	srand(time(seed));
	// tds_insert - New Field New Key
	elapsed = 0;
	printf("tds_insert - New Field New Key\n");
	for(int i = 0; i < N_RUNS; i++)
	{
		char *key = create_key(i);

		clock_gettime(CLOCK_MONOTONIC, &begin);
		if((res = tds_insert(key, key, data, data_size)) != TEEC_SUCCESS)
		{
			printf("%s:%d: Failed tds_insert, error = %x, dec = %d\n", __func__, __LINE__, res, res);
			exit(-1);
		}
		clock_gettime(CLOCK_MONOTONIC, &end);
		elapsed = get_elapsed_time_nanos(begin, end);
		printf("%ld\n", elapsed);

		free(key);
	}
/*
	srand(time(seed));
	// tds_contains_key
	printf("tds_contains_key\n");
	for(int i = 0; i < N_RUNS; i++)
	{
		char *key = create_key(i);

		clock_gettime(CLOCK_MONOTONIC, &begin);
		if((res = tds_contains_key(key)) != TEEC_SUCCESS)
		{
			printf("%s:%d: Failed tds_contains_key, error = %x, dec = %d\n", __func__, __LINE__, res, res);
			exit(-1);
		}
		clock_gettime(CLOCK_MONOTONIC, &end);
		elapsed = get_elapsed_time_nanos(begin, end);
		printf("%ld\n", elapsed);

		free(key);
	}

	srand(time(seed));
	// tds_contains_field
	printf("tds_contains_field\n");
	for(int i = 0; i < N_RUNS; i++)
	{
		char *key = create_key(0);

		clock_gettime(CLOCK_MONOTONIC, &begin);
		if((res = tds_contains_field(key, key)) != TEEC_SUCCESS)
		{
			printf("%s:%d: Failed tds_contains_field, error = %x, dec = %d\n", __func__, __LINE__, res, res);
			exit(-1);
		}
		clock_gettime(CLOCK_MONOTONIC, &end);
		elapsed = get_elapsed_time_nanos(begin, end);
		printf("%ld\n", elapsed);

		free(key);
	}

	srand(time(seed));
	// tds_get
	printf("tds_get\n");
	for(int i = 0; i < N_RUNS; i++)
	{
		char *key = create_key(0);
		out_size = buf_size;

		clock_gettime(CLOCK_MONOTONIC, &begin);
		if((res = tds_get(key, key, buf, &out_size)) != TEEC_SUCCESS)
		{
			printf("%s:%d: Failed tds_get, error = %x, dec = %d\n", __func__, __LINE__, res, res);
			exit(-1);
		}
		clock_gettime(CLOCK_MONOTONIC, &end);
		elapsed = get_elapsed_time_nanos(begin, end);
		printf("%ld\n", elapsed);

		free(key);
	}
*/
	srand(time(seed));
	// Bulk read/write evaluation
	printf("Bulk Read/Write Evaluation\n");
	for(int i = 0; i <= 100; i +=10)
	{
		printf("Reads = %d, Writes = %d\n", i, 100-i);		
		for(int j = 0; j < N_RUNS; j++)
		{
			evaluate_read_write(i);
		}
	}
/*
	// Save and Load
	printf("Save and Load (%d entries)\n", N_RUNS);
	long total_save_time = 0;
	long total_load_time = 0;
	for(int i = 0; i < N_RUNS; i++)
	{
		clock_gettime(CLOCK_MONOTONIC, &begin);
		exit_tds_ta();
		clock_gettime(CLOCK_MONOTONIC, &end);
		total_save_time = get_elapsed_time_nanos(begin, end);

		clock_gettime(CLOCK_MONOTONIC, &begin);
		if((res = init_tds_ta()) != TEEC_SUCCESS)
		{
			printf("%s:%d: Failed init_tds_ta, error = %x, dec = %d\n", __func__, __LINE__, res, res);
			exit(-1);
		}
		clock_gettime(CLOCK_MONOTONIC, &end);
		total_load_time = get_elapsed_time_nanos(begin, end);

		printf("%ld, %ld\n", total_save_time, total_load_time);
	}

	srand(time(seed));
	// tds_insert - New Field Existing key
	printf("tds_insert - New Field Existing Key\n");
	char *key = create_key(0);
	for(int i = 0; i < N_RUNS; i++)
	{
		char *field = create_key(i+1);

		clock_gettime(CLOCK_MONOTONIC, &begin);
		if((res = tds_insert(key, field, data, data_size)) != TEEC_SUCCESS)
		{
			printf("%s:%d: Failed tds_insert, error = %x, dec = %d\n", __func__, __LINE__, res, res);
			exit(-1);
		}
		clock_gettime(CLOCK_MONOTONIC, &end);
		elapsed = get_elapsed_time_nanos(begin, end);
		printf("%ld\n", elapsed);

		free(field);
	}
	free(key);

	srand(time(seed));
	// tds_insert - Existing Field Existing key (update)
	printf("tds_insert - Existing Field Existing key (update)\n");
	for(int i = 0; i < N_RUNS; i++)
	{
		char *key = create_key(i);

		clock_gettime(CLOCK_MONOTONIC, &begin);
		if((res = tds_insert(key, key, data, data_size)) != TEEC_SUCCESS)
		{
			printf("%s:%d: Failed tds_insert, error = %x, dec = %d\n", __func__, __LINE__, res, res);
			exit(-1);
		}
		clock_gettime(CLOCK_MONOTONIC, &end);
		elapsed = get_elapsed_time_nanos(begin, end);
		printf("%ld\n", elapsed);

		free(key);
	}

	srand(time(seed));
	// Delete field from key with multiple fields
	printf("tds_delete - Key with Multiple Fields\n");
	for(int i = 0; i < N_RUNS; i++)
	{
		char *key = create_key(0);

		if((res = tds_insert(key, key, data, data_size)) != TEEC_SUCCESS)
		{
			printf("%s:%d: Failed tds_insert, error = %x, dec = %d\n", __func__, __LINE__, res, res);
			exit(-1);
		}
		clock_gettime(CLOCK_MONOTONIC, &begin);
		if((res = tds_delete(key, key)) != TEEC_SUCCESS)
		{
			printf("%s:%d: Failed tds_delete, error = %x, dec = %d\n", __func__, __LINE__, res, res);
			exit(-1);
		}
		clock_gettime(CLOCK_MONOTONIC, &end);
		elapsed = get_elapsed_time_nanos(begin, end);
		printf("%ld\n", elapsed);

		free(key);
	}

	srand(time(seed));
	// Delete field from key with a single field
	printf("tds_delete - Key with Single Field\n");
	for(int i = 0; i < N_RUNS; i++)
	{
		char *key = create_key(i);

		if((res = tds_insert(key, key, data, data_size)) != TEEC_SUCCESS)
		{
			printf("%s:%d: Failed tds_insert, error = %x, dec = %d\n", __func__, __LINE__, res, res);
			exit(-1);
		}
		clock_gettime(CLOCK_MONOTONIC, &begin);
		if((res = tds_delete(key, key)) != TEEC_SUCCESS)
		{
			printf("%s:%d: Failed tds_delete, error = %x, dec = %d\n", __func__, __LINE__, res, res);
			exit(-1);
		}
		clock_gettime(CLOCK_MONOTONIC, &end);
		elapsed = get_elapsed_time_nanos(begin, end);
		printf("%ld\n", elapsed);


		free(key);
	}
*/

	//exit_tds_ta();

	/*
	printf("clock_gettime w CLOCK_MONOTONIC overhead\n");
	for(int i = 0; i < 1000000; i++)
	{
		clock_gettime(CLOCK_MONOTONIC, &begin);
		clock_gettime(CLOCK_MONOTONIC, &end);
		elapsed += get_elapsed_time_nanos(begin, end);
	}
	printf("Total time = %ld\n", elapsed);
	printf("Overhead = %lf\n", (elapsed*1.0f) / (1000000*1.0f));
	*/

	return 0;
}
