/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - pthread.h
 * TESTED:
 *    - pthread_mutex_init()
 *    - pthread_mutex_destroy()
 *    - pthread_mutex_lock()
 *    - pthread_mutex_trylock()
 *    - pthread_mutex_unlock()
 *    - pthread_mutexattr_init()
 *    - pthread_mutexattr_destroy()
 *    - pthread_mutexattr_gettype()
 *    - pthread_mutexattr_settype()
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

#include "unity_fixture.h"


/* ===== pthread_mutex group ===== */


TEST_GROUP(pthread_mutex);


TEST_SETUP(pthread_mutex)
{
}


TEST_TEAR_DOWN(pthread_mutex)
{
}


/* pthread_mutex_init: init with NULL attr (default), return 0 */
TEST(pthread_mutex, mutex_init_default)
{
	pthread_mutex_t mtx;
	int ret;

	ret = pthread_mutex_init(&mtx, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	pthread_mutex_destroy(&mtx);
}


/* pthread_mutex_init: init with explicit attributes */
TEST(pthread_mutex, mutex_init_with_attr)
{
	pthread_mutex_t mtx;
	pthread_mutexattr_t mattr;
	int ret;

	ret = pthread_mutexattr_init(&mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_ERRORCHECK);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutex_init(&mtx, &mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	pthread_mutex_destroy(&mtx);
	pthread_mutexattr_destroy(&mattr);
}


/* PTHREAD_MUTEX_INITIALIZER: static init equivalent to pthread_mutex_init with NULL */
TEST(pthread_mutex, mutex_initializer_static)
{
	pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
	int ret;

	ret = pthread_mutex_lock(&mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutex_unlock(&mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);

	pthread_mutex_destroy(&mtx);
}


/* pthread_mutex_destroy: destroy unlocked mutex, return 0 */
TEST(pthread_mutex, mutex_destroy_unlocked)
{
	pthread_mutex_t mtx;
	int ret;

	ret = pthread_mutex_init(&mtx, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutex_destroy(&mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


/* pthread_mutex_destroy: destroyed object can be reinitialized */
TEST(pthread_mutex, mutex_destroy_reinit)
{
	pthread_mutex_t mtx;
	int ret;

	ret = pthread_mutex_init(&mtx, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutex_destroy(&mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutex_init(&mtx, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	pthread_mutex_destroy(&mtx);
}


/* pthread_mutex_lock/unlock: basic lock and unlock, return 0 */
TEST(pthread_mutex, mutex_lock_unlock_basic)
{
	pthread_mutex_t mtx;
	int ret;

	ret = pthread_mutex_init(&mtx, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutex_lock(&mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutex_unlock(&mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);

	pthread_mutex_destroy(&mtx);
}


/* pthread_mutex_trylock: acquire unlocked mutex, return 0 */
TEST(pthread_mutex, mutex_trylock_unlocked)
{
	pthread_mutex_t mtx;
	int ret;

	ret = pthread_mutex_init(&mtx, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutex_trylock(&mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutex_unlock(&mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);

	pthread_mutex_destroy(&mtx);
}


/* pthread_mutex_trylock: shall fail with EBUSY when mutex is locked */
static void *test_mutexHolder(void *arg)
{
	pthread_mutex_t *mtx = (pthread_mutex_t *)arg;
	pthread_mutex_lock(mtx);
	/* Hold for a bit so main thread can try */
	usleep(50000);
	pthread_mutex_unlock(mtx);
	return NULL;
}


TEST(pthread_mutex, mutex_trylock_ebusy)
{
	pthread_mutex_t mtx;
	pthread_t thread;
	int ret;

	ret = pthread_mutex_init(&mtx, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutex_lock(&mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* trylock on already-locked mutex from same thread (DEFAULT type) */
	ret = pthread_mutex_trylock(&mtx);
	TEST_ASSERT_EQUAL_INT(EBUSY, ret);

	ret = pthread_mutex_unlock(&mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Also verify with another thread */
	ret = pthread_create(&thread, NULL, test_mutexHolder, &mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Give thread time to acquire */
	usleep(10000);

	ret = pthread_mutex_trylock(&mtx);
	TEST_ASSERT_EQUAL_INT(EBUSY, ret);

	ret = pthread_join(thread, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	pthread_mutex_destroy(&mtx);
}


/* pthread_mutex_lock: ERRORCHECK type shall return EDEADLK on relock */
TEST(pthread_mutex, mutex_lock_errorcheck_edeadlk)
{
	pthread_mutex_t mtx;
	pthread_mutexattr_t mattr;
	int ret;

	ret = pthread_mutexattr_init(&mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_ERRORCHECK);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutex_init(&mtx, &mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutex_lock(&mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutex_lock(&mtx);
	TEST_ASSERT_EQUAL_INT(EDEADLK, ret);

	ret = pthread_mutex_unlock(&mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);

	pthread_mutex_destroy(&mtx);
	pthread_mutexattr_destroy(&mattr);
}


/* pthread_mutex_unlock: ERRORCHECK type shall return EPERM if not owner */
TEST(pthread_mutex, mutex_unlock_errorcheck_eperm)
{
	pthread_mutex_t mtx;
	pthread_mutexattr_t mattr;
	int ret;

	ret = pthread_mutexattr_init(&mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_ERRORCHECK);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutex_init(&mtx, &mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Unlock without owning */
	ret = pthread_mutex_unlock(&mtx);
	TEST_ASSERT_EQUAL_INT(EPERM, ret);

	pthread_mutex_destroy(&mtx);
	pthread_mutexattr_destroy(&mattr);
}


/* pthread_mutex_lock: RECURSIVE type increments lock count */
TEST(pthread_mutex, mutex_lock_recursive)
{
	pthread_mutex_t mtx;
	pthread_mutexattr_t mattr;
	int ret;

	ret = pthread_mutexattr_init(&mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutex_init(&mtx, &mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutex_lock(&mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutex_lock(&mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutex_lock(&mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutex_unlock(&mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutex_unlock(&mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutex_unlock(&mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);

	pthread_mutex_destroy(&mtx);
	pthread_mutexattr_destroy(&mattr);
}


/* pthread_mutex_trylock: RECURSIVE type increments lock count */
TEST(pthread_mutex, mutex_trylock_recursive)
{
	pthread_mutex_t mtx;
	pthread_mutexattr_t mattr;
	int ret;

	ret = pthread_mutexattr_init(&mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutex_init(&mtx, &mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutex_trylock(&mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);

#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1643 issue");
#else
	ret = pthread_mutex_trylock(&mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutex_unlock(&mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutex_unlock(&mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);
#endif

	pthread_mutex_destroy(&mtx);
	pthread_mutexattr_destroy(&mattr);
}


/* pthread_mutex_unlock: RECURSIVE type shall return EPERM if not owner */
TEST(pthread_mutex, mutex_unlock_recursive_eperm)
{
	pthread_mutex_t mtx;
	pthread_mutexattr_t mattr;
	int ret;

	ret = pthread_mutexattr_init(&mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutex_init(&mtx, &mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Unlock without owning */
	ret = pthread_mutex_unlock(&mtx);
	TEST_ASSERT_EQUAL_INT(EPERM, ret);

	pthread_mutex_destroy(&mtx);
	pthread_mutexattr_destroy(&mattr);
}


/* pthread_mutex_lock: mutex provides mutual exclusion between threads */
static struct {
	pthread_mutex_t mtx;
	int counter;
} test_mutex_shared;

#define MUTEX_INCR_COUNT 10000

static void *test_mutexIncrThread(void *arg)
{
	int i;
	(void)arg;

	for (i = 0; i < MUTEX_INCR_COUNT; i++) {
		pthread_mutex_lock(&test_mutex_shared.mtx);
		test_mutex_shared.counter++;
		pthread_mutex_unlock(&test_mutex_shared.mtx);
	}

	return NULL;
}


TEST(pthread_mutex, mutex_lock_mutual_exclusion)
{
	pthread_t t1, t2;
	int ret;

	test_mutex_shared.counter = 0;
	ret = pthread_mutex_init(&test_mutex_shared.mtx, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_create(&t1, NULL, test_mutexIncrThread, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_create(&t2, NULL, test_mutexIncrThread, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_join(t1, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_join(t2, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	TEST_ASSERT_EQUAL_INT(2 * MUTEX_INCR_COUNT, test_mutex_shared.counter);

	pthread_mutex_destroy(&test_mutex_shared.mtx);
}


TEST_GROUP_RUNNER(pthread_mutex)
{
	RUN_TEST_CASE(pthread_mutex, mutex_init_default);
	RUN_TEST_CASE(pthread_mutex, mutex_init_with_attr);
	RUN_TEST_CASE(pthread_mutex, mutex_initializer_static);
	RUN_TEST_CASE(pthread_mutex, mutex_destroy_unlocked);
	RUN_TEST_CASE(pthread_mutex, mutex_destroy_reinit);
	RUN_TEST_CASE(pthread_mutex, mutex_lock_unlock_basic);
	RUN_TEST_CASE(pthread_mutex, mutex_trylock_unlocked);
	RUN_TEST_CASE(pthread_mutex, mutex_trylock_ebusy);
	RUN_TEST_CASE(pthread_mutex, mutex_lock_errorcheck_edeadlk);
	RUN_TEST_CASE(pthread_mutex, mutex_unlock_errorcheck_eperm);
	RUN_TEST_CASE(pthread_mutex, mutex_lock_recursive);
	RUN_TEST_CASE(pthread_mutex, mutex_trylock_recursive);
	RUN_TEST_CASE(pthread_mutex, mutex_unlock_recursive_eperm);
	RUN_TEST_CASE(pthread_mutex, mutex_lock_mutual_exclusion);
}


/* ===== pthread_mutexattr group ===== */


TEST_GROUP(pthread_mutexattr);


TEST_SETUP(pthread_mutexattr)
{
}


TEST_TEAR_DOWN(pthread_mutexattr)
{
}


/* pthread_mutexattr_init: shall return 0 */
TEST(pthread_mutexattr, mutexattr_init_success)
{
	pthread_mutexattr_t mattr;
	int ret;

	ret = pthread_mutexattr_init(&mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	pthread_mutexattr_destroy(&mattr);
}


/* pthread_mutexattr_destroy: shall return 0 */
TEST(pthread_mutexattr, mutexattr_destroy_success)
{
	pthread_mutexattr_t mattr;
	int ret;

	ret = pthread_mutexattr_init(&mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutexattr_destroy(&mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


/* pthread_mutexattr_destroy: destroyed can be reinitialized */
TEST(pthread_mutexattr, mutexattr_destroy_reinit)
{
	pthread_mutexattr_t mattr;
	int ret;

	ret = pthread_mutexattr_init(&mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutexattr_destroy(&mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutexattr_init(&mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	pthread_mutexattr_destroy(&mattr);
}


/* pthread_mutexattr_gettype: default is PTHREAD_MUTEX_DEFAULT */
TEST(pthread_mutexattr, mutexattr_gettype_default)
{
	pthread_mutexattr_t mattr;
	int type;
	int ret;

	ret = pthread_mutexattr_init(&mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutexattr_gettype(&mattr, &type);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(PTHREAD_MUTEX_DEFAULT, type);

	pthread_mutexattr_destroy(&mattr);
}


/* pthread_mutexattr_settype: set PTHREAD_MUTEX_NORMAL */
TEST(pthread_mutexattr, mutexattr_settype_normal)
{
	pthread_mutexattr_t mattr;
	int type;
	int ret;

	ret = pthread_mutexattr_init(&mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_NORMAL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutexattr_gettype(&mattr, &type);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(PTHREAD_MUTEX_NORMAL, type);

	pthread_mutexattr_destroy(&mattr);
}


/* pthread_mutexattr_settype: set PTHREAD_MUTEX_ERRORCHECK */
TEST(pthread_mutexattr, mutexattr_settype_errorcheck)
{
	pthread_mutexattr_t mattr;
	int type;
	int ret;

	ret = pthread_mutexattr_init(&mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_ERRORCHECK);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutexattr_gettype(&mattr, &type);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(PTHREAD_MUTEX_ERRORCHECK, type);

	pthread_mutexattr_destroy(&mattr);
}


/* pthread_mutexattr_settype: set PTHREAD_MUTEX_RECURSIVE */
TEST(pthread_mutexattr, mutexattr_settype_recursive)
{
	pthread_mutexattr_t mattr;
	int type;
	int ret;

	ret = pthread_mutexattr_init(&mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutexattr_gettype(&mattr, &type);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(PTHREAD_MUTEX_RECURSIVE, type);

	pthread_mutexattr_destroy(&mattr);
}


/* pthread_mutexattr_settype: shall fail with EINVAL for invalid type */
TEST(pthread_mutexattr, mutexattr_settype_einval)
{
	pthread_mutexattr_t mattr;
	int ret;

	ret = pthread_mutexattr_init(&mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutexattr_settype(&mattr, -1);
	TEST_ASSERT_EQUAL_INT(EINVAL, ret);

	ret = pthread_mutexattr_settype(&mattr, 999);
	TEST_ASSERT_EQUAL_INT(EINVAL, ret);

	pthread_mutexattr_destroy(&mattr);
}


/* pthread_mutexattr: attr object not affected after used to init mutex */
TEST(pthread_mutexattr, mutexattr_not_affected_after_init)
{
	pthread_mutexattr_t mattr;
	pthread_mutex_t mtx;
	int type;
	int ret;

	ret = pthread_mutexattr_init(&mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutex_init(&mtx, &mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Modify attr after mutex creation — mutex is unaffected */
	ret = pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_NORMAL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutexattr_gettype(&mattr, &type);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(PTHREAD_MUTEX_NORMAL, type);

	/* Verify the mutex still behaves as RECURSIVE */
	ret = pthread_mutex_lock(&mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutex_lock(&mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutex_unlock(&mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutex_unlock(&mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);

	pthread_mutex_destroy(&mtx);
	pthread_mutexattr_destroy(&mattr);
}


TEST_GROUP_RUNNER(pthread_mutexattr)
{
	RUN_TEST_CASE(pthread_mutexattr, mutexattr_init_success);
	RUN_TEST_CASE(pthread_mutexattr, mutexattr_destroy_success);
	RUN_TEST_CASE(pthread_mutexattr, mutexattr_destroy_reinit);
	RUN_TEST_CASE(pthread_mutexattr, mutexattr_gettype_default);
	RUN_TEST_CASE(pthread_mutexattr, mutexattr_settype_normal);
	RUN_TEST_CASE(pthread_mutexattr, mutexattr_settype_errorcheck);
	RUN_TEST_CASE(pthread_mutexattr, mutexattr_settype_recursive);
	RUN_TEST_CASE(pthread_mutexattr, mutexattr_settype_einval);
	RUN_TEST_CASE(pthread_mutexattr, mutexattr_not_affected_after_init);
}
