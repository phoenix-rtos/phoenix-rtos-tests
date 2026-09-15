/*
 * Phoenix-RTOS
 *
 * Phoenix-RTOS extension tests
 * HEADER:
 *    - <sys/proc.h>
 * TESTED:
 *    - procExists()
 *
 * procExists() is the kernel primitive a terminal driver needs in order to tell
 * whether a recorded foreground process group still exists and whether the
 * controlling process of a session is still alive. getpgid()/getsid() cannot
 * answer either question: they probe a single pid, so a group that outlived its
 * leader looks gone, while a process that has terminated keeps answering until
 * its parent reaps it. The cases below pin down exactly those two boundaries.
 *
 * Copyright 2026 Phoenix Systems
 * Author: Adam Greloch
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#ifdef __phoenix__
#include <sys/proc.h>
#endif

#include "unity_fixture.h"


TEST_GROUP(sys_procExists);


#ifdef __phoenix__

/*
 * A child parked on a pipe, so that the test decides when it terminates and,
 * separately, when it is reaped.
 */
typedef struct {
	pid_t pid;
	int ready[2]; /* child -> parent, also signals the child's death by EOF */
	int quit[2];  /* parent -> child, closing it tells the child to exit */
	int reaped;
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


/*
 * Forks a child and returns once it is parked. newPgrp puts it in a process
 * group of its own, joinPgrp (when non-zero) makes it join that group instead.
 */
static int parked_start(parked_t *p, int newPgrp, pid_t joinPgrp)
{
	char c;

	p->pid = -1;
	p->reaped = 0;
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

		(void)close(p->ready[0]);
		(void)close(p->quit[1]);
		p->ready[0] = -1;
		p->quit[1] = -1;
		parked_dropInherited(p);

		if (joinPgrp > 0) {
			if (setpgid(0, joinPgrp) != 0) {
				_exit(1);
			}
		}
		else if (newPgrp != 0) {
			if (setpgid(0, 0) != 0) {
				_exit(1);
			}
		}
		else {
			/* Stay in the parent's process group */
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


/*
 * Lets the child exit and waits until it really has, without reaping it - the
 * child's end of the ready pipe is closed by the kernel as it tears the process
 * down, so the read below returns 0 exactly once the child is a zombie.
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
	if ((p->pid < 0) || (p->reaped != 0)) {
		return;
	}

	(void)waitpid(p->pid, NULL, 0);
	p->reaped = 1;
	parked_unregister(p);
}


static void parked_stop(parked_t *p)
{
	parked_terminate(p);
	parked_reap(p);
	p->pid = -1;
}


TEST_SETUP(sys_procExists)
{
}


TEST_TEAR_DOWN(sys_procExists)
{
}


TEST(sys_procExists, einval_on_no_filter)
{
	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, procExists(0, 0, 0, 0));
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST(sys_procExists, einval_on_negative_filter)
{
	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, procExists(-1, 0, 0, 0));
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, procExists(0, -1, 0, 0));
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, procExists(0, 0, -1, 0));
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST(sys_procExists, einval_on_unknown_flag)
{
	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, procExists(getpid(), 0, 0, ~PROCQ_ALIVE));
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST(sys_procExists, finds_self)
{
	pid_t pid = getpid();

	TEST_ASSERT_EQUAL_INT(1, procExists(pid, 0, 0, 0));
	TEST_ASSERT_EQUAL_INT(1, procExists(0, getpgrp(), 0, 0));
	TEST_ASSERT_EQUAL_INT(1, procExists(0, 0, getsid(0), 0));
	TEST_ASSERT_EQUAL_INT(1, procExists(pid, getpgrp(), getsid(0), PROCQ_ALIVE));
}


TEST(sys_procExists, filters_are_conjunctive)
{
	/*
	 * The pid filter narrows the search down to this process, whose group is
	 * getpgrp() - so no process can match both filters below.
	 */
	TEST_ASSERT_EQUAL_INT(1, procExists(getpid(), getpgrp(), 0, 0));
	TEST_ASSERT_EQUAL_INT(0, procExists(getpid(), getpgrp() + 1, 0, 0));
}


TEST(sys_procExists, group_outlives_its_leader)
{
	/*
	 * A process group exists for as long as any of its members is alive, not
	 * only while its leader is. This is the case getpgid(pgid) == pgid gets
	 * wrong, and the reason a terminal driver cannot use it to validate the
	 * argument of tcsetpgrp().
	 */
	parked_t leader, member;
	pid_t pgid;

	TEST_ASSERT_EQUAL_INT(0, parked_start(&leader, 1, 0));
	TEST_ASSERT_EQUAL_INT(0, parked_start(&member, 0, leader.pid));

	/* parked_stop() forgets the pid, and the group is named after the leader */
	pgid = leader.pid;

	TEST_ASSERT_EQUAL_INT(1, procExists(0, pgid, 0, 0));

	/* The leader is gone for good, the group still has a live member */
	parked_stop(&leader);

	/* getpgid() probes a single pid, so the group now looks gone to it */
	TEST_ASSERT_EQUAL_INT(-1, getpgid(pgid));
	TEST_ASSERT_EQUAL_INT(0, procExists(pgid, 0, 0, 0));

	/* ... while the group itself is very much alive */
	TEST_ASSERT_EQUAL_INT(1, procExists(0, pgid, 0, 0));
	TEST_ASSERT_EQUAL_INT(1, procExists(0, pgid, getsid(0), 0));
	TEST_ASSERT_EQUAL_INT(1, procExists(0, pgid, getsid(0), PROCQ_ALIVE));

	parked_stop(&member);

	/* Last member gone, so is the group */
	TEST_ASSERT_EQUAL_INT(0, procExists(0, pgid, 0, 0));
}


TEST(sys_procExists, zombie_counts_only_without_procq_alive)
{
	/*
	 * A process that has terminated but has not been reaped keeps holding its
	 * group and session, the way getpgid() and Linux report it - PROCQ_ALIVE
	 * asks for the POSIX-literal reading, where its lifetime is already over.
	 */
	parked_t child;

	TEST_ASSERT_EQUAL_INT(0, parked_start(&child, 1, 0));
	TEST_ASSERT_EQUAL_INT(1, procExists(child.pid, 0, 0, PROCQ_ALIVE));

	parked_terminate(&child);

	/* Unreaped: still a group of its own as far as getpgid() is concerned */
	TEST_ASSERT_EQUAL_INT(child.pid, getpgid(child.pid));
	TEST_ASSERT_EQUAL_INT(1, procExists(child.pid, 0, 0, 0));
	TEST_ASSERT_EQUAL_INT(1, procExists(0, child.pid, 0, 0));

	/* ... but not alive */
	TEST_ASSERT_EQUAL_INT(0, procExists(child.pid, 0, 0, PROCQ_ALIVE));
	TEST_ASSERT_EQUAL_INT(0, procExists(0, child.pid, 0, PROCQ_ALIVE));

	parked_reap(&child);

	TEST_ASSERT_EQUAL_INT(0, procExists(child.pid, 0, 0, 0));
	TEST_ASSERT_EQUAL_INT(0, procExists(child.pid, 0, 0, PROCQ_ALIVE));
}


TEST(sys_procExists, unknown_session_is_not_alive)
{
	pid_t sid = getsid(0);

	TEST_ASSERT_NOT_EQUAL_INT(-1, (int)sid);

	/* We are in it, so it is alive by definition */
	TEST_ASSERT_EQUAL_INT(1, procExists(0, 0, sid, PROCQ_ALIVE));

	/* A session id that names no process at all */
	TEST_ASSERT_EQUAL_INT(0, procExists(0, 0, sid + 1000000, 0));
}


TEST(sys_procExists, session_outlives_its_leader)
{
	/*
	 * A session is not over when its leader dies - its remaining members stay
	 * in it. Only the controlling terminal is given up, which is why a terminal
	 * driver must ask about the leader, procExists(sid, 0, sid, PROCQ_ALIVE),
	 * and not about the session as a whole.
	 *
	 * The member is a grandchild, so that it survives its own session leader.
	 * It cannot be reaped here, it is left to init.
	 */
	int hold[2], info[2];
	pid_t leader, member = -1;

	TEST_ASSERT_EQUAL_INT(0, pipe(hold));
	TEST_ASSERT_EQUAL_INT(0, pipe(info));

	leader = fork();
	TEST_ASSERT_NOT_EQUAL_INT(-1, (int)leader);

	if (leader == 0) {
		pid_t child;

		(void)close(info[0]);
		(void)close(hold[1]);

		if (setsid() == (pid_t)-1) {
			_exit(1);
		}

		child = fork();
		if (child < 0) {
			_exit(2);
		}

		if (child == 0) {
			char buf[1];

			(void)close(info[1]);

			/* Parked until the test closes the write end of 'hold' */
			while (read(hold[0], buf, sizeof(buf)) > 0) {
			}

			_exit(0);
		}

		if (write(info[1], &child, sizeof(child)) != (ssize_t)sizeof(child)) {
			_exit(3);
		}

		_exit(0);
	}

	(void)close(info[1]);
	(void)close(hold[0]);

	TEST_ASSERT_EQUAL_INT((int)sizeof(member), (int)read(info[0], &member, sizeof(member)));
	(void)close(info[0]);

	TEST_ASSERT_EQUAL_INT(leader, (int)waitpid(leader, NULL, 0));

	/* The session id is the leader's pid; the leader is gone and reaped */
	TEST_ASSERT_EQUAL_INT(0, procExists(leader, 0, 0, 0));
	TEST_ASSERT_EQUAL_INT(0, procExists(leader, 0, leader, PROCQ_ALIVE));

	/* ... but the session still has a live member, so it is not over */
	TEST_ASSERT_EQUAL_INT(1, procExists(member, 0, leader, PROCQ_ALIVE));
	TEST_ASSERT_EQUAL_INT(1, procExists(0, 0, leader, PROCQ_ALIVE));

	/* Let the member go; it is init's child now and init reaps it */
	(void)close(hold[1]);
}


TEST_GROUP_RUNNER(sys_procExists)
{
	RUN_TEST_CASE(sys_procExists, einval_on_no_filter);
	RUN_TEST_CASE(sys_procExists, einval_on_negative_filter);
	RUN_TEST_CASE(sys_procExists, einval_on_unknown_flag);
	RUN_TEST_CASE(sys_procExists, finds_self);
	RUN_TEST_CASE(sys_procExists, filters_are_conjunctive);
	RUN_TEST_CASE(sys_procExists, group_outlives_its_leader);
	RUN_TEST_CASE(sys_procExists, zombie_counts_only_without_procq_alive);
	RUN_TEST_CASE(sys_procExists, unknown_session_is_not_alive);
	RUN_TEST_CASE(sys_procExists, session_outlives_its_leader);
}

#else /* !__phoenix__ */

TEST_SETUP(sys_procExists)
{
}


TEST_TEAR_DOWN(sys_procExists)
{
}


TEST(sys_procExists, not_available)
{
	TEST_IGNORE_MESSAGE("procExists() is a Phoenix-RTOS extension");
}


TEST_GROUP_RUNNER(sys_procExists)
{
	RUN_TEST_CASE(sys_procExists, not_available);
}

#endif
