/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <pthread.h>
 * TESTED:
 *    - pthread_mutex_consistent()
 *    - pthread_mutex_getprioceiling()
 *    - pthread_mutex_setprioceiling()
 *    - pthread_mutex_timedlock()
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
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sched.h>

#include "unity_fixture.h"
#include "libc_features.h"


/* ===== pthread_mutex_consistent group ===== */


TEST_GROUP(pthread_mutex_consistent);


TEST_SETUP(pthread_mutex_consistent)
{
}


TEST_TEAR_DOWN(pthread_mutex_consistent)
{
}


/* pthread_mutex_consistent: EINVAL on non-robust mutex */
TEST(pthread_mutex_consistent, einval_non_robust)
{
#ifndef HAS_PTHREAD_MUTEX_CONSISTENT
	TEST_IGNORE_MESSAGE("pthread_mutex_consistent is not implemented");
#else
	pthread_mutex_t mtx;
	pthread_mutexattr_t mattr;
	int ret;

	ret = pthread_mutexattr_init(&mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Default is PTHREAD_MUTEX_STALLED (non-robust) */
	ret = pthread_mutex_init(&mtx, &mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutex_lock(&mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Calling consistent on a non-robust mutex that is not inconsistent */
	ret = pthread_mutex_consistent(&mtx);
	TEST_ASSERT_EQUAL_INT(EINVAL, ret);

	ret = pthread_mutex_unlock(&mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);

	pthread_mutex_destroy(&mtx);
	pthread_mutexattr_destroy(&mattr);
#endif
}


/* pthread_mutex_consistent: EINVAL on robust mutex that is not inconsistent */
TEST(pthread_mutex_consistent, einval_robust_not_inconsistent)
{
#ifndef HAS_PTHREAD_MUTEX_CONSISTENT
	TEST_IGNORE_MESSAGE("pthread_mutex_consistent is not implemented");
#else
	pthread_mutex_t mtx;
	pthread_mutexattr_t mattr;
	int ret;

	ret = pthread_mutexattr_init(&mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutexattr_setrobust(&mattr, PTHREAD_MUTEX_ROBUST);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutex_init(&mtx, &mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutex_lock(&mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Mutex is locked but not in inconsistent state */
	ret = pthread_mutex_consistent(&mtx);
	TEST_ASSERT_EQUAL_INT(EINVAL, ret);

	ret = pthread_mutex_unlock(&mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);

	pthread_mutex_destroy(&mtx);
	pthread_mutexattr_destroy(&mattr);
#endif
}


TEST_GROUP_RUNNER(pthread_mutex_consistent)
{
	RUN_TEST_CASE(pthread_mutex_consistent, einval_non_robust);
	RUN_TEST_CASE(pthread_mutex_consistent, einval_robust_not_inconsistent);
}


/* ===== pthread_mutex_prioceiling group ===== */
/* Tests: pthread_mutex_getprioceiling, pthread_mutex_setprioceiling */


TEST_GROUP(pthread_mutex_prioceiling);


TEST_SETUP(pthread_mutex_prioceiling)
{
}


TEST_TEAR_DOWN(pthread_mutex_prioceiling)
{
}


/* pthread_mutex_getprioceiling: EINVAL when protocol is PTHREAD_PRIO_NONE */
TEST(pthread_mutex_prioceiling, getprioceiling_einval_prio_none)
{
#ifndef HAS_PTHREAD_MUTEX_GETPRIOCEILING
	TEST_IGNORE_MESSAGE("pthread_mutex_getprioceiling is not implemented");
#else
	pthread_mutex_t mtx;
	pthread_mutexattr_t mattr;
	int prioceiling;
	int ret;

	ret = pthread_mutexattr_init(&mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* POSIX-DEVIATION: Default protocol is PTHREAD_PRIO_INHERIT in Phoenix, hence the setprotocol */
	ret = pthread_mutexattr_setprotocol(&mattr, PTHREAD_PRIO_NONE);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutex_init(&mtx, &mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutex_getprioceiling(&mtx, &prioceiling);
	/* POSIX: shall fail with EINVAL if protocol is PTHREAD_PRIO_NONE */
	TEST_ASSERT_EQUAL_INT(EINVAL, ret);

	pthread_mutex_destroy(&mtx);
	pthread_mutexattr_destroy(&mattr);
#endif
}


/* pthread_mutex_getprioceiling: returns ceiling for PTHREAD_PRIO_PROTECT mutex */
TEST(pthread_mutex_prioceiling, getprioceiling_protect)
{
#ifndef HAS_PTHREAD_MUTEX_GETPRIOCEILING
	TEST_IGNORE_MESSAGE("pthread_mutex_getprioceiling is not implemented");
#else
	pthread_mutex_t mtx;
	pthread_mutexattr_t mattr;
	int prioceiling;
	int maxPrio;
	int ret;

	ret = pthread_mutexattr_init(&mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutexattr_setprotocol(&mattr, PTHREAD_PRIO_PROTECT);
	if (ret == ENOTSUP) {
		pthread_mutexattr_destroy(&mattr);
		TEST_IGNORE_MESSAGE("PTHREAD_PRIO_PROTECT not supported");
	}
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* POSIX-DEVIATION: Kernel has SCHED_RR only */
	maxPrio = sched_get_priority_max(SCHED_RR);
	TEST_ASSERT_TRUE(maxPrio >= 0);

	ret = pthread_mutexattr_setprioceiling(&mattr, maxPrio);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutex_init(&mtx, &mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutex_getprioceiling(&mtx, &prioceiling);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(maxPrio, prioceiling);

	pthread_mutex_destroy(&mtx);
	pthread_mutexattr_destroy(&mattr);
#endif
}


/* pthread_mutex_setprioceiling: changes ceiling and returns old value */
TEST(pthread_mutex_prioceiling, setprioceiling_changes_ceiling)
{
#ifndef HAS_PTHREAD_MUTEX_SETPRIOCEILING
	TEST_IGNORE_MESSAGE("pthread_mutex_setprioceiling is not implemented");
#else
	pthread_mutex_t mtx;
	pthread_mutexattr_t mattr;
	int oldCeiling;
	int newCeiling;
	int maxPrio;
	int minPrio;
	int ret;

	ret = pthread_mutexattr_init(&mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutexattr_setprotocol(&mattr, PTHREAD_PRIO_PROTECT);
	if (ret == ENOTSUP) {
		pthread_mutexattr_destroy(&mattr);
		TEST_IGNORE_MESSAGE("PTHREAD_PRIO_PROTECT not supported");
	}
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* POSIX-DEVIATION: Kernel has SCHED_RR only */
	maxPrio = sched_get_priority_max(SCHED_RR);
	minPrio = sched_get_priority_min(SCHED_RR);
	TEST_ASSERT_TRUE(maxPrio >= minPrio);

	ret = pthread_mutexattr_setprioceiling(&mattr, maxPrio);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutex_init(&mtx, &mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Change ceiling to minPrio */
	ret = pthread_mutex_setprioceiling(&mtx, minPrio, &oldCeiling);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(maxPrio, oldCeiling);

	/* Verify new ceiling */
	ret = pthread_mutex_getprioceiling(&mtx, &newCeiling);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(minPrio, newCeiling);

	pthread_mutex_destroy(&mtx);
	pthread_mutexattr_destroy(&mattr);
#endif
}


/* pthread_mutex_setprioceiling: EINVAL when protocol is PTHREAD_PRIO_NONE */
TEST(pthread_mutex_prioceiling, setprioceiling_einval_prio_none)
{
#ifndef HAS_PTHREAD_MUTEX_SETPRIOCEILING
	TEST_IGNORE_MESSAGE("pthread_mutex_setprioceiling is not implemented");
#else
	pthread_mutex_t mtx;
	pthread_mutexattr_t mattr;
	int oldCeiling;
	int ret;

	ret = pthread_mutexattr_init(&mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* POSIX-DEVIATION: Default protocol is PTHREAD_PRIO_INHERIT in Phoenix, hence the setprotocol */
	ret = pthread_mutexattr_setprotocol(&mattr, PTHREAD_PRIO_NONE);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutex_init(&mtx, &mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutex_setprioceiling(&mtx, 1, &oldCeiling);
	TEST_ASSERT_EQUAL_INT(EINVAL, ret);

	pthread_mutex_destroy(&mtx);
#endif
}


TEST_GROUP_RUNNER(pthread_mutex_prioceiling)
{
	RUN_TEST_CASE(pthread_mutex_prioceiling, getprioceiling_einval_prio_none);
	RUN_TEST_CASE(pthread_mutex_prioceiling, getprioceiling_protect);
	RUN_TEST_CASE(pthread_mutex_prioceiling, setprioceiling_changes_ceiling);
	RUN_TEST_CASE(pthread_mutex_prioceiling, setprioceiling_einval_prio_none);
}


/* ===== pthread_mutex_timedlock group ===== */


TEST_GROUP(pthread_mutex_timedlock);


TEST_SETUP(pthread_mutex_timedlock)
{
}


TEST_TEAR_DOWN(pthread_mutex_timedlock)
{
}


#ifdef HAS_PTHREAD_MUTEX_TIMEDLOCK
static void test_getAbstime(struct timespec *ts, long offsetMs)
{
	int ret;

	ret = clock_gettime(CLOCK_REALTIME, ts);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ts->tv_nsec += offsetMs * 1000000L;
	while (ts->tv_nsec >= 1000000000L) {
		ts->tv_sec++;
		ts->tv_nsec -= 1000000000L;
	}
}
#endif


/* pthread_mutex_timedlock: acquire unlocked mutex immediately, return 0 */
TEST(pthread_mutex_timedlock, lock_unlocked_immediately)
{
#ifndef HAS_PTHREAD_MUTEX_TIMEDLOCK
	TEST_IGNORE_MESSAGE("pthread_mutex_timedlock is not implemented");
#else
	pthread_mutex_t mtx;
	struct timespec ts;
	int ret;

	ret = pthread_mutex_init(&mtx, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	test_getAbstime(&ts, 1000);

	ret = pthread_mutex_timedlock(&mtx, &ts);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutex_unlock(&mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);

	pthread_mutex_destroy(&mtx);
#endif
}


/* pthread_mutex_timedlock: ETIMEDOUT when mutex held by another thread */
#ifdef HAS_PTHREAD_MUTEX_TIMEDLOCK
static void *test_timedlockHolder(void *arg)
{
	pthread_mutex_t *mtx = (pthread_mutex_t *)arg;

	pthread_mutex_lock(mtx);
	/* Hold lock for 200ms */
	usleep(200000);
	pthread_mutex_unlock(mtx);
	return NULL;
}
#endif


TEST(pthread_mutex_timedlock, etimedout_held_by_other)
{
#ifndef HAS_PTHREAD_MUTEX_TIMEDLOCK
	TEST_IGNORE_MESSAGE("pthread_mutex_timedlock is not implemented");
#else
	pthread_mutex_t mtx;
	pthread_t thread;
	struct timespec ts;
	int ret;

	ret = pthread_mutex_init(&mtx, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_create(&thread, NULL, test_timedlockHolder, &mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Give thread time to acquire lock */
	usleep(20000);

	/* Set timeout to 50ms from now — much less than 200ms hold time */
	test_getAbstime(&ts, 50);

	ret = pthread_mutex_timedlock(&mtx, &ts);
	TEST_ASSERT_EQUAL_INT(ETIMEDOUT, ret);

	ret = pthread_join(thread, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	pthread_mutex_destroy(&mtx);
#endif
}


/* pthread_mutex_timedlock: succeed if lock becomes available before timeout */
#ifdef HAS_PTHREAD_MUTEX_TIMEDLOCK
static void *test_timedlockShortHolder(void *arg)
{
	pthread_mutex_t *mtx = (pthread_mutex_t *)arg;

	pthread_mutex_lock(mtx);
	/* Hold lock for only 20ms */
	usleep(20000);
	pthread_mutex_unlock(mtx);
	return NULL;
}
#endif


TEST(pthread_mutex_timedlock, succeeds_before_timeout)
{
#ifndef HAS_PTHREAD_MUTEX_TIMEDLOCK
	TEST_IGNORE_MESSAGE("pthread_mutex_timedlock is not implemented");
#else
	pthread_mutex_t mtx;
	pthread_t thread;
	struct timespec ts;
	int ret;

	ret = pthread_mutex_init(&mtx, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_create(&thread, NULL, test_timedlockShortHolder, &mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);

	usleep(5000);

	/* Set timeout to 500ms — lock should be released in ~20ms */
	test_getAbstime(&ts, 500);

	ret = pthread_mutex_timedlock(&mtx, &ts);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutex_unlock(&mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_join(thread, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	pthread_mutex_destroy(&mtx);
#endif
}


/* pthread_mutex_timedlock: ETIMEDOUT when abstime already passed */
TEST(pthread_mutex_timedlock, etimedout_abstime_in_past)
{
#ifndef HAS_PTHREAD_MUTEX_TIMEDLOCK
	TEST_IGNORE_MESSAGE("pthread_mutex_timedlock is not implemented");
#else
	pthread_mutex_t mtx;
	pthread_t thread;
	struct timespec ts;
	int ret;

	ret = pthread_mutex_init(&mtx, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_create(&thread, NULL, test_timedlockHolder, &mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);

	usleep(20000);

	/* Set abstime in the past */
	ts.tv_sec = 0;
	ts.tv_nsec = 0;

	ret = pthread_mutex_timedlock(&mtx, &ts);
	TEST_ASSERT_EQUAL_INT(ETIMEDOUT, ret);

	ret = pthread_join(thread, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	pthread_mutex_destroy(&mtx);
#endif
}


/* pthread_mutex_timedlock: EINVAL for invalid nanosecond value */
TEST(pthread_mutex_timedlock, einval_invalid_nsec)
{
#ifndef HAS_PTHREAD_MUTEX_TIMEDLOCK
	TEST_IGNORE_MESSAGE("pthread_mutex_timedlock is not implemented");
#else
	pthread_mutex_t mtx;
	pthread_t thread;
	struct timespec ts;
	int ret;

	ret = pthread_mutex_init(&mtx, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_create(&thread, NULL, test_timedlockHolder, &mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);

	usleep(20000);

	/* Negative nanoseconds */
	test_getAbstime(&ts, 100);
	ts.tv_nsec = -1;

	ret = pthread_mutex_timedlock(&mtx, &ts);
	TEST_ASSERT_EQUAL_INT(EINVAL, ret);

	/* nsec >= 1000000000 */
	test_getAbstime(&ts, 100);
	ts.tv_nsec = 1000000000L;

	ret = pthread_mutex_timedlock(&mtx, &ts);
	TEST_ASSERT_EQUAL_INT(EINVAL, ret);

	ret = pthread_join(thread, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	pthread_mutex_destroy(&mtx);
#endif
}


/* pthread_mutex_timedlock: EDEADLK for ERRORCHECK mutex relock */
TEST(pthread_mutex_timedlock, edeadlk_errorcheck)
{
#ifndef HAS_PTHREAD_MUTEX_TIMEDLOCK
	TEST_IGNORE_MESSAGE("pthread_mutex_timedlock is not implemented");
#else
	pthread_mutex_t mtx;
	pthread_mutexattr_t mattr;
	struct timespec ts;
	int ret;

	ret = pthread_mutexattr_init(&mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_ERRORCHECK);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutex_init(&mtx, &mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutex_lock(&mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);

	test_getAbstime(&ts, 100);

	ret = pthread_mutex_timedlock(&mtx, &ts);
	TEST_ASSERT_EQUAL_INT(EDEADLK, ret);

	ret = pthread_mutex_unlock(&mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);

	pthread_mutex_destroy(&mtx);
	pthread_mutexattr_destroy(&mattr);
#endif
}


/* pthread_mutex_timedlock: shall not fail with timeout if mutex can be locked immediately */
TEST(pthread_mutex_timedlock, no_timeout_if_immediately_lockable)
{
#ifndef HAS_PTHREAD_MUTEX_TIMEDLOCK
	TEST_IGNORE_MESSAGE("pthread_mutex_timedlock is not implemented");
#else
	pthread_mutex_t mtx;
	struct timespec ts;
	int ret;

	ret = pthread_mutex_init(&mtx, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* abstime in the past — but lock is available, so must succeed */
	ts.tv_sec = 0;
	ts.tv_nsec = 0;

	ret = pthread_mutex_timedlock(&mtx, &ts);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutex_unlock(&mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);

	pthread_mutex_destroy(&mtx);
#endif
}


TEST_GROUP_RUNNER(pthread_mutex_timedlock)
{
	RUN_TEST_CASE(pthread_mutex_timedlock, lock_unlocked_immediately);
	RUN_TEST_CASE(pthread_mutex_timedlock, etimedout_held_by_other);
	RUN_TEST_CASE(pthread_mutex_timedlock, succeeds_before_timeout);
	RUN_TEST_CASE(pthread_mutex_timedlock, etimedout_abstime_in_past);
	RUN_TEST_CASE(pthread_mutex_timedlock, einval_invalid_nsec);
	RUN_TEST_CASE(pthread_mutex_timedlock, edeadlk_errorcheck);
	RUN_TEST_CASE(pthread_mutex_timedlock, no_timeout_if_immediately_lockable);
}
