/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - pthread.h
 * TESTED:
 *    - pthread_attr_init()
 *    - pthread_attr_destroy()
 *    - pthread_attr_getdetachstate()
 *    - pthread_attr_setdetachstate()
 *    - pthread_attr_getschedparam()
 *    - pthread_attr_setschedparam()
 *    - pthread_attr_getschedpolicy()
 *    - pthread_attr_setschedpolicy()
 *    - pthread_attr_getscope()
 *    - pthread_attr_getstack()
 *    - pthread_attr_getstacksize()
 *    - pthread_attr_setstacksize()
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
#include <limits.h>
#include <sched.h>
#include <stdlib.h>
#include <string.h>

#include "unity_fixture.h"


TEST_GROUP(pthread_attr);


TEST_SETUP(pthread_attr)
{
}


TEST_TEAR_DOWN(pthread_attr)
{
}


/* pthread_attr_init: shall initialize with default values, return 0 */
TEST(pthread_attr, attr_init_success)
{
	pthread_attr_t attr;
	int ret;

	ret = pthread_attr_init(&attr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	pthread_attr_destroy(&attr);
}


/* pthread_attr_destroy: shall destroy an initialized attr object, return 0 */
TEST(pthread_attr, attr_destroy_success)
{
	pthread_attr_t attr;
	int ret;

	ret = pthread_attr_init(&attr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_attr_destroy(&attr);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


/* pthread_attr_destroy: destroyed object can be reinitialized */
TEST(pthread_attr, attr_destroy_reinit)
{
	pthread_attr_t attr;
	int ret;

	ret = pthread_attr_init(&attr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_attr_destroy(&attr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_attr_init(&attr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	pthread_attr_destroy(&attr);
}


/* pthread_attr_getdetachstate: default shall be PTHREAD_CREATE_JOINABLE */
TEST(pthread_attr, attr_detachstate_default_joinable)
{
	pthread_attr_t attr;
	int state;
	int ret;

	ret = pthread_attr_init(&attr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_attr_getdetachstate(&attr, &state);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(PTHREAD_CREATE_JOINABLE, state);

	pthread_attr_destroy(&attr);
}


/* pthread_attr_setdetachstate: set to PTHREAD_CREATE_DETACHED */
TEST(pthread_attr, attr_setdetachstate_detached)
{
	pthread_attr_t attr;
	int state;
	int ret;

	ret = pthread_attr_init(&attr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_attr_getdetachstate(&attr, &state);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(PTHREAD_CREATE_DETACHED, state);

	pthread_attr_destroy(&attr);
}


/* pthread_attr_setdetachstate: set to PTHREAD_CREATE_JOINABLE */
TEST(pthread_attr, attr_setdetachstate_joinable)
{
	pthread_attr_t attr;
	int state;
	int ret;

	ret = pthread_attr_init(&attr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_attr_getdetachstate(&attr, &state);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(PTHREAD_CREATE_JOINABLE, state);

	pthread_attr_destroy(&attr);
}


/* pthread_attr_setdetachstate: shall fail with EINVAL for invalid value */
TEST(pthread_attr, attr_setdetachstate_einval)
{
	pthread_attr_t attr;
	int ret;

	ret = pthread_attr_init(&attr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_attr_setdetachstate(&attr, -1);
	TEST_ASSERT_EQUAL_INT(EINVAL, ret);

	ret = pthread_attr_setdetachstate(&attr, 99);
	TEST_ASSERT_EQUAL_INT(EINVAL, ret);

	pthread_attr_destroy(&attr);
}


/* pthread_attr_getschedparam/setschedparam: round-trip with valid priority */
TEST(pthread_attr, attr_schedparam_roundtrip)
{
	pthread_attr_t attr;
	struct sched_param param;
	struct sched_param got;
	int ret;

	ret = pthread_attr_init(&attr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	param.sched_priority = 0;
	ret = pthread_attr_setschedparam(&attr, &param);
	TEST_ASSERT_EQUAL_INT(0, ret);

	memset(&got, 0xff, sizeof(got));
	ret = pthread_attr_getschedparam(&attr, &got);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, got.sched_priority);

	pthread_attr_destroy(&attr);
}


/* pthread_attr_getschedpolicy/setschedpolicy: set SCHED_FIFO */
TEST(pthread_attr, attr_schedpolicy_fifo)
{
	pthread_attr_t attr;
	int policy;
	int ret;

	ret = pthread_attr_init(&attr);
	TEST_ASSERT_EQUAL_INT(0, ret);

#ifdef __phoenix__
	(void)policy;
	TEST_IGNORE_MESSAGE("#1639 issue - phoenix limitation");
#else
	ret = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_attr_getschedpolicy(&attr, &policy);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(SCHED_FIFO, policy);
#endif

	pthread_attr_destroy(&attr);
}


/* pthread_attr_getschedpolicy/setschedpolicy: set SCHED_RR */
TEST(pthread_attr, attr_schedpolicy_rr)
{
	pthread_attr_t attr;
	int policy;
	int ret;

	ret = pthread_attr_init(&attr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_attr_setschedpolicy(&attr, SCHED_RR);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_attr_getschedpolicy(&attr, &policy);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(SCHED_RR, policy);

	pthread_attr_destroy(&attr);
}


/* pthread_attr_getschedpolicy/setschedpolicy: set SCHED_OTHER */
TEST(pthread_attr, attr_schedpolicy_other)
{
	pthread_attr_t attr;
	int policy;
	int ret;

	ret = pthread_attr_init(&attr);
	TEST_ASSERT_EQUAL_INT(0, ret);

#ifdef __phoenix__
	(void)policy;
	TEST_IGNORE_MESSAGE("#1639 issue - phoenix limitation");
#else
	ret = pthread_attr_setschedpolicy(&attr, SCHED_OTHER);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_attr_getschedpolicy(&attr, &policy);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(SCHED_OTHER, policy);
#endif

	pthread_attr_destroy(&attr);
}


/* pthread_attr_setschedpolicy: shall fail with ENOTSUP for invalid policy */
TEST(pthread_attr, attr_setschedpolicy_enotsup)
{
	pthread_attr_t attr;
	int ret;

	ret = pthread_attr_init(&attr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_attr_setschedpolicy(&attr, -1);
	TEST_ASSERT_TRUE(ret == EINVAL || ret == ENOTSUP);

	pthread_attr_destroy(&attr);
}


/* pthread_attr_getscope: get contentionscope (at least one of SYSTEM/PROCESS must work) */
TEST(pthread_attr, attr_getscope_default)
{
	pthread_attr_t attr;
	int scope;
	int ret;

	ret = pthread_attr_init(&attr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_attr_getscope(&attr, &scope);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_TRUE(scope == PTHREAD_SCOPE_SYSTEM || scope == PTHREAD_SCOPE_PROCESS);

	pthread_attr_destroy(&attr);
}


/* pthread_attr_getstacksize: shall return a value >= PTHREAD_STACK_MIN */
TEST(pthread_attr, attr_getstacksize_default)
{
	pthread_attr_t attr;
	size_t stacksize;
	int ret;

	ret = pthread_attr_init(&attr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_attr_getstacksize(&attr, &stacksize);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_TRUE(stacksize >= PTHREAD_STACK_MIN);

	pthread_attr_destroy(&attr);
}


/* pthread_attr_setstacksize: set to PTHREAD_STACK_MIN */
TEST(pthread_attr, attr_setstacksize_min)
{
	pthread_attr_t attr;
	size_t stacksize;
	int ret;

	ret = pthread_attr_init(&attr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_attr_getstacksize(&attr, &stacksize);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_TRUE(stacksize >= (size_t)PTHREAD_STACK_MIN);

	pthread_attr_destroy(&attr);
}


/* pthread_attr_setstacksize: set to a large value */
TEST(pthread_attr, attr_setstacksize_large)
{
	pthread_attr_t attr;
	size_t stacksize;
	size_t requested = 1024U * 1024U;
	int ret;

	ret = pthread_attr_init(&attr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_attr_setstacksize(&attr, requested);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_attr_getstacksize(&attr, &stacksize);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_TRUE(stacksize >= requested);

	pthread_attr_destroy(&attr);
}


/* pthread_attr_setstacksize: shall fail with EINVAL for size < PTHREAD_STACK_MIN */
TEST(pthread_attr, attr_setstacksize_einval_too_small)
{
	pthread_attr_t attr;
	int ret;

	ret = pthread_attr_init(&attr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_attr_setstacksize(&attr, 1);
	TEST_ASSERT_EQUAL_INT(EINVAL, ret);

	pthread_attr_destroy(&attr);
}


/* pthread_attr_getstack: after setstack, retrieve same values */
TEST(pthread_attr, attr_getstack_after_setstack)
{
	pthread_attr_t attr;
	void *stackaddr;
	size_t stacksize;
	int ret;
	static char stack[PTHREAD_STACK_MIN];

	ret = pthread_attr_init(&attr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_attr_setstack(&attr, stack, sizeof(stack));
	if (ret == EINVAL) {
		/* alignment issue — skip this test */
		TEST_IGNORE_MESSAGE("pthread_attr_setstack returned EINVAL (alignment)");
	}
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_attr_getstack(&attr, &stackaddr, &stacksize);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_PTR(stack, stackaddr);
	TEST_ASSERT_EQUAL_UINT64(sizeof(stack), stacksize);

	pthread_attr_destroy(&attr);
}


/* pthread_attr_setstack: shall fail with EINVAL for size < PTHREAD_STACK_MIN */
TEST(pthread_attr, attr_setstack_einval_small_size)
{
	pthread_attr_t attr;
	static char stack[16];
	int ret;

	ret = pthread_attr_init(&attr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_attr_setstack(&attr, stack, sizeof(stack));
	TEST_ASSERT_EQUAL_INT(EINVAL, ret);

	pthread_attr_destroy(&attr);
}


/* pthread_attr_init: attributes object usable with pthread_create */
static void *test_attrThread(void *arg)
{
	(void)arg;
	return (void *)42;
}


TEST(pthread_attr, attr_init_usable_with_create)
{
	pthread_attr_t attr;
	pthread_t thread;
	void *retval;
	int ret;

	ret = pthread_attr_init(&attr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_create(&thread, &attr, test_attrThread, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_join(thread, &retval);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_PTR((void *)42, retval);

	pthread_attr_destroy(&attr);
}


/* pthread_attr: single object usable in multiple create calls */
TEST(pthread_attr, attr_reuse_across_creates)
{
	pthread_attr_t attr;
	pthread_t t1, t2;
	void *r1, *r2;
	int ret;

	ret = pthread_attr_init(&attr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_create(&t1, &attr, test_attrThread, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_create(&t2, &attr, test_attrThread, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_join(t1, &r1);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_PTR((void *)42, r1);

	ret = pthread_join(t2, &r2);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_PTR((void *)42, r2);

	pthread_attr_destroy(&attr);
}


TEST_GROUP_RUNNER(pthread_attr)
{
	RUN_TEST_CASE(pthread_attr, attr_init_success);
	RUN_TEST_CASE(pthread_attr, attr_destroy_success);
	RUN_TEST_CASE(pthread_attr, attr_destroy_reinit);
	RUN_TEST_CASE(pthread_attr, attr_detachstate_default_joinable);
	RUN_TEST_CASE(pthread_attr, attr_setdetachstate_detached);
	RUN_TEST_CASE(pthread_attr, attr_setdetachstate_joinable);
	RUN_TEST_CASE(pthread_attr, attr_setdetachstate_einval);
	RUN_TEST_CASE(pthread_attr, attr_schedparam_roundtrip);
	RUN_TEST_CASE(pthread_attr, attr_schedpolicy_fifo);
	RUN_TEST_CASE(pthread_attr, attr_schedpolicy_rr);
	RUN_TEST_CASE(pthread_attr, attr_schedpolicy_other);
	RUN_TEST_CASE(pthread_attr, attr_setschedpolicy_enotsup);
	RUN_TEST_CASE(pthread_attr, attr_getscope_default);
	RUN_TEST_CASE(pthread_attr, attr_getstacksize_default);
	RUN_TEST_CASE(pthread_attr, attr_setstacksize_min);
	RUN_TEST_CASE(pthread_attr, attr_setstacksize_large);
	RUN_TEST_CASE(pthread_attr, attr_setstacksize_einval_too_small);
	RUN_TEST_CASE(pthread_attr, attr_getstack_after_setstack);
	RUN_TEST_CASE(pthread_attr, attr_setstack_einval_small_size);
	RUN_TEST_CASE(pthread_attr, attr_init_usable_with_create);
	RUN_TEST_CASE(pthread_attr, attr_reuse_across_creates);
}
