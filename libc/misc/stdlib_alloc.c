/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - stdlib.h
 *
 * TESTED:
 *    - malloc()
 *    - calloc()
 *    - realloc()
 *    - free()
 *
 * Copyright 2023 Phoenix Systems
 * Author: Mateusz Bloch
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <unity_fixture.h>


#define BLOCK_SIZE 32
#define LARGE_SIZE 2048

#ifndef __phoenix__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-prototypes"

const char *__asan_default_options()
{
	return "allocator_may_return_null=1";
}

#pragma GCC diagnostic pop

#endif

/*
Test group for:
malloc, calloc,
realloc, free
*/

TEST_GROUP(stdlib_alloc);


TEST_SETUP(stdlib_alloc)
{
}


TEST_TEAR_DOWN(stdlib_alloc)
{
}


TEST(stdlib_alloc, malloc_basic)
{
	void *ptr;
	ptr = malloc(BLOCK_SIZE);
	TEST_ASSERT_NOT_NULL(ptr);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(BLOCK_SIZE, malloc_usable_size(ptr));
	memset(ptr, 0x5a, BLOCK_SIZE);
	TEST_ASSERT_EACH_EQUAL_HEX8(0x5a, ptr, BLOCK_SIZE);
	free(ptr);
}


TEST(stdlib_alloc, malloc_one)
{
	void *ptr;
	ptr = malloc(1);
	TEST_ASSERT_NOT_NULL(ptr);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(1, malloc_usable_size(ptr));
	memset(ptr, 0x2d, 1);
	TEST_ASSERT_EACH_EQUAL_HEX8(0x2d, ptr, 1);
	free(ptr);
}


TEST(stdlib_alloc, malloc_large)
{
	void *ptr;
	ptr = malloc(LARGE_SIZE);
	TEST_ASSERT_NOT_NULL(ptr);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(LARGE_SIZE, malloc_usable_size(ptr));
	memset(ptr, 0xb4, LARGE_SIZE);
	TEST_ASSERT_EACH_EQUAL_HEX8(0xb4, ptr, LARGE_SIZE);
	free(ptr);
}


TEST(stdlib_alloc, malloc_multiple)
{
	void *ptr0;
	ptr0 = malloc(sizeof(int));
	TEST_ASSERT_NOT_NULL(ptr0);
	TEST_ASSERT_EQUAL_INT(0, (uintptr_t)ptr0 % sizeof(int));
	TEST_ASSERT_GREATER_OR_EQUAL_INT(sizeof(int), malloc_usable_size(ptr0));
	memset(ptr0, 0x50, sizeof(int));
	TEST_ASSERT_EACH_EQUAL_HEX8(0x50, ptr0, sizeof(int));

	void *ptr1;
	ptr1 = malloc(sizeof(float));
	TEST_ASSERT_NOT_NULL(ptr1);
	TEST_ASSERT_EQUAL_INT(0, (uintptr_t)ptr1 % sizeof(float));
	TEST_ASSERT_GREATER_OR_EQUAL_INT(sizeof(float), malloc_usable_size(ptr1));
	memset(ptr1, 0xa0, sizeof(float));
	TEST_ASSERT_EACH_EQUAL_HEX8(0xa0, ptr1, sizeof(float));

	void *ptr2;
	ptr2 = malloc(sizeof(char));
	TEST_ASSERT_NOT_NULL(ptr2);
	TEST_ASSERT_EQUAL_INT(0, (uintptr_t)ptr2 % sizeof(char));
	TEST_ASSERT_GREATER_OR_EQUAL_INT(sizeof(char), malloc_usable_size(ptr2));
	memset(ptr2, 0x28, sizeof(char));
	TEST_ASSERT_EACH_EQUAL_HEX8(0x28, ptr2, sizeof(char));

	free(ptr0);
	free(ptr1);
	free(ptr2);
}


TEST(stdlib_alloc, malloc_zero)
{
	void *ptr;
	ptr = malloc(0);
#ifdef __phoenix__
	TEST_ASSERT_NULL(ptr);
#else
	TEST_ASSERT_NOT_NULL(ptr);
#endif
	free(ptr);
}


TEST(stdlib_alloc, malloc_iterate)
{
	for (size_t s = 1; s <= LARGE_SIZE; s++) {
		void *ptr = malloc(s);
		TEST_ASSERT_NOT_NULL(ptr);
		TEST_ASSERT_GREATER_OR_EQUAL_INT(s, malloc_usable_size(ptr));
		memset(ptr, 0x5a, s);
		TEST_ASSERT_EACH_EQUAL_HEX8(0x5a, ptr, s);
		free(ptr);
	}
}


TEST(stdlib_alloc, malloc_overflow)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Walloc-size-larger-than="

	errno = 0;
	TEST_ASSERT_NULL(malloc(SIZE_MAX));
	TEST_ASSERT_EQUAL_INT(ENOMEM, errno);

#pragma GCC diagnostic pop
}


TEST(stdlib_alloc, calloc_basic)
{
	int *ptr = calloc(BLOCK_SIZE, sizeof(int));
	TEST_ASSERT_NOT_NULL(ptr);
	TEST_ASSERT_EACH_EQUAL_INT(0, ptr, BLOCK_SIZE);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(BLOCK_SIZE, malloc_usable_size(ptr));
	free(ptr);
}


TEST(stdlib_alloc, calloc_zero)
{
	int *ptr = calloc(0, sizeof(int));
#ifdef __phoenix__
	TEST_ASSERT_NULL(ptr);
#else
	TEST_ASSERT_NOT_NULL(ptr);
#endif

	int *ptr1 = calloc(BLOCK_SIZE, 0);
#ifdef __phoenix__
	TEST_ASSERT_NULL(ptr1);
#else
	TEST_ASSERT_NOT_NULL(ptr1);
#endif

	int *ptr2 = calloc(0, 0);
#ifdef __phoenix__
	TEST_ASSERT_NULL(ptr2);
#else
	TEST_ASSERT_NOT_NULL(ptr2);
#endif

	free(ptr);
	free(ptr1);
	free(ptr2);
}


TEST(stdlib_alloc, calloc_one)
{
	int *ptr = calloc(1, sizeof(int));
	TEST_ASSERT_NOT_NULL(ptr);
	TEST_ASSERT_EQUAL_INT(0, *ptr);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(sizeof(int), malloc_usable_size(ptr));
	free(ptr);
}


TEST(stdlib_alloc, calloc_large)
{
	int *ptr = calloc(LARGE_SIZE, sizeof(int));
	TEST_ASSERT_NOT_NULL(ptr);
	TEST_ASSERT_EACH_EQUAL_INT(0, ptr, LARGE_SIZE);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(sizeof(int), malloc_usable_size(ptr));
	free(ptr);
}


TEST(stdlib_alloc, calloc_iterate)
{
	for (size_t s = 1; s <= LARGE_SIZE; s++) {
		char *ptr = calloc(s, sizeof(char));
		TEST_ASSERT_NOT_NULL(ptr);
		TEST_ASSERT_EACH_EQUAL_CHAR(0, ptr, s);
		TEST_ASSERT_EQUAL_INT(0, (uintptr_t)ptr % sizeof(char));
		TEST_ASSERT_GREATER_OR_EQUAL_INT(s, malloc_usable_size(ptr));
		memset(ptr, 0x98, s);
		TEST_ASSERT_EACH_EQUAL_HEX8(0x98, ptr, s);
		free(ptr);
	}
}


TEST(stdlib_alloc, calloc_overflow)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Walloc-size-larger-than="

	errno = 0;
	TEST_ASSERT_NULL(calloc(1, SIZE_MAX));
	TEST_ASSERT_EQUAL_INT(ENOMEM, errno);

	errno = 0;
	TEST_ASSERT_NULL(calloc(SIZE_MAX, 1));
	TEST_ASSERT_EQUAL_INT(ENOMEM, errno);

	errno = 0;
	TEST_ASSERT_NULL(calloc(SIZE_MAX, SIZE_MAX));
	TEST_ASSERT_EQUAL_INT(ENOMEM, errno);

#pragma GCC diagnostic pop
}


TEST(stdlib_alloc, realloc_null)
{
	int *ptr = realloc(NULL, sizeof(int));
	TEST_ASSERT_NOT_NULL(ptr);

	int *ptr1 = realloc(NULL, 0);
	TEST_ASSERT_NOT_NULL(ptr);

	free(ptr);
	free(ptr1);
}


TEST(stdlib_alloc, realloc_zero_size)
{
	int *ptr = calloc(1, sizeof(int));
	TEST_ASSERT_NOT_NULL(ptr);
	TEST_ASSERT_EQUAL_INT(0, *ptr);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(sizeof(int), malloc_usable_size(ptr));

	int *ptr1 = realloc(ptr, 0);
	TEST_ASSERT_NULL(ptr1);
}


TEST(stdlib_alloc, realloc_calloc_resize)
{
	int *ptr = calloc(BLOCK_SIZE, sizeof(int));
	TEST_ASSERT_NOT_NULL(ptr);
	TEST_ASSERT_EACH_EQUAL_INT(0, ptr, BLOCK_SIZE);
	TEST_ASSERT_EQUAL_INT(0, (uintptr_t)ptr % sizeof(int));
	for (int i = 0; i < BLOCK_SIZE; i++) {
		ptr[i] = i;
		TEST_ASSERT_EQUAL_INT(i, ptr[i]);
	}

	int *ptr1 = realloc(ptr, BLOCK_SIZE * sizeof(int));
	TEST_ASSERT_NOT_NULL(ptr1);
	TEST_ASSERT_EQUAL_INT(0, (uintptr_t)ptr1 % sizeof(int));
	for (int i = 0; i < BLOCK_SIZE; i++) {
		TEST_ASSERT_EQUAL_INT(i, ptr1[i]);
	}

	free(ptr1);
}


TEST(stdlib_alloc, realloc_calloc_resize_smaller)
{
	int *ptr = calloc(2, 2 * sizeof(int));
	TEST_ASSERT_NOT_NULL(ptr);
	TEST_ASSERT_EACH_EQUAL_INT(0, ptr, 4);
	TEST_ASSERT_EQUAL_INT(0, (uintptr_t)ptr % sizeof(int));
	ptr[0] = 1;

	int *ptr1 = realloc(ptr, sizeof(int));
	TEST_ASSERT_NOT_NULL(ptr1);
	TEST_ASSERT_EQUAL_INT(1, ptr1[0]);
	TEST_ASSERT_EQUAL_INT(0, (uintptr_t)ptr1 % sizeof(int));

	int *ptr2 = calloc(1, 2 * LARGE_SIZE);
	TEST_ASSERT_NOT_NULL(ptr2);
	TEST_ASSERT_EACH_EQUAL_INT(0, ptr2, (2 * LARGE_SIZE) / sizeof(int));
	for (int i = 0; i < (2 * LARGE_SIZE) / sizeof(int); i++) {
		ptr2[i] = i;
		TEST_ASSERT_EQUAL_INT(i, ptr2[i]);
	}

	int *ptr3 = realloc(ptr2, LARGE_SIZE);
	TEST_ASSERT_NOT_NULL(ptr3);
	for (int i = 0; i < LARGE_SIZE / sizeof(int); i++) {
		TEST_ASSERT_EQUAL_INT(i, ptr3[i]);
	}

	free(ptr1);
	free(ptr3);
}


TEST(stdlib_alloc, realloc_calloc_resize_larger)
{
	int *ptr = calloc(1, sizeof(int));
	TEST_ASSERT_NOT_NULL(ptr);
	TEST_ASSERT_EQUAL_INT(0, *ptr);
	TEST_ASSERT_EQUAL_INT(0, (uintptr_t)ptr % sizeof(int));
	ptr[0] = 1;

	int *ptr1 = realloc(ptr, 2 * sizeof(int));
	TEST_ASSERT_NOT_NULL(ptr1);
	TEST_ASSERT_EQUAL_INT(0, (uintptr_t)ptr1 % sizeof(int));
	TEST_ASSERT_EQUAL_INT(1, ptr1[0]);

	int *ptr2 = calloc(LARGE_SIZE, sizeof(int));
	TEST_ASSERT_NOT_NULL(ptr2);
	TEST_ASSERT_EACH_EQUAL_INT(0, ptr2, LARGE_SIZE);
	TEST_ASSERT_EQUAL_INT(0, (uintptr_t)ptr2 % sizeof(int));
	for (int i = 0; i < LARGE_SIZE; i++) {
		ptr2[i] = i;
		TEST_ASSERT_EQUAL_INT(i, ptr2[i]);
	}

	int *ptr3 = realloc(ptr2, 2 * sizeof(int) * LARGE_SIZE);
	TEST_ASSERT_NOT_NULL(ptr3);
	TEST_ASSERT_EQUAL_INT(0, (uintptr_t)ptr3 % sizeof(int));
	for (int i = 0; i < LARGE_SIZE; i++) {
		TEST_ASSERT_EQUAL_INT(i, ptr3[i]);
	}
	free(ptr1);
	free(ptr3);
}


TEST(stdlib_alloc, realloc_malloc_resize)
{
	int *ptr = malloc(sizeof(int));
	TEST_ASSERT_NOT_NULL(ptr);
	memset(ptr, 0x40, sizeof(int));
	TEST_ASSERT_EQUAL_INT(0, (uintptr_t)ptr % sizeof(int));

	int *ptr1 = realloc(ptr, BLOCK_SIZE * sizeof(int));
	TEST_ASSERT_NOT_NULL(ptr1);
	TEST_ASSERT_EACH_EQUAL_HEX8(0x40, ptr1, sizeof(int));
	TEST_ASSERT_EQUAL_INT(0, (uintptr_t)ptr1 % sizeof(int));

	free(ptr1);
}


TEST(stdlib_alloc, realloc_malloc_resize_smaller)
{
	int *ptr = malloc(2 * sizeof(int));
	TEST_ASSERT_NOT_NULL(ptr);
	memset(ptr, 0x40, 2 * sizeof(int));
	TEST_ASSERT_EACH_EQUAL_HEX8(0x40, ptr, 2 * sizeof(int));

	int *ptr1 = realloc(ptr, sizeof(int));
	TEST_ASSERT_NOT_NULL(ptr1);
	TEST_ASSERT_EACH_EQUAL_HEX8(0x40, ptr1, sizeof(int));
	TEST_ASSERT_EQUAL_INT(0, (uintptr_t)ptr1 % sizeof(int));

	int *ptr2 = malloc(2 * LARGE_SIZE);
	TEST_ASSERT_NOT_NULL(ptr2);
	memset(ptr2, 0x80, 2 * LARGE_SIZE);
	TEST_ASSERT_EACH_EQUAL_HEX8(0x80, ptr2, 2 * LARGE_SIZE);

	int *ptr3 = realloc(ptr2, LARGE_SIZE);
	TEST_ASSERT_NOT_NULL(ptr3);
	TEST_ASSERT_EACH_EQUAL_HEX8(0x80, ptr3, LARGE_SIZE);

	free(ptr1);
	free(ptr3);
}


TEST(stdlib_alloc, realloc_malloc_resize_larger)
{
	int *ptr = malloc(sizeof(int));
	TEST_ASSERT_NOT_NULL(ptr);
	memset(ptr, 0x40, sizeof(int));
	TEST_ASSERT_EACH_EQUAL_HEX8(0x40, ptr, sizeof(int));

	int *ptr1 = realloc(ptr, 2 * sizeof(int));
	TEST_ASSERT_NOT_NULL(ptr1);
	TEST_ASSERT_EACH_EQUAL_HEX8(0x40, ptr1, sizeof(int));
	TEST_ASSERT_EQUAL_INT(0, (uintptr_t)ptr1 % sizeof(int));

	int *ptr2 = malloc(LARGE_SIZE);
	TEST_ASSERT_NOT_NULL(ptr2);
	memset(ptr2, 0x80, LARGE_SIZE);
	TEST_ASSERT_EACH_EQUAL_HEX8(0x80, ptr2, LARGE_SIZE);

	int *ptr3 = realloc(ptr2, 2 * LARGE_SIZE);
	TEST_ASSERT_NOT_NULL(ptr3);
	TEST_ASSERT_EACH_EQUAL_HEX8(0x80, ptr3, LARGE_SIZE);

	free(ptr1);
	free(ptr3);
}


TEST(stdlib_alloc, realloc_multiple)
{
	int *ptr = calloc(BLOCK_SIZE, BLOCK_SIZE);
	TEST_ASSERT_NOT_NULL(ptr);
	TEST_ASSERT_EACH_EQUAL_INT(0, ptr, (BLOCK_SIZE * BLOCK_SIZE) / sizeof(int));
	memset(ptr, 0x5f, BLOCK_SIZE);
	TEST_ASSERT_EACH_EQUAL_HEX8(0x5f, ptr, BLOCK_SIZE);

	int *ptr1 = realloc(ptr, BLOCK_SIZE);
	TEST_ASSERT_NOT_NULL(ptr1);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(BLOCK_SIZE, malloc_usable_size(ptr1));
	TEST_ASSERT_EACH_EQUAL_HEX8(0x5f, ptr1, BLOCK_SIZE);
	memset(ptr1, 0xbe, BLOCK_SIZE);
	TEST_ASSERT_EACH_EQUAL_HEX8(0xbe, ptr1, BLOCK_SIZE);

	int *ptr2 = realloc(ptr1, BLOCK_SIZE * 2);
	TEST_ASSERT_NOT_NULL(ptr2);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(BLOCK_SIZE * 2, malloc_usable_size(ptr2));
	TEST_ASSERT_EACH_EQUAL_HEX8(0xbe, ptr2, BLOCK_SIZE);
	memset(ptr2, 0x30, BLOCK_SIZE * 2);
	TEST_ASSERT_EACH_EQUAL_HEX8(0x30, ptr2, BLOCK_SIZE * 2);

	int *ptr3 = realloc(ptr2, BLOCK_SIZE / 2);
	TEST_ASSERT_NOT_NULL(ptr3);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(BLOCK_SIZE / 2, malloc_usable_size(ptr3));
	TEST_ASSERT_EACH_EQUAL_HEX8(0x30, ptr3, BLOCK_SIZE / 2);

	free(ptr3);
}


TEST(stdlib_alloc, realloc_overflow)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Walloc-size-larger-than="

	errno = 0;
	TEST_ASSERT_NULL(realloc(NULL, SIZE_MAX));
	TEST_ASSERT_EQUAL_INT(ENOMEM, errno);

	errno = 0;
	void *ptr = malloc(BLOCK_SIZE);
	TEST_ASSERT_NOT_NULL(ptr);
	TEST_ASSERT_NULL(realloc(ptr, SIZE_MAX));
	TEST_ASSERT_EQUAL_INT(ENOMEM, errno);

	errno = 0;
	int *ptr1 = calloc(1, BLOCK_SIZE);
	TEST_ASSERT_NOT_NULL(ptr);
	TEST_ASSERT_EACH_EQUAL_INT(0, ptr1, BLOCK_SIZE / sizeof(int));
	TEST_ASSERT_NULL(realloc(ptr, SIZE_MAX));
	TEST_ASSERT_EQUAL_INT(ENOMEM, errno);

	free(ptr);
	free(ptr1);

#pragma GCC diagnostic pop
}


TEST(stdlib_alloc, free_null)
{
	int *ptr = NULL;
	free(ptr);
	TEST_ASSERT_NULL(ptr);
}


TEST_GROUP_RUNNER(stdlib_alloc)
{
	RUN_TEST_CASE(stdlib_alloc, malloc_basic);
	RUN_TEST_CASE(stdlib_alloc, malloc_one);
	RUN_TEST_CASE(stdlib_alloc, malloc_large);
	RUN_TEST_CASE(stdlib_alloc, malloc_multiple);
	RUN_TEST_CASE(stdlib_alloc, malloc_zero);
	RUN_TEST_CASE(stdlib_alloc, malloc_iterate);
	RUN_TEST_CASE(stdlib_alloc, malloc_overflow);
	RUN_TEST_CASE(stdlib_alloc, calloc_basic);
	RUN_TEST_CASE(stdlib_alloc, calloc_zero);
	RUN_TEST_CASE(stdlib_alloc, calloc_one);
	RUN_TEST_CASE(stdlib_alloc, calloc_large);
	RUN_TEST_CASE(stdlib_alloc, calloc_iterate);
	RUN_TEST_CASE(stdlib_alloc, calloc_overflow);
	RUN_TEST_CASE(stdlib_alloc, realloc_null);
	RUN_TEST_CASE(stdlib_alloc, realloc_zero_size);
	RUN_TEST_CASE(stdlib_alloc, realloc_calloc_resize);
	RUN_TEST_CASE(stdlib_alloc, realloc_calloc_resize_smaller);
	RUN_TEST_CASE(stdlib_alloc, realloc_calloc_resize_larger);
	RUN_TEST_CASE(stdlib_alloc, realloc_malloc_resize);
	RUN_TEST_CASE(stdlib_alloc, realloc_malloc_resize_smaller);
	RUN_TEST_CASE(stdlib_alloc, realloc_malloc_resize_larger);
	RUN_TEST_CASE(stdlib_alloc, realloc_multiple);
	RUN_TEST_CASE(stdlib_alloc, realloc_overflow);
	RUN_TEST_CASE(stdlib_alloc, free_null);
}
