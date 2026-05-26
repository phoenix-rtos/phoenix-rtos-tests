/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <sys/socket.h>
 * TESTED:
 *    - recvmsg()
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

#define RECVMSG_MSG       "hello"
#define RECVMSG_MSG_LEN   5
#define RECVMSG_BUF_SZ    32
#define RECVMSG_SOCK_PATH "/tmp/test_recvmsg_sock"

#ifndef MSG_TRUNC
#define MSG_TRUNC 0x20
#endif

static struct {
	int sv[2];
} test_common;


TEST_GROUP(socket_api_recvmsg);

TEST_SETUP(socket_api_recvmsg)
{
	int ret;

	ret = socketpair(AF_UNIX, SOCK_STREAM, 0, test_common.sv);
	TEST_ASSERT_EQUAL_INT(0, ret);
}

TEST_TEAR_DOWN(socket_api_recvmsg)
{
	if (test_common.sv[0] >= 0) {
		close(test_common.sv[0]);
		test_common.sv[0] = -1;
	}
	if (test_common.sv[1] >= 0) {
		close(test_common.sv[1]);
		test_common.sv[1] = -1;
	}
	unlink(RECVMSG_SOCK_PATH);
}


TEST(socket_api_recvmsg, basic_success)
{
	ssize_t n;
	char buf[RECVMSG_BUF_SZ];
	struct msghdr msg;
	struct iovec iov;

	n = send(test_common.sv[0], RECVMSG_MSG, RECVMSG_MSG_LEN, 0);
	TEST_ASSERT_EQUAL_INT(RECVMSG_MSG_LEN, (int)n);

	memset(buf, 0, sizeof(buf));
	iov.iov_base = buf;
	iov.iov_len = sizeof(buf);

	memset(&msg, 0, sizeof(msg));
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	n = recvmsg(test_common.sv[1], &msg, 0);
	TEST_ASSERT_EQUAL_INT(RECVMSG_MSG_LEN, (int)n);
	TEST_ASSERT_EQUAL_MEMORY(RECVMSG_MSG, buf, RECVMSG_MSG_LEN);
}


TEST(socket_api_recvmsg, returns_total_length)
{
	ssize_t n;
	char buf[RECVMSG_BUF_SZ];
	struct msghdr msg;
	struct iovec iov;
	const char *data = "abcdefghij";
	const size_t dataLen = 10;

	n = send(test_common.sv[0], data, dataLen, 0);
	TEST_ASSERT_EQUAL_INT((int)dataLen, (int)n);

	memset(buf, 0, sizeof(buf));
	iov.iov_base = buf;
	iov.iov_len = sizeof(buf);

	memset(&msg, 0, sizeof(msg));
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	n = recvmsg(test_common.sv[1], &msg, 0);
	TEST_ASSERT_EQUAL_INT((int)dataLen, (int)n);
	TEST_ASSERT_EQUAL_MEMORY(data, buf, dataLen);
}


TEST(socket_api_recvmsg, scatter_multiple_iovecs)
{
	ssize_t n;
	char buf1[4];
	char buf2[4];
	char buf3[4];
	struct msghdr msg;
	struct iovec iov[3];
	const char *data = "abcdefghij";
	const size_t dataLen = 10;

	n = send(test_common.sv[0], data, dataLen, 0);
	TEST_ASSERT_EQUAL_INT((int)dataLen, (int)n);

	memset(buf1, 0, sizeof(buf1));
	memset(buf2, 0, sizeof(buf2));
	memset(buf3, 0, sizeof(buf3));

	iov[0].iov_base = buf1;
	iov[0].iov_len = sizeof(buf1);
	iov[1].iov_base = buf2;
	iov[1].iov_len = sizeof(buf2);
	iov[2].iov_base = buf3;
	iov[2].iov_len = sizeof(buf3);

	memset(&msg, 0, sizeof(msg));
	msg.msg_iov = iov;
	msg.msg_iovlen = 3;

	n = recvmsg(test_common.sv[1], &msg, 0);
	TEST_ASSERT_EQUAL_INT((int)dataLen, (int)n);
	TEST_ASSERT_EQUAL_MEMORY("abcd", buf1, 4);
	TEST_ASSERT_EQUAL_MEMORY("efgh", buf2, 4);
	TEST_ASSERT_EQUAL_MEMORY("ij\0\0", buf3, 4);
}


TEST(socket_api_recvmsg, msg_name_null)
{
	ssize_t n;
	char buf[RECVMSG_BUF_SZ];
	struct msghdr msg;
	struct iovec iov;

	n = send(test_common.sv[0], RECVMSG_MSG, RECVMSG_MSG_LEN, 0);
	TEST_ASSERT_EQUAL_INT(RECVMSG_MSG_LEN, (int)n);

	memset(buf, 0, sizeof(buf));
	iov.iov_base = buf;
	iov.iov_len = sizeof(buf);

	memset(&msg, 0, sizeof(msg));
	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	n = recvmsg(test_common.sv[1], &msg, 0);
	TEST_ASSERT_EQUAL_INT(RECVMSG_MSG_LEN, (int)n);
}


TEST(socket_api_recvmsg, msg_peek)
{
	ssize_t n;
	char buf[RECVMSG_BUF_SZ];
	struct msghdr msg;
	struct iovec iov;

	n = send(test_common.sv[0], RECVMSG_MSG, RECVMSG_MSG_LEN, 0);
	TEST_ASSERT_EQUAL_INT(RECVMSG_MSG_LEN, (int)n);

	memset(buf, 0, sizeof(buf));
	iov.iov_base = buf;
	iov.iov_len = sizeof(buf);

	memset(&msg, 0, sizeof(msg));
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	/* MSG_PEEK: data is treated as unread */
	n = recvmsg(test_common.sv[1], &msg, MSG_PEEK);
	TEST_ASSERT_EQUAL_INT(RECVMSG_MSG_LEN, (int)n);
	TEST_ASSERT_EQUAL_MEMORY(RECVMSG_MSG, buf, RECVMSG_MSG_LEN);

	/* Next recvmsg shall still return the same data */
	memset(buf, 0, sizeof(buf));
	n = recvmsg(test_common.sv[1], &msg, 0);
	TEST_ASSERT_EQUAL_INT(RECVMSG_MSG_LEN, (int)n);
	TEST_ASSERT_EQUAL_MEMORY(RECVMSG_MSG, buf, RECVMSG_MSG_LEN);
}


TEST(socket_api_recvmsg, msg_waitall_stream)
{
	ssize_t n;
	char buf[RECVMSG_BUF_SZ];
	struct msghdr msg;
	struct iovec iov;

	n = send(test_common.sv[0], RECVMSG_MSG, RECVMSG_MSG_LEN, 0);
	TEST_ASSERT_EQUAL_INT(RECVMSG_MSG_LEN, (int)n);

	close(test_common.sv[0]);
	test_common.sv[0] = -1;

	memset(buf, 0, sizeof(buf));
	iov.iov_base = buf;
	iov.iov_len = sizeof(buf);

	memset(&msg, 0, sizeof(msg));
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	n = recvmsg(test_common.sv[1], &msg, MSG_WAITALL);
	TEST_ASSERT_EQUAL_INT(RECVMSG_MSG_LEN, (int)n);
	TEST_ASSERT_EQUAL_MEMORY(RECVMSG_MSG, buf, RECVMSG_MSG_LEN);
}


TEST(socket_api_recvmsg, orderly_shutdown_returns_zero)
{
	ssize_t n;
	char buf[RECVMSG_BUF_SZ];
	struct msghdr msg;
	struct iovec iov;

	close(test_common.sv[0]);
	test_common.sv[0] = -1;

	memset(buf, 0, sizeof(buf));
	iov.iov_base = buf;
	iov.iov_len = sizeof(buf);

	memset(&msg, 0, sizeof(msg));
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	n = recvmsg(test_common.sv[1], &msg, 0);
	TEST_ASSERT_EQUAL_INT(0, (int)n);
}


TEST(socket_api_recvmsg, eagain_nonblock_no_data)
{
	ssize_t n;
	int ret;
	int flags;
	char buf[RECVMSG_BUF_SZ];
	struct msghdr msg;
	struct iovec iov;

	flags = fcntl(test_common.sv[1], F_GETFL, 0);
	TEST_ASSERT_TRUE(flags >= 0);
	ret = fcntl(test_common.sv[1], F_SETFL, flags | O_NONBLOCK);
	TEST_ASSERT_EQUAL_INT(0, ret);

	memset(buf, 0, sizeof(buf));
	iov.iov_base = buf;
	iov.iov_len = sizeof(buf);

	memset(&msg, 0, sizeof(msg));
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	errno = 0;
	n = recvmsg(test_common.sv[1], &msg, 0);
	TEST_ASSERT_EQUAL_INT(-1, (int)n);
	TEST_ASSERT_TRUE(errno == EAGAIN || errno == EWOULDBLOCK);
}


TEST(socket_api_recvmsg, ebadf_invalid_fd)
{
	ssize_t n;
	char buf[RECVMSG_BUF_SZ];
	struct msghdr msg;
	struct iovec iov;

	memset(buf, 0, sizeof(buf));
	iov.iov_base = buf;
	iov.iov_len = sizeof(buf);

	memset(&msg, 0, sizeof(msg));
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	errno = 0;
	n = recvmsg(-1, &msg, 0);
	TEST_ASSERT_EQUAL_INT(-1, (int)n);
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


TEST(socket_api_recvmsg, enotsock_pipe_fd)
{
	ssize_t n;
	int ret;
	int pipefd[2];
	char buf[RECVMSG_BUF_SZ];
	struct msghdr msg;
	struct iovec iov;

	ret = pipe(pipefd);
	TEST_ASSERT_EQUAL_INT(0, ret);

	memset(buf, 0, sizeof(buf));
	iov.iov_base = buf;
	iov.iov_len = sizeof(buf);

	memset(&msg, 0, sizeof(msg));
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	errno = 0;
	if (TEST_PROTECT()) {
		n = recvmsg(pipefd[0], &msg, 0);
		TEST_ASSERT_EQUAL_INT(-1, (int)n);
		TEST_ASSERT_EQUAL_INT(ENOTSOCK, errno);
	}
	close(pipefd[0]);
	close(pipefd[1]);
}


TEST(socket_api_recvmsg, enotconn_unconnected_stream)
{
	ssize_t n;
	int fd;
	char buf[RECVMSG_BUF_SZ];
	struct msghdr msg;
	struct iovec iov;

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_TRUE(fd >= 0);

	memset(buf, 0, sizeof(buf));
	iov.iov_base = buf;
	iov.iov_len = sizeof(buf);

	memset(&msg, 0, sizeof(msg));
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	errno = 0;
	n = recvmsg(fd, &msg, 0);
	TEST_ASSERT_EQUAL_INT(-1, (int)n);
#ifndef __phoenix__
	/* glibc may return EINVAL instead of ENOTCONN on unbound AF_UNIX stream socket */
	TEST_ASSERT_TRUE(errno == ENOTCONN || errno == EINVAL);
#else
	TEST_ASSERT_EQUAL_INT(ENOTCONN, errno);
#endif

	close(fd);
}


TEST(socket_api_recvmsg, dgram_msg_trunc_flag)
{
	int ret;
	int *sv;
	ssize_t n;
	char buf[4];
	struct msghdr msg;
	struct iovec iov;
	const char *data = "abcdefgh";
	const size_t dataLen = 8;

	/* Close the default stream pair */
	close(test_common.sv[0]);
	close(test_common.sv[1]);
	test_common.sv[0] = -1;
	test_common.sv[1] = -1;

	sv = test_common.sv;

	ret = socketpair(AF_UNIX, SOCK_DGRAM, 0, sv);
	TEST_ASSERT_EQUAL_INT(0, ret);

	n = send(sv[0], data, dataLen, 0);
	TEST_ASSERT_EQUAL_INT((int)dataLen, (int)n);

	/* Buffer smaller than message: MSG_TRUNC should be set in msg_flags */
	memset(buf, 0, sizeof(buf));
	iov.iov_base = buf;
	iov.iov_len = sizeof(buf);

	memset(&msg, 0, sizeof(msg));
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	n = recvmsg(sv[1], &msg, 0);
	/* Returns bytes written to buffer (truncated) */
	TEST_ASSERT_GREATER_THAN_INT(0, (int)n);
	TEST_ASSERT_EQUAL_MEMORY("abcd", buf, 4);
	/* MSG_TRUNC shall be set when data was truncated */
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1648 issue");
#endif
	TEST_ASSERT_TRUE((msg.msg_flags & MSG_TRUNC) != 0);
}


TEST(socket_api_recvmsg, dgram_source_address)
{
	volatile int srvFd = -1;
	volatile int cliFd = -1;
	int ret;
	ssize_t n;
	char buf[RECVMSG_BUF_SZ];
	struct sockaddr_un srvAddr;
	struct sockaddr_un srcAddr;
	struct msghdr msg;
	struct iovec iov;
	const char *cliPath = "/tmp/test_recvmsg_cli_sock";

#ifdef __phoenix__
	/* https://github.com/phoenix-rtos/phoenix-rtos-project/issues/1665 */
	TEST_IGNORE_MESSAGE("#1665 issue");
#endif

	/* Close the default stream pair */
	close(test_common.sv[0]);
	close(test_common.sv[1]);
	test_common.sv[0] = -1;
	test_common.sv[1] = -1;

	unlink(RECVMSG_SOCK_PATH);

	srvFd = socket(AF_UNIX, SOCK_DGRAM, 0);
	TEST_ASSERT_TRUE(srvFd >= 0);

	if (TEST_PROTECT()) {
		memset(&srvAddr, 0, sizeof(srvAddr));
		srvAddr.sun_family = AF_UNIX;
		snprintf(srvAddr.sun_path, sizeof(srvAddr.sun_path), "%s", RECVMSG_SOCK_PATH);

		ret = bind(srvFd, (struct sockaddr *)&srvAddr, sizeof(srvAddr));
		TEST_ASSERT_EQUAL_INT(0, ret);

		cliFd = socket(AF_UNIX, SOCK_DGRAM, 0);
		TEST_ASSERT_TRUE(cliFd >= 0);

		struct sockaddr_un cliAddr;
		memset(&cliAddr, 0, sizeof(cliAddr));
		cliAddr.sun_family = AF_UNIX;
		strncpy(cliAddr.sun_path, cliPath, sizeof(cliAddr.sun_path) - 1);

		unlink(cliPath);
		ret = bind(cliFd, (struct sockaddr *)&cliAddr, sizeof(cliAddr));
		TEST_ASSERT_EQUAL_INT(0, ret);

		n = sendto(cliFd, RECVMSG_MSG, RECVMSG_MSG_LEN, 0,
				(struct sockaddr *)&srvAddr, sizeof(srvAddr));
		TEST_ASSERT_EQUAL_INT(RECVMSG_MSG_LEN, (int)n);

		/* Receive with source address */
		memset(buf, 0, sizeof(buf));
		memset(&srcAddr, 0, sizeof(srcAddr));

		iov.iov_base = buf;
		iov.iov_len = sizeof(buf);

		memset(&msg, 0, sizeof(msg));
		msg.msg_name = &srcAddr;
		msg.msg_namelen = sizeof(srcAddr);
		msg.msg_iov = &iov;
		msg.msg_iovlen = 1;

		n = recvmsg(srvFd, &msg, 0);
		TEST_ASSERT_EQUAL_INT(RECVMSG_MSG_LEN, (int)n);
		TEST_ASSERT_EQUAL_MEMORY(RECVMSG_MSG, buf, RECVMSG_MSG_LEN);
		TEST_ASSERT_GREATER_THAN_INT(0, msg.msg_namelen);
		TEST_ASSERT_EQUAL_INT(AF_UNIX, srcAddr.sun_family);
		TEST_ASSERT_EQUAL_STRING(cliPath, srcAddr.sun_path);
	}
	if (srvFd >= 0) {
		close(srvFd);
	}
	if (cliFd >= 0) {
		close(cliFd);
	}
}


TEST_GROUP_RUNNER(socket_api_recvmsg)
{
	RUN_TEST_CASE(socket_api_recvmsg, basic_success);
	RUN_TEST_CASE(socket_api_recvmsg, returns_total_length);
	RUN_TEST_CASE(socket_api_recvmsg, scatter_multiple_iovecs);
	RUN_TEST_CASE(socket_api_recvmsg, msg_name_null);
	RUN_TEST_CASE(socket_api_recvmsg, msg_peek);
	RUN_TEST_CASE(socket_api_recvmsg, msg_waitall_stream);
	RUN_TEST_CASE(socket_api_recvmsg, orderly_shutdown_returns_zero);
	RUN_TEST_CASE(socket_api_recvmsg, eagain_nonblock_no_data);
	RUN_TEST_CASE(socket_api_recvmsg, ebadf_invalid_fd);
	RUN_TEST_CASE(socket_api_recvmsg, enotsock_pipe_fd);
	RUN_TEST_CASE(socket_api_recvmsg, enotconn_unconnected_stream);
	RUN_TEST_CASE(socket_api_recvmsg, dgram_msg_trunc_flag);
	RUN_TEST_CASE(socket_api_recvmsg, dgram_source_address);
}
