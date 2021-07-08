/*
 * Phoenix-RTOS
 *
 * libc-tests
 *
 * Testing POSIX file operations.
 *
 * Copyright 2021 Phoenix Systems
 * Author: Marek Bialowas
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

/* make compilable against glibc (all these tests were run on host against libc/linux kernel) */
#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <unity_fixture.h>

#include "common.h"


TEST_GROUP(file);

TEST_SETUP(file)
{
}


TEST_TEAR_DOWN(file)
{
}


TEST(file, fclose_stdin)
{
	int save_stdin = dup(STDIN_FILENO);
	TEST_ASSERT_GREATER_THAN(-1, save_stdin);

	TEST_ASSERT_EQUAL_INT(0, fclose(stdin));

	/* reopen/recreate the stdin */
	TEST_ASSERT_EQUAL_INT(STDIN_FILENO, dup2(save_stdin, STDIN_FILENO));
	stdin = fdopen(STDIN_FILENO, "r");
	TEST_ASSERT_NOT_NULL(stdin);

	/* note: not actually testing if stdin works */
}


TEST(file, fclose_stdin_ebadf)
{
	int save_stdin = dup(STDIN_FILENO);
	TEST_ASSERT_GREATER_THAN(-1, save_stdin);

	TEST_ASSERT_EQUAL_INT(0, close(STDIN_FILENO));
	TEST_ASSERT_EQUAL_INT(-1, fclose(stdin));
	TEST_ASSERT_EQUAL_INT(EBADF, errno);

	/* reopen the stdin */
	TEST_ASSERT_EQUAL_INT(STDIN_FILENO, dup2(save_stdin, STDIN_FILENO));

	/* note: not actually testing if stdin works */
}


TEST_GROUP_RUNNER(file)
{
	RUN_TEST_CASE(file, fclose_stdin);
	RUN_TEST_CASE(file, fclose_stdin_ebadf);
}
