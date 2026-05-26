/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <sys/socket.h>
 * TESTED:
 *    - listen()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Lukasz Kruszynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#include "unity_fixture.h"

#define LISTEN_SOCK_PATH "/tmp/test_listen_sock"
#define LISTEN_BACKLOG   5

#ifndef SOMAXCONN
#define SOMAXCONN 128
#endif

static struct {
	int fd;
} test_common;


TEST_GROUP(socket_api_listen);

TEST_SETUP(socket_api_listen)
{
	unlink(LISTEN_SOCK_PATH);
	test_common.fd = -1;
}

TEST_TEAR_DOWN(socket_api_listen)
{
	if (test_common.fd >= 0) {
		close(test_common.fd);
		test_common.fd = -1;
	}
	unlink(LISTEN_SOCK_PATH);
}


static int test_bindUnixStream(int fd)
{
	struct sockaddr_un addr;

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, LISTEN_SOCK_PATH, sizeof(addr.sun_path) - 1);

	return bind(fd, (struct sockaddr *)&addr, sizeof(addr));
}


TEST(socket_api_listen, basic_success)
{
	int ret;

	test_common.fd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_TRUE(test_common.fd >= 0);

	ret = test_bindUnixStream(test_common.fd);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = listen(test_common.fd, LISTEN_BACKLOG);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST(socket_api_listen, backlog_zero)
{
	int ret;

	test_common.fd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_TRUE(test_common.fd >= 0);

	ret = test_bindUnixStream(test_common.fd);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* backlog of 0 may allow the socket to accept connections */
	ret = listen(test_common.fd, 0);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST(socket_api_listen, backlog_negative)
{
	int ret;

	test_common.fd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_TRUE(test_common.fd >= 0);

	ret = test_bindUnixStream(test_common.fd);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Negative backlog shall behave as if called with backlog of 0 */
	ret = listen(test_common.fd, -1);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST(socket_api_listen, backlog_somaxconn)
{
	int ret;

	test_common.fd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_TRUE(test_common.fd >= 0);

	ret = test_bindUnixStream(test_common.fd);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Implementations shall support values of backlog up to SOMAXCONN */
	ret = listen(test_common.fd, SOMAXCONN);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST(socket_api_listen, backlog_exceeds_somaxconn)
{
	int ret;

	test_common.fd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_TRUE(test_common.fd >= 0);

	ret = test_bindUnixStream(test_common.fd);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Backlog exceeding limit: length of listen queue is set to the limit, no error */
	ret = listen(test_common.fd, SOMAXCONN + 100);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST(socket_api_listen, marks_socket_accepting)
{
	int ret;
	int optval;
	socklen_t optlen;

	test_common.fd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_TRUE(test_common.fd >= 0);

	ret = test_bindUnixStream(test_common.fd);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = listen(test_common.fd, LISTEN_BACKLOG);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Verify socket is now accepting connections via SO_ACCEPTCONN */
	optval = 0;
	optlen = sizeof(optval);
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1644 issue");
#endif
	ret = getsockopt(test_common.fd, SOL_SOCKET, SO_ACCEPTCONN, &optval, &optlen);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_NOT_EQUAL_INT(0, optval);
}


TEST(socket_api_listen, ebadf_invalid_fd)
{
	int ret;

	errno = 0;
	ret = listen(-1, LISTEN_BACKLOG);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


TEST(socket_api_listen, enotsock_pipe_fd)
{
	int ret;
	int pipefd[2];

	ret = pipe(pipefd);
	TEST_ASSERT_EQUAL_INT(0, ret);

	if (TEST_PROTECT()) {
		errno = 0;
		ret = listen(pipefd[0], LISTEN_BACKLOG);
		TEST_ASSERT_EQUAL_INT(-1, ret);
		TEST_ASSERT_EQUAL_INT(ENOTSOCK, errno);
	}
	close(pipefd[0]);
	close(pipefd[1]);
}


TEST(socket_api_listen, eopnotsupp_dgram_socket)
{
	int ret;

	test_common.fd = socket(AF_UNIX, SOCK_DGRAM, 0);
	TEST_ASSERT_TRUE(test_common.fd >= 0);

	/* DGRAM sockets do not support listen */
	errno = 0;
	ret = listen(test_common.fd, LISTEN_BACKLOG);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EOPNOTSUPP, errno);
}


TEST(socket_api_listen, einval_already_connected)
{
	int ret;
	int sv[2];

#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1649 issue");
#endif

	ret = socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
	TEST_ASSERT_EQUAL_INT(0, ret);
	if (TEST_PROTECT()) {
		/* Socket is already connected - shall fail with EINVAL */
		errno = 0;
		ret = listen(sv[0], LISTEN_BACKLOG);
		TEST_ASSERT_EQUAL_INT(-1, ret);
		TEST_ASSERT_EQUAL_INT(EINVAL, errno);
	}
	close(sv[0]);
	close(sv[1]);
}


TEST(socket_api_listen, listen_called_twice)
{
	int ret;

	test_common.fd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_TRUE(test_common.fd >= 0);

	ret = test_bindUnixStream(test_common.fd);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = listen(test_common.fd, LISTEN_BACKLOG);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Calling listen again on an already listening socket should succeed */
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1649 issue");
#endif
	ret = listen(test_common.fd, LISTEN_BACKLOG + 1);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST_GROUP_RUNNER(socket_api_listen)
{
	RUN_TEST_CASE(socket_api_listen, basic_success);
	RUN_TEST_CASE(socket_api_listen, backlog_zero);
	RUN_TEST_CASE(socket_api_listen, backlog_negative);
	RUN_TEST_CASE(socket_api_listen, backlog_somaxconn);
	RUN_TEST_CASE(socket_api_listen, backlog_exceeds_somaxconn);
	RUN_TEST_CASE(socket_api_listen, marks_socket_accepting);
	RUN_TEST_CASE(socket_api_listen, ebadf_invalid_fd);
	RUN_TEST_CASE(socket_api_listen, enotsock_pipe_fd);
	RUN_TEST_CASE(socket_api_listen, eopnotsupp_dgram_socket);
	RUN_TEST_CASE(socket_api_listen, einval_already_connected);
	RUN_TEST_CASE(socket_api_listen, listen_called_twice);
}
