/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - sched.h
 * TESTED:
 *    - sched_yield()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <sched.h>

#include "unity_fixture.h"


TEST_GROUP(sched_yield);

TEST_SETUP(sched_yield)
{
}

TEST_TEAR_DOWN(sched_yield)
{
}


TEST(sched_yield, returns_zero_on_success)
{
	int ret;

	errno = 0;
	ret = sched_yield();
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, errno);
}


TEST(sched_yield, repeated_calls_succeed)
{
	int ret;
	int i;

	for (i = 0; i < 100; i++) {
		errno = 0;
		ret = sched_yield();
		TEST_ASSERT_EQUAL_INT(0, ret);
		TEST_ASSERT_EQUAL_INT(0, errno);
	}
}


TEST_GROUP_RUNNER(sched_yield)
{
	RUN_TEST_CASE(sched_yield, returns_zero_on_success);
	RUN_TEST_CASE(sched_yield, repeated_calls_succeed);
}
