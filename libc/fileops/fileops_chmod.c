/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <sys/stat.h>
 *    - <fcntl.h>
 * TESTED:
 *    - fchmod()
 *    - fchmodat()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "unity_fixture.h"

#define FCHMOD_TEST_FILE "/tmp/test_fchmod_file"
#define FCHMOD_TEST_DIR  "/tmp/test_fchmod_dir"

#ifndef __phoenix__
static struct {
	int fd;
	int dirFd;
} test_common;
#endif


/* ========================================================================= */
/* fchmod */
/* ========================================================================= */

#ifndef __phoenix__

TEST_GROUP(fileops_fchmod);

TEST_SETUP(fileops_fchmod)
{
	test_common.fd = -1;
	unlink(FCHMOD_TEST_FILE);
	test_common.fd = open(FCHMOD_TEST_FILE, O_RDWR | O_CREAT | O_TRUNC, 0644);
	TEST_ASSERT_TRUE(test_common.fd >= 0);
}

TEST_TEAR_DOWN(fileops_fchmod)
{
	if (test_common.fd >= 0) {
		close(test_common.fd);
		test_common.fd = -1;
	}
	unlink(FCHMOD_TEST_FILE);
}


TEST(fileops_fchmod, fchmod_set_permissions)
{
	int ret;
	struct stat st;

	ret = fchmod(test_common.fd, 0755);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = fstat(test_common.fd, &st);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0755, st.st_mode & 0777);
}


TEST(fileops_fchmod, fchmod_remove_all_permissions)
{
	int ret;
	struct stat st;

	ret = fchmod(test_common.fd, 0000);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = fstat(test_common.fd, &st);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0000, st.st_mode & 0777);

	/* Restore for cleanup */
	fchmod(test_common.fd, 0644);
}


TEST(fileops_fchmod, fchmod_readonly)
{
	int ret;
	struct stat st;

	ret = fchmod(test_common.fd, 0444);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = fstat(test_common.fd, &st);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0444, st.st_mode & 0777);
}


TEST(fileops_fchmod, fchmod_ebadf)
{
	int ret;

	errno = 0;
	ret = fchmod(-1, 0644);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


TEST(fileops_fchmod, fchmod_returns_zero_on_success)
{
	int ret;

	ret = fchmod(test_common.fd, 0600);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST_GROUP_RUNNER(fileops_fchmod)
{
	RUN_TEST_CASE(fileops_fchmod, fchmod_set_permissions);
	RUN_TEST_CASE(fileops_fchmod, fchmod_remove_all_permissions);
	RUN_TEST_CASE(fileops_fchmod, fchmod_readonly);
	RUN_TEST_CASE(fileops_fchmod, fchmod_ebadf);
	RUN_TEST_CASE(fileops_fchmod, fchmod_returns_zero_on_success);
}
#else
TEST_GROUP_UNIMPLEMENTED(fileops_fchmod, "fchmod only stubbed not implemented")
#endif


/* ========================================================================= */
/* fchmodat */
/* ========================================================================= */

#ifndef __phoenix__

TEST_GROUP(fileops_fchmodat);

TEST_SETUP(fileops_fchmodat)
{
	int ret;

	test_common.fd = -1;
	test_common.dirFd = -1;
	unlink(FCHMOD_TEST_FILE);

	test_common.fd = open(FCHMOD_TEST_FILE, O_RDWR | O_CREAT | O_TRUNC, 0644);
	TEST_ASSERT_TRUE(test_common.fd >= 0);

	test_common.dirFd = open("/tmp", O_RDONLY | O_DIRECTORY);
	TEST_ASSERT_TRUE(test_common.dirFd >= 0);

	/* Verify the file was created with expected name */
	ret = fchmodat(test_common.dirFd, "test_fchmod_file", 0644, 0);
	TEST_ASSERT_EQUAL_INT(0, ret);
}

TEST_TEAR_DOWN(fileops_fchmodat)
{
	if (test_common.fd >= 0) {
		close(test_common.fd);
		test_common.fd = -1;
	}
	if (test_common.dirFd >= 0) {
		close(test_common.dirFd);
		test_common.dirFd = -1;
	}
	unlink(FCHMOD_TEST_FILE);
}


TEST(fileops_fchmodat, fchmodat_relative_path)
{
	int ret;
	struct stat st;

	ret = fchmodat(test_common.dirFd, "test_fchmod_file", 0750, 0);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = fstat(test_common.fd, &st);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0750, st.st_mode & 0777);
}


TEST(fileops_fchmodat, fchmodat_at_fdcwd)
{
	int ret;
	struct stat st;

	ret = fchmodat(AT_FDCWD, FCHMOD_TEST_FILE, 0711, 0);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = fstat(test_common.fd, &st);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0711, st.st_mode & 0777);
}


TEST(fileops_fchmodat, fchmodat_absolute_path)
{
	int ret;
	struct stat st;

	ret = fchmodat(test_common.dirFd, FCHMOD_TEST_FILE, 0600, 0);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = fstat(test_common.fd, &st);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0600, st.st_mode & 0777);
}


TEST(fileops_fchmodat, fchmodat_enoent)
{
	int ret;

	errno = 0;
	ret = fchmodat(test_common.dirFd, "nonexistent_fchmodat_file", 0644, 0);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);
}


TEST(fileops_fchmodat, fchmodat_ebadf_invalid_fd)
{
	int ret;

	errno = 0;
	ret = fchmodat(-1, "test_fchmod_file", 0644, 0);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


TEST(fileops_fchmodat, fchmodat_enotdir_fd_not_dir)
{
	int ret;

	/* Use a regular file fd instead of a directory fd */
	errno = 0;
	ret = fchmodat(test_common.fd, "test_fchmod_file", 0644, 0);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(ENOTDIR, errno);
}


TEST_GROUP_RUNNER(fileops_fchmodat)
{
	RUN_TEST_CASE(fileops_fchmodat, fchmodat_relative_path);
	RUN_TEST_CASE(fileops_fchmodat, fchmodat_at_fdcwd);
	RUN_TEST_CASE(fileops_fchmodat, fchmodat_absolute_path);
	RUN_TEST_CASE(fileops_fchmodat, fchmodat_enoent);
	RUN_TEST_CASE(fileops_fchmodat, fchmodat_ebadf_invalid_fd);
	RUN_TEST_CASE(fileops_fchmodat, fchmodat_enotdir_fd_not_dir);
}
#else
TEST_GROUP_UNIMPLEMENTED(fileops_fchmodat, "fchmodat not implemented")
#endif
