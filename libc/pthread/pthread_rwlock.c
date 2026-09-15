/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <pthread.h>
 * TESTED:
 *    - pthread_rwlock_destroy()
 *    - pthread_rwlock_init()
 *    - pthread_rwlock_rdlock()
 *    - pthread_rwlock_tryrdlock()
 *    - pthread_rwlock_trywrlock()
 *    - pthread_rwlock_unlock()
 *    - pthread_rwlock_wrlock()
 *    - pthread_rwlock_timedrdlock()
 *    - pthread_rwlock_timedwrlock()
 *    - pthread_rwlock_clockrdlock()
 *    - pthread_rwlock_clockwrlock()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* pthread_rwlock_clockrdlock()/pthread_rwlock_clockwrlock() are GNU extensions on glibc hosts */
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


#ifdef HAS_PTHREAD_RWLOCK_RDLOCK
static struct {
	pthread_rwlock_t rwl;
} test_common;
#endif


TEST_GROUP(pthread_rwlock);


TEST_SETUP(pthread_rwlock)
{
#ifdef HAS_PTHREAD_RWLOCK_RDLOCK
	int ret;

	/*
	 * The HAS_ macro only proves the symbol exists in libc; on Phoenix the
	 * pthread_rwlock_* family is currently a stub returning ENOSYS. Detect
	 * that at runtime and skip the whole group instead of failing - once a
	 * real implementation lands, init returns 0 and the tests run for real.
	 */
	ret = pthread_rwlock_init(&test_common.rwl, NULL);
	if (ret == ENOSYS) {
		TEST_IGNORE_MESSAGE("pthread_rwlock is not implemented (ENOSYS)");
	}
	TEST_ASSERT_EQUAL_INT(0, ret);
#endif
}


TEST_TEAR_DOWN(pthread_rwlock)
{
#ifdef HAS_PTHREAD_RWLOCK_RDLOCK
	pthread_rwlock_destroy(&test_common.rwl);
#endif
}


/* pthread_rwlock_init: init returns 0 with NULL attr */
TEST(pthread_rwlock, init_default)
{
#ifndef HAS_PTHREAD_RWLOCK_RDLOCK
	TEST_IGNORE_MESSAGE("pthread_rwlock is not implemented (ENOSYS)");
#else
	pthread_rwlock_t rwl;
	int ret;

	ret = pthread_rwlock_init(&rwl, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	pthread_rwlock_destroy(&rwl);
#endif
}


/* pthread_rwlock_destroy: destroy returns 0 */
TEST(pthread_rwlock, destroy_initialized)
{
#ifndef HAS_PTHREAD_RWLOCK_RDLOCK
	TEST_IGNORE_MESSAGE("pthread_rwlock is not implemented (ENOSYS)");
#else
	pthread_rwlock_t rwl;
	int ret;

	ret = pthread_rwlock_init(&rwl, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlock_destroy(&rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);
#endif
}


/* pthread_rwlock_rdlock: acquire read lock on unlocked rwlock */
TEST(pthread_rwlock, rdlock_unlocked)
{
#ifndef HAS_PTHREAD_RWLOCK_RDLOCK
	TEST_IGNORE_MESSAGE("pthread_rwlock is not implemented (ENOSYS)");
#else
	int ret;

	ret = pthread_rwlock_rdlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlock_unlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);
#endif
}


/* pthread_rwlock_rdlock: multiple readers allowed */
TEST(pthread_rwlock, rdlock_multiple_readers)
{
#ifndef HAS_PTHREAD_RWLOCK_RDLOCK
	TEST_IGNORE_MESSAGE("pthread_rwlock is not implemented (ENOSYS)");
#else
	int ret;

	ret = pthread_rwlock_rdlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlock_rdlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlock_unlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlock_unlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);
#endif
}


/* pthread_rwlock_wrlock: acquire write lock on unlocked rwlock */
TEST(pthread_rwlock, wrlock_unlocked)
{
#ifndef HAS_PTHREAD_RWLOCK_RDLOCK
	TEST_IGNORE_MESSAGE("pthread_rwlock is not implemented (ENOSYS)");
#else
	int ret;

	ret = pthread_rwlock_wrlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlock_unlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);
#endif
}


/* pthread_rwlock_tryrdlock: success on unlocked rwlock */
TEST(pthread_rwlock, tryrdlock_unlocked)
{
#ifndef HAS_PTHREAD_RWLOCK_RDLOCK
	TEST_IGNORE_MESSAGE("pthread_rwlock is not implemented (ENOSYS)");
#else
	int ret;

	ret = pthread_rwlock_tryrdlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlock_unlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);
#endif
}


/* pthread_rwlock_tryrdlock: success when read-locked (multiple readers) */
TEST(pthread_rwlock, tryrdlock_while_rdlocked)
{
#ifndef HAS_PTHREAD_RWLOCK_RDLOCK
	TEST_IGNORE_MESSAGE("pthread_rwlock is not implemented (ENOSYS)");
#else
	int ret;

	ret = pthread_rwlock_rdlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlock_tryrdlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlock_unlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlock_unlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);
#endif
}


/* pthread_rwlock_tryrdlock: EBUSY when write-locked */
TEST(pthread_rwlock, tryrdlock_while_wrlocked)
{
#ifndef HAS_PTHREAD_RWLOCK_RDLOCK
	TEST_IGNORE_MESSAGE("pthread_rwlock is not implemented (ENOSYS)");
#else
	int ret;

	ret = pthread_rwlock_wrlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlock_tryrdlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(EBUSY, ret);

	ret = pthread_rwlock_unlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);
#endif
}


/* pthread_rwlock_trywrlock: success on unlocked rwlock */
TEST(pthread_rwlock, trywrlock_unlocked)
{
#ifndef HAS_PTHREAD_RWLOCK_RDLOCK
	TEST_IGNORE_MESSAGE("pthread_rwlock is not implemented (ENOSYS)");
#else
	int ret;

	ret = pthread_rwlock_trywrlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlock_unlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);
#endif
}


/* pthread_rwlock_trywrlock: EBUSY when read-locked */
TEST(pthread_rwlock, trywrlock_while_rdlocked)
{
#ifndef HAS_PTHREAD_RWLOCK_RDLOCK
	TEST_IGNORE_MESSAGE("pthread_rwlock is not implemented (ENOSYS)");
#else
	int ret;

	ret = pthread_rwlock_rdlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlock_trywrlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(EBUSY, ret);

	ret = pthread_rwlock_unlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);
#endif
}


/* pthread_rwlock_trywrlock: EBUSY when write-locked */
TEST(pthread_rwlock, trywrlock_while_wrlocked)
{
#ifndef HAS_PTHREAD_RWLOCK_RDLOCK
	TEST_IGNORE_MESSAGE("pthread_rwlock is not implemented (ENOSYS)");
#else
	int ret;

	ret = pthread_rwlock_wrlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlock_trywrlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(EBUSY, ret);

	ret = pthread_rwlock_unlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);
#endif
}


/* pthread_rwlock_unlock: unlock after rdlock */
TEST(pthread_rwlock, unlock_after_rdlock)
{
#ifndef HAS_PTHREAD_RWLOCK_RDLOCK
	TEST_IGNORE_MESSAGE("pthread_rwlock is not implemented (ENOSYS)");
#else
	int ret;

	ret = pthread_rwlock_rdlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlock_unlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Verify unlocked by acquiring write lock */
	ret = pthread_rwlock_trywrlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlock_unlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);
#endif
}


/* pthread_rwlock_unlock: unlock after wrlock */
TEST(pthread_rwlock, unlock_after_wrlock)
{
#ifndef HAS_PTHREAD_RWLOCK_RDLOCK
	TEST_IGNORE_MESSAGE("pthread_rwlock is not implemented (ENOSYS)");
#else
	int ret;

	ret = pthread_rwlock_wrlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlock_unlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Verify unlocked by acquiring read lock */
	ret = pthread_rwlock_tryrdlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlock_unlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);
#endif
}


/* pthread_rwlock_timedrdlock: success on unlocked rwlock */
TEST(pthread_rwlock, timedrdlock_unlocked)
{
#ifndef HAS_PTHREAD_RWLOCK_TIMEDRDLOCK
	TEST_IGNORE_MESSAGE("pthread_rwlock_timedrdlock/timedwrlock is not implemented");
#else
	struct timespec ts;
	int ret;

	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += 1;

	ret = pthread_rwlock_timedrdlock(&test_common.rwl, &ts);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlock_unlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);
#endif
}


/* pthread_rwlock_timedrdlock: success when already read-locked (multiple readers) */
TEST(pthread_rwlock, timedrdlock_while_rdlocked)
{
#ifndef HAS_PTHREAD_RWLOCK_TIMEDRDLOCK
	TEST_IGNORE_MESSAGE("pthread_rwlock_timedrdlock/timedwrlock is not implemented");
#else
	struct timespec ts;
	int ret;

	ret = pthread_rwlock_rdlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += 1;

	ret = pthread_rwlock_timedrdlock(&test_common.rwl, &ts);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlock_unlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlock_unlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);
#endif
}


#ifdef HAS_PTHREAD_RWLOCK_TIMEDRDLOCK
static void *writerHoldThread(void *arg)
{
	pthread_rwlock_t *rwl = (pthread_rwlock_t *)arg;

	pthread_rwlock_wrlock(rwl);
	/* Hold until cancelled */
	while (1) {
		usleep(10000);
		pthread_testcancel();
	}
	return NULL;
}
#endif


/* pthread_rwlock_timedrdlock: ETIMEDOUT when write-locked by another thread */
TEST(pthread_rwlock, timedrdlock_timeout)
{
#ifndef HAS_PTHREAD_RWLOCK_TIMEDRDLOCK
	TEST_IGNORE_MESSAGE("pthread_rwlock_timedrdlock/timedwrlock is not implemented");
#else
	struct timespec ts;
	pthread_t thr;
	int ret;

	ret = pthread_create(&thr, NULL, writerHoldThread, &test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Let thread acquire wrlock */
	usleep(20000);

	clock_gettime(CLOCK_REALTIME, &ts);
	/* Set timeout in the past to trigger immediate timeout */
	ts.tv_sec -= 1;

	ret = pthread_rwlock_timedrdlock(&test_common.rwl, &ts);
	TEST_ASSERT_EQUAL_INT(ETIMEDOUT, ret);

	pthread_cancel(thr);
	pthread_join(thr, NULL);
#endif
}


/* pthread_rwlock_timedwrlock: success on unlocked rwlock */
TEST(pthread_rwlock, timedwrlock_unlocked)
{
#ifndef HAS_PTHREAD_RWLOCK_TIMEDRDLOCK
	TEST_IGNORE_MESSAGE("pthread_rwlock_timedrdlock/timedwrlock is not implemented");
#else
	struct timespec ts;
	int ret;

	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += 1;

	ret = pthread_rwlock_timedwrlock(&test_common.rwl, &ts);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlock_unlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);
#endif
}


/* pthread_rwlock_timedwrlock: ETIMEDOUT when read-locked */
TEST(pthread_rwlock, timedwrlock_timeout_rdlocked)
{
#ifndef HAS_PTHREAD_RWLOCK_TIMEDRDLOCK
	TEST_IGNORE_MESSAGE("pthread_rwlock_timedrdlock/timedwrlock is not implemented");
#else
	struct timespec ts;
	int ret;

	ret = pthread_rwlock_rdlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec -= 1;

	ret = pthread_rwlock_timedwrlock(&test_common.rwl, &ts);
	TEST_ASSERT_EQUAL_INT(ETIMEDOUT, ret);

	ret = pthread_rwlock_unlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);
#endif
}


/* pthread_rwlock_timedwrlock: ETIMEDOUT when write-locked by another thread */
TEST(pthread_rwlock, timedwrlock_timeout_wrlocked)
{
#ifndef HAS_PTHREAD_RWLOCK_TIMEDRDLOCK
	TEST_IGNORE_MESSAGE("pthread_rwlock_timedrdlock/timedwrlock is not implemented");
#else
	struct timespec ts;
	pthread_t thr;
	int ret;

	ret = pthread_create(&thr, NULL, writerHoldThread, &test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Let thread acquire wrlock */
	usleep(20000);

	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec -= 1;

	ret = pthread_rwlock_timedwrlock(&test_common.rwl, &ts);
	TEST_ASSERT_EQUAL_INT(ETIMEDOUT, ret);

	pthread_cancel(thr);
	pthread_join(thr, NULL);
#endif
}


/* pthread_rwlock_timedrdlock: EINVAL for invalid timespec */
TEST(pthread_rwlock, timedrdlock_einval)
{
#ifndef HAS_PTHREAD_RWLOCK_TIMEDRDLOCK
	TEST_IGNORE_MESSAGE("pthread_rwlock_timedrdlock/timedwrlock is not implemented");
#else
	struct timespec ts;
	int ret;

	/*
	 * obtain wrlock beforehand - POSIX doesn't require the implementation to
	 * validate the timespec if the lock can be acquired immediately
	 */
	ret = pthread_rwlock_wrlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ts.tv_sec = 0;
	ts.tv_nsec = -1;

	ret = pthread_rwlock_timedrdlock(&test_common.rwl, &ts);
	TEST_ASSERT_EQUAL_INT(EINVAL, ret);

	ts.tv_nsec = 1000000000L;

	ret = pthread_rwlock_timedrdlock(&test_common.rwl, &ts);
	TEST_ASSERT_EQUAL_INT(EINVAL, ret);
#endif
}


/* pthread_rwlock_timedwrlock: EINVAL for invalid timespec */
TEST(pthread_rwlock, timedwrlock_einval)
{
#ifndef HAS_PTHREAD_RWLOCK_TIMEDRDLOCK
	TEST_IGNORE_MESSAGE("pthread_rwlock_timedrdlock/timedwrlock is not implemented");
#else
	struct timespec ts;
	int ret;

	/* see note in timedrdlock_einval */
	ret = pthread_rwlock_wrlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ts.tv_sec = 0;
	ts.tv_nsec = -1;

	ret = pthread_rwlock_timedwrlock(&test_common.rwl, &ts);
	TEST_ASSERT_EQUAL_INT(EINVAL, ret);

	ts.tv_nsec = 1000000000L;

	ret = pthread_rwlock_timedwrlock(&test_common.rwl, &ts);
	TEST_ASSERT_EQUAL_INT(EINVAL, ret);
#endif
}


#ifdef HAS_PTHREAD_RWLOCK_TIMEDRDLOCK
static void *writerThread(void *arg)
{
	pthread_rwlock_t *rwl = (pthread_rwlock_t *)arg;
	int ret;

	ret = pthread_rwlock_wrlock(rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	usleep(50000);

	ret = pthread_rwlock_unlock(rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	return NULL;
}
#endif


/* pthread_rwlock_timedrdlock: succeeds after writer releases */
TEST(pthread_rwlock, timedrdlock_waits_for_writer)
{
#ifndef HAS_PTHREAD_RWLOCK_TIMEDRDLOCK
	TEST_IGNORE_MESSAGE("pthread_rwlock_timedrdlock/timedwrlock is not implemented");
#else
	pthread_t thr;
	struct timespec ts;
	int ret;

	/* Writer thread acquires and holds the lock briefly */
	ret = pthread_create(&thr, NULL, writerThread, &test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Give writer time to acquire */
	usleep(10000);

	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += 2;

	/* This will block until the writer releases (~50ms) */
	ret = pthread_rwlock_timedrdlock(&test_common.rwl, &ts);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlock_unlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	pthread_join(thr, NULL);
#endif
}


#ifdef HAS_PTHREAD_RWLOCK_CLOCKRDLOCK
static void clockGetAbstime(clockid_t clockId, struct timespec *ts, long offsetMs)
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


static long clockElapsedMs(clockid_t clockId, const struct timespec *start)
{
	struct timespec now;
	int ret;

	ret = clock_gettime(clockId, &now);
	TEST_ASSERT_EQUAL_INT(0, ret);

	return (now.tv_sec - start->tv_sec) * 1000L + (now.tv_nsec - start->tv_nsec) / 1000000L;
}


/* Holds the write lock until cancelled */
static void *clockWriterHoldThread(void *arg)
{
	pthread_rwlock_t *rwl = (pthread_rwlock_t *)arg;

	pthread_rwlock_wrlock(rwl);
	while (1) {
		usleep(10000);
		pthread_testcancel();
	}
	return NULL;
}


/* Holds the write lock for a short while and releases it */
static void *clockWriterThread(void *arg)
{
	pthread_rwlock_t *rwl = (pthread_rwlock_t *)arg;

	pthread_rwlock_wrlock(rwl);
	usleep(50000);
	pthread_rwlock_unlock(rwl);

	return NULL;
}
#endif


/* pthread_rwlock_clockrdlock: success on unlocked rwlock with a CLOCK_REALTIME deadline */
TEST(pthread_rwlock, clockrdlock_unlocked)
{
#ifndef HAS_PTHREAD_RWLOCK_CLOCKRDLOCK
	TEST_IGNORE_MESSAGE("pthread_rwlock_clockrdlock/clockwrlock is not implemented");
#else
	struct timespec ts;
	int ret;

	clockGetAbstime(CLOCK_REALTIME, &ts, 1000);

	ret = pthread_rwlock_clockrdlock(&test_common.rwl, CLOCK_REALTIME, &ts);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlock_unlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);
#endif
}


/* pthread_rwlock_clockrdlock: success on unlocked rwlock with a CLOCK_MONOTONIC deadline */
TEST(pthread_rwlock, clockrdlock_monotonic_unlocked)
{
#ifndef HAS_PTHREAD_RWLOCK_CLOCKRDLOCK
	TEST_IGNORE_MESSAGE("pthread_rwlock_clockrdlock/clockwrlock is not implemented");
#else
	struct timespec ts;
	int ret;

	clockGetAbstime(CLOCK_MONOTONIC, &ts, 1000);

	ret = pthread_rwlock_clockrdlock(&test_common.rwl, CLOCK_MONOTONIC, &ts);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlock_unlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);
#endif
}


/* pthread_rwlock_clockrdlock: success when already read-locked (multiple readers) */
TEST(pthread_rwlock, clockrdlock_while_rdlocked)
{
#ifndef HAS_PTHREAD_RWLOCK_CLOCKRDLOCK
	TEST_IGNORE_MESSAGE("pthread_rwlock_clockrdlock/clockwrlock is not implemented");
#else
	struct timespec ts;
	int ret;

	ret = pthread_rwlock_rdlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	clockGetAbstime(CLOCK_MONOTONIC, &ts, 1000);

	ret = pthread_rwlock_clockrdlock(&test_common.rwl, CLOCK_MONOTONIC, &ts);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlock_unlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlock_unlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);
#endif
}


/* pthread_rwlock_clockrdlock: ETIMEDOUT not earlier than the CLOCK_MONOTONIC deadline */
TEST(pthread_rwlock, clockrdlock_timeout_monotonic)
{
#ifndef HAS_PTHREAD_RWLOCK_CLOCKRDLOCK
	TEST_IGNORE_MESSAGE("pthread_rwlock_clockrdlock/clockwrlock is not implemented");
#else
	struct timespec start, ts;
	pthread_t thr;
	int ret;

	ret = pthread_create(&thr, NULL, clockWriterHoldThread, &test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Let thread acquire wrlock */
	usleep(20000);

	ret = clock_gettime(CLOCK_MONOTONIC, &start);
	TEST_ASSERT_EQUAL_INT(0, ret);

	clockGetAbstime(CLOCK_MONOTONIC, &ts, 100);

	ret = pthread_rwlock_clockrdlock(&test_common.rwl, CLOCK_MONOTONIC, &ts);
	TEST_ASSERT_EQUAL_INT(ETIMEDOUT, ret);

	/* The call must not give up before the deadline */
	TEST_ASSERT_GREATER_OR_EQUAL_INT(90, clockElapsedMs(CLOCK_MONOTONIC, &start));

	pthread_cancel(thr);
	pthread_join(thr, NULL);
#endif
}


/* pthread_rwlock_clockrdlock: ETIMEDOUT when the deadline has already passed */
TEST(pthread_rwlock, clockrdlock_timeout_in_past)
{
#ifndef HAS_PTHREAD_RWLOCK_CLOCKRDLOCK
	TEST_IGNORE_MESSAGE("pthread_rwlock_clockrdlock/clockwrlock is not implemented");
#else
	struct timespec ts;
	pthread_t thr;
	int ret;

	ret = pthread_create(&thr, NULL, clockWriterHoldThread, &test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Let thread acquire wrlock */
	usleep(20000);

	clock_gettime(CLOCK_MONOTONIC, &ts);
	ts.tv_sec -= 1;

	ret = pthread_rwlock_clockrdlock(&test_common.rwl, CLOCK_MONOTONIC, &ts);
	TEST_ASSERT_EQUAL_INT(ETIMEDOUT, ret);

	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec -= 1;

	ret = pthread_rwlock_clockrdlock(&test_common.rwl, CLOCK_REALTIME, &ts);
	TEST_ASSERT_EQUAL_INT(ETIMEDOUT, ret);

	pthread_cancel(thr);
	pthread_join(thr, NULL);
#endif
}


/* pthread_rwlock_clockrdlock: succeeds after the writer releases the lock */
TEST(pthread_rwlock, clockrdlock_waits_for_writer)
{
#ifndef HAS_PTHREAD_RWLOCK_CLOCKRDLOCK
	TEST_IGNORE_MESSAGE("pthread_rwlock_clockrdlock/clockwrlock is not implemented");
#else
	struct timespec ts;
	pthread_t thr;
	int ret;

	ret = pthread_create(&thr, NULL, clockWriterThread, &test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Give writer time to acquire */
	usleep(10000);

	clockGetAbstime(CLOCK_MONOTONIC, &ts, 2000);

	/* This will block until the writer releases (~50ms) */
	ret = pthread_rwlock_clockrdlock(&test_common.rwl, CLOCK_MONOTONIC, &ts);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlock_unlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	pthread_join(thr, NULL);
#endif
}


/* pthread_rwlock_clockrdlock: EINVAL for invalid timespec */
TEST(pthread_rwlock, clockrdlock_einval_nsec)
{
#ifndef HAS_PTHREAD_RWLOCK_CLOCKRDLOCK
	TEST_IGNORE_MESSAGE("pthread_rwlock_clockrdlock/clockwrlock is not implemented");
#else
	struct timespec ts;
	int ret;

	/* see note in timedrdlock_einval */
	ret = pthread_rwlock_wrlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	clockGetAbstime(CLOCK_MONOTONIC, &ts, 100);
	ts.tv_nsec = -1;

	ret = pthread_rwlock_clockrdlock(&test_common.rwl, CLOCK_MONOTONIC, &ts);
	TEST_ASSERT_EQUAL_INT(EINVAL, ret);

	clockGetAbstime(CLOCK_MONOTONIC, &ts, 100);
	ts.tv_nsec = 1000000000L;

	ret = pthread_rwlock_clockrdlock(&test_common.rwl, CLOCK_MONOTONIC, &ts);
	TEST_ASSERT_EQUAL_INT(EINVAL, ret);
#endif
}


/* pthread_rwlock_clockrdlock: EINVAL for a clock that cannot be used for timeouts */
TEST(pthread_rwlock, clockrdlock_einval_clock)
{
#ifndef HAS_PTHREAD_RWLOCK_CLOCKRDLOCK
	TEST_IGNORE_MESSAGE("pthread_rwlock_clockrdlock/clockwrlock is not implemented");
#else
	struct timespec ts;
	int ret;

	/* see note in timedrdlock_einval */
	ret = pthread_rwlock_wrlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	clockGetAbstime(CLOCK_REALTIME, &ts, 100);

	ret = pthread_rwlock_clockrdlock(&test_common.rwl, TEST_INVALID_CLOCK, &ts);
	TEST_ASSERT_EQUAL_INT(EINVAL, ret);
#endif
}


/* pthread_rwlock_clockwrlock: success on unlocked rwlock with a CLOCK_REALTIME deadline */
TEST(pthread_rwlock, clockwrlock_unlocked)
{
#ifndef HAS_PTHREAD_RWLOCK_CLOCKRDLOCK
	TEST_IGNORE_MESSAGE("pthread_rwlock_clockrdlock/clockwrlock is not implemented");
#else
	struct timespec ts;
	int ret;

	clockGetAbstime(CLOCK_REALTIME, &ts, 1000);

	ret = pthread_rwlock_clockwrlock(&test_common.rwl, CLOCK_REALTIME, &ts);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlock_unlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);
#endif
}


/* pthread_rwlock_clockwrlock: success on unlocked rwlock with a CLOCK_MONOTONIC deadline */
TEST(pthread_rwlock, clockwrlock_monotonic_unlocked)
{
#ifndef HAS_PTHREAD_RWLOCK_CLOCKRDLOCK
	TEST_IGNORE_MESSAGE("pthread_rwlock_clockrdlock/clockwrlock is not implemented");
#else
	struct timespec ts;
	int ret;

	clockGetAbstime(CLOCK_MONOTONIC, &ts, 1000);

	ret = pthread_rwlock_clockwrlock(&test_common.rwl, CLOCK_MONOTONIC, &ts);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlock_unlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);
#endif
}


/* pthread_rwlock_clockwrlock: ETIMEDOUT not earlier than the CLOCK_MONOTONIC deadline when read-locked */
TEST(pthread_rwlock, clockwrlock_timeout_monotonic_rdlocked)
{
#ifndef HAS_PTHREAD_RWLOCK_CLOCKRDLOCK
	TEST_IGNORE_MESSAGE("pthread_rwlock_clockrdlock/clockwrlock is not implemented");
#else
	struct timespec start, ts;
	int ret;

	ret = pthread_rwlock_rdlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = clock_gettime(CLOCK_MONOTONIC, &start);
	TEST_ASSERT_EQUAL_INT(0, ret);

	clockGetAbstime(CLOCK_MONOTONIC, &ts, 100);

	ret = pthread_rwlock_clockwrlock(&test_common.rwl, CLOCK_MONOTONIC, &ts);
	TEST_ASSERT_EQUAL_INT(ETIMEDOUT, ret);

	/* The call must not give up before the deadline */
	TEST_ASSERT_GREATER_OR_EQUAL_INT(90, clockElapsedMs(CLOCK_MONOTONIC, &start));

	ret = pthread_rwlock_unlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);
#endif
}


/* pthread_rwlock_clockwrlock: ETIMEDOUT when the deadline has already passed */
TEST(pthread_rwlock, clockwrlock_timeout_in_past)
{
#ifndef HAS_PTHREAD_RWLOCK_CLOCKRDLOCK
	TEST_IGNORE_MESSAGE("pthread_rwlock_clockrdlock/clockwrlock is not implemented");
#else
	struct timespec ts;
	pthread_t thr;
	int ret;

	ret = pthread_create(&thr, NULL, clockWriterHoldThread, &test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Let thread acquire wrlock */
	usleep(20000);

	clock_gettime(CLOCK_MONOTONIC, &ts);
	ts.tv_sec -= 1;

	ret = pthread_rwlock_clockwrlock(&test_common.rwl, CLOCK_MONOTONIC, &ts);
	TEST_ASSERT_EQUAL_INT(ETIMEDOUT, ret);

	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec -= 1;

	ret = pthread_rwlock_clockwrlock(&test_common.rwl, CLOCK_REALTIME, &ts);
	TEST_ASSERT_EQUAL_INT(ETIMEDOUT, ret);

	pthread_cancel(thr);
	pthread_join(thr, NULL);
#endif
}


/* pthread_rwlock_clockwrlock: succeeds after the writer releases the lock */
TEST(pthread_rwlock, clockwrlock_waits_for_writer)
{
#ifndef HAS_PTHREAD_RWLOCK_CLOCKRDLOCK
	TEST_IGNORE_MESSAGE("pthread_rwlock_clockrdlock/clockwrlock is not implemented");
#else
	struct timespec ts;
	pthread_t thr;
	int ret;

	ret = pthread_create(&thr, NULL, clockWriterThread, &test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Give writer time to acquire */
	usleep(10000);

	clockGetAbstime(CLOCK_MONOTONIC, &ts, 2000);

	/* This will block until the writer releases (~50ms) */
	ret = pthread_rwlock_clockwrlock(&test_common.rwl, CLOCK_MONOTONIC, &ts);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlock_unlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	pthread_join(thr, NULL);
#endif
}


/* pthread_rwlock_clockwrlock: EINVAL for invalid timespec */
TEST(pthread_rwlock, clockwrlock_einval_nsec)
{
#ifndef HAS_PTHREAD_RWLOCK_CLOCKRDLOCK
	TEST_IGNORE_MESSAGE("pthread_rwlock_clockrdlock/clockwrlock is not implemented");
#else
	struct timespec ts;
	int ret;

	/* see note in timedrdlock_einval */
	ret = pthread_rwlock_wrlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	clockGetAbstime(CLOCK_MONOTONIC, &ts, 100);
	ts.tv_nsec = -1;

	ret = pthread_rwlock_clockwrlock(&test_common.rwl, CLOCK_MONOTONIC, &ts);
	TEST_ASSERT_EQUAL_INT(EINVAL, ret);

	clockGetAbstime(CLOCK_MONOTONIC, &ts, 100);
	ts.tv_nsec = 1000000000L;

	ret = pthread_rwlock_clockwrlock(&test_common.rwl, CLOCK_MONOTONIC, &ts);
	TEST_ASSERT_EQUAL_INT(EINVAL, ret);
#endif
}


/* pthread_rwlock_clockwrlock: EINVAL for a clock that cannot be used for timeouts */
TEST(pthread_rwlock, clockwrlock_einval_clock)
{
#ifndef HAS_PTHREAD_RWLOCK_CLOCKRDLOCK
	TEST_IGNORE_MESSAGE("pthread_rwlock_clockrdlock/clockwrlock is not implemented");
#else
	struct timespec ts;
	int ret;

	/* see note in timedrdlock_einval */
	ret = pthread_rwlock_wrlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	clockGetAbstime(CLOCK_REALTIME, &ts, 100);

	ret = pthread_rwlock_clockwrlock(&test_common.rwl, TEST_INVALID_CLOCK, &ts);
	TEST_ASSERT_EQUAL_INT(EINVAL, ret);
#endif
}


TEST_GROUP_RUNNER(pthread_rwlock)
{
	RUN_TEST_CASE(pthread_rwlock, init_default);
	RUN_TEST_CASE(pthread_rwlock, destroy_initialized);
	RUN_TEST_CASE(pthread_rwlock, rdlock_unlocked);
	RUN_TEST_CASE(pthread_rwlock, rdlock_multiple_readers);
	RUN_TEST_CASE(pthread_rwlock, wrlock_unlocked);
	RUN_TEST_CASE(pthread_rwlock, tryrdlock_unlocked);
	RUN_TEST_CASE(pthread_rwlock, tryrdlock_while_rdlocked);
	RUN_TEST_CASE(pthread_rwlock, tryrdlock_while_wrlocked);
	RUN_TEST_CASE(pthread_rwlock, trywrlock_unlocked);
	RUN_TEST_CASE(pthread_rwlock, trywrlock_while_rdlocked);
	RUN_TEST_CASE(pthread_rwlock, trywrlock_while_wrlocked);
	RUN_TEST_CASE(pthread_rwlock, unlock_after_rdlock);
	RUN_TEST_CASE(pthread_rwlock, unlock_after_wrlock);
	RUN_TEST_CASE(pthread_rwlock, timedrdlock_unlocked);
	RUN_TEST_CASE(pthread_rwlock, timedrdlock_while_rdlocked);
	RUN_TEST_CASE(pthread_rwlock, timedrdlock_timeout);
	RUN_TEST_CASE(pthread_rwlock, timedwrlock_unlocked);
	RUN_TEST_CASE(pthread_rwlock, timedwrlock_timeout_rdlocked);
	RUN_TEST_CASE(pthread_rwlock, timedwrlock_timeout_wrlocked);
	RUN_TEST_CASE(pthread_rwlock, timedrdlock_einval);
	RUN_TEST_CASE(pthread_rwlock, timedwrlock_einval);
	RUN_TEST_CASE(pthread_rwlock, timedrdlock_waits_for_writer);
	RUN_TEST_CASE(pthread_rwlock, clockrdlock_unlocked);
	RUN_TEST_CASE(pthread_rwlock, clockrdlock_monotonic_unlocked);
	RUN_TEST_CASE(pthread_rwlock, clockrdlock_while_rdlocked);
	RUN_TEST_CASE(pthread_rwlock, clockrdlock_timeout_monotonic);
	RUN_TEST_CASE(pthread_rwlock, clockrdlock_timeout_in_past);
	RUN_TEST_CASE(pthread_rwlock, clockrdlock_waits_for_writer);
	RUN_TEST_CASE(pthread_rwlock, clockrdlock_einval_nsec);
	RUN_TEST_CASE(pthread_rwlock, clockrdlock_einval_clock);
	RUN_TEST_CASE(pthread_rwlock, clockwrlock_unlocked);
	RUN_TEST_CASE(pthread_rwlock, clockwrlock_monotonic_unlocked);
	RUN_TEST_CASE(pthread_rwlock, clockwrlock_timeout_monotonic_rdlocked);
	RUN_TEST_CASE(pthread_rwlock, clockwrlock_timeout_in_past);
	RUN_TEST_CASE(pthread_rwlock, clockwrlock_waits_for_writer);
	RUN_TEST_CASE(pthread_rwlock, clockwrlock_einval_nsec);
	RUN_TEST_CASE(pthread_rwlock, clockwrlock_einval_clock);
}
