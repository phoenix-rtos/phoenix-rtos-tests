/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <sys/statvfs.h>
 * TESTED:
 *    - fstatvfs()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Lukasz Kruszynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <sys/statvfs.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

#include "unity_fixture.h"

#define FSTATVFS_TEST_FILE "/tmp/test_fstatvfs_file"
#define FSTATVFS_TEST_DIR  "/tmp/test_fstatvfs_dir"

static struct {
	int fd;
} test_common;

TEST_GROUP(statvfs_fstatvfs);

TEST_SETUP(statvfs_fstatvfs)
{
	unlink(FSTATVFS_TEST_FILE);
	rmdir(FSTATVFS_TEST_DIR);
	test_common.fd = -1;
}

TEST_TEAR_DOWN(statvfs_fstatvfs)
{
	if (test_common.fd >= 0) {
		close(test_common.fd);
		test_common.fd = -1;
	}
	unlink(FSTATVFS_TEST_FILE);
	rmdir(FSTATVFS_TEST_DIR);
}


TEST(statvfs_fstatvfs, fstatvfs_regular_file)
{
	struct statvfs buf;
	int ret;

	test_common.fd = open(FSTATVFS_TEST_FILE, O_CREAT | O_WRONLY, 0644);
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.fd);

	memset(&buf, 0, sizeof(buf));
	ret = fstatvfs(test_common.fd, &buf);
	TEST_ASSERT_EQUAL_INT(0, ret);

	TEST_ASSERT_GREATER_THAN_UINT(0U, buf.f_bsize);
	TEST_ASSERT_GREATER_THAN_UINT(0U, buf.f_frsize);
}


TEST(statvfs_fstatvfs, fstatvfs_directory_fd)
{
	struct statvfs buf;
	int ret;

	ret = mkdir(FSTATVFS_TEST_DIR, 0755);
	TEST_ASSERT_EQUAL_INT(0, ret);

	test_common.fd = open(FSTATVFS_TEST_DIR, O_RDONLY);
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.fd);

	memset(&buf, 0, sizeof(buf));
	ret = fstatvfs(test_common.fd, &buf);
	TEST_ASSERT_EQUAL_INT(0, ret);

	TEST_ASSERT_GREATER_THAN_UINT(0U, buf.f_bsize);
}


TEST(statvfs_fstatvfs, fstatvfs_readonly_fd)
{
	struct statvfs buf;
	int ret;

	test_common.fd = open(FSTATVFS_TEST_FILE, O_CREAT | O_WRONLY, 0644);
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.fd);
	close(test_common.fd);

	/* Reopen read-only — fstatvfs does not require write permission */
	test_common.fd = open(FSTATVFS_TEST_FILE, O_RDONLY);
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.fd);

	memset(&buf, 0, sizeof(buf));
	ret = fstatvfs(test_common.fd, &buf);
	TEST_ASSERT_EQUAL_INT(0, ret);

	TEST_ASSERT_GREATER_THAN_UINT(0U, buf.f_bsize);
}


TEST(statvfs_fstatvfs, fstatvfs_struct_fields_consistent)
{
	struct statvfs buf;
	int ret;

	test_common.fd = open("/", O_RDONLY);
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.fd);

	memset(&buf, 0, sizeof(buf));
	ret = fstatvfs(test_common.fd, &buf);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* namemax should be positive */
	TEST_ASSERT_GREATER_THAN_UINT(0U, buf.f_namemax);
}


TEST(statvfs_fstatvfs, fstatvfs_matches_statvfs)
{
	struct statvfs bufFd;
	struct statvfs bufPath;
	int ret;

	test_common.fd = open(FSTATVFS_TEST_FILE, O_CREAT | O_WRONLY, 0644);
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.fd);

	memset(&bufFd, 0, sizeof(bufFd));
	ret = fstatvfs(test_common.fd, &bufFd);
	TEST_ASSERT_EQUAL_INT(0, ret);

	memset(&bufPath, 0, sizeof(bufPath));
	ret = statvfs(FSTATVFS_TEST_FILE, &bufPath);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Same file — both calls must report identical fs parameters */
	TEST_ASSERT_EQUAL_UINT(bufPath.f_bsize, bufFd.f_bsize);
	TEST_ASSERT_EQUAL_UINT(bufPath.f_frsize, bufFd.f_frsize);
	TEST_ASSERT_EQUAL_UINT(bufPath.f_fsid, bufFd.f_fsid);
	TEST_ASSERT_EQUAL_UINT(bufPath.f_namemax, bufFd.f_namemax);
}


TEST(statvfs_fstatvfs, fstatvfs_ebadf_invalid_fd)
{
	struct statvfs buf;
	int ret;

	errno = 0;
	ret = fstatvfs(-1, &buf);
	TEST_ASSERT_EQUAL_INT(-1, ret);
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1632 issue");
#else
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
#endif
}


TEST(statvfs_fstatvfs, fstatvfs_ebadf_closed_fd)
{
	struct statvfs buf;
	int ret;
	int closedFd;

	closedFd = open(FSTATVFS_TEST_FILE, O_CREAT | O_WRONLY, 0644);
	TEST_ASSERT_NOT_EQUAL_INT(-1, closedFd);
	close(closedFd);

	errno = 0;
	ret = fstatvfs(closedFd, &buf);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


TEST(statvfs_fstatvfs, fstatvfs_ebadf_large_fd)
{
	struct statvfs buf;
	int ret;

	errno = 0;
	ret = fstatvfs(99999, &buf);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


TEST_GROUP_RUNNER(statvfs_fstatvfs)
{
	RUN_TEST_CASE(statvfs_fstatvfs, fstatvfs_regular_file);
	RUN_TEST_CASE(statvfs_fstatvfs, fstatvfs_directory_fd);
	RUN_TEST_CASE(statvfs_fstatvfs, fstatvfs_readonly_fd);
	RUN_TEST_CASE(statvfs_fstatvfs, fstatvfs_struct_fields_consistent);
	RUN_TEST_CASE(statvfs_fstatvfs, fstatvfs_matches_statvfs);
	RUN_TEST_CASE(statvfs_fstatvfs, fstatvfs_ebadf_invalid_fd);
	RUN_TEST_CASE(statvfs_fstatvfs, fstatvfs_ebadf_closed_fd);
	RUN_TEST_CASE(statvfs_fstatvfs, fstatvfs_ebadf_large_fd);
}
