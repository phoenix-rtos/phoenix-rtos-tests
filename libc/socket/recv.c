/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <sys/socket.h>
 * TESTED:
 *    - recv()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Lukasz Kruszynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>

#include "unity_fixture.h"

#define RECV_MSG     "hello"
#define RECV_MSG_LEN 5
#define RECV_BUF_SZ  32

static struct {
	int sv[2];
} test_common;


TEST_GROUP(socket_api_recv);

TEST_SETUP(socket_api_recv)
{
	int ret;

	ret = socketpair(AF_UNIX, SOCK_STREAM, 0, test_common.sv);
	TEST_ASSERT_EQUAL_INT(0, ret);
}

TEST_TEAR_DOWN(socket_api_recv)
{
	if (test_common.sv[0] >= 0) {
		close(test_common.sv[0]);
		test_common.sv[0] = -1;
	}
	if (test_common.sv[1] >= 0) {
		close(test_common.sv[1]);
		test_common.sv[1] = -1;
	}
}


TEST(socket_api_recv, basic_success)
{
	ssize_t n;
	char buf[RECV_BUF_SZ];

	n = send(test_common.sv[0], RECV_MSG, RECV_MSG_LEN, 0);
	TEST_ASSERT_EQUAL_INT(RECV_MSG_LEN, (int)n);

	memset(buf, 0, sizeof(buf));
	n = recv(test_common.sv[1], buf, sizeof(buf), 0);
	TEST_ASSERT_EQUAL_INT(RECV_MSG_LEN, (int)n);
	TEST_ASSERT_EQUAL_MEMORY(RECV_MSG, buf, RECV_MSG_LEN);
}


TEST(socket_api_recv, returns_message_length)
{
	ssize_t n;
	char buf[RECV_BUF_SZ];
	const char *msg = "abcdefghij";
	const size_t msgLen = 10;

	n = send(test_common.sv[0], msg, msgLen, 0);
	TEST_ASSERT_EQUAL_INT((int)msgLen, (int)n);

	memset(buf, 0, sizeof(buf));
	n = recv(test_common.sv[1], buf, sizeof(buf), 0);
	TEST_ASSERT_EQUAL_INT((int)msgLen, (int)n);
	TEST_ASSERT_EQUAL_MEMORY(msg, buf, msgLen);
}


TEST(socket_api_recv, partial_read_stream)
{
	ssize_t n;
	char buf[4];
	const char *msg = "abcdefgh";
	const size_t msgLen = 8;

	n = send(test_common.sv[0], msg, msgLen, 0);
	TEST_ASSERT_EQUAL_INT((int)msgLen, (int)n);

	/* Stream socket: read only 4 bytes, rest remains available */
	memset(buf, 0, sizeof(buf));
	n = recv(test_common.sv[1], buf, sizeof(buf), 0);
	TEST_ASSERT_EQUAL_INT(4, (int)n);
	TEST_ASSERT_EQUAL_MEMORY("abcd", buf, 4);

	/* Read remaining data */
	memset(buf, 0, sizeof(buf));
	n = recv(test_common.sv[1], buf, sizeof(buf), 0);
	TEST_ASSERT_EQUAL_INT(4, (int)n);
	TEST_ASSERT_EQUAL_MEMORY("efgh", buf, 4);
}


TEST(socket_api_recv, msg_peek)
{
	ssize_t n;
	char buf[RECV_BUF_SZ];

	n = send(test_common.sv[0], RECV_MSG, RECV_MSG_LEN, 0);
	TEST_ASSERT_EQUAL_INT(RECV_MSG_LEN, (int)n);

	/* MSG_PEEK: data is treated as unread */
	memset(buf, 0, sizeof(buf));
	n = recv(test_common.sv[1], buf, sizeof(buf), MSG_PEEK);
	TEST_ASSERT_EQUAL_INT(RECV_MSG_LEN, (int)n);
	TEST_ASSERT_EQUAL_MEMORY(RECV_MSG, buf, RECV_MSG_LEN);

	/* Next recv shall still return the same data */
	memset(buf, 0, sizeof(buf));
	n = recv(test_common.sv[1], buf, sizeof(buf), 0);
	TEST_ASSERT_EQUAL_INT(RECV_MSG_LEN, (int)n);
	TEST_ASSERT_EQUAL_MEMORY(RECV_MSG, buf, RECV_MSG_LEN);
}


TEST(socket_api_recv, msg_waitall)
{
	ssize_t n;
	char buf[RECV_BUF_SZ];

	n = send(test_common.sv[0], RECV_MSG, RECV_MSG_LEN, 0);
	TEST_ASSERT_EQUAL_INT(RECV_MSG_LEN, (int)n);

	/* Close writer so MSG_WAITALL returns what's available */
	close(test_common.sv[0]);
	test_common.sv[0] = -1;

	memset(buf, 0, sizeof(buf));
	n = recv(test_common.sv[1], buf, sizeof(buf), MSG_WAITALL);
	TEST_ASSERT_EQUAL_INT(RECV_MSG_LEN, (int)n);
	TEST_ASSERT_EQUAL_MEMORY(RECV_MSG, buf, RECV_MSG_LEN);
}


TEST(socket_api_recv, orderly_shutdown_returns_zero)
{
	ssize_t n;
	char buf[RECV_BUF_SZ];

	/* Close the writing end */
	close(test_common.sv[0]);
	test_common.sv[0] = -1;

	/* Peer has performed orderly shutdown: recv shall return 0 */
	n = recv(test_common.sv[1], buf, sizeof(buf), 0);
	TEST_ASSERT_EQUAL_INT(0, (int)n);
}


TEST(socket_api_recv, eagain_nonblock_no_data)
{
	ssize_t n;
	int ret;
	int flags;
	char buf[RECV_BUF_SZ];

	/* Set non-blocking */
	flags = fcntl(test_common.sv[1], F_GETFL, 0);
	TEST_ASSERT_TRUE(flags >= 0);
	ret = fcntl(test_common.sv[1], F_SETFL, flags | O_NONBLOCK);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* No data available, O_NONBLOCK set */
	errno = 0;
	n = recv(test_common.sv[1], buf, sizeof(buf), 0);
	TEST_ASSERT_EQUAL_INT(-1, (int)n);
	TEST_ASSERT_TRUE(errno == EAGAIN || errno == EWOULDBLOCK);
}


TEST(socket_api_recv, ebadf_invalid_fd)
{
	ssize_t n;
	char buf[RECV_BUF_SZ];

	errno = 0;
	n = recv(-1, buf, sizeof(buf), 0);
	TEST_ASSERT_EQUAL_INT(-1, (int)n);
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


TEST(socket_api_recv, enotsock_pipe_fd)
{
	ssize_t n;
	int ret;
	int pipefd[2];
	char buf[RECV_BUF_SZ];

	ret = pipe(pipefd);
	TEST_ASSERT_EQUAL_INT(0, ret);

	errno = 0;
	n = recv(pipefd[0], buf, sizeof(buf), 0);
	TEST_ASSERT_EQUAL_INT(-1, (int)n);
	TEST_ASSERT_EQUAL_INT(ENOTSOCK, errno);

	close(pipefd[0]);
	close(pipefd[1]);
}


TEST(socket_api_recv, enotconn_unconnected_stream)
{
	ssize_t n;
	int fd;
	char buf[RECV_BUF_SZ];

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_TRUE(fd >= 0);

	errno = 0;
	n = recv(fd, buf, sizeof(buf), 0);
	TEST_ASSERT_EQUAL_INT(-1, (int)n);
#ifndef __phoenix__
	/* glibc may return EINVAL instead of ENOTCONN on unbound AF_UNIX stream socket */
	TEST_ASSERT_TRUE(errno == ENOTCONN || errno == EINVAL);
#else
	TEST_ASSERT_EQUAL_INT(ENOTCONN, errno);
#endif

	close(fd);
}


TEST(socket_api_recv, multiple_sends_single_recv)
{
	ssize_t n;
	char buf[RECV_BUF_SZ];

	/* Send two messages */
	n = send(test_common.sv[0], "abc", 3, 0);
	TEST_ASSERT_EQUAL_INT(3, (int)n);
	n = send(test_common.sv[0], "def", 3, 0);
	TEST_ASSERT_EQUAL_INT(3, (int)n);

	/* Stream socket: boundaries are ignored, single recv gets all */
	memset(buf, 0, sizeof(buf));
	n = recv(test_common.sv[1], buf, sizeof(buf), 0);
	TEST_ASSERT_GREATER_THAN_INT(0, (int)n);
	/* At least 3 bytes should be available (may be 6 if coalesced) */
	TEST_ASSERT_TRUE(n >= 3);
	TEST_ASSERT_EQUAL_MEMORY("abc", buf, 3);
}


TEST_GROUP_RUNNER(socket_api_recv)
{
	RUN_TEST_CASE(socket_api_recv, basic_success);
	RUN_TEST_CASE(socket_api_recv, returns_message_length);
	RUN_TEST_CASE(socket_api_recv, partial_read_stream);
	RUN_TEST_CASE(socket_api_recv, msg_peek);
	RUN_TEST_CASE(socket_api_recv, msg_waitall);
	RUN_TEST_CASE(socket_api_recv, orderly_shutdown_returns_zero);
	RUN_TEST_CASE(socket_api_recv, eagain_nonblock_no_data);
	RUN_TEST_CASE(socket_api_recv, ebadf_invalid_fd);
	RUN_TEST_CASE(socket_api_recv, enotsock_pipe_fd);
	RUN_TEST_CASE(socket_api_recv, enotconn_unconnected_stream);
	RUN_TEST_CASE(socket_api_recv, multiple_sends_single_recv);
}
