/*
 * Phoenix-RTOS
 *
 * libc/posixsrv
 *
 * Controlling-terminal regression tests for process groups and sessions
 * (RTOS-1449). Each case documents the POSIX/Linux behaviour it expects and
 * the way the implementation used to get it wrong.
 *
 * Copyright 2026 Phoenix Systems
 * Author: Ziemowit Leszczynski
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* glibc (host-generic-pc) hides unlockpt()/ptsname_r() without it */
#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include "unity_fixture.h"


#define PTS_NAME_SZ 32

/* Child exit codes - anything but 0 and EXIT_CLAIMED means the setup failed */
#define EXIT_SETSID  11
#define EXIT_OPEN    12
#define EXIT_SCTTY   13
#define EXIT_OPEN2   14
#define EXIT_SCTTY2  15
#define EXIT_CLAIMED 20


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"


typedef struct {
	int master;
	char slave[PTS_NAME_SZ];
} pty_t;


static pty_t pty_a, pty_b;
static int chan_up[2];   /* child -> parent */
static int chan_down[2]; /* parent -> child */


static int pty_open(pty_t *pty)
{
	pty->master = -1;

	pty->master = open("/dev/ptmx", O_RDWR | O_NOCTTY);
	if (pty->master < 0) {
		return -1;
	}

	if (unlockpt(pty->master) < 0) {
		close(pty->master);
		pty->master = -1;
		return -1;
	}

	if (ptsname_r(pty->master, pty->slave, sizeof(pty->slave)) < 0) {
		close(pty->master);
		pty->master = -1;
		return -1;
	}

	return 0;
}


static void pty_close(pty_t *pty)
{
	if (pty->master >= 0) {
		close(pty->master);
		pty->master = -1;
	}
}


static void chan_close(int fd[2])
{
	if (fd[0] >= 0) {
		close(fd[0]);
		fd[0] = -1;
	}
	if (fd[1] >= 0) {
		close(fd[1]);
		fd[1] = -1;
	}
}


static void chan_post(int fd)
{
	char c = 'x';
	(void)write(fd, &c, 1);
}


static int chan_wait(int fd)
{
	char c;
	ssize_t n;

	do {
		n = read(fd, &c, 1);
	} while ((n < 0) && (errno == EINTR));

	return (n == 1) ? 0 : -1;
}


/*
 * The test binary is started by psh, which puts it in a process group of its
 * own but leaves it in psh's session, so setsid() here would fail with EPERM.
 * Every case that needs a session leader forks one.
 */
/* Reaps a child that died before signalling, so the failure names its exit code */
static void assert_child_ready(int fd, pid_t child)
{
	int status = 0;

	if (chan_wait(fd) == 0) {
		return;
	}

	(void)waitpid(child, &status, 0);
	TEST_FAIL_MESSAGE(WIFEXITED(status) ?
					"child exited during setup (11 setsid, 12/14 open, 13 TIOCSCTTY)" :
					"child died during setup");
}


static pid_t fork_or_skip(void)
{
	pid_t pid = fork();

	if ((pid < 0) && (errno == ENOSYS)) {
		TEST_IGNORE_MESSAGE("fork() unavailable on this target");
	}

	return pid;
}


TEST_GROUP(pty_ctty);


TEST_SETUP(pty_ctty)
{
	pty_a.master = -1;
	pty_b.master = -1;
	chan_up[0] = chan_up[1] = -1;
	chan_down[0] = chan_down[1] = -1;
}


TEST_TEAR_DOWN(pty_ctty)
{
	pty_close(&pty_a);
	pty_close(&pty_b);
	chan_close(chan_up);
	chan_close(chan_down);
}


/*
 * P2: a program that holds the pty master - a terminal emulator, script(1),
 * expect(1) - must be able to ask which process group is in the foreground of
 * the pty it drives, even though the pty is the controlling terminal of the
 * child's session, not of its own. Linux allows this. libtty allowed it before
 * RTOS-1449, because TIOCGPGRP/TIOCGSID were unchecked reads.
 *
 * It regressed once libtty started checking sessions, because posixsrv forwarded
 * every master-side ioctl to _libtty_ioctl() with the master's pid, which is in
 * a foreign session by construction. ptm_devctl_op() now answers both getters
 * from the shared tty state itself.
 */
TEST(pty_ctty, master_side_getters_from_another_session)
{
	pid_t child, fg, sid;
	int status = 0;

	TEST_ASSERT_EQUAL_INT(0, pty_open(&pty_a));
	TEST_ASSERT_EQUAL_INT(0, pipe(chan_up));
	TEST_ASSERT_EQUAL_INT(0, pipe(chan_down));

	child = fork_or_skip();
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, child);

	if (child == 0) {
		int slave;

		/* the parent keeps the master and stays in psh's session */
		close(pty_a.master);
		close(chan_up[0]);
		close(chan_down[1]);

		if (setsid() == (pid_t)-1) {
			_exit(EXIT_SETSID);
		}

		slave = open(pty_a.slave, O_RDWR);
		if (slave < 0) {
			_exit(EXIT_OPEN);
		}

		/* claim the pty: tty->sid = tty->pgrp = our pid */
		if (ioctl(slave, TIOCSCTTY, 0) < 0) {
			_exit(EXIT_SCTTY);
		}

		chan_post(chan_up[1]);
		(void)chan_wait(chan_down[0]);
		_exit(EXIT_SUCCESS);
	}

	close(chan_up[1]);
	chan_up[1] = -1;
	close(chan_down[0]);
	chan_down[0] = -1;

	/* wait until the child owns the pty, and keep it alive while we look */
	assert_child_ready(chan_up[0], child);

	errno = 0;
	fg = tcgetpgrp(pty_a.master);
	sid = tcgetsid(pty_a.master);

	chan_post(chan_down[1]);
	(void)waitpid(child, &status, 0);

	TEST_ASSERT_TRUE(WIFEXITED(status));
	TEST_ASSERT_EQUAL_INT_MESSAGE(EXIT_SUCCESS, WEXITSTATUS(status), "child failed to claim the pty");

	TEST_ASSERT_EQUAL_INT_MESSAGE(child, fg, "tcgetpgrp() on the pty master");
	TEST_ASSERT_EQUAL_INT_MESSAGE(child, sid, "tcgetsid() on the pty master");
}


/*
 * P3: a session has at most one controlling terminal (XBD 3.100, and Linux
 * returns EPERM from TIOCSCTTY when current->signal->tty is already set).
 * Terminal drivers cannot see each other's state, so the kernel records whether
 * a session holds one and TIOCSCTTY claims it through sessionCtty(); before
 * that every unclaimed terminal libtty was pointed at was handed over.
 */
TEST(pty_ctty, second_controlling_terminal_is_rejected)
{
	pid_t child;
	int status = 0;

	TEST_ASSERT_EQUAL_INT(0, pty_open(&pty_a));
	TEST_ASSERT_EQUAL_INT(0, pty_open(&pty_b));

	child = fork_or_skip();
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, child);

	if (child == 0) {
		int fa, fb;

		close(pty_a.master);
		close(pty_b.master);

		if (setsid() == (pid_t)-1) {
			_exit(EXIT_SETSID);
		}

		fa = open(pty_a.slave, O_RDWR);
		if (fa < 0) {
			_exit(EXIT_OPEN);
		}
		if (ioctl(fa, TIOCSCTTY, 0) < 0) {
			_exit(EXIT_SCTTY);
		}

		fb = open(pty_b.slave, O_RDWR);
		if (fb < 0) {
			_exit(EXIT_OPEN2);
		}

		/* we already have one - this must fail */
		_exit((ioctl(fb, TIOCSCTTY, 0) < 0) ? EXIT_SUCCESS : EXIT_CLAIMED);
	}

	TEST_ASSERT_EQUAL_INT(child, waitpid(child, &status, 0));
	TEST_ASSERT_TRUE(WIFEXITED(status));

	TEST_ASSERT_EQUAL_INT_MESSAGE(EXIT_SUCCESS, WEXITSTATUS(status),
			"TIOCSCTTY on a second terminal should fail with EPERM");
}


/*
 * P3, the consequence: while the second claim above still succeeded, pty B was
 * recorded as belonging to a session that never used it. libtty only releases a
 * terminal once its recorded session leader is gone, so for as long as that
 * leader lived - a login shell lives for the uptime of the system - no other
 * session could acquire pty B. Before RTOS-1449 nothing could be locked out this
 * way, because TIOCSCTTY kept no ownership at all.
 */
TEST(pty_ctty, terminal_claimed_by_accident_stays_locked)
{
	pid_t owner, other;
	int status = 0;

	TEST_ASSERT_EQUAL_INT(0, pty_open(&pty_a));
	TEST_ASSERT_EQUAL_INT(0, pty_open(&pty_b));
	TEST_ASSERT_EQUAL_INT(0, pipe(chan_up));
	TEST_ASSERT_EQUAL_INT(0, pipe(chan_down));

	owner = fork_or_skip();
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, owner);

	if (owner == 0) {
		int fa, fb;

		close(pty_a.master);
		close(pty_b.master);
		close(chan_up[0]);
		close(chan_down[1]);

		if (setsid() == (pid_t)-1) {
			_exit(EXIT_SETSID);
		}

		fa = open(pty_a.slave, O_RDWR);
		fb = open(pty_b.slave, O_RDWR);
		if ((fa < 0) || (fb < 0)) {
			_exit(EXIT_OPEN);
		}

		(void)ioctl(fa, TIOCSCTTY, 0); /* intended */
		(void)ioctl(fb, TIOCSCTTY, 0); /* accidental - should have failed */

		chan_post(chan_up[1]);
		(void)chan_wait(chan_down[0]);
		_exit(EXIT_SUCCESS);
	}

	close(chan_up[1]);
	chan_up[1] = -1;
	close(chan_down[0]);
	chan_down[0] = -1;

	assert_child_ready(chan_up[0], owner);

	other = fork_or_skip();
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, other);

	if (other == 0) {
		int fb;

		close(pty_a.master);
		close(pty_b.master);
		close(chan_up[0]);
		close(chan_down[1]);

		if (setsid() == (pid_t)-1) {
			_exit(EXIT_SETSID);
		}

		fb = open(pty_b.slave, O_RDWR);
		if (fb < 0) {
			_exit(EXIT_OPEN);
		}

		/* pty B is unused - a fresh session must be able to take it */
		_exit((ioctl(fb, TIOCSCTTY, 0) < 0) ? EXIT_SCTTY : EXIT_SUCCESS);
	}

	TEST_ASSERT_EQUAL_INT(other, waitpid(other, &status, 0));

	chan_post(chan_down[1]);
	(void)waitpid(owner, NULL, 0);

	TEST_ASSERT_TRUE(WIFEXITED(status));

	TEST_ASSERT_EQUAL_INT_MESSAGE(EXIT_SUCCESS, WEXITSTATUS(status),
			"a terminal nobody uses could not be acquired");
}


/*
 * A hangup dissociates the terminal from its session (XSH 11.1.11), so the
 * session leader is free to acquire another one. It is the only way it ever
 * becomes free again: the kernel records that a session holds a controlling
 * terminal, and only TIOCNOTTY, setsid() and process death clear that record.
 * Without a release on hangup a leader that outlives its pty - it is not in the
 * foreground group, or it ignores SIGHUP as here - can never take a terminal
 * again, and every later TIOCSCTTY answers EPERM.
 */
TEST(pty_ctty, ctty_released_by_hangup)
{
	pid_t child;
	int status = 0;

	TEST_ASSERT_EQUAL_INT(0, pty_open(&pty_a));
	TEST_ASSERT_EQUAL_INT(0, pty_open(&pty_b));
	TEST_ASSERT_EQUAL_INT(0, pipe(chan_up));
	TEST_ASSERT_EQUAL_INT(0, pipe(chan_down));

	child = fork_or_skip();
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, child);

	if (child == 0) {
		int fa, fb;

		close(pty_a.master);
		close(pty_b.master);
		close(chan_up[0]);
		close(chan_down[1]);

		/* Survive the hangup: TIOCSCTTY makes us the foreground group too */
		signal(SIGHUP, SIG_IGN);

		if (setsid() == (pid_t)-1) {
			_exit(EXIT_SETSID);
		}

		fa = open(pty_a.slave, O_RDWR);
		if (fa < 0) {
			_exit(EXIT_OPEN);
		}
		if (ioctl(fa, TIOCSCTTY, 0) < 0) {
			_exit(EXIT_SCTTY);
		}

		chan_post(chan_up[1]);
		if (chan_wait(chan_down[0]) != 0) {
			_exit(EXIT_OPEN);
		}

		/* pty A has hung up, so the session holds no terminal any more */
		fb = open(pty_b.slave, O_RDWR);
		if (fb < 0) {
			_exit(EXIT_OPEN2);
		}

		_exit((ioctl(fb, TIOCSCTTY, 0) < 0) ? EXIT_SCTTY2 : EXIT_SUCCESS);
	}

	close(chan_up[1]);
	chan_up[1] = -1;
	close(chan_down[0]);
	chan_down[0] = -1;

	assert_child_ready(chan_up[0], child);

	/* Hang the child's terminal up */
	pty_close(&pty_a);
	chan_post(chan_down[1]);

	TEST_ASSERT_EQUAL_INT(child, waitpid(child, &status, 0));
	TEST_ASSERT_TRUE(WIFEXITED(status));
	TEST_ASSERT_EQUAL_INT_MESSAGE(EXIT_SUCCESS, WEXITSTATUS(status),
			"a session whose terminal hung up could not acquire another one");
}


TEST_GROUP_RUNNER(pty_ctty)
{
	RUN_TEST_CASE(pty_ctty, master_side_getters_from_another_session);
	RUN_TEST_CASE(pty_ctty, second_controlling_terminal_is_rejected);
	RUN_TEST_CASE(pty_ctty, terminal_claimed_by_accident_stays_locked);
	RUN_TEST_CASE(pty_ctty, ctty_released_by_hangup);
}

#pragma GCC diagnostic push
