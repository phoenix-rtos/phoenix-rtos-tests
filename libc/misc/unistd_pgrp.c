/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <unistd.h>
 * TESTED:
 *    - getpgid()
 *    - getpgrp()
 *    - getsid()
 *    - setpgrp()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>

#include "unity_fixture.h"


/*
 * Tests: getpgid
 */

TEST_GROUP(unistd_getpgid);


TEST_SETUP(unistd_getpgid)
{
}


TEST_TEAR_DOWN(unistd_getpgid)
{
}


TEST(unistd_getpgid, getpgid_self_zero)
{
	/* "If pid is equal to 0, getpgid() shall return the process group ID
	 *  of the calling process." */
	pid_t pgid = getpgid(0);
	TEST_ASSERT_GREATER_THAN_INT(0, (int)pgid);
}


TEST(unistd_getpgid, getpgid_self_pid)
{
	/* getpgid(getpid()) shall be equivalent to getpgid(0) */
	pid_t pgid0 = getpgid(0);
	pid_t pgidSelf = getpgid(getpid());
	TEST_ASSERT_EQUAL_INT((int)pgid0, (int)pgidSelf);
}


TEST(unistd_getpgid, getpgid_equals_getpgrp)
{
	/* getpgid(0) shall equal getpgrp() for the calling process */
	pid_t pgid = getpgid(0);
	pid_t pgrp = getpgrp();
	TEST_ASSERT_EQUAL_INT((int)pgrp, (int)pgid);
}


TEST(unistd_getpgid, getpgid_esrch_invalid_pid)
{
	/* "ESRCH: There is no process with a process ID equal to pid." */
	pid_t badPid = 99999;
	pid_t ret;

	errno = 0;
	ret = getpgid(badPid);
	if (ret == (pid_t)-1) {
		TEST_ASSERT_EQUAL_INT(ESRCH, errno);
	}
	else {
		/* On some systems the PID might exist; try a larger one */
		errno = 0;
		ret = getpgid(2000000);
		if (ret == (pid_t)-1) {
			TEST_ASSERT_EQUAL_INT(ESRCH, errno);
		}
		else {
			TEST_IGNORE_MESSAGE("could not find unused PID to trigger ESRCH");
		}
	}
}


TEST(unistd_getpgid, getpgid_child_inherits)
{
	/* Child process inherits parent's process group */
	pid_t parentPgid = getpgid(0);
	pid_t pid;

	pid = fork();
	TEST_ASSERT_NOT_EQUAL_INT(-1, (int)pid);

	if (pid == 0) {
		pid_t childPgid = getpgid(0);
		_exit((childPgid == parentPgid) ? 0 : 1);
	}
	else {
		int status;
		TEST_ASSERT_EQUAL_INT(pid, (int)waitpid(pid, &status, 0));
		TEST_ASSERT_TRUE(WIFEXITED(status));
		TEST_ASSERT_EQUAL_INT(0, WEXITSTATUS(status));
	}
}


TEST_GROUP_RUNNER(unistd_getpgid)
{
	RUN_TEST_CASE(unistd_getpgid, getpgid_self_zero);
	RUN_TEST_CASE(unistd_getpgid, getpgid_self_pid);
	RUN_TEST_CASE(unistd_getpgid, getpgid_equals_getpgrp);
	RUN_TEST_CASE(unistd_getpgid, getpgid_esrch_invalid_pid);
	RUN_TEST_CASE(unistd_getpgid, getpgid_child_inherits);
}


/*
 * Tests: getpgrp
 */

TEST_GROUP(unistd_getpgrp);


TEST_SETUP(unistd_getpgrp)
{
}


TEST_TEAR_DOWN(unistd_getpgrp)
{
}


TEST(unistd_getpgrp, getpgrp_returns_positive)
{
	/* "getpgrp() shall return the process group ID of the calling process."
	 * "No errors are defined." — always succeeds. */
	pid_t pgrp = getpgrp();
	TEST_ASSERT_GREATER_THAN_INT(0, (int)pgrp);
}


TEST(unistd_getpgrp, getpgrp_consistent)
{
	/* Two consecutive calls return the same value */
	pid_t pgrp1 = getpgrp();
	pid_t pgrp2 = getpgrp();
	TEST_ASSERT_EQUAL_INT((int)pgrp1, (int)pgrp2);
}


TEST(unistd_getpgrp, getpgrp_child_inherits)
{
	/* Child inherits parent's process group */
	pid_t parentPgrp = getpgrp();
	pid_t pid;

	pid = fork();
	TEST_ASSERT_NOT_EQUAL_INT(-1, (int)pid);

	if (pid == 0) {
		_exit((getpgrp() == parentPgrp) ? 0 : 1);
	}
	else {
		int status;
		TEST_ASSERT_EQUAL_INT(pid, (int)waitpid(pid, &status, 0));
		TEST_ASSERT_TRUE(WIFEXITED(status));
		TEST_ASSERT_EQUAL_INT(0, WEXITSTATUS(status));
	}
}


TEST_GROUP_RUNNER(unistd_getpgrp)
{
	RUN_TEST_CASE(unistd_getpgrp, getpgrp_returns_positive);
	RUN_TEST_CASE(unistd_getpgrp, getpgrp_consistent);
	RUN_TEST_CASE(unistd_getpgrp, getpgrp_child_inherits);
}


/*
 * Tests: getsid
 */

TEST_GROUP(unistd_getsid);


TEST_SETUP(unistd_getsid)
{
}


TEST_TEAR_DOWN(unistd_getsid)
{
}


TEST(unistd_getsid, getsid_self_zero)
{
	/* "If pid is (pid_t)0, it specifies the calling process." */
	pid_t sid = getsid(0);
	TEST_ASSERT_GREATER_THAN_INT(0, (int)sid);
}


TEST(unistd_getsid, getsid_self_pid)
{
	/* getsid(getpid()) shall return the same as getsid(0) */
	pid_t sid0 = getsid(0);
	pid_t sidSelf = getsid(getpid());
	TEST_ASSERT_EQUAL_INT((int)sid0, (int)sidSelf);
}


TEST(unistd_getsid, getsid_consistent)
{
	/* Two calls return the same session ID */
	pid_t sid1 = getsid(0);
	pid_t sid2 = getsid(0);
	TEST_ASSERT_EQUAL_INT((int)sid1, (int)sid2);
}


TEST(unistd_getsid, getsid_esrch_invalid_pid)
{
	/* "ESRCH: There is no process with a process ID equal to pid." */
	pid_t ret;

	errno = 0;
	ret = getsid(2000000);
	if (ret == (pid_t)-1) {
		TEST_ASSERT_EQUAL_INT(ESRCH, errno);
	}
	else {
		TEST_IGNORE_MESSAGE("PID 2000000 exists on this system");
	}
}


TEST(unistd_getsid, getsid_child_same_session)
{
	/* Child shall be in the same session as parent */
	pid_t parentSid = getsid(0);
	pid_t pid;

	pid = fork();
	TEST_ASSERT_NOT_EQUAL_INT(-1, (int)pid);

	if (pid == 0) {
		_exit((getsid(0) == parentSid) ? 0 : 1);
	}
	else {
		int status;
		TEST_ASSERT_EQUAL_INT(pid, (int)waitpid(pid, &status, 0));
		TEST_ASSERT_TRUE(WIFEXITED(status));
		TEST_ASSERT_EQUAL_INT(0, WEXITSTATUS(status));
	}
}


TEST_GROUP_RUNNER(unistd_getsid)
{
	RUN_TEST_CASE(unistd_getsid, getsid_self_zero);
	RUN_TEST_CASE(unistd_getsid, getsid_self_pid);
	RUN_TEST_CASE(unistd_getsid, getsid_consistent);
	RUN_TEST_CASE(unistd_getsid, getsid_esrch_invalid_pid);
	RUN_TEST_CASE(unistd_getsid, getsid_child_same_session);
}


/*
 * Tests: setpgrp
 */

TEST_GROUP(unistd_setpgrp);


TEST_SETUP(unistd_setpgrp)
{
}


TEST_TEAR_DOWN(unistd_setpgrp)
{
}


TEST(unistd_setpgrp, setpgrp_returns_pgid)
{
	/* "Upon completion, setpgrp() shall return the process group ID." */
	/* Run in child to avoid modifying the parent's session/group */
	pid_t pid;

	pid = fork();
	TEST_ASSERT_NOT_EQUAL_INT(-1, (int)pid);

	if (pid == 0) {
		pid_t ret = setpgrp();
		/* setpgrp sets pgid to process ID if not session leader.
		 * On glibc, setpgrp() returns 0 on success (not the pgid). */
		pid_t pgid = getpgrp();
		if (ret == pgid || ret == 0) {
			_exit(0);
		}
		_exit(1);
	}
	else {
		int status;
		TEST_ASSERT_EQUAL_INT(pid, (int)waitpid(pid, &status, 0));
		TEST_ASSERT_TRUE(WIFEXITED(status));
		TEST_ASSERT_EQUAL_INT(0, WEXITSTATUS(status));
	}
}


TEST(unistd_setpgrp, setpgrp_sets_pgid_to_pid)
{
	/* "setpgrp() sets the process group ID of the calling process
	 *  to the process ID of the calling process" (if not session leader) */
	pid_t pid;

	pid = fork();
	TEST_ASSERT_NOT_EQUAL_INT(-1, (int)pid);

	if (pid == 0) {
		pid_t myPid = getpid();
		setpgrp();
		pid_t pgid = getpgid(0);
		_exit((pgid == myPid) ? 0 : 1);
	}
	else {
		int status;
		TEST_ASSERT_EQUAL_INT(pid, (int)waitpid(pid, &status, 0));
		TEST_ASSERT_TRUE(WIFEXITED(status));
		TEST_ASSERT_EQUAL_INT(0, WEXITSTATUS(status));
	}
}


TEST(unistd_setpgrp, setpgrp_session_leader_no_effect)
{
	/* "setpgrp() has no effect when the calling process is a session leader." */
	pid_t pid;

	pid = fork();
	TEST_ASSERT_NOT_EQUAL_INT(-1, (int)pid);

	if (pid == 0) {
		pid_t sid;
		pid_t pgidBefore;
		pid_t pgidAfter;

		/* Become session leader */
		sid = setsid();
		if (sid == (pid_t)-1) {
			_exit(2);
		}
		pgidBefore = getpgrp();
		setpgrp();
		pgidAfter = getpgrp();
		/* No effect: pgid unchanged */
		_exit((pgidBefore == pgidAfter) ? 0 : 1);
	}
	else {
		int status;
		TEST_ASSERT_EQUAL_INT(pid, (int)waitpid(pid, &status, 0));
		TEST_ASSERT_TRUE(WIFEXITED(status));
		TEST_ASSERT_EQUAL_INT(0, WEXITSTATUS(status));
	}
}


TEST_GROUP_RUNNER(unistd_setpgrp)
{
	RUN_TEST_CASE(unistd_setpgrp, setpgrp_returns_pgid);
	RUN_TEST_CASE(unistd_setpgrp, setpgrp_sets_pgid_to_pid);
	RUN_TEST_CASE(unistd_setpgrp, setpgrp_session_leader_no_effect);
}
