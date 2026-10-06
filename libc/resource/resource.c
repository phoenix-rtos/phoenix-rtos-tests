/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <sys/resource.h>
 * TESTED:
 *    - getpriority()
 *    - getrlimit()
 *    - getrusage()
 *    - setpriority()
 *    - setrlimit()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>

#include "unity_fixture.h"


TEST_GROUP(resource_priority);

static struct {
	int origPriority;
} test_common;


TEST_SETUP(resource_priority)
{
	errno = 0;
	test_common.origPriority = getpriority(PRIO_PROCESS, 0);
	/* getpriority can legitimately return -1, check errno */
	if (test_common.origPriority == -1 && errno != 0) {
		TEST_FAIL_MESSAGE("getpriority failed in setup");
	}
}

TEST_TEAR_DOWN(resource_priority)
{
	/* Restore original priority (best effort) */
	setpriority(PRIO_PROCESS, 0, test_common.origPriority);
}


TEST(resource_priority, getpriority_self_process)
{
	int prio;

	errno = 0;
	prio = getpriority(PRIO_PROCESS, 0);
	/* If return is -1, must check errno to distinguish error from valid value */
	if (prio == -1) {
		TEST_ASSERT_EQUAL_INT(0, errno);
	}
}


TEST(resource_priority, getpriority_self_pgrp)
{
	int prio;

	errno = 0;
	prio = getpriority(PRIO_PGRP, 0);
	if (prio == -1) {
		TEST_ASSERT_EQUAL_INT(0, errno);
	}
}


TEST(resource_priority, getpriority_self_user)
{
	int prio;

	errno = 0;
	prio = getpriority(PRIO_USER, 0);
	if (prio == -1) {
		TEST_ASSERT_EQUAL_INT(0, errno);
	}
}


TEST(resource_priority, setpriority_self_process)
{
	int ret;
	int prio;

	/* Raise nice value (lower priority) - always allowed */
	errno = 0;
	ret = setpriority(PRIO_PROCESS, 0, test_common.origPriority + 1);
	if (ret == -1 && errno == EACCES) {
		TEST_IGNORE_MESSAGE("insufficient privileges to change priority");
	}
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Verify the change */
	errno = 0;
	prio = getpriority(PRIO_PROCESS, 0);
	if (prio == -1 && errno != 0) {
		TEST_FAIL_MESSAGE("getpriority failed after setpriority");
	}
	TEST_ASSERT_EQUAL_INT(test_common.origPriority + 1, prio);
}


TEST_GROUP_RUNNER(resource_priority)
{
	RUN_TEST_CASE(resource_priority, getpriority_self_process);
	RUN_TEST_CASE(resource_priority, getpriority_self_pgrp);
	RUN_TEST_CASE(resource_priority, getpriority_self_user);
	RUN_TEST_CASE(resource_priority, setpriority_self_process);
}


TEST_GROUP(resource_rusage);

TEST_SETUP(resource_rusage)
{
}

TEST_TEAR_DOWN(resource_rusage)
{
}


TEST(resource_rusage, getrusage_self)
{
	struct rusage usage;
	int ret;

	memset(&usage, 0, sizeof(usage));
	errno = 0;
	ret = getrusage(RUSAGE_SELF, &usage);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, errno);

	/* User time should be non-negative */
	TEST_ASSERT_TRUE(usage.ru_utime.tv_sec >= 0);
	TEST_ASSERT_TRUE(usage.ru_utime.tv_usec >= 0);
	/* System time should be non-negative */
	TEST_ASSERT_TRUE(usage.ru_stime.tv_sec >= 0);
	TEST_ASSERT_TRUE(usage.ru_stime.tv_usec >= 0);
}


TEST(resource_rusage, getrusage_children)
{
	struct rusage usage;
	int ret;

	memset(&usage, 0, sizeof(usage));
	errno = 0;
	ret = getrusage(RUSAGE_CHILDREN, &usage);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, errno);

	/* Times should be non-negative */
	TEST_ASSERT_TRUE(usage.ru_utime.tv_sec >= 0);
	TEST_ASSERT_TRUE(usage.ru_stime.tv_sec >= 0);
}


TEST(resource_rusage, getrusage_self_time_increases)
{
	struct rusage usage1, usage2;
	int ret;
	volatile int i;

	ret = getrusage(RUSAGE_SELF, &usage1);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Do some work */
	for (i = 0; i < 100000; i++) {
		/* busy loop */
	}

	ret = getrusage(RUSAGE_SELF, &usage2);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Total time should be >= previous (user + system) */
	long total1 = usage1.ru_utime.tv_sec * 1000000L + usage1.ru_utime.tv_usec +
			usage1.ru_stime.tv_sec * 1000000L + usage1.ru_stime.tv_usec;
	long total2 = usage2.ru_utime.tv_sec * 1000000L + usage2.ru_utime.tv_usec +
			usage2.ru_stime.tv_sec * 1000000L + usage2.ru_stime.tv_usec;
	TEST_ASSERT_TRUE(total2 >= total1);
}


TEST_GROUP_RUNNER(resource_rusage)
{
	RUN_TEST_CASE(resource_rusage, getrusage_self);
	RUN_TEST_CASE(resource_rusage, getrusage_children);
	RUN_TEST_CASE(resource_rusage, getrusage_self_time_increases);
}
