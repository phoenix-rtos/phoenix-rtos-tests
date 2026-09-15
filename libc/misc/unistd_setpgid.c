/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <unistd.h>
 * TESTED:
 *    - setpgid()
 *    - setsid()
 *
 * These tests focus on the permission and validity rules of setpgid() and
 * setsid() rather than on the happy path, which is covered by
 * unistd_pgrp.c and libc/proc/proc_basic.c. A couple of cases check what must
 * *not* change a process group or session - notably exec().
 *
 * NOTE: a successful setpgid()/setsid() on the calling process would move the
 * test runner out of the process group the shell put it in, which interferes
 * with job control. Every case that is expected to *succeed* therefore either
 * operates on a child or runs inside a forked child. Cases that are expected
 * to fail change nothing and may run in the test process itself.
 *
 * Copyright 2026 Phoenix Systems
 * Author: Adam Greloch
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#include "unity_fixture.h"


/* A pid that is very unlikely to be in use. Both Phoenix and Linux allow pids
 * this large, so setpgid() shall report EPERM (an unusable group) rather than
 * EINVAL (an unrepresentable value) for it. */
#define UNUSED_PID 2000000

/* Multicall shell applet used as a stand-in for "some other program". It only
 * needs to print its argument, so that the parent can tell a completed exec
 * apart from an execl() that failed and fell through to _exit(). */
#define EXEC_HELPER "/bin/echo"
#define EXEC_MARKER "pgidmarker"


/* Several cases below expect EPERM for a reason that only applies to a process
 * that is not a session leader; a session leader is refused earlier and by a
 * different rule, which would let them pass without testing anything. The test
 * runner is started by a shell, so it never is one - skip rather than report a
 * defect if it is, the same way the other preconditions here do. */
#define REQUIRE_NOT_SESSION_LEADER() \
	do { \
		if (getpid() == getsid(0)) { \
			TEST_IGNORE_MESSAGE("the test runner is a session leader, so this case cannot be reached"); \
		} \
	} while (0)


/*
 * A child parked in read(), so that the parent can inspect and modify its
 * process group while it is demonstrably alive.
 */
typedef struct {
	pid_t pid;
	int ready[2]; /* child -> parent: one byte, written once the child parked */
	int quit[2];  /* parent -> child: closed to release the child */
} parked_t;


static void parked_closePipe(int fds[2])
{
	if (fds[0] >= 0) {
		(void)close(fds[0]);
		fds[0] = -1;
	}
	if (fds[1] >= 0) {
		(void)close(fds[1]);
		fds[1] = -1;
	}
}


/* Runs in the child; never returns */
static void parked_body(int readyFd, int quitFd, int newSession)
{
	char c = 'r';
	char buf[1];

	if ((newSession != 0) && (setsid() == (pid_t)-1)) {
		_exit(1);
	}

	if (write(readyFd, &c, sizeof(c)) != (ssize_t)sizeof(c)) {
		_exit(2);
	}

	/* Released when the parent closes its end of the quit pipe */
	while (read(quitFd, buf, sizeof(buf)) > 0) {
	}

	_exit(0);
}


/* Starts a parked child and returns only once it is running. When newSession
 * is set the child makes itself a session leader before parking. */
static int parked_start(parked_t *p, int newSession)
{
	char c;

	p->pid = -1;
	p->ready[0] = p->ready[1] = -1;
	p->quit[0] = p->quit[1] = -1;

	if (pipe(p->ready) < 0) {
		return -1;
	}

	if (pipe(p->quit) < 0) {
		parked_closePipe(p->ready);
		return -1;
	}

	p->pid = fork();
	if (p->pid < 0) {
		parked_closePipe(p->ready);
		parked_closePipe(p->quit);
		return -1;
	}

	if (p->pid == 0) {
		(void)close(p->ready[0]);
		(void)close(p->quit[1]);
		parked_body(p->ready[1], p->quit[0], newSession);
		/* Not reached */
	}

	(void)close(p->ready[1]);
	p->ready[1] = -1;
	(void)close(p->quit[0]);
	p->quit[0] = -1;

	if (read(p->ready[0], &c, sizeof(c)) != (ssize_t)sizeof(c)) {
		/* The child died before parking */
		parked_closePipe(p->ready);
		parked_closePipe(p->quit);
		(void)waitpid(p->pid, NULL, 0);
		p->pid = -1;
		return -1;
	}

	return 0;
}


/* Releases a parked child and reaps it. Returns its exit status, or -1. */
static int parked_stop(parked_t *p)
{
	int status;
	int ret = -1;

	if (p->pid < 0) {
		return -1;
	}

	/* Closing the last write end makes the child's read() return 0 */
	parked_closePipe(p->quit);
	parked_closePipe(p->ready);

	if (waitpid(p->pid, &status, 0) == p->pid) {
		ret = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
	}
	p->pid = -1;

	return ret;
}


/*
 * Tests: setpgid
 */

TEST_GROUP(unistd_setpgid);


TEST_SETUP(unistd_setpgid)
{
}


TEST_TEAR_DOWN(unistd_setpgid)
{
}


TEST(unistd_setpgid, setpgid_einval_negative_pgid)
{
	/* "[EINVAL] The value of the pgid argument is less than 0" */
	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, setpgid(0, -1));
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST(unistd_setpgid, setpgid_esrch_unused_pid)
{
	/* "[ESRCH] The value of the pid argument does not match the process ID of
	 * the calling process or of a child process of the calling process." */
	if (getpgid(UNUSED_PID) != (pid_t)-1) {
		TEST_IGNORE_MESSAGE("the pid used as an unused-pid probe exists on this system");
	}

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, setpgid(UNUSED_PID, UNUSED_PID));
	TEST_ASSERT_EQUAL_INT(ESRCH, errno);
}


TEST(unistd_setpgid, setpgid_esrch_not_own_child)
{
	/* A live process that is neither the caller nor one of its children.
	 * The caller's parent is the obvious candidate. */
	pid_t ppid = getppid();

	if (ppid <= 0) {
		TEST_IGNORE_MESSAGE("no parent process to test against");
	}
	TEST_ASSERT_NOT_EQUAL_INT(getpid(), (int)ppid);

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, setpgid(ppid, ppid));
	TEST_ASSERT_EQUAL_INT(ESRCH, errno);
}


TEST(unistd_setpgid, setpgid_eperm_unused_pgid)
{
	/* "[EPERM] The value of the pgid argument is valid but does not match the
	 * process ID of the process indicated by the pid argument and there is no
	 * process with a process group ID that matches the value of the pgid
	 * argument in the same session as the calling process." */
	if (getpgid(UNUSED_PID) != (pid_t)-1) {
		TEST_IGNORE_MESSAGE("the pid used as an unused-pid probe exists on this system");
	}
	REQUIRE_NOT_SESSION_LEADER();

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, setpgid(0, UNUSED_PID));
	TEST_ASSERT_EQUAL_INT(EPERM, errno);
}


TEST(unistd_setpgid, setpgid_eperm_pgid_in_other_session)
{
	/* A group that exists, but not in the caller's session, may not be joined */
	parked_t leader;
	int ret, err;

	REQUIRE_NOT_SESSION_LEADER();

	TEST_ASSERT_EQUAL_INT(0, parked_start(&leader, 1));

	/* The child is a session leader, hence the leader of group == its own pid */
	TEST_ASSERT_EQUAL_INT((int)leader.pid, (int)getpgid(leader.pid));
	TEST_ASSERT_EQUAL_INT((int)leader.pid, (int)getsid(leader.pid));
	TEST_ASSERT_NOT_EQUAL_INT((int)getsid(0), (int)getsid(leader.pid));

	errno = 0;
	ret = setpgid(0, leader.pid);
	err = errno;

	TEST_ASSERT_EQUAL_INT(0, parked_stop(&leader));

	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EPERM, err);
}


TEST(unistd_setpgid, setpgid_eperm_child_is_session_leader)
{
	/* "[EPERM] The process indicated by the pid argument is a session leader."
	 * Such a child is also, by construction, no longer in the caller's
	 * session, which POSIX lists as a separate EPERM condition. */
	parked_t leader;
	int ret, err;

	TEST_ASSERT_EQUAL_INT(0, parked_start(&leader, 1));

	errno = 0;
	ret = setpgid(leader.pid, leader.pid);
	err = errno;

	TEST_ASSERT_EQUAL_INT(0, parked_stop(&leader));

	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EPERM, err);
}


TEST(unistd_setpgid, setpgid_eperm_self_is_session_leader)
{
	/* Same rule seen from the session leader itself */
	pid_t pid;
	int status;

	pid = fork();
	TEST_ASSERT_NOT_EQUAL_INT(-1, (int)pid);

	if (pid == 0) {
		if (setsid() == (pid_t)-1) {
			_exit(1);
		}

		errno = 0;
		if (setpgid(0, 0) != -1) {
			_exit(2);
		}
		if (errno != EPERM) {
			_exit(3);
		}

		/* The failed call must not have changed anything */
		if (getpgrp() != getpid()) {
			_exit(4);
		}

		_exit(0);
	}

	TEST_ASSERT_EQUAL_INT((int)pid, (int)waitpid(pid, &status, 0));
	TEST_ASSERT_TRUE(WIFEXITED(status));
	TEST_ASSERT_EQUAL_INT(0, WEXITSTATUS(status));
}


TEST(unistd_setpgid, setpgid_eacces_child_execd)
{
	/* "[EACCES] The value of the pid argument matches the process ID of a
	 * child process of the calling process and the child process has
	 * successfully executed one of the exec functions." */
	int p[2];
	pid_t child;
	char buf[64];
	size_t got = 0;
	ssize_t n;
	int ret, err, status;

	if (access(EXEC_HELPER, X_OK) != 0) {
		TEST_IGNORE_MESSAGE(EXEC_HELPER " is not available");
	}

	TEST_ASSERT_EQUAL_INT(0, pipe(p));

	child = fork();
	TEST_ASSERT_NOT_EQUAL_INT(-1, (int)child);

	if (child == 0) {
		(void)close(p[0]);
		if (dup2(p[1], STDOUT_FILENO) < 0) {
			_exit(126);
		}
		(void)close(p[1]);
		(void)execl(EXEC_HELPER, "echo", EXEC_MARKER, (char *)NULL);
		_exit(127);
	}

	(void)close(p[1]);

	/* Read to EOF. The new image has run and closed its stdout by then, so the
	 * child has provably made it through execl(). It may already have exited,
	 * but it has not been reaped, so it is still a child process. */
	for (;;) {
		n = read(p[0], buf + got, sizeof(buf) - 1U - got);
		if (n <= 0) {
			break;
		}
		got += (size_t)n;
		if (got >= (sizeof(buf) - 1U)) {
			break;
		}
	}
	buf[got] = '\0';
	(void)close(p[0]);

	errno = 0;
	ret = setpgid(child, child);
	err = errno;

	TEST_ASSERT_EQUAL_INT((int)child, (int)waitpid(child, &status, 0));

	/* Distinguish a completed exec from an execl() that failed */
	TEST_ASSERT_NOT_NULL_MESSAGE(strstr(buf, EXEC_MARKER), "child never reached the new image");

	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EACCES, err);
}


TEST(unistd_setpgid, setpgid_creates_new_group_for_child)
{
	/* The shell's use case: the parent moves a fresh child into a group of its
	 * own. Allowed because pgid == pid creates a new group. */
	parked_t child;
	pid_t inherited;

	TEST_ASSERT_EQUAL_INT(0, parked_start(&child, 0));

	inherited = getpgid(child.pid);
	TEST_ASSERT_EQUAL_INT((int)getpgrp(), (int)inherited);

	TEST_ASSERT_EQUAL_INT(0, setpgid(child.pid, child.pid));
	TEST_ASSERT_EQUAL_INT((int)child.pid, (int)getpgid(child.pid));

	/* The session must be untouched by a group change */
	TEST_ASSERT_EQUAL_INT((int)getsid(0), (int)getsid(child.pid));

	TEST_ASSERT_EQUAL_INT(0, parked_stop(&child));
}


TEST(unistd_setpgid, setpgid_joins_existing_group)
{
	/* Second and later members of a pipeline join the group of the first */
	parked_t first, second;

	TEST_ASSERT_EQUAL_INT(0, parked_start(&first, 0));
	TEST_ASSERT_EQUAL_INT(0, setpgid(first.pid, first.pid));

	TEST_ASSERT_EQUAL_INT(0, parked_start(&second, 0));
	TEST_ASSERT_EQUAL_INT(0, setpgid(second.pid, first.pid));

	TEST_ASSERT_EQUAL_INT((int)first.pid, (int)getpgid(second.pid));
	TEST_ASSERT_NOT_EQUAL_INT((int)second.pid, (int)getpgid(second.pid));
	TEST_ASSERT_EQUAL_INT((int)getsid(0), (int)getsid(second.pid));

	TEST_ASSERT_EQUAL_INT(0, parked_stop(&second));
	TEST_ASSERT_EQUAL_INT(0, parked_stop(&first));
}


TEST(unistd_setpgid, setpgid_idempotent_on_own_group)
{
	/* Setting the group a process is already in is not an error */
	parked_t child;

	TEST_ASSERT_EQUAL_INT(0, parked_start(&child, 0));

	TEST_ASSERT_EQUAL_INT(0, setpgid(child.pid, child.pid));
	TEST_ASSERT_EQUAL_INT(0, setpgid(child.pid, child.pid));
	TEST_ASSERT_EQUAL_INT((int)child.pid, (int)getpgid(child.pid));

	TEST_ASSERT_EQUAL_INT(0, parked_stop(&child));
}


TEST(unistd_setpgid, setpgid_self_keeps_session)
{
	/* "If the process indicated by the pid argument is the calling process,
	 * setpgid() shall change the process group ID" - and nothing else. */
	pid_t pid;
	int status;

	pid = fork();
	TEST_ASSERT_NOT_EQUAL_INT(-1, (int)pid);

	if (pid == 0) {
		pid_t sidBefore = getsid(0);

		if (setpgid(0, 0) != 0) {
			_exit(1);
		}
		if (getpgrp() != getpid()) {
			_exit(2);
		}
		if (getsid(0) != sidBefore) {
			_exit(3);
		}

		_exit(0);
	}

	TEST_ASSERT_EQUAL_INT((int)pid, (int)waitpid(pid, &status, 0));
	TEST_ASSERT_TRUE(WIFEXITED(status));
	TEST_ASSERT_EQUAL_INT(0, WEXITSTATUS(status));
}


TEST(unistd_setpgid, setpgid_child_group_not_inherited_backwards)
{
	/* Moving a child must not disturb the caller's own group */
	parked_t child;
	pid_t before;

	before = getpgrp();

	TEST_ASSERT_EQUAL_INT(0, parked_start(&child, 0));
	TEST_ASSERT_EQUAL_INT(0, setpgid(child.pid, child.pid));

	TEST_ASSERT_EQUAL_INT((int)before, (int)getpgrp());

	TEST_ASSERT_EQUAL_INT(0, parked_stop(&child));
}


TEST(unistd_setpgid, setpgid_eperm_child_left_in_old_session)
{
	/* "[EPERM] The value of the pid argument matches the process ID of a child
	 * process of the calling process and the child process is not in the same
	 * session as the calling process."
	 *
	 * The obvious way to reach this - a child that called setsid() - also makes
	 * that child a session leader, which POSIX refuses under a different rule,
	 * so it never exercises this one. Here the *parent* leaves instead: P forks
	 * Q, then P calls setsid(). Q stays behind in the old session, is still P's
	 * child, and is not a session leader.
	 *
	 * Runs in a forked child because the test runner may not leave its own
	 * session. Codes: 1 fork, 2 setsid, 3 Q is a session leader after all,
	 * 4 Q is in our new session after all, 5 setpgid succeeded, 6 wrong errno,
	 * 7 the refused call moved Q anyway, 8 Q misbehaved. */
	pid_t pid;
	int status;

	pid = fork();
	TEST_ASSERT_NOT_EQUAL_INT(-1, (int)pid);

	if (pid == 0) {
		parked_t q;
		pid_t qPgidBefore;
		int ret = 0;

		if (parked_start(&q, 0) != 0) {
			_exit(1);
		}

		qPgidBefore = getpgid(q.pid);

		if (setsid() == (pid_t)-1) {
			(void)parked_stop(&q);
			_exit(2);
		}

		if (getsid(q.pid) == q.pid) {
			ret = 3;
		}
		else if (getsid(q.pid) == getsid(0)) {
			ret = 4;
		}
		else {
			errno = 0;
			if (setpgid(q.pid, q.pid) != -1) {
				ret = 5;
			}
			else if (errno != EPERM) {
				ret = 6;
			}
			else if (getpgid(q.pid) != qPgidBefore) {
				ret = 7;
			}
			else {
				/* Nothing to do */
			}
		}

		if (parked_stop(&q) != 0) {
			ret = (ret != 0) ? ret : 8;
		}

		_exit(ret);
	}

	TEST_ASSERT_EQUAL_INT((int)pid, (int)waitpid(pid, &status, 0));
	TEST_ASSERT_TRUE(WIFEXITED(status));
	TEST_ASSERT_EQUAL_INT(0, WEXITSTATUS(status));
}


TEST(unistd_setpgid, exec_keeps_group_and_session)
{
	/* "The new process image shall inherit ... process group ID" and session
	 * membership across exec(). A shell puts a job into its group *before*
	 * handing it over to the new program, and the group has to survive the
	 * handover - otherwise every job would fall back into the shell's own
	 * group the moment it started running.
	 *
	 * Both are checked from the parent, so no cooperation from the new image is
	 * needed beyond proving that it ran. */
	int go[2], out[2];
	pid_t child;
	pid_t sid;
	char buf[64];
	size_t got = 0;
	ssize_t n;
	int status;

	if (access(EXEC_HELPER, X_OK) != 0) {
		TEST_IGNORE_MESSAGE(EXEC_HELPER " is not available");
	}

	sid = getsid(0);

	TEST_ASSERT_EQUAL_INT(0, pipe(go));
	TEST_ASSERT_EQUAL_INT(0, pipe(out));

	child = fork();
	TEST_ASSERT_NOT_EQUAL_INT(-1, (int)child);

	if (child == 0) {
		char c;

		(void)close(go[1]);
		(void)close(out[0]);
		if (dup2(out[1], STDOUT_FILENO) < 0) {
			_exit(126);
		}
		(void)close(out[1]);

		/* Wait to be placed in a group of our own, then replace the image */
		if (read(go[0], &c, sizeof(c)) != (ssize_t)sizeof(c)) {
			_exit(125);
		}
		(void)execl(EXEC_HELPER, "echo", EXEC_MARKER, (char *)NULL);
		_exit(127);
	}

	(void)close(go[0]);
	(void)close(out[1]);

	/* The move has to happen before the exec, or it would fail with EACCES */
	TEST_ASSERT_EQUAL_INT(0, setpgid(child, child));
	TEST_ASSERT_EQUAL_INT((int)child, (int)getpgid(child));

	TEST_ASSERT_EQUAL_INT(1, (int)write(go[1], "g", 1));
	(void)close(go[1]);

	/* Read to EOF: the new image has run and closed its stdout by then */
	for (;;) {
		n = read(out[0], buf + got, sizeof(buf) - 1U - got);
		if (n <= 0) {
			break;
		}
		got += (size_t)n;
		if (got >= (sizeof(buf) - 1U)) {
			break;
		}
	}
	buf[got] = '\0';
	(void)close(out[0]);

	TEST_ASSERT_NOT_NULL_MESSAGE(strstr(buf, EXEC_MARKER), "child never reached the new image");

	/* Still unreaped, so it still has a group and a session to report */
	TEST_ASSERT_EQUAL_INT_MESSAGE((int)child, (int)getpgid(child), "exec() changed the process group");
	TEST_ASSERT_EQUAL_INT_MESSAGE((int)sid, (int)getsid(child), "exec() changed the session");

	TEST_ASSERT_EQUAL_INT((int)child, (int)waitpid(child, &status, 0));
}


TEST_GROUP_RUNNER(unistd_setpgid)
{
	RUN_TEST_CASE(unistd_setpgid, setpgid_einval_negative_pgid);
	RUN_TEST_CASE(unistd_setpgid, setpgid_esrch_unused_pid);
	RUN_TEST_CASE(unistd_setpgid, setpgid_esrch_not_own_child);
	RUN_TEST_CASE(unistd_setpgid, setpgid_eperm_unused_pgid);
	RUN_TEST_CASE(unistd_setpgid, setpgid_eperm_pgid_in_other_session);
	RUN_TEST_CASE(unistd_setpgid, setpgid_eperm_child_is_session_leader);
	RUN_TEST_CASE(unistd_setpgid, setpgid_eperm_self_is_session_leader);
	RUN_TEST_CASE(unistd_setpgid, setpgid_eacces_child_execd);
	RUN_TEST_CASE(unistd_setpgid, setpgid_creates_new_group_for_child);
	RUN_TEST_CASE(unistd_setpgid, setpgid_joins_existing_group);
	RUN_TEST_CASE(unistd_setpgid, setpgid_idempotent_on_own_group);
	RUN_TEST_CASE(unistd_setpgid, setpgid_self_keeps_session);
	RUN_TEST_CASE(unistd_setpgid, setpgid_child_group_not_inherited_backwards);
	RUN_TEST_CASE(unistd_setpgid, setpgid_eperm_child_left_in_old_session);
	RUN_TEST_CASE(unistd_setpgid, exec_keeps_group_and_session);
}


/*
 * Tests: setsid
 */

TEST_GROUP(unistd_setsid);


TEST_SETUP(unistd_setsid)
{
}


TEST_TEAR_DOWN(unistd_setsid)
{
}


TEST(unistd_setsid, setsid_eperm_already_group_leader)
{
	/* "[EPERM] The calling process is already a process group leader" - reached
	 * here through setpgid() rather than through a previous setsid(). */
	pid_t pid;
	int status;

	pid = fork();
	TEST_ASSERT_NOT_EQUAL_INT(-1, (int)pid);

	if (pid == 0) {
		if (setpgid(0, 0) != 0) {
			_exit(1);
		}
		if (getpgrp() != getpid()) {
			_exit(2);
		}

		errno = 0;
		if (setsid() != (pid_t)-1) {
			_exit(3);
		}
		if (errno != EPERM) {
			_exit(4);
		}

		_exit(0);
	}

	TEST_ASSERT_EQUAL_INT((int)pid, (int)waitpid(pid, &status, 0));
	TEST_ASSERT_TRUE(WIFEXITED(status));
	TEST_ASSERT_EQUAL_INT(0, WEXITSTATUS(status));
}


TEST(unistd_setsid, setsid_eperm_pgid_of_other_process_matches_pid)
{
	/* "[EPERM] ... or the process group ID of a process other than the calling
	 * process matches the process ID of the calling process."
	 *
	 * Reaching this state takes three steps, done in a child P:
	 *   1. P becomes the leader of group P,
	 *   2. P forks Q, which inherits group P,
	 *   3. P rejoins the group it started in, leaving Q behind in group P.
	 * P is now not a group leader, yet group P still has a member, so setsid()
	 * must refuse: the new session would otherwise swallow Q's group. */
	pid_t pid;
	int status;

	pid = fork();
	TEST_ASSERT_NOT_EQUAL_INT(-1, (int)pid);

	if (pid == 0) {
		parked_t q;
		pid_t origPgid = getpgrp();

		if (origPgid == getpid()) {
			/* Cannot leave a group we lead without another group to join */
			_exit(10);
		}

		if (setpgid(0, 0) != 0) {
			_exit(1);
		}

		if (parked_start(&q, 0) != 0) {
			_exit(2);
		}

		if (getpgid(q.pid) != getpid()) {
			(void)parked_stop(&q);
			_exit(3);
		}

		/* Rejoin the original group; Q stays behind in group P */
		if (setpgid(0, origPgid) != 0) {
			(void)parked_stop(&q);
			_exit(4);
		}

		if (getpgrp() == getpid()) {
			(void)parked_stop(&q);
			_exit(5);
		}

		errno = 0;
		if (setsid() != (pid_t)-1) {
			(void)parked_stop(&q);
			_exit(6);
		}
		if (errno != EPERM) {
			(void)parked_stop(&q);
			_exit(7);
		}

		/* The refused call must not have changed our group or session */
		if (getpgrp() != origPgid) {
			(void)parked_stop(&q);
			_exit(8);
		}

		if (parked_stop(&q) != 0) {
			_exit(9);
		}

		_exit(0);
	}

	TEST_ASSERT_EQUAL_INT((int)pid, (int)waitpid(pid, &status, 0));
	TEST_ASSERT_TRUE(WIFEXITED(status));
	TEST_ASSERT_EQUAL_INT(0, WEXITSTATUS(status));
}


TEST(unistd_setsid, setsid_new_session_visible_to_parent)
{
	/* The new session and group must be observable from outside the child,
	 * not just through the child's own getsid()/getpgrp(). */
	parked_t child;
	pid_t parentSid = getsid(0);

	TEST_ASSERT_EQUAL_INT(0, parked_start(&child, 1));

	TEST_ASSERT_EQUAL_INT((int)child.pid, (int)getsid(child.pid));
	TEST_ASSERT_EQUAL_INT((int)child.pid, (int)getpgid(child.pid));
	TEST_ASSERT_NOT_EQUAL_INT((int)parentSid, (int)getsid(child.pid));

	/* The parent's own session and group are unaffected */
	TEST_ASSERT_EQUAL_INT((int)parentSid, (int)getsid(0));

	TEST_ASSERT_EQUAL_INT(0, parked_stop(&child));
}


TEST(unistd_setsid, setsid_new_session_inherited_by_grandchild)
{
	/* A process forked after setsid() joins the new session, not the old one */
	pid_t pid;
	int status;
	pid_t oldSid = getsid(0);

	pid = fork();
	TEST_ASSERT_NOT_EQUAL_INT(-1, (int)pid);

	if (pid == 0) {
		pid_t newSid;
		pid_t grandchild;
		int gstatus;

		if (setsid() == (pid_t)-1) {
			_exit(1);
		}

		newSid = getsid(0);
		if (newSid != getpid()) {
			_exit(2);
		}

		grandchild = fork();
		if (grandchild < 0) {
			_exit(3);
		}

		if (grandchild == 0) {
			if (getsid(0) != newSid) {
				_exit(1);
			}
			/* Not a group leader, so it can start a session of its own */
			if (getpgrp() == getpid()) {
				_exit(2);
			}
			_exit(0);
		}

		if (waitpid(grandchild, &gstatus, 0) != grandchild) {
			_exit(4);
		}
		if (!WIFEXITED(gstatus) || (WEXITSTATUS(gstatus) != 0)) {
			_exit(5);
		}

		_exit(0);
	}

	TEST_ASSERT_EQUAL_INT((int)pid, (int)waitpid(pid, &status, 0));
	TEST_ASSERT_TRUE(WIFEXITED(status));
	TEST_ASSERT_EQUAL_INT(0, WEXITSTATUS(status));

	/* The old session is still ours */
	TEST_ASSERT_EQUAL_INT((int)oldSid, (int)getsid(0));
}


TEST_GROUP_RUNNER(unistd_setsid)
{
	RUN_TEST_CASE(unistd_setsid, setsid_eperm_already_group_leader);
	RUN_TEST_CASE(unistd_setsid, setsid_eperm_pgid_of_other_process_matches_pid);
	RUN_TEST_CASE(unistd_setsid, setsid_new_session_visible_to_parent);
	RUN_TEST_CASE(unistd_setsid, setsid_new_session_inherited_by_grandchild);
}
