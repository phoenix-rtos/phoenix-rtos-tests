/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <semaphore.h>
 * TESTED:
 *    - sem_destroy()
 *    - sem_getvalue()
 *    - sem_init()
 *    - sem_post()
 *    - sem_timedwait()
 *    - sem_trywait()
 *    - sem_wait()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <limits.h>
#include <semaphore.h>
#include <string.h>
#include <time.h>

#include "unity_fixture.h"

/* Tests: sem_init, sem_destroy, sem_post, sem_wait, sem_trywait, sem_timedwait, sem_getvalue */
TEST_GROUP(sem_unnamed);

static struct {
	sem_t sem;
	int initialized;
} test_common;


TEST_SETUP(sem_unnamed)
{
	test_common.initialized = 0;
}

TEST_TEAR_DOWN(sem_unnamed)
{
	if (test_common.initialized != 0) {
		sem_destroy(&test_common.sem);
		test_common.initialized = 0;
	}
}


TEST(sem_unnamed, init_zero_value)
{
	int ret;

	errno = 0;
	ret = sem_init(&test_common.sem, 0, 0);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, errno);
	test_common.initialized = 1;
}


TEST(sem_unnamed, init_positive_value)
{
	int ret;
	int sval;

	ret = sem_init(&test_common.sem, 0, 5);
	TEST_ASSERT_EQUAL_INT(0, ret);
	test_common.initialized = 1;

	ret = sem_getvalue(&test_common.sem, &sval);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(5, sval);
}


TEST(sem_unnamed, init_einval_exceeds_max)
{
	int ret;

	errno = 0;
	ret = sem_init(&test_common.sem, 0, (unsigned int)SEM_VALUE_MAX + 1U);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST(sem_unnamed, destroy_initialized)
{
	int ret;

	ret = sem_init(&test_common.sem, 0, 1);
	TEST_ASSERT_EQUAL_INT(0, ret);
	test_common.initialized = 1;

	errno = 0;
	ret = sem_destroy(&test_common.sem);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, errno);
	test_common.initialized = 0;
}


TEST(sem_unnamed, post_increments_value)
{
	int ret;
	int sval;

	ret = sem_init(&test_common.sem, 0, 0);
	TEST_ASSERT_EQUAL_INT(0, ret);
	test_common.initialized = 1;

	errno = 0;
	ret = sem_post(&test_common.sem);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, errno);

	ret = sem_getvalue(&test_common.sem, &sval);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(1, sval);
}


TEST(sem_unnamed, post_multiple)
{
	int ret;
	int sval;
	int i;

	ret = sem_init(&test_common.sem, 0, 0);
	TEST_ASSERT_EQUAL_INT(0, ret);
	test_common.initialized = 1;

	for (i = 0; i < 10; i++) {
		ret = sem_post(&test_common.sem);
		TEST_ASSERT_EQUAL_INT(0, ret);
	}

	ret = sem_getvalue(&test_common.sem, &sval);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(10, sval);
}


TEST(sem_unnamed, wait_decrements_value)
{
	int ret;
	int sval;

	ret = sem_init(&test_common.sem, 0, 3);
	TEST_ASSERT_EQUAL_INT(0, ret);
	test_common.initialized = 1;

	errno = 0;
	ret = sem_wait(&test_common.sem);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, errno);

	ret = sem_getvalue(&test_common.sem, &sval);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(2, sval);
}


TEST(sem_unnamed, wait_locks_at_zero)
{
	int ret;
	int sval;

	ret = sem_init(&test_common.sem, 0, 1);
	TEST_ASSERT_EQUAL_INT(0, ret);
	test_common.initialized = 1;

	/* First wait should succeed */
	ret = sem_wait(&test_common.sem);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = sem_getvalue(&test_common.sem, &sval);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, sval);
}


TEST(sem_unnamed, trywait_success)
{
	int ret;
	int sval;

	ret = sem_init(&test_common.sem, 0, 2);
	TEST_ASSERT_EQUAL_INT(0, ret);
	test_common.initialized = 1;

	errno = 0;
	ret = sem_trywait(&test_common.sem);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, errno);

	ret = sem_getvalue(&test_common.sem, &sval);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(1, sval);
}


TEST(sem_unnamed, trywait_eagain_when_locked)
{
	int ret;

	ret = sem_init(&test_common.sem, 0, 0);
	TEST_ASSERT_EQUAL_INT(0, ret);
	test_common.initialized = 1;

	errno = 0;
	ret = sem_trywait(&test_common.sem);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EAGAIN, errno);
}


TEST(sem_unnamed, timedwait_success_immediate)
{
	int ret;
	struct timespec ts;

	ret = sem_init(&test_common.sem, 0, 1);
	TEST_ASSERT_EQUAL_INT(0, ret);
	test_common.initialized = 1;

	ret = clock_gettime(CLOCK_REALTIME, &ts);
	TEST_ASSERT_EQUAL_INT(0, ret);
	ts.tv_sec += 5;

	errno = 0;
	ret = sem_timedwait(&test_common.sem, &ts);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, errno);
}


TEST(sem_unnamed, timedwait_etimedout)
{
	int ret;
	struct timespec ts;

	ret = sem_init(&test_common.sem, 0, 0);
	TEST_ASSERT_EQUAL_INT(0, ret);
	test_common.initialized = 1;

	ret = clock_gettime(CLOCK_REALTIME, &ts);
	TEST_ASSERT_EQUAL_INT(0, ret);
	/* Set timeout in the past */
	ts.tv_sec -= 1;

	errno = 0;
	ret = sem_timedwait(&test_common.sem, &ts);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(ETIMEDOUT, errno);
}


TEST(sem_unnamed, timedwait_einval_bad_nsec)
{
	int ret;
	struct timespec ts;

	ret = sem_init(&test_common.sem, 0, 0);
	TEST_ASSERT_EQUAL_INT(0, ret);
	test_common.initialized = 1;

	ts.tv_sec = 0;
	ts.tv_nsec = -1;
	errno = 0;
	ret = sem_timedwait(&test_common.sem, &ts);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);

	ts.tv_nsec = 1000000000L;
	errno = 0;
	ret = sem_timedwait(&test_common.sem, &ts);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST(sem_unnamed, getvalue_returns_current)
{
	int ret;
	int sval;

	ret = sem_init(&test_common.sem, 0, 7);
	TEST_ASSERT_EQUAL_INT(0, ret);
	test_common.initialized = 1;

	ret = sem_getvalue(&test_common.sem, &sval);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(7, sval);

	/* After a wait, value decreases */
	ret = sem_wait(&test_common.sem);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = sem_getvalue(&test_common.sem, &sval);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(6, sval);
}


TEST(sem_unnamed, getvalue_zero_when_locked)
{
	int ret;
	int sval;

	ret = sem_init(&test_common.sem, 0, 0);
	TEST_ASSERT_EQUAL_INT(0, ret);
	test_common.initialized = 1;

	ret = sem_getvalue(&test_common.sem, &sval);
	TEST_ASSERT_EQUAL_INT(0, ret);
	/* When locked (value is 0), sval shall be 0 or negative */
	TEST_ASSERT_TRUE(sval <= 0);
}


TEST(sem_unnamed, post_then_wait_roundtrip)
{
	int ret;
	int sval;

	ret = sem_init(&test_common.sem, 0, 0);
	TEST_ASSERT_EQUAL_INT(0, ret);
	test_common.initialized = 1;

	ret = sem_post(&test_common.sem);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = sem_wait(&test_common.sem);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = sem_getvalue(&test_common.sem, &sval);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, sval);
}


TEST(sem_unnamed, init_pshared_zero_thread_shared)
{
	int ret;
	int sval;

	/* pshared == 0 means shared between threads only */
	ret = sem_init(&test_common.sem, 0, 3);
	TEST_ASSERT_EQUAL_INT(0, ret);
	test_common.initialized = 1;

	ret = sem_getvalue(&test_common.sem, &sval);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(3, sval);
}


TEST_GROUP_RUNNER(sem_unnamed)
{
	RUN_TEST_CASE(sem_unnamed, init_zero_value);
	RUN_TEST_CASE(sem_unnamed, init_positive_value);
	RUN_TEST_CASE(sem_unnamed, init_einval_exceeds_max);
	RUN_TEST_CASE(sem_unnamed, destroy_initialized);
	RUN_TEST_CASE(sem_unnamed, post_increments_value);
	RUN_TEST_CASE(sem_unnamed, post_multiple);
	RUN_TEST_CASE(sem_unnamed, wait_decrements_value);
	RUN_TEST_CASE(sem_unnamed, wait_locks_at_zero);
	RUN_TEST_CASE(sem_unnamed, trywait_success);
	RUN_TEST_CASE(sem_unnamed, trywait_eagain_when_locked);
	RUN_TEST_CASE(sem_unnamed, timedwait_success_immediate);
	RUN_TEST_CASE(sem_unnamed, timedwait_etimedout);
	RUN_TEST_CASE(sem_unnamed, timedwait_einval_bad_nsec);
	RUN_TEST_CASE(sem_unnamed, getvalue_returns_current);
	RUN_TEST_CASE(sem_unnamed, getvalue_zero_when_locked);
	RUN_TEST_CASE(sem_unnamed, post_then_wait_roundtrip);
	RUN_TEST_CASE(sem_unnamed, init_pshared_zero_thread_shared);
}
