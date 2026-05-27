/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <pthread.h>
 * TESTED:
 *    - pthread_rwlockattr_destroy()
 *    - pthread_rwlockattr_init()
 *    - pthread_rwlockattr_getpshared()
 *    - pthread_rwlockattr_setpshared()
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

#include "unity_fixture.h"


TEST_GROUP(pthread_rwlockattr);


TEST_SETUP(pthread_rwlockattr)
{
}


TEST_TEAR_DOWN(pthread_rwlockattr)
{
}


/* pthread_rwlockattr_init: returns 0 */
TEST(pthread_rwlockattr, init_success)
{
	pthread_rwlockattr_t attr;
	int ret;

	ret = pthread_rwlockattr_init(&attr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	pthread_rwlockattr_destroy(&attr);
}


/* pthread_rwlockattr_destroy: returns 0 */
TEST(pthread_rwlockattr, destroy_success)
{
	pthread_rwlockattr_t attr;
	int ret;

	ret = pthread_rwlockattr_init(&attr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlockattr_destroy(&attr);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


/* pthread_rwlockattr_getpshared: default is PTHREAD_PROCESS_PRIVATE */
TEST(pthread_rwlockattr, getpshared_default_private)
{
	pthread_rwlockattr_t attr;
	int pshared;
	int ret;

	ret = pthread_rwlockattr_init(&attr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlockattr_getpshared(&attr, &pshared);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(PTHREAD_PROCESS_PRIVATE, pshared);

	pthread_rwlockattr_destroy(&attr);
}


/* pthread_rwlockattr_setpshared: set PTHREAD_PROCESS_PRIVATE */
TEST(pthread_rwlockattr, setpshared_private)
{
	pthread_rwlockattr_t attr;
	int pshared;
	int ret;

	ret = pthread_rwlockattr_init(&attr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlockattr_setpshared(&attr, PTHREAD_PROCESS_PRIVATE);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlockattr_getpshared(&attr, &pshared);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(PTHREAD_PROCESS_PRIVATE, pshared);

	pthread_rwlockattr_destroy(&attr);
}


/* pthread_rwlockattr_setpshared: set PTHREAD_PROCESS_SHARED */
TEST(pthread_rwlockattr, setpshared_shared)
{
	pthread_rwlockattr_t attr;
	int pshared;
	int ret;

	ret = pthread_rwlockattr_init(&attr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlockattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlockattr_getpshared(&attr, &pshared);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(PTHREAD_PROCESS_SHARED, pshared);

	pthread_rwlockattr_destroy(&attr);
}


/* pthread_rwlockattr_setpshared: EINVAL for invalid value */
TEST(pthread_rwlockattr, setpshared_invalid_einval)
{
	pthread_rwlockattr_t attr;
	int ret;

	ret = pthread_rwlockattr_init(&attr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlockattr_setpshared(&attr, -1);
	TEST_ASSERT_EQUAL_INT(EINVAL, ret);

	ret = pthread_rwlockattr_setpshared(&attr, 999);
	TEST_ASSERT_EQUAL_INT(EINVAL, ret);

	pthread_rwlockattr_destroy(&attr);
}


/* pthread_rwlockattr_setpshared: roundtrip private -> shared -> private */
TEST(pthread_rwlockattr, setpshared_roundtrip)
{
	pthread_rwlockattr_t attr;
	int pshared;
	int ret;

	ret = pthread_rwlockattr_init(&attr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlockattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlockattr_setpshared(&attr, PTHREAD_PROCESS_PRIVATE);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlockattr_getpshared(&attr, &pshared);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(PTHREAD_PROCESS_PRIVATE, pshared);

	pthread_rwlockattr_destroy(&attr);
}


/* pthread_rwlock_init: use attr with pshared to init rwlock */
TEST(pthread_rwlockattr, usable_with_rwlock_init)
{
	pthread_rwlockattr_t attr;
	pthread_rwlock_t rwl;
	int ret;

	ret = pthread_rwlockattr_init(&attr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlockattr_setpshared(&attr, PTHREAD_PROCESS_PRIVATE);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlock_init(&rwl, &attr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlock_rdlock(&rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_rwlock_unlock(&rwl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	pthread_rwlock_destroy(&rwl);
	pthread_rwlockattr_destroy(&attr);
}


TEST_GROUP_RUNNER(pthread_rwlockattr)
{
	RUN_TEST_CASE(pthread_rwlockattr, init_success);
	RUN_TEST_CASE(pthread_rwlockattr, destroy_success);
	RUN_TEST_CASE(pthread_rwlockattr, getpshared_default_private);
	RUN_TEST_CASE(pthread_rwlockattr, setpshared_private);
	RUN_TEST_CASE(pthread_rwlockattr, setpshared_shared);
	RUN_TEST_CASE(pthread_rwlockattr, setpshared_invalid_einval);
	RUN_TEST_CASE(pthread_rwlockattr, setpshared_roundtrip);
	RUN_TEST_CASE(pthread_rwlockattr, usable_with_rwlock_init);
}
