/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <sys/socket.h>
 * TESTED:
 *    - recvfrom()
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

#define RECVFROM_MSG       "hello"
#define RECVFROM_MSG_LEN   5
#define RECVFROM_BUF_SZ    32
#define RECVFROM_SOCK_PATH "/tmp/test_recvfrom_sock"

static struct {
	int sv[2];
} test_common;


TEST_GROUP(socket_api_recvfrom);

TEST_SETUP(socket_api_recvfrom)
{
	int ret;

	ret = socketpair(AF_UNIX, SOCK_STREAM, 0, test_common.sv);
	TEST_ASSERT_EQUAL_INT(0, ret);
}

TEST_TEAR_DOWN(socket_api_recvfrom)
{
	if (test_common.sv[0] >= 0) {
		close(test_common.sv[0]);
		test_common.sv[0] = -1;
	}
	if (test_common.sv[1] >= 0) {
		close(test_common.sv[1]);
		test_common.sv[1] = -1;
	}
	unlink(RECVFROM_SOCK_PATH);
}


TEST(socket_api_recvfrom, basic_success_null_address)
{
	ssize_t n;
	char buf[RECVFROM_BUF_SZ];

	n = send(test_common.sv[0], RECVFROM_MSG, RECVFROM_MSG_LEN, 0);
	TEST_ASSERT_EQUAL_INT(RECVFROM_MSG_LEN, (int)n);

	/* recvfrom with NULL address and address_len */
	memset(buf, 0, sizeof(buf));
	n = recvfrom(test_common.sv[1], buf, sizeof(buf), 0, NULL, NULL);
	TEST_ASSERT_EQUAL_INT(RECVFROM_MSG_LEN, (int)n);
	TEST_ASSERT_EQUAL_MEMORY(RECVFROM_MSG, buf, RECVFROM_MSG_LEN);
}


TEST(socket_api_recvfrom, returns_message_length)
{
	ssize_t n;
	char buf[RECVFROM_BUF_SZ];
	const char *msg = "abcdefghij";
	const size_t msgLen = 10;

	n = send(test_common.sv[0], msg, msgLen, 0);
	TEST_ASSERT_EQUAL_INT((int)msgLen, (int)n);

	memset(buf, 0, sizeof(buf));
	n = recvfrom(test_common.sv[1], buf, sizeof(buf), 0, NULL, NULL);
	TEST_ASSERT_EQUAL_INT((int)msgLen, (int)n);
	TEST_ASSERT_EQUAL_MEMORY(msg, buf, msgLen);
}


TEST(socket_api_recvfrom, dgram_source_address)
{
	int srvFd;
	int cliFd;
	int ret;
	ssize_t n;
	char buf[RECVFROM_BUF_SZ];
	struct sockaddr_un srvAddr;
	struct sockaddr_un srcAddr;
	socklen_t addrLen;

	/* Close the default socketpair - use dgram sockets instead */
	close(test_common.sv[0]);
	close(test_common.sv[1]);
	test_common.sv[0] = -1;
	test_common.sv[1] = -1;

	unlink(RECVFROM_SOCK_PATH);

	srvFd = socket(AF_UNIX, SOCK_DGRAM, 0);
	TEST_ASSERT_TRUE(srvFd >= 0);

	memset(&srvAddr, 0, sizeof(srvAddr));
	srvAddr.sun_family = AF_UNIX;
	strncpy(srvAddr.sun_path, RECVFROM_SOCK_PATH, sizeof(srvAddr.sun_path) - 1);

	ret = bind(srvFd, (struct sockaddr *)&srvAddr, sizeof(srvAddr));
	TEST_ASSERT_EQUAL_INT(0, ret);

	cliFd = socket(AF_UNIX, SOCK_DGRAM, 0);
	TEST_ASSERT_TRUE(cliFd >= 0);

	n = sendto(cliFd, RECVFROM_MSG, RECVFROM_MSG_LEN, 0,
		(struct sockaddr *)&srvAddr, sizeof(srvAddr));
	TEST_ASSERT_EQUAL_INT(RECVFROM_MSG_LEN, (int)n);

	/* Receive with source address retrieval */
	memset(buf, 0, sizeof(buf));
	memset(&srcAddr, 0, sizeof(srcAddr));
	addrLen = sizeof(srcAddr);
	n = recvfrom(srvFd, buf, sizeof(buf), 0, (struct sockaddr *)&srcAddr, &addrLen);
	TEST_ASSERT_EQUAL_INT(RECVFROM_MSG_LEN, (int)n);
	TEST_ASSERT_EQUAL_MEMORY(RECVFROM_MSG, buf, RECVFROM_MSG_LEN);

	close(srvFd);
	close(cliFd);
}


TEST(socket_api_recvfrom, msg_peek)
{
	ssize_t n;
	char buf[RECVFROM_BUF_SZ];

	n = send(test_common.sv[0], RECVFROM_MSG, RECVFROM_MSG_LEN, 0);
	TEST_ASSERT_EQUAL_INT(RECVFROM_MSG_LEN, (int)n);

	/* MSG_PEEK: data is treated as unread */
	memset(buf, 0, sizeof(buf));
	n = recvfrom(test_common.sv[1], buf, sizeof(buf), MSG_PEEK, NULL, NULL);
	TEST_ASSERT_EQUAL_INT(RECVFROM_MSG_LEN, (int)n);
	TEST_ASSERT_EQUAL_MEMORY(RECVFROM_MSG, buf, RECVFROM_MSG_LEN);

	/* Next recvfrom shall still return the same data */
	memset(buf, 0, sizeof(buf));
	n = recvfrom(test_common.sv[1], buf, sizeof(buf), 0, NULL, NULL);
	TEST_ASSERT_EQUAL_INT(RECVFROM_MSG_LEN, (int)n);
	TEST_ASSERT_EQUAL_MEMORY(RECVFROM_MSG, buf, RECVFROM_MSG_LEN);
}


TEST(socket_api_recvfrom, msg_waitall_stream)
{
	ssize_t n;
	char buf[RECVFROM_BUF_SZ];

	n = send(test_common.sv[0], RECVFROM_MSG, RECVFROM_MSG_LEN, 0);
	TEST_ASSERT_EQUAL_INT(RECVFROM_MSG_LEN, (int)n);

	/* Close writer so MSG_WAITALL returns what's available */
	close(test_common.sv[0]);
	test_common.sv[0] = -1;

	memset(buf, 0, sizeof(buf));
	n = recvfrom(test_common.sv[1], buf, sizeof(buf), MSG_WAITALL, NULL, NULL);
	TEST_ASSERT_EQUAL_INT(RECVFROM_MSG_LEN, (int)n);
	TEST_ASSERT_EQUAL_MEMORY(RECVFROM_MSG, buf, RECVFROM_MSG_LEN);
}


TEST(socket_api_recvfrom, orderly_shutdown_returns_zero)
{
	ssize_t n;
	char buf[RECVFROM_BUF_SZ];

	close(test_common.sv[0]);
	test_common.sv[0] = -1;

	n = recvfrom(test_common.sv[1], buf, sizeof(buf), 0, NULL, NULL);
	TEST_ASSERT_EQUAL_INT(0, (int)n);
}


TEST(socket_api_recvfrom, eagain_nonblock_no_data)
{
	ssize_t n;
	int ret;
	int flags;
	char buf[RECVFROM_BUF_SZ];

	flags = fcntl(test_common.sv[1], F_GETFL, 0);
	TEST_ASSERT_TRUE(flags >= 0);
	ret = fcntl(test_common.sv[1], F_SETFL, flags | O_NONBLOCK);
	TEST_ASSERT_EQUAL_INT(0, ret);

	errno = 0;
	n = recvfrom(test_common.sv[1], buf, sizeof(buf), 0, NULL, NULL);
	TEST_ASSERT_EQUAL_INT(-1, (int)n);
	TEST_ASSERT_TRUE(errno == EAGAIN || errno == EWOULDBLOCK);
}


TEST(socket_api_recvfrom, ebadf_invalid_fd)
{
	ssize_t n;
	char buf[RECVFROM_BUF_SZ];

	errno = 0;
	n = recvfrom(-1, buf, sizeof(buf), 0, NULL, NULL);
	TEST_ASSERT_EQUAL_INT(-1, (int)n);
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


TEST(socket_api_recvfrom, enotsock_pipe_fd)
{
	ssize_t n;
	int ret;
	int pipefd[2];
	char buf[RECVFROM_BUF_SZ];

	ret = pipe(pipefd);
	TEST_ASSERT_EQUAL_INT(0, ret);

	errno = 0;
	n = recvfrom(pipefd[0], buf, sizeof(buf), 0, NULL, NULL);
	TEST_ASSERT_EQUAL_INT(-1, (int)n);
	TEST_ASSERT_EQUAL_INT(ENOTSOCK, errno);

	close(pipefd[0]);
	close(pipefd[1]);
}


TEST(socket_api_recvfrom, enotconn_unconnected_stream)
{
	ssize_t n;
	int fd;
	char buf[RECVFROM_BUF_SZ];

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_TRUE(fd >= 0);

	errno = 0;
	n = recvfrom(fd, buf, sizeof(buf), 0, NULL, NULL);
	TEST_ASSERT_EQUAL_INT(-1, (int)n);
#ifndef __phoenix__
	/* glibc may return EINVAL instead of ENOTCONN on unbound AF_UNIX stream socket */
	TEST_ASSERT_TRUE(errno == ENOTCONN || errno == EINVAL);
#else
	TEST_ASSERT_EQUAL_INT(ENOTCONN, errno);
#endif

	close(fd);
}


TEST(socket_api_recvfrom, partial_read_stream)
{
	ssize_t n;
	char buf[4];
	const char *msg = "abcdefgh";
	const size_t msgLen = 8;

	n = send(test_common.sv[0], msg, msgLen, 0);
	TEST_ASSERT_EQUAL_INT((int)msgLen, (int)n);

	/* Read only 4 bytes from stream */
	memset(buf, 0, sizeof(buf));
	n = recvfrom(test_common.sv[1], buf, sizeof(buf), 0, NULL, NULL);
	TEST_ASSERT_EQUAL_INT(4, (int)n);
	TEST_ASSERT_EQUAL_MEMORY("abcd", buf, 4);

	/* Remaining 4 bytes still available */
	memset(buf, 0, sizeof(buf));
	n = recvfrom(test_common.sv[1], buf, sizeof(buf), 0, NULL, NULL);
	TEST_ASSERT_EQUAL_INT(4, (int)n);
	TEST_ASSERT_EQUAL_MEMORY("efgh", buf, 4);
}


TEST(socket_api_recvfrom, dgram_excess_bytes_discarded)
{
	int ret;
	int sv[2];
	ssize_t n;
	char buf[4];
	const char *msg = "abcdefgh";
	const size_t msgLen = 8;

	/* Close the default stream pair */
	close(test_common.sv[0]);
	close(test_common.sv[1]);
	test_common.sv[0] = -1;
	test_common.sv[1] = -1;

	ret = socketpair(AF_UNIX, SOCK_DGRAM, 0, sv);
	TEST_ASSERT_EQUAL_INT(0, ret);

	n = send(sv[0], msg, msgLen, 0);
	TEST_ASSERT_EQUAL_INT((int)msgLen, (int)n);

	/* Buffer smaller than message: excess bytes discarded for DGRAM */
	memset(buf, 0, sizeof(buf));
	n = recvfrom(sv[1], buf, sizeof(buf), 0, NULL, NULL);
	TEST_ASSERT_EQUAL_INT(4, (int)n);
	TEST_ASSERT_EQUAL_MEMORY("abcd", buf, 4);

	/* Next recv should not get remaining bytes since they were discarded */
	ret = fcntl(sv[1], F_SETFL, fcntl(sv[1], F_GETFL, 0) | O_NONBLOCK);
	TEST_ASSERT_EQUAL_INT(0, ret);

	errno = 0;
	n = recvfrom(sv[1], buf, sizeof(buf), 0, NULL, NULL);
	TEST_ASSERT_EQUAL_INT(-1, (int)n);
	TEST_ASSERT_TRUE(errno == EAGAIN || errno == EWOULDBLOCK);

	close(sv[0]);
	close(sv[1]);
}


TEST_GROUP_RUNNER(socket_api_recvfrom)
{
	RUN_TEST_CASE(socket_api_recvfrom, basic_success_null_address);
	RUN_TEST_CASE(socket_api_recvfrom, returns_message_length);
	RUN_TEST_CASE(socket_api_recvfrom, dgram_source_address);
	RUN_TEST_CASE(socket_api_recvfrom, msg_peek);
	RUN_TEST_CASE(socket_api_recvfrom, msg_waitall_stream);
	RUN_TEST_CASE(socket_api_recvfrom, orderly_shutdown_returns_zero);
	RUN_TEST_CASE(socket_api_recvfrom, eagain_nonblock_no_data);
	RUN_TEST_CASE(socket_api_recvfrom, ebadf_invalid_fd);
	RUN_TEST_CASE(socket_api_recvfrom, enotsock_pipe_fd);
	RUN_TEST_CASE(socket_api_recvfrom, enotconn_unconnected_stream);
	RUN_TEST_CASE(socket_api_recvfrom, partial_read_stream);
	RUN_TEST_CASE(socket_api_recvfrom, dgram_excess_bytes_discarded);
}
