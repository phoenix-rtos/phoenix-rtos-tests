/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <sched.h>
 * TESTED:
 *    - sched_getparam()
 *    - sched_getscheduler()
 *    - sched_rr_get_interval()
 *    - sched_setparam()
 *    - sched_setscheduler()
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
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "unity_fixture.h"

#ifndef __phoenix__
/* Tests: sched_getparam, sched_setparam */
TEST_GROUP(sched_param);

static struct {
	int origPolicy;
	struct sched_param origParam;
} test_common;


TEST_SETUP(sched_param)
{
	/* Save original scheduling parameters */
	test_common.origPolicy = sched_getscheduler(0);
	TEST_ASSERT_GREATER_THAN_INT(-1, test_common.origPolicy);
	TEST_ASSERT_EQUAL_INT(0, sched_getparam(0, &test_common.origParam));
}

TEST_TEAR_DOWN(sched_param)
{
	/* Restore original scheduling policy and parameters */
	sched_setscheduler(0, test_common.origPolicy, &test_common.origParam);
}


TEST(sched_param, getparam_self_pid_zero)
{
	struct sched_param param;
	int ret;

	memset(&param, 0xff, sizeof(param));
	errno = 0;
	ret = sched_getparam(0, &param);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, errno);
}


TEST(sched_param, getparam_self_own_pid)
{
	struct sched_param param;
	int ret;
	pid_t myPid;

	myPid = getpid();
	memset(&param, 0xff, sizeof(param));
	errno = 0;
	ret = sched_getparam(myPid, &param);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, errno);
}


TEST(sched_param, getparam_esrch_invalid_pid)
{
	struct sched_param param;
	int ret;

	errno = 0;
	ret = sched_getparam(99999, &param);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(ESRCH, errno);
}


TEST(sched_param, setparam_self)
{
	struct sched_param param;
	struct sched_param getParam;
	int ret;
	int minPrio;

	minPrio = sched_get_priority_min(test_common.origPolicy);
	TEST_ASSERT_GREATER_THAN_INT(-1, minPrio);

	param.sched_priority = minPrio;
	errno = 0;
	ret = sched_setparam(0, &param);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, errno);

	/* Verify the param was set */
	ret = sched_getparam(0, &getParam);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(minPrio, getParam.sched_priority);
}


TEST(sched_param, setparam_einval_out_of_range)
{
	struct sched_param param;
	int ret;
	int maxPrio;

	maxPrio = sched_get_priority_max(test_common.origPolicy);
	TEST_ASSERT_GREATER_THAN_INT(-1, maxPrio);

	/* Priority above maximum */
	param.sched_priority = maxPrio + 100;
	errno = 0;
	ret = sched_setparam(0, &param);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST(sched_param, setparam_esrch_invalid_pid)
{
	struct sched_param param;
	int ret;

	param.sched_priority = 0;
	errno = 0;
	ret = sched_setparam(99999, &param);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(ESRCH, errno);
}

TEST_GROUP_RUNNER(sched_param)
{
	RUN_TEST_CASE(sched_param, getparam_self_pid_zero);
	RUN_TEST_CASE(sched_param, getparam_self_own_pid);
	RUN_TEST_CASE(sched_param, getparam_esrch_invalid_pid);
	RUN_TEST_CASE(sched_param, setparam_self);
	RUN_TEST_CASE(sched_param, setparam_einval_out_of_range);
	RUN_TEST_CASE(sched_param, setparam_esrch_invalid_pid);
}
#else
TEST_GROUP_UNIMPLEMENTED(sched_param, "sched_getparam and sched_setparam not implemented")
#endif


#ifndef __phoenix__
/* Tests: sched_getscheduler, sched_setscheduler */
TEST_GROUP(sched_scheduler);

TEST_SETUP(sched_scheduler)
{
	test_common.origPolicy = sched_getscheduler(0);
	TEST_ASSERT_GREATER_THAN_INT(-1, test_common.origPolicy);
	TEST_ASSERT_EQUAL_INT(0, sched_getparam(0, &test_common.origParam));
}

TEST_TEAR_DOWN(sched_scheduler)
{
	sched_setscheduler(0, test_common.origPolicy, &test_common.origParam);
}


TEST(sched_scheduler, getscheduler_self_pid_zero)
{
	int policy;

	errno = 0;
	policy = sched_getscheduler(0);
	TEST_ASSERT_GREATER_THAN_INT(-1, policy);
	TEST_ASSERT_EQUAL_INT(0, errno);
	/* Result should be one of the known policies */
	TEST_ASSERT_TRUE(policy == SCHED_OTHER || policy == SCHED_FIFO || policy == SCHED_RR);
}


TEST(sched_scheduler, getscheduler_self_own_pid)
{
	int policy;
	pid_t myPid;

	myPid = getpid();
	errno = 0;
	policy = sched_getscheduler(myPid);
	TEST_ASSERT_GREATER_THAN_INT(-1, policy);
	TEST_ASSERT_EQUAL_INT(0, errno);
}


TEST(sched_scheduler, getscheduler_esrch_invalid_pid)
{
	int ret;

	errno = 0;
	ret = sched_getscheduler(99999);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(ESRCH, errno);
}


TEST(sched_scheduler, setscheduler_sched_other)
{
	struct sched_param param;
	int ret;
	int minPrio;

	minPrio = sched_get_priority_min(SCHED_OTHER);
	TEST_ASSERT_GREATER_THAN_INT(-1, minPrio);

	param.sched_priority = minPrio;
	errno = 0;
	ret = sched_setscheduler(0, SCHED_OTHER, &param);
	/* Returns former policy on success */
	TEST_ASSERT_GREATER_THAN_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(0, errno);

	/* Verify */
	TEST_ASSERT_EQUAL_INT(SCHED_OTHER, sched_getscheduler(0));
}


TEST(sched_scheduler, setscheduler_returns_former_policy)
{
	struct sched_param param;
	int ret;
	int currentPolicy;
	int minPrio;

	currentPolicy = sched_getscheduler(0);
	TEST_ASSERT_GREATER_THAN_INT(-1, currentPolicy);

	minPrio = sched_get_priority_min(SCHED_OTHER);
	TEST_ASSERT_GREATER_THAN_INT(-1, minPrio);

	param.sched_priority = minPrio;
	ret = sched_setscheduler(0, SCHED_OTHER, &param);
	TEST_ASSERT_EQUAL_INT(currentPolicy, ret);
}


TEST(sched_scheduler, setscheduler_einval_invalid_policy)
{
	struct sched_param param;
	int ret;

	param.sched_priority = 0;
	errno = 0;
	ret = sched_setscheduler(0, -1, &param);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST(sched_scheduler, setscheduler_einval_invalid_priority)
{
	struct sched_param param;
	int ret;
	int maxPrio;

	maxPrio = sched_get_priority_max(SCHED_OTHER);
	TEST_ASSERT_GREATER_THAN_INT(-1, maxPrio);

	param.sched_priority = maxPrio + 100;
	errno = 0;
	ret = sched_setscheduler(0, SCHED_OTHER, &param);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST(sched_scheduler, setscheduler_esrch_invalid_pid)
{
	struct sched_param param;
	int ret;

	param.sched_priority = 0;
	errno = 0;
	ret = sched_setscheduler(99999, SCHED_OTHER, &param);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(ESRCH, errno);
}

TEST_GROUP_RUNNER(sched_scheduler)
{
	RUN_TEST_CASE(sched_scheduler, getscheduler_self_pid_zero);
	RUN_TEST_CASE(sched_scheduler, getscheduler_self_own_pid);
	RUN_TEST_CASE(sched_scheduler, getscheduler_esrch_invalid_pid);
	RUN_TEST_CASE(sched_scheduler, setscheduler_sched_other);
	RUN_TEST_CASE(sched_scheduler, setscheduler_returns_former_policy);
	RUN_TEST_CASE(sched_scheduler, setscheduler_einval_invalid_policy);
	RUN_TEST_CASE(sched_scheduler, setscheduler_einval_invalid_priority);
	RUN_TEST_CASE(sched_scheduler, setscheduler_esrch_invalid_pid);
}
#else
TEST_GROUP_UNIMPLEMENTED(sched_scheduler, "sched_getscheduler and sched_setscheduler not implemented")
#endif


#ifndef __phoenix__
/* Tests: sched_rr_get_interval */
TEST_GROUP(sched_rr_get_interval);

TEST_SETUP(sched_rr_get_interval)
{
}

TEST_TEAR_DOWN(sched_rr_get_interval)
{
}


TEST(sched_rr_get_interval, self_pid_zero)
{
	struct timespec interval;
	int ret;

	memset(&interval, 0, sizeof(interval));
	errno = 0;
	ret = sched_rr_get_interval(0, &interval);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, errno);
	/* Interval should be non-negative */
	TEST_ASSERT_TRUE(interval.tv_sec >= 0);
	TEST_ASSERT_TRUE(interval.tv_nsec >= 0);
}


TEST(sched_rr_get_interval, self_own_pid)
{
	struct timespec interval;
	int ret;
	pid_t myPid;

	myPid = getpid();
	memset(&interval, 0, sizeof(interval));
	errno = 0;
	ret = sched_rr_get_interval(myPid, &interval);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, errno);
	TEST_ASSERT_TRUE(interval.tv_sec >= 0);
	TEST_ASSERT_TRUE(interval.tv_nsec >= 0);
}


TEST(sched_rr_get_interval, esrch_invalid_pid)
{
	struct timespec interval;
	int ret;

	errno = 0;
	ret = sched_rr_get_interval(99999, &interval);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(ESRCH, errno);
}


TEST(sched_rr_get_interval, interval_positive_under_rr)
{
	struct sched_param param;
	struct timespec interval;
	int ret;
	int origPolicy;
	struct sched_param origParam;
	int minPrio;

	/* Save original */
	origPolicy = sched_getscheduler(0);
	TEST_ASSERT_GREATER_THAN_INT(-1, origPolicy);
	TEST_ASSERT_EQUAL_INT(0, sched_getparam(0, &origParam));

	minPrio = sched_get_priority_min(SCHED_RR);
	TEST_ASSERT_GREATER_THAN_INT(-1, minPrio);

	param.sched_priority = minPrio;
	ret = sched_setscheduler(0, SCHED_RR, &param);
	if (ret == -1 && errno == EPERM) {
		/* Restore and skip if no permission */
		sched_setscheduler(0, origPolicy, &origParam);
		TEST_IGNORE_MESSAGE("insufficient privileges for SCHED_RR");
	}
	TEST_ASSERT_GREATER_THAN_INT(-1, ret);

	memset(&interval, 0, sizeof(interval));
	ret = sched_rr_get_interval(0, &interval);
	TEST_ASSERT_EQUAL_INT(0, ret);
	/* Under SCHED_RR the interval should be positive */
	TEST_ASSERT_TRUE(interval.tv_sec > 0 || interval.tv_nsec > 0);

	/* Restore */
	sched_setscheduler(0, origPolicy, &origParam);
}

TEST_GROUP_RUNNER(sched_rr_get_interval)
{
	RUN_TEST_CASE(sched_rr_get_interval, self_pid_zero);
	RUN_TEST_CASE(sched_rr_get_interval, self_own_pid);
	RUN_TEST_CASE(sched_rr_get_interval, esrch_invalid_pid);
	RUN_TEST_CASE(sched_rr_get_interval, interval_positive_under_rr);
}
#else
TEST_GROUP_UNIMPLEMENTED(sched_rr_get_interval, "sched_getscheduler sched_setscheduler sched_rr_get_interval sched_get_priority_min not implemented")
#endif
