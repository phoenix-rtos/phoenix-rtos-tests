/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <sys/socket.h>
 * TESTED:
 *    - sendto()
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
#include <signal.h>

#include "unity_fixture.h"

#define SENDTO_MSG       "sendto_test"
#define SENDTO_MSG_LEN   11
#define SENDTO_BUF_SZ    64
#define SENDTO_SOCK_PATH "/tmp/test_sendto_sock"

static struct {
	int sv[2];
} test_common;

TEST_GROUP(socket_api_sendto);

TEST_SETUP(socket_api_sendto)
{
	int ret;

	unlink(SENDTO_SOCK_PATH);
	ret = socketpair(AF_UNIX, SOCK_STREAM, 0, test_common.sv);
	TEST_ASSERT_EQUAL_INT(0, ret);
}

TEST_TEAR_DOWN(socket_api_sendto)
{
	if (test_common.sv[0] >= 0) {
		close(test_common.sv[0]);
		test_common.sv[0] = -1;
	}
	if (test_common.sv[1] >= 0) {
		close(test_common.sv[1]);
		test_common.sv[1] = -1;
	}
	unlink(SENDTO_SOCK_PATH);
}


TEST(socket_api_sendto, basic_connected_stream)
{
	ssize_t n;
	char buf[SENDTO_BUF_SZ];

	/* On connection-mode socket, dest_addr shall be ignored */
	n = sendto(test_common.sv[0], SENDTO_MSG, SENDTO_MSG_LEN, 0, NULL, 0);
	TEST_ASSERT_EQUAL_INT(SENDTO_MSG_LEN, (int)n);

	memset(buf, 0, sizeof(buf));
	n = recv(test_common.sv[1], buf, sizeof(buf), 0);
	TEST_ASSERT_EQUAL_INT(SENDTO_MSG_LEN, (int)n);
	TEST_ASSERT_EQUAL_MEMORY(SENDTO_MSG, buf, SENDTO_MSG_LEN);
}


TEST(socket_api_sendto, returns_bytes_sent)
{
	ssize_t n;

	n = sendto(test_common.sv[0], SENDTO_MSG, SENDTO_MSG_LEN, 0, NULL, 0);
	TEST_ASSERT_EQUAL_INT(SENDTO_MSG_LEN, (int)n);
}


TEST(socket_api_sendto, dgram_with_address)
{
	int recvFd;
	int sendFd;
	int ret;
	ssize_t n;
	char buf[SENDTO_BUF_SZ];
	struct sockaddr_un addr;

	recvFd = socket(AF_UNIX, SOCK_DGRAM, 0);
	TEST_ASSERT_TRUE(recvFd >= 0);

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, SENDTO_SOCK_PATH, sizeof(addr.sun_path) - 1);

	ret = bind(recvFd, (struct sockaddr *)&addr, sizeof(addr));
	TEST_ASSERT_EQUAL_INT(0, ret);

	sendFd = socket(AF_UNIX, SOCK_DGRAM, 0);
	TEST_ASSERT_TRUE(sendFd >= 0);

	/* sendto with explicit destination address */
	n = sendto(sendFd, SENDTO_MSG, SENDTO_MSG_LEN, 0,
		(struct sockaddr *)&addr, sizeof(addr));
	TEST_ASSERT_EQUAL_INT(SENDTO_MSG_LEN, (int)n);

	memset(buf, 0, sizeof(buf));
	n = recvfrom(recvFd, buf, sizeof(buf), 0, NULL, NULL);
	TEST_ASSERT_EQUAL_INT(SENDTO_MSG_LEN, (int)n);
	TEST_ASSERT_EQUAL_MEMORY(SENDTO_MSG, buf, SENDTO_MSG_LEN);

	close(recvFd);
	close(sendFd);
}


TEST(socket_api_sendto, connection_mode_ignores_dest_addr)
{
	ssize_t n;
	char buf[SENDTO_BUF_SZ];
	struct sockaddr_un addr;

	/* For connection-mode socket, dest_addr shall be ignored */
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, "/tmp/nonexistent_path", sizeof(addr.sun_path) - 1);

/* Linux returns EISCONN instead of ignoring dest_addr on connected socket */
#ifndef __phoenix__
	TEST_IGNORE();
#else
	TEST_IGNORE_MESSAGE("#1640 issue");
#endif
	n = sendto(test_common.sv[0], SENDTO_MSG, SENDTO_MSG_LEN, 0,
		(struct sockaddr *)&addr, sizeof(addr));
	TEST_ASSERT_EQUAL_INT(SENDTO_MSG_LEN, (int)n);

	memset(buf, 0, sizeof(buf));
	n = recv(test_common.sv[1], buf, sizeof(buf), 0);
	TEST_ASSERT_EQUAL_INT(SENDTO_MSG_LEN, (int)n);
	TEST_ASSERT_EQUAL_MEMORY(SENDTO_MSG, buf, SENDTO_MSG_LEN);
}


TEST(socket_api_sendto, ebadf_invalid_fd)
{
	ssize_t n;

	errno = 0;
	n = sendto(-1, SENDTO_MSG, SENDTO_MSG_LEN, 0, NULL, 0);
	TEST_ASSERT_EQUAL_INT(-1, (int)n);
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


TEST(socket_api_sendto, enotsock_not_socket)
{
	ssize_t n;
	int pipefd[2];
	int ret;

	ret = pipe(pipefd);
	TEST_ASSERT_EQUAL_INT(0, ret);

	errno = 0;
	n = sendto(pipefd[0], SENDTO_MSG, SENDTO_MSG_LEN, 0, NULL, 0);
	TEST_ASSERT_EQUAL_INT(-1, (int)n);
	TEST_ASSERT_EQUAL_INT(ENOTSOCK, errno);

	close(pipefd[0]);
	close(pipefd[1]);
}


TEST(socket_api_sendto, enotconn_stream_unconnected)
{
	ssize_t n;
	int fd;

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_TRUE(fd >= 0);

	errno = 0;
	n = sendto(fd, SENDTO_MSG, SENDTO_MSG_LEN, 0, NULL, 0);
	TEST_ASSERT_EQUAL_INT(-1, (int)n);
	TEST_ASSERT_EQUAL_INT(ENOTCONN, errno);

	close(fd);
}


TEST(socket_api_sendto, epipe_after_shutdown_wr)
{
	ssize_t n;
	int ret;

	ret = shutdown(test_common.sv[0], SHUT_WR);
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1634 issue");
#endif
	TEST_ASSERT_EQUAL_INT(0, ret);

	errno = 0;
	n = sendto(test_common.sv[0], SENDTO_MSG, SENDTO_MSG_LEN, MSG_NOSIGNAL, NULL, 0);
	TEST_ASSERT_EQUAL_INT(-1, (int)n);
	TEST_ASSERT_EQUAL_INT(EPIPE, errno);
}


TEST(socket_api_sendto, epipe_peer_closed)
{
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1635 issue");
#endif
	ssize_t n;

	close(test_common.sv[1]);
	test_common.sv[1] = -1;

	errno = 0;
	n = sendto(test_common.sv[0], SENDTO_MSG, SENDTO_MSG_LEN, MSG_NOSIGNAL, NULL, 0);
	TEST_ASSERT_EQUAL_INT(-1, (int)n);
	TEST_ASSERT_EQUAL_INT(EPIPE, errno);
}


TEST(socket_api_sendto, ewouldblock_nonblocking)
{
	ssize_t n;
	int ret;
	int flags;
	static char bigBuf[65536];

	flags = fcntl(test_common.sv[0], F_GETFL);
	TEST_ASSERT_TRUE(flags >= 0);

	ret = fcntl(test_common.sv[0], F_SETFL, flags | O_NONBLOCK);
	TEST_ASSERT_EQUAL_INT(0, ret);

	memset(bigBuf, 'B', sizeof(bigBuf));
	while (1) {
		errno = 0;
		n = sendto(test_common.sv[0], bigBuf, sizeof(bigBuf), MSG_NOSIGNAL, NULL, 0);
		if (n == -1) {
			TEST_ASSERT_TRUE(errno == EAGAIN || errno == EWOULDBLOCK);
			break;
		}
	}
}


TEST(socket_api_sendto, enoent_unix_path_missing)
{
	int fd;
	ssize_t n;
	struct sockaddr_un addr;

	fd = socket(AF_UNIX, SOCK_DGRAM, 0);
	TEST_ASSERT_TRUE(fd >= 0);

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, "/tmp/nonexistent_sendto_path_xyz", sizeof(addr.sun_path) - 1);

	errno = 0;
	n = sendto(fd, SENDTO_MSG, SENDTO_MSG_LEN, 0,
		(struct sockaddr *)&addr, sizeof(addr));
	TEST_ASSERT_EQUAL_INT(-1, (int)n);
	/* ENOENT or ECONNREFUSED depending on implementation */
	TEST_ASSERT_TRUE(errno == ENOENT || errno == ECONNREFUSED);

	close(fd);
}


TEST(socket_api_sendto, msg_nosignal_flag)
{
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1635 issue");
#endif
	ssize_t n;

	close(test_common.sv[1]);
	test_common.sv[1] = -1;

	/* MSG_NOSIGNAL prevents SIGPIPE, but EPIPE error is still returned */
	errno = 0;
	n = sendto(test_common.sv[0], SENDTO_MSG, SENDTO_MSG_LEN, MSG_NOSIGNAL, NULL, 0);
	TEST_ASSERT_EQUAL_INT(-1, (int)n);
	TEST_ASSERT_EQUAL_INT(EPIPE, errno);
}


TEST(socket_api_sendto, emsgsize_dgram_too_large)
{
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1644");
#endif
	int sv[2];
	int ret;
	ssize_t n;
	int sndbuf;
	socklen_t optlen;
	static char bigBuf[262144];

	ret = socketpair(AF_UNIX, SOCK_DGRAM, 0, sv);
	TEST_ASSERT_EQUAL_INT(0, ret);

	optlen = sizeof(sndbuf);
	ret = getsockopt(sv[0], SOL_SOCKET, SO_SNDBUF, &sndbuf, &optlen);
	TEST_ASSERT_EQUAL_INT(0, ret);

	memset(bigBuf, 'Z', sizeof(bigBuf));
	errno = 0;
	n = sendto(sv[0], bigBuf, (size_t)sndbuf + 1U > sizeof(bigBuf) ? sizeof(bigBuf) : (size_t)sndbuf + 1U, 0, NULL, 0);
	TEST_ASSERT_EQUAL_INT(-1, (int)n);
	TEST_ASSERT_EQUAL_INT(EMSGSIZE, errno);

	close(sv[0]);
	close(sv[1]);
}


TEST_GROUP_RUNNER(socket_api_sendto)
{
	RUN_TEST_CASE(socket_api_sendto, basic_connected_stream);
	RUN_TEST_CASE(socket_api_sendto, returns_bytes_sent);
	RUN_TEST_CASE(socket_api_sendto, dgram_with_address);
	RUN_TEST_CASE(socket_api_sendto, connection_mode_ignores_dest_addr);
	RUN_TEST_CASE(socket_api_sendto, ebadf_invalid_fd);
	RUN_TEST_CASE(socket_api_sendto, enotsock_not_socket);
	RUN_TEST_CASE(socket_api_sendto, enotconn_stream_unconnected);
	RUN_TEST_CASE(socket_api_sendto, epipe_after_shutdown_wr);
	RUN_TEST_CASE(socket_api_sendto, epipe_peer_closed);
	RUN_TEST_CASE(socket_api_sendto, ewouldblock_nonblocking);
	RUN_TEST_CASE(socket_api_sendto, enoent_unix_path_missing);
	RUN_TEST_CASE(socket_api_sendto, msg_nosignal_flag);
	RUN_TEST_CASE(socket_api_sendto, emsgsize_dgram_too_large);
}
