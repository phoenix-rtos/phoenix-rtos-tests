/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <unistd.h>
 *    - <sys/wait.h>
 * TESTED:
 *    - fork()
 *    - waitid()
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
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "unity_fixture.h"


/* ========================================================================= */
/* fork */
/* ========================================================================= */

TEST_GROUP(proc_fork);

TEST_SETUP(proc_fork) {}

TEST_TEAR_DOWN(proc_fork) {}


TEST(proc_fork, fork_returns_zero_to_child)
{
	pid_t childPid;
	int status;

	childPid = fork();
	TEST_ASSERT_TRUE(childPid >= 0);

	if (childPid == 0) {
		/* In child: fork returned 0 */
		_exit(0);
	}

	childPid = waitpid(childPid, &status, 0);
	TEST_ASSERT_TRUE(childPid > 0);
	TEST_ASSERT_TRUE(WIFEXITED(status));
	TEST_ASSERT_EQUAL_INT(0, WEXITSTATUS(status));
}


TEST(proc_fork, fork_returns_child_pid_to_parent)
{
	pid_t childPid;
	int status;

	childPid = fork();
	TEST_ASSERT_TRUE(childPid >= 0);

	if (childPid == 0) {
		_exit(42);
	}

	/* Parent: childPid should be > 0 */
	TEST_ASSERT_TRUE(childPid > 0);

	childPid = waitpid(childPid, &status, 0);
	TEST_ASSERT_TRUE(childPid > 0);
	TEST_ASSERT_TRUE(WIFEXITED(status));
	TEST_ASSERT_EQUAL_INT(42, WEXITSTATUS(status));
}


TEST(proc_fork, fork_child_has_unique_pid)
{
	pid_t parentPid;
	pid_t childPid;
	int pipeFds[2];
	int ret;
	int status;
	pid_t childSelfPid;

	parentPid = getpid();

	ret = pipe(pipeFds);
	TEST_ASSERT_EQUAL_INT(0, ret);

	childPid = fork();
	TEST_ASSERT_TRUE(childPid >= 0);

	if (childPid == 0) {
		pid_t myPid = getpid();
		close(pipeFds[0]);
		if (write(pipeFds[1], &myPid, sizeof(myPid)) < 0) {
			_exit(99);
		}
		close(pipeFds[1]);
		_exit(0);
	}

	close(pipeFds[1]);
	ret = read(pipeFds[0], &childSelfPid, sizeof(childSelfPid));
	TEST_ASSERT_EQUAL_INT((int)sizeof(childSelfPid), ret);
	close(pipeFds[0]);

	/* Child's PID must differ from parent's */
	TEST_ASSERT_TRUE(childSelfPid != parentPid);
	/* fork() return in parent must match child's getpid() */
	TEST_ASSERT_EQUAL_INT(childPid, childSelfPid);

	waitpid(childPid, &status, 0);
}


TEST(proc_fork, fork_child_has_different_parent_pid)
{
	pid_t parentPid;
	pid_t childPid;
	int pipeFds[2];
	int ret;
	int status;
	pid_t childParentPid;

	parentPid = getpid();

	ret = pipe(pipeFds);
	TEST_ASSERT_EQUAL_INT(0, ret);

	childPid = fork();
	TEST_ASSERT_TRUE(childPid >= 0);

	if (childPid == 0) {
		pid_t ppid = getppid();
		close(pipeFds[0]);
		if (write(pipeFds[1], &ppid, sizeof(ppid)) < 0) {
			_exit(99);
		}
		close(pipeFds[1]);
		_exit(0);
	}

	close(pipeFds[1]);
	ret = read(pipeFds[0], &childParentPid, sizeof(childParentPid));
	TEST_ASSERT_EQUAL_INT((int)sizeof(childParentPid), ret);
	close(pipeFds[0]);

	/* Child's parent PID should be our PID */
	TEST_ASSERT_EQUAL_INT(parentPid, childParentPid);

	waitpid(childPid, &status, 0);
}


TEST(proc_fork, fork_child_inherits_fd)
{
	pid_t childPid;
	int pipeFds[2];
	int ret;
	int status;
	char buf[4];
	const char msg[] = "hi";

	ret = pipe(pipeFds);
	TEST_ASSERT_EQUAL_INT(0, ret);

	childPid = fork();
	TEST_ASSERT_TRUE(childPid >= 0);

	if (childPid == 0) {
		/* Child writes to inherited pipe write end */
		close(pipeFds[0]);
		if (write(pipeFds[1], msg, strlen(msg)) < 0) {
			_exit(99);
		}
		close(pipeFds[1]);
		_exit(0);
	}

	close(pipeFds[1]);
	memset(buf, 0, sizeof(buf));
	ret = read(pipeFds[0], buf, sizeof(buf) - 1);
	TEST_ASSERT_EQUAL_INT(2, ret);
	TEST_ASSERT_EQUAL_STRING("hi", buf);
	close(pipeFds[0]);

	waitpid(childPid, &status, 0);
}


TEST(proc_fork, fork_child_pending_signals_empty)
{
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("sigpending not implemented");
#else
	pid_t childPid;
	int pipeFds[2];
	int ret;
	int status;
	int hasSignals;

	/* Raise a signal and block it so it's pending in parent */
	sigset_t blockSet;
	sigemptyset(&blockSet);
	sigaddset(&blockSet, SIGUSR1);
	ret = sigprocmask(SIG_BLOCK, &blockSet, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	raise(SIGUSR1);

	ret = pipe(pipeFds);
	TEST_ASSERT_EQUAL_INT(0, ret);

	childPid = fork();
	TEST_ASSERT_TRUE(childPid >= 0);

	if (childPid == 0) {
		int sig;
		sigset_t childPending;

		sigpending(&childPending);
		sig = sigismember(&childPending, SIGUSR1);
		close(pipeFds[0]);
		if (write(pipeFds[1], &sig, sizeof(sig)) < 0) {
			_exit(99);
		}
		close(pipeFds[1]);
		_exit(0);
	}

	close(pipeFds[1]);
	ret = read(pipeFds[0], &hasSignals, sizeof(hasSignals));
	TEST_ASSERT_EQUAL_INT((int)sizeof(hasSignals), ret);
	close(pipeFds[0]);

	/* Child's pending signal set should be empty */
	TEST_ASSERT_EQUAL_INT(0, hasSignals);

	waitpid(childPid, &status, 0);

	/* Clean up: unblock and consume the pending signal in parent */
	{
		struct sigaction act;
		struct sigaction oldAct;
		memset(&act, 0, sizeof(act));
		act.sa_handler = SIG_IGN;
		sigaction(SIGUSR1, &act, &oldAct);
		sigprocmask(SIG_UNBLOCK, &blockSet, NULL);
		sigaction(SIGUSR1, &oldAct, NULL);
	}
#endif
}


TEST(proc_fork, fork_child_alarm_cleared)
{
	pid_t childPid;
	int pipeFds[2];
	int ret;
	int status;
	unsigned childAlarm;

	alarm(100);

	ret = pipe(pipeFds);
	TEST_ASSERT_EQUAL_INT(0, ret);

	childPid = fork();
	TEST_ASSERT_TRUE(childPid >= 0);

	if (childPid == 0) {
		unsigned a = alarm(0);
		close(pipeFds[0]);
		if (write(pipeFds[1], &a, sizeof(a)) < 0) {
			_exit(99);
		}
		close(pipeFds[1]);
		_exit(0);
	}

	close(pipeFds[1]);
	ret = read(pipeFds[0], &childAlarm, sizeof(childAlarm));
	TEST_ASSERT_EQUAL_INT((int)sizeof(childAlarm), ret);
	close(pipeFds[0]);

	/* fork() clears pending alarms in the child */
	TEST_ASSERT_EQUAL_UINT(0, childAlarm);

	alarm(0);
	waitpid(childPid, &status, 0);
}


TEST_GROUP_RUNNER(proc_fork)
{
	RUN_TEST_CASE(proc_fork, fork_returns_zero_to_child);
	RUN_TEST_CASE(proc_fork, fork_returns_child_pid_to_parent);
	RUN_TEST_CASE(proc_fork, fork_child_has_unique_pid);
	RUN_TEST_CASE(proc_fork, fork_child_has_different_parent_pid);
	RUN_TEST_CASE(proc_fork, fork_child_inherits_fd);
	RUN_TEST_CASE(proc_fork, fork_child_pending_signals_empty);
	RUN_TEST_CASE(proc_fork, fork_child_alarm_cleared);
}


/* ========================================================================= */
/* waitid */
/* ========================================================================= */

#ifndef __phoenix__

TEST_GROUP(proc_waitid);

TEST_SETUP(proc_waitid) {}

TEST_TEAR_DOWN(proc_waitid) {}


TEST(proc_waitid, waitid_wexited_normal)
{
	pid_t childPid;
	siginfo_t info;
	int ret;

	childPid = fork();
	TEST_ASSERT_TRUE(childPid >= 0);

	if (childPid == 0) {
		_exit(7);
	}

	memset(&info, 0, sizeof(info));
	ret = waitid(P_PID, (id_t)childPid, &info, WEXITED);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(SIGCHLD, info.si_signo);
	TEST_ASSERT_EQUAL_INT(childPid, info.si_pid);
	TEST_ASSERT_EQUAL_INT(CLD_EXITED, info.si_code);
	TEST_ASSERT_EQUAL_INT(7, info.si_status);
}


TEST(proc_waitid, waitid_wexited_signal_killed)
{
	pid_t childPid;
	siginfo_t info;
	int ret;

	childPid = fork();
	TEST_ASSERT_TRUE(childPid >= 0);

	if (childPid == 0) {
		/* Kill self */
		raise(SIGKILL);
		_exit(99);
	}

	memset(&info, 0, sizeof(info));
	ret = waitid(P_PID, (id_t)childPid, &info, WEXITED);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(SIGCHLD, info.si_signo);
	TEST_ASSERT_EQUAL_INT(childPid, info.si_pid);
	TEST_ASSERT_EQUAL_INT(CLD_KILLED, info.si_code);
	TEST_ASSERT_EQUAL_INT(SIGKILL, info.si_status);
}


TEST(proc_waitid, waitid_wnohang_no_child_ready)
{
	pid_t childPid;
	siginfo_t info;
	int ret;
	int pipeFds[2];
	int status;

	ret = pipe(pipeFds);
	TEST_ASSERT_EQUAL_INT(0, ret);

	childPid = fork();
	TEST_ASSERT_TRUE(childPid >= 0);

	if (childPid == 0) {
		char c;
		close(pipeFds[1]);
		/* Block until parent signals */
		if (read(pipeFds[0], &c, 1) < 0) {
			_exit(99);
		}
		close(pipeFds[0]);
		_exit(0);
	}

	close(pipeFds[0]);

	/* Child is still running, WNOHANG should return immediately */
	memset(&info, 0, sizeof(info));
	ret = waitid(P_PID, (id_t)childPid, &info, WEXITED | WNOHANG);
	TEST_ASSERT_EQUAL_INT(0, ret);
	/* si_pid and si_signo should be 0 when no status available */
	TEST_ASSERT_EQUAL_INT(0, info.si_pid);
	TEST_ASSERT_EQUAL_INT(0, info.si_signo);

	/* Let child exit */
	if (write(pipeFds[1], "x", 1) < 0) {
		TEST_FAIL_MESSAGE("write to pipe failed");
	}
	close(pipeFds[1]);
	waitpid(childPid, &status, 0);
}


TEST(proc_waitid, waitid_echild_no_children)
{
	siginfo_t info;
	int ret;

	memset(&info, 0, sizeof(info));
	errno = 0;
	ret = waitid(P_ALL, 0, &info, WEXITED | WNOHANG);
	/* If no children exist, should fail with ECHILD */
	if (ret == -1) {
		TEST_ASSERT_EQUAL_INT(ECHILD, errno);
	}
	else {
		/* Some systems return 0 with si_pid=0 if no children */
		TEST_ASSERT_EQUAL_INT(0, ret);
	}
}


TEST(proc_waitid, waitid_einval_no_flags)
{
	siginfo_t info;
	int ret;

	memset(&info, 0, sizeof(info));
	errno = 0;
	ret = waitid(P_ALL, 0, &info, 0);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST(proc_waitid, waitid_p_pgid)
{
	pid_t childPid;
	siginfo_t info;
	int ret;

	childPid = fork();
	TEST_ASSERT_TRUE(childPid >= 0);

	if (childPid == 0) {
		_exit(22);
	}

	memset(&info, 0, sizeof(info));
	ret = waitid(P_PGID, (id_t)getpgrp(), &info, WEXITED);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(SIGCHLD, info.si_signo);
	TEST_ASSERT_EQUAL_INT(childPid, info.si_pid);
	TEST_ASSERT_EQUAL_INT(CLD_EXITED, info.si_code);
	TEST_ASSERT_EQUAL_INT(22, info.si_status);
}


TEST_GROUP_RUNNER(proc_waitid)
{
	RUN_TEST_CASE(proc_waitid, waitid_wexited_normal);
	RUN_TEST_CASE(proc_waitid, waitid_wexited_signal_killed);
	RUN_TEST_CASE(proc_waitid, waitid_wnohang_no_child_ready);
	RUN_TEST_CASE(proc_waitid, waitid_echild_no_children);
	RUN_TEST_CASE(proc_waitid, waitid_einval_no_flags);
	RUN_TEST_CASE(proc_waitid, waitid_p_pgid);
}
#else
TEST_GROUP_UNIMPLEMENTED(proc_waitid, "waitid not implemented")
#endif
