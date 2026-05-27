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
#include <time.h>
#include <unistd.h>

#include "unity_fixture.h"


static struct {
	pthread_rwlock_t rwl;
} test_common;


TEST_GROUP(pthread_rwlock);


TEST_SETUP(pthread_rwlock)
{
	int ret;

	ret = pthread_rwlock_init(&test_common.rwl, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST_TEAR_DOWN(pthread_rwlock)
{
	pthread_rwlock_destroy(&test_common.rwl);
}


/* pthread_rwlock_init: init returns 0 with NULL attr */
TEST(pthread_rwlock, init_default)
{
	pthread_rwlock_t rwl;
	int ret;

	ret = pthread_rwlock_init(&rwl, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	pthread_rwlock_destroy(&rwl);
}


/* pthread_rwlock_destroy: destroy returns 0 */
TEST(pthread_rwlock, destroy_initialized)
{
	pthread_rwlock_t rwl;
	int ret;

	ret = pthread_rwlock_init(&rwl, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlock_destroy(&rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


/* pthread_rwlock_rdlock: acquire read lock on unlocked rwlock */
TEST(pthread_rwlock, rdlock_unlocked)
{
	int ret;

	ret = pthread_rwlock_rdlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlock_unlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


/* pthread_rwlock_rdlock: multiple readers allowed */
TEST(pthread_rwlock, rdlock_multiple_readers)
{
	int ret;

	ret = pthread_rwlock_rdlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlock_rdlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlock_unlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlock_unlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


/* pthread_rwlock_wrlock: acquire write lock on unlocked rwlock */
TEST(pthread_rwlock, wrlock_unlocked)
{
	int ret;

	ret = pthread_rwlock_wrlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlock_unlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


/* pthread_rwlock_tryrdlock: success on unlocked rwlock */
TEST(pthread_rwlock, tryrdlock_unlocked)
{
	int ret;

	ret = pthread_rwlock_tryrdlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlock_unlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


/* pthread_rwlock_tryrdlock: success when read-locked (multiple readers) */
TEST(pthread_rwlock, tryrdlock_while_rdlocked)
{
	int ret;

	ret = pthread_rwlock_rdlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlock_tryrdlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlock_unlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlock_unlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


/* pthread_rwlock_tryrdlock: EBUSY when write-locked */
TEST(pthread_rwlock, tryrdlock_while_wrlocked)
{
	int ret;

	ret = pthread_rwlock_wrlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlock_tryrdlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(EBUSY, ret);

	ret = pthread_rwlock_unlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


/* pthread_rwlock_trywrlock: success on unlocked rwlock */
TEST(pthread_rwlock, trywrlock_unlocked)
{
	int ret;

	ret = pthread_rwlock_trywrlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlock_unlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


/* pthread_rwlock_trywrlock: EBUSY when read-locked */
TEST(pthread_rwlock, trywrlock_while_rdlocked)
{
	int ret;

	ret = pthread_rwlock_rdlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlock_trywrlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(EBUSY, ret);

	ret = pthread_rwlock_unlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


/* pthread_rwlock_trywrlock: EBUSY when write-locked */
TEST(pthread_rwlock, trywrlock_while_wrlocked)
{
	int ret;

	ret = pthread_rwlock_wrlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlock_trywrlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(EBUSY, ret);

	ret = pthread_rwlock_unlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


/* pthread_rwlock_unlock: unlock after rdlock */
TEST(pthread_rwlock, unlock_after_rdlock)
{
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
}


/* pthread_rwlock_unlock: unlock after wrlock */
TEST(pthread_rwlock, unlock_after_wrlock)
{
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
}


/* pthread_rwlock_timedrdlock: success on unlocked rwlock */
TEST(pthread_rwlock, timedrdlock_unlocked)
{
	struct timespec ts;
	int ret;

	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += 1;

	ret = pthread_rwlock_timedrdlock(&test_common.rwl, &ts);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlock_unlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


/* pthread_rwlock_timedrdlock: success when already read-locked (multiple readers) */
TEST(pthread_rwlock, timedrdlock_while_rdlocked)
{
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
}


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


/* pthread_rwlock_timedrdlock: ETIMEDOUT when write-locked by another thread */
TEST(pthread_rwlock, timedrdlock_timeout)
{
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
}


/* pthread_rwlock_timedwrlock: success on unlocked rwlock */
TEST(pthread_rwlock, timedwrlock_unlocked)
{
	struct timespec ts;
	int ret;

	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += 1;

	ret = pthread_rwlock_timedwrlock(&test_common.rwl, &ts);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlock_unlock(&test_common.rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


/* pthread_rwlock_timedwrlock: ETIMEDOUT when read-locked */
TEST(pthread_rwlock, timedwrlock_timeout_rdlocked)
{
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
}


/* pthread_rwlock_timedwrlock: ETIMEDOUT when write-locked by another thread */
TEST(pthread_rwlock, timedwrlock_timeout_wrlocked)
{
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
}


/* pthread_rwlock_timedrdlock: EINVAL for invalid timespec */
TEST(pthread_rwlock, timedrdlock_einval)
{
	struct timespec ts;
	int ret;

	ts.tv_sec = 0;
	ts.tv_nsec = -1;

	ret = pthread_rwlock_timedrdlock(&test_common.rwl, &ts);
	TEST_ASSERT_EQUAL_INT(EINVAL, ret);

	ts.tv_nsec = 1000000000L;

	ret = pthread_rwlock_timedrdlock(&test_common.rwl, &ts);
	TEST_ASSERT_EQUAL_INT(EINVAL, ret);
}


/* pthread_rwlock_timedwrlock: EINVAL for invalid timespec */
TEST(pthread_rwlock, timedwrlock_einval)
{
	struct timespec ts;
	int ret;

	ts.tv_sec = 0;
	ts.tv_nsec = -1;

	ret = pthread_rwlock_timedwrlock(&test_common.rwl, &ts);
	TEST_ASSERT_EQUAL_INT(EINVAL, ret);

	ts.tv_nsec = 1000000000L;

	ret = pthread_rwlock_timedwrlock(&test_common.rwl, &ts);
	TEST_ASSERT_EQUAL_INT(EINVAL, ret);
}


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


/* pthread_rwlock_timedrdlock: succeeds after writer releases */
TEST(pthread_rwlock, timedrdlock_waits_for_writer)
{
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
}
