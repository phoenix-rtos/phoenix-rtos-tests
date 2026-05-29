/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <time.h>
 *    - <signal.h>
 * TESTED:
 *    - clock_getcpuclockid()
 *    - clock_getres()
 *    - timer_create()
 *    - timer_delete()
 *    - timer_getoverrun()
 *    - timer_gettime()
 *    - timer_settime()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <time.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "unity_fixture.h"

#define NSEC_PER_SEC 1000000000L


/* ========================================================================= */
/* clock_getcpuclockid */
/* ========================================================================= */

TEST_GROUP(time_clock_getcpuclockid);

TEST_SETUP(time_clock_getcpuclockid) {}

TEST_TEAR_DOWN(time_clock_getcpuclockid) {}


TEST(time_clock_getcpuclockid, getcpuclockid_self)
{
	clockid_t clk;
	int ret;

	/* pid 0 means the calling process */
	ret = clock_getcpuclockid(0, &clk);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST(time_clock_getcpuclockid, getcpuclockid_own_pid)
{
	clockid_t clk;
	int ret;

	ret = clock_getcpuclockid(getpid(), &clk);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST(time_clock_getcpuclockid, getcpuclockid_can_be_used_with_gettime)
{
	clockid_t clk;
	struct timespec tp;
	int ret;

	ret = clock_getcpuclockid(0, &clk);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = clock_gettime(clk, &tp);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_TRUE(tp.tv_sec >= 0);
	TEST_ASSERT_TRUE(tp.tv_nsec >= 0);
	TEST_ASSERT_TRUE(tp.tv_nsec < NSEC_PER_SEC);
}


TEST_GROUP_RUNNER(time_clock_getcpuclockid)
{
	RUN_TEST_CASE(time_clock_getcpuclockid, getcpuclockid_self);
	RUN_TEST_CASE(time_clock_getcpuclockid, getcpuclockid_own_pid);
	RUN_TEST_CASE(time_clock_getcpuclockid, getcpuclockid_can_be_used_with_gettime);
}


/* ========================================================================= */
/* clock_getres */
/* ========================================================================= */

TEST_GROUP(time_clock_getres);

TEST_SETUP(time_clock_getres) {}

TEST_TEAR_DOWN(time_clock_getres) {}


TEST(time_clock_getres, getres_realtime)
{
	struct timespec res;
	int ret;

	ret = clock_getres(CLOCK_REALTIME, &res);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_TRUE(res.tv_sec >= 0);
	TEST_ASSERT_TRUE(res.tv_nsec >= 0);
	TEST_ASSERT_TRUE(res.tv_nsec < NSEC_PER_SEC);
	/* Resolution should be at most 1 second */
	TEST_ASSERT_TRUE(res.tv_sec == 0 || (res.tv_sec == 1 && res.tv_nsec == 0));
}


TEST(time_clock_getres, getres_monotonic)
{
	struct timespec res;
	int ret;

	ret = clock_getres(CLOCK_MONOTONIC, &res);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_TRUE(res.tv_sec >= 0);
	TEST_ASSERT_TRUE(res.tv_nsec >= 0);
	TEST_ASSERT_TRUE(res.tv_nsec < NSEC_PER_SEC);
}


TEST(time_clock_getres, getres_null_res_ptr)
{
	int ret;

	/* Passing NULL for res is allowed — no resolution stored */
	ret = clock_getres(CLOCK_REALTIME, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST(time_clock_getres, getres_einval_bad_clockid)
{
	struct timespec res;
	int ret;

	errno = 0;
	ret = clock_getres((clockid_t)-99, &res);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST_GROUP_RUNNER(time_clock_getres)
{
	RUN_TEST_CASE(time_clock_getres, getres_realtime);
	RUN_TEST_CASE(time_clock_getres, getres_monotonic);
	RUN_TEST_CASE(time_clock_getres, getres_null_res_ptr);
	RUN_TEST_CASE(time_clock_getres, getres_einval_bad_clockid);
}


/* ========================================================================= */
/* timer_create / timer_delete */
/* ========================================================================= */

TEST_GROUP(time_timer_create);

static timer_t testTimer;

TEST_SETUP(time_timer_create)
{
	memset(&testTimer, 0, sizeof(testTimer));
}

TEST_TEAR_DOWN(time_timer_create) {}


TEST(time_timer_create, timer_create_realtime_default)
{
	int ret;

	/* NULL evp: default signal notification */
	ret = timer_create(CLOCK_REALTIME, NULL, &testTimer);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = timer_delete(testTimer);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST(time_timer_create, timer_create_monotonic)
{
	int ret;

	ret = timer_create(CLOCK_MONOTONIC, NULL, &testTimer);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = timer_delete(testTimer);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST(time_timer_create, timer_create_sigev_none)
{
	int ret;
	struct sigevent sev;

	memset(&sev, 0, sizeof(sev));
	sev.sigev_notify = SIGEV_NONE;

	ret = timer_create(CLOCK_REALTIME, &sev, &testTimer);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = timer_delete(testTimer);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST(time_timer_create, timer_create_einval_bad_clockid)
{
	int ret;

	errno = 0;
	ret = timer_create((clockid_t)-99, NULL, &testTimer);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST_GROUP_RUNNER(time_timer_create)
{
	RUN_TEST_CASE(time_timer_create, timer_create_realtime_default);
	RUN_TEST_CASE(time_timer_create, timer_create_monotonic);
	RUN_TEST_CASE(time_timer_create, timer_create_sigev_none);
	RUN_TEST_CASE(time_timer_create, timer_create_einval_bad_clockid);
}


/* ========================================================================= */
/* timer_settime / timer_gettime */
/* ========================================================================= */

TEST_GROUP(time_timer_settime);

static timer_t settimeTimer;

TEST_SETUP(time_timer_settime)
{
	int ret;
	struct sigevent sev;

	memset(&sev, 0, sizeof(sev));
	sev.sigev_notify = SIGEV_NONE;

	ret = timer_create(CLOCK_REALTIME, &sev, &settimeTimer);
	TEST_ASSERT_EQUAL_INT(0, ret);
}

TEST_TEAR_DOWN(time_timer_settime)
{
	timer_delete(settimeTimer);
}


TEST(time_timer_settime, settime_arm_relative)
{
	int ret;
	struct itimerspec its;

	memset(&its, 0, sizeof(its));
	its.it_value.tv_sec = 10;
	its.it_value.tv_nsec = 0;
	its.it_interval.tv_sec = 0;
	its.it_interval.tv_nsec = 0;

	ret = timer_settime(settimeTimer, 0, &its, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST(time_timer_settime, settime_disarm)
{
	int ret;
	struct itimerspec its;

	/* Arm the timer first */
	memset(&its, 0, sizeof(its));
	its.it_value.tv_sec = 10;
	ret = timer_settime(settimeTimer, 0, &its, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Disarm: it_value = {0, 0} — POSIX says timer shall be disarmed */
	memset(&its, 0, sizeof(its));
	ret = timer_settime(settimeTimer, 0, &its, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST(time_timer_settime, settime_returns_old_value)
{
	int ret;
	struct itimerspec its;
	struct itimerspec old;

	/* Arm with 10 seconds */
	memset(&its, 0, sizeof(its));
	its.it_value.tv_sec = 10;
	ret = timer_settime(settimeTimer, 0, &its, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Re-arm with 20 seconds; old should contain remaining from first arm */
	memset(&its, 0, sizeof(its));
	its.it_value.tv_sec = 20;
	ret = timer_settime(settimeTimer, 0, &its, &old);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Old value should be close to 10 seconds (within a second tolerance) */
	TEST_ASSERT_TRUE(old.it_value.tv_sec >= 9);
	TEST_ASSERT_TRUE(old.it_value.tv_sec <= 10);
}


TEST(time_timer_settime, settime_periodic)
{
	int ret;
	struct itimerspec its;
	struct itimerspec curr;

	memset(&its, 0, sizeof(its));
	its.it_value.tv_sec = 5;
	its.it_interval.tv_sec = 2;

	ret = timer_settime(settimeTimer, 0, &its, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = timer_gettime(settimeTimer, &curr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* it_interval should reflect what we set */
	TEST_ASSERT_EQUAL_INT(2, curr.it_interval.tv_sec);
	TEST_ASSERT_EQUAL_INT(0, curr.it_interval.tv_nsec);

	/* it_value should be close to 5 sec (within tolerance) */
	TEST_ASSERT_TRUE(curr.it_value.tv_sec >= 4);
	TEST_ASSERT_TRUE(curr.it_value.tv_sec <= 5);
}


TEST(time_timer_settime, settime_einval_bad_nsec)
{
	int ret;
	struct itimerspec its;

	memset(&its, 0, sizeof(its));
	its.it_value.tv_sec = 1;
	its.it_value.tv_nsec = -1;

	errno = 0;
	ret = timer_settime(settimeTimer, 0, &its, NULL);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST(time_timer_settime, settime_einval_nsec_too_large)
{
	int ret;
	struct itimerspec its;

	memset(&its, 0, sizeof(its));
	its.it_value.tv_sec = 1;
	its.it_value.tv_nsec = NSEC_PER_SEC;

	errno = 0;
	ret = timer_settime(settimeTimer, 0, &its, NULL);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST_GROUP_RUNNER(time_timer_settime)
{
	RUN_TEST_CASE(time_timer_settime, settime_arm_relative);
	RUN_TEST_CASE(time_timer_settime, settime_disarm);
	RUN_TEST_CASE(time_timer_settime, settime_returns_old_value);
	RUN_TEST_CASE(time_timer_settime, settime_periodic);
	RUN_TEST_CASE(time_timer_settime, settime_einval_bad_nsec);
	RUN_TEST_CASE(time_timer_settime, settime_einval_nsec_too_large);
}


/* ========================================================================= */
/* timer_gettime (standalone checks) */
/* ========================================================================= */

TEST_GROUP(time_timer_gettime);

static timer_t gettimeTimer;

TEST_SETUP(time_timer_gettime)
{
	int ret;
	struct sigevent sev;

	memset(&sev, 0, sizeof(sev));
	sev.sigev_notify = SIGEV_NONE;

	ret = timer_create(CLOCK_REALTIME, &sev, &gettimeTimer);
	TEST_ASSERT_EQUAL_INT(0, ret);
}

TEST_TEAR_DOWN(time_timer_gettime)
{
	timer_delete(gettimeTimer);
}


TEST(time_timer_gettime, gettime_disarmed_timer)
{
	int ret;
	struct itimerspec curr;

	/* Newly created timer is disarmed */
	ret = timer_gettime(gettimeTimer, &curr);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, curr.it_value.tv_sec);
	TEST_ASSERT_EQUAL_INT(0, curr.it_value.tv_nsec);
	TEST_ASSERT_EQUAL_INT(0, curr.it_interval.tv_sec);
	TEST_ASSERT_EQUAL_INT(0, curr.it_interval.tv_nsec);
}


TEST(time_timer_gettime, gettime_armed_timer_remaining)
{
	int ret;
	struct itimerspec its;
	struct itimerspec curr;

	memset(&its, 0, sizeof(its));
	its.it_value.tv_sec = 60;

	ret = timer_settime(gettimeTimer, 0, &its, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = timer_gettime(gettimeTimer, &curr);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Should have close to 60 seconds remaining */
	TEST_ASSERT_TRUE(curr.it_value.tv_sec >= 58);
	TEST_ASSERT_TRUE(curr.it_value.tv_sec <= 60);
}


TEST_GROUP_RUNNER(time_timer_gettime)
{
	RUN_TEST_CASE(time_timer_gettime, gettime_disarmed_timer);
	RUN_TEST_CASE(time_timer_gettime, gettime_armed_timer_remaining);
}


/* ========================================================================= */
/* timer_getoverrun */
/* ========================================================================= */

TEST_GROUP(time_timer_getoverrun);

static timer_t overrunTimer;

TEST_SETUP(time_timer_getoverrun)
{
	int ret;
	struct sigevent sev;

	memset(&sev, 0, sizeof(sev));
	sev.sigev_notify = SIGEV_NONE;

	ret = timer_create(CLOCK_REALTIME, &sev, &overrunTimer);
	TEST_ASSERT_EQUAL_INT(0, ret);
}

TEST_TEAR_DOWN(time_timer_getoverrun)
{
	timer_delete(overrunTimer);
}


TEST(time_timer_getoverrun, getoverrun_returns_nonnegative)
{
	int ret;

	/* For a timer that hasn't fired, behavior is unspecified per POSIX,
	 * but on Linux it returns 0. Just verify it doesn't return an error. */
	ret = timer_getoverrun(overrunTimer);
	TEST_ASSERT_TRUE(ret >= 0);
}


TEST_GROUP_RUNNER(time_timer_getoverrun)
{
	RUN_TEST_CASE(time_timer_getoverrun, getoverrun_returns_nonnegative);
}
