/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <sys/socket.h>
 * TESTED:
 *    - send()
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

#define SEND_MSG       "hello"
#define SEND_MSG_LEN   5
#define SEND_BUF_SZ    32
#define SEND_EMPTY_LEN 0

static struct {
	int sv[2];
	struct sigaction oldsa;
	int sigaction_changed;
} test_common;

TEST_GROUP(socket_api_send);

TEST_SETUP(socket_api_send)
{
	int ret;
	test_common.sigaction_changed = 0;
	ret = socketpair(AF_UNIX, SOCK_STREAM, 0, test_common.sv);
	TEST_ASSERT_EQUAL_INT(0, ret);
}

TEST_TEAR_DOWN(socket_api_send)
{
	if (test_common.sv[0] >= 0) {
		close(test_common.sv[0]);
		test_common.sv[0] = -1;
	}
	if (test_common.sv[1] >= 0) {
		close(test_common.sv[1]);
		test_common.sv[1] = -1;
	}
	if (test_common.sigaction_changed) {
		alarm(0);
		sigaction(SIGALRM, &test_common.oldsa, NULL);
		test_common.sigaction_changed = 0;
	}
}


TEST(socket_api_send, basic_success)
{
	ssize_t n;
	char buf[SEND_BUF_SZ];

	n = send(test_common.sv[0], SEND_MSG, SEND_MSG_LEN, 0);
	TEST_ASSERT_EQUAL_INT(SEND_MSG_LEN, (int)n);

	memset(buf, 0, sizeof(buf));
	n = recv(test_common.sv[1], buf, sizeof(buf), 0);
	TEST_ASSERT_EQUAL_INT(SEND_MSG_LEN, (int)n);
	TEST_ASSERT_EQUAL_MEMORY(SEND_MSG, buf, SEND_MSG_LEN);
}


TEST(socket_api_send, returns_bytes_sent)
{
	ssize_t n;

	/* Upon successful completion, send() shall return the number of bytes sent */
	n = send(test_common.sv[0], SEND_MSG, SEND_MSG_LEN, 0);
	TEST_ASSERT_EQUAL_INT(SEND_MSG_LEN, (int)n);
}


TEST(socket_api_send, zero_length_message)
{
	ssize_t n;

	/* Sending zero-length message should succeed */
	n = send(test_common.sv[0], SEND_MSG, SEND_EMPTY_LEN, 0);
	TEST_ASSERT_EQUAL_INT(SEND_EMPTY_LEN, (int)n);
}


TEST(socket_api_send, msg_nosignal_flag)
{
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1635 issue");
#endif
	ssize_t n;

	/* Shut down write end of peer so send will get EPIPE */
	close(test_common.sv[1]);
	test_common.sv[1] = -1;

	/* MSG_NOSIGNAL: requests not to send SIGPIPE, EPIPE shall still be returned */
	errno = 0;
	n = send(test_common.sv[0], SEND_MSG, SEND_MSG_LEN, MSG_NOSIGNAL);
	TEST_ASSERT_EQUAL_INT(-1, (int)n);
	TEST_ASSERT_EQUAL_INT(EPIPE, errno);
}


TEST(socket_api_send, ebadf_invalid_fd)
{
	ssize_t n;

	errno = 0;
	n = send(-1, SEND_MSG, SEND_MSG_LEN, 0);
	TEST_ASSERT_EQUAL_INT(-1, (int)n);
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


TEST(socket_api_send, enotsock_not_socket)
{
	ssize_t n;
	int pipefd[2] = { -1, -1 };
	int ret;

	ret = pipe(pipefd);
	TEST_ASSERT_EQUAL_INT(0, ret);
	if (TEST_PROTECT()) {
		errno = 0;
		n = send(pipefd[0], SEND_MSG, SEND_MSG_LEN, 0);
		TEST_ASSERT_EQUAL_INT(-1, (int)n);
		TEST_ASSERT_EQUAL_INT(ENOTSOCK, errno);
	}
	if (pipefd[0] >= 0) {
		close(pipefd[0]);
	}
	if (pipefd[1] >= 0) {
		close(pipefd[1]);
	}
}


TEST(socket_api_send, enotconn_unconnected)
{
	ssize_t n;
	int fd;

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_TRUE(fd >= 0);
	if (TEST_PROTECT()) {
		errno = 0;
		n = send(fd, SEND_MSG, SEND_MSG_LEN, 0);
		TEST_ASSERT_EQUAL_INT(-1, (int)n);
		TEST_ASSERT_EQUAL_INT(ENOTCONN, errno);
	}
	close(fd);
}


TEST(socket_api_send, epipe_after_shutdown_wr)
{
	ssize_t n;
	int ret;

	ret = shutdown(test_common.sv[0], SHUT_WR);
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1634 issue");
#endif
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* send on socket shut down for writing shall fail with EPIPE */
	errno = 0;
	n = send(test_common.sv[0], SEND_MSG, SEND_MSG_LEN, MSG_NOSIGNAL);
	TEST_ASSERT_EQUAL_INT(-1, (int)n);
	TEST_ASSERT_EQUAL_INT(EPIPE, errno);
}


TEST(socket_api_send, ewouldblock_nonblocking)
{
	ssize_t n;
	int ret;
	int flags;
	static char bigBuf[65536];

	flags = fcntl(test_common.sv[0], F_GETFL);
	TEST_ASSERT_TRUE(flags >= 0);

	ret = fcntl(test_common.sv[0], F_SETFL, flags | O_NONBLOCK);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Fill the send buffer until it would block */
	memset(bigBuf, 'A', sizeof(bigBuf));
	while (1) {
		errno = 0;
		n = send(test_common.sv[0], bigBuf, sizeof(bigBuf), MSG_NOSIGNAL);
		if (n == -1) {
			TEST_ASSERT_TRUE(errno == EAGAIN || errno == EWOULDBLOCK);
			break;
		}
	}
}


TEST(socket_api_send, equivalent_to_write_with_flags_zero)
{
	/* send() with flags==0 is equivalent to write() */
	ssize_t n1;
	ssize_t n2;
	char buf[SEND_BUF_SZ];

	n1 = send(test_common.sv[0], SEND_MSG, SEND_MSG_LEN, 0);
	TEST_ASSERT_EQUAL_INT(SEND_MSG_LEN, (int)n1);

	memset(buf, 0, sizeof(buf));
	n2 = read(test_common.sv[1], buf, sizeof(buf));
	TEST_ASSERT_EQUAL_INT(SEND_MSG_LEN, (int)n2);
	TEST_ASSERT_EQUAL_MEMORY(SEND_MSG, buf, SEND_MSG_LEN);
}


TEST(socket_api_send, edestaddrreq_dgram_no_peer)
{
	ssize_t n;
	int fd;

	/* connectionless-mode socket with no peer address set */
	fd = socket(AF_UNIX, SOCK_DGRAM, 0);
	TEST_ASSERT_TRUE(fd >= 0);
	if (TEST_PROTECT()) {
		errno = 0;
		n = send(fd, SEND_MSG, SEND_MSG_LEN, 0);
		TEST_ASSERT_EQUAL_INT(-1, (int)n);
		/* EDESTADDRREQ or ENOTCONN depending on implementation */
		TEST_ASSERT_TRUE(errno == EDESTADDRREQ || errno == ENOTCONN);
	}
	close(fd);
}


static void test_sigAlarmHandler(int sig)
{
	(void)sig;
}


TEST(socket_api_send, eintr_signal_interrupts)
{
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1635 issue");
#endif
	ssize_t n;
	struct sigaction sa;
	int ret;
	int flags;
	static char bigBuf[65536];

	/* Fill the send buffer so next send blocks */
	flags = fcntl(test_common.sv[0], F_GETFL);
	TEST_ASSERT_TRUE(flags >= 0);

	ret = fcntl(test_common.sv[0], F_SETFL, flags | O_NONBLOCK);
	TEST_ASSERT_EQUAL_INT(0, ret);

	memset(bigBuf, 'X', sizeof(bigBuf));
	while (1) {
		n = send(test_common.sv[0], bigBuf, sizeof(bigBuf), MSG_NOSIGNAL);
		if (n == -1) {
			break;
		}
	}

	/* Switch back to blocking */
	ret = fcntl(test_common.sv[0], F_SETFL, flags);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Install SIGALRM handler without SA_RESTART */
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = test_sigAlarmHandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	ret = sigaction(SIGALRM, &sa, &test_common.oldsa);
	TEST_ASSERT_EQUAL_INT(0, ret);
	test_common.sigaction_changed = 1;

	/* Schedule alarm in 1 second */
	alarm(1);

	/* Blocking send should be interrupted */
	errno = 0;
	n = send(test_common.sv[0], bigBuf, sizeof(bigBuf), MSG_NOSIGNAL);
	TEST_ASSERT_EQUAL_INT(-1, (int)n);
	TEST_ASSERT_EQUAL_INT(EINTR, errno);
}


TEST(socket_api_send, emsgsize_dgram_too_large)
{
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1644");
#endif
	int *sv = test_common.sv;
	int ret;
	ssize_t n;
	int sndbuf = 4096;
	socklen_t optlen;
	static char bigBuf[65536];

	close(sv[0]);
	close(sv[1]);

	ret = socketpair(AF_UNIX, SOCK_DGRAM, 0, sv);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = setsockopt(sv[0], SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(sndbuf));
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Read back the effective limit (the kernel may round or double it) */
	optlen = sizeof(sndbuf);
	ret = getsockopt(sv[0], SOL_SOCKET, SO_SNDBUF, &sndbuf, &optlen);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_TRUE((size_t)sndbuf < sizeof(bigBuf));

	/* Single datagram exceeding buffer limit */
	memset(bigBuf, 'Y', sizeof(bigBuf));
	errno = 0;
	n = send(sv[0], bigBuf, (size_t)sndbuf + 1U, 0);
	TEST_ASSERT_EQUAL_INT(-1, (int)n);
	TEST_ASSERT_EQUAL_INT(EMSGSIZE, errno);
}


TEST_GROUP_RUNNER(socket_api_send)
{
	RUN_TEST_CASE(socket_api_send, basic_success);
	RUN_TEST_CASE(socket_api_send, returns_bytes_sent);
	RUN_TEST_CASE(socket_api_send, zero_length_message);
	RUN_TEST_CASE(socket_api_send, msg_nosignal_flag);
	RUN_TEST_CASE(socket_api_send, ebadf_invalid_fd);
	RUN_TEST_CASE(socket_api_send, enotsock_not_socket);
	RUN_TEST_CASE(socket_api_send, enotconn_unconnected);
	RUN_TEST_CASE(socket_api_send, epipe_after_shutdown_wr);
	RUN_TEST_CASE(socket_api_send, ewouldblock_nonblocking);
	RUN_TEST_CASE(socket_api_send, equivalent_to_write_with_flags_zero);
	RUN_TEST_CASE(socket_api_send, edestaddrreq_dgram_no_peer);
	RUN_TEST_CASE(socket_api_send, eintr_signal_interrupts);
	RUN_TEST_CASE(socket_api_send, emsgsize_dgram_too_large);
}
