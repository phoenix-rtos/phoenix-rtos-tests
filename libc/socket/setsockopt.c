/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <sys/socket.h>
 * TESTED:
 *    - setsockopt()
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
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#include "unity_fixture.h"

TEST_GROUP(socket_api_setsockopt);

TEST_SETUP(socket_api_setsockopt)
{
}

TEST_TEAR_DOWN(socket_api_setsockopt)
{
}


TEST(socket_api_setsockopt, so_reuseaddr)
{
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1644");
#endif
	int fd;
	int ret;
	int optval;
	socklen_t optlen;

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_TRUE(fd >= 0);

	optval = 1;
	ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Verify the option was set */
	optval = 0;
	optlen = sizeof(optval);
	ret = getsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, &optlen);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_NOT_EQUAL_INT(0, optval);

	close(fd);
}


TEST(socket_api_setsockopt, so_keepalive)
{
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1644");
#endif
	int fd;
	int ret;
	int optval;
	socklen_t optlen;

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_TRUE(fd >= 0);

	optval = 1;
	ret = setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
	TEST_ASSERT_EQUAL_INT(0, ret);

	optval = 0;
	optlen = sizeof(optval);
	ret = getsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &optval, &optlen);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_NOT_EQUAL_INT(0, optval);

	close(fd);
}


TEST(socket_api_setsockopt, so_rcvbuf)
{
	int fd;
	int ret;
	int optval;
	socklen_t optlen;

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_TRUE(fd >= 0);

	optval = 4096;
	ret = setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &optval, sizeof(optval));
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Kernel may double the value, but it should be at least what we set */
	optval = 0;
	optlen = sizeof(optval);
	ret = getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &optval, &optlen);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_TRUE(optval >= 4096);

	close(fd);
}


TEST(socket_api_setsockopt, so_sndbuf)
{
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1644");
#endif
	int fd;
	int ret;
	int optval;
	socklen_t optlen;

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_TRUE(fd >= 0);

	optval = 4096;
	ret = setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &optval, sizeof(optval));
	TEST_ASSERT_EQUAL_INT(0, ret);

	optval = 0;
	optlen = sizeof(optval);
	ret = getsockopt(fd, SOL_SOCKET, SO_SNDBUF, &optval, &optlen);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_TRUE(optval >= 4096);

	close(fd);
}


TEST(socket_api_setsockopt, so_rcvtimeo)
{
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1644");
#endif
	int fd;
	int ret;
	struct timeval tv;
	struct timeval tvOut;
	socklen_t optlen;

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_TRUE(fd >= 0);

	tv.tv_sec = 2;
	tv.tv_usec = 500000;
	ret = setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	TEST_ASSERT_EQUAL_INT(0, ret);

	memset(&tvOut, 0, sizeof(tvOut));
	optlen = sizeof(tvOut);
	ret = getsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tvOut, &optlen);
	TEST_ASSERT_EQUAL_INT(0, ret);
	/* Value may be rounded up but should be at least what was set */
	TEST_ASSERT_TRUE(tvOut.tv_sec >= 2);

	close(fd);
}


TEST(socket_api_setsockopt, so_sndtimeo)
{
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1644");
#endif
	int fd;
	int ret;
	struct timeval tv;
	struct timeval tvOut;
	socklen_t optlen;

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_TRUE(fd >= 0);

	tv.tv_sec = 3;
	tv.tv_usec = 0;
	ret = setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
	TEST_ASSERT_EQUAL_INT(0, ret);

	memset(&tvOut, 0, sizeof(tvOut));
	optlen = sizeof(tvOut);
	ret = getsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tvOut, &optlen);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_TRUE(tvOut.tv_sec >= 3);

	close(fd);
}


TEST(socket_api_setsockopt, so_broadcast)
{
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1644");
#endif
	int fd;
	int ret;
	int optval;
	socklen_t optlen;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	TEST_ASSERT_TRUE(fd >= 0);

	optval = 1;
	ret = setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval));
	TEST_ASSERT_EQUAL_INT(0, ret);

	optval = 0;
	optlen = sizeof(optval);
	ret = getsockopt(fd, SOL_SOCKET, SO_BROADCAST, &optval, &optlen);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_NOT_EQUAL_INT(0, optval);

	close(fd);
}


TEST(socket_api_setsockopt, return_zero_on_success)
{
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("state-dependent failure");
#endif
	int fd;
	int ret;
	int optval;

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_TRUE(fd >= 0);

	optval = 1;
	ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	TEST_ASSERT_EQUAL_INT(0, ret);

	close(fd);
}


TEST(socket_api_setsockopt, ebadf_invalid_fd)
{
	int ret;
	int optval;

	optval = 1;
	errno = 0;
	ret = setsockopt(-1, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


TEST(socket_api_setsockopt, enotsock_not_socket)
{
	int ret;
	int optval;
	int pipefd[2];

	ret = pipe(pipefd);
	TEST_ASSERT_EQUAL_INT(0, ret);

	optval = 1;
	errno = 0;
	ret = setsockopt(pipefd[0], SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(ENOTSOCK, errno);

	close(pipefd[0]);
	close(pipefd[1]);
}


TEST(socket_api_setsockopt, enoprotoopt_invalid_option)
{
	int fd;
	int ret;
	int optval;

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_TRUE(fd >= 0);

	optval = 1;
	errno = 0;
	ret = setsockopt(fd, SOL_SOCKET, -1, &optval, sizeof(optval));
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(ENOPROTOOPT, errno);

	close(fd);
}


TEST(socket_api_setsockopt, einval_after_shutdown)
{
	int sv[2];
	int ret;
	int optval;

	ret = socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = shutdown(sv[0], SHUT_RDWR);
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1634 issue");
#endif
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Setting option on shut down socket - EINVAL: "the socket has been shut down" */
	optval = 1;
	errno = 0;
	ret = setsockopt(sv[0], SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	/* Some implementations allow this; only check if it returns error */
	if (ret == -1) {
		TEST_ASSERT_EQUAL_INT(EINVAL, errno);
	}

	close(sv[0]);
	close(sv[1]);
}


TEST(socket_api_setsockopt, disable_option)
{
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1644");
#endif
	int fd;
	int ret;
	int optval;
	socklen_t optlen;

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_TRUE(fd >= 0);

	/* Enable */
	optval = 1;
	ret = setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Disable */
	optval = 0;
	ret = setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Verify disabled */
	optval = 1;
	optlen = sizeof(optval);
	ret = getsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &optval, &optlen);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, optval);

	close(fd);
}


TEST(socket_api_setsockopt, so_linger)
{
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1644");
#endif
	int fd;
	int ret;
	struct linger lin;
	struct linger linOut;
	socklen_t optlen;

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_TRUE(fd >= 0);

	lin.l_onoff = 1;
	lin.l_linger = 5;
	ret = setsockopt(fd, SOL_SOCKET, SO_LINGER, &lin, sizeof(lin));
	TEST_ASSERT_EQUAL_INT(0, ret);

	memset(&linOut, 0, sizeof(linOut));
	optlen = sizeof(linOut);
	ret = getsockopt(fd, SOL_SOCKET, SO_LINGER, &linOut, &optlen);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_NOT_EQUAL_INT(0, linOut.l_onoff);
	TEST_ASSERT_EQUAL_INT(5, linOut.l_linger);

	close(fd);
}


TEST(socket_api_setsockopt, readonly_so_error)
{
	int fd;
	int ret;
	int optval;

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_TRUE(fd >= 0);

	/* SO_ERROR is read-only; setting it should fail */
	optval = 0;
	errno = 0;
	ret = setsockopt(fd, SOL_SOCKET, SO_ERROR, &optval, sizeof(optval));
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_TRUE(errno == ENOPROTOOPT || errno == EINVAL);

	close(fd);
}


TEST(socket_api_setsockopt, readonly_so_type)
{
	int fd;
	int ret;
	int optval;

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_TRUE(fd >= 0);

	/* SO_TYPE is read-only; setting it should fail */
	optval = SOCK_STREAM;
	errno = 0;
	ret = setsockopt(fd, SOL_SOCKET, SO_TYPE, &optval, sizeof(optval));
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_TRUE(errno == ENOPROTOOPT || errno == EINVAL);

	close(fd);
}


TEST(socket_api_setsockopt, einval_short_optlen)
{
	int fd;
	int ret;
	int optval;

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_TRUE(fd >= 0);

	/* Passing optlen too small for the option */
	optval = 1;
	errno = 0;
	ret = setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &optval, 0);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	/* POSIX does not mandate a specific errno for invalid optlen */
	TEST_ASSERT_TRUE(errno == EINVAL || errno == ENOENT || errno == ENOPROTOOPT);

	close(fd);
}


TEST(socket_api_setsockopt, readonly_so_acceptconn)
{
	int fd;
	int ret;
	int optval;

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_TRUE(fd >= 0);

	/* SO_ACCEPTCONN is read-only; setting it should fail */
	optval = 1;
	errno = 0;
	ret = setsockopt(fd, SOL_SOCKET, SO_ACCEPTCONN, &optval, sizeof(optval));
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_TRUE(errno == ENOPROTOOPT || errno == EINVAL);

	close(fd);
}


TEST(socket_api_setsockopt, so_dontroute)
{
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1644");
#endif
	int fd;
	int ret;
	int optval;
	socklen_t optlen;

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_TRUE(fd >= 0);

	optval = 1;
	ret = setsockopt(fd, SOL_SOCKET, SO_DONTROUTE, &optval, sizeof(optval));
	TEST_ASSERT_EQUAL_INT(0, ret);

	optval = 0;
	optlen = sizeof(optval);
	ret = getsockopt(fd, SOL_SOCKET, SO_DONTROUTE, &optval, &optlen);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_NOT_EQUAL_INT(0, optval);

	close(fd);
}


TEST(socket_api_setsockopt, so_oobinline)
{
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1644");
#endif
	int fd;
	int ret;
	int optval;
	socklen_t optlen;

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_TRUE(fd >= 0);

	optval = 1;
	ret = setsockopt(fd, SOL_SOCKET, SO_OOBINLINE, &optval, sizeof(optval));
	TEST_ASSERT_EQUAL_INT(0, ret);

	optval = 0;
	optlen = sizeof(optval);
	ret = getsockopt(fd, SOL_SOCKET, SO_OOBINLINE, &optval, &optlen);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_NOT_EQUAL_INT(0, optval);

	close(fd);
}


TEST(socket_api_setsockopt, so_rcvlowat)
{
	int fd;
	int ret;
	int optval;
	socklen_t optlen;

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_TRUE(fd >= 0);

	optval = 128;
	ret = setsockopt(fd, SOL_SOCKET, SO_RCVLOWAT, &optval, sizeof(optval));
	if (ret == -1) {
		/* Some implementations don't support setting SO_RCVLOWAT */
		TEST_ASSERT_TRUE(errno == ENOPROTOOPT || errno == EINVAL);
	}
	else {
		optval = 0;
		optlen = sizeof(optval);
		ret = getsockopt(fd, SOL_SOCKET, SO_RCVLOWAT, &optval, &optlen);
		TEST_ASSERT_EQUAL_INT(0, ret);
		TEST_ASSERT_TRUE(optval >= 1);
	}

	close(fd);
}


TEST(socket_api_setsockopt, so_sndlowat)
{
	int fd;
	int ret;
	int optval;
	socklen_t optlen;

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_TRUE(fd >= 0);

	optval = 512;
	errno = 0;
	ret = setsockopt(fd, SOL_SOCKET, SO_SNDLOWAT, &optval, sizeof(optval));
	if (ret == -1) {
		/* Linux does not allow setting SO_SNDLOWAT (ENOPROTOOPT) */
		TEST_ASSERT_TRUE(errno == ENOPROTOOPT || errno == EINVAL);
	}
	else {
		optval = 0;
		optlen = sizeof(optval);
		ret = getsockopt(fd, SOL_SOCKET, SO_SNDLOWAT, &optval, &optlen);
		TEST_ASSERT_EQUAL_INT(0, ret);
		TEST_ASSERT_TRUE(optval >= 1);
	}

	close(fd);
}


TEST_GROUP_RUNNER(socket_api_setsockopt)
{
	RUN_TEST_CASE(socket_api_setsockopt, so_reuseaddr);
	RUN_TEST_CASE(socket_api_setsockopt, so_keepalive);
	RUN_TEST_CASE(socket_api_setsockopt, so_rcvbuf);
	RUN_TEST_CASE(socket_api_setsockopt, so_sndbuf);
	RUN_TEST_CASE(socket_api_setsockopt, so_rcvtimeo);
	RUN_TEST_CASE(socket_api_setsockopt, so_sndtimeo);
	RUN_TEST_CASE(socket_api_setsockopt, so_broadcast);
	RUN_TEST_CASE(socket_api_setsockopt, return_zero_on_success);
	RUN_TEST_CASE(socket_api_setsockopt, ebadf_invalid_fd);
	RUN_TEST_CASE(socket_api_setsockopt, enotsock_not_socket);
	RUN_TEST_CASE(socket_api_setsockopt, enoprotoopt_invalid_option);
	RUN_TEST_CASE(socket_api_setsockopt, einval_after_shutdown);
	RUN_TEST_CASE(socket_api_setsockopt, disable_option);
	RUN_TEST_CASE(socket_api_setsockopt, so_linger);
	RUN_TEST_CASE(socket_api_setsockopt, readonly_so_error);
	RUN_TEST_CASE(socket_api_setsockopt, readonly_so_type);
	RUN_TEST_CASE(socket_api_setsockopt, einval_short_optlen);
	RUN_TEST_CASE(socket_api_setsockopt, readonly_so_acceptconn);
	RUN_TEST_CASE(socket_api_setsockopt, so_dontroute);
	RUN_TEST_CASE(socket_api_setsockopt, so_oobinline);
	RUN_TEST_CASE(socket_api_setsockopt, so_rcvlowat);
	RUN_TEST_CASE(socket_api_setsockopt, so_sndlowat);
}
