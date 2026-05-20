/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - pthread.h
 * TESTED:
 *    - pthread_cancel()
 *    - pthread_setcancelstate()
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


TEST_GROUP(pthread_cancel);


TEST_SETUP(pthread_cancel)
{
}


TEST_TEAR_DOWN(pthread_cancel)
{
}


/* pthread_cancel: shall return 0 on success */
static void *test_cancelLongThread(void *arg)
{
	(void)arg;
	/* Loop with a cancellation point */
	while (1) {
		usleep(10000);
	}
	return NULL;
}


TEST(pthread_cancel, cancel_returns_zero)
{
	pthread_t thread;
	int ret;

	ret = pthread_create(&thread, NULL, test_cancelLongThread, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Give thread time to start */
	usleep(20000);

	ret = pthread_cancel(thread);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_join(thread, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


/* pthread_cancel: cancelled thread's exit status is PTHREAD_CANCELED */
TEST(pthread_cancel, cancel_exit_status)
{
	pthread_t thread;
	void *retval;
	int ret;

	ret = pthread_create(&thread, NULL, test_cancelLongThread, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	usleep(20000);

	ret = pthread_cancel(thread);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_join(thread, &retval);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_PTR(PTHREAD_CANCELED, retval);
}


/* pthread_cancel: cleanup handlers are invoked on cancellation */
static int test_cancelCleanupFlag;

static void test_cancelCleanupHandler(void *arg)
{
	int *flag = (int *)arg;
	*flag = 1;
}


static void *test_cancelWithCleanup(void *arg)
{
	int *flag = (int *)arg;

	pthread_cleanup_push(test_cancelCleanupHandler, flag);

	while (1) {
		usleep(10000);
	}

	pthread_cleanup_pop(0);
	return NULL;
}


TEST(pthread_cancel, cancel_invokes_cleanup_handlers)
{
	pthread_t thread;
	int ret;

	test_cancelCleanupFlag = 0;

	ret = pthread_create(&thread, NULL, test_cancelWithCleanup, &test_cancelCleanupFlag);
	TEST_ASSERT_EQUAL_INT(0, ret);

	usleep(20000);

	ret = pthread_cancel(thread);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_join(thread, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	TEST_ASSERT_EQUAL_INT(1, test_cancelCleanupFlag);
}


/* pthread_setcancelstate: disable cancellation prevents thread from being cancelled */
static void *test_cancelDisabledThread(void *arg)
{
	int *completed = (int *)arg;
	int oldstate;

	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);

	/* Do work that includes cancellation points */
	usleep(50000);
	*completed = 1;

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &oldstate);

	/* Now cancellation can take effect at next cancellation point */
	usleep(10000);

	return NULL;
}


TEST(pthread_cancel, setcancelstate_disable)
{
	pthread_t thread;
	int completed = 0;
	void *retval;
	int ret;

	ret = pthread_create(&thread, NULL, test_cancelDisabledThread, &completed);
	TEST_ASSERT_EQUAL_INT(0, ret);

	usleep(10000);

	ret = pthread_cancel(thread);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_join(thread, &retval);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Thread completed its work because cancel was disabled */
	TEST_ASSERT_EQUAL_INT(1, completed);
	TEST_ASSERT_EQUAL_PTR(PTHREAD_CANCELED, retval);
}


/* pthread_setcancelstate: shall return 0 and store old state */
TEST(pthread_cancel, setcancelstate_returns_oldstate)
{
	int oldstate;
	int ret;

	/* Default state is PTHREAD_CANCEL_ENABLE */
	ret = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(PTHREAD_CANCEL_ENABLE, oldstate);

	ret = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &oldstate);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(PTHREAD_CANCEL_DISABLE, oldstate);
}


/* pthread_setcancelstate: set to PTHREAD_CANCEL_ENABLE */
TEST(pthread_cancel, setcancelstate_enable)
{
	int oldstate;
	int ret;

	ret = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &oldstate);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(PTHREAD_CANCEL_DISABLE, oldstate);

	/* Restore */
	ret = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


/* pthread_cancel: cancellation runs asynchronously wrt calling thread */
TEST(pthread_cancel, cancel_async_wrt_caller)
{
	pthread_t thread;
	int ret;

	ret = pthread_create(&thread, NULL, test_cancelLongThread, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	usleep(20000);

	/* pthread_cancel returns immediately — does not wait for target to terminate */
	ret = pthread_cancel(thread);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Join to wait for actual termination */
	ret = pthread_join(thread, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST_GROUP_RUNNER(pthread_cancel)
{
	RUN_TEST_CASE(pthread_cancel, cancel_returns_zero);
	RUN_TEST_CASE(pthread_cancel, cancel_exit_status);
	RUN_TEST_CASE(pthread_cancel, cancel_invokes_cleanup_handlers);
	RUN_TEST_CASE(pthread_cancel, setcancelstate_disable);
	RUN_TEST_CASE(pthread_cancel, setcancelstate_returns_oldstate);
	RUN_TEST_CASE(pthread_cancel, setcancelstate_enable);
	RUN_TEST_CASE(pthread_cancel, cancel_async_wrt_caller);
}
