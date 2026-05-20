/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - fcntl.h
 * TESTED:
 *    - creat()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>

#include "common.h"
#include "unity_fixture.h"

#define CREAT_TEST_FILE    "/tmp/test_creat_file"
#define CREAT_TEST_SYMLINK "/tmp/test_creat_symlink"
#define CREAT_TEST_DIR     "/tmp/test_creat_dir"
#define CREAT_TEST_DATA    "existing data"


static struct {
	int fd;
} test_common;


TEST_GROUP(fcntl_creat);


TEST_SETUP(fcntl_creat)
{
	unlink(CREAT_TEST_FILE);
	unlink(CREAT_TEST_SYMLINK);
	rmdir(CREAT_TEST_DIR);

	test_common.fd = -1;
}


TEST_TEAR_DOWN(fcntl_creat)
{
	if (test_common.fd >= 0) {
		close(test_common.fd);
	}
	unlink(CREAT_TEST_FILE);
	unlink(CREAT_TEST_SYMLINK);
	rmdir(CREAT_TEST_DIR);
}


TEST(fcntl_creat, creat_new_file)
{
	struct stat st;
	int ret;

	test_common.fd = creat(CREAT_TEST_FILE, 0644);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_common.fd);

	ret = fstat(test_common.fd, &st);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_TRUE(S_ISREG(st.st_mode));
}


TEST(fcntl_creat, creat_applies_mode_with_umask)
{
	struct stat st;
	mode_t prevMask;
	int ret;

	prevMask = umask(0027);

	test_common.fd = creat(CREAT_TEST_FILE, 0666);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_common.fd);

	ret = fstat(test_common.fd, &st);
	TEST_ASSERT_EQUAL_INT(0, ret);
	/* 0666 & ~0027 = 0640 */
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1628 issue");
#else
	TEST_ASSERT_EQUAL_INT(0640, (int)(st.st_mode & 0777));
#endif

	umask(prevMask);
}


TEST(fcntl_creat, creat_truncates_existing)
{
	struct stat st;
	int ret;

	ret = _create_file(CREAT_TEST_FILE, CREAT_TEST_DATA);
	TEST_ASSERT_EQUAL_INT(0, ret);

	test_common.fd = creat(CREAT_TEST_FILE, 0644);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_common.fd);

	ret = fstat(test_common.fd, &st);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, (int)st.st_size);
}


TEST(fcntl_creat, creat_wronly)
{
	ssize_t n;
	char buf[16];
	const char *data = "test";

	test_common.fd = creat(CREAT_TEST_FILE, 0644);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_common.fd);

	/* write should succeed */
	n = write(test_common.fd, data, strlen(data));
	TEST_ASSERT_EQUAL_INT((int)strlen(data), (int)n);

	/* read should fail — fd is write-only */
	n = read(test_common.fd, buf, sizeof(buf));
	TEST_ASSERT_EQUAL_INT(-1, (int)n);
}


TEST(fcntl_creat, creat_return_value_nonnegative)
{
	test_common.fd = creat(CREAT_TEST_FILE, 0644);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_common.fd);
}


TEST(fcntl_creat, creat_failure_returns_minus_one)
{
	int fd;

	/* path prefix does not exist */
	errno = 0;
	fd = creat("/tmp/nonexistent_creat_dir/file", 0644);
	TEST_ASSERT_EQUAL_INT(-1, fd);
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);
}


TEST(fcntl_creat, creat_enoent_empty_path)
{
	int fd;

	errno = 0;
	fd = creat("", 0644);
	TEST_ASSERT_EQUAL_INT(-1, fd);
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);
}


TEST(fcntl_creat, creat_eisdir)
{
	int fd;
	int ret;

	ret = mkdir(CREAT_TEST_DIR, 0755);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* creat() implies O_WRONLY, so opening a dir should fail with EISDIR */
	errno = 0;
	fd = creat(CREAT_TEST_DIR, 0644);
	TEST_ASSERT_EQUAL_INT(-1, fd);
	TEST_ASSERT_EQUAL_INT(EISDIR, errno);
}


TEST(fcntl_creat, creat_enotdir_prefix)
{
	int fd;
	int ret;

	ret = _create_file(CREAT_TEST_FILE, CREAT_TEST_DATA);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* path prefix component is a regular file */
	errno = 0;
	fd = creat(CREAT_TEST_FILE "/child", 0644);
	TEST_ASSERT_EQUAL_INT(-1, fd);
	TEST_ASSERT_EQUAL_INT(ENOTDIR, errno);
}


TEST(fcntl_creat, creat_enametoolong)
{
	static char longName[NAME_MAX + 2];
	int fd;

	memset(longName, 'b', NAME_MAX + 1);
	longName[NAME_MAX + 1] = '\0';

	(void)fd;
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1258 issue");
#else
	errno = 0;
	fd = creat(longName, 0644);
	TEST_ASSERT_EQUAL_INT(-1, fd);
	TEST_ASSERT_EQUAL_INT(ENAMETOOLONG, errno);
#endif
}


TEST_GROUP_RUNNER(fcntl_creat)
{
	RUN_TEST_CASE(fcntl_creat, creat_new_file);
	RUN_TEST_CASE(fcntl_creat, creat_applies_mode_with_umask);
	RUN_TEST_CASE(fcntl_creat, creat_truncates_existing);
	RUN_TEST_CASE(fcntl_creat, creat_wronly);
	RUN_TEST_CASE(fcntl_creat, creat_return_value_nonnegative);
	RUN_TEST_CASE(fcntl_creat, creat_failure_returns_minus_one);
	RUN_TEST_CASE(fcntl_creat, creat_enoent_empty_path);
	RUN_TEST_CASE(fcntl_creat, creat_eisdir);
	RUN_TEST_CASE(fcntl_creat, creat_enotdir_prefix);
	RUN_TEST_CASE(fcntl_creat, creat_enametoolong);
}
