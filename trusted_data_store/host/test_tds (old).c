// Standard libraries
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <stdbool.h>

// OP-TEE specific libraries
#include <tee_client_api.h>
#include <secure_data_store_ta.h>

// My libraries
#include <data.h>

#define N_TESTS 50

TEEC_Context ctx;
TEEC_Session sess;
TEEC_UUID uuid = TA_TRUSTED_DATA_STORE_UUID;	
TEEC_Result res;
uint32_t err_origin;
TEEC_Operation op;

void init()
{
	srand(time(NULL));
}

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

char* create_random_string()
{
	int len = rand() % 25 + 25;

	return create_random_length_string(len);
}

void test_buffer(const unsigned char *buffer, const size_t buffer_size, const unsigned char *expected_buffer, const size_t expected_buffer_size)
{	
	assert(expected_buffer_size == buffer_size);
	assert(memcmp(expected_buffer, buffer, expected_buffer_size) == 0);
}

void set_ta_get_params(char *key, char *field, unsigned char* out_buffer, const size_t out_buffer_size)
{
	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT, TEEC_MEMREF_TEMP_INPUT, TEEC_MEMREF_TEMP_OUTPUT, TEEC_NONE);
	
	// Set key parameter
	op.params[0].tmpref.buffer = key;
	op.params[0].tmpref.size = strlen(key) + 1;

	// Set field parameter
	op.params[1].tmpref.buffer = field;
	op.params[1].tmpref.size = strlen(field) + 1;

	// Set output buffer parameter
	op.params[2].tmpref.buffer = out_buffer;
	op.params[2].tmpref.size = out_buffer_size;
}

void test_get(char *key, char *field, unsigned char *expected_value, size_t expected_value_size)
{
	unsigned char* out_buffer = (unsigned char*)calloc(expected_value_size, sizeof(unsigned char));	
	set_ta_get_params(key, field, out_buffer, expected_value_size);

	res = TEEC_InvokeCommand(&sess, TA_TDS_GET, &op, &err_origin);
	if(res != TEEC_SUCCESS)
	{
		errx(1, "TEEC_InvokeCommand %d failed with code 0x%x origin 0x%x", TA_TDS_GET, res, err_origin);
	}

	// Tes value
	test_buffer(out_buffer, op.params[2].tmpref.size, expected_value, expected_value_size);

	free(out_buffer);
}

void set_ta_insert_params(char *key, char *field, unsigned char *value, size_t value_size)
{
	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT, TEEC_MEMREF_TEMP_INPUT, TEEC_MEMREF_TEMP_INPUT, TEEC_NONE);

	// Set key parameter
	op.params[0].tmpref.buffer = key;
	op.params[0].tmpref.size = strlen(key) + 1;

	// Set field parameter
	op.params[1].tmpref.buffer = field;
	op.params[1].tmpref.size = strlen(field) + 1;

	// Set value and value_size parameter
	op.params[2].tmpref.buffer = value;
	op.params[2].tmpref.size = value_size;
}

void test_insert(char *key, char *field, unsigned char *value, const size_t value_size)
{
	set_ta_insert_params(key, field, value, value_size);

	res = TEEC_InvokeCommand(&sess, TA_TDS_INSERT, &op, &err_origin);
	if(res != TEEC_SUCCESS)
	{
		errx(1, "TEEC_InvokeCommand %d failed with code 0x%x origin 0x%x", TA_TDS_INSERT, res, err_origin);
	}

	test_get(key, field, value, value_size);
}

void set_ta_delete_field_params(char *key, char *field)
{
	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT, TEEC_MEMREF_TEMP_INPUT, TEEC_NONE, TEEC_NONE);

	// Set key parameter
	op.params[0].tmpref.buffer = key;
	op.params[0].tmpref.size = strlen(key) + 1;

	// Set field parameter
	op.params[1].tmpref.buffer = field;
	op.params[1].tmpref.size = strlen(field) + 1;
}

void test_delete_field(char *key, char *field)
{
	set_ta_delete_field_params(key, field);

	res = TEEC_InvokeCommand(&sess, TA_TDS_DELETE_FIELD, &op, &err_origin);
	if(res != TEEC_SUCCESS)
	{
		errx(1, "TEEC_InvokeCommand %d failed with code 0x%x origin 0x%x", TA_TDS_DELETE_FIELD, res, err_origin);
	}

	res = TEEC_InvokeCommand(&sess, TA_TDS_DELETE_FIELD, &op, &err_origin);
	if(res != TEEC_ERROR_GENERIC)
	{
		errx(1, "TEEC_InvokeCommand %d failed with code 0x%x origin 0x%x", TA_TDS_DELETE_FIELD, res, err_origin);
	}
}

data* create_random_data_struct()
{
	char* random_bytes = create_random_string();
	assert(random_bytes);
	data* data = create_data_struct((unsigned char*)random_bytes, strlen(random_bytes)+1);
	assert(data);
	return data;
}

typedef struct struct_test_field
{
	char *field;
	data *data;
} struct_test_field;

void delete_test_field(struct_test_field* test_field)
{
	free(test_field->field);
	delete_data(test_field->data);
	free(test_field);
}

struct_test_field* create_random_test_field()
{
	struct_test_field *t = (struct_test_field*)calloc(1, sizeof(struct_test_field));
	assert(t);
	if(t)
	{	
		t->field = create_random_string();
		assert(t->field);
		t->data = create_random_data_struct();
		assert(t->data);
		return t;
	}

	return NULL;
}

void test_set(char *key)
{
	struct_test_field **random_fields = (struct_test_field*)calloc(N_TESTS, sizeof(struct_test_field*));

	// Add random fields with random data
	for(int i = 0; i < N_TESTS; i++)
	{
		random_fields[i] = create_random_test_field();
		test_insert(key, random_fields[i]->field, random_fields[i]->data->value, random_fields[i]->data->value_size);
	}

	// Delete all fields
	for(int i = 0; i < N_TESTS; i++)
	{
		test_delete_field(key, random_fields[i]->field);
		delete_test_field(random_fields[i]);
	}

	free(random_fields);
}

void init_ta()
{
	res = TEEC_InitializeContext(NULL, &ctx);
	if(res != TEEC_SUCCESS)
	{
		errx(1, "TEEC_InitializeContext failed with code 0x%x", res);
	}

	printf("[i] Context with SDS TA initialized\n");

	res = TEEC_OpenSession(&ctx, &sess, &uuid, TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin);
	if(res != TEEC_SUCCESS)
	{
		errx(1, "TEEC_Opensession failed with code 0x%x origin 0x%x", res, err_origin);
	}

	printf("[i] Session opened with SDS TA\n");
}

void exit_ta()
{
	printf("[i] Closing Session and Context\n");

	TEEC_CloseSession(&sess);
	TEEC_FinalizeContext(&ctx);

	printf("[i] Closed Session and Context\n");
}

int main(int agrc, char* argv[])
{
	init_ta();
	
	init();

	char *random_data = create_random_length_string(10);

	test_insert("key1", "field1", (unsigned char*)random_data, strlen(random_data)+1);
	test_delete_field("key1", "field1");

	printf("Testing set operations\n");
	test_set("key1");
	printf("Passed\n");

	exit_ta();
	
	return 0;
}
