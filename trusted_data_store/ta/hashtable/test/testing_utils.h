#pragma once

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

char* create_random_length_string(const size_t len);
char* create_random_bounded_string(const size_t min, const size_t max);
void test_buffer(const unsigned char *buffer, const size_t buffer_size, const unsigned char *expected_buffer, const size_t expected_buffer_size);
