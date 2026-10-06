/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <unistd.h>
 * TESTED:
 *    - fchown()
 *    - fdatasync()
 *    - lockf()
 *    - sync()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>

#include "unity_fixture.h"

#define MISC_TEST_FILE "/tmp/test_fileops_misc"

static struct {
	int fd;
} test_common;


TEST_GROUP(fileops_fchown);

TEST_SETUP(fileops_fchown)
{
	test_common.fd = -1;
	unlink(MISC_TEST_FILE);
	test_common.fd = open(MISC_TEST_FILE, O_RDWR | O_CREAT | O_TRUNC, 0644);
	TEST_ASSERT_TRUE(test_common.fd >= 0);
}

TEST_TEAR_DOWN(fileops_fchown)
{
	if (test_common.fd >= 0) {
		close(test_common.fd);
		test_common.fd = -1;
	}
	unlink(MISC_TEST_FILE);
}


TEST(fileops_fchown, fchown_no_change)
{
	int ret;

	/* Passing -1 for both owner and group means no change */
	ret = fchown(test_common.fd, (uid_t)-1, (gid_t)-1);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST(fileops_fchown, fchown_set_own_uid_gid)
{
	int ret;
	struct stat st;
	uid_t myUid;
	gid_t myGid;

	myUid = getuid();
	myGid = getgid();

	/* Setting to our own uid/gid should always succeed */
	ret = fchown(test_common.fd, myUid, myGid);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = fstat(test_common.fd, &st);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(myUid, st.st_uid);
	TEST_ASSERT_EQUAL_INT(myGid, st.st_gid);
}


TEST_GROUP_RUNNER(fileops_fchown)
{
	RUN_TEST_CASE(fileops_fchown, fchown_no_change);
	RUN_TEST_CASE(fileops_fchown, fchown_set_own_uid_gid);
}


TEST_GROUP(fileops_sync);

TEST_SETUP(fileops_sync) { }

TEST_TEAR_DOWN(fileops_sync) { }


TEST(fileops_sync, sync_does_not_crash)
{
	/* sync() returns void and has no defined errors; just verify it doesn't crash */
	sync();
}


TEST_GROUP_RUNNER(fileops_sync)
{
	RUN_TEST_CASE(fileops_sync, sync_does_not_crash);
}
