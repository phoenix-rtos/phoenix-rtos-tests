/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <signal.h>
 * TESTED:
 *    - kill()
 *    - killpg()
 *
 * Covers the group-directed forms of kill(): pid == 0 (the caller's own
 * process group), pid < -1 (a named process group, the form a shell uses to
 * signal a job) and pid == -1 (every process the caller may signal), plus
 * killpg(), which is the same reach spelled with a positive argument.
 *
 * NOTE: the pid == -1 broadcast is only exercised with signal 0, which POSIX
 * defines as an error check that delivers nothing. Sending a real signal to
 * every process in the system would take down the drivers and the shell.
 *
 * Copyright 2026 Phoenix Systems
 * Author: Adam Greloch
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "unity_fixture.h"


/* How long to wait for a signalled child to actually die */
#define REAP_ATTEMPTS 200
#define REAP_DELAY_US 10000

/* A process group id that is very unlikely to be in use */
#define UNUSED_PGID 2000000


TEST_GROUP(signal_killgroup);


TEST_SETUP(signal_killgroup)
{
}


TEST_TEAR_DOWN(signal_killgroup)
{
}


/* Waits for 'pid' to be reapable without blocking forever, so that a signal
 * that never arrives fails the test instead of hanging it. Returns 1 when the
 * child was reaped, and stores its wait status in *status. */
static int reapWithin(pid_t pid, int *status)
{
	int i;

	for (i = 0; i < REAP_ATTEMPTS; ++i) {
		if (waitpid(pid, status, WNOHANG) == pid) {
			return 1;
		}
		usleep(REAP_DELAY_US);
	}

	return 0;
}


/* A child that was reached by the signal died of it. Merely being gone is not
 * enough: every child below has error paths of its own that end in _exit(),
 * and those must not read as a delivered signal. */
static void assertKilledBy(pid_t pid, int sig, const char *what)
{
	int status = 0;

	TEST_ASSERT_TRUE_MESSAGE(reapWithin(pid, &status), what);
	TEST_ASSERT_TRUE_MESSAGE(WIFSIGNALED(status), what);
	TEST_ASSERT_EQUAL_INT_MESSAGE(sig, WTERMSIG(status), what);
}


/*
 * Forks a child, puts it in the process group 'pgid' - a group of its own when
 * pgid is 0 - and returns only once it is parked in pause(), so that the group
 * is known to exist before anything is sent to it. Returns -1 on failure.
 */
static pid_t startGroupMember(pid_t pgid)
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

		if (setpgid(0, pgid) != 0) {
			_exit(1);
		}
		if (write(ready[1], "r", 1) != 1) {
			_exit(2);
		}

		for (;;) {
			(void)pause();
		}
	}

	/* Both ends are closed here before returning, so that a member started
	 * later does not inherit this one's pipe */
	(void)close(ready[1]);

	if (read(ready[0], &c, sizeof(c)) != (ssize_t)sizeof(c)) {
		(void)close(ready[0]);
		(void)waitpid(pid, NULL, 0);
		return -1;
	}
	(void)close(ready[0]);

	return pid;
}


/* A group is named after the process that created it, so the absence of that
 * process is a good enough proxy for the absence of the group. */
static int unusedPgidIsFree(void)
{
	return (getpgid(UNUSED_PGID) == (pid_t)-1) ? 1 : 0;
}


TEST(signal_killgroup, kill_zero_signal_checks_own_group)
{
	/* "If sig is 0 (the null signal), error checking is performed but no
	 * signal is actually sent." The caller's own group always exists. */
	errno = 0;
	TEST_ASSERT_EQUAL_INT(0, kill(0, 0));
}


TEST(signal_killgroup, killpg_zero_signal_checks_own_group)
{
	/* "If pgrp is 0, killpg() shall send the signal to the calling process's
	 * process group." */
	errno = 0;
	TEST_ASSERT_EQUAL_INT(0, killpg(0, 0));
}


TEST(signal_killgroup, kill_broadcast_null_signal_permitted)
{
	/* "If pid is -1, sig shall be sent to all processes ... for which the
	 * process has permission to send that signal." With sig 0 this only
	 * confirms that at least one such process exists. */
	errno = 0;
	TEST_ASSERT_EQUAL_INT(0, kill(-1, 0));
}


TEST(signal_killgroup, kill_zero_reaches_whole_own_group)
{
	/* pid == 0 shall reach every member of the caller's process group, not
	 * just the caller. Two children are put into one group and one of them is
	 * told to signal the group it belongs to; both must die. */
	int toChild[2], fromChild[2], fromBystander[2];
	pid_t killer, bystander;
	char c;

	TEST_ASSERT_EQUAL_INT(0, pipe(toChild));
	TEST_ASSERT_EQUAL_INT(0, pipe(fromChild));
	TEST_ASSERT_EQUAL_INT(0, pipe(fromBystander));

	killer = fork();
	TEST_ASSERT_NOT_EQUAL_INT(-1, (int)killer);

	if (killer == 0) {
		char buf[1];

		(void)close(toChild[1]);
		(void)close(fromChild[0]);
		(void)close(fromBystander[0]);
		(void)close(fromBystander[1]);

		/* Lead a group of our own, away from the test runner's group */
		if (setpgid(0, 0) != 0) {
			_exit(1);
		}
		if (write(fromChild[1], "r", 1) != 1) {
			_exit(2);
		}

		/* Wait for the bystander to join, then signal the whole group */
		if (read(toChild[0], buf, sizeof(buf)) != 1) {
			_exit(3);
		}

		(void)kill(0, SIGKILL);

		/* Must not be reached: SIGKILL cannot be caught or ignored */
		_exit(4);
	}

	(void)close(toChild[0]);
	(void)close(fromChild[1]);

	/* The killer is in its own group before anything else happens */
	TEST_ASSERT_EQUAL_INT(1, (int)read(fromChild[0], &c, 1));
	TEST_ASSERT_EQUAL_INT((int)killer, (int)getpgid(killer));

	bystander = fork();
	TEST_ASSERT_NOT_EQUAL_INT(-1, (int)bystander);

	if (bystander == 0) {
		(void)close(toChild[1]);
		(void)close(fromChild[0]);
		(void)close(fromBystander[0]);

		/* Join the killer's group and wait to be killed */
		if (setpgid(0, killer) != 0) {
			_exit(1);
		}
		if (write(fromBystander[1], "r", 1) != 1) {
			_exit(2);
		}

		for (;;) {
			(void)pause();
		}
	}

	(void)close(fromBystander[1]);

	/* Both children must be in the killer's group before the signal flies.
	 * The bystander reports in once setpgid() has returned, so the group is
	 * complete by construction rather than after a guessed delay. */
	TEST_ASSERT_EQUAL_INT(1, (int)read(fromBystander[0], &c, 1));
	(void)close(fromBystander[0]);

	TEST_ASSERT_EQUAL_INT((int)killer, (int)getpgid(killer));
	TEST_ASSERT_EQUAL_INT((int)killer, (int)getpgid(bystander));

	TEST_ASSERT_EQUAL_INT(1, (int)write(toChild[1], "g", 1));

	assertKilledBy(killer, SIGKILL, "the signalling process survived kill(0, SIGKILL)");
	assertKilledBy(bystander, SIGKILL, "kill(0, SIGKILL) did not reach the whole group");

	(void)close(toChild[1]);
	(void)close(fromChild[0]);
}


TEST(signal_killgroup, kill_zero_does_not_leave_own_group)
{
	/* The group form must not reach processes outside the group. The test
	 * runner stays in its own group while a child group is wiped out. */
	int fromChild[2];
	pid_t child;
	pid_t runnerPgid;
	char c;

	runnerPgid = getpgrp();

	TEST_ASSERT_EQUAL_INT(0, pipe(fromChild));

	child = fork();
	TEST_ASSERT_NOT_EQUAL_INT(-1, (int)child);

	if (child == 0) {
		(void)close(fromChild[0]);

		if (setpgid(0, 0) != 0) {
			_exit(1);
		}
		if (write(fromChild[1], "r", 1) != 1) {
			_exit(2);
		}

		(void)kill(0, SIGKILL);
		_exit(3);
	}

	(void)close(fromChild[1]);
	TEST_ASSERT_EQUAL_INT(1, (int)read(fromChild[0], &c, 1));
	(void)close(fromChild[0]);

	assertKilledBy(child, SIGKILL, "child survived kill(0, SIGKILL)");

	/* Still here, still in the same group */
	TEST_ASSERT_EQUAL_INT((int)runnerPgid, (int)getpgrp());
}


TEST(signal_killgroup, kill_negative_pid_reaches_named_group)
{
	/* "If pid is negative, but not -1, sig shall be sent to all processes ...
	 * whose process group ID is equal to the absolute value of pid".
	 *
	 * This is the form a shell uses to signal a job, and unlike pid == 0 it
	 * reaches a group the caller is not a member of. */
	pid_t leader, member, runnerPgid;

	runnerPgid = getpgrp();

	leader = startGroupMember(0);
	TEST_ASSERT_NOT_EQUAL_INT(-1, (int)leader);
	TEST_ASSERT_EQUAL_INT((int)leader, (int)getpgid(leader));
	TEST_ASSERT_NOT_EQUAL_INT((int)runnerPgid, (int)leader);

	member = startGroupMember(leader);
	TEST_ASSERT_NOT_EQUAL_INT(-1, (int)member);
	TEST_ASSERT_EQUAL_INT((int)leader, (int)getpgid(member));

	TEST_ASSERT_EQUAL_INT(0, kill(-leader, SIGKILL));

	assertKilledBy(leader, SIGKILL, "kill(-pgid) did not reach the group leader");
	assertKilledBy(member, SIGKILL, "kill(-pgid) did not reach the whole group");

	/* The caller is in a different group, so it must be untouched */
	TEST_ASSERT_EQUAL_INT((int)runnerPgid, (int)getpgrp());
}


TEST(signal_killgroup, killpg_reaches_named_group)
{
	/* "The killpg() function shall send the signal ... to a process group" -
	 * the same reach as kill(-pgrp, sig), spelled with a positive argument. */
	pid_t member, runnerPgid;

	runnerPgid = getpgrp();

	member = startGroupMember(0);
	TEST_ASSERT_NOT_EQUAL_INT(-1, (int)member);
	TEST_ASSERT_EQUAL_INT((int)member, (int)getpgid(member));
	TEST_ASSERT_NOT_EQUAL_INT((int)runnerPgid, (int)member);

	TEST_ASSERT_EQUAL_INT(0, killpg(member, SIGKILL));

	assertKilledBy(member, SIGKILL, "killpg() did not reach the named group");

	TEST_ASSERT_EQUAL_INT((int)runnerPgid, (int)getpgrp());
}


TEST(signal_killgroup, kill_esrch_unused_group)
{
	/* "[ESRCH] No process or process group can be found corresponding to that
	 * specified by pid." */
	if (unusedPgidIsFree() == 0) {
		TEST_IGNORE_MESSAGE("the pid used as an unused-group probe exists on this system");
	}

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, kill(-UNUSED_PGID, 0));
	TEST_ASSERT_EQUAL_INT(ESRCH, errno);
}


TEST(signal_killgroup, killpg_esrch_unused_group)
{
	/* "[ESRCH] No process can be found in the process group specified by
	 * pgrp." */
	if (unusedPgidIsFree() == 0) {
		TEST_IGNORE_MESSAGE("the pid used as an unused-group probe exists on this system");
	}

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, killpg(UNUSED_PGID, 0));
	TEST_ASSERT_EQUAL_INT(ESRCH, errno);
}


TEST(signal_killgroup, killpg_einval_negative_pgrp)
{
	/* "[EINVAL] The value of the pgrp argument is not a valid process group
	 * ID."
	 *
	 * A negative pgrp must be refused outright rather than folded back into a
	 * positive one: negating it would turn killpg() into a kill() of the single
	 * process whose *pid* is that value, and killpg(-1, sig) into the broadcast.
	 * Signal 0 is used throughout so that an implementation that does fold it
	 * cannot do any damage while failing this test. */
	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, killpg(-1, 0));
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, killpg(-UNUSED_PGID, 0));
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST_GROUP_RUNNER(signal_killgroup)
{
	RUN_TEST_CASE(signal_killgroup, kill_zero_signal_checks_own_group);
	RUN_TEST_CASE(signal_killgroup, killpg_zero_signal_checks_own_group);
	RUN_TEST_CASE(signal_killgroup, kill_broadcast_null_signal_permitted);
	RUN_TEST_CASE(signal_killgroup, kill_zero_reaches_whole_own_group);
	RUN_TEST_CASE(signal_killgroup, kill_zero_does_not_leave_own_group);
	RUN_TEST_CASE(signal_killgroup, kill_negative_pid_reaches_named_group);
	RUN_TEST_CASE(signal_killgroup, killpg_reaches_named_group);
	RUN_TEST_CASE(signal_killgroup, kill_esrch_unused_group);
	RUN_TEST_CASE(signal_killgroup, killpg_esrch_unused_group);
	RUN_TEST_CASE(signal_killgroup, killpg_einval_negative_pgrp);
}
