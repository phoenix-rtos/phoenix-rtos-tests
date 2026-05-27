/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <pthread.h>
 * TESTED:
 *    - pthread_mutexattr_getprioceiling()
 *    - pthread_mutexattr_setprioceiling()
 *    - pthread_mutexattr_getprotocol()
 *    - pthread_mutexattr_setprotocol()
 *    - pthread_mutexattr_getpshared()
 *    - pthread_mutexattr_setpshared()
 *    - pthread_mutexattr_getrobust()
 *    - pthread_mutexattr_setrobust()
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
#include <sched.h>

#include "unity_fixture.h"


/* ===== pthread_mutexattr_prioceiling group ===== */


TEST_GROUP(pthread_mutexattr_prioceiling);


TEST_SETUP(pthread_mutexattr_prioceiling)
{
}


TEST_TEAR_DOWN(pthread_mutexattr_prioceiling)
{
}


/* pthread_mutexattr_getprioceiling: returns value after set */
TEST(pthread_mutexattr_prioceiling, get_after_set)
{
	pthread_mutexattr_t mattr;
	int prioceiling;
	int maxPrio;
	int ret;

	ret = pthread_mutexattr_init(&mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	maxPrio = sched_get_priority_max(SCHED_FIFO);
	TEST_ASSERT_TRUE(maxPrio >= 0);

	ret = pthread_mutexattr_setprioceiling(&mattr, maxPrio);
	if (ret == EINVAL || ret == EPERM) {
		pthread_mutexattr_destroy(&mattr);
		TEST_IGNORE_MESSAGE("pthread_mutexattr_setprioceiling not supported");
	}
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutexattr_getprioceiling(&mattr, &prioceiling);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(maxPrio, prioceiling);

	pthread_mutexattr_destroy(&mattr);
}


/* pthread_mutexattr_setprioceiling: set min priority */
TEST(pthread_mutexattr_prioceiling, set_min_priority)
{
	pthread_mutexattr_t mattr;
	int prioceiling;
	int minPrio;
	int ret;

	ret = pthread_mutexattr_init(&mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	minPrio = sched_get_priority_min(SCHED_FIFO);
	TEST_ASSERT_TRUE(minPrio >= 0);

	ret = pthread_mutexattr_setprioceiling(&mattr, minPrio);
	if (ret == EINVAL || ret == EPERM) {
		pthread_mutexattr_destroy(&mattr);
		TEST_IGNORE_MESSAGE("pthread_mutexattr_setprioceiling not supported");
	}
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutexattr_getprioceiling(&mattr, &prioceiling);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(minPrio, prioceiling);

	pthread_mutexattr_destroy(&mattr);
}


/* pthread_mutexattr_setprioceiling: roundtrip multiple values */
TEST(pthread_mutexattr_prioceiling, roundtrip_multiple)
{
	pthread_mutexattr_t mattr;
	int prioceiling;
	int minPrio;
	int maxPrio;
	int ret;

	ret = pthread_mutexattr_init(&mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	minPrio = sched_get_priority_min(SCHED_FIFO);
	maxPrio = sched_get_priority_max(SCHED_FIFO);

	ret = pthread_mutexattr_setprioceiling(&mattr, maxPrio);
	if (ret == EINVAL || ret == EPERM) {
		pthread_mutexattr_destroy(&mattr);
		TEST_IGNORE_MESSAGE("pthread_mutexattr_setprioceiling not supported");
	}
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutexattr_setprioceiling(&mattr, minPrio);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutexattr_getprioceiling(&mattr, &prioceiling);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(minPrio, prioceiling);

	pthread_mutexattr_destroy(&mattr);
}


TEST_GROUP_RUNNER(pthread_mutexattr_prioceiling)
{
	RUN_TEST_CASE(pthread_mutexattr_prioceiling, get_after_set);
	RUN_TEST_CASE(pthread_mutexattr_prioceiling, set_min_priority);
	RUN_TEST_CASE(pthread_mutexattr_prioceiling, roundtrip_multiple);
}


/* ===== pthread_mutexattr_protocol group ===== */


TEST_GROUP(pthread_mutexattr_protocol);


TEST_SETUP(pthread_mutexattr_protocol)
{
}


TEST_TEAR_DOWN(pthread_mutexattr_protocol)
{
}


/* pthread_mutexattr_getprotocol: default is PTHREAD_PRIO_NONE */
TEST(pthread_mutexattr_protocol, get_default_prio_none)
{
	pthread_mutexattr_t mattr;
	int protocol;
	int ret;

	ret = pthread_mutexattr_init(&mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutexattr_getprotocol(&mattr, &protocol);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(PTHREAD_PRIO_NONE, protocol);

	pthread_mutexattr_destroy(&mattr);
}


/* pthread_mutexattr_setprotocol: set PTHREAD_PRIO_NONE */
TEST(pthread_mutexattr_protocol, set_prio_none)
{
	pthread_mutexattr_t mattr;
	int protocol;
	int ret;

	ret = pthread_mutexattr_init(&mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutexattr_setprotocol(&mattr, PTHREAD_PRIO_NONE);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutexattr_getprotocol(&mattr, &protocol);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(PTHREAD_PRIO_NONE, protocol);

	pthread_mutexattr_destroy(&mattr);
}


/* pthread_mutexattr_setprotocol: set PTHREAD_PRIO_INHERIT */
TEST(pthread_mutexattr_protocol, set_prio_inherit)
{
	pthread_mutexattr_t mattr;
	int protocol;
	int ret;

	ret = pthread_mutexattr_init(&mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutexattr_setprotocol(&mattr, PTHREAD_PRIO_INHERIT);
	if (ret == ENOTSUP) {
		pthread_mutexattr_destroy(&mattr);
		TEST_IGNORE_MESSAGE("PTHREAD_PRIO_INHERIT not supported");
	}
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutexattr_getprotocol(&mattr, &protocol);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(PTHREAD_PRIO_INHERIT, protocol);

	pthread_mutexattr_destroy(&mattr);
}


/* pthread_mutexattr_setprotocol: set PTHREAD_PRIO_PROTECT */
TEST(pthread_mutexattr_protocol, set_prio_protect)
{
	pthread_mutexattr_t mattr;
	int protocol;
	int ret;

	ret = pthread_mutexattr_init(&mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutexattr_setprotocol(&mattr, PTHREAD_PRIO_PROTECT);
	if (ret == ENOTSUP) {
		pthread_mutexattr_destroy(&mattr);
		TEST_IGNORE_MESSAGE("PTHREAD_PRIO_PROTECT not supported");
	}
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutexattr_getprotocol(&mattr, &protocol);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(PTHREAD_PRIO_PROTECT, protocol);

	pthread_mutexattr_destroy(&mattr);
}


/* pthread_mutexattr_setprotocol: ENOTSUP for invalid value */
TEST(pthread_mutexattr_protocol, set_invalid_enotsup)
{
	pthread_mutexattr_t mattr;
	int ret;

	ret = pthread_mutexattr_init(&mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutexattr_setprotocol(&mattr, -1);
	TEST_ASSERT_TRUE(ret == ENOTSUP || ret == EINVAL);

	ret = pthread_mutexattr_setprotocol(&mattr, 999);
	TEST_ASSERT_TRUE(ret == ENOTSUP || ret == EINVAL);

	pthread_mutexattr_destroy(&mattr);
}


TEST_GROUP_RUNNER(pthread_mutexattr_protocol)
{
	RUN_TEST_CASE(pthread_mutexattr_protocol, get_default_prio_none);
	RUN_TEST_CASE(pthread_mutexattr_protocol, set_prio_none);
	RUN_TEST_CASE(pthread_mutexattr_protocol, set_prio_inherit);
	RUN_TEST_CASE(pthread_mutexattr_protocol, set_prio_protect);
	RUN_TEST_CASE(pthread_mutexattr_protocol, set_invalid_enotsup);
}


/* ===== pthread_mutexattr_pshared group ===== */


TEST_GROUP(pthread_mutexattr_pshared);


TEST_SETUP(pthread_mutexattr_pshared)
{
}


TEST_TEAR_DOWN(pthread_mutexattr_pshared)
{
}


/* pthread_mutexattr_getpshared: default is PTHREAD_PROCESS_PRIVATE */
TEST(pthread_mutexattr_pshared, get_default_private)
{
	pthread_mutexattr_t mattr;
	int pshared;
	int ret;

	ret = pthread_mutexattr_init(&mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutexattr_getpshared(&mattr, &pshared);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(PTHREAD_PROCESS_PRIVATE, pshared);

	pthread_mutexattr_destroy(&mattr);
}


/* pthread_mutexattr_setpshared: set PTHREAD_PROCESS_PRIVATE */
TEST(pthread_mutexattr_pshared, set_private)
{
	pthread_mutexattr_t mattr;
	int pshared;
	int ret;

	ret = pthread_mutexattr_init(&mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_PRIVATE);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutexattr_getpshared(&mattr, &pshared);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(PTHREAD_PROCESS_PRIVATE, pshared);

	pthread_mutexattr_destroy(&mattr);
}


/* pthread_mutexattr_setpshared: set PTHREAD_PROCESS_SHARED */
TEST(pthread_mutexattr_pshared, set_shared)
{
	pthread_mutexattr_t mattr;
	int pshared;
	int ret;

	ret = pthread_mutexattr_init(&mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutexattr_getpshared(&mattr, &pshared);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(PTHREAD_PROCESS_SHARED, pshared);

	pthread_mutexattr_destroy(&mattr);
}


/* pthread_mutexattr_setpshared: EINVAL for invalid value */
TEST(pthread_mutexattr_pshared, set_invalid_einval)
{
	pthread_mutexattr_t mattr;
	int ret;

	ret = pthread_mutexattr_init(&mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutexattr_setpshared(&mattr, -1);
	TEST_ASSERT_EQUAL_INT(EINVAL, ret);

	ret = pthread_mutexattr_setpshared(&mattr, 999);
	TEST_ASSERT_EQUAL_INT(EINVAL, ret);

	pthread_mutexattr_destroy(&mattr);
}


TEST_GROUP_RUNNER(pthread_mutexattr_pshared)
{
	RUN_TEST_CASE(pthread_mutexattr_pshared, get_default_private);
	RUN_TEST_CASE(pthread_mutexattr_pshared, set_private);
	RUN_TEST_CASE(pthread_mutexattr_pshared, set_shared);
	RUN_TEST_CASE(pthread_mutexattr_pshared, set_invalid_einval);
}


/* ===== pthread_mutexattr_robust group ===== */


TEST_GROUP(pthread_mutexattr_robust);


TEST_SETUP(pthread_mutexattr_robust)
{
}


TEST_TEAR_DOWN(pthread_mutexattr_robust)
{
}


/* pthread_mutexattr_getrobust: default is PTHREAD_MUTEX_STALLED */
TEST(pthread_mutexattr_robust, get_default_stalled)
{
	pthread_mutexattr_t mattr;
	int robust;
	int ret;

	ret = pthread_mutexattr_init(&mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutexattr_getrobust(&mattr, &robust);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(PTHREAD_MUTEX_STALLED, robust);

	pthread_mutexattr_destroy(&mattr);
}


/* pthread_mutexattr_setrobust: set PTHREAD_MUTEX_ROBUST */
TEST(pthread_mutexattr_robust, set_robust)
{
	pthread_mutexattr_t mattr;
	int robust;
	int ret;

	ret = pthread_mutexattr_init(&mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutexattr_setrobust(&mattr, PTHREAD_MUTEX_ROBUST);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutexattr_getrobust(&mattr, &robust);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(PTHREAD_MUTEX_ROBUST, robust);

	pthread_mutexattr_destroy(&mattr);
}


/* pthread_mutexattr_setrobust: set PTHREAD_MUTEX_STALLED */
TEST(pthread_mutexattr_robust, set_stalled)
{
	pthread_mutexattr_t mattr;
	int robust;
	int ret;

	ret = pthread_mutexattr_init(&mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutexattr_setrobust(&mattr, PTHREAD_MUTEX_ROBUST);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutexattr_setrobust(&mattr, PTHREAD_MUTEX_STALLED);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutexattr_getrobust(&mattr, &robust);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(PTHREAD_MUTEX_STALLED, robust);

	pthread_mutexattr_destroy(&mattr);
}


/* pthread_mutexattr_setrobust: EINVAL for invalid value */
TEST(pthread_mutexattr_robust, set_invalid_einval)
{
	pthread_mutexattr_t mattr;
	int ret;

	ret = pthread_mutexattr_init(&mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutexattr_setrobust(&mattr, -1);
	TEST_ASSERT_EQUAL_INT(EINVAL, ret);

	ret = pthread_mutexattr_setrobust(&mattr, 999);
	TEST_ASSERT_EQUAL_INT(EINVAL, ret);

	pthread_mutexattr_destroy(&mattr);
}


/* pthread_mutexattr_setrobust: robust attr usable with mutex_init */
TEST(pthread_mutexattr_robust, robust_usable_with_mutex_init)
{
	pthread_mutexattr_t mattr;
	pthread_mutex_t mtx;
	int ret;

	ret = pthread_mutexattr_init(&mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutexattr_setrobust(&mattr, PTHREAD_MUTEX_ROBUST);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutex_init(&mtx, &mattr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutex_lock(&mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_mutex_unlock(&mtx);
	TEST_ASSERT_EQUAL_INT(0, ret);

	pthread_mutex_destroy(&mtx);
	pthread_mutexattr_destroy(&mattr);
}


TEST_GROUP_RUNNER(pthread_mutexattr_robust)
{
	RUN_TEST_CASE(pthread_mutexattr_robust, get_default_stalled);
	RUN_TEST_CASE(pthread_mutexattr_robust, set_robust);
	RUN_TEST_CASE(pthread_mutexattr_robust, set_stalled);
	RUN_TEST_CASE(pthread_mutexattr_robust, set_invalid_einval);
	RUN_TEST_CASE(pthread_mutexattr_robust, robust_usable_with_mutex_init);
}
