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


TEST_GROUP_RUNNER(proc_fork)
{
	RUN_TEST_CASE(proc_fork, fork_returns_zero_to_child);
	RUN_TEST_CASE(proc_fork, fork_returns_child_pid_to_parent);
}
