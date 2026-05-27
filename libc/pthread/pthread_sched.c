/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <pthread.h>
 * TESTED:
 *    - pthread_getschedparam()
 *    - pthread_setschedparam()
 *    - pthread_setschedprio()
 *    - pthread_getcpuclockid()
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
#include <time.h>

#include "unity_fixture.h"


/* ===== pthread_sched group ===== */


TEST_GROUP(pthread_sched);


TEST_SETUP(pthread_sched)
{
}


TEST_TEAR_DOWN(pthread_sched)
{
}


/* pthread_getschedparam: returns 0 and valid policy for self */
TEST(pthread_sched, getschedparam_self)
{
	struct sched_param param;
	int policy;
	int ret;

	ret = pthread_getschedparam(pthread_self(), &policy, &param);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_TRUE(policy == SCHED_FIFO || policy == SCHED_RR || policy == SCHED_OTHER);
}


/* pthread_getschedparam: priority within valid range */
TEST(pthread_sched, getschedparam_priority_in_range)
{
	struct sched_param param;
	int policy;
	int minPrio;
	int maxPrio;
	int ret;

	ret = pthread_getschedparam(pthread_self(), &policy, &param);
	TEST_ASSERT_EQUAL_INT(0, ret);

	minPrio = sched_get_priority_min(policy);
	maxPrio = sched_get_priority_max(policy);

	TEST_ASSERT_TRUE(param.sched_priority >= minPrio);
	TEST_ASSERT_TRUE(param.sched_priority <= maxPrio);
}


/* pthread_setschedparam: set SCHED_OTHER with min priority */
TEST(pthread_sched, setschedparam_sched_other)
{
	struct sched_param param;
	int policy;
	int minPrio;
	int ret;

	minPrio = sched_get_priority_min(SCHED_OTHER);
	TEST_ASSERT_TRUE(minPrio >= 0);

	param.sched_priority = minPrio;

	ret = pthread_setschedparam(pthread_self(), SCHED_OTHER, &param);
	if (ret == EPERM) {
		TEST_IGNORE_MESSAGE("Insufficient privileges for pthread_setschedparam");
	}
	if (ret == ENOTSUP) {
		TEST_IGNORE_MESSAGE("SCHED_OTHER not supported for pthread_setschedparam");
	}
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_getschedparam(pthread_self(), &policy, &param);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(SCHED_OTHER, policy);
	TEST_ASSERT_EQUAL_INT(minPrio, param.sched_priority);
}


/* pthread_setschedparam: set SCHED_FIFO */
TEST(pthread_sched, setschedparam_sched_fifo)
{
	struct sched_param param;
	struct sched_param origParam;
	int policy;
	int origPolicy;
	int minPrio;
	int ret;

	/* Save original */
	ret = pthread_getschedparam(pthread_self(), &origPolicy, &origParam);
	TEST_ASSERT_EQUAL_INT(0, ret);

	minPrio = sched_get_priority_min(SCHED_FIFO);
	TEST_ASSERT_TRUE(minPrio >= 0);

	param.sched_priority = minPrio;

	ret = pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
	if (ret == EPERM) {
		TEST_IGNORE_MESSAGE("Insufficient privileges for SCHED_FIFO");
	}
	if (ret == ENOTSUP) {
		TEST_IGNORE_MESSAGE("SCHED_FIFO not supported");
	}
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_getschedparam(pthread_self(), &policy, &param);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(SCHED_FIFO, policy);
	TEST_ASSERT_EQUAL_INT(minPrio, param.sched_priority);

	/* Restore original */
	pthread_setschedparam(pthread_self(), origPolicy, &origParam);
}


/* pthread_setschedparam: set SCHED_RR */
TEST(pthread_sched, setschedparam_sched_rr)
{
	struct sched_param param;
	struct sched_param origParam;
	int policy;
	int origPolicy;
	int minPrio;
	int ret;

	/* Save original */
	ret = pthread_getschedparam(pthread_self(), &origPolicy, &origParam);
	TEST_ASSERT_EQUAL_INT(0, ret);

	minPrio = sched_get_priority_min(SCHED_RR);
	TEST_ASSERT_TRUE(minPrio >= 0);

	param.sched_priority = minPrio;

	ret = pthread_setschedparam(pthread_self(), SCHED_RR, &param);
	if (ret == EPERM) {
		TEST_IGNORE_MESSAGE("Insufficient privileges for SCHED_RR");
	}
	if (ret == ENOTSUP) {
		TEST_IGNORE_MESSAGE("SCHED_RR not supported");
	}
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_getschedparam(pthread_self(), &policy, &param);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(SCHED_RR, policy);
	TEST_ASSERT_EQUAL_INT(minPrio, param.sched_priority);

	/* Restore original */
	pthread_setschedparam(pthread_self(), origPolicy, &origParam);
}


/* pthread_setschedparam: EINVAL for invalid policy */
TEST(pthread_sched, setschedparam_invalid_policy)
{
	struct sched_param param;
	int ret;

	param.sched_priority = 0;

	ret = pthread_setschedparam(pthread_self(), -1, &param);
	TEST_ASSERT_TRUE(ret == EINVAL || ret == ENOTSUP);
}


/* pthread_setschedprio: set priority of current thread */
TEST(pthread_sched, setschedprio_self)
{
	struct sched_param param;
	int policy;
	int minPrio;
	int ret;

	ret = pthread_getschedparam(pthread_self(), &policy, &param);
	TEST_ASSERT_EQUAL_INT(0, ret);

	minPrio = sched_get_priority_min(policy);

	ret = pthread_setschedprio(pthread_self(), minPrio);
	if (ret == EPERM) {
		TEST_IGNORE_MESSAGE("Insufficient privileges for pthread_setschedprio");
	}
	if (ret == ENOTSUP) {
		TEST_IGNORE_MESSAGE("pthread_setschedprio not supported");
	}
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_getschedparam(pthread_self(), &policy, &param);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(minPrio, param.sched_priority);
}


/* pthread_setschedprio: EINVAL for out-of-range priority */
TEST(pthread_sched, setschedprio_invalid_einval)
{
	int ret;

	ret = pthread_setschedprio(pthread_self(), -9999);
	TEST_ASSERT_TRUE(ret == EINVAL || ret == EPERM || ret == ENOTSUP);
}


/* pthread_setschedparam: roundtrip preserves priority */
TEST(pthread_sched, setschedparam_roundtrip)
{
	struct sched_param param;
	struct sched_param origParam;
	int policy;
	int origPolicy;
	int maxPrio;
	int ret;

	ret = pthread_getschedparam(pthread_self(), &origPolicy, &origParam);
	TEST_ASSERT_EQUAL_INT(0, ret);

	maxPrio = sched_get_priority_max(origPolicy);

	param.sched_priority = maxPrio;

	ret = pthread_setschedparam(pthread_self(), origPolicy, &param);
	if (ret == EPERM) {
		TEST_IGNORE_MESSAGE("Insufficient privileges");
	}
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_getschedparam(pthread_self(), &policy, &param);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(origPolicy, policy);
	TEST_ASSERT_EQUAL_INT(maxPrio, param.sched_priority);

	/* Restore */
	pthread_setschedparam(pthread_self(), origPolicy, &origParam);
}


TEST_GROUP_RUNNER(pthread_sched)
{
	RUN_TEST_CASE(pthread_sched, getschedparam_self);
	RUN_TEST_CASE(pthread_sched, getschedparam_priority_in_range);
	RUN_TEST_CASE(pthread_sched, setschedparam_sched_other);
	RUN_TEST_CASE(pthread_sched, setschedparam_sched_fifo);
	RUN_TEST_CASE(pthread_sched, setschedparam_sched_rr);
	RUN_TEST_CASE(pthread_sched, setschedparam_invalid_policy);
	RUN_TEST_CASE(pthread_sched, setschedprio_self);
	RUN_TEST_CASE(pthread_sched, setschedprio_invalid_einval);
	RUN_TEST_CASE(pthread_sched, setschedparam_roundtrip);
}


/* ===== pthread_getcpuclockid group ===== */


TEST_GROUP(pthread_getcpuclockid);


TEST_SETUP(pthread_getcpuclockid)
{
}


TEST_TEAR_DOWN(pthread_getcpuclockid)
{
}


/* pthread_getcpuclockid: returns 0 for current thread */
TEST(pthread_getcpuclockid, self_success)
{
	clockid_t clkId;
	int ret;

	ret = pthread_getcpuclockid(pthread_self(), &clkId);
	if (ret == ENOENT) {
		TEST_IGNORE_MESSAGE("Per-thread CPU clock not supported");
	}
	TEST_ASSERT_EQUAL_INT(0, ret);
}


/* pthread_getcpuclockid: returned clock is usable with clock_gettime */
TEST(pthread_getcpuclockid, clock_usable)
{
	clockid_t clkId;
	struct timespec ts;
	int ret;

	ret = pthread_getcpuclockid(pthread_self(), &clkId);
	if (ret == ENOENT) {
		TEST_IGNORE_MESSAGE("Per-thread CPU clock not supported");
	}
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = clock_gettime(clkId, &ts);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_TRUE(ts.tv_sec >= 0);
	TEST_ASSERT_TRUE(ts.tv_nsec >= 0);
	TEST_ASSERT_TRUE(ts.tv_nsec < 1000000000L);
}


/* pthread_getcpuclockid: clock advances with CPU work */
TEST(pthread_getcpuclockid, clock_advances)
{
	clockid_t clkId;
	struct timespec ts1;
	struct timespec ts2;
	volatile int i;
	int ret;

	ret = pthread_getcpuclockid(pthread_self(), &clkId);
	if (ret == ENOENT) {
		TEST_IGNORE_MESSAGE("Per-thread CPU clock not supported");
	}
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = clock_gettime(clkId, &ts1);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Burn some CPU */
	for (i = 0; i < 1000000; i++) {
		/* spin */
	}

	ret = clock_gettime(clkId, &ts2);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* ts2 should be >= ts1 */
	TEST_ASSERT_TRUE((ts2.tv_sec > ts1.tv_sec) ||
		(ts2.tv_sec == ts1.tv_sec && ts2.tv_nsec >= ts1.tv_nsec));
}


static void *cpuClockThread(void *arg)
{
	clockid_t *clkId = (clockid_t *)arg;
	volatile int i;

	/* Burn some CPU so clock advances */
	for (i = 0; i < 1000000; i++) {
		/* spin */
	}

	(void)clkId;
	return NULL;
}


/* pthread_getcpuclockid: get clock for another thread */
TEST(pthread_getcpuclockid, other_thread)
{
	pthread_t thr;
	clockid_t clkId;
	struct timespec ts;
	int ret;

	ret = pthread_create(&thr, NULL, cpuClockThread, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_getcpuclockid(thr, &clkId);
	if (ret == ENOENT) {
		pthread_join(thr, NULL);
		TEST_IGNORE_MESSAGE("Per-thread CPU clock not supported");
	}
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = clock_gettime(clkId, &ts);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_join(thr, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST_GROUP_RUNNER(pthread_getcpuclockid)
{
	RUN_TEST_CASE(pthread_getcpuclockid, self_success);
	RUN_TEST_CASE(pthread_getcpuclockid, clock_usable);
	RUN_TEST_CASE(pthread_getcpuclockid, clock_advances);
	RUN_TEST_CASE(pthread_getcpuclockid, other_thread);
}
