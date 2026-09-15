/*
 * Phoenix-RTOS
 *
 * POSIX.1-2024 standard library functions tests
 * HEADER:
 *    - <pthread.h>
 * TESTED:
 *    - pthread_cond_clockwait()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Adam Greloch
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* pthread_cond_clockwait() is a GNU extension on glibc hosts */
#define _GNU_SOURCE

#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "unity_fixture.h"
#include "libc_features.h"


/* An arbitrary clock id that is not a valid timeout clock (CLOCK_PROCESS_CPUTIME_ID is not defined on Phoenix) */
#define TEST_INVALID_CLOCK ((clockid_t)1234)


#ifdef HAS_PTHREAD_COND_CLOCKWAIT
static struct {
	pthread_mutex_t mtx;
	pthread_cond_t cond;
	volatile int ready;
} test_common;


static void test_getAbstime(clockid_t clockId, struct timespec *ts, long offsetMs)
{
	int ret;

	ret = clock_gettime(clockId, ts);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ts->tv_sec += offsetMs / 1000;
	ts->tv_nsec += (offsetMs % 1000) * 1000000L;
	while (ts->tv_nsec >= 1000000000L) {
		ts->tv_sec++;
		ts->tv_nsec -= 1000000000L;
	}
}


static long test_elapsedMs(clockid_t clockId, const struct timespec *start)
{
	struct timespec now;
	int ret;

	ret = clock_gettime(clockId, &now);
	TEST_ASSERT_EQUAL_INT(0, ret);

	return (now.tv_sec - start->tv_sec) * 1000L + (now.tv_nsec - start->tv_nsec) / 1000000L;
}


/* Signals the condition variable after a short delay */
static void *test_signalThread(void *arg)
{
	(void)arg;

	usleep(20000);

	pthread_mutex_lock(&test_common.mtx);
	test_common.ready = 1;
	pthread_cond_signal(&test_common.cond);
	pthread_mutex_unlock(&test_common.mtx);

	return NULL;
}


/* Broadcasts on the condition variable after a short delay */
static void *test_broadcastThread(void *arg)
{
	(void)arg;

	usleep(20000);

	pthread_mutex_lock(&test_common.mtx);
	test_common.ready = 1;
	pthread_cond_broadcast(&test_common.cond);
	pthread_mutex_unlock(&test_common.mtx);

	return NULL;
}


/* Waits on the condition variable using a CLOCK_MONOTONIC deadline, reports the error via arg */
static void *test_waitThread(void *arg)
{
	int *err = (int *)arg;
	struct timespec ts;

	*err = pthread_mutex_lock(&test_common.mtx);
	if (*err != 0) {
		return NULL;
	}

	test_getAbstime(CLOCK_MONOTONIC, &ts, 5000);

	while (test_common.ready == 0) {
		*err = pthread_cond_clockwait(&test_common.cond, &test_common.mtx, CLOCK_MONOTONIC, &ts);
		if (*err != 0) {
			break;
		}
	}

	pthread_mutex_unlock(&test_common.mtx);

	return NULL;
}


/* Reports the result of pthread_mutex_trylock() on the shared mutex via arg */
static void *test_trylockThread(void *arg)
{
	int *ret = (int *)arg;

	*ret = pthread_mutex_trylock(&test_common.mtx);
	if (*ret == 0) {
		pthread_mutex_unlock(&test_common.mtx);
	}

	return NULL;
}
#endif


TEST_GROUP(pthread_cond_clockwait);


TEST_SETUP(pthread_cond_clockwait)
{
#ifdef HAS_PTHREAD_COND_CLOCKWAIT
	TEST_ASSERT_EQUAL_INT(0, pthread_mutex_init(&test_common.mtx, NULL));
	TEST_ASSERT_EQUAL_INT(0, pthread_cond_init(&test_common.cond, NULL));
	test_common.ready = 0;
#endif
}


TEST_TEAR_DOWN(pthread_cond_clockwait)
{
#ifdef HAS_PTHREAD_COND_CLOCKWAIT
	pthread_cond_destroy(&test_common.cond);
	pthread_mutex_destroy(&test_common.mtx);
#endif
}


/* pthread_cond_clockwait: CLOCK_REALTIME deadline, woken up by pthread_cond_signal */
TEST(pthread_cond_clockwait, realtime_signal)
{
#ifndef HAS_PTHREAD_COND_CLOCKWAIT
	TEST_IGNORE_MESSAGE("pthread_cond_clockwait is not implemented");
#else
	pthread_t thread;
	struct timespec ts;
	int ret;

	ret = pthread_mutex_lock(&test_common.mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_create(&thread, NULL, test_signalThread, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	test_getAbstime(CLOCK_REALTIME, &ts, 5000);

	while (test_common.ready == 0) {
		ret = pthread_cond_clockwait(&test_common.cond, &test_common.mtx, CLOCK_REALTIME, &ts);
		TEST_ASSERT_EQUAL_INT(0, ret);
	}

	ret = pthread_mutex_unlock(&test_common.mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_join(thread, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);
#endif
}


/*
 * pthread_cond_clockwait: CLOCK_MONOTONIC deadline is honored even though the
 * condition variable was initialized with the default (CLOCK_REALTIME) clock
 */
TEST(pthread_cond_clockwait, monotonic_signal)
{
#ifndef HAS_PTHREAD_COND_CLOCKWAIT
	TEST_IGNORE_MESSAGE("pthread_cond_clockwait is not implemented");
#else
	pthread_t thread;
	struct timespec ts;
	int ret;

	ret = pthread_mutex_lock(&test_common.mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_create(&thread, NULL, test_signalThread, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	test_getAbstime(CLOCK_MONOTONIC, &ts, 5000);

	while (test_common.ready == 0) {
		ret = pthread_cond_clockwait(&test_common.cond, &test_common.mtx, CLOCK_MONOTONIC, &ts);
		TEST_ASSERT_EQUAL_INT(0, ret);
	}

	ret = pthread_mutex_unlock(&test_common.mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_join(thread, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);
#endif
}


/* pthread_cond_clockwait: pthread_cond_broadcast wakes up all the waiters */
TEST(pthread_cond_clockwait, broadcast_wakes_all)
{
#ifndef HAS_PTHREAD_COND_CLOCKWAIT
	TEST_IGNORE_MESSAGE("pthread_cond_clockwait is not implemented");
#else
	pthread_t first, second, broadcaster;
	int errFirst = -1, errSecond = -1;
	int ret;

	ret = pthread_create(&first, NULL, test_waitThread, &errFirst);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_create(&second, NULL, test_waitThread, &errSecond);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_create(&broadcaster, NULL, test_broadcastThread, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	TEST_ASSERT_EQUAL_INT(0, pthread_join(first, NULL));
	TEST_ASSERT_EQUAL_INT(0, pthread_join(second, NULL));
	TEST_ASSERT_EQUAL_INT(0, pthread_join(broadcaster, NULL));

	TEST_ASSERT_EQUAL_INT(0, errFirst);
	TEST_ASSERT_EQUAL_INT(0, errSecond);
#endif
}


/* pthread_cond_clockwait: ETIMEDOUT after the CLOCK_MONOTONIC deadline elapses */
TEST(pthread_cond_clockwait, etimedout_monotonic)
{
#ifndef HAS_PTHREAD_COND_CLOCKWAIT
	TEST_IGNORE_MESSAGE("pthread_cond_clockwait is not implemented");
#else
	struct timespec start, ts;
	int ret;

	ret = clock_gettime(CLOCK_MONOTONIC, &start);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutex_lock(&test_common.mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);

	test_getAbstime(CLOCK_MONOTONIC, &ts, 100);

	/* Nobody signals the condition variable - loop to ignore spurious wakeups */
	do {
		ret = pthread_cond_clockwait(&test_common.cond, &test_common.mtx, CLOCK_MONOTONIC, &ts);
	} while (ret == 0);

	TEST_ASSERT_EQUAL_INT(ETIMEDOUT, ret);
	/* The wait must not return before the deadline */
	TEST_ASSERT_GREATER_OR_EQUAL_INT(90, test_elapsedMs(CLOCK_MONOTONIC, &start));

	ret = pthread_mutex_unlock(&test_common.mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);
#endif
}


/* pthread_cond_clockwait: ETIMEDOUT after the CLOCK_REALTIME deadline elapses */
TEST(pthread_cond_clockwait, etimedout_realtime)
{
#ifndef HAS_PTHREAD_COND_CLOCKWAIT
	TEST_IGNORE_MESSAGE("pthread_cond_clockwait is not implemented");
#else
	struct timespec start, ts;
	int ret;

	ret = clock_gettime(CLOCK_REALTIME, &start);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutex_lock(&test_common.mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);

	test_getAbstime(CLOCK_REALTIME, &ts, 100);

	do {
		ret = pthread_cond_clockwait(&test_common.cond, &test_common.mtx, CLOCK_REALTIME, &ts);
	} while (ret == 0);

	TEST_ASSERT_EQUAL_INT(ETIMEDOUT, ret);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(90, test_elapsedMs(CLOCK_REALTIME, &start));

	ret = pthread_mutex_unlock(&test_common.mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);
#endif
}


/* pthread_cond_clockwait: ETIMEDOUT when abstime has already passed */
TEST(pthread_cond_clockwait, etimedout_abstime_in_past)
{
#ifndef HAS_PTHREAD_COND_CLOCKWAIT
	TEST_IGNORE_MESSAGE("pthread_cond_clockwait is not implemented");
#else
	struct timespec ts;
	int ret;

	ret = pthread_mutex_lock(&test_common.mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ts.tv_sec = 0;
	ts.tv_nsec = 0;

	ret = pthread_cond_clockwait(&test_common.cond, &test_common.mtx, CLOCK_REALTIME, &ts);
	TEST_ASSERT_EQUAL_INT(ETIMEDOUT, ret);

	ret = pthread_cond_clockwait(&test_common.cond, &test_common.mtx, CLOCK_MONOTONIC, &ts);
	TEST_ASSERT_EQUAL_INT(ETIMEDOUT, ret);

	ret = pthread_mutex_unlock(&test_common.mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);
#endif
}


/* pthread_cond_clockwait: the mutex is reacquired before the call returns with ETIMEDOUT */
TEST(pthread_cond_clockwait, mutex_reacquired_on_timeout)
{
#ifndef HAS_PTHREAD_COND_CLOCKWAIT
	TEST_IGNORE_MESSAGE("pthread_cond_clockwait is not implemented");
#else
	pthread_t thread;
	struct timespec ts;
	int trylockRet = -1;
	int ret;

	ret = pthread_mutex_lock(&test_common.mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);

	test_getAbstime(CLOCK_MONOTONIC, &ts, 50);

	do {
		ret = pthread_cond_clockwait(&test_common.cond, &test_common.mtx, CLOCK_MONOTONIC, &ts);
	} while (ret == 0);

	TEST_ASSERT_EQUAL_INT(ETIMEDOUT, ret);

	/* The mutex is owned by this thread again, so another thread must not be able to lock it */
	ret = pthread_create(&thread, NULL, test_trylockThread, &trylockRet);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_join(thread, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	TEST_ASSERT_EQUAL_INT(EBUSY, trylockRet);

	ret = pthread_mutex_unlock(&test_common.mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);
#endif
}


/* pthread_cond_clockwait: EINVAL for an invalid nanosecond value */
TEST(pthread_cond_clockwait, einval_invalid_nsec)
{
#ifndef HAS_PTHREAD_COND_CLOCKWAIT
	TEST_IGNORE_MESSAGE("pthread_cond_clockwait is not implemented");
#else
	struct timespec ts;
	int ret;

	ret = pthread_mutex_lock(&test_common.mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Negative nanoseconds */
	test_getAbstime(CLOCK_MONOTONIC, &ts, 100);
	ts.tv_nsec = -1;

	ret = pthread_cond_clockwait(&test_common.cond, &test_common.mtx, CLOCK_MONOTONIC, &ts);
	TEST_ASSERT_EQUAL_INT(EINVAL, ret);

	/* nsec >= 1000000000 */
	test_getAbstime(CLOCK_MONOTONIC, &ts, 100);
	ts.tv_nsec = 1000000000L;

	ret = pthread_cond_clockwait(&test_common.cond, &test_common.mtx, CLOCK_MONOTONIC, &ts);
	TEST_ASSERT_EQUAL_INT(EINVAL, ret);

	ret = pthread_mutex_unlock(&test_common.mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);
#endif
}


/* pthread_cond_clockwait: EINVAL for a clock that cannot be used for timeouts */
TEST(pthread_cond_clockwait, einval_unsupported_clock)
{
#ifndef HAS_PTHREAD_COND_CLOCKWAIT
	TEST_IGNORE_MESSAGE("pthread_cond_clockwait is not implemented");
#else
	struct timespec ts;
	int ret;

	ret = pthread_mutex_lock(&test_common.mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);

	test_getAbstime(CLOCK_REALTIME, &ts, 100);

	ret = pthread_cond_clockwait(&test_common.cond, &test_common.mtx, TEST_INVALID_CLOCK, &ts);
	TEST_ASSERT_EQUAL_INT(EINVAL, ret);

	ret = pthread_mutex_unlock(&test_common.mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);
#endif
}


/* pthread_cond_clockwait: CLOCK_MONOTONIC_RAW is accepted on Phoenix-RTOS only */
TEST(pthread_cond_clockwait, monotonic_raw)
{
#ifndef HAS_PTHREAD_COND_CLOCKWAIT
	TEST_IGNORE_MESSAGE("pthread_cond_clockwait is not implemented");
#else
	struct timespec ts;
	int ret;

	ret = pthread_mutex_lock(&test_common.mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);

	test_getAbstime(CLOCK_MONOTONIC_RAW, &ts, 50);

	do {
		ret = pthread_cond_clockwait(&test_common.cond, &test_common.mtx, CLOCK_MONOTONIC_RAW, &ts);
	} while (ret == 0);

#ifdef __phoenix__
	/* CLOCK_MONOTONIC_RAW is handled as an alias of CLOCK_MONOTONIC */
	TEST_ASSERT_EQUAL_INT(ETIMEDOUT, ret);
#else
	/* glibc rejects every clock other than CLOCK_REALTIME and CLOCK_MONOTONIC */
	TEST_ASSERT_EQUAL_INT(EINVAL, ret);
#endif

	ret = pthread_mutex_unlock(&test_common.mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);
#endif
}


TEST_GROUP_RUNNER(pthread_cond_clockwait)
{
	RUN_TEST_CASE(pthread_cond_clockwait, realtime_signal);
	RUN_TEST_CASE(pthread_cond_clockwait, monotonic_signal);
	RUN_TEST_CASE(pthread_cond_clockwait, broadcast_wakes_all);
	RUN_TEST_CASE(pthread_cond_clockwait, etimedout_monotonic);
	RUN_TEST_CASE(pthread_cond_clockwait, etimedout_realtime);
	RUN_TEST_CASE(pthread_cond_clockwait, etimedout_abstime_in_past);
	RUN_TEST_CASE(pthread_cond_clockwait, mutex_reacquired_on_timeout);
	RUN_TEST_CASE(pthread_cond_clockwait, einval_invalid_nsec);
	RUN_TEST_CASE(pthread_cond_clockwait, einval_unsupported_clock);
	RUN_TEST_CASE(pthread_cond_clockwait, monotonic_raw);
}
