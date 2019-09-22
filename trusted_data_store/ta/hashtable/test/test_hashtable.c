#include <stdlib.h>
#include <stdio.h>
#include <testing_utils.h>
#include <hashtable.h>

#define INITIAL_SIZE 15

hashtable* test_ht;

void test_new_hashtable()
{
	test_ht = new_hashtable(INITIAL_SIZE);

	assert(test_ht);
	assert(test_ht->size >= INITIAL_SIZE);
	assert(test_ht->n_entries == 0);
	assert(test_ht->entries);
}

void test_hashtable_get(const char *key, const void* expected_ptr_value)
{
	// Retrieving the element should succeed, and be the same
	void* ret = hashtable_get(test_ht, key);
	assert(ret);
	assert(ret == expected_ptr_value);
}

void test_hashtable_insert(const char *key, void* ptr_value)
{
	void* ret = NULL;

	size_t old_n_entries = test_ht->n_entries;

	// Inserting should succeed
	printf("%s\n", key);
	assert(hashtable_insert(test_ht, key, ptr_value, &ret));
	assert(!ret);
	assert(test_ht->n_entries == (old_n_entries + 1));

	// Retrieving the element should succeed, and be the same
	test_hashtable_get(key, ptr_value);
}

void test_hashtable_delete(const char* key)
{
	void* ret = NULL;

	size_t old_n_entries = test_ht->n_entries;

	// Deleting should succeed	
	assert(hashtable_delete(test_ht, key, &ret));
	assert(ret);
	assert(test_ht->n_entries == (old_n_entries - 1));

	ret = NULL;

	// Deleting again should fail
	assert(!hashtable_delete(test_ht, key, &ret));
	assert(!ret);
	assert(test_ht->n_entries == (old_n_entries - 1));
}

void test_hashtable_resize_up(char** test_random_keys, const size_t n_test_random_keys)
{
	size_t old_size = test_ht->size;
	
	for(int i = 0; i < n_test_random_keys; i++)
	{
		test_hashtable_insert(test_random_keys[i], (void*)test_random_keys[i]);
	}

	assert(old_size < test_ht->size);
}

void test_hashtable_resize_down(char** test_random_keys, const size_t n_test_random_keys)
{
	size_t old_size = test_ht->size;
	
	for(int i = 0; i < n_test_random_keys; i++)
	{
		test_hashtable_delete(test_random_keys[i]);
	}

	assert(old_size > test_ht->size);
}

void test_hashtable_resizes()
{
	size_t n_test_random_keys = test_ht->size;
	char** test_random_keys = calloc(n_test_random_keys, sizeof(char*));

	for(int i = 0; i < n_test_random_keys; i++)
	{
		test_random_keys[i] = create_random_bounded_string(50, 75);
	}

	printf("Testing resize_up\n");
	test_hashtable_resize_up(test_random_keys, n_test_random_keys);
	printf("Passed\n");
	printf("Testing resize_down\n");
	test_hashtable_resize_down(test_random_keys, n_test_random_keys);
	printf("Passed\n");
}


int main(int argc, char** argv)
{
	test_new_hashtable();

	srand(time(NULL));

	char* test_random_key = create_random_length_string(25);
	printf("Testing insert\n");
	test_hashtable_insert(test_random_key, (void*)test_random_key);
	printf("Passed\n");
	printf("Testing delete\n");
	test_hashtable_delete(test_random_key);
	printf("Passed\n");
	free(test_random_key);
	printf("Testing resizes\n");
	test_hashtable_resizes();
	printf("Passed\n");

	return 0;
}
