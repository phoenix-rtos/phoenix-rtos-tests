/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <unistd.h>
 *    - <sys/times.h>
 * TESTED:
 *    - alarm()
 *    - pause()
 *    - sleep()
 *    - nice()
 *    - setsid()
 *    - times()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

#include "unity_fixture.h"

static struct {
	volatile sig_atomic_t sigReceived;
	struct sigaction oldAct;
} test_common;


static void test_sigHandler(int sig)
{
	(void)sig;
	test_common.sigReceived = 1;
}


/* ========================================================================= */
/* alarm */
/* ========================================================================= */

TEST_GROUP(proc_alarm);

TEST_SETUP(proc_alarm)
{
	test_common.sigReceived = 0;
}

TEST_TEAR_DOWN(proc_alarm)
{
	alarm(0);
	sigaction(SIGALRM, &test_common.oldAct, NULL);
}


TEST(proc_alarm, alarm_returns_zero_no_pending)
{
	unsigned ret;

	/* No pending alarm, so alarm() returns 0 */
	ret = alarm(5);
	TEST_ASSERT_EQUAL_UINT(0, ret);

	/* Cancel */
	alarm(0);
}


TEST(proc_alarm, alarm_returns_remaining_time)
{
	unsigned ret;

	ret = alarm(10);
	TEST_ASSERT_EQUAL_UINT(0, ret);

	/* Reschedule with new value; should return remaining from previous */
	ret = alarm(5);
	TEST_ASSERT_TRUE(ret > 0);
	TEST_ASSERT_TRUE(ret <= 10);

	alarm(0);
}


TEST(proc_alarm, alarm_cancel_with_zero)
{
	unsigned ret;

	ret = alarm(10);
	TEST_ASSERT_EQUAL_UINT(0, ret);

	/* Cancel pending alarm */
	ret = alarm(0);
	TEST_ASSERT_TRUE(ret > 0);
	TEST_ASSERT_TRUE(ret <= 10);

	/* No pending alarm now */
	ret = alarm(0);
	TEST_ASSERT_EQUAL_UINT(0, ret);
}


TEST(proc_alarm, alarm_generates_sigalrm)
{
	int ret;
	struct sigaction act;

	memset(&act, 0, sizeof(act));
	act.sa_handler = test_sigHandler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;

	ret = sigaction(SIGALRM, &act, &test_common.oldAct);
	TEST_ASSERT_EQUAL_INT(0, ret);

	test_common.sigReceived = 0;
	alarm(1);

	/* Wait for signal delivery */
	sleep(2);

	TEST_ASSERT_EQUAL_INT(1, test_common.sigReceived);
}


TEST(proc_alarm, alarm_reschedule_replaces_previous)
{
	unsigned ret;

	alarm(100);

	/* Reschedule to a much shorter time */
	ret = alarm(1);
	TEST_ASSERT_TRUE(ret > 0);
	TEST_ASSERT_TRUE(ret <= 100);

	alarm(0);
}


TEST_GROUP_RUNNER(proc_alarm)
{
	RUN_TEST_CASE(proc_alarm, alarm_returns_zero_no_pending);
	RUN_TEST_CASE(proc_alarm, alarm_returns_remaining_time);
	RUN_TEST_CASE(proc_alarm, alarm_cancel_with_zero);
	RUN_TEST_CASE(proc_alarm, alarm_generates_sigalrm);
	RUN_TEST_CASE(proc_alarm, alarm_reschedule_replaces_previous);
}


/* ========================================================================= */
/* pause */
/* ========================================================================= */

TEST_GROUP(proc_pause);

TEST_SETUP(proc_pause)
{
	test_common.sigReceived = 0;
}

TEST_TEAR_DOWN(proc_pause)
{
	alarm(0);
	sigaction(SIGALRM, &test_common.oldAct, NULL);
}


TEST(proc_pause, pause_returns_eintr_on_signal)
{
	int ret;
	struct sigaction act;

	memset(&act, 0, sizeof(act));
	act.sa_handler = test_sigHandler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;

	ret = sigaction(SIGALRM, &act, &test_common.oldAct);
	TEST_ASSERT_EQUAL_INT(0, ret);

	alarm(1);

	errno = 0;
	ret = pause();
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EINTR, errno);
	TEST_ASSERT_EQUAL_INT(1, test_common.sigReceived);
}


TEST_GROUP_RUNNER(proc_pause)
{
	RUN_TEST_CASE(proc_pause, pause_returns_eintr_on_signal);
}


/* ========================================================================= */
/* sleep */
/* ========================================================================= */

TEST_GROUP(proc_sleep);

TEST_SETUP(proc_sleep)
{
	test_common.sigReceived = 0;
}

TEST_TEAR_DOWN(proc_sleep)
{
	alarm(0);
	sigaction(SIGALRM, &test_common.oldAct, NULL);
}


TEST(proc_sleep, sleep_returns_zero_on_completion)
{
	unsigned ret;

	ret = sleep(1);
	TEST_ASSERT_EQUAL_UINT(0, ret);
}


TEST(proc_sleep, sleep_zero_returns_immediately)
{
	unsigned ret;

	ret = sleep(0);
	TEST_ASSERT_EQUAL_UINT(0, ret);
}


TEST(proc_sleep, sleep_interrupted_returns_unslept)
{
	int ret;
	unsigned remaining;
	struct sigaction act;

	memset(&act, 0, sizeof(act));
	act.sa_handler = test_sigHandler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;

	ret = sigaction(SIGALRM, &act, &test_common.oldAct);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Set alarm to fire in 1 second, sleep for 5 */
	alarm(1);
	remaining = sleep(5);

	/* Should return unslept time (at least some seconds remain) */
	TEST_ASSERT_TRUE(remaining > 0);
	TEST_ASSERT_TRUE(remaining <= 4);
	TEST_ASSERT_EQUAL_INT(1, test_common.sigReceived);
}


TEST_GROUP_RUNNER(proc_sleep)
{
	RUN_TEST_CASE(proc_sleep, sleep_returns_zero_on_completion);
	RUN_TEST_CASE(proc_sleep, sleep_zero_returns_immediately);
	RUN_TEST_CASE(proc_sleep, sleep_interrupted_returns_unslept);
}


/* ========================================================================= */
/* nice */
/* ========================================================================= */

#ifndef __phoenix__

TEST_GROUP(proc_nice);

TEST_SETUP(proc_nice) { }

TEST_TEAR_DOWN(proc_nice) { }


TEST(proc_nice, nice_increase_priority_value)
{
	int ret;

	/* Increase nice value (lower priority); should succeed for any user */
	errno = 0;
	ret = nice(1);
	/* nice() returns new nice value - NZERO. Check no error */
	TEST_ASSERT_EQUAL_INT(0, errno);
	(void)ret;
}


TEST(proc_nice, nice_zero_returns_current)
{
	int ret;

	errno = 0;
	ret = nice(0);
	/* Should succeed with no error */
	TEST_ASSERT_EQUAL_INT(0, errno);
	(void)ret;
}


TEST(proc_nice, nice_clamps_to_maximum)
{
	int ret1;
	int ret2;

	/* Increase by a very large value; should clamp to 2*NZERO-1 */
	errno = 0;
	ret1 = nice(1000);
	TEST_ASSERT_EQUAL_INT(0, errno);

	/* Another increase should return the same clamped value */
	errno = 0;
	ret2 = nice(1);
	TEST_ASSERT_EQUAL_INT(0, errno);
	TEST_ASSERT_EQUAL_INT(ret1, ret2);
}


TEST(proc_nice, nice_eperm_decrease_unprivileged)
{
	int ret;
	pid_t childPid;
	int status;

	/* Fork a child to test decreasing nice (needs privilege check) */
	childPid = fork();
	TEST_ASSERT_TRUE(childPid >= 0);

	if (childPid == 0) {
		/* First increase to ensure we're not at minimum */
		if (nice(5) == -1 && errno != 0) {
			_exit(99);
		}

		/* Try to decrease (lower nice value = higher priority) */
		errno = 0;
		ret = nice(-1);
		if (ret == -1 && errno == EPERM) {
			_exit(0);
		}
		/* If we're running as root, decreasing succeeds — that's OK */
		if (errno == 0) {
			_exit(0);
		}
		_exit(1);
	}

	childPid = waitpid(childPid, &status, 0);
	TEST_ASSERT_TRUE(childPid > 0);
	TEST_ASSERT_TRUE(WIFEXITED(status));
	TEST_ASSERT_EQUAL_INT(0, WEXITSTATUS(status));
}

TEST_GROUP_RUNNER(proc_nice)
{
	RUN_TEST_CASE(proc_nice, nice_increase_priority_value);
	RUN_TEST_CASE(proc_nice, nice_zero_returns_current);
	RUN_TEST_CASE(proc_nice, nice_clamps_to_maximum);
	RUN_TEST_CASE(proc_nice, nice_eperm_decrease_unprivileged);
}
#else
TEST_GROUP_UNIMPLEMENTED(proc_nice, "nice not implemented")
#endif


/* ========================================================================= */
/* setsid */
/* ========================================================================= */

TEST_GROUP(proc_setsid);

TEST_SETUP(proc_setsid) { }

TEST_TEAR_DOWN(proc_setsid) { }


TEST(proc_setsid, setsid_creates_new_session_in_child)
{
	pid_t childPid;
	int status;

	childPid = fork();
	TEST_ASSERT_TRUE(childPid >= 0);

	if (childPid == 0) {
		pid_t sid;

		sid = setsid();
		if (sid < 0) {
			_exit(1);
		}

		/* After setsid: process group ID == process ID */
		if (getpgrp() != getpid()) {
			_exit(2);
		}

		/* Session ID == process ID */
		if (getsid(0) != getpid()) {
			_exit(3);
		}

		_exit(0);
	}

	childPid = waitpid(childPid, &status, 0);
	TEST_ASSERT_TRUE(childPid > 0);
	TEST_ASSERT_TRUE(WIFEXITED(status));
	TEST_ASSERT_EQUAL_INT(0, WEXITSTATUS(status));
}


TEST(proc_setsid, setsid_returns_new_pgid)
{
	pid_t childPid;
	int status;

	childPid = fork();
	TEST_ASSERT_TRUE(childPid >= 0);

	if (childPid == 0) {
		pid_t sid;

		sid = setsid();
		if (sid < 0) {
			_exit(1);
		}

		/* Return value equals new process group ID */
		if (sid != getpgrp()) {
			_exit(2);
		}

		_exit(0);
	}

	childPid = waitpid(childPid, &status, 0);
	TEST_ASSERT_TRUE(childPid > 0);
	TEST_ASSERT_TRUE(WIFEXITED(status));
	TEST_ASSERT_EQUAL_INT(0, WEXITSTATUS(status));
}


TEST(proc_setsid, setsid_eperm_process_group_leader)
{
	pid_t childPid;
	int status;

	childPid = fork();
	TEST_ASSERT_TRUE(childPid >= 0);

	if (childPid == 0) {
		pid_t sid;

		/* First setsid makes us session leader */
		sid = setsid();
		if (sid < 0) {
			_exit(1);
		}

		/* Second setsid should fail with EPERM (we are pgrp leader) */
		errno = 0;
		sid = setsid();
		if (sid == -1 && errno == EPERM) {
			_exit(0);
		}
		_exit(2);
	}

	childPid = waitpid(childPid, &status, 0);
	TEST_ASSERT_TRUE(childPid > 0);
	TEST_ASSERT_TRUE(WIFEXITED(status));
	TEST_ASSERT_EQUAL_INT(0, WEXITSTATUS(status));
}


TEST_GROUP_RUNNER(proc_setsid)
{
	RUN_TEST_CASE(proc_setsid, setsid_creates_new_session_in_child);
	RUN_TEST_CASE(proc_setsid, setsid_returns_new_pgid);
	RUN_TEST_CASE(proc_setsid, setsid_eperm_process_group_leader);
}


/* ========================================================================= */
/* times */
/* ========================================================================= */

TEST_GROUP(proc_times);

TEST_SETUP(proc_times) { }

TEST_TEAR_DOWN(proc_times) { }


TEST(proc_times, times_fills_structure)
{
	struct tms buf;
	clock_t ret;

	memset(&buf, 0xff, sizeof(buf));

	ret = times(&buf);
	TEST_ASSERT_TRUE(ret != (clock_t)-1);

	/* After times(), user and system times should be non-negative */
#ifdef __phoenix__
	/* #1692 issue unpublished */
	TEST_IGNORE_MESSAGE("times not implemented");
#else
	TEST_ASSERT_TRUE(buf.tms_utime >= 0);
	TEST_ASSERT_TRUE(buf.tms_stime >= 0);
	TEST_ASSERT_TRUE(buf.tms_cutime >= 0);
	TEST_ASSERT_TRUE(buf.tms_cstime >= 0);
#endif
}


TEST(proc_times, times_return_value_increases)
{
	struct tms buf;
	clock_t t1;
	clock_t t2;
	volatile long i;

	t1 = times(&buf);
	TEST_ASSERT_TRUE(t1 != (clock_t)-1);

	/* Burn some CPU time */
	for (i = 0; i < 1000000L; i++) {
		/* spin */
	}

	t2 = times(&buf);
	TEST_ASSERT_TRUE(t2 != (clock_t)-1);

	/* Elapsed time should not decrease */
	TEST_ASSERT_TRUE(t2 >= t1);
}


TEST(proc_times, times_user_time_increases_with_work)
{
	struct tms buf1;
	struct tms buf2;
	clock_t ret;
	volatile long i;

	ret = times(&buf1);
	TEST_ASSERT_TRUE(ret != (clock_t)-1);

	/* Burn CPU in user space */
	for (i = 0; i < 5000000L; i++) {
		/* spin */
	}

	ret = times(&buf2);
	TEST_ASSERT_TRUE(ret != (clock_t)-1);

	/* User time should have increased */
#ifdef __phoenix__
	/* #1692 issue unpublished */
	TEST_IGNORE_MESSAGE("times not implemented");
#else
	TEST_ASSERT_TRUE(buf2.tms_utime >= buf1.tms_utime);
#endif
}


TEST(proc_times, times_child_times_from_waited_child)
{
	struct tms buf1;
	struct tms buf2;
	clock_t ret;
	pid_t childPid;
	int status;

	ret = times(&buf1);
	TEST_ASSERT_TRUE(ret != (clock_t)-1);

	childPid = fork();
	TEST_ASSERT_TRUE(childPid >= 0);

	if (childPid == 0) {
		volatile long i;

		/* Burn CPU in child */
		for (i = 0; i < 5000000L; i++) {
			/* spin */
		}
		_exit(0);
	}

	childPid = waitpid(childPid, &status, 0);
	TEST_ASSERT_TRUE(childPid > 0);
	TEST_ASSERT_TRUE(WIFEXITED(status));

	ret = times(&buf2);
	TEST_ASSERT_TRUE(ret != (clock_t)-1);

	/* After waiting for child, cutime should include child's user time */
#ifdef __phoenix__
	/* #1692 issue unpublished */
	TEST_IGNORE_MESSAGE("times not implemented");
#else
	TEST_ASSERT_TRUE(buf2.tms_cutime >= buf1.tms_cutime);
#endif
}


TEST_GROUP_RUNNER(proc_times)
{
	RUN_TEST_CASE(proc_times, times_fills_structure);
	RUN_TEST_CASE(proc_times, times_return_value_increases);
	RUN_TEST_CASE(proc_times, times_user_time_increases_with_work);
	RUN_TEST_CASE(proc_times, times_child_times_from_waited_child);
}
