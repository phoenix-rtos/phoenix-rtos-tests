/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - pthread.h
 * TESTED:
 *    - pthread_self()
 *    - pthread_equal()
 *    - pthread_exit()
 *    - pthread_join()
 *    - pthread_detach()
 *    - pthread_once()
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
#include <unistd.h>

#include "unity_fixture.h"


/* ===== pthread_lifecycle group ===== */
/* Tests: pthread_self, pthread_equal, pthread_exit, pthread_join, pthread_detach */


TEST_GROUP(pthread_lifecycle);


TEST_SETUP(pthread_lifecycle)
{
}


TEST_TEAR_DOWN(pthread_lifecycle)
{
}


/* pthread_self: shall return the thread ID of the calling thread (always succeeds) */
TEST(pthread_lifecycle, self_returns_valid_id)
{
	pthread_t id;

	id = pthread_self();
	/* pthread_equal with itself must be non-zero */
	TEST_ASSERT_TRUE(pthread_equal(id, id) != 0);
}


/* pthread_equal: same thread IDs shall return non-zero */
TEST(pthread_lifecycle, equal_same_thread)
{
	pthread_t id1, id2;

	id1 = pthread_self();
	id2 = pthread_self();

	TEST_ASSERT_TRUE(pthread_equal(id1, id2) != 0);
}


/* pthread_equal: different thread IDs shall return zero */
static void *test_lifecycleGetId(void *arg)
{
	pthread_t *out = (pthread_t *)arg;
	*out = pthread_self();
	return NULL;
}


TEST(pthread_lifecycle, equal_different_threads)
{
	pthread_t mainId, childId;
	pthread_t thread;
	int ret;

	mainId = pthread_self();

	ret = pthread_create(&thread, NULL, test_lifecycleGetId, &childId);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_join(thread, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	TEST_ASSERT_EQUAL_INT(0, pthread_equal(mainId, childId));
}


/* pthread_exit: value_ptr made available via join */
static void *test_lifecycleExitValue(void *arg)
{
	(void)arg;
	pthread_exit((void *)123);
	return NULL;
}


TEST(pthread_lifecycle, exit_value_available_via_join)
{
	pthread_t thread;
	void *retval;
	int ret;

	ret = pthread_create(&thread, NULL, test_lifecycleExitValue, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_join(thread, &retval);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_PTR((void *)123, retval);
}


/* pthread_exit: implicit call when thread returns from start routine */
static void *test_lifecycleReturnValue(void *arg)
{
	(void)arg;
	return (void *)456;
}


TEST(pthread_lifecycle, exit_implicit_on_return)
{
	pthread_t thread;
	void *retval;
	int ret;

	ret = pthread_create(&thread, NULL, test_lifecycleReturnValue, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_join(thread, &retval);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_PTR((void *)456, retval);
}


/* pthread_join: shall suspend until target terminates */
static void *test_lifecycleSleepThread(void *arg)
{
	int *flag = (int *)arg;
	usleep(50000);
	*flag = 1;
	return NULL;
}


TEST(pthread_lifecycle, join_suspends_until_termination)
{
	pthread_t thread;
	int flag = 0;
	int ret;

	ret = pthread_create(&thread, NULL, test_lifecycleSleepThread, &flag);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_join(thread, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(1, flag);
}


/* pthread_join: NULL value_ptr is acceptable */
TEST(pthread_lifecycle, join_null_value_ptr)
{
	pthread_t thread;
	int ret;

	ret = pthread_create(&thread, NULL, test_lifecycleReturnValue, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_join(thread, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


/* pthread_join: target already terminated — shall return immediately */
TEST(pthread_lifecycle, join_already_terminated)
{
	pthread_t thread;
	void *retval;
	int ret;

	ret = pthread_create(&thread, NULL, test_lifecycleReturnValue, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Give thread time to finish */
	usleep(50000);

	ret = pthread_join(thread, &retval);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_PTR((void *)456, retval);
}


/* pthread_detach: shall return 0 for joinable thread */
static void *test_lifecycleDetachThread(void *arg)
{
	(void)arg;
	usleep(20000);
	return NULL;
}


TEST(pthread_lifecycle, detach_joinable_thread)
{
	pthread_t thread;
	int ret;

	ret = pthread_create(&thread, NULL, test_lifecycleDetachThread, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_detach(thread);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Allow detached thread to finish */
	usleep(100000);
}


/* pthread_detach: thread created in detached state does not need join */
TEST(pthread_lifecycle, detach_created_detached)
{
	pthread_attr_t attr;
	pthread_t thread;
	int ret;

	ret = pthread_attr_init(&attr);
	TEST_ASSERT_EQUAL_INT(0, ret);

#ifdef __phoenix__
	(void)thread;
	TEST_IGNORE_MESSAGE("#1637 issue");
#else
	ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_create(&thread, &attr, test_lifecycleDetachThread, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	pthread_attr_destroy(&attr);

	/* Allow detached thread to finish */
	usleep(100000);
#endif
}


TEST_GROUP_RUNNER(pthread_lifecycle)
{
	RUN_TEST_CASE(pthread_lifecycle, self_returns_valid_id);
	RUN_TEST_CASE(pthread_lifecycle, equal_same_thread);
	RUN_TEST_CASE(pthread_lifecycle, equal_different_threads);
	RUN_TEST_CASE(pthread_lifecycle, exit_value_available_via_join);
	RUN_TEST_CASE(pthread_lifecycle, exit_implicit_on_return);
	RUN_TEST_CASE(pthread_lifecycle, join_suspends_until_termination);
	RUN_TEST_CASE(pthread_lifecycle, join_null_value_ptr);
	RUN_TEST_CASE(pthread_lifecycle, join_already_terminated);
	RUN_TEST_CASE(pthread_lifecycle, detach_joinable_thread);
	RUN_TEST_CASE(pthread_lifecycle, detach_created_detached);
}


/* ===== pthread_once group ===== */


static int test_onceCounter;


static void test_onceInit(void)
{
	test_onceCounter++;
}


TEST_GROUP(pthread_once);


TEST_SETUP(pthread_once)
{
}


TEST_TEAR_DOWN(pthread_once)
{
}


/* pthread_once: init_routine called exactly once */
TEST(pthread_once, once_called_once)
{
	pthread_once_t once = PTHREAD_ONCE_INIT;
	int ret;

	test_onceCounter = 0;

	ret = pthread_once(&once, test_onceInit);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(1, test_onceCounter);

	ret = pthread_once(&once, test_onceInit);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(1, test_onceCounter);

	ret = pthread_once(&once, test_onceInit);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(1, test_onceCounter);
}


/* pthread_once: multiple threads — init_routine called only once */
static pthread_once_t test_onceControl = PTHREAD_ONCE_INIT;


static void *test_onceThread(void *arg)
{
	(void)arg;
	pthread_once(&test_onceControl, test_onceInit);
	return NULL;
}


TEST(pthread_once, once_multiple_threads)
{
	pthread_t t1, t2, t3;
	int ret;

	test_onceCounter = 0;
	test_onceControl = (pthread_once_t)PTHREAD_ONCE_INIT;

	ret = pthread_create(&t1, NULL, test_onceThread, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_create(&t2, NULL, test_onceThread, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_create(&t3, NULL, test_onceThread, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_join(t1, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_join(t2, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_join(t3, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	TEST_ASSERT_EQUAL_INT(1, test_onceCounter);
}


/* pthread_once: on return init_routine shall have completed */
static int test_onceCompleted;

static void test_onceSlowInit(void)
{
	usleep(50000);
	test_onceCompleted = 1;
}


static pthread_once_t test_onceSlowControl = PTHREAD_ONCE_INIT;


static void *test_onceSlowThread(void *arg)
{
	(void)arg;
	pthread_once(&test_onceSlowControl, test_onceSlowInit);
	return NULL;
}


TEST(pthread_once, once_completed_on_return)
{
	pthread_t t1, t2;
	int ret;

	test_onceCompleted = 0;
	test_onceSlowControl = (pthread_once_t)PTHREAD_ONCE_INIT;

	ret = pthread_create(&t1, NULL, test_onceSlowThread, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_create(&t2, NULL, test_onceSlowThread, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_join(t1, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_join(t2, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* After both threads return, init must have completed */
	TEST_ASSERT_EQUAL_INT(1, test_onceCompleted);
}


TEST_GROUP_RUNNER(pthread_once)
{
	RUN_TEST_CASE(pthread_once, once_called_once);
	RUN_TEST_CASE(pthread_once, once_multiple_threads);
	RUN_TEST_CASE(pthread_once, once_completed_on_return);
}
