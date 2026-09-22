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


TEST_GROUP(sem_unnamed);


TEST_SETUP(sem_unnamed)
{
	inited = 0;
}


TEST_TEAR_DOWN(sem_unnamed)
{
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
}
