/*
 * Phoenix-RTOS
 *
 * libc/semaphore
 *
 * tests for unnamed POSIX semaphores (sem_init/sem_destroy)
 *
 * Copyright 2026 Phoenix Systems
 * Author: Ziemowit Leszczynski
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* glibc declares sem_clockwait() only under _GNU_SOURCE */
#define _GNU_SOURCE

#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>

#include "unity_fixture.h"

#include "common.h"


static sem_t sem;
static int inited;


static void *unnamed_waiter(void *arg)
{
	int *ret = (int *)arg;

	*ret = sem_wait(&sem);

	return NULL;
}


static void *unnamed_clockwaiter(void *arg)
{
	int *ret = (int *)arg;
	struct timespec ts;

	sem_test_deadlineOn(&ts, CLOCK_MONOTONIC, 500);
	*ret = sem_clockwait(&sem, CLOCK_MONOTONIC, &ts);

	return NULL;
}


TEST_GROUP(sem_unnamed);


TEST_SETUP(sem_unnamed)
{
	inited = 0;
}


TEST_TEAR_DOWN(sem_unnamed)
{
	sem_test_clockRestore();

	if (inited != 0) {
		sem_destroy(&sem);
		inited = 0;
	}
}


static void unnamed_init(unsigned int value)
{
	TEST_ASSERT_EQUAL_INT(0, sem_init(&sem, 0, value));
	inited = 1;
}


TEST(sem_unnamed, init_destroy)
{
	unnamed_init(0);
	TEST_ASSERT_EQUAL_INT(0, sem_destroy(&sem));
	inited = 0;
}


TEST(sem_unnamed, init_pshared_unsupported)
{
#ifndef __phoenix__
	/* Linux implements process-shared semaphores, so there is nothing to reject */
	TEST_IGNORE_MESSAGE("phoenix-specific");
#else
	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, sem_init(&sem, 1, 0));
	TEST_ASSERT_EQUAL_INT(ENOSYS, errno);
#endif
}


TEST(sem_unnamed, init_null)
{
#ifndef __phoenix__
	/* POSIX leaves a NULL `sem` undefined, and glibc declares sem_init() nonnull */
	TEST_IGNORE_MESSAGE("phoenix-specific");
#else
	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, sem_init(NULL, 0, 0));
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
#endif
}


/* an oversized value is EINVAL */
TEST(sem_unnamed, init_value_too_large)
{
	if (SEM_VALUE_MAX == UINT_MAX) {
		TEST_IGNORE_MESSAGE("SEM_VALUE_MAX is UINT_MAX - no oversized value exists");
	}

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, sem_init(&sem, 0, (unsigned int)SEM_VALUE_MAX + 1u));
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST(sem_unnamed, init_value_max)
{
	unnamed_init((unsigned int)SEM_VALUE_MAX);
}


/* returns 0 (not the count), and stores the value even when it is 0 */
TEST(sem_unnamed, getvalue_zero)
{
	int value = -1;

	unnamed_init(0);

	TEST_ASSERT_EQUAL_INT(0, sem_getvalue(&sem, &value));
	TEST_ASSERT_EQUAL_INT(0, value);
}


/* the return value must be 0, not the count */
TEST(sem_unnamed, getvalue_nonzero)
{
	int value = -1;

	unnamed_init(7);

	TEST_ASSERT_EQUAL_INT(0, sem_getvalue(&sem, &value));
	TEST_ASSERT_EQUAL_INT(7, value);
}


TEST(sem_unnamed, wait_post_getvalue)
{
	int value = -1;

	unnamed_init(2);

	TEST_ASSERT_EQUAL_INT(0, sem_wait(&sem));
	TEST_ASSERT_EQUAL_INT(0, sem_getvalue(&sem, &value));
	TEST_ASSERT_EQUAL_INT(1, value);

	TEST_ASSERT_EQUAL_INT(0, sem_post(&sem));
	TEST_ASSERT_EQUAL_INT(0, sem_getvalue(&sem, &value));
	TEST_ASSERT_EQUAL_INT(2, value);
}


TEST(sem_unnamed, trywait_success)
{
	unnamed_init(1);

	TEST_ASSERT_EQUAL_INT(0, sem_trywait(&sem));
}


/* contention on the semaphore is EAGAIN */
TEST(sem_unnamed, trywait_empty)
{
	unnamed_init(0);

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, sem_trywait(&sem));
	TEST_ASSERT_EQUAL_INT(EAGAIN, errno);
}


/* sem_post() must report EOVERFLOW instead of wrapping */
TEST(sem_unnamed, post_overflow)
{
	int value = -1;

	unnamed_init((unsigned int)SEM_VALUE_MAX);

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, sem_post(&sem));
	TEST_ASSERT_EQUAL_INT(EOVERFLOW, errno);

	/* the value must be untouched, not wrapped to 0 */
	TEST_ASSERT_EQUAL_INT(0, sem_getvalue(&sem, &value));
	TEST_ASSERT_EQUAL_INT(SEM_VALUE_MAX, value);
}


TEST(sem_unnamed, post_to_max)
{
	int value = -1;

	unnamed_init((unsigned int)SEM_VALUE_MAX - 1u);

	TEST_ASSERT_EQUAL_INT(0, sem_post(&sem));
	TEST_ASSERT_EQUAL_INT(0, sem_getvalue(&sem, &value));
	TEST_ASSERT_EQUAL_INT(SEM_VALUE_MAX, value);
}


TEST(sem_unnamed, timedwait_success)
{
	struct timespec ts;

	unnamed_init(1);

	sem_test_deadline(&ts, 500);
	TEST_ASSERT_EQUAL_INT(0, sem_timedwait(&sem, &ts));
}


TEST(sem_unnamed, timedwait_timeout)
{
	struct timespec ts, start;

	unnamed_init(0);

	clock_gettime(CLOCK_REALTIME, &start);
	sem_test_deadline(&ts, 200);

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, sem_timedwait(&sem, &ts));
	TEST_ASSERT_EQUAL_INT(ETIMEDOUT, errno);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(150, sem_test_elapsedMs(&start));
}


/* POSIX: an expired deadline still succeeds if the semaphore can be locked at once */
TEST(sem_unnamed, timedwait_expired_but_available)
{
	struct timespec ts;

	unnamed_init(1);

	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec -= 10;

	TEST_ASSERT_EQUAL_INT(0, sem_timedwait(&sem, &ts));
}


TEST(sem_unnamed, timedwait_expired_and_empty)
{
	struct timespec ts;

	unnamed_init(0);

	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec -= 10;

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, sem_timedwait(&sem, &ts));
	TEST_ASSERT_EQUAL_INT(ETIMEDOUT, errno);
}


/*
 * The epoch is an ordinary already-passed deadline, not "no deadline" - both
 * backends must time out rather than block.
 */
TEST(sem_unnamed, timedwait_epoch_and_empty)
{
	struct timespec ts = { .tv_sec = 0, .tv_nsec = 0 };

	unnamed_init(0);

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, sem_timedwait(&sem, &ts));
	TEST_ASSERT_EQUAL_INT(ETIMEDOUT, errno);
}


TEST(sem_unnamed, timedwait_invalid_nsec)
{
	struct timespec ts;

	unnamed_init(0);

	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_nsec = 1000000000L;

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, sem_timedwait(&sem, &ts));
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST(sem_unnamed, post_wakes_waiter)
{
	pthread_t tid;
	int ret = -1;

	unnamed_init(0);

	TEST_ASSERT_EQUAL_INT(0, pthread_create(&tid, NULL, unnamed_waiter, &ret));

	usleep(100 * 1000);
	TEST_ASSERT_EQUAL_INT(0, sem_post(&sem));

	TEST_ASSERT_EQUAL_INT(0, pthread_join(tid, NULL));
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST(sem_unnamed, clockwait_monotonic_success)
{
	struct timespec ts;

	unnamed_init(1);

	sem_test_deadlineOn(&ts, CLOCK_MONOTONIC, 500);
	TEST_ASSERT_EQUAL_INT(0, sem_clockwait(&sem, CLOCK_MONOTONIC, &ts));
}


TEST(sem_unnamed, clockwait_realtime_success)
{
	struct timespec ts;

	unnamed_init(1);

	sem_test_deadlineOn(&ts, CLOCK_REALTIME, 500);
	TEST_ASSERT_EQUAL_INT(0, sem_clockwait(&sem, CLOCK_REALTIME, &ts));
}


/*
 * The elapsed-time check is what proves the clock argument is honoured: a
 * monotonic timestamp fed to a realtime wait would be far in the past on a
 * system whose realtime clock is set, and would return at once.
 */
TEST(sem_unnamed, clockwait_monotonic_timeout)
{
	struct timespec ts, start;

	unnamed_init(0);

	clock_gettime(CLOCK_REALTIME, &start);
	sem_test_deadlineOn(&ts, CLOCK_MONOTONIC, 200);

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, sem_clockwait(&sem, CLOCK_MONOTONIC, &ts));
	TEST_ASSERT_EQUAL_INT(ETIMEDOUT, errno);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(150, sem_test_elapsedMs(&start));
}


TEST(sem_unnamed, clockwait_realtime_timeout)
{
	struct timespec ts, start;

	unnamed_init(0);

	clock_gettime(CLOCK_REALTIME, &start);
	sem_test_deadlineOn(&ts, CLOCK_REALTIME, 200);

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, sem_clockwait(&sem, CLOCK_REALTIME, &ts));
	TEST_ASSERT_EQUAL_INT(ETIMEDOUT, errno);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(150, sem_test_elapsedMs(&start));
}


/*
 * The only check that can tell the two clocks apart: with no RTC they read the
 * same, so realtime is shifted first. 5 s is small enough to be harmless and
 * large enough that a deadline read on the wrong clock is unmistakable - the
 * monotonic one would already have passed, the realtime one would be 5 s out.
 */
TEST(sem_unnamed, clockwait_clock_is_honoured)
{
	struct timespec ts, start;

	unnamed_init(0);

	if (sem_test_clockShift(5) != 0) {
		TEST_IGNORE_MESSAGE("CLOCK_REALTIME cannot be set");
	}

	clock_gettime(CLOCK_MONOTONIC, &start);
	sem_test_deadlineOn(&ts, CLOCK_MONOTONIC, 400);
	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, sem_clockwait(&sem, CLOCK_MONOTONIC, &ts));
	TEST_ASSERT_EQUAL_INT(ETIMEDOUT, errno);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(300, sem_test_elapsedMsOn(&start, CLOCK_MONOTONIC));

	clock_gettime(CLOCK_MONOTONIC, &start);
	sem_test_deadlineOn(&ts, CLOCK_REALTIME, 400);
	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, sem_clockwait(&sem, CLOCK_REALTIME, &ts));
	TEST_ASSERT_EQUAL_INT(ETIMEDOUT, errno);
	TEST_ASSERT_LESS_THAN_INT(2000, sem_test_elapsedMsOn(&start, CLOCK_MONOTONIC));
}


TEST(sem_unnamed, clockwait_expired_but_available)
{
	struct timespec ts;

	unnamed_init(1);

	clock_gettime(CLOCK_MONOTONIC, &ts);
	ts.tv_sec -= 10;

	TEST_ASSERT_EQUAL_INT(0, sem_clockwait(&sem, CLOCK_MONOTONIC, &ts));
}


TEST(sem_unnamed, clockwait_expired_and_empty)
{
	struct timespec ts;

	unnamed_init(0);

	clock_gettime(CLOCK_MONOTONIC, &ts);
	ts.tv_sec -= 10;

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, sem_clockwait(&sem, CLOCK_MONOTONIC, &ts));
	TEST_ASSERT_EQUAL_INT(ETIMEDOUT, errno);
}


/* the epoch is an ordinary already-passed deadline, not "no deadline" */
TEST(sem_unnamed, clockwait_epoch_and_empty)
{
	struct timespec ts = { .tv_sec = 0, .tv_nsec = 0 };

	unnamed_init(0);

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, sem_clockwait(&sem, CLOCK_MONOTONIC, &ts));
	TEST_ASSERT_EQUAL_INT(ETIMEDOUT, errno);
}


TEST(sem_unnamed, clockwait_invalid_clock)
{
	struct timespec ts;

	unnamed_init(0);

	sem_test_deadlineOn(&ts, CLOCK_REALTIME, 200);

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, sem_clockwait(&sem, (clockid_t)0x7fff, &ts));
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST(sem_unnamed, clockwait_invalid_nsec)
{
	struct timespec ts;

	unnamed_init(0);

	clock_gettime(CLOCK_MONOTONIC, &ts);
	ts.tv_nsec = 1000000000L;

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, sem_clockwait(&sem, CLOCK_MONOTONIC, &ts));
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST(sem_unnamed, clockwait_wakes_on_post)
{
	pthread_t tid;
	struct timespec ts;
	int ret = -1;

	unnamed_init(0);

	TEST_ASSERT_EQUAL_INT(0, pthread_create(&tid, NULL, unnamed_clockwaiter, &ret));
	usleep(100 * 1000);
	TEST_ASSERT_EQUAL_INT(0, sem_post(&sem));
	TEST_ASSERT_EQUAL_INT(0, pthread_join(tid, NULL));
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* and the same semaphore still serves a timed wait afterwards */
	TEST_ASSERT_EQUAL_INT(0, sem_post(&sem));
	sem_test_deadlineOn(&ts, CLOCK_MONOTONIC, 500);
	TEST_ASSERT_EQUAL_INT(0, sem_clockwait(&sem, CLOCK_MONOTONIC, &ts));
}


TEST_GROUP_RUNNER(sem_unnamed)
{
	RUN_TEST_CASE(sem_unnamed, init_destroy);
	RUN_TEST_CASE(sem_unnamed, init_pshared_unsupported);
	RUN_TEST_CASE(sem_unnamed, init_null);
	RUN_TEST_CASE(sem_unnamed, init_value_too_large);
	RUN_TEST_CASE(sem_unnamed, init_value_max);
	RUN_TEST_CASE(sem_unnamed, getvalue_zero);
	RUN_TEST_CASE(sem_unnamed, getvalue_nonzero);
	RUN_TEST_CASE(sem_unnamed, wait_post_getvalue);
	RUN_TEST_CASE(sem_unnamed, trywait_success);
	RUN_TEST_CASE(sem_unnamed, trywait_empty);
	RUN_TEST_CASE(sem_unnamed, post_overflow);
	RUN_TEST_CASE(sem_unnamed, post_to_max);
	RUN_TEST_CASE(sem_unnamed, timedwait_success);
	RUN_TEST_CASE(sem_unnamed, timedwait_timeout);
	RUN_TEST_CASE(sem_unnamed, timedwait_expired_but_available);
	RUN_TEST_CASE(sem_unnamed, timedwait_expired_and_empty);
	RUN_TEST_CASE(sem_unnamed, timedwait_epoch_and_empty);
	RUN_TEST_CASE(sem_unnamed, timedwait_invalid_nsec);
	RUN_TEST_CASE(sem_unnamed, post_wakes_waiter);
	RUN_TEST_CASE(sem_unnamed, clockwait_monotonic_success);
	RUN_TEST_CASE(sem_unnamed, clockwait_realtime_success);
	RUN_TEST_CASE(sem_unnamed, clockwait_monotonic_timeout);
	RUN_TEST_CASE(sem_unnamed, clockwait_realtime_timeout);
	RUN_TEST_CASE(sem_unnamed, clockwait_clock_is_honoured);
	RUN_TEST_CASE(sem_unnamed, clockwait_expired_but_available);
	RUN_TEST_CASE(sem_unnamed, clockwait_expired_and_empty);
	RUN_TEST_CASE(sem_unnamed, clockwait_epoch_and_empty);
	RUN_TEST_CASE(sem_unnamed, clockwait_invalid_clock);
	RUN_TEST_CASE(sem_unnamed, clockwait_invalid_nsec);
	RUN_TEST_CASE(sem_unnamed, clockwait_wakes_on_post);
}
