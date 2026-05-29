/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <sys/stat.h>
 *    - <fcntl.h>
 * TESTED:
 *    - futimens()
 *    - utimensat()
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
#include <time.h>

#include "unity_fixture.h"

#define FUTIMENS_TEST_FILE "/tmp/test_futimens_file"

static struct {
	int fd;
	int dirFd;
} test_common;


/* ========================================================================= */
/* futimens */
/* ========================================================================= */

TEST_GROUP(fileops_futimens);

TEST_SETUP(fileops_futimens)
{
	test_common.fd = -1;
	unlink(FUTIMENS_TEST_FILE);
	test_common.fd = open(FUTIMENS_TEST_FILE, O_RDWR | O_CREAT | O_TRUNC, 0644);
	TEST_ASSERT_TRUE(test_common.fd >= 0);
}

TEST_TEAR_DOWN(fileops_futimens)
{
	if (test_common.fd >= 0) {
		close(test_common.fd);
		test_common.fd = -1;
	}
	unlink(FUTIMENS_TEST_FILE);
}


TEST(fileops_futimens, futimens_set_specific_times)
{
	int ret;
	struct stat st;
	struct timespec times[2];

	times[0].tv_sec = 1000000;
	times[0].tv_nsec = 0;
	times[1].tv_sec = 2000000;
	times[1].tv_nsec = 0;

	ret = futimens(test_common.fd, times);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = fstat(test_common.fd, &st);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(1000000, st.st_atim.tv_sec);
	TEST_ASSERT_EQUAL_INT(2000000, st.st_mtim.tv_sec);
}


TEST(fileops_futimens, futimens_null_sets_current_time)
{
	int ret;
	struct stat st;
	time_t before;
	time_t after;

	before = time(NULL);
	TEST_ASSERT_TRUE(before != (time_t)-1);

	ret = futimens(test_common.fd, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	after = time(NULL);
	TEST_ASSERT_TRUE(after != (time_t)-1);

	ret = fstat(test_common.fd, &st);
	TEST_ASSERT_EQUAL_INT(0, ret);

	TEST_ASSERT_TRUE(st.st_atim.tv_sec >= before);
	TEST_ASSERT_TRUE(st.st_atim.tv_sec <= after);
	TEST_ASSERT_TRUE(st.st_mtim.tv_sec >= before);
	TEST_ASSERT_TRUE(st.st_mtim.tv_sec <= after);
}


TEST(fileops_futimens, futimens_utime_now)
{
	int ret;
	struct stat st;
	struct timespec times[2];
	time_t before;
	time_t after;

	before = time(NULL);

	times[0].tv_sec = 0;
	times[0].tv_nsec = UTIME_NOW;
	times[1].tv_sec = 0;
	times[1].tv_nsec = UTIME_NOW;

	ret = futimens(test_common.fd, times);
	TEST_ASSERT_EQUAL_INT(0, ret);

	after = time(NULL);

	ret = fstat(test_common.fd, &st);
	TEST_ASSERT_EQUAL_INT(0, ret);

	TEST_ASSERT_TRUE(st.st_atim.tv_sec >= before);
	TEST_ASSERT_TRUE(st.st_atim.tv_sec <= after);
	TEST_ASSERT_TRUE(st.st_mtim.tv_sec >= before);
	TEST_ASSERT_TRUE(st.st_mtim.tv_sec <= after);
}


TEST(fileops_futimens, futimens_utime_omit)
{
	int ret;
	struct stat stBefore;
	struct stat stAfter;
	struct timespec times[2];

	/* Set known times first */
	times[0].tv_sec = 500000;
	times[0].tv_nsec = 0;
	times[1].tv_sec = 600000;
	times[1].tv_nsec = 0;
	ret = futimens(test_common.fd, times);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = fstat(test_common.fd, &stBefore);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Now use UTIME_OMIT — times should not change */
	times[0].tv_sec = 0;
	times[0].tv_nsec = UTIME_OMIT;
	times[1].tv_sec = 0;
	times[1].tv_nsec = UTIME_OMIT;

	ret = futimens(test_common.fd, times);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = fstat(test_common.fd, &stAfter);
	TEST_ASSERT_EQUAL_INT(0, ret);

	TEST_ASSERT_EQUAL_INT(stBefore.st_atim.tv_sec, stAfter.st_atim.tv_sec);
	TEST_ASSERT_EQUAL_INT(stBefore.st_mtim.tv_sec, stAfter.st_mtim.tv_sec);
}


TEST(fileops_futimens, futimens_ebadf)
{
	int ret;
	struct timespec times[2];

	times[0].tv_sec = 100;
	times[0].tv_nsec = 0;
	times[1].tv_sec = 200;
	times[1].tv_nsec = 0;

	errno = 0;
	ret = futimens(-1, times);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


TEST(fileops_futimens, futimens_einval_bad_nsec)
{
	int ret;
	struct timespec times[2];

	times[0].tv_sec = 100;
	times[0].tv_nsec = -1;
	times[1].tv_sec = 200;
	times[1].tv_nsec = 0;

	errno = 0;
	ret = futimens(test_common.fd, times);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST(fileops_futimens, futimens_einval_nsec_too_large)
{
	int ret;
	struct timespec times[2];

	times[0].tv_sec = 100;
	times[0].tv_nsec = 0;
	times[1].tv_sec = 200;
	times[1].tv_nsec = 1000000000L;

	errno = 0;
	ret = futimens(test_common.fd, times);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST_GROUP_RUNNER(fileops_futimens)
{
	RUN_TEST_CASE(fileops_futimens, futimens_set_specific_times);
	RUN_TEST_CASE(fileops_futimens, futimens_null_sets_current_time);
	RUN_TEST_CASE(fileops_futimens, futimens_utime_now);
	RUN_TEST_CASE(fileops_futimens, futimens_utime_omit);
	RUN_TEST_CASE(fileops_futimens, futimens_ebadf);
	RUN_TEST_CASE(fileops_futimens, futimens_einval_bad_nsec);
	RUN_TEST_CASE(fileops_futimens, futimens_einval_nsec_too_large);
}


/* ========================================================================= */
/* utimensat */
/* ========================================================================= */

TEST_GROUP(fileops_utimensat);

TEST_SETUP(fileops_utimensat)
{
	test_common.fd = -1;
	test_common.dirFd = -1;
	unlink(FUTIMENS_TEST_FILE);

	test_common.fd = open(FUTIMENS_TEST_FILE, O_RDWR | O_CREAT | O_TRUNC, 0644);
	TEST_ASSERT_TRUE(test_common.fd >= 0);

	test_common.dirFd = open("/tmp", O_RDONLY | O_DIRECTORY);
	TEST_ASSERT_TRUE(test_common.dirFd >= 0);
}

TEST_TEAR_DOWN(fileops_utimensat)
{
	if (test_common.fd >= 0) {
		close(test_common.fd);
		test_common.fd = -1;
	}
	if (test_common.dirFd >= 0) {
		close(test_common.dirFd);
		test_common.dirFd = -1;
	}
	unlink(FUTIMENS_TEST_FILE);
}


TEST(fileops_utimensat, utimensat_relative_path)
{
	int ret;
	struct stat st;
	struct timespec times[2];

	times[0].tv_sec = 300000;
	times[0].tv_nsec = 0;
	times[1].tv_sec = 400000;
	times[1].tv_nsec = 0;

	ret = utimensat(test_common.dirFd, "test_futimens_file", times, 0);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = fstat(test_common.fd, &st);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(300000, st.st_atim.tv_sec);
	TEST_ASSERT_EQUAL_INT(400000, st.st_mtim.tv_sec);
}


TEST(fileops_utimensat, utimensat_at_fdcwd)
{
	int ret;
	struct stat st;
	struct timespec times[2];

	times[0].tv_sec = 111111;
	times[0].tv_nsec = 0;
	times[1].tv_sec = 222222;
	times[1].tv_nsec = 0;

	ret = utimensat(AT_FDCWD, FUTIMENS_TEST_FILE, times, 0);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = fstat(test_common.fd, &st);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(111111, st.st_atim.tv_sec);
	TEST_ASSERT_EQUAL_INT(222222, st.st_mtim.tv_sec);
}


TEST(fileops_utimensat, utimensat_null_times)
{
	int ret;
	struct stat st;
	time_t before;
	time_t after;

	before = time(NULL);

	ret = utimensat(test_common.dirFd, "test_futimens_file", NULL, 0);
	TEST_ASSERT_EQUAL_INT(0, ret);

	after = time(NULL);

	ret = fstat(test_common.fd, &st);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_TRUE(st.st_atim.tv_sec >= before);
	TEST_ASSERT_TRUE(st.st_atim.tv_sec <= after);
}


TEST(fileops_utimensat, utimensat_enoent)
{
	int ret;
	struct timespec times[2];

	times[0].tv_sec = 100;
	times[0].tv_nsec = 0;
	times[1].tv_sec = 200;
	times[1].tv_nsec = 0;

	errno = 0;
	ret = utimensat(test_common.dirFd, "nonexistent_utimensat", times, 0);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);
}


TEST(fileops_utimensat, utimensat_ebadf)
{
	int ret;
	struct timespec times[2];

	times[0].tv_sec = 100;
	times[0].tv_nsec = 0;
	times[1].tv_sec = 200;
	times[1].tv_nsec = 0;

	errno = 0;
	ret = utimensat(-1, "test_futimens_file", times, 0);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


TEST(fileops_utimensat, utimensat_einval_bad_nsec)
{
	int ret;
	struct timespec times[2];

	times[0].tv_sec = 100;
	times[0].tv_nsec = 2000000000L;
	times[1].tv_sec = 200;
	times[1].tv_nsec = 0;

	errno = 0;
	ret = utimensat(test_common.dirFd, "test_futimens_file", times, 0);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST_GROUP_RUNNER(fileops_utimensat)
{
	RUN_TEST_CASE(fileops_utimensat, utimensat_relative_path);
	RUN_TEST_CASE(fileops_utimensat, utimensat_at_fdcwd);
	RUN_TEST_CASE(fileops_utimensat, utimensat_null_times);
	RUN_TEST_CASE(fileops_utimensat, utimensat_enoent);
	RUN_TEST_CASE(fileops_utimensat, utimensat_ebadf);
	RUN_TEST_CASE(fileops_utimensat, utimensat_einval_bad_nsec);
}
