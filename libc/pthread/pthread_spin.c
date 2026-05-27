/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <pthread.h>
 * TESTED:
 *    - pthread_spin_destroy()
 *    - pthread_spin_init()
 *    - pthread_spin_lock()
 *    - pthread_spin_trylock()
 *    - pthread_spin_unlock()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Daniel Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <pthread.h>
#include <errno.h>
#include <unistd.h>

#include "unity_fixture.h"


static struct {
	pthread_spinlock_t spin;
} test_common;


TEST_GROUP(pthread_spin);


TEST_SETUP(pthread_spin)
{
	int ret;

	ret = pthread_spin_init(&test_common.spin, PTHREAD_PROCESS_PRIVATE);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST_TEAR_DOWN(pthread_spin)
{
	pthread_spin_destroy(&test_common.spin);
}


/* pthread_spin_init: init with PTHREAD_PROCESS_PRIVATE returns 0 */
TEST(pthread_spin, init_private)
{
	pthread_spinlock_t spin;
	int ret;

	ret = pthread_spin_init(&spin, PTHREAD_PROCESS_PRIVATE);
	TEST_ASSERT_EQUAL_INT(0, ret);

	pthread_spin_destroy(&spin);
}


/* pthread_spin_init: init with PTHREAD_PROCESS_SHARED returns 0 */
TEST(pthread_spin, init_shared)
{
	pthread_spinlock_t spin;
	int ret;

	ret = pthread_spin_init(&spin, PTHREAD_PROCESS_SHARED);
	TEST_ASSERT_EQUAL_INT(0, ret);

	pthread_spin_destroy(&spin);
}


/* pthread_spin_destroy: destroy returns 0 */
TEST(pthread_spin, destroy_success)
{
	pthread_spinlock_t spin;
	int ret;

	ret = pthread_spin_init(&spin, PTHREAD_PROCESS_PRIVATE);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_spin_destroy(&spin);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


/* pthread_spin_lock: lock on unlocked spinlock returns 0 */
TEST(pthread_spin, lock_unlocked)
{
	int ret;

	ret = pthread_spin_lock(&test_common.spin);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_spin_unlock(&test_common.spin);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


/* pthread_spin_trylock: success on unlocked spinlock */
TEST(pthread_spin, trylock_unlocked)
{
	int ret;

	ret = pthread_spin_trylock(&test_common.spin);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_spin_unlock(&test_common.spin);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


/* pthread_spin_trylock: EBUSY when already locked */
TEST(pthread_spin, trylock_locked_ebusy)
{
	int ret;

	ret = pthread_spin_lock(&test_common.spin);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_spin_trylock(&test_common.spin);
	TEST_ASSERT_EQUAL_INT(EBUSY, ret);

	ret = pthread_spin_unlock(&test_common.spin);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


/* pthread_spin_unlock: unlock returns 0 */
TEST(pthread_spin, unlock_success)
{
	int ret;

	ret = pthread_spin_lock(&test_common.spin);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_spin_unlock(&test_common.spin);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Verify unlocked: trylock should succeed */
	ret = pthread_spin_trylock(&test_common.spin);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_spin_unlock(&test_common.spin);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


/* pthread_spin_lock: lock/unlock cycle multiple times */
TEST(pthread_spin, lock_unlock_cycle)
{
	int ret;
	int i;

	for (i = 0; i < 10; i++) {
		ret = pthread_spin_lock(&test_common.spin);
		TEST_ASSERT_EQUAL_INT(0, ret);

		ret = pthread_spin_unlock(&test_common.spin);
		TEST_ASSERT_EQUAL_INT(0, ret);
	}
}


static void *spinThread(void *arg)
{
	pthread_spinlock_t *spin = (pthread_spinlock_t *)arg;
	int ret;

	ret = pthread_spin_lock(spin);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_spin_unlock(spin);
	TEST_ASSERT_EQUAL_INT(0, ret);

	return NULL;
}


/* pthread_spin_lock: thread contention - second thread waits */
TEST(pthread_spin, lock_thread_contention)
{
	pthread_t thr;
	int ret;

	ret = pthread_spin_lock(&test_common.spin);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_create(&thr, NULL, spinThread, (void *)&test_common.spin);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Let the thread attempt to lock (it will spin) */
	usleep(10000);

	/* Release so the thread can proceed */
	ret = pthread_spin_unlock(&test_common.spin);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_join(thr, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


/* pthread_spin_trylock: trylock from another thread while locked */
static void *spinTrylockThread(void *arg)
{
	pthread_spinlock_t *spin = (pthread_spinlock_t *)arg;
	int ret;

	ret = pthread_spin_trylock(spin);
	TEST_ASSERT_EQUAL_INT(EBUSY, ret);

	return NULL;
}


TEST(pthread_spin, trylock_thread_ebusy)
{
	pthread_t thr;
	int ret;

	ret = pthread_spin_lock(&test_common.spin);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_create(&thr, NULL, spinTrylockThread, (void *)&test_common.spin);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_join(thr, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_spin_unlock(&test_common.spin);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST_GROUP_RUNNER(pthread_spin)
{
	RUN_TEST_CASE(pthread_spin, init_private);
	RUN_TEST_CASE(pthread_spin, init_shared);
	RUN_TEST_CASE(pthread_spin, destroy_success);
	RUN_TEST_CASE(pthread_spin, lock_unlocked);
	RUN_TEST_CASE(pthread_spin, trylock_unlocked);
	RUN_TEST_CASE(pthread_spin, trylock_locked_ebusy);
	RUN_TEST_CASE(pthread_spin, unlock_success);
	RUN_TEST_CASE(pthread_spin, lock_unlock_cycle);
	RUN_TEST_CASE(pthread_spin, lock_thread_contention);
	RUN_TEST_CASE(pthread_spin, trylock_thread_ebusy);
}
