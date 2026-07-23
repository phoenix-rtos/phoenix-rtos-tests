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


/* pthread_key_create: shall return 0 and store key */
TEST(pthread_key, key_create_success)
{
	pthread_key_t key;
	int ret;

	ret = pthread_key_create(&key, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1642 issue");
#else
	ret = pthread_key_delete(key);
	TEST_ASSERT_EQUAL_INT(0, ret);
#endif
}


/* pthread_key_create: newly created key has NULL value in calling thread */
TEST(pthread_key, key_initial_value_null)
{
	pthread_key_t key;
	void *val;
	int ret;

	ret = pthread_key_create(&key, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	val = pthread_getspecific(key);
	TEST_ASSERT_NULL(val);

#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1642 issue");
#else
	ret = pthread_key_delete(key);
	TEST_ASSERT_EQUAL_INT(0, ret);
#endif
}


/* pthread_key_create: newly created key has NULL value in new thread */
static pthread_key_t test_keyNewThread;

static void *test_keyCheckNull(void *arg)
{
	void **out = (void **)arg;
	*out = pthread_getspecific(test_keyNewThread);
	return NULL;
}


TEST(pthread_key, key_initial_value_null_in_new_thread)
{
	pthread_t thread;
	void *childVal = (void *)1;
	int ret;

	ret = pthread_key_create(&test_keyNewThread, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_create(&thread, NULL, test_keyCheckNull, &childVal);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_join(thread, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	TEST_ASSERT_NULL(childVal);

#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1642 issue");
#else
	ret = pthread_key_delete(test_keyNewThread);
	TEST_ASSERT_EQUAL_INT(0, ret);
#endif
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


/* pthread_key_delete: shall return 0 */
TEST(pthread_key, key_delete_success)
{
	pthread_key_t key;
	int ret;

	ret = pthread_key_create(&key, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1642 issue");
#else
	ret = pthread_key_delete(key);
	TEST_ASSERT_EQUAL_INT(0, ret);
#endif
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


/* pthread_key_create: destructor called at thread exit for non-NULL value */
static int test_keyDtorExitCalled;
static pthread_key_t test_keyDtorExit;

static void test_keyDtorExit_fn(void *arg)
{
	(void)arg;
	test_keyDtorExitCalled++;
}


static void *test_keyDtorExitThread(void *arg)
{
	int *data = (int *)arg;

	pthread_setspecific(test_keyDtorExit, data);
	return NULL;
}


TEST(pthread_key, key_destructor_called_on_thread_exit)
{
	pthread_t thread;
	int data = 42;
	int ret;

	test_keyDtorExitCalled = 0;

	ret = pthread_key_create(&test_keyDtorExit, test_keyDtorExit_fn);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_create(&thread, NULL, test_keyDtorExitThread, &data);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_join(thread, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	TEST_ASSERT_EQUAL_INT(1, test_keyDtorExitCalled);

#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1642 issue");
#else
	ret = pthread_key_delete(test_keyDtorExit);
	TEST_ASSERT_EQUAL_INT(0, ret);
#endif
}


/* pthread_key_create: destructor NOT called if value is NULL at thread exit */
static int test_keyDtorNullCalled;
static pthread_key_t test_keyDtorNull;

static void test_keyDtorNull_fn(void *arg)
{
	(void)arg;
	test_keyDtorNullCalled++;
}


static void *test_keyDtorNullThread(void *arg)
{
	(void)arg;
	/* Value remains NULL — destructor should not be called */
	return NULL;
}


TEST(pthread_key, key_destructor_not_called_for_null)
{
	pthread_t thread;
	int ret;

	test_keyDtorNullCalled = 0;

	ret = pthread_key_create(&test_keyDtorNull, test_keyDtorNull_fn);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_create(&thread, NULL, test_keyDtorNullThread, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_join(thread, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	TEST_ASSERT_EQUAL_INT(0, test_keyDtorNullCalled);

#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1642 issue");
#else
	ret = pthread_key_delete(test_keyDtorNull);
	TEST_ASSERT_EQUAL_INT(0, ret);
#endif
}


/* pthread_key_create: destructor receives previously associated value */
static void *test_keyDtorReceivedArg;
static pthread_key_t test_keyDtorArg;

static void test_keyDtorArg_fn(void *arg)
{
	test_keyDtorReceivedArg = arg;
}


static void *test_keyDtorArgThread(void *arg)
{
	pthread_setspecific(test_keyDtorArg, arg);
	return NULL;
}


TEST(pthread_key, key_destructor_receives_value)
{
	pthread_t thread;
	int data = 77;
	int ret;

	test_keyDtorReceivedArg = NULL;

	ret = pthread_key_create(&test_keyDtorArg, test_keyDtorArg_fn);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_create(&thread, NULL, test_keyDtorArgThread, &data);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_join(thread, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	TEST_ASSERT_EQUAL_PTR(&data, test_keyDtorReceivedArg);

#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1642 issue");
#else
	ret = pthread_key_delete(test_keyDtorArg);
	TEST_ASSERT_EQUAL_INT(0, ret);
#endif
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
	RUN_TEST_CASE(pthread_key, key_create_success);
	RUN_TEST_CASE(pthread_key, key_initial_value_null);
	RUN_TEST_CASE(pthread_key, key_initial_value_null_in_new_thread);
	RUN_TEST_CASE(pthread_key, key_setget_same_thread);
	RUN_TEST_CASE(pthread_key, key_per_thread_values);
	RUN_TEST_CASE(pthread_key, key_overwrite_value);
	RUN_TEST_CASE(pthread_key, key_set_null);
	RUN_TEST_CASE(pthread_key, key_delete_success);
	RUN_TEST_CASE(pthread_key, key_delete_no_destructor_call);
	RUN_TEST_CASE(pthread_key, key_destructor_called_on_thread_exit);
	RUN_TEST_CASE(pthread_key, key_destructor_not_called_for_null);
	RUN_TEST_CASE(pthread_key, key_destructor_receives_value);
	RUN_TEST_CASE(pthread_key, key_multiple_keys_independent);
}
