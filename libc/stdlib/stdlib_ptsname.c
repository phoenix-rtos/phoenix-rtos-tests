/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - stdlib.h
 * TESTED:
 *    - ptsname()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Lukasz Kruszynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define _XOPEN_SOURCE 600

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


/* Opens a master pseudo-terminal, the prerequisite for calling ptsname(). */
static int test_openMaster(void)
{
#ifdef __phoenix__
	return open(PTMX_PATH, O_RDWR | O_NOCTTY);
#else
	return posix_openpt(O_RDWR | O_NOCTTY);
#endif
}


TEST_GROUP(stdlib_ptsname);


TEST_SETUP(stdlib_ptsname)
{
	test_common.masterFd = -1;
	test_common.slaveFd = -1;
}


TEST_TEAR_DOWN(stdlib_ptsname)
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


TEST(stdlib_ptsname, ptsname_returns_slave_name)
{
	char *slaveName;

	/* A master pseudo-terminal is required to exercise ptsname(). */
	test_common.masterFd = test_openMaster();
#ifdef __phoenix__
	if (test_common.masterFd < 0) {
		TEST_IGNORE_MESSAGE("no pseudo-terminal device available to test");
	}
#endif
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_common.masterFd);

	TEST_ASSERT_EQUAL_INT(0, grantpt(test_common.masterFd));
	TEST_ASSERT_EQUAL_INT(0, unlockpt(test_common.masterFd));

	/* On success ptsname() shall return a pointer to the slave device name. */
	slaveName = ptsname(test_common.masterFd);
	TEST_ASSERT_NOT_NULL(slaveName);

	/* The returned string shall name the actual slave device: it is openable. */
	test_common.slaveFd = open(slaveName, O_RDWR | O_NOCTTY);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_common.slaveFd);

	/* The file the name refers to is a terminal. */
	TEST_ASSERT_EQUAL_INT(1, isatty(test_common.slaveFd));
}


TEST_GROUP_RUNNER(stdlib_ptsname)
{
	RUN_TEST_CASE(stdlib_ptsname, ptsname_returns_slave_name);
}
