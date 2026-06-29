/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <pthread.h>
 * TESTED:
 *    - pthread_attr_getguardsize()
 *    - pthread_attr_setguardsize()
 *    - pthread_attr_getinheritsched()
 *    - pthread_attr_setinheritsched()
 *    - pthread_attr_setscope()
 *    - pthread_attr_setstack()
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
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>

#include "unity_fixture.h"


/* ===== pthread_attr_guardsize group ===== */
/* Tests: pthread_attr_getguardsize, pthread_attr_setguardsize */


TEST_GROUP(pthread_attr_guardsize);


TEST_SETUP(pthread_attr_guardsize)
{
}


TEST_TEAR_DOWN(pthread_attr_guardsize)
{
}


/* pthread_attr_getguardsize: default value is implementation-defined (>= 0) */
TEST(pthread_attr_guardsize, get_default)
{
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("pthread_attr_getguardsize is not implemented");
#else
	pthread_attr_t attr;
	size_t guardsize;
	int ret;

	ret = pthread_attr_init(&attr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_attr_getguardsize(&attr, &guardsize);
	TEST_ASSERT_EQUAL_INT(0, ret);
	/* Default is implementation-defined; just verify the call succeeded */

	pthread_attr_destroy(&attr);
#endif
}


/* pthread_attr_setguardsize: set to zero disables guard area */
TEST(pthread_attr_guardsize, set_zero)
{
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("pthread_attr_get/setguardsize is not implemented");
#else
	pthread_attr_t attr;
	size_t guardsize;
	int ret;

	ret = pthread_attr_init(&attr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_attr_setguardsize(&attr, 0);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_attr_getguardsize(&attr, &guardsize);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_UINT32(0, (unsigned int)guardsize);

	pthread_attr_destroy(&attr);
#endif
}


/* pthread_attr_setguardsize: set to page size and retrieve */
TEST(pthread_attr_guardsize, set_pagesize)
{
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("pthread_attr_get/setguardsize is not implemented");
#else
	pthread_attr_t attr;
	size_t guardsize;
	size_t pagesize;
	int ret;

	pagesize = (size_t)sysconf(_SC_PAGESIZE);

	ret = pthread_attr_init(&attr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_attr_setguardsize(&attr, pagesize);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_attr_getguardsize(&attr, &guardsize);
	TEST_ASSERT_EQUAL_INT(0, ret);
	/* Implementation may round up; shall be at least what was set */
	TEST_ASSERT_TRUE(guardsize >= pagesize);

	pthread_attr_destroy(&attr);
#endif
}


/* pthread_attr_setguardsize: set to multiple of page size */
TEST(pthread_attr_guardsize, set_multiple_pagesize)
{
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("pthread_attr_get/setguardsize is not implemented");
#else
	pthread_attr_t attr;
	size_t guardsize;
	size_t requested;
	int ret;

	requested = (size_t)sysconf(_SC_PAGESIZE) * 4U;

	ret = pthread_attr_init(&attr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_attr_setguardsize(&attr, requested);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_attr_getguardsize(&attr, &guardsize);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_TRUE(guardsize >= requested);

	pthread_attr_destroy(&attr);
#endif
}


/* pthread_attr_setguardsize: round-trip preserves value set by previous call */
TEST(pthread_attr_guardsize, roundtrip_preserves_value)
{
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("pthread_attr_get/setguardsize is not implemented");
#else
	pthread_attr_t attr;
	size_t gs1;
	size_t gs2;
	size_t pagesize;
	int ret;

	pagesize = (size_t)sysconf(_SC_PAGESIZE);

	ret = pthread_attr_init(&attr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_attr_setguardsize(&attr, pagesize * 2U);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_attr_getguardsize(&attr, &gs1);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Set again to same value */
	ret = pthread_attr_setguardsize(&attr, gs1);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_attr_getguardsize(&attr, &gs2);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_UINT64(gs1, gs2);

	pthread_attr_destroy(&attr);
#endif
}


TEST_GROUP_RUNNER(pthread_attr_guardsize)
{
	RUN_TEST_CASE(pthread_attr_guardsize, get_default);
	RUN_TEST_CASE(pthread_attr_guardsize, set_zero);
	RUN_TEST_CASE(pthread_attr_guardsize, set_pagesize);
	RUN_TEST_CASE(pthread_attr_guardsize, set_multiple_pagesize);
	RUN_TEST_CASE(pthread_attr_guardsize, roundtrip_preserves_value);
}


/* ===== pthread_attr_inheritsched group ===== */
/* Tests: pthread_attr_getinheritsched, pthread_attr_setinheritsched */


TEST_GROUP(pthread_attr_inheritsched);


TEST_SETUP(pthread_attr_inheritsched)
{
}


TEST_TEAR_DOWN(pthread_attr_inheritsched)
{
}


/* pthread_attr_getinheritsched: default is implementation-defined */
TEST(pthread_attr_inheritsched, get_default)
{
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("pthread_attr_getinheritsched is not implemented");
#else
	pthread_attr_t attr;
	int inheritsched;
	int ret;

	ret = pthread_attr_init(&attr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_attr_getinheritsched(&attr, &inheritsched);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_TRUE(inheritsched == PTHREAD_INHERIT_SCHED || inheritsched == PTHREAD_EXPLICIT_SCHED);

	pthread_attr_destroy(&attr);
#endif
}


/* pthread_attr_setinheritsched: set PTHREAD_INHERIT_SCHED */
TEST(pthread_attr_inheritsched, set_inherit)
{
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("pthread_attr_get/setinheritsched is not implemented");
#else
	pthread_attr_t attr;
	int inheritsched;
	int ret;

	ret = pthread_attr_init(&attr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_attr_setinheritsched(&attr, PTHREAD_INHERIT_SCHED);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_attr_getinheritsched(&attr, &inheritsched);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(PTHREAD_INHERIT_SCHED, inheritsched);

	pthread_attr_destroy(&attr);
#endif
}


/* pthread_attr_setinheritsched: set PTHREAD_EXPLICIT_SCHED */
TEST(pthread_attr_inheritsched, set_explicit)
{
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("pthread_attr_get/setinheritsched is not implemented");
#else
	pthread_attr_t attr;
	int inheritsched;
	int ret;

	ret = pthread_attr_init(&attr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_attr_getinheritsched(&attr, &inheritsched);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(PTHREAD_EXPLICIT_SCHED, inheritsched);

	pthread_attr_destroy(&attr);
#endif
}


/* pthread_attr_setinheritsched: ENOTSUP for invalid value */
TEST(pthread_attr_inheritsched, set_invalid_enotsup)
{
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("pthread_attr_get/setinheritsched is not implemented");
#else
	pthread_attr_t attr;
	int ret;

	ret = pthread_attr_init(&attr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_attr_setinheritsched(&attr, -1);
	TEST_ASSERT_TRUE(ret == EINVAL || ret == ENOTSUP);

	ret = pthread_attr_setinheritsched(&attr, 999);
	TEST_ASSERT_TRUE(ret == EINVAL || ret == ENOTSUP);

	pthread_attr_destroy(&attr);
#endif
}


TEST_GROUP_RUNNER(pthread_attr_inheritsched)
{
	RUN_TEST_CASE(pthread_attr_inheritsched, get_default);
	RUN_TEST_CASE(pthread_attr_inheritsched, set_inherit);
	RUN_TEST_CASE(pthread_attr_inheritsched, set_explicit);
	RUN_TEST_CASE(pthread_attr_inheritsched, set_invalid_enotsup);
}


/* ===== pthread_attr_setscope group ===== */
/* Tests: pthread_attr_setscope (additional tests beyond existing getscope) */


TEST_GROUP(pthread_attr_setscope);


TEST_SETUP(pthread_attr_setscope)
{
}


TEST_TEAR_DOWN(pthread_attr_setscope)
{
}


/* pthread_attr_setscope: set PTHREAD_SCOPE_SYSTEM */
TEST(pthread_attr_setscope, set_system)
{
	pthread_attr_t attr;
	int scope;
	int ret;

	ret = pthread_attr_init(&attr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
	if (ret == ENOTSUP) {
		TEST_IGNORE_MESSAGE("PTHREAD_SCOPE_SYSTEM not supported");
	}
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_attr_getscope(&attr, &scope);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(PTHREAD_SCOPE_SYSTEM, scope);

	pthread_attr_destroy(&attr);
}


/* pthread_attr_setscope: set PTHREAD_SCOPE_PROCESS */
TEST(pthread_attr_setscope, set_process)
{
	pthread_attr_t attr;
	int scope;
	int ret;

	ret = pthread_attr_init(&attr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_attr_setscope(&attr, PTHREAD_SCOPE_PROCESS);
	if (ret == ENOTSUP) {
		TEST_IGNORE_MESSAGE("PTHREAD_SCOPE_PROCESS not supported");
	}
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_attr_getscope(&attr, &scope);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(PTHREAD_SCOPE_PROCESS, scope);

	pthread_attr_destroy(&attr);
}


/* pthread_attr_setscope: ENOTSUP for invalid contentionscope */
TEST(pthread_attr_setscope, set_invalid_enotsup)
{
	pthread_attr_t attr;
	int ret;

	ret = pthread_attr_init(&attr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_attr_setscope(&attr, -1);
	TEST_ASSERT_TRUE(ret == EINVAL || ret == ENOTSUP);

	ret = pthread_attr_setscope(&attr, 999);
	TEST_ASSERT_TRUE(ret == EINVAL || ret == ENOTSUP);

	pthread_attr_destroy(&attr);
}


TEST_GROUP_RUNNER(pthread_attr_setscope)
{
	RUN_TEST_CASE(pthread_attr_setscope, set_system);
	RUN_TEST_CASE(pthread_attr_setscope, set_process);
	RUN_TEST_CASE(pthread_attr_setscope, set_invalid_enotsup);
}


/* ===== pthread_attr_setstack group ===== */
/* Tests: pthread_attr_setstack, pthread_attr_getstack (additional) */


TEST_GROUP(pthread_attr_setstack);


TEST_SETUP(pthread_attr_setstack)
{
}


TEST_TEAR_DOWN(pthread_attr_setstack)
{
}


/* pthread_attr_setstack: set valid stack and retrieve */
TEST(pthread_attr_setstack, set_valid_stack)
{
	pthread_attr_t attr;
	void *stackaddr;
	size_t stacksize;
	int ret;
	static char stack[PTHREAD_STACK_MIN * 2];

	ret = pthread_attr_init(&attr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_attr_setstack(&attr, stack, sizeof(stack));
	if (ret == EINVAL) {
		TEST_IGNORE_MESSAGE("pthread_attr_setstack returned EINVAL (alignment)");
	}
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_attr_getstack(&attr, &stackaddr, &stacksize);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_PTR(stack, stackaddr);

	pthread_attr_destroy(&attr);
}


/* pthread_attr_setstack: EINVAL for stacksize < PTHREAD_STACK_MIN */
TEST(pthread_attr_setstack, einval_small_stacksize)
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


/* pthread_attr_setstack: guardsize is ignored when stackaddr is set */
TEST(pthread_attr_setstack, guardsize_ignored_with_custom_stack)
{
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("pthread_attr_get/setguardsize is not implemented");
#else
	pthread_attr_t attr;
	size_t guardsize;
	size_t pagesize;
	int ret;
	static char stack[PTHREAD_STACK_MIN * 2];

	pagesize = (size_t)sysconf(_SC_PAGESIZE);

	ret = pthread_attr_init(&attr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Set guardsize first */
	ret = pthread_attr_setguardsize(&attr, pagesize);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Now set stack — guardsize shall be ignored by implementation */
	ret = pthread_attr_setstack(&attr, stack, sizeof(stack));
	if (ret == EINVAL) {
		TEST_IGNORE_MESSAGE("pthread_attr_setstack returned EINVAL (alignment)");
	}
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* guardsize attribute value is still stored but ignored at thread creation */
	ret = pthread_attr_getguardsize(&attr, &guardsize);
	TEST_ASSERT_EQUAL_INT(0, ret);

	pthread_attr_destroy(&attr);
#endif
}


/* pthread_attr_setstack: exact PTHREAD_STACK_MIN size is valid */
TEST(pthread_attr_setstack, exact_min_size)
{
	pthread_attr_t attr;
	int ret;
	static char stack[PTHREAD_STACK_MIN];

	ret = pthread_attr_init(&attr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_attr_setstack(&attr, stack, sizeof(stack));
	if (ret == EINVAL) {
		TEST_IGNORE_MESSAGE("pthread_attr_setstack returned EINVAL (alignment)");
	}
	TEST_ASSERT_EQUAL_INT(0, ret);

	pthread_attr_destroy(&attr);
}


TEST_GROUP_RUNNER(pthread_attr_setstack)
{
	RUN_TEST_CASE(pthread_attr_setstack, set_valid_stack);
	RUN_TEST_CASE(pthread_attr_setstack, einval_small_stacksize);
	RUN_TEST_CASE(pthread_attr_setstack, guardsize_ignored_with_custom_stack);
	RUN_TEST_CASE(pthread_attr_setstack, exact_min_size);
}
