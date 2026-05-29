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


static void test_openPty(void)
{
	int ret;
	char slaveName[256];

	test_common.masterFd = posix_openpt(O_RDWR | O_NOCTTY);
	TEST_ASSERT_TRUE(test_common.masterFd >= 0);

	ret = grantpt(test_common.masterFd);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = unlockpt(test_common.masterFd);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = ptsname_r(test_common.masterFd, slaveName, sizeof(slaveName));
	TEST_ASSERT_EQUAL_INT(0, ret);

	test_common.slaveFd = open(slaveName, O_RDWR | O_NOCTTY);
	TEST_ASSERT_TRUE(test_common.slaveFd >= 0);
}


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


/* ========================================================================= */
/* tcdrain */
/* ========================================================================= */

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


TEST(termios_tcdrain, tcdrain_success_pty)
{
	int ret;

	test_openPty();

	ret = tcdrain(test_common.slaveFd);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST(termios_tcdrain, tcdrain_ebadf_invalid)
{
	int ret;

	errno = 0;
	ret = tcdrain(-1);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


TEST(termios_tcdrain, tcdrain_enotty_regular_file)
{
	int ret;

	test_common.fileFd = open(TERMIOS_TEST_FILE, O_RDWR | O_CREAT | O_TRUNC, 0666);
	TEST_ASSERT_TRUE(test_common.fileFd >= 0);

	errno = 0;
	ret = tcdrain(test_common.fileFd);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(ENOTTY, errno);
}


TEST_GROUP_RUNNER(termios_tcdrain)
{
	RUN_TEST_CASE(termios_tcdrain, tcdrain_success_pty);
	RUN_TEST_CASE(termios_tcdrain, tcdrain_ebadf_invalid);
	RUN_TEST_CASE(termios_tcdrain, tcdrain_enotty_regular_file);
}


/* ========================================================================= */
/* tcflow */
/* ========================================================================= */

TEST_GROUP(termios_tcflow);

TEST_SETUP(termios_tcflow)
{
	test_common.masterFd = -1;
	test_common.slaveFd = -1;
	test_common.fileFd = -1;
	unlink(TERMIOS_TEST_FILE);
}

TEST_TEAR_DOWN(termios_tcflow)
{
	test_closePty();
	if (test_common.fileFd >= 0) {
		close(test_common.fileFd);
		test_common.fileFd = -1;
	}
	unlink(TERMIOS_TEST_FILE);
}


TEST(termios_tcflow, tcflow_tcooff)
{
	int ret;

	test_openPty();

	ret = tcflow(test_common.slaveFd, TCOOFF);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Restore */
	ret = tcflow(test_common.slaveFd, TCOON);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST(termios_tcflow, tcflow_tcoon)
{
	int ret;

	test_openPty();

	ret = tcflow(test_common.slaveFd, TCOON);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST(termios_tcflow, tcflow_tcioff)
{
	int ret;

	test_openPty();

	ret = tcflow(test_common.slaveFd, TCIOFF);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST(termios_tcflow, tcflow_tcion)
{
	int ret;

	test_openPty();

	ret = tcflow(test_common.slaveFd, TCION);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST(termios_tcflow, tcflow_ebadf)
{
	int ret;

	errno = 0;
	ret = tcflow(-1, TCOOFF);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


TEST(termios_tcflow, tcflow_einval_bad_action)
{
	int ret;

	test_openPty();

	errno = 0;
	ret = tcflow(test_common.slaveFd, 9999);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST(termios_tcflow, tcflow_enotty_regular_file)
{
	int ret;

	test_common.fileFd = open(TERMIOS_TEST_FILE, O_RDWR | O_CREAT | O_TRUNC, 0666);
	TEST_ASSERT_TRUE(test_common.fileFd >= 0);

	errno = 0;
	ret = tcflow(test_common.fileFd, TCOOFF);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(ENOTTY, errno);
}


TEST_GROUP_RUNNER(termios_tcflow)
{
	RUN_TEST_CASE(termios_tcflow, tcflow_tcooff);
	RUN_TEST_CASE(termios_tcflow, tcflow_tcoon);
	RUN_TEST_CASE(termios_tcflow, tcflow_tcioff);
	RUN_TEST_CASE(termios_tcflow, tcflow_tcion);
	RUN_TEST_CASE(termios_tcflow, tcflow_ebadf);
	RUN_TEST_CASE(termios_tcflow, tcflow_einval_bad_action);
	RUN_TEST_CASE(termios_tcflow, tcflow_enotty_regular_file);
}


/* ========================================================================= */
/* tcflush */
/* ========================================================================= */

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


TEST(termios_tcflush, tcflush_tciflush)
{
	int ret;

	test_openPty();

	ret = tcflush(test_common.slaveFd, TCIFLUSH);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST(termios_tcflush, tcflush_tcoflush)
{
	int ret;

	test_openPty();

	ret = tcflush(test_common.slaveFd, TCOFLUSH);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST(termios_tcflush, tcflush_tcioflush)
{
	int ret;

	test_openPty();

	ret = tcflush(test_common.slaveFd, TCIOFLUSH);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST(termios_tcflush, tcflush_ebadf)
{
	int ret;

	errno = 0;
	ret = tcflush(-1, TCIFLUSH);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


TEST(termios_tcflush, tcflush_einval_bad_selector)
{
	int ret;

	test_openPty();

	errno = 0;
	ret = tcflush(test_common.slaveFd, 9999);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST(termios_tcflush, tcflush_enotty_regular_file)
{
	int ret;

	test_common.fileFd = open(TERMIOS_TEST_FILE, O_RDWR | O_CREAT | O_TRUNC, 0666);
	TEST_ASSERT_TRUE(test_common.fileFd >= 0);

	errno = 0;
	ret = tcflush(test_common.fileFd, TCIFLUSH);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(ENOTTY, errno);
}


TEST_GROUP_RUNNER(termios_tcflush)
{
	RUN_TEST_CASE(termios_tcflush, tcflush_tciflush);
	RUN_TEST_CASE(termios_tcflush, tcflush_tcoflush);
	RUN_TEST_CASE(termios_tcflush, tcflush_tcioflush);
	RUN_TEST_CASE(termios_tcflush, tcflush_ebadf);
	RUN_TEST_CASE(termios_tcflush, tcflush_einval_bad_selector);
	RUN_TEST_CASE(termios_tcflush, tcflush_enotty_regular_file);
}


/* ========================================================================= */
/* tcgetsid */
/* ========================================================================= */

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


TEST(termios_tcgetsid, tcgetsid_enotty_regular_file)
{
	pid_t ret;

	test_common.fileFd = open(TERMIOS_TEST_FILE, O_RDWR | O_CREAT | O_TRUNC, 0666);
	TEST_ASSERT_TRUE(test_common.fileFd >= 0);

	errno = 0;
	ret = tcgetsid(test_common.fileFd);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(ENOTTY, errno);
}


TEST(termios_tcgetsid, tcgetsid_enotty_noctty_pty)
{
	pid_t ret;

	/* PTY opened with O_NOCTTY is not the controlling terminal */
	test_openPty();

	errno = 0;
	ret = tcgetsid(test_common.slaveFd);
	/* This should fail with ENOTTY because the pty is not our controlling terminal */
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(ENOTTY, errno);
}


TEST_GROUP_RUNNER(termios_tcgetsid)
{
	RUN_TEST_CASE(termios_tcgetsid, tcgetsid_ebadf);
	RUN_TEST_CASE(termios_tcgetsid, tcgetsid_enotty_regular_file);
	RUN_TEST_CASE(termios_tcgetsid, tcgetsid_enotty_noctty_pty);
}


/* ========================================================================= */
/* tcsetattr */
/* ========================================================================= */

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


TEST(termios_tcsetattr, tcsetattr_tcsanow)
{
	int ret;
	struct termios origTerm;
	struct termios newTerm;

	test_openPty();

	ret = tcgetattr(test_common.slaveFd, &origTerm);
	TEST_ASSERT_EQUAL_INT(0, ret);

	memcpy(&newTerm, &origTerm, sizeof(newTerm));
	newTerm.c_lflag &= ~(tcflag_t)ECHO;

	ret = tcsetattr(test_common.slaveFd, TCSANOW, &newTerm);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Verify the change took effect */
	ret = tcgetattr(test_common.slaveFd, &newTerm);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_FALSE(newTerm.c_lflag & ECHO);

	/* Restore */
	ret = tcsetattr(test_common.slaveFd, TCSANOW, &origTerm);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST(termios_tcsetattr, tcsetattr_tcsadrain)
{
	int ret;
	struct termios origTerm;
	struct termios newTerm;

	test_openPty();

	ret = tcgetattr(test_common.slaveFd, &origTerm);
	TEST_ASSERT_EQUAL_INT(0, ret);

	memcpy(&newTerm, &origTerm, sizeof(newTerm));
	newTerm.c_lflag &= ~(tcflag_t)ICANON;

	ret = tcsetattr(test_common.slaveFd, TCSADRAIN, &newTerm);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = tcgetattr(test_common.slaveFd, &newTerm);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_FALSE(newTerm.c_lflag & ICANON);

	ret = tcsetattr(test_common.slaveFd, TCSANOW, &origTerm);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST(termios_tcsetattr, tcsetattr_tcsaflush)
{
	int ret;
	struct termios origTerm;
	struct termios newTerm;

	test_openPty();

	ret = tcgetattr(test_common.slaveFd, &origTerm);
	TEST_ASSERT_EQUAL_INT(0, ret);

	memcpy(&newTerm, &origTerm, sizeof(newTerm));
	newTerm.c_lflag &= ~(tcflag_t)ISIG;

	ret = tcsetattr(test_common.slaveFd, TCSAFLUSH, &newTerm);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = tcgetattr(test_common.slaveFd, &newTerm);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_FALSE(newTerm.c_lflag & ISIG);

	ret = tcsetattr(test_common.slaveFd, TCSANOW, &origTerm);
	TEST_ASSERT_EQUAL_INT(0, ret);
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


TEST(termios_tcsetattr, tcsetattr_einval_bad_action)
{
	int ret;
	struct termios term;

	test_openPty();

	ret = tcgetattr(test_common.slaveFd, &term);
	TEST_ASSERT_EQUAL_INT(0, ret);

	errno = 0;
	ret = tcsetattr(test_common.slaveFd, 9999, &term);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST(termios_tcsetattr, tcsetattr_enotty_regular_file)
{
	int ret;
	struct termios term;

	test_common.fileFd = open(TERMIOS_TEST_FILE, O_RDWR | O_CREAT | O_TRUNC, 0666);
	TEST_ASSERT_TRUE(test_common.fileFd >= 0);

	memset(&term, 0, sizeof(term));

	errno = 0;
	ret = tcsetattr(test_common.fileFd, TCSANOW, &term);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(ENOTTY, errno);
}


TEST(termios_tcsetattr, tcsetattr_preserves_struct)
{
	int ret;
	struct termios origTerm;
	struct termios modTerm;
	struct termios savedCopy;

	test_openPty();

	ret = tcgetattr(test_common.slaveFd, &origTerm);
	TEST_ASSERT_EQUAL_INT(0, ret);

	memcpy(&modTerm, &origTerm, sizeof(modTerm));
	modTerm.c_lflag &= ~(tcflag_t)ECHO;
	memcpy(&savedCopy, &modTerm, sizeof(savedCopy));

	ret = tcsetattr(test_common.slaveFd, TCSANOW, &modTerm);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* tcsetattr shall not change the values in the termios structure */
	TEST_ASSERT_EQUAL_MEMORY(&savedCopy, &modTerm, sizeof(struct termios));

	ret = tcsetattr(test_common.slaveFd, TCSANOW, &origTerm);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST_GROUP_RUNNER(termios_tcsetattr)
{
	RUN_TEST_CASE(termios_tcsetattr, tcsetattr_tcsanow);
	RUN_TEST_CASE(termios_tcsetattr, tcsetattr_tcsadrain);
	RUN_TEST_CASE(termios_tcsetattr, tcsetattr_tcsaflush);
	RUN_TEST_CASE(termios_tcsetattr, tcsetattr_ebadf);
	RUN_TEST_CASE(termios_tcsetattr, tcsetattr_einval_bad_action);
	RUN_TEST_CASE(termios_tcsetattr, tcsetattr_enotty_regular_file);
	RUN_TEST_CASE(termios_tcsetattr, tcsetattr_preserves_struct);
}


/* ========================================================================= */
/* tcgetpgrp */
/* ========================================================================= */

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


TEST(termios_tcgetpgrp, tcgetpgrp_enotty_regular_file)
{
	pid_t ret;

	test_common.fileFd = open(TERMIOS_TEST_FILE, O_RDWR | O_CREAT | O_TRUNC, 0666);
	TEST_ASSERT_TRUE(test_common.fileFd >= 0);

	errno = 0;
	ret = tcgetpgrp(test_common.fileFd);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(ENOTTY, errno);
}


TEST(termios_tcgetpgrp, tcgetpgrp_enotty_noctty_pty)
{
	pid_t ret;

	test_openPty();

	errno = 0;
	ret = tcgetpgrp(test_common.slaveFd);
	/* PTY opened with O_NOCTTY is not controlling terminal */
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(ENOTTY, errno);
}


TEST_GROUP_RUNNER(termios_tcgetpgrp)
{
	RUN_TEST_CASE(termios_tcgetpgrp, tcgetpgrp_ebadf);
	RUN_TEST_CASE(termios_tcgetpgrp, tcgetpgrp_enotty_regular_file);
	RUN_TEST_CASE(termios_tcgetpgrp, tcgetpgrp_enotty_noctty_pty);
}


/* ========================================================================= */
/* tcsetpgrp */
/* ========================================================================= */

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


TEST(termios_tcsetpgrp, tcsetpgrp_enotty_regular_file)
{
	int ret;

	test_common.fileFd = open(TERMIOS_TEST_FILE, O_RDWR | O_CREAT | O_TRUNC, 0666);
	TEST_ASSERT_TRUE(test_common.fileFd >= 0);

	errno = 0;
	ret = tcsetpgrp(test_common.fileFd, getpgrp());
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(ENOTTY, errno);
}


TEST(termios_tcsetpgrp, tcsetpgrp_enotty_noctty_pty)
{
	int ret;

	test_openPty();

	errno = 0;
	ret = tcsetpgrp(test_common.slaveFd, getpgrp());
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(ENOTTY, errno);
}


TEST_GROUP_RUNNER(termios_tcsetpgrp)
{
	RUN_TEST_CASE(termios_tcsetpgrp, tcsetpgrp_ebadf);
	RUN_TEST_CASE(termios_tcsetpgrp, tcsetpgrp_enotty_regular_file);
	RUN_TEST_CASE(termios_tcsetpgrp, tcsetpgrp_enotty_noctty_pty);
}
