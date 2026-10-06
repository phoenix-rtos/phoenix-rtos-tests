/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <termios.h>
 *    - <unistd.h>
 * TESTED:
 *    - tcdrain()
 *    - tcflow()
 *    - tcflush()
 *    - tcgetsid()
 *    - tcsetattr()
 *    - tcgetpgrp()
 *    - tcsetpgrp()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define _XOPEN_SOURCE 600

#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>

#include "unity_fixture.h"

#define TERMIOS_TEST_FILE "/tmp/test_termios_notty"

static struct {
	int masterFd;
	int slaveFd;
	int fileFd;
} test_common;


static void test_closePty(void)
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


TEST_GROUP(termios_tcdrain);

TEST_SETUP(termios_tcdrain)
{
	test_common.masterFd = -1;
	test_common.slaveFd = -1;
	test_common.fileFd = -1;
	unlink(TERMIOS_TEST_FILE);
}

TEST_TEAR_DOWN(termios_tcdrain)
{
	test_closePty();
	if (test_common.fileFd >= 0) {
		close(test_common.fileFd);
		test_common.fileFd = -1;
	}
	unlink(TERMIOS_TEST_FILE);
}


TEST(termios_tcdrain, tcdrain_ebadf_invalid)
{
	int ret;

	errno = 0;
	ret = tcdrain(-1);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


TEST_GROUP_RUNNER(termios_tcdrain)
{
	RUN_TEST_CASE(termios_tcdrain, tcdrain_ebadf_invalid);
}


TEST_GROUP(termios_tcflush);

TEST_SETUP(termios_tcflush)
{
	test_common.masterFd = -1;
	test_common.slaveFd = -1;
	test_common.fileFd = -1;
	unlink(TERMIOS_TEST_FILE);
}

TEST_TEAR_DOWN(termios_tcflush)
{
	test_closePty();
	if (test_common.fileFd >= 0) {
		close(test_common.fileFd);
		test_common.fileFd = -1;
	}
	unlink(TERMIOS_TEST_FILE);
}


TEST(termios_tcflush, tcflush_ebadf)
{
	int ret;

	errno = 0;
	ret = tcflush(-1, TCIFLUSH);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


TEST_GROUP_RUNNER(termios_tcflush)
{
	RUN_TEST_CASE(termios_tcflush, tcflush_ebadf);
}


TEST_GROUP(termios_tcgetsid);

TEST_SETUP(termios_tcgetsid)
{
	test_common.masterFd = -1;
	test_common.slaveFd = -1;
	test_common.fileFd = -1;
	unlink(TERMIOS_TEST_FILE);
}

TEST_TEAR_DOWN(termios_tcgetsid)
{
	test_closePty();
	if (test_common.fileFd >= 0) {
		close(test_common.fileFd);
		test_common.fileFd = -1;
	}
	unlink(TERMIOS_TEST_FILE);
}


TEST(termios_tcgetsid, tcgetsid_ebadf)
{
	pid_t ret;

	errno = 0;
	ret = tcgetsid(-1);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


TEST_GROUP_RUNNER(termios_tcgetsid)
{
	RUN_TEST_CASE(termios_tcgetsid, tcgetsid_ebadf);
}


TEST_GROUP(termios_tcsetattr);

TEST_SETUP(termios_tcsetattr)
{
	test_common.masterFd = -1;
	test_common.slaveFd = -1;
	test_common.fileFd = -1;
	unlink(TERMIOS_TEST_FILE);
}

TEST_TEAR_DOWN(termios_tcsetattr)
{
	test_closePty();
	if (test_common.fileFd >= 0) {
		close(test_common.fileFd);
		test_common.fileFd = -1;
	}
	unlink(TERMIOS_TEST_FILE);
}


TEST(termios_tcsetattr, tcsetattr_ebadf)
{
	int ret;
	struct termios term;

	memset(&term, 0, sizeof(term));

	errno = 0;
	ret = tcsetattr(-1, TCSANOW, &term);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


TEST_GROUP_RUNNER(termios_tcsetattr)
{
	RUN_TEST_CASE(termios_tcsetattr, tcsetattr_ebadf);
}


TEST_GROUP(termios_tcgetpgrp);

TEST_SETUP(termios_tcgetpgrp)
{
	test_common.masterFd = -1;
	test_common.slaveFd = -1;
	test_common.fileFd = -1;
	unlink(TERMIOS_TEST_FILE);
}

TEST_TEAR_DOWN(termios_tcgetpgrp)
{
	test_closePty();
	if (test_common.fileFd >= 0) {
		close(test_common.fileFd);
		test_common.fileFd = -1;
	}
	unlink(TERMIOS_TEST_FILE);
}


TEST(termios_tcgetpgrp, tcgetpgrp_ebadf)
{
	pid_t ret;

	errno = 0;
	ret = tcgetpgrp(-1);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


TEST_GROUP_RUNNER(termios_tcgetpgrp)
{
	RUN_TEST_CASE(termios_tcgetpgrp, tcgetpgrp_ebadf);
}


TEST_GROUP(termios_tcsetpgrp);

TEST_SETUP(termios_tcsetpgrp)
{
	test_common.masterFd = -1;
	test_common.slaveFd = -1;
	test_common.fileFd = -1;
	unlink(TERMIOS_TEST_FILE);
}

TEST_TEAR_DOWN(termios_tcsetpgrp)
{
	test_closePty();
	if (test_common.fileFd >= 0) {
		close(test_common.fileFd);
		test_common.fileFd = -1;
	}
	unlink(TERMIOS_TEST_FILE);
}


TEST(termios_tcsetpgrp, tcsetpgrp_ebadf)
{
	int ret;

	errno = 0;
	ret = tcsetpgrp(-1, getpgrp());
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


TEST_GROUP_RUNNER(termios_tcsetpgrp)
{
	RUN_TEST_CASE(termios_tcsetpgrp, tcsetpgrp_ebadf);
}
