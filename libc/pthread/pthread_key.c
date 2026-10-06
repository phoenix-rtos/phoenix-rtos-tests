/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - pthread.h
 * TESTED:
 *    - pthread_key_create()
 *    - pthread_key_delete()
 *    - pthread_getspecific()
 *    - pthread_setspecific()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <pthread.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include "unity_fixture.h"


TEST_GROUP(pthread_key);


TEST_SETUP(pthread_key)
{
}


TEST_TEAR_DOWN(pthread_key)
{
}


/* pthread_setspecific/getspecific: round-trip in same thread */
TEST(pthread_key, key_setget_same_thread)
{
	pthread_key_t key;
	int data = 42;
	void *val;
	int ret;

	ret = pthread_key_create(&key, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_setspecific(key, &data);
	TEST_ASSERT_EQUAL_INT(0, ret);

	val = pthread_getspecific(key);
	TEST_ASSERT_EQUAL_PTR(&data, val);
	TEST_ASSERT_EQUAL_INT(42, *(int *)val);

	ret = pthread_key_delete(key);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


/* pthread_setspecific: different threads have independent values */
static pthread_key_t test_keyPerThread;

static void *test_keySetAndGet(void *arg)
{
	void **out = (void **)arg;
	int localData = 99;
	int ret;

	ret = pthread_setspecific(test_keyPerThread, &localData);
	if (ret != 0) {
		*out = NULL;
		return NULL;
	}

	*out = pthread_getspecific(test_keyPerThread);
	/* Keep thread alive briefly so main can check its own value */
	usleep(20000);
	return NULL;
}


TEST(pthread_key, key_per_thread_values)
{
	pthread_t thread;
	int mainData = 77;
	void *childResult = NULL;
	void *mainVal;
	int ret;

	ret = pthread_key_create(&test_keyPerThread, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_setspecific(test_keyPerThread, &mainData);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_create(&thread, NULL, test_keySetAndGet, &childResult);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* main thread's value must still be mainData */
	mainVal = pthread_getspecific(test_keyPerThread);
	TEST_ASSERT_EQUAL_PTR(&mainData, mainVal);

	ret = pthread_join(thread, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* child got its own value (not mainData) */
	TEST_ASSERT_NOT_NULL(childResult);
	TEST_ASSERT_TRUE(childResult != &mainData);

	ret = pthread_key_delete(test_keyPerThread);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


/* pthread_setspecific: overwrite value */
TEST(pthread_key, key_overwrite_value)
{
	pthread_key_t key;
	int data1 = 10;
	int data2 = 20;
	void *val;
	int ret;

	ret = pthread_key_create(&key, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_setspecific(key, &data1);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_setspecific(key, &data2);
	TEST_ASSERT_EQUAL_INT(0, ret);

	val = pthread_getspecific(key);
	TEST_ASSERT_EQUAL_PTR(&data2, val);

	ret = pthread_key_delete(key);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


/* pthread_setspecific: set NULL value */
TEST(pthread_key, key_set_null)
{
	pthread_key_t key;
	int data = 5;
	void *val;
	int ret;

	ret = pthread_key_create(&key, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_setspecific(key, &data);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_setspecific(key, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	val = pthread_getspecific(key);
	TEST_ASSERT_NULL(val);

	ret = pthread_key_delete(key);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


/* pthread_key_delete: does not call destructor */
static int test_keyDtorCalled;

static void test_keyDtor(void *arg)
{
	(void)arg;
	test_keyDtorCalled++;
}


TEST(pthread_key, key_delete_no_destructor_call)
{
	pthread_key_t key;
	int data = 1;
	int ret;

	test_keyDtorCalled = 0;

	ret = pthread_key_create(&key, test_keyDtor);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_setspecific(key, &data);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_key_delete(key);
	TEST_ASSERT_EQUAL_INT(0, ret);

	TEST_ASSERT_EQUAL_INT(0, test_keyDtorCalled);
}


/* pthread_key_create: multiple keys work independently */
TEST(pthread_key, key_multiple_keys_independent)
{
	pthread_key_t key1, key2;
	int d1 = 1, d2 = 2;
	void *val;
	int ret;

	ret = pthread_key_create(&key1, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_key_create(&key2, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_setspecific(key1, &d1);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_setspecific(key2, &d2);
	TEST_ASSERT_EQUAL_INT(0, ret);

	val = pthread_getspecific(key1);
	TEST_ASSERT_EQUAL_PTR(&d1, val);

	val = pthread_getspecific(key2);
	TEST_ASSERT_EQUAL_PTR(&d2, val);

	ret = pthread_key_delete(key1);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_key_delete(key2);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST_GROUP_RUNNER(pthread_key)
{
	RUN_TEST_CASE(pthread_key, key_setget_same_thread);
	RUN_TEST_CASE(pthread_key, key_per_thread_values);
	RUN_TEST_CASE(pthread_key, key_overwrite_value);
	RUN_TEST_CASE(pthread_key, key_set_null);
	RUN_TEST_CASE(pthread_key, key_delete_no_destructor_call);
	RUN_TEST_CASE(pthread_key, key_multiple_keys_independent);
}
