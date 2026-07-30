/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - stdlib.h
 * TESTED:
 *    - unlockpt()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Lukasz Kruszynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define _XOPEN_SOURCE 700

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <unity_fixture.h>

/* Pseudo-terminal multiplexer device, opened when posix_openpt() is absent. */
#define PTMX_PATH "/dev/ptmx"


static struct {
	int masterFd;
	int slaveFd;
} test_common;


/* Opens a master pseudo-terminal, the prerequisite for calling unlockpt(). */
static int test_openMaster(void)
{
#ifdef __phoenix__
	return open(PTMX_PATH, O_RDWR | O_NOCTTY);
#else
	return posix_openpt(O_RDWR | O_NOCTTY);
#endif
}


TEST_GROUP(stdlib_unlockpt);


TEST_SETUP(stdlib_unlockpt)
{
	test_common.masterFd = -1;
	test_common.slaveFd = -1;
}


TEST_TEAR_DOWN(stdlib_unlockpt)
{
	if (test_common.slaveFd >= 0) {
		close(test_common.slaveFd);
		test_common.slaveFd = -1;
	}
	if (test_common.masterFd >= 0) {
		close(test_common.masterFd);
		test_common.masterFd = -1;
	}
}


/* unlockpt() shall return 0 upon successful completion. */
TEST(stdlib_unlockpt, returns_zero_on_success)
{
	test_common.masterFd = test_openMaster();
#ifdef __phoenix__
	if (test_common.masterFd < 0) {
		TEST_IGNORE_MESSAGE("no pseudo-terminal device available to test");
	}
#endif
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_common.masterFd);

	TEST_ASSERT_EQUAL_INT(0, unlockpt(test_common.masterFd));
}


/*
 * unlockpt() shall unlock the slave device associated with the master, so that
 * after the call the slave side can be opened.
 */
TEST(stdlib_unlockpt, unlocks_slave_for_open)
{
	char *slaveName;

	test_common.masterFd = test_openMaster();
#ifdef __phoenix__
	if (test_common.masterFd < 0) {
		TEST_IGNORE_MESSAGE("no pseudo-terminal device available to test");
	}
#endif
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_common.masterFd);

	TEST_ASSERT_EQUAL_INT(0, grantpt(test_common.masterFd));
	TEST_ASSERT_EQUAL_INT(0, unlockpt(test_common.masterFd));

	/* With the slave unlocked, opening it shall succeed and yield a terminal. */
	slaveName = ptsname(test_common.masterFd);
	TEST_ASSERT_NOT_NULL(slaveName);

	test_common.slaveFd = open(slaveName, O_RDWR | O_NOCTTY);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_common.slaveFd);
	TEST_ASSERT_EQUAL_INT(1, isatty(test_common.slaveFd));
}


TEST_GROUP_RUNNER(stdlib_unlockpt)
{
	RUN_TEST_CASE(stdlib_unlockpt, returns_zero_on_success);
	RUN_TEST_CASE(stdlib_unlockpt, unlocks_slave_for_open);
}
