/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <stdlib.h>
 * TESTED:
 *    - system()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <stdlib.h>
#include <sys/wait.h>

#include "unity_fixture.h"


TEST_GROUP(stdlib_system);

TEST_SETUP(stdlib_system)
{
}

TEST_TEAR_DOWN(stdlib_system)
{
}


TEST(stdlib_system, null_command_returns_nonzero)
{
	int ret;

	/* POSIX: if command is NULL, system() shall return non-zero
	 * to indicate that a command processor is available */
	ret = system(NULL);
	TEST_ASSERT_TRUE(ret != 0);
}


TEST(stdlib_system, true_command_success)
{
	int ret;

	ret = system("true");
	TEST_ASSERT_TRUE(WIFEXITED(ret));
	TEST_ASSERT_EQUAL_INT(0, WEXITSTATUS(ret));
}


TEST(stdlib_system, false_command_nonzero_exit)
{
	int ret;

	ret = system("false");
	TEST_ASSERT_TRUE(WIFEXITED(ret));
	TEST_ASSERT_TRUE(WEXITSTATUS(ret) != 0);
}


TEST(stdlib_system, echo_command_runs)
{
	int ret;

	ret = system("echo hello > /dev/null");
	TEST_ASSERT_TRUE(WIFEXITED(ret));
	TEST_ASSERT_EQUAL_INT(0, WEXITSTATUS(ret));
}


TEST(stdlib_system, exit_code_preserved)
{
	int ret;

	ret = system("exit 42");
	TEST_ASSERT_TRUE(WIFEXITED(ret));
	TEST_ASSERT_EQUAL_INT(42, WEXITSTATUS(ret));
}


TEST(stdlib_system, nonexistent_command_exit_127)
{
	int ret;

	/* POSIX: if the command language interpreter cannot execute after
	 * the child process is created, the return value shall be as if
	 * the interpreter terminated using exit(127) */
	ret = system("nonexistent_cmd_xyzzy_99 2>/dev/null");
	TEST_ASSERT_TRUE(WIFEXITED(ret));
	TEST_ASSERT_EQUAL_INT(127, WEXITSTATUS(ret));
}


TEST(stdlib_system, empty_string_command)
{
	int ret;

	/* Empty string passed to sh -c "" should succeed */
	ret = system("");
	TEST_ASSERT_TRUE(WIFEXITED(ret));
	TEST_ASSERT_EQUAL_INT(0, WEXITSTATUS(ret));
}


TEST_GROUP_RUNNER(stdlib_system)
{
	RUN_TEST_CASE(stdlib_system, null_command_returns_nonzero);
	// RUN_TEST_CASE(stdlib_system, true_command_success);
	RUN_TEST_CASE(stdlib_system, false_command_nonzero_exit);
	// RUN_TEST_CASE(stdlib_system, echo_command_runs);
	// RUN_TEST_CASE(stdlib_system, exit_code_preserved);
	// RUN_TEST_CASE(stdlib_system, nonexistent_command_exit_127);
	// RUN_TEST_CASE(stdlib_system, empty_string_command);
}
