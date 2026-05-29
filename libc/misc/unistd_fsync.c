/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <unistd.h>
 * TESTED:
 *    - fsync()
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
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/socket.h>

#include "unity_fixture.h"

#define FSYNC_TEST_FILE "/tmp/test_fsync_file"
#define FSYNC_TEST_DATA "fsync test data payload"


static struct {
	int fd;
} test_common;


TEST_GROUP(unistd_fsync);


TEST_SETUP(unistd_fsync)
{
	unlink(FSYNC_TEST_FILE);
	test_common.fd = -1;
}


TEST_TEAR_DOWN(unistd_fsync)
{
	if (test_common.fd >= 0) {
		close(test_common.fd);
		test_common.fd = -1;
	}
	unlink(FSYNC_TEST_FILE);
}


TEST(unistd_fsync, fsync_regular_file)
{
	/* "Upon successful completion, fsync() shall return 0." */
	int ret;
	ssize_t n;

	test_common.fd = open(FSYNC_TEST_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.fd);

	n = write(test_common.fd, FSYNC_TEST_DATA, strlen(FSYNC_TEST_DATA));
	TEST_ASSERT_EQUAL_INT((int)strlen(FSYNC_TEST_DATA), (int)n);

	errno = 0;
	ret = fsync(test_common.fd);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, errno);
}


TEST(unistd_fsync, fsync_after_multiple_writes)
{
	/* fsync shall transfer all data to storage */
	int ret;
	ssize_t n;
	int i;

	test_common.fd = open(FSYNC_TEST_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.fd);

	for (i = 0; i < 10; i++) {
		n = write(test_common.fd, FSYNC_TEST_DATA, strlen(FSYNC_TEST_DATA));
		TEST_ASSERT_EQUAL_INT((int)strlen(FSYNC_TEST_DATA), (int)n);
	}

	ret = fsync(test_common.fd);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST(unistd_fsync, fsync_readonly_file)
{
	/* fsync on a read-only fd — implementation may succeed or fail with EINVAL/EBADF.
	 * POSIX does not mandate failure for read-only fds, but the data must be synced. */
	int wfd;
	ssize_t n;
	int ret;

	wfd = open(FSYNC_TEST_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	TEST_ASSERT_NOT_EQUAL_INT(-1, wfd);
	n = write(wfd, FSYNC_TEST_DATA, strlen(FSYNC_TEST_DATA));
	TEST_ASSERT_EQUAL_INT((int)strlen(FSYNC_TEST_DATA), (int)n);
	close(wfd);

	test_common.fd = open(FSYNC_TEST_FILE, O_RDONLY);
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.fd);

	errno = 0;
	ret = fsync(test_common.fd);
	/* Both 0 and -1 are acceptable; if -1, errno must be set */
	if (ret == -1) {
		TEST_ASSERT_NOT_EQUAL_INT(0, errno);
	}
	else {
		TEST_ASSERT_EQUAL_INT(0, ret);
	}
}


TEST(unistd_fsync, fsync_ebadf_invalid_fd)
{
	/* "EBADF: The fildes argument is not a valid descriptor." */
	int ret;

	errno = 0;
	ret = fsync(-1);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


TEST(unistd_fsync, fsync_ebadf_closed_fd)
{
	/* fsync on a closed fd shall fail with EBADF */
	int fd;
	int ret;

	fd = open(FSYNC_TEST_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	TEST_ASSERT_NOT_EQUAL_INT(-1, fd);
	close(fd);

	errno = 0;
	ret = fsync(fd);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


TEST(unistd_fsync, fsync_einval_socket)
{
	/* "EINVAL: The fildes argument does not refer to a file on which
	 *  this operation is possible." — a socket fd is not syncable */
	int sfd;
	int ret;

	sfd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_NOT_EQUAL_INT(-1, sfd);

	errno = 0;
	ret = fsync(sfd);
	/* POSIX mandates EINVAL for non-syncable descriptors */
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);

	close(sfd);
}


TEST(unistd_fsync, fsync_data_persists)
{
	/* After fsync + close, data is readable */
	ssize_t n;
	int ret;
	char buf[64];

	test_common.fd = open(FSYNC_TEST_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.fd);

	n = write(test_common.fd, FSYNC_TEST_DATA, strlen(FSYNC_TEST_DATA));
	TEST_ASSERT_EQUAL_INT((int)strlen(FSYNC_TEST_DATA), (int)n);

	ret = fsync(test_common.fd);
	TEST_ASSERT_EQUAL_INT(0, ret);

	close(test_common.fd);
	test_common.fd = -1;

	/* Verify data */
	test_common.fd = open(FSYNC_TEST_FILE, O_RDONLY);
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.fd);

	memset(buf, 0, sizeof(buf));
	n = read(test_common.fd, buf, sizeof(buf) - 1);
	TEST_ASSERT_EQUAL_INT((int)strlen(FSYNC_TEST_DATA), (int)n);
	TEST_ASSERT_EQUAL_STRING(FSYNC_TEST_DATA, buf);
}


TEST_GROUP_RUNNER(unistd_fsync)
{
	RUN_TEST_CASE(unistd_fsync, fsync_regular_file);
	RUN_TEST_CASE(unistd_fsync, fsync_after_multiple_writes);
	RUN_TEST_CASE(unistd_fsync, fsync_readonly_file);
	RUN_TEST_CASE(unistd_fsync, fsync_ebadf_invalid_fd);
	RUN_TEST_CASE(unistd_fsync, fsync_ebadf_closed_fd);
	RUN_TEST_CASE(unistd_fsync, fsync_einval_socket);
	RUN_TEST_CASE(unistd_fsync, fsync_data_persists);
}
