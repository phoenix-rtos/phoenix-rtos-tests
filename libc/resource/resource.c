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


/* Tests: getpriority, setpriority */
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


TEST(resource_priority, getpriority_einval_bad_which)
{
	int prio;

	errno = 0;
	prio = getpriority(-1, 0);
	TEST_ASSERT_EQUAL_INT(-1, prio);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST(resource_priority, getpriority_esrch_invalid_pid)
{
	int prio;

	errno = 0;
	prio = getpriority(PRIO_PROCESS, 99999);
	TEST_ASSERT_EQUAL_INT(-1, prio);
	TEST_ASSERT_EQUAL_INT(ESRCH, errno);
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


TEST(resource_priority, setpriority_einval_bad_which)
{
	int ret;

	errno = 0;
	ret = setpriority(-1, 0, 0);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST(resource_priority, setpriority_esrch_invalid_pid)
{
	int ret;

	errno = 0;
	ret = setpriority(PRIO_PROCESS, 99999, 0);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(ESRCH, errno);
}


TEST_GROUP_RUNNER(resource_priority)
{
	RUN_TEST_CASE(resource_priority, getpriority_self_process);
	RUN_TEST_CASE(resource_priority, getpriority_self_pgrp);
	RUN_TEST_CASE(resource_priority, getpriority_self_user);
	RUN_TEST_CASE(resource_priority, getpriority_einval_bad_which);
	RUN_TEST_CASE(resource_priority, getpriority_esrch_invalid_pid);
	RUN_TEST_CASE(resource_priority, setpriority_self_process);
	RUN_TEST_CASE(resource_priority, setpriority_einval_bad_which);
	RUN_TEST_CASE(resource_priority, setpriority_esrch_invalid_pid);
}


/* Tests: getrlimit, setrlimit */
TEST_GROUP(resource_rlimit);

TEST_SETUP(resource_rlimit)
{
}

TEST_TEAR_DOWN(resource_rlimit)
{
}


TEST(resource_rlimit, getrlimit_nofile)
{
	struct rlimit rl;
	int ret;

	errno = 0;
	ret = getrlimit(RLIMIT_NOFILE, &rl);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, errno);
	/* Soft limit should be <= hard limit */
	TEST_ASSERT_TRUE(rl.rlim_cur <= rl.rlim_max);
}


TEST(resource_rlimit, getrlimit_stack)
{
	struct rlimit rl;
	int ret;

	ret = getrlimit(RLIMIT_STACK, &rl);
	TEST_ASSERT_EQUAL_INT(0, ret);
	/* Stack should have a positive limit */
	TEST_ASSERT_TRUE(rl.rlim_cur > 0);
	TEST_ASSERT_TRUE(rl.rlim_cur <= rl.rlim_max);
}


TEST(resource_rlimit, getrlimit_core)
{
	struct rlimit rl;
	int ret;

	ret = getrlimit(RLIMIT_CORE, &rl);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_TRUE(rl.rlim_cur <= rl.rlim_max);
}


TEST(resource_rlimit, getrlimit_cpu)
{
	struct rlimit rl;
	int ret;

	ret = getrlimit(RLIMIT_CPU, &rl);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_TRUE(rl.rlim_cur <= rl.rlim_max);
}


TEST(resource_rlimit, getrlimit_data)
{
	struct rlimit rl;
	int ret;

	ret = getrlimit(RLIMIT_DATA, &rl);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_TRUE(rl.rlim_cur <= rl.rlim_max);
}


TEST(resource_rlimit, getrlimit_fsize)
{
	struct rlimit rl;
	int ret;

	ret = getrlimit(RLIMIT_FSIZE, &rl);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_TRUE(rl.rlim_cur <= rl.rlim_max);
}


TEST(resource_rlimit, getrlimit_as)
{
	struct rlimit rl;
	int ret;

	ret = getrlimit(RLIMIT_AS, &rl);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_TRUE(rl.rlim_cur <= rl.rlim_max);
}


TEST(resource_rlimit, getrlimit_einval_invalid_resource)
{
	struct rlimit rl;
	int ret;

	errno = 0;
	ret = getrlimit(-1, &rl);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST(resource_rlimit, setrlimit_lower_soft)
{
	struct rlimit rl, newRl, checkRl;
	int ret;

	ret = getrlimit(RLIMIT_NOFILE, &rl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	if (rl.rlim_cur <= 1) {
		TEST_IGNORE_MESSAGE("RLIMIT_NOFILE soft limit too low to reduce");
	}

	/* Lower the soft limit */
	newRl.rlim_cur = rl.rlim_cur - 1;
	newRl.rlim_max = rl.rlim_max;
	errno = 0;
	ret = setrlimit(RLIMIT_NOFILE, &newRl);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, errno);

	/* Verify */
	ret = getrlimit(RLIMIT_NOFILE, &checkRl);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT((int)newRl.rlim_cur, (int)checkRl.rlim_cur);

	/* Restore */
	setrlimit(RLIMIT_NOFILE, &rl);
}


TEST(resource_rlimit, setrlimit_einval_cur_exceeds_max)
{
	struct rlimit rl, newRl;
	int ret;

	ret = getrlimit(RLIMIT_NOFILE, &rl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	if (rl.rlim_max == RLIM_INFINITY) {
		TEST_IGNORE_MESSAGE("hard limit is RLIM_INFINITY, cannot test cur > max");
	}

	/* Set soft > hard - should fail */
	newRl.rlim_cur = rl.rlim_max + 1;
	newRl.rlim_max = rl.rlim_max;
	errno = 0;
	ret = setrlimit(RLIMIT_NOFILE, &newRl);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST(resource_rlimit, setrlimit_einval_invalid_resource)
{
	struct rlimit rl;
	int ret;

	rl.rlim_cur = 100;
	rl.rlim_max = 100;
	errno = 0;
	ret = setrlimit(-1, &rl);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST(resource_rlimit, setrlimit_eperm_raise_hard)
{
	struct rlimit rl, newRl;
	int ret;

	ret = getrlimit(RLIMIT_NOFILE, &rl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	if (rl.rlim_max == RLIM_INFINITY) {
		TEST_IGNORE_MESSAGE("hard limit is already RLIM_INFINITY");
	}

	if (getuid() == 0) {
		TEST_IGNORE_MESSAGE("running as root - can raise hard limits");
	}

	/* Attempt to raise hard limit without privileges */
	newRl.rlim_cur = rl.rlim_cur;
	newRl.rlim_max = rl.rlim_max + 1;
	errno = 0;
	ret = setrlimit(RLIMIT_NOFILE, &newRl);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EPERM, errno);
}


TEST_GROUP_RUNNER(resource_rlimit)
{
	RUN_TEST_CASE(resource_rlimit, getrlimit_nofile);
	RUN_TEST_CASE(resource_rlimit, getrlimit_stack);
	RUN_TEST_CASE(resource_rlimit, getrlimit_core);
	RUN_TEST_CASE(resource_rlimit, getrlimit_cpu);
	RUN_TEST_CASE(resource_rlimit, getrlimit_data);
	RUN_TEST_CASE(resource_rlimit, getrlimit_fsize);
	RUN_TEST_CASE(resource_rlimit, getrlimit_as);
	RUN_TEST_CASE(resource_rlimit, getrlimit_einval_invalid_resource);
	RUN_TEST_CASE(resource_rlimit, setrlimit_lower_soft);
	RUN_TEST_CASE(resource_rlimit, setrlimit_einval_cur_exceeds_max);
	RUN_TEST_CASE(resource_rlimit, setrlimit_einval_invalid_resource);
	RUN_TEST_CASE(resource_rlimit, setrlimit_eperm_raise_hard);
}


/* Tests: getrusage */
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


TEST(resource_rusage, getrusage_einval_bad_who)
{
	struct rusage usage;
	int ret;

	errno = 0;
	ret = getrusage(-99, &usage);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
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
	RUN_TEST_CASE(resource_rusage, getrusage_einval_bad_who);
	RUN_TEST_CASE(resource_rusage, getrusage_self_time_increases);
}
