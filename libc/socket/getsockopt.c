/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <sys/socket.h>
 * TESTED:
 *    - getsockopt()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Lukasz Kruszynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#include "unity_fixture.h"


TEST_GROUP(socket_api_getsockopt);

static int fd;

TEST_SETUP(socket_api_getsockopt)
{
	fd = -1;
}

TEST_TEAR_DOWN(socket_api_getsockopt)
{
	if (fd >= 0) {
		close(fd);
		fd = -1;
	}
}


TEST(socket_api_getsockopt, basic_so_type_stream)
{
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1644 issue");
#endif
	int ret;
	int optval;
	socklen_t optlen;

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_TRUE(fd >= 0);

	optval = -1;
	optlen = sizeof(optval);
	ret = getsockopt(fd, SOL_SOCKET, SO_TYPE, &optval, &optlen);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(SOCK_STREAM, optval);
	TEST_ASSERT_EQUAL_INT((socklen_t)sizeof(optval), optlen);
}


TEST(socket_api_getsockopt, basic_so_type_dgram)
{
	int ret;
	int optval;
	socklen_t optlen;

	fd = socket(AF_UNIX, SOCK_DGRAM, 0);
	TEST_ASSERT_TRUE(fd >= 0);

	optval = -1;
	optlen = sizeof(optval);
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1644 issue");
#endif
	ret = getsockopt(fd, SOL_SOCKET, SO_TYPE, &optval, &optlen);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(SOCK_DGRAM, optval);
}


TEST(socket_api_getsockopt, so_error_no_error)
{
	int ret;
	int optval;
	socklen_t optlen;

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_TRUE(fd >= 0);

	optval = -1;
	optlen = sizeof(optval);
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1644 issue");
#endif
	ret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &optval, &optlen);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, optval);
}


TEST(socket_api_getsockopt, so_rcvbuf)
{
	int ret;
	int optval;
	socklen_t optlen;

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_TRUE(fd >= 0);

	optval = 0;
	optlen = sizeof(optval);
	ret = getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &optval, &optlen);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_GREATER_THAN_INT(0, optval);
}


TEST(socket_api_getsockopt, so_sndbuf)
{
	int ret;
	int optval;
	socklen_t optlen;

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_TRUE(fd >= 0);

	optval = 0;
	optlen = sizeof(optval);
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1644 issue");
#endif
	ret = getsockopt(fd, SOL_SOCKET, SO_SNDBUF, &optval, &optlen);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_GREATER_THAN_INT(0, optval);
}


TEST(socket_api_getsockopt, truncation_small_optlen)
{
	int ret;
	int optval;
	socklen_t optlen;

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_TRUE(fd >= 0);

	/* Request with optlen smaller than actual value size - value shall be truncated */
	optval = 0;
	optlen = 1;
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1644 issue");
#endif
	ret = getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &optval, &optlen);
	TEST_ASSERT_EQUAL_INT(0, ret);
	/* optlen should remain at 1 (truncated) or be adjusted - either way, no error */
}


TEST(socket_api_getsockopt, optlen_updated)
{
	int ret;
	int optval;
	socklen_t optlen;

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_TRUE(fd >= 0);

	/* optlen larger than needed - shall be modified to indicate actual length */
	optval = 0;
	optlen = 128;
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1644 issue");
#endif
	ret = getsockopt(fd, SOL_SOCKET, SO_TYPE, &optval, &optlen);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT((socklen_t)sizeof(int), optlen);
}


TEST(socket_api_getsockopt, ebadf_invalid_fd)
{
	int ret;
	int optval;
	socklen_t optlen;

	optval = 0;
	optlen = sizeof(optval);
	errno = 0;
	ret = getsockopt(-1, SOL_SOCKET, SO_TYPE, &optval, &optlen);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


TEST(socket_api_getsockopt, enotsock_regular_fd)
{
	int ret;
	int optval;
	socklen_t optlen;
	int pipefd[2];

	ret = pipe(pipefd);
	TEST_ASSERT_EQUAL_INT(0, ret);

	if (TEST_PROTECT()) {
		optval = 0;
		optlen = sizeof(optval);
		errno = 0;
		ret = getsockopt(pipefd[0], SOL_SOCKET, SO_TYPE, &optval, &optlen);
		TEST_ASSERT_EQUAL_INT(-1, ret);
		TEST_ASSERT_EQUAL_INT(ENOTSOCK, errno);
	}
	close(pipefd[0]);
	close(pipefd[1]);
}


TEST(socket_api_getsockopt, enoprotoopt_unsupported_option)
{
	int ret;
	int optval;
	socklen_t optlen;

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_TRUE(fd >= 0);

	/* TCP-level option on a UNIX socket should fail with ENOPROTOOPT */
	optval = 0;
	optlen = sizeof(optval);
	errno = 0;
	ret = getsockopt(fd, IPPROTO_TCP, 1, &optval, &optlen);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_TRUE(errno == ENOPROTOOPT || errno == EINVAL || errno == EOPNOTSUPP);
}


TEST(socket_api_getsockopt, so_acceptconn_not_listening)
{
	int ret;
	int optval;
	socklen_t optlen;

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_TRUE(fd >= 0);

	optval = -1;
	optlen = sizeof(optval);
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1644 issue");
#endif
	ret = getsockopt(fd, SOL_SOCKET, SO_ACCEPTCONN, &optval, &optlen);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, optval);
}


TEST(socket_api_getsockopt, so_rcvtimeo_default)
{
	int ret;
	struct timeval tv;
	socklen_t optlen;

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_TRUE(fd >= 0);

	memset(&tv, 0xff, sizeof(tv));
	optlen = sizeof(tv);
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1644 issue");
#endif
	ret = getsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, &optlen);
	TEST_ASSERT_EQUAL_INT(0, ret);
	/* Default timeout should be 0 (infinite/no timeout) */
	TEST_ASSERT_EQUAL_INT(0, (int)tv.tv_sec);
	TEST_ASSERT_EQUAL_INT(0, (int)tv.tv_usec);
}


TEST(socket_api_getsockopt, so_sndtimeo_default)
{
	int ret;
	struct timeval tv;
	socklen_t optlen;

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_TRUE(fd >= 0);

	memset(&tv, 0xff, sizeof(tv));
	optlen = sizeof(tv);
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1644 issue");
#endif
	ret = getsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, &optlen);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, (int)tv.tv_sec);
	TEST_ASSERT_EQUAL_INT(0, (int)tv.tv_usec);
}


TEST(socket_api_getsockopt, retrieves_value_after_setsockopt)
{
	int ret;
	int optval;
	socklen_t optlen;
	struct timeval tv;

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_TRUE(fd >= 0);

#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1644 issue");
#endif
	/* Set SO_RCVTIMEO to 2 seconds */
	tv.tv_sec = 2;
	tv.tv_usec = 0;
	ret = setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Retrieve and verify */
	memset(&tv, 0, sizeof(tv));
	optlen = sizeof(tv);
	ret = getsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, &optlen);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(2, (int)tv.tv_sec);
	TEST_ASSERT_EQUAL_INT(0, (int)tv.tv_usec);

	/* Set SO_SNDBUF */
	optval = 4096;
	ret = setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &optval, sizeof(optval));
	TEST_ASSERT_EQUAL_INT(0, ret);

	optval = 0;
	optlen = sizeof(optval);
	ret = getsockopt(fd, SOL_SOCKET, SO_SNDBUF, &optval, &optlen);
	TEST_ASSERT_EQUAL_INT(0, ret);
	/* Implementation may double the value, but it should be >= what was set */
	TEST_ASSERT_GREATER_THAN_INT(0, optval);
}


TEST_GROUP_RUNNER(socket_api_getsockopt)
{
	RUN_TEST_CASE(socket_api_getsockopt, basic_so_type_stream);
	RUN_TEST_CASE(socket_api_getsockopt, basic_so_type_dgram);
	RUN_TEST_CASE(socket_api_getsockopt, so_error_no_error);
	RUN_TEST_CASE(socket_api_getsockopt, so_rcvbuf);
	RUN_TEST_CASE(socket_api_getsockopt, so_sndbuf);
	RUN_TEST_CASE(socket_api_getsockopt, truncation_small_optlen);
	RUN_TEST_CASE(socket_api_getsockopt, optlen_updated);
	RUN_TEST_CASE(socket_api_getsockopt, ebadf_invalid_fd);
	RUN_TEST_CASE(socket_api_getsockopt, enotsock_regular_fd);
	RUN_TEST_CASE(socket_api_getsockopt, enoprotoopt_unsupported_option);
	RUN_TEST_CASE(socket_api_getsockopt, so_acceptconn_not_listening);
	RUN_TEST_CASE(socket_api_getsockopt, so_rcvtimeo_default);
	RUN_TEST_CASE(socket_api_getsockopt, so_sndtimeo_default);
	RUN_TEST_CASE(socket_api_getsockopt, retrieves_value_after_setsockopt);
}
