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
 *    - waitpid() (the process-group forms)
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
/* waitpid, process-group forms */
/* ========================================================================= */

TEST_GROUP(proc_waitpid_group);


TEST_SETUP(proc_waitpid_group)
{
}


TEST_TEAR_DOWN(proc_waitpid_group)
{
}


/* A process group id that is very unlikely to be in use */
#define WAITPID_UNUSED_PGID 2000000


/*
 * Forks a child that puts itself in a group of its own and exits at once with
 * 'code'. Returns once the child is a zombie, so that the waitpid() under test
 * has something to reap without having to block. Returns -1 on failure.
 */
static pid_t startExitedChildInOwnGroup(int code)
{
	int ready[2];
	pid_t pid;
	char c;

	if (pipe(ready) < 0) {
		return -1;
	}

	pid = fork();
	if (pid < 0) {
		(void)close(ready[0]);
		(void)close(ready[1]);
		return -1;
	}

	if (pid == 0) {
		(void)close(ready[0]);
		if (setpgid(0, 0) != 0) {
			_exit(120);
		}
		if (write(ready[1], "r", 1) != 1) {
			_exit(121);
		}
		_exit(code);
	}

	(void)close(ready[1]);

	if (read(ready[0], &c, sizeof(c)) != (ssize_t)sizeof(c)) {
		(void)close(ready[0]);
		(void)waitpid(pid, NULL, 0);
		return -1;
	}

	/* The child's copy of the pipe is closed as the kernel tears it down, so
	 * EOF here means it has exited and is waiting to be reaped */
	while (read(ready[0], &c, sizeof(c)) > 0) {
	}
	(void)close(ready[0]);

	return pid;
}


TEST(proc_waitpid_group, waitpid_pgid_zero_is_own_group_only)
{
	/* "If pid is 0, status is requested for any child process whose process
	 * group ID is equal to that of the calling process."
	 *
	 * The only child here is in a group of its own and has already exited, so
	 * it is not eligible: waitpid(0) must report that there is nothing to wait
	 * for rather than reap it. */
	pid_t child;
	int status = 0;

	child = startExitedChildInOwnGroup(11);
	TEST_ASSERT_NOT_EQUAL_INT(-1, (int)child);
	TEST_ASSERT_NOT_EQUAL_INT((int)getpgrp(), (int)child);

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, (int)waitpid(0, &status, WNOHANG));
	TEST_ASSERT_EQUAL_INT(ECHILD, errno);

	/* Still reapable by pid, so the call above really did decline it */
	TEST_ASSERT_EQUAL_INT((int)child, (int)waitpid(child, &status, 0));
	TEST_ASSERT_TRUE(WIFEXITED(status));
	TEST_ASSERT_EQUAL_INT(11, WEXITSTATUS(status));
}


TEST(proc_waitpid_group, waitpid_negative_pgid_reaps_named_group)
{
	/* "If pid is less than (pid_t)-1, status is requested for any child process
	 * whose process group ID is equal to the absolute value of pid." */
	pid_t child;
	int status = 0;

	child = startExitedChildInOwnGroup(12);
	TEST_ASSERT_NOT_EQUAL_INT(-1, (int)child);

	TEST_ASSERT_EQUAL_INT((int)child, (int)waitpid(-child, &status, 0));
	TEST_ASSERT_TRUE(WIFEXITED(status));
	TEST_ASSERT_EQUAL_INT(12, WEXITSTATUS(status));
}


TEST(proc_waitpid_group, waitpid_echild_unused_group)
{
	/* "[ECHILD] The process specified by pid does not exist or is not a child
	 * of the calling process" - a group none of our children is in. */
	pid_t child;
	int status = 0;

	child = startExitedChildInOwnGroup(13);
	TEST_ASSERT_NOT_EQUAL_INT(-1, (int)child);
	TEST_ASSERT_NOT_EQUAL_INT(WAITPID_UNUSED_PGID, (int)child);

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, (int)waitpid(-WAITPID_UNUSED_PGID, &status, WNOHANG));
	TEST_ASSERT_EQUAL_INT(ECHILD, errno);

	TEST_ASSERT_EQUAL_INT((int)child, (int)waitpid(child, &status, 0));
}


TEST_GROUP_RUNNER(proc_waitpid_group)
{
	RUN_TEST_CASE(proc_waitpid_group, waitpid_pgid_zero_is_own_group_only);
	RUN_TEST_CASE(proc_waitpid_group, waitpid_negative_pgid_reaps_named_group);
	RUN_TEST_CASE(proc_waitpid_group, waitpid_echild_unused_group);
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
