/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <sys/socket.h>
 * TESTED:
 *    - shutdown()
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

#include "unity_fixture.h"

#define SHUTDOWN_MSG     "test"
#define SHUTDOWN_MSG_LEN 4
#define SHUTDOWN_BUF_SZ  32

TEST_GROUP(socket_api_shutdown);

TEST_SETUP(socket_api_shutdown)
{
}

TEST_TEAR_DOWN(socket_api_shutdown)
{
}


TEST(socket_api_shutdown, shut_rd_disables_recv)
{
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1634 issue");
#endif
	int sv[2];
	int ret;
	ssize_t n;
	char buf[SHUTDOWN_BUF_SZ];

	ret = socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = shutdown(sv[0], SHUT_RD);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Reading from a socket after SHUT_RD should return 0 (EOF) */
	n = recv(sv[0], buf, sizeof(buf), 0);
	TEST_ASSERT_EQUAL_INT(0, (int)n);

	close(sv[0]);
	close(sv[1]);
}


TEST(socket_api_shutdown, shut_wr_disables_send)
{
	int sv[2];
	int ret;
	ssize_t n;

	ret = socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = shutdown(sv[0], SHUT_WR);
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1634 issue");
#endif
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Writing to a socket after SHUT_WR should fail with EPIPE */
	errno = 0;
	n = send(sv[0], SHUTDOWN_MSG, SHUTDOWN_MSG_LEN, MSG_NOSIGNAL);
	TEST_ASSERT_EQUAL_INT(-1, (int)n);
	TEST_ASSERT_EQUAL_INT(EPIPE, errno);

	close(sv[0]);
	close(sv[1]);
}


TEST(socket_api_shutdown, shut_rdwr_disables_both)
{
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1634 issue");
#endif
	int sv[2];
	int ret;
	ssize_t n;
	char buf[SHUTDOWN_BUF_SZ];

	ret = socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = shutdown(sv[0], SHUT_RDWR);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Read should return 0 (EOF) */
	n = recv(sv[0], buf, sizeof(buf), 0);
	TEST_ASSERT_EQUAL_INT(0, (int)n);

	/* Write should fail with EPIPE */
	errno = 0;
	n = send(sv[0], SHUTDOWN_MSG, SHUTDOWN_MSG_LEN, MSG_NOSIGNAL);
	TEST_ASSERT_EQUAL_INT(-1, (int)n);
	TEST_ASSERT_EQUAL_INT(EPIPE, errno);

	close(sv[0]);
	close(sv[1]);
}


TEST(socket_api_shutdown, shut_wr_peer_reads_eof)
{
	int sv[2];
	int ret;
	ssize_t n;
	char buf[SHUTDOWN_BUF_SZ];

	ret = socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Send data, then shutdown write end */
	n = send(sv[0], SHUTDOWN_MSG, SHUTDOWN_MSG_LEN, 0);
	TEST_ASSERT_EQUAL_INT(SHUTDOWN_MSG_LEN, (int)n);

	ret = shutdown(sv[0], SHUT_WR);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Peer should be able to read pending data */
	memset(buf, 0, sizeof(buf));
	n = recv(sv[1], buf, sizeof(buf), 0);
	TEST_ASSERT_EQUAL_INT(SHUTDOWN_MSG_LEN, (int)n);
	TEST_ASSERT_EQUAL_MEMORY(SHUTDOWN_MSG, buf, SHUTDOWN_MSG_LEN);

	/* Next read should return EOF */
	n = recv(sv[1], buf, sizeof(buf), 0);
	TEST_ASSERT_EQUAL_INT(0, (int)n);

	close(sv[0]);
	close(sv[1]);
}


TEST(socket_api_shutdown, return_zero_on_success)
{
	int sv[2];
	int ret;

	ret = socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = shutdown(sv[0], SHUT_RD);
	TEST_ASSERT_EQUAL_INT(0, ret);

	close(sv[0]);
	close(sv[1]);
}


TEST(socket_api_shutdown, ebadf_invalid_fd)
{
	int ret;

	errno = 0;
	ret = shutdown(-1, SHUT_RDWR);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


TEST(socket_api_shutdown, ebadf_closed_fd)
{
	int sv[2];
	int ret;

	ret = socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
	TEST_ASSERT_EQUAL_INT(0, ret);

	close(sv[0]);

	errno = 0;
	ret = shutdown(sv[0], SHUT_RDWR);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EBADF, errno);

	close(sv[1]);
}


TEST(socket_api_shutdown, einval_invalid_how)
{
	int sv[2];
	int ret;

	ret = socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
	TEST_ASSERT_EQUAL_INT(0, ret);

	errno = 0;
	ret = shutdown(sv[0], 42);
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1634 issue");
#endif
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);

	close(sv[0]);
	close(sv[1]);
}


TEST(socket_api_shutdown, enotsock_not_socket)
{
	int fd;
	int ret;
	int pipefd[2];

	ret = pipe(pipefd);
	TEST_ASSERT_EQUAL_INT(0, ret);

	fd = pipefd[0];

	errno = 0;
	ret = shutdown(fd, SHUT_RDWR);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(ENOTSOCK, errno);

	close(pipefd[0]);
	close(pipefd[1]);
}


TEST(socket_api_shutdown, enotconn_unconnected)
{
	int fd;
	int ret;

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_TRUE(fd >= 0);

	errno = 0;
	ret = shutdown(fd, SHUT_RDWR);
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1634 issue");
#else
	TEST_IGNORE_MESSAGE("Linux allows shutdown on unconnected AF_UNIX socket");
#endif
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(ENOTCONN, errno);

	close(fd);
}


TEST(socket_api_shutdown, shut_rd_peer_can_still_send)
{
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1634 issue");
#endif
	int sv[2];
	int ret;
	ssize_t n;
	char buf[SHUTDOWN_BUF_SZ];

	ret = socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Shut down read on sv[0], peer sv[1] should still be able to write
	 * (the data just won't be delivered to sv[0]) */
	ret = shutdown(sv[0], SHUT_RD);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* sv[1] can still send (peer write direction is not affected) */
	n = send(sv[1], SHUTDOWN_MSG, SHUTDOWN_MSG_LEN, MSG_NOSIGNAL);
	/* On some implementations this may fail with EPIPE since receiver shut down reads,
	 * but POSIX says SHUT_RD disables receive operations - send may still succeed */
	(void)n;

	/* sv[1] can still receive from sv[0] if sv[0] hasn't shut down write */
	n = send(sv[0], SHUTDOWN_MSG, SHUTDOWN_MSG_LEN, MSG_NOSIGNAL);
	TEST_ASSERT_EQUAL_INT(SHUTDOWN_MSG_LEN, (int)n);

	memset(buf, 0, sizeof(buf));
	n = recv(sv[1], buf, sizeof(buf), 0);
	TEST_ASSERT_EQUAL_INT(SHUTDOWN_MSG_LEN, (int)n);
	TEST_ASSERT_EQUAL_MEMORY(SHUTDOWN_MSG, buf, SHUTDOWN_MSG_LEN);

	close(sv[0]);
	close(sv[1]);
}


TEST(socket_api_shutdown, dgram_shutdown_rdwr)
{
	int sv[2];
	int ret;
	ssize_t n;
	char buf[SHUTDOWN_BUF_SZ];

	ret = socketpair(AF_UNIX, SOCK_DGRAM, 0, sv);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Verify communication works before shutdown */
	n = send(sv[0], SHUTDOWN_MSG, SHUTDOWN_MSG_LEN, 0);
	TEST_ASSERT_EQUAL_INT(SHUTDOWN_MSG_LEN, (int)n);

	memset(buf, 0, sizeof(buf));
	n = recv(sv[1], buf, sizeof(buf), 0);
	TEST_ASSERT_EQUAL_INT(SHUTDOWN_MSG_LEN, (int)n);

	/* Shutdown SOCK_DGRAM for both read and write */
	ret = shutdown(sv[0], SHUT_RDWR);
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1634 issue");
#endif
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Sending on shutdown socket should fail */
	errno = 0;
	n = send(sv[0], SHUTDOWN_MSG, SHUTDOWN_MSG_LEN, MSG_NOSIGNAL);
	TEST_ASSERT_EQUAL_INT(-1, (int)n);
	TEST_ASSERT_EQUAL_INT(EPIPE, errno);

	close(sv[0]);
	close(sv[1]);
}


TEST_GROUP_RUNNER(socket_api_shutdown)
{
	RUN_TEST_CASE(socket_api_shutdown, shut_rd_disables_recv);
	RUN_TEST_CASE(socket_api_shutdown, shut_wr_disables_send);
	RUN_TEST_CASE(socket_api_shutdown, shut_rdwr_disables_both);
	RUN_TEST_CASE(socket_api_shutdown, shut_wr_peer_reads_eof);
	RUN_TEST_CASE(socket_api_shutdown, return_zero_on_success);
	RUN_TEST_CASE(socket_api_shutdown, ebadf_invalid_fd);
	RUN_TEST_CASE(socket_api_shutdown, ebadf_closed_fd);
	RUN_TEST_CASE(socket_api_shutdown, einval_invalid_how);
	RUN_TEST_CASE(socket_api_shutdown, enotsock_not_socket);
	RUN_TEST_CASE(socket_api_shutdown, enotconn_unconnected);
	RUN_TEST_CASE(socket_api_shutdown, shut_rd_peer_can_still_send);
	RUN_TEST_CASE(socket_api_shutdown, dgram_shutdown_rdwr);
}
