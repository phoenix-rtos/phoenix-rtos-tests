/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <sys/uio.h>
 * TESTED:
 *    - readv()
 *    - writev()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <sys/uio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#include "unity_fixture.h"

#define UIO_TEST_FILE     "/tmp/test_uio_readv_writev"
#define UIO_BUF_SIZE      64
#define UIO_LARGE_IOV_CNT 4

static struct {
	int fd;
} test_common;


TEST_GROUP(uio_writev);

TEST_SETUP(uio_writev)
{
	mkdir("/tmp", 0777);
	unlink(UIO_TEST_FILE);
	test_common.fd = open(UIO_TEST_FILE, O_RDWR | O_CREAT | O_TRUNC, 0666);
	TEST_ASSERT_TRUE(test_common.fd >= 0);
}

TEST_TEAR_DOWN(uio_writev)
{
	if (test_common.fd >= 0) {
		close(test_common.fd);
		test_common.fd = -1;
	}
	unlink(UIO_TEST_FILE);
}


TEST(uio_writev, writev_single_iov)
{
	ssize_t ret;
	const char data[] = "hello";
	struct iovec iov[1];
	char readBuf[UIO_BUF_SIZE];

	iov[0].iov_base = (void *)data;
	iov[0].iov_len = strlen(data);

	ret = writev(test_common.fd, iov, 1);
	TEST_ASSERT_EQUAL_INT((ssize_t)strlen(data), ret);

	/* Verify written content */
	ret = lseek(test_common.fd, 0, SEEK_SET);
	TEST_ASSERT_EQUAL_INT(0, ret);

	memset(readBuf, 0, sizeof(readBuf));
	ret = read(test_common.fd, readBuf, sizeof(readBuf));
	TEST_ASSERT_EQUAL_INT((ssize_t)strlen(data), ret);
	TEST_ASSERT_EQUAL_MEMORY(data, readBuf, strlen(data));
}


TEST(uio_writev, writev_multiple_iov)
{
	ssize_t ret;
	const char data1[] = "abc";
	const char data2[] = "defg";
	const char data3[] = "hi";
	const char expected[] = "abcdefghi";
	struct iovec iov[3];
	char readBuf[UIO_BUF_SIZE];

	iov[0].iov_base = (void *)data1;
	iov[0].iov_len = strlen(data1);
	iov[1].iov_base = (void *)data2;
	iov[1].iov_len = strlen(data2);
	iov[2].iov_base = (void *)data3;
	iov[2].iov_len = strlen(data3);

	ret = writev(test_common.fd, iov, 3);
	TEST_ASSERT_EQUAL_INT((ssize_t)(strlen(data1) + strlen(data2) + strlen(data3)), ret);

	/* Verify content is gathered in order */
	ret = lseek(test_common.fd, 0, SEEK_SET);
	TEST_ASSERT_EQUAL_INT(0, ret);

	memset(readBuf, 0, sizeof(readBuf));
	ret = read(test_common.fd, readBuf, sizeof(readBuf));
	TEST_ASSERT_EQUAL_INT((ssize_t)strlen(expected), ret);
	TEST_ASSERT_EQUAL_MEMORY(expected, readBuf, strlen(expected));
}


TEST(uio_writev, writev_zero_len_iov)
{
	ssize_t ret;
	const char data[] = "test";
	struct iovec iov[3];
	char readBuf[UIO_BUF_SIZE];

	/* First iov has zero length, second has data, third is zero */
	iov[0].iov_base = (void *)data;
	iov[0].iov_len = 0;
	iov[1].iov_base = (void *)data;
	iov[1].iov_len = strlen(data);
	iov[2].iov_base = (void *)data;
	iov[2].iov_len = 0;

	ret = writev(test_common.fd, iov, 3);
	TEST_ASSERT_EQUAL_INT((ssize_t)strlen(data), ret);

	ret = lseek(test_common.fd, 0, SEEK_SET);
	TEST_ASSERT_EQUAL_INT(0, ret);

	memset(readBuf, 0, sizeof(readBuf));
	ret = read(test_common.fd, readBuf, sizeof(readBuf));
	TEST_ASSERT_EQUAL_INT((ssize_t)strlen(data), ret);
	TEST_ASSERT_EQUAL_MEMORY(data, readBuf, strlen(data));
}


TEST(uio_writev, writev_all_zero_len_regular_file)
{
	ssize_t ret;
	struct iovec iov[2];
	char buf1[4];
	char buf2[4];

	/* For regular files, if all iov_len are 0, writev returns 0 */
	iov[0].iov_base = buf1;
	iov[0].iov_len = 0;
	iov[1].iov_base = buf2;
	iov[1].iov_len = 0;

	ret = writev(test_common.fd, iov, 2);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST(uio_writev, writev_ebadf)
{
	ssize_t ret;
	struct iovec iov[1];
	char buf[4] = "abc";

	iov[0].iov_base = buf;
	iov[0].iov_len = 3;

	errno = 0;
	ret = writev(-1, iov, 1);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


TEST(uio_writev, writev_ebadf_readonly)
{
	ssize_t ret;
	int rdFd;
	struct iovec iov[1];
	char buf[4] = "abc";

	/* Close the r/w fd and open read-only */
	close(test_common.fd);
	test_common.fd = -1;

	rdFd = open(UIO_TEST_FILE, O_RDONLY);
	TEST_ASSERT_TRUE(rdFd >= 0);

	iov[0].iov_base = buf;
	iov[0].iov_len = 3;

	errno = 0;
	ret = writev(rdFd, iov, 1);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EBADF, errno);

	close(rdFd);
}





TEST(uio_writev, writev_fills_areas_in_order)
{
	ssize_t ret;
	const char seg1[] = "FIRST";
	const char seg2[] = "SECOND";
	struct iovec iov[2];
	char readBuf[UIO_BUF_SIZE];

	iov[0].iov_base = (void *)seg1;
	iov[0].iov_len = strlen(seg1);
	iov[1].iov_base = (void *)seg2;
	iov[1].iov_len = strlen(seg2);

	ret = writev(test_common.fd, iov, 2);
	TEST_ASSERT_EQUAL_INT((ssize_t)(strlen(seg1) + strlen(seg2)), ret);

	ret = lseek(test_common.fd, 0, SEEK_SET);
	TEST_ASSERT_EQUAL_INT(0, ret);

	memset(readBuf, 0, sizeof(readBuf));
	ret = read(test_common.fd, readBuf, sizeof(readBuf));
	TEST_ASSERT_EQUAL_INT((ssize_t)(strlen(seg1) + strlen(seg2)), ret);

	/* Verify ordering: seg1 comes first, then seg2 */
	TEST_ASSERT_EQUAL_MEMORY(seg1, readBuf, strlen(seg1));
	TEST_ASSERT_EQUAL_MEMORY(seg2, readBuf + strlen(seg1), strlen(seg2));
}


TEST(uio_writev, writev_pipe_atomicity)
{
#if defined(__TARGET_ARMV7R5F) || defined(__TARGET_AARCH64A53)
	TEST_IGNORE_MESSAGE("no posixsrv");
#endif
	ssize_t ret;
	int pipeFds[2];
	const char data1[] = "aaa";
	const char data2[] = "bbb";
	struct iovec iov[2];
	char readBuf[UIO_BUF_SIZE];

	ret = pipe(pipeFds);
	TEST_ASSERT_EQUAL_INT(0, ret);

	iov[0].iov_base = (void *)data1;
	iov[0].iov_len = strlen(data1);
	iov[1].iov_base = (void *)data2;
	iov[1].iov_len = strlen(data2);

	ret = writev(pipeFds[1], iov, 2);
	TEST_ASSERT_EQUAL_INT((ssize_t)(strlen(data1) + strlen(data2)), ret);

	memset(readBuf, 0, sizeof(readBuf));
	ret = read(pipeFds[0], readBuf, sizeof(readBuf));
	TEST_ASSERT_EQUAL_INT((ssize_t)(strlen(data1) + strlen(data2)), ret);
	TEST_ASSERT_EQUAL_MEMORY("aaabbb", readBuf, 6);

	close(pipeFds[0]);
	close(pipeFds[1]);
}


TEST_GROUP_RUNNER(uio_writev)
{
	RUN_TEST_CASE(uio_writev, writev_single_iov);
	RUN_TEST_CASE(uio_writev, writev_multiple_iov);
	RUN_TEST_CASE(uio_writev, writev_zero_len_iov);
	RUN_TEST_CASE(uio_writev, writev_all_zero_len_regular_file);
	RUN_TEST_CASE(uio_writev, writev_ebadf);
	RUN_TEST_CASE(uio_writev, writev_ebadf_readonly);
	RUN_TEST_CASE(uio_writev, writev_fills_areas_in_order);
	RUN_TEST_CASE(uio_writev, writev_pipe_atomicity);
}


/* ========================================================================= */


TEST_GROUP(uio_readv);

TEST_SETUP(uio_readv)
{
	mkdir("/tmp", 0777);
	unlink(UIO_TEST_FILE);
	test_common.fd = open(UIO_TEST_FILE, O_RDWR | O_CREAT | O_TRUNC, 0666);
	TEST_ASSERT_TRUE(test_common.fd >= 0);
}

TEST_TEAR_DOWN(uio_readv)
{
	if (test_common.fd >= 0) {
		close(test_common.fd);
		test_common.fd = -1;
	}
	unlink(UIO_TEST_FILE);
}


TEST(uio_readv, readv_single_iov)
{
	ssize_t ret;
	const char data[] = "hello";
	char buf[UIO_BUF_SIZE];
	struct iovec iov[1];

	ret = write(test_common.fd, data, strlen(data));
	TEST_ASSERT_EQUAL_INT((ssize_t)strlen(data), ret);

	ret = lseek(test_common.fd, 0, SEEK_SET);
	TEST_ASSERT_EQUAL_INT(0, ret);

	memset(buf, 0, sizeof(buf));
	iov[0].iov_base = buf;
	iov[0].iov_len = sizeof(buf);

	ret = readv(test_common.fd, iov, 1);
	TEST_ASSERT_EQUAL_INT((ssize_t)strlen(data), ret);
	TEST_ASSERT_EQUAL_MEMORY(data, buf, strlen(data));
}


TEST(uio_readv, readv_multiple_iov_scatter)
{
	ssize_t ret;
	const char data[] = "abcdefghij";
	char buf1[3];
	char buf2[4];
	char buf3[3];
	struct iovec iov[3];

	ret = write(test_common.fd, data, strlen(data));
	TEST_ASSERT_EQUAL_INT((ssize_t)strlen(data), ret);

	ret = lseek(test_common.fd, 0, SEEK_SET);
	TEST_ASSERT_EQUAL_INT(0, ret);

	memset(buf1, 0, sizeof(buf1));
	memset(buf2, 0, sizeof(buf2));
	memset(buf3, 0, sizeof(buf3));

	iov[0].iov_base = buf1;
	iov[0].iov_len = sizeof(buf1);
	iov[1].iov_base = buf2;
	iov[1].iov_len = sizeof(buf2);
	iov[2].iov_base = buf3;
	iov[2].iov_len = sizeof(buf3);

	ret = readv(test_common.fd, iov, 3);
	TEST_ASSERT_EQUAL_INT((ssize_t)strlen(data), ret);

	/* readv fills areas completely before proceeding to next */
	TEST_ASSERT_EQUAL_MEMORY("abc", buf1, 3);
	TEST_ASSERT_EQUAL_MEMORY("defg", buf2, 4);
	TEST_ASSERT_EQUAL_MEMORY("hij", buf3, 3);
}


TEST(uio_readv, readv_fills_completely_before_next)
{
	ssize_t ret;
	const char data[] = "12345678";
	char buf1[4];
	char buf2[8];
	struct iovec iov[2];

	ret = write(test_common.fd, data, strlen(data));
	TEST_ASSERT_EQUAL_INT((ssize_t)strlen(data), ret);

	ret = lseek(test_common.fd, 0, SEEK_SET);
	TEST_ASSERT_EQUAL_INT(0, ret);

	memset(buf1, 0, sizeof(buf1));
	memset(buf2, 0, sizeof(buf2));

	iov[0].iov_base = buf1;
	iov[0].iov_len = sizeof(buf1);
	iov[1].iov_base = buf2;
	iov[1].iov_len = sizeof(buf2);

	ret = readv(test_common.fd, iov, 2);
	TEST_ASSERT_EQUAL_INT((ssize_t)strlen(data), ret);

	/* First buffer must be completely filled */
	TEST_ASSERT_EQUAL_MEMORY("1234", buf1, 4);
	/* Remaining data goes to second buffer */
	TEST_ASSERT_EQUAL_MEMORY("5678", buf2, 4);
}


TEST(uio_readv, readv_eof_returns_zero)
{
	ssize_t ret;
	char buf[UIO_BUF_SIZE];
	struct iovec iov[1];

	/* File is empty, reading at EOF should return 0 */
	iov[0].iov_base = buf;
	iov[0].iov_len = sizeof(buf);

	ret = readv(test_common.fd, iov, 1);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST(uio_readv, readv_partial_eof)
{
	ssize_t ret;
	const char data[] = "short";
	char buf1[3];
	char buf2[8];
	struct iovec iov[2];

	ret = write(test_common.fd, data, strlen(data));
	TEST_ASSERT_EQUAL_INT((ssize_t)strlen(data), ret);

	ret = lseek(test_common.fd, 0, SEEK_SET);
	TEST_ASSERT_EQUAL_INT(0, ret);

	memset(buf1, 0, sizeof(buf1));
	memset(buf2, 0, sizeof(buf2));

	iov[0].iov_base = buf1;
	iov[0].iov_len = sizeof(buf1);
	iov[1].iov_base = buf2;
	iov[1].iov_len = sizeof(buf2);

	/* Total iov capacity (11) > file size (5): returns actual bytes read */
	ret = readv(test_common.fd, iov, 2);
	TEST_ASSERT_EQUAL_INT((ssize_t)strlen(data), ret);

	/* First buffer completely filled */
	TEST_ASSERT_EQUAL_MEMORY("sho", buf1, 3);
	/* Second buffer partially filled */
	TEST_ASSERT_EQUAL_MEMORY("rt", buf2, 2);
}


TEST(uio_readv, readv_ebadf)
{
	ssize_t ret;
	char buf[4];
	struct iovec iov[1];

	iov[0].iov_base = buf;
	iov[0].iov_len = sizeof(buf);

	errno = 0;
	ret = readv(-1, iov, 1);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


TEST(uio_readv, readv_ebadf_writeonly)
{
	ssize_t ret;
	int wrFd;
	char buf[4];
	struct iovec iov[1];

	close(test_common.fd);
	test_common.fd = -1;

	wrFd = open(UIO_TEST_FILE, O_WRONLY);
	TEST_ASSERT_TRUE(wrFd >= 0);

	iov[0].iov_base = buf;
	iov[0].iov_len = sizeof(buf);

	errno = 0;
	ret = readv(wrFd, iov, 1);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EBADF, errno);

	close(wrFd);
}





TEST(uio_readv, readv_zero_len_iov)
{
	ssize_t ret;
	const char data[] = "hello";
	char buf1[4];
	char buf2[8];
	struct iovec iov[3];

	ret = write(test_common.fd, data, strlen(data));
	TEST_ASSERT_EQUAL_INT((ssize_t)strlen(data), ret);

	ret = lseek(test_common.fd, 0, SEEK_SET);
	TEST_ASSERT_EQUAL_INT(0, ret);

	memset(buf1, 0, sizeof(buf1));
	memset(buf2, 0, sizeof(buf2));

	/* iov[0] zero len, iov[1] has space, iov[2] zero len */
	iov[0].iov_base = buf1;
	iov[0].iov_len = 0;
	iov[1].iov_base = buf2;
	iov[1].iov_len = sizeof(buf2);
	iov[2].iov_base = buf1;
	iov[2].iov_len = 0;

	ret = readv(test_common.fd, iov, 3);
	TEST_ASSERT_EQUAL_INT((ssize_t)strlen(data), ret);
	TEST_ASSERT_EQUAL_MEMORY("hello", buf2, 5);
}


TEST(uio_readv, readv_pipe)
{
#if defined(__TARGET_ARMV7R5F) || defined(__TARGET_AARCH64A53)
	TEST_IGNORE_MESSAGE("no posixsrv");
#endif
	ssize_t ret;
	int pipeFds[2];
	const char data[] = "pipedata";
	char buf1[4];
	char buf2[4];
	struct iovec iov[2];

	ret = pipe(pipeFds);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = write(pipeFds[1], data, strlen(data));
	TEST_ASSERT_EQUAL_INT((ssize_t)strlen(data), ret);

	memset(buf1, 0, sizeof(buf1));
	memset(buf2, 0, sizeof(buf2));

	iov[0].iov_base = buf1;
	iov[0].iov_len = sizeof(buf1);
	iov[1].iov_base = buf2;
	iov[1].iov_len = sizeof(buf2);

	ret = readv(pipeFds[0], iov, 2);
	TEST_ASSERT_EQUAL_INT((ssize_t)strlen(data), ret);
	TEST_ASSERT_EQUAL_MEMORY("pipe", buf1, 4);
	TEST_ASSERT_EQUAL_MEMORY("data", buf2, 4);

	close(pipeFds[0]);
	close(pipeFds[1]);
}


TEST_GROUP_RUNNER(uio_readv)
{
	RUN_TEST_CASE(uio_readv, readv_single_iov);
	RUN_TEST_CASE(uio_readv, readv_multiple_iov_scatter);
	RUN_TEST_CASE(uio_readv, readv_fills_completely_before_next);
	RUN_TEST_CASE(uio_readv, readv_eof_returns_zero);
	RUN_TEST_CASE(uio_readv, readv_partial_eof);
	RUN_TEST_CASE(uio_readv, readv_ebadf);
	RUN_TEST_CASE(uio_readv, readv_ebadf_writeonly);
	RUN_TEST_CASE(uio_readv, readv_zero_len_iov);
	RUN_TEST_CASE(uio_readv, readv_pipe);
}
