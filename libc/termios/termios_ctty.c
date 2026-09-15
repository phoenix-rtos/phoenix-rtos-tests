/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <termios.h>
 * TESTED:
 *    - tcgetpgrp()
 *    - tcsetpgrp()
 *    - tcgetsid()
 *
 * Covers the controlling-terminal rules, which the cases in termios.c cannot
 * reach because they only exercise EBADF and ENOTTY-on-a-regular-file.
 *
 * Every case runs against a freshly opened pty, never the console, so that a
 * failure cannot take down the shell that started the test. Since acquiring a
 * controlling terminal requires leaving the current session, the work happens
 * in a forked child that reports which step failed through its exit code; the
 * comment above each test lists the codes.
 *
 * NOTE: a pty master being closed, and a session leader with a controlling
 * terminal exiting, both send SIGHUP to the terminal's foreground process
 * group, whose default disposition is to terminate. The foreground group is
 * therefore only ever a disposable grandchild, always released before the
 * owning child exits.
 *
 * Copyright 2026 Phoenix Systems
 * Author: Adam Greloch
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* _GNU_SOURCE, not _XOPEN_SOURCE: ptsname_r() is an extension on glibc, and
 * posix_openpt() needs _XOPEN_SOURCE >= 600 on top of that. */
#define _GNU_SOURCE

#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "unity_fixture.h"


#ifndef TIOCNOTTY
#define TIOCNOTTY _IO('t', 0x22)
#endif

/* A process group id that is very unlikely to be in use */
#define CTTY_UNUSED_PGID 2000000

/* How long to wait for a signal that should already be on its way */
#define SIGNAL_ATTEMPTS 200
#define SIGNAL_DELAY_US 10000


static struct {
	int masterFd;
	int slaveFd;
	char slaveName[64];
} ctty_common;


/*
 * A grandchild parked in read(), used as a disposable process group so that
 * the foreground group is never the test runner's own.
 */
typedef struct {
	pid_t pid;
	int ready[2];
	int quit[2];
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


/*
 * Every parked child inherits the parent's ends of the control pipes of the
 * children started before it, which would keep those pipes from ever reaching
 * EOF. Track the live ones so that a new child can close them.
 */
#define PARKED_MAX 8

static parked_t *parked_active[PARKED_MAX];


static void parked_register(parked_t *p)
{
	size_t i;

	for (i = 0; i < PARKED_MAX; i++) {
		if (parked_active[i] == NULL) {
			parked_active[i] = p;
			return;
		}
	}
}


static void parked_unregister(parked_t *p)
{
	size_t i;

	for (i = 0; i < PARKED_MAX; i++) {
		if (parked_active[i] == p) {
			parked_active[i] = NULL;
			return;
		}
	}
}


/* Runs in a freshly forked child: drop every other child's control pipe */
static void parked_dropInherited(const parked_t *self)
{
	size_t i;

	for (i = 0; i < PARKED_MAX; i++) {
		if ((parked_active[i] == NULL) || (parked_active[i] == self)) {
			continue;
		}
		if (parked_active[i]->ready[0] >= 0) {
			(void)close(parked_active[i]->ready[0]);
		}
		if (parked_active[i]->quit[1] >= 0) {
			(void)close(parked_active[i]->quit[1]);
		}
	}
}


/* Starts a child in a process group of its own, in a session of its own when
 * newSession is set, or in the existing group joinPgrp when that is positive,
 * and returns once it is parked. */
static int parked_startEx(parked_t *p, int newSession, pid_t joinPgrp)
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
		char buf[1];
		int ok;

		(void)close(p->ready[0]);
		(void)close(p->quit[1]);
		p->ready[0] = -1;
		p->quit[1] = -1;
		parked_dropInherited(p);

		if (joinPgrp > 0) {
			ok = (setpgid(0, joinPgrp) == 0);
		}
		else {
			ok = (newSession != 0) ? (setsid() != (pid_t)-1) : (setpgid(0, 0) == 0);
		}
		if (ok == 0) {
			_exit(1);
		}

		if (write(p->ready[1], "r", 1) != 1) {
			_exit(2);
		}

		while (read(p->quit[0], buf, sizeof(buf)) > 0) {
		}

		_exit(0);
	}

	(void)close(p->ready[1]);
	p->ready[1] = -1;
	(void)close(p->quit[0]);
	p->quit[0] = -1;

	if (read(p->ready[0], &c, sizeof(c)) != (ssize_t)sizeof(c)) {
		parked_closePipe(p->ready);
		parked_closePipe(p->quit);
		(void)waitpid(p->pid, NULL, 0);
		p->pid = -1;
		return -1;
	}

	parked_register(p);

	return 0;
}


static int parked_start(parked_t *p, int newSession)
{
	return parked_startEx(p, newSession, 0);
}


/*
 * Lets the child exit and waits until it really has, without reaping it. The
 * child's end of the ready pipe is closed as the kernel tears the process down,
 * so the read below returns 0 exactly once the child has become a zombie.
 */
static void parked_terminate(parked_t *p)
{
	char c;

	if (p->pid < 0) {
		return;
	}

	parked_closePipe(p->quit);

	while (read(p->ready[0], &c, sizeof(c)) > 0) {
	}

	parked_closePipe(p->ready);
}


static void parked_reap(parked_t *p)
{
	if (p->pid < 0) {
		return;
	}

	(void)waitpid(p->pid, NULL, 0);
	p->pid = -1;
	parked_unregister(p);
}


static void parked_stop(parked_t *p)
{
	parked_terminate(p);
	parked_reap(p);
}


/* Opens a pty pair with O_NOCTTY, so that opening it does not disturb the
 * runner's own controlling terminal. posix_openpt() is not implemented on
 * Phoenix, but posixsrv registers /dev/ptmx, so the master is opened there. */
static int ctty_openPty(void)
{
	ctty_common.masterFd = -1;
	ctty_common.slaveFd = -1;

#ifdef __phoenix__
	ctty_common.masterFd = open("/dev/ptmx", O_RDWR | O_NOCTTY);
#else
	ctty_common.masterFd = posix_openpt(O_RDWR | O_NOCTTY);
#endif
	if (ctty_common.masterFd < 0) {
		return -1;
	}

	if ((grantpt(ctty_common.masterFd) != 0) || (unlockpt(ctty_common.masterFd) != 0) ||
			(ptsname_r(ctty_common.masterFd, ctty_common.slaveName, sizeof(ctty_common.slaveName)) != 0)) {
		(void)close(ctty_common.masterFd);
		ctty_common.masterFd = -1;
		return -1;
	}

	ctty_common.slaveFd = open(ctty_common.slaveName, O_RDWR | O_NOCTTY);
	if (ctty_common.slaveFd < 0) {
		(void)close(ctty_common.masterFd);
		ctty_common.masterFd = -1;
		return -1;
	}

	return 0;
}


static void ctty_closePty(void)
{
	if (ctty_common.slaveFd >= 0) {
		(void)close(ctty_common.slaveFd);
		ctty_common.slaveFd = -1;
	}
	if (ctty_common.masterFd >= 0) {
		(void)close(ctty_common.masterFd);
		ctty_common.masterFd = -1;
	}
}


/* Runs in a child: leave the current session and take the pty slave as the new
 * session's controlling terminal. This is the only portable way to reach the
 * controlling-terminal rules, since a process may not set the foreground group
 * of a terminal that is not its own. */
static int ctty_acquire(void)
{
	if (setsid() == (pid_t)-1) {
		return -1;
	}

	return (ioctl(ctty_common.slaveFd, TIOCSCTTY, 0) == 0) ? 0 : -1;
}


#define REQUIRE_PTY() \
	do { \
		if (ctty_openPty() != 0) { \
			TEST_IGNORE_MESSAGE("no pty available"); \
		} \
	} while (0)


/* Forks, runs 'body' in the child and asserts that it exited with 0.
 *
 * Every body below issues its tcsetpgrp() from the foreground process group, as
 * it must. Should an implementation bug make one of them a background call, the
 * default disposition of SIGTTOU would *stop* the child rather than kill it, and
 * the whole run would hang with no diagnostic. Ignoring SIGTTOU makes such a
 * call return instead, so the body's own assertions decide the outcome. It is
 * inherited by the parked grandchildren, which never touch the terminal. */
#define RUN_IN_CHILD(body) \
	do { \
		pid_t _pid = fork(); \
		int _status; \
		TEST_ASSERT_NOT_EQUAL_INT(-1, (int)_pid); \
		if (_pid == 0) { \
			(void)signal(SIGTTOU, SIG_IGN); \
			_exit(body); \
		} \
		TEST_ASSERT_EQUAL_INT((int)_pid, (int)waitpid(_pid, &_status, 0)); \
		TEST_ASSERT_TRUE(WIFEXITED(_status)); \
		TEST_ASSERT_EQUAL_INT(0, WEXITSTATUS(_status)); \
	} while (0)


TEST_GROUP(termios_ctty);


TEST_SETUP(termios_ctty)
{
	ctty_common.masterFd = -1;
	ctty_common.slaveFd = -1;
}


TEST_TEAR_DOWN(termios_ctty)
{
	ctty_closePty();
}


/* 1: setsid failed, 2: tcgetpgrp succeeded, 3: wrong errno */
static int body_noCtty_tcgetpgrp(void)
{
	if (setsid() == (pid_t)-1) {
		return 1;
	}

	/* A session that has just been created has no controlling terminal */
	errno = 0;
	if (tcgetpgrp(ctty_common.slaveFd) != (pid_t)-1) {
		return 2;
	}

	return (errno == ENOTTY) ? 0 : 3;
}


TEST(termios_ctty, tcgetpgrp_enotty_without_ctty)
{
	/* "[ENOTTY] The calling process does not have a controlling terminal, or
	 * the file is not the controlling terminal." */
	REQUIRE_PTY();
	RUN_IN_CHILD(body_noCtty_tcgetpgrp());
}


/* 1: setsid failed, 2: tcgetsid succeeded, 3: wrong errno */
static int body_noCtty_tcgetsid(void)
{
	if (setsid() == (pid_t)-1) {
		return 1;
	}

	errno = 0;
	if (tcgetsid(ctty_common.slaveFd) != (pid_t)-1) {
		return 2;
	}

	return (errno == ENOTTY) ? 0 : 3;
}


TEST(termios_ctty, tcgetsid_enotty_without_ctty)
{
	REQUIRE_PTY();
	RUN_IN_CHILD(body_noCtty_tcgetsid());
}


/* 1: setsid failed, 2: could not park a group, 3: tcsetpgrp succeeded,
 * 4: wrong errno, 5: the terminal became ours as a side effect */
static int body_noCtty_tcsetpgrp(void)
{
	parked_t fg;
	int ret = 0;

	if (setsid() == (pid_t)-1) {
		return 1;
	}

	if (parked_start(&fg, 0) != 0) {
		return 2;
	}

	errno = 0;
	if (tcsetpgrp(ctty_common.slaveFd, fg.pid) != -1) {
		ret = 3;
	}
	else if (errno != ENOTTY) {
		ret = 4;
	}
	else if (tcgetpgrp(ctty_common.slaveFd) != (pid_t)-1) {
		ret = 5;
	}
	else {
		/* Nothing to do */
	}

	parked_stop(&fg);

	return ret;
}


TEST(termios_ctty, tcsetpgrp_enotty_without_ctty)
{
	/* "[ENOTTY] The calling process does not have a controlling terminal, or
	 * the file is not the controlling terminal." tcsetpgrp() never acquires a
	 * terminal - a session takes one explicitly, with TIOCSCTTY. A terminal
	 * that quietly attached itself to whoever called tcsetpgrp() first would
	 * let any process holding the fd lock the real login shell out. */
	REQUIRE_PTY();
	RUN_IN_CHILD(body_noCtty_tcsetpgrp());
}


/* 1: acquire failed, 2: could not park a group, 3: tcsetpgrp failed,
 * 4: tcgetpgrp disagrees */
static int body_roundtrip(void)
{
	parked_t fg;
	int ret = 0;

	if (ctty_acquire() != 0) {
		return 1;
	}

	if (parked_start(&fg, 0) != 0) {
		return 2;
	}

	if (tcsetpgrp(ctty_common.slaveFd, fg.pid) != 0) {
		ret = 3;
	}
	else if (tcgetpgrp(ctty_common.slaveFd) != fg.pid) {
		ret = 4;
	}
	else {
		/* Nothing to do */
	}

	/* Release the foreground group before this session leader exits */
	parked_stop(&fg);

	return ret;
}


TEST(termios_ctty, tcsetpgrp_then_tcgetpgrp_roundtrip)
{
	/* "tcsetpgrp() shall set the foreground process group ID" and
	 * "tcgetpgrp() shall return the value of the process group ID of the
	 * foreground process group". */
	REQUIRE_PTY();
	RUN_IN_CHILD(body_roundtrip());
}


/* 1: acquire failed, 2: could not park a group, 3: tcsetpgrp failed,
 * 4: foreground group is not distinct from the session, 5: wrong session */
static int body_tcgetsid(void)
{
	parked_t fg;
	int ret = 0;

	if (ctty_acquire() != 0) {
		return 1;
	}

	if (parked_start(&fg, 0) != 0) {
		return 2;
	}

	if (tcsetpgrp(ctty_common.slaveFd, fg.pid) != 0) {
		ret = 3;
	}
	else if (fg.pid == getsid(0)) {
		/* Would let a session/group mix-up pass unnoticed */
		ret = 4;
	}
	else if (tcgetsid(ctty_common.slaveFd) != getsid(0)) {
		ret = 5;
	}
	else {
		/* Nothing to do */
	}

	parked_stop(&fg);

	return ret;
}


TEST(termios_ctty, tcgetsid_reports_session_not_foreground_group)
{
	/* "tcgetsid() shall return the process group ID of the session for which
	 * the terminal ... is the controlling terminal" - the owning session, which
	 * is deliberately different from the foreground process group here. */
	REQUIRE_PTY();
	RUN_IN_CHILD(body_tcgetsid());
}


/* 1: acquire failed, 2: tcsetpgrp accepted a negative group, 3: wrong errno
 *
 * NOTE: only a negative pgid is checked. 0 is not a process group id either -
 * unlike in setpgid() it has no "the calling process" meaning here - but Linux
 * rejects only pgid < 0 with EINVAL and lets 0 fall through to the lookup,
 * which reports ESRCH. libtty refuses everything <= 0 with EINVAL, so the two
 * disagree and no portable expectation can be written down. */
static int body_einvalPgid(void)
{
	if (ctty_acquire() != 0) {
		return 1;
	}

	errno = 0;
	if (tcsetpgrp(ctty_common.slaveFd, -1) != -1) {
		return 2;
	}

	return (errno == EINVAL) ? 0 : 3;
}


TEST(termios_ctty, tcsetpgrp_einval_non_positive_pgid)
{
	/* A process group id is always positive; giving a terminal up is
	 * TIOCNOTTY's job, not tcsetpgrp()'s */
	REQUIRE_PTY();
	RUN_IN_CHILD(body_einvalPgid());
}


/* 1: acquire failed, 2: could not take the foreground, 3: could not start a
 * foreign session, 4: handing over succeeded, 5: wrong errno, 6: the refused
 * call changed the foreground group */
static int body_epermForeignPgid(void)
{
	parked_t other;
	pid_t ownPgrp;
	int ret = 0;

	if (ctty_acquire() != 0) {
		return 1;
	}

	ownPgrp = getpgrp();

	/* Stay in the foreground ourselves. Handing the terminal to another group
	 * first would turn the call under test into a background write, which is
	 * governed by SIGTTOU and orphaned-group rules rather than by the session
	 * check we are after. */
	if (tcsetpgrp(ctty_common.slaveFd, ownPgrp) != 0) {
		return 2;
	}

	if (parked_start(&other, 1) != 0) {
		return 3;
	}

	errno = 0;
	if (tcsetpgrp(ctty_common.slaveFd, other.pid) != -1) {
		ret = 4;
	}
	else if (errno != EPERM) {
		ret = 5;
	}
	else if (tcgetpgrp(ctty_common.slaveFd) != ownPgrp) {
		ret = 6;
	}
	else {
		/* Nothing to do */
	}

	parked_stop(&other);

	return ret;
}


TEST(termios_ctty, tcsetpgrp_eperm_pgid_in_other_session)
{
	/* "[EPERM] The value of pgid_id is ... not in the same session as the
	 * calling process." */
	REQUIRE_PTY();
	RUN_IN_CHILD(body_epermForeignPgid());
}


/* 1: acquire failed, 2: could not park a group, 3: tcsetpgrp failed,
 * 4: fork failed, 5..9 reported by the outsider, 10: the outsider died of a
 * signal, 11: it changed our terminal */
static int body_outsiderRefused(void)
{
	parked_t fg;
	pid_t outsider;
	int status;
	int ret = 0;

	if (ctty_acquire() != 0) {
		return 1;
	}

	if (parked_start(&fg, 0) != 0) {
		return 2;
	}

	if (tcsetpgrp(ctty_common.slaveFd, fg.pid) != 0) {
		parked_stop(&fg);
		return 3;
	}

	outsider = fork();
	if (outsider < 0) {
		parked_stop(&fg);
		return 4;
	}

	if (outsider == 0) {
		/* Leave the session that owns the terminal */
		if (setsid() == (pid_t)-1) {
			_exit(5);
		}

		errno = 0;
		if (tcgetpgrp(ctty_common.slaveFd) != (pid_t)-1) {
			_exit(6);
		}
		if (errno != ENOTTY) {
			_exit(7);
		}

		errno = 0;
		if (tcsetpgrp(ctty_common.slaveFd, getpgrp()) != -1) {
			_exit(8);
		}
		if (errno != ENOTTY) {
			_exit(9);
		}

		_exit(0);
	}

	(void)waitpid(outsider, &status, 0);

	if (!WIFEXITED(status)) {
		ret = 10;
	}
	else if (WEXITSTATUS(status) != 0) {
		ret = WEXITSTATUS(status);
	}
	else if (tcgetpgrp(ctty_common.slaveFd) != fg.pid) {
		ret = 11;
	}
	else {
		/* Nothing to do */
	}

	parked_stop(&fg);

	return ret;
}


TEST(termios_ctty, tcsetpgrp_refused_from_other_session)
{
	/* A process outside the owning session may neither read nor set the
	 * terminal's foreground group, whatever group it names. */
	REQUIRE_PTY();
	RUN_IN_CHILD(body_outsiderRefused());
}


/* 1: acquire failed, 2: could not park a group, 3: tcsetpgrp failed,
 * 4: TIOCNOTTY failed, 5: terminal still reachable, 6: wrong errno */
static int body_notty(void)
{
	parked_t fg;

	if (ctty_acquire() != 0) {
		return 1;
	}

	if (parked_start(&fg, 0) != 0) {
		return 2;
	}

	if (tcsetpgrp(ctty_common.slaveFd, fg.pid) != 0) {
		parked_stop(&fg);
		return 3;
	}

	/* Drop the foreground group first: after TIOCNOTTY we could not */
	parked_stop(&fg);

	if (ioctl(ctty_common.slaveFd, TIOCNOTTY, 0) != 0) {
		return 4;
	}

	errno = 0;
	if (tcgetpgrp(ctty_common.slaveFd) != (pid_t)-1) {
		return 5;
	}

	return (errno == ENOTTY) ? 0 : 6;
}


TEST(termios_ctty, tiocnotty_releases_terminal)
{
	/* After the owning session gives the terminal up, it is nobody's
	 * controlling terminal again. */
	REQUIRE_PTY();
	RUN_IN_CHILD(body_notty());
}


/* 1: acquire failed, 2: could not park a group, 3: tcsetpgrp failed */
static int body_acquireAndExit(void)
{
	parked_t fg;
	int ret = 0;

	if (ctty_acquire() != 0) {
		return 1;
	}

	if (parked_start(&fg, 0) != 0) {
		return 2;
	}

	if (tcsetpgrp(ctty_common.slaveFd, fg.pid) != 0) {
		ret = 3;
	}

	parked_stop(&fg);

	return ret;
}


TEST(termios_ctty, ctty_reclaimed_after_session_leader_exits)
{
	/* A terminal outlives the session that owned it. Once that session's
	 * leader is gone the terminal belongs to nobody, so a later session must
	 * be able to claim and use it - otherwise one dead session locks the
	 * terminal out of use for good. */
	REQUIRE_PTY();

	RUN_IN_CHILD(body_acquireAndExit());

	/* Same again: fails if the terminal still names the departed session */
	RUN_IN_CHILD(body_acquireAndExit());
}


/* 1: acquire failed, 2/3: could not park a group, 4: tcsetpgrp refused a group
 * that outlived its leader, 5: tcgetpgrp disagrees */
static int body_fgGroupOutlivesLeader(void)
{
	parked_t leader, member;
	pid_t pgid;
	int ret = 0;

	if (ctty_acquire() != 0) {
		return 1;
	}

	/* A group of two, then its leader is terminated and reaped */
	if (parked_startEx(&leader, 0, 0) != 0) {
		return 2;
	}

	/* parked_stop() forgets the pid, and the group is named after the leader */
	pgid = leader.pid;

	if (parked_startEx(&member, 0, pgid) != 0) {
		parked_stop(&leader);
		return 3;
	}

	parked_stop(&leader);

	if (tcsetpgrp(ctty_common.slaveFd, pgid) != 0) {
		ret = 4;
	}
	else if (tcgetpgrp(ctty_common.slaveFd) != pgid) {
		ret = 5;
	}
	else {
		/* Nothing to do */
	}

	parked_stop(&member);

	return ret;
}


TEST(termios_ctty, tcsetpgrp_accepts_group_outliving_its_leader)
{
	/* A process group lives on for as long as any of its members does, so a
	 * job whose leader has already exited is still a valid foreground group.
	 * Probing the group's existence through its leader - getpgid(pgid) == pgid
	 * - rejects it, which would make "fg" fail on a perfectly live pipeline. */
	REQUIRE_PTY();
	RUN_IN_CHILD(body_fgGroupOutlivesLeader());
}


/* 1: acquire failed, 2: could not park a group, 3: tcsetpgrp refused a group
 * whose member is an unreaped zombie */
static int body_fgGroupUnreapedMember(void)
{
	parked_t grp;
	pid_t pgid;
	int ret = 0;

	if (ctty_acquire() != 0) {
		return 1;
	}

	if (parked_start(&grp, 0) != 0) {
		return 2;
	}

	pgid = grp.pid;

	/* Terminated, deliberately not reaped yet */
	parked_terminate(&grp);

	if (tcsetpgrp(ctty_common.slaveFd, pgid) != 0) {
		ret = 3;
	}

	parked_reap(&grp);

	return ret;
}


TEST(termios_ctty, tcsetpgrp_accepts_group_with_unreaped_member)
{
	/* A job that has just exited but has not been reaped is still a valid
	 * argument: a shell running "fg" on it must not get a spurious EPERM,
	 * it should reap the job and move on. */
	REQUIRE_PTY();
	RUN_IN_CHILD(body_fgGroupUnreapedMember());
}


/* 1: pipe failed, 2: fork failed, 3: the leader never took the terminal,
 * 4: setsid failed, 5: TIOCSCTTY refused although the controlling process had
 * already terminated */
static int body_cttyReleasedOnUnreapedLeader(void)
{
	int info[2];
	pid_t leader;
	char c;
	int ret = 0;

	if (pipe(info) < 0) {
		return 1;
	}

	leader = fork();
	if (leader < 0) {
		(void)close(info[0]);
		(void)close(info[1]);
		return 2;
	}

	if (leader == 0) {
		(void)close(info[0]);

		if (ctty_acquire() != 0) {
			_exit(1);
		}

		/* Announce ownership, then terminate without anyone reaping us */
		if (write(info[1], "a", 1) != 1) {
			_exit(2);
		}

		_exit(0);
	}

	(void)close(info[1]);

	if (read(info[0], &c, sizeof(c)) != (ssize_t)sizeof(c)) {
		(void)close(info[0]);
		(void)waitpid(leader, NULL, 0);
		return 3;
	}

	/* EOF means the leader is gone - a zombie, since we have not reaped it */
	while (read(info[0], &c, sizeof(c)) > 0) {
	}
	(void)close(info[0]);

	if (setsid() == (pid_t)-1) {
		ret = 4;
	}
	else if (ioctl(ctty_common.slaveFd, TIOCSCTTY, 0) != 0) {
		ret = 5;
	}
	else {
		/* Nothing to do */
	}

	(void)waitpid(leader, NULL, 0);

	return ret;
}


TEST(termios_ctty, ctty_released_when_controlling_process_unreaped)
{
	/* "If the process is a controlling process, the controlling terminal
	 * associated with the session shall be disassociated from the session,
	 * allowing it to be acquired by a new controlling process." - that happens
	 * when the controlling process terminates, not when its parent gets round
	 * to reaping it. Probing with getsid() cannot tell the two apart, so the
	 * terminal would stay locked to a dead session, in the worst case until
	 * init reaps an orphan. */
	REQUIRE_PTY();
	RUN_IN_CHILD(body_cttyReleasedOnUnreapedLeader());
}


/* 1: acquire failed, 2: could not park a group, 3: tcsetpgrp failed,
 * 4: second TIOCSCTTY failed, 5: the foreground group was reset */
static int body_tiocsctty_repeated(void)
{
	parked_t fg;
	int ret = 0;

	if (ctty_acquire() != 0) {
		return 1;
	}

	if (parked_start(&fg, 0) != 0) {
		return 2;
	}

	if (tcsetpgrp(ctty_common.slaveFd, fg.pid) != 0) {
		ret = 3;
	}
	else if (ioctl(ctty_common.slaveFd, TIOCSCTTY, 0) != 0) {
		ret = 4;
	}
	else if (tcgetpgrp(ctty_common.slaveFd) != fg.pid) {
		ret = 5;
	}
	else {
		/* Nothing to do */
	}

	parked_stop(&fg);

	return ret;
}


TEST(termios_ctty, tiocsctty_repeated_keeps_foreground_group)
{
	/* Re-acquiring a terminal the session already owns must not disturb job
	 * control: if it reset the foreground group to the session leader's own,
	 * the job currently in the foreground would start taking SIGTTIN. */
	REQUIRE_PTY();
	RUN_IN_CHILD(body_tiocsctty_repeated());
}


/* 1: acquire failed, 2: could not park a group, 3: tcsetpgrp failed,
 * 4: fork failed, 5: the session lost the terminal */
static int body_tiocnotty_nonLeader(void)
{
	parked_t fg;
	pid_t helper;
	int ret = 0;

	if (ctty_acquire() != 0) {
		return 1;
	}

	if (parked_start(&fg, 0) != 0) {
		return 2;
	}

	if (tcsetpgrp(ctty_common.slaveFd, fg.pid) != 0) {
		parked_stop(&fg);
		return 3;
	}

	helper = fork();
	if (helper < 0) {
		parked_stop(&fg);
		return 4;
	}

	if (helper == 0) {
		/* In the owning session, but not its leader */
		(void)ioctl(ctty_common.slaveFd, TIOCNOTTY, 0);
		_exit(0);
	}

	(void)waitpid(helper, NULL, 0);

	if (tcgetpgrp(ctty_common.slaveFd) != fg.pid) {
		ret = 5;
	}

	parked_stop(&fg);

	return ret;
}


TEST(termios_ctty, ctty_survives_tiocnotty_from_non_leader)
{
	/* Only the session leader gives up the terminal for the whole session.
	 * A non-leader detaching itself must leave the session's controlling
	 * terminal - and its foreground group - untouched, otherwise any process
	 * in the session could knock the shell off its terminal. */
	REQUIRE_PTY();
	RUN_IN_CHILD(body_tiocnotty_nonLeader());
}


/* 1: acquire failed, 2: fork failed, 3..6 reported by the grandchild */
static int body_setsidDropsCtty(void)
{
	pid_t child;
	int status;

	if (ctty_acquire() != 0) {
		return 1;
	}

	child = fork();
	if (child < 0) {
		return 2;
	}

	if (child == 0) {
		/* Still in the session that owns the terminal, so it is ours */
		if (tcgetpgrp(ctty_common.slaveFd) == (pid_t)-1) {
			_exit(3);
		}

		if (setsid() == (pid_t)-1) {
			_exit(4);
		}

		errno = 0;
		if (tcgetpgrp(ctty_common.slaveFd) != (pid_t)-1) {
			_exit(5);
		}

		_exit((errno == ENOTTY) ? 0 : 6);
	}

	if (waitpid(child, &status, 0) != child) {
		return 2;
	}

	return WIFEXITED(status) ? WEXITSTATUS(status) : 2;
}


TEST(termios_ctty, setsid_releases_inherited_ctty)
{
	/* "The process shall become a session leader of a new session ... The
	 * process shall have no controlling terminal." The descriptor stays open
	 * and still refers to a terminal, but it is no longer *this* process's
	 * controlling terminal, so the controlling-terminal calls must refuse it.
	 *
	 * This is the case the ENOTTY tests above cannot reach: they use a session
	 * that never had a controlling terminal, whereas here one is inherited and
	 * has to be given up. */
	REQUIRE_PTY();
	RUN_IN_CHILD(body_setsidDropsCtty());
}


/* 1: pipe failed, 2: fork failed, 3: the owner never took the terminal,
 * 4: setsid failed in the thief, 5: the thief took the terminal,
 * 6: the owner lost it anyway */
static int body_cttyCannotBeStolen(void)
{
	int info[2], hold[2];
	pid_t owner, thief;
	char c;
	int status;
	int ret = 0;

	if (pipe(info) < 0) {
		return 1;
	}

	if (pipe(hold) < 0) {
		(void)close(info[0]);
		(void)close(info[1]);
		return 1;
	}

	owner = fork();
	if (owner < 0) {
		(void)close(info[0]);
		(void)close(info[1]);
		(void)close(hold[0]);
		(void)close(hold[1]);
		return 2;
	}

	if (owner == 0) {
		char buf[1];

		(void)close(info[0]);
		(void)close(hold[1]);

		if (ctty_acquire() != 0) {
			_exit(1);
		}
		if (write(info[1], "o", 1) != 1) {
			_exit(2);
		}

		/* Stay alive - and keep owning the terminal - until the test lets go */
		while (read(hold[0], buf, sizeof(buf)) > 0) {
		}

		_exit(0);
	}

	(void)close(info[1]);
	(void)close(hold[0]);

	if (read(info[0], &c, sizeof(c)) != (ssize_t)sizeof(c)) {
		(void)close(info[0]);
		(void)close(hold[1]);
		(void)waitpid(owner, NULL, 0);
		return 3;
	}
	(void)close(info[0]);

	thief = fork();
	if (thief < 0) {
		(void)close(hold[1]);
		(void)waitpid(owner, NULL, 0);
		return 2;
	}

	if (thief == 0) {
		(void)close(hold[1]);

		if (setsid() == (pid_t)-1) {
			_exit(4);
		}

		/* NOTE: the refusal is what matters, not the errno. Linux reports
		 * EPERM, libtty reports ENOTTY - POSIX describes neither, since it
		 * leaves the way a session acquires a terminal implementation-defined. */
		_exit((ioctl(ctty_common.slaveFd, TIOCSCTTY, 0) == 0) ? 5 : 0);
	}

	if (waitpid(thief, &status, 0) != thief) {
		ret = 2;
	}
	else if (!WIFEXITED(status)) {
		ret = 2;
	}
	else if (WEXITSTATUS(status) != 0) {
		ret = WEXITSTATUS(status);
	}
	else {
		/* Nothing to do */
	}

	/* Release the owner and confirm it held the terminal throughout */
	(void)close(hold[1]);
	if (waitpid(owner, &status, 0) != owner) {
		ret = (ret != 0) ? ret : 2;
	}
	else if (WIFEXITED(status) && (WEXITSTATUS(status) != 0)) {
		ret = (ret != 0) ? ret : 6;
	}
	else {
		/* Nothing to do */
	}

	return ret;
}


TEST(termios_ctty, ctty_cannot_be_stolen_from_a_live_session)
{
	/* A session leader must not be able to take over a terminal that another,
	 * still living, session owns. If it could, any process on the system could
	 * detach the login shell from its terminal and read what is typed at it. */
	REQUIRE_PTY();
	RUN_IN_CHILD(body_cttyCannotBeStolen());
}


/* 1: acquire failed, 2: fork failed, 3: the helper took the terminal,
 * 4: wrong errno, 5: the session lost its terminal */
static int body_tiocsctty_nonLeader(void)
{
	pid_t helper;
	int status;
	int ret = 0;

	if (ctty_acquire() != 0) {
		return 1;
	}

	helper = fork();
	if (helper < 0) {
		return 2;
	}

	if (helper == 0) {
		/* In the owning session, but not its leader */
		errno = 0;
		if (ioctl(ctty_common.slaveFd, TIOCSCTTY, 0) == 0) {
			_exit(3);
		}
		_exit((errno == EPERM) ? 0 : 4);
	}

	if (waitpid(helper, &status, 0) != helper) {
		return 2;
	}

	if (!WIFEXITED(status)) {
		ret = 2;
	}
	else if (WEXITSTATUS(status) != 0) {
		ret = WEXITSTATUS(status);
	}
	else if (tcgetsid(ctty_common.slaveFd) != getsid(0)) {
		ret = 5;
	}
	else {
		/* Nothing to do */
	}

	return ret;
}


TEST(termios_ctty, tiocsctty_refused_from_non_leader)
{
	/* Only a session leader takes a controlling terminal for its session.
	 * A non-leader asking for one would otherwise silently redirect the whole
	 * session's job control to a terminal its leader never chose. */
	REQUIRE_PTY();
	RUN_IN_CHILD(body_tiocsctty_nonLeader());
}


/*
 * 1: pipe failed, 2: fork failed, 3: the controlling process never reported in,
 * 4: it failed to set things up, 5: the foreground group outlived it
 *
 * The foreground group here is a grandchild, so it survives its own session
 * leader and cannot be waited for - its death is observed through the EOF of a
 * pipe only it still holds open.
 */
static int body_sighupOnControllingProcessExit(void)
{
	int info[2], alive[2];
	pid_t leader, fgPid = -1;
	int status, i;
	int ret = 5;

	if (pipe(info) < 0) {
		return 1;
	}

	if (pipe(alive) < 0) {
		(void)close(info[0]);
		(void)close(info[1]);
		return 1;
	}

	leader = fork();
	if (leader < 0) {
		(void)close(info[0]);
		(void)close(info[1]);
		(void)close(alive[0]);
		(void)close(alive[1]);
		return 2;
	}

	if (leader == 0) {
		pid_t fg;

		(void)close(info[0]);
		(void)close(alive[0]);

		if (ctty_acquire() != 0) {
			_exit(1);
		}

		fg = fork();
		if (fg < 0) {
			_exit(2);
		}

		if (fg == 0) {
			(void)close(info[1]);

			if (setpgid(0, 0) != 0) {
				_exit(1);
			}

			/* Holds the last write end of 'alive' from here on */
			for (;;) {
				(void)pause();
			}
		}

		if (setpgid(fg, fg) != 0) {
			_exit(3);
		}
		if (tcsetpgrp(ctty_common.slaveFd, fg) != 0) {
			_exit(4);
		}
		if (write(info[1], &fg, sizeof(fg)) != (ssize_t)sizeof(fg)) {
			_exit(5);
		}

		/* The controlling process terminates, with the terminal still its own */
		_exit(0);
	}

	(void)close(info[1]);
	(void)close(alive[1]);

	if (read(info[0], &fgPid, sizeof(fgPid)) != (ssize_t)sizeof(fgPid)) {
		(void)close(info[0]);
		(void)close(alive[0]);
		(void)waitpid(leader, NULL, 0);
		return 3;
	}
	(void)close(info[0]);

	if (waitpid(leader, &status, 0) != leader) {
		(void)close(alive[0]);
		return 2;
	}
	if (!WIFEXITED(status) || (WEXITSTATUS(status) != 0)) {
		(void)close(alive[0]);
		(void)kill(fgPid, SIGKILL);
		return 4;
	}

	/* Poll rather than block, so that a SIGHUP that never comes fails the test
	 * instead of hanging the run */
	if (fcntl(alive[0], F_SETFL, O_NONBLOCK) == 0) {
		for (i = 0; i < SIGNAL_ATTEMPTS; i++) {
			char c;
			ssize_t n = read(alive[0], &c, sizeof(c));

			if (n == 0) {
				/* EOF: the foreground group is gone */
				ret = 0;
				break;
			}
			usleep(SIGNAL_DELAY_US);
		}
	}

	(void)close(alive[0]);

	if (ret != 0) {
		(void)kill(fgPid, SIGKILL);
	}

	return ret;
}


TEST(termios_ctty, sighup_to_foreground_group_when_controlling_process_exits)
{
	/* "If the process is a controlling process, the SIGHUP signal shall be sent
	 * to each process in the foreground process group of the controlling
	 * terminal belonging to the calling process."
	 *
	 * This is how a job finds out that its terminal has gone away. Without it,
	 * the jobs of a shell that has died keep running against a terminal nobody
	 * owns, and nothing ever tells them to stop. */
	REQUIRE_PTY();
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("SIGHUP to the foreground group is not implemented");
#endif
	RUN_IN_CHILD(body_sighupOnControllingProcessExit());
}


#ifdef __phoenix__
/* 1: acquire failed, 2: tcsetpgrp accepted a group that holds no process,
 * 3: wrong errno */
static int body_epermDeadPgid(void)
{
	if (ctty_acquire() != 0) {
		return 1;
	}

	errno = 0;
	if (tcsetpgrp(ctty_common.slaveFd, CTTY_UNUSED_PGID) != -1) {
		return 2;
	}

	return (errno == EPERM) ? 0 : 3;
}


TEST(termios_ctty, tcsetpgrp_eperm_pgid_without_any_process)
{
	/* "[EPERM] The value of pgid_id is a value supported by the implementation,
	 * but does not match the process group ID of a process in the same session
	 * as the calling process."
	 *
	 * Phoenix only: Linux looks the argument up as a pid when no group of that
	 * id exists (session_of_pgrp() falls back to PIDTYPE_PID) and reports ESRCH
	 * when even that finds nothing, so no portable expectation exists. libtty
	 * asks procExists() about the group itself and refuses with EPERM. */
	REQUIRE_PTY();
	RUN_IN_CHILD(body_epermDeadPgid());
}
#endif


TEST_GROUP_RUNNER(termios_ctty)
{
	RUN_TEST_CASE(termios_ctty, tcgetpgrp_enotty_without_ctty);
	RUN_TEST_CASE(termios_ctty, tcgetsid_enotty_without_ctty);
	RUN_TEST_CASE(termios_ctty, tcsetpgrp_enotty_without_ctty);
	RUN_TEST_CASE(termios_ctty, tcsetpgrp_then_tcgetpgrp_roundtrip);
	RUN_TEST_CASE(termios_ctty, tcgetsid_reports_session_not_foreground_group);
	RUN_TEST_CASE(termios_ctty, tcsetpgrp_einval_non_positive_pgid);
	RUN_TEST_CASE(termios_ctty, tcsetpgrp_eperm_pgid_in_other_session);
	RUN_TEST_CASE(termios_ctty, tcsetpgrp_refused_from_other_session);
	RUN_TEST_CASE(termios_ctty, tiocnotty_releases_terminal);
	RUN_TEST_CASE(termios_ctty, ctty_reclaimed_after_session_leader_exits);
	RUN_TEST_CASE(termios_ctty, tcsetpgrp_accepts_group_outliving_its_leader);
	RUN_TEST_CASE(termios_ctty, tcsetpgrp_accepts_group_with_unreaped_member);
	RUN_TEST_CASE(termios_ctty, ctty_released_when_controlling_process_unreaped);
	RUN_TEST_CASE(termios_ctty, tiocsctty_repeated_keeps_foreground_group);
	RUN_TEST_CASE(termios_ctty, ctty_survives_tiocnotty_from_non_leader);
	RUN_TEST_CASE(termios_ctty, setsid_releases_inherited_ctty);
	RUN_TEST_CASE(termios_ctty, ctty_cannot_be_stolen_from_a_live_session);
	RUN_TEST_CASE(termios_ctty, tiocsctty_refused_from_non_leader);
	RUN_TEST_CASE(termios_ctty, sighup_to_foreground_group_when_controlling_process_exits);
#ifdef __phoenix__
	RUN_TEST_CASE(termios_ctty, tcsetpgrp_eperm_pgid_without_any_process);
#endif
}
