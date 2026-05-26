/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <sys/socket.h>
 * TESTED:
 *    - sendmsg()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Lukasz Kruszynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* sendmsg() is declared with a warning attribute on Phoenix-RTOS; suppress it since we are testing this function */
#pragma GCC diagnostic ignored "-Wattribute-warning"

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/uio.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>

#include "unity_fixture.h"

/* IOV_MAX may not be defined as a macro on all systems */
#ifndef IOV_MAX
#define IOV_MAX 1024
#endif

#define SENDMSG_MSG1      "hello"
#define SENDMSG_MSG1_LEN  5
#define SENDMSG_MSG2      "world"
#define SENDMSG_MSG2_LEN  5
#define SENDMSG_BUF_SZ    64
#define SENDMSG_SOCK_PATH "/tmp/test_sendmsg_sock"

static struct {
	int sv[2];
} test_common;

TEST_GROUP(socket_api_sendmsg);

TEST_SETUP(socket_api_sendmsg)
{
	int ret;

	unlink(SENDMSG_SOCK_PATH);
	ret = socketpair(AF_UNIX, SOCK_STREAM, 0, test_common.sv);
	TEST_ASSERT_EQUAL_INT(0, ret);
}

TEST_TEAR_DOWN(socket_api_sendmsg)
{
	if (test_common.sv[0] >= 0) {
		close(test_common.sv[0]);
		test_common.sv[0] = -1;
	}
	if (test_common.sv[1] >= 0) {
		close(test_common.sv[1]);
		test_common.sv[1] = -1;
	}
	unlink(SENDMSG_SOCK_PATH);
}


TEST(socket_api_sendmsg, basic_single_iov)
{
	ssize_t n;
	struct msghdr msg;
	struct iovec iov;
	char buf[SENDMSG_BUF_SZ];

	memset(&msg, 0, sizeof(msg));
	iov.iov_base = (void *)SENDMSG_MSG1;
	iov.iov_len = SENDMSG_MSG1_LEN;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	n = sendmsg(test_common.sv[0], &msg, 0);
	TEST_ASSERT_EQUAL_INT(SENDMSG_MSG1_LEN, (int)n);

	memset(buf, 0, sizeof(buf));
	n = recv(test_common.sv[1], buf, sizeof(buf), 0);
	TEST_ASSERT_EQUAL_INT(SENDMSG_MSG1_LEN, (int)n);
	TEST_ASSERT_EQUAL_MEMORY(SENDMSG_MSG1, buf, SENDMSG_MSG1_LEN);
}


TEST(socket_api_sendmsg, scatter_gather_multiple_iov)
{
	ssize_t n;
	struct msghdr msg;
	struct iovec iov[2];
	char buf[SENDMSG_BUF_SZ];
	const int totalLen = SENDMSG_MSG1_LEN + SENDMSG_MSG2_LEN;

	memset(&msg, 0, sizeof(msg));
	iov[0].iov_base = (void *)SENDMSG_MSG1;
	iov[0].iov_len = SENDMSG_MSG1_LEN;
	iov[1].iov_base = (void *)SENDMSG_MSG2;
	iov[1].iov_len = SENDMSG_MSG2_LEN;
	msg.msg_iov = iov;
	msg.msg_iovlen = 2;

	/* Data from each iov is sent in turn */
	n = sendmsg(test_common.sv[0], &msg, 0);
	TEST_ASSERT_EQUAL_INT(totalLen, (int)n);

	memset(buf, 0, sizeof(buf));
	n = recv(test_common.sv[1], buf, sizeof(buf), 0);
	TEST_ASSERT_EQUAL_INT(totalLen, (int)n);
	TEST_ASSERT_EQUAL_MEMORY(SENDMSG_MSG1, buf, SENDMSG_MSG1_LEN);
	TEST_ASSERT_EQUAL_MEMORY(SENDMSG_MSG2, buf + SENDMSG_MSG1_LEN, SENDMSG_MSG2_LEN);
}


TEST(socket_api_sendmsg, zero_length_iov)
{
	ssize_t n;
	struct msghdr msg;
	struct iovec iov;

	memset(&msg, 0, sizeof(msg));
	iov.iov_base = (void *)SENDMSG_MSG1;
	iov.iov_len = 0;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	/* Zero-length send should succeed returning 0 */
	n = sendmsg(test_common.sv[0], &msg, 0);
	TEST_ASSERT_EQUAL_INT(0, (int)n);
}


TEST(socket_api_sendmsg, returns_bytes_sent)
{
	ssize_t n;
	struct msghdr msg;
	struct iovec iov;

	memset(&msg, 0, sizeof(msg));
	iov.iov_base = (void *)SENDMSG_MSG1;
	iov.iov_len = SENDMSG_MSG1_LEN;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	n = sendmsg(test_common.sv[0], &msg, 0);
	TEST_ASSERT_EQUAL_INT(SENDMSG_MSG1_LEN, (int)n);
}


TEST(socket_api_sendmsg, msg_flags_ignored)
{
	ssize_t n;
	struct msghdr msg;
	struct iovec iov;
	char buf[SENDMSG_BUF_SZ];

	/* The msg_flags member is ignored on send */
	memset(&msg, 0, sizeof(msg));
	iov.iov_base = (void *)SENDMSG_MSG1;
	iov.iov_len = SENDMSG_MSG1_LEN;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_flags = 0x7fff; /* arbitrary value - should be ignored */

	n = sendmsg(test_common.sv[0], &msg, 0);
	TEST_ASSERT_EQUAL_INT(SENDMSG_MSG1_LEN, (int)n);

	memset(buf, 0, sizeof(buf));
	n = recv(test_common.sv[1], buf, sizeof(buf), 0);
	TEST_ASSERT_EQUAL_INT(SENDMSG_MSG1_LEN, (int)n);
	TEST_ASSERT_EQUAL_MEMORY(SENDMSG_MSG1, buf, SENDMSG_MSG1_LEN);
}


TEST(socket_api_sendmsg, dgram_with_dest_addr)
{
	int recvFd;
	int sendFd;
	int ret;
	ssize_t n;
	struct msghdr msg;
	struct iovec iov;
	struct sockaddr_un addr;
	char buf[SENDMSG_BUF_SZ];

	recvFd = socket(AF_UNIX, SOCK_DGRAM, 0);
	TEST_ASSERT_TRUE(recvFd >= 0);

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, SENDMSG_SOCK_PATH, sizeof(addr.sun_path) - 1);

	ret = bind(recvFd, (struct sockaddr *)&addr, sizeof(addr));
	TEST_ASSERT_EQUAL_INT(0, ret);

	sendFd = socket(AF_UNIX, SOCK_DGRAM, 0);
	TEST_ASSERT_TRUE(sendFd >= 0);

	memset(&msg, 0, sizeof(msg));
	iov.iov_base = (void *)SENDMSG_MSG1;
	iov.iov_len = SENDMSG_MSG1_LEN;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_name = &addr;
	msg.msg_namelen = sizeof(addr);

	n = sendmsg(sendFd, &msg, 0);
	TEST_ASSERT_EQUAL_INT(SENDMSG_MSG1_LEN, (int)n);

	memset(buf, 0, sizeof(buf));
	n = recvfrom(recvFd, buf, sizeof(buf), 0, NULL, NULL);
	TEST_ASSERT_EQUAL_INT(SENDMSG_MSG1_LEN, (int)n);
	TEST_ASSERT_EQUAL_MEMORY(SENDMSG_MSG1, buf, SENDMSG_MSG1_LEN);

	close(recvFd);
	close(sendFd);
}


TEST(socket_api_sendmsg, ebadf_invalid_fd)
{
	ssize_t n;
	struct msghdr msg;
	struct iovec iov;

	memset(&msg, 0, sizeof(msg));
	iov.iov_base = (void *)SENDMSG_MSG1;
	iov.iov_len = SENDMSG_MSG1_LEN;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	errno = 0;
	n = sendmsg(-1, &msg, 0);
	TEST_ASSERT_EQUAL_INT(-1, (int)n);
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


TEST(socket_api_sendmsg, enotsock_not_socket)
{
	ssize_t n;
	struct msghdr msg;
	struct iovec iov;
	int pipefd[2];
	int ret;

	ret = pipe(pipefd);
	TEST_ASSERT_EQUAL_INT(0, ret);

	memset(&msg, 0, sizeof(msg));
	iov.iov_base = (void *)SENDMSG_MSG1;
	iov.iov_len = SENDMSG_MSG1_LEN;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	errno = 0;
	n = sendmsg(pipefd[0], &msg, 0);
	TEST_ASSERT_EQUAL_INT(-1, (int)n);
	TEST_ASSERT_EQUAL_INT(ENOTSOCK, errno);

	close(pipefd[0]);
	close(pipefd[1]);
}


TEST(socket_api_sendmsg, enotconn_unconnected)
{
	ssize_t n;
	struct msghdr msg;
	struct iovec iov;
	int fd;

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_TRUE(fd >= 0);

	memset(&msg, 0, sizeof(msg));
	iov.iov_base = (void *)SENDMSG_MSG1;
	iov.iov_len = SENDMSG_MSG1_LEN;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	errno = 0;
	n = sendmsg(fd, &msg, 0);
	TEST_ASSERT_EQUAL_INT(-1, (int)n);
	TEST_ASSERT_EQUAL_INT(ENOTCONN, errno);

	close(fd);
}


TEST(socket_api_sendmsg, epipe_after_shutdown_wr)
{
	ssize_t n;
	struct msghdr msg;
	struct iovec iov;
	int ret;

	ret = shutdown(test_common.sv[0], SHUT_WR);
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1634 issue");
#endif
	TEST_ASSERT_EQUAL_INT(0, ret);

	memset(&msg, 0, sizeof(msg));
	iov.iov_base = (void *)SENDMSG_MSG1;
	iov.iov_len = SENDMSG_MSG1_LEN;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	errno = 0;
	n = sendmsg(test_common.sv[0], &msg, MSG_NOSIGNAL);
	TEST_ASSERT_EQUAL_INT(-1, (int)n);
	TEST_ASSERT_EQUAL_INT(EPIPE, errno);
}


TEST(socket_api_sendmsg, ewouldblock_nonblocking)
{
	ssize_t n;
	struct msghdr msg;
	struct iovec iov;
	int ret;
	int flags;
	static char bigBuf[65536];

	flags = fcntl(test_common.sv[0], F_GETFL);
	TEST_ASSERT_TRUE(flags >= 0);

	ret = fcntl(test_common.sv[0], F_SETFL, flags | O_NONBLOCK);
	TEST_ASSERT_EQUAL_INT(0, ret);

	memset(bigBuf, 'C', sizeof(bigBuf));
	memset(&msg, 0, sizeof(msg));
	iov.iov_base = bigBuf;
	iov.iov_len = sizeof(bigBuf);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	while (1) {
		errno = 0;
		n = sendmsg(test_common.sv[0], &msg, MSG_NOSIGNAL);
		if (n == -1) {
			TEST_ASSERT_TRUE(errno == EAGAIN || errno == EWOULDBLOCK);
			break;
		}
	}
}


TEST(socket_api_sendmsg, connection_mode_ignores_dest_addr)
{
	ssize_t n;
	struct msghdr msg;
	struct iovec iov;
	struct sockaddr_un addr;
	char buf[SENDMSG_BUF_SZ];

	/* On connection-mode socket, destination address in msghdr shall be ignored */
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, "/tmp/nonexistent", sizeof(addr.sun_path) - 1);

	memset(&msg, 0, sizeof(msg));
	iov.iov_base = (void *)SENDMSG_MSG1;
	iov.iov_len = SENDMSG_MSG1_LEN;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_name = &addr;
	msg.msg_namelen = sizeof(addr);

#ifndef __phoenix__
	TEST_IGNORE_MESSAGE("Linux returns EISCONN instead of ignoring dest_addr on connected socket");
#else
	TEST_IGNORE_MESSAGE("#1640 issue");
#endif
	n = sendmsg(test_common.sv[0], &msg, 0);
	TEST_ASSERT_EQUAL_INT(SENDMSG_MSG1_LEN, (int)n);

	memset(buf, 0, sizeof(buf));
	n = recv(test_common.sv[1], buf, sizeof(buf), 0);
	TEST_ASSERT_EQUAL_INT(SENDMSG_MSG1_LEN, (int)n);
	TEST_ASSERT_EQUAL_MEMORY(SENDMSG_MSG1, buf, SENDMSG_MSG1_LEN);
}


TEST(socket_api_sendmsg, msg_nosignal_flag)
{
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1635 issue");
#endif
	ssize_t n;
	struct msghdr msg;
	struct iovec iov;

	close(test_common.sv[1]);
	test_common.sv[1] = -1;

	memset(&msg, 0, sizeof(msg));
	iov.iov_base = (void *)SENDMSG_MSG1;
	iov.iov_len = SENDMSG_MSG1_LEN;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	/* MSG_NOSIGNAL prevents SIGPIPE, EPIPE error still returned */
	errno = 0;
	n = sendmsg(test_common.sv[0], &msg, MSG_NOSIGNAL);
	TEST_ASSERT_EQUAL_INT(-1, (int)n);
	TEST_ASSERT_EQUAL_INT(EPIPE, errno);
}


TEST(socket_api_sendmsg, scm_rights_send_fd)
{
	/* Send a file descriptor over AF_UNIX socket using SCM_RIGHTS */
	int sv[2];
	int ret;
	int pipefd[2];
	ssize_t n;
	struct msghdr msg;
	struct iovec iov;
	struct msghdr rmsg;
	struct iovec riov;
	char buf[SENDMSG_BUF_SZ];
	char ctrl[CMSG_SPACE(sizeof(int))];
	char rctrl[CMSG_SPACE(sizeof(int))];
	struct cmsghdr *cmsg;
	int receivedFd;

	ret = socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pipe(pipefd);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Build message with ancillary data carrying pipefd[0] */
	memset(&msg, 0, sizeof(msg));
	iov.iov_base = (void *)SENDMSG_MSG1;
	iov.iov_len = SENDMSG_MSG1_LEN;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = ctrl;
	msg.msg_controllen = sizeof(ctrl);

	cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;
	cmsg->cmsg_len = CMSG_LEN(sizeof(int));
	memcpy(CMSG_DATA(cmsg), &pipefd[0], sizeof(int));

	n = sendmsg(sv[0], &msg, 0);
	TEST_ASSERT_EQUAL_INT(SENDMSG_MSG1_LEN, (int)n);

	/* Receive on the other end */
	memset(&rmsg, 0, sizeof(rmsg));
	memset(buf, 0, sizeof(buf));
	riov.iov_base = buf;
	riov.iov_len = sizeof(buf);
	rmsg.msg_iov = &riov;
	rmsg.msg_iovlen = 1;
	rmsg.msg_control = rctrl;
	rmsg.msg_controllen = sizeof(rctrl);

	n = recvmsg(sv[1], &rmsg, 0);
	TEST_ASSERT_EQUAL_INT(SENDMSG_MSG1_LEN, (int)n);
	TEST_ASSERT_EQUAL_MEMORY(SENDMSG_MSG1, buf, SENDMSG_MSG1_LEN);

	/* Extract the received file descriptor */
	cmsg = CMSG_FIRSTHDR(&rmsg);
	TEST_ASSERT_NOT_NULL(cmsg);
	TEST_ASSERT_EQUAL_INT(SOL_SOCKET, cmsg->cmsg_level);
	TEST_ASSERT_EQUAL_INT(SCM_RIGHTS, cmsg->cmsg_type);
	memcpy(&receivedFd, CMSG_DATA(cmsg), sizeof(int));
	TEST_ASSERT_TRUE(receivedFd >= 0);

	close(receivedFd);
	close(pipefd[0]);
	close(pipefd[1]);
	close(sv[0]);
	close(sv[1]);
}


static void test_sigAlarmHandler(int sig)
{
	(void)sig;
}


TEST(socket_api_sendmsg, eintr_signal_interrupts)
{
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1635 issue");
#endif
	ssize_t n;
	struct msghdr msg;
	struct iovec iov;
	struct sigaction sa;
	struct sigaction oldsa;
	int ret;
	int flags;
	static char bigBuf[65536];

	/* Fill the send buffer so next sendmsg blocks */
	flags = fcntl(test_common.sv[0], F_GETFL);
	TEST_ASSERT_TRUE(flags >= 0);

	ret = fcntl(test_common.sv[0], F_SETFL, flags | O_NONBLOCK);
	TEST_ASSERT_EQUAL_INT(0, ret);

	memset(bigBuf, 'D', sizeof(bigBuf));
	while (1) {
		n = send(test_common.sv[0], bigBuf, sizeof(bigBuf), MSG_NOSIGNAL);
		if (n == -1) {
			break;
		}
	}

	/* Switch back to blocking */
	ret = fcntl(test_common.sv[0], F_SETFL, flags);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Install SIGALRM handler */
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = test_sigAlarmHandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0; /* no SA_RESTART */
	ret = sigaction(SIGALRM, &sa, &oldsa);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Schedule alarm in 1 second */
	alarm(1);

	/* Blocking sendmsg should be interrupted by SIGALRM */
	memset(&msg, 0, sizeof(msg));
	iov.iov_base = bigBuf;
	iov.iov_len = sizeof(bigBuf);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	errno = 0;
	n = sendmsg(test_common.sv[0], &msg, MSG_NOSIGNAL);
	TEST_ASSERT_EQUAL_INT(-1, (int)n);
	TEST_ASSERT_EQUAL_INT(EINTR, errno);

	/* Restore signal handler */
	alarm(0);
	sigaction(SIGALRM, &oldsa, NULL);
}


TEST(socket_api_sendmsg, emsgsize_dgram_too_large)
{
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1644");
#endif
	int sv[2];
	int ret;
	ssize_t n;
	struct msghdr msg;
	struct iovec iov;
	int sndbuf;
	socklen_t optlen;
	static char bigBuf[262144];

	ret = socketpair(AF_UNIX, SOCK_DGRAM, 0, sv);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Get the send buffer size to know the limit */
	optlen = sizeof(sndbuf);
	ret = getsockopt(sv[0], SOL_SOCKET, SO_SNDBUF, &sndbuf, &optlen);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Try to send a message larger than the socket buffer */
	memset(bigBuf, 'E', sizeof(bigBuf));
	memset(&msg, 0, sizeof(msg));
	iov.iov_base = bigBuf;
	iov.iov_len = (size_t)sndbuf + 1U;
	if (iov.iov_len > sizeof(bigBuf)) {
		iov.iov_len = sizeof(bigBuf);
	}
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	errno = 0;
	n = sendmsg(sv[0], &msg, 0);
	TEST_ASSERT_EQUAL_INT(-1, (int)n);
	TEST_ASSERT_EQUAL_INT(EMSGSIZE, errno);

	close(sv[0]);
	close(sv[1]);
}


TEST(socket_api_sendmsg, iovlen_exceeds_iov_max)
{
	ssize_t n;
	struct msghdr msg;
	struct iovec iov;

	memset(&msg, 0, sizeof(msg));
	iov.iov_base = (void *)SENDMSG_MSG1;
	iov.iov_len = SENDMSG_MSG1_LEN;
	msg.msg_iov = &iov;

	/* msg_iovlen > IOV_MAX must fail */
	msg.msg_iovlen = IOV_MAX + 1;
	errno = 0;
	n = sendmsg(test_common.sv[0], &msg, 0);
	TEST_ASSERT_EQUAL_INT(-1, (int)n);
	TEST_ASSERT_TRUE(errno == EMSGSIZE || errno == EINVAL);
}


TEST(socket_api_sendmsg, iovlen_zero_or_negative)
{
	ssize_t n;
	struct msghdr msg;
	struct iovec iov;

	memset(&msg, 0, sizeof(msg));
	iov.iov_base = (void *)SENDMSG_MSG1;
	iov.iov_len = SENDMSG_MSG1_LEN;
	msg.msg_iov = &iov;

	/* msg_iovlen of 0: POSIX says send 0 bytes or fail */
	msg.msg_iovlen = 0;
	errno = 0;
	n = sendmsg(test_common.sv[0], &msg, 0);
	/* Some implementations return 0 (send nothing), others return -1/EINVAL */
	TEST_ASSERT_TRUE(n == 0 || (n == -1 && errno == EINVAL));
}


TEST(socket_api_sendmsg, invalid_cmsg_len)
{
	int sv[2];
	int ret;
	ssize_t n;
	struct msghdr msg;
	struct iovec iov;
	char ctrl[CMSG_SPACE(sizeof(int))];
	struct cmsghdr *cmsg;

	ret = socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
	TEST_ASSERT_EQUAL_INT(0, ret);

	memset(&msg, 0, sizeof(msg));
	iov.iov_base = (void *)SENDMSG_MSG1;
	iov.iov_len = SENDMSG_MSG1_LEN;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = ctrl;
	msg.msg_controllen = sizeof(ctrl);

	/* Set cmsg_len smaller than sizeof(struct cmsghdr) - malformed */
	cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;
	cmsg->cmsg_len = sizeof(struct cmsghdr) - 1;

	errno = 0;
	n = sendmsg(sv[0], &msg, 0);
	/* POSIX does not mandate a specific errno for malformed ancillary data */
	TEST_ASSERT_EQUAL_INT(-1, (int)n);
	TEST_ASSERT_TRUE(errno != 0);

	close(sv[0]);
	close(sv[1]);
}


TEST_GROUP_RUNNER(socket_api_sendmsg)
{
	RUN_TEST_CASE(socket_api_sendmsg, basic_single_iov);
	RUN_TEST_CASE(socket_api_sendmsg, scatter_gather_multiple_iov);
	RUN_TEST_CASE(socket_api_sendmsg, zero_length_iov);
	RUN_TEST_CASE(socket_api_sendmsg, returns_bytes_sent);
	RUN_TEST_CASE(socket_api_sendmsg, msg_flags_ignored);
	RUN_TEST_CASE(socket_api_sendmsg, dgram_with_dest_addr);
	RUN_TEST_CASE(socket_api_sendmsg, ebadf_invalid_fd);
	RUN_TEST_CASE(socket_api_sendmsg, enotsock_not_socket);
	RUN_TEST_CASE(socket_api_sendmsg, enotconn_unconnected);
	RUN_TEST_CASE(socket_api_sendmsg, epipe_after_shutdown_wr);
	RUN_TEST_CASE(socket_api_sendmsg, ewouldblock_nonblocking);
	RUN_TEST_CASE(socket_api_sendmsg, connection_mode_ignores_dest_addr);
	RUN_TEST_CASE(socket_api_sendmsg, msg_nosignal_flag);
	RUN_TEST_CASE(socket_api_sendmsg, scm_rights_send_fd);
	RUN_TEST_CASE(socket_api_sendmsg, eintr_signal_interrupts);
	RUN_TEST_CASE(socket_api_sendmsg, emsgsize_dgram_too_large);
	RUN_TEST_CASE(socket_api_sendmsg, iovlen_exceeds_iov_max);
	RUN_TEST_CASE(socket_api_sendmsg, iovlen_zero_or_negative);
	RUN_TEST_CASE(socket_api_sendmsg, invalid_cmsg_len);
}
