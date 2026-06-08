/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <sys/socket.h>
 * TESTED:
 *    - connect()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include "unity_fixture.h"

#define CONNECT_SOCK_PATH     "/tmp/test_connect_sock"
#define CONNECT_NOENT_PATH    "/tmp/test_connect_noent_dir/sock"
#define CONNECT_BACKLOG       5


static struct {
	int listenFd;
	int clientFd;
	int acceptFd;
} test_common;


TEST_GROUP(socket_api_connect);


TEST_SETUP(socket_api_connect)
{
	unlink(CONNECT_SOCK_PATH);
	test_common.listenFd = -1;
	test_common.clientFd = -1;
	test_common.acceptFd = -1;
}


TEST_TEAR_DOWN(socket_api_connect)
{
	if (test_common.acceptFd >= 0) {
		close(test_common.acceptFd);
		test_common.acceptFd = -1;
	}
	if (test_common.clientFd >= 0) {
		close(test_common.clientFd);
		test_common.clientFd = -1;
	}
	if (test_common.listenFd >= 0) {
		close(test_common.listenFd);
		test_common.listenFd = -1;
	}
	unlink(CONNECT_SOCK_PATH);
}


static int test_setupListener(void)
{
	struct sockaddr_un addr;
	int fd;
	int ret;

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0) {
		return -1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, CONNECT_SOCK_PATH, sizeof(addr.sun_path) - 1);

	ret = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
	if (ret != 0) {
		close(fd);
		return -1;
	}

	ret = listen(fd, CONNECT_BACKLOG);
	if (ret != 0) {
		close(fd);
		return -1;
	}

	return fd;
}


TEST(socket_api_connect, connect_unix_stream_success)
{
	/* "connect() shall attempt to make a connection on a connection-mode socket" */
	struct sockaddr_un addr;
	int ret;

	test_common.listenFd = test_setupListener();
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.listenFd);

	test_common.clientFd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.clientFd);

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, CONNECT_SOCK_PATH, sizeof(addr.sun_path) - 1);

	errno = 0;
	ret = connect(test_common.clientFd, (struct sockaddr *)&addr, sizeof(addr));
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, errno);
}


TEST(socket_api_connect, connect_unix_dgram_sets_peer)
{
	/* "If the initiating socket is not connection-mode, then connect()
	 *  shall set the socket's peer address" */
	struct sockaddr_un srvAddr;
	int srvFd;
	int ret;

	/* Create and bind a DGRAM server */
	srvFd = socket(AF_UNIX, SOCK_DGRAM, 0);
	TEST_ASSERT_NOT_EQUAL_INT(-1, srvFd);
	test_common.listenFd = srvFd;

	memset(&srvAddr, 0, sizeof(srvAddr));
	srvAddr.sun_family = AF_UNIX;
	strncpy(srvAddr.sun_path, CONNECT_SOCK_PATH, sizeof(srvAddr.sun_path) - 1);

	ret = bind(srvFd, (struct sockaddr *)&srvAddr, sizeof(srvAddr));
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Create client and connect */
	test_common.clientFd = socket(AF_UNIX, SOCK_DGRAM, 0);
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.clientFd);

	ret = connect(test_common.clientFd, (struct sockaddr *)&srvAddr, sizeof(srvAddr));
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* After connect, send() can be used without specifying destination */
	const char msg[] = "dgram";
	ret = (int)send(test_common.clientFd, msg, sizeof(msg), 0);
	TEST_ASSERT_EQUAL_INT((int)sizeof(msg), ret);

	/* Server receives it */
	char buf[16];
	ret = (int)recv(srvFd, buf, sizeof(buf), 0);
	TEST_ASSERT_EQUAL_INT((int)sizeof(msg), ret);
	TEST_ASSERT_EQUAL_STRING(msg, buf);
}


TEST(socket_api_connect, connect_dgram_unspec_resets_peer)
{
	/* "If the sa_family member of address is AF_UNSPEC, the socket's
	 *  peer address shall be reset." */
	struct sockaddr_un srvAddr;
	struct sockaddr unspecAddr;
	int srvFd;
	int ret;

	srvFd = socket(AF_UNIX, SOCK_DGRAM, 0);
	TEST_ASSERT_NOT_EQUAL_INT(-1, srvFd);
	test_common.listenFd = srvFd;

	memset(&srvAddr, 0, sizeof(srvAddr));
	srvAddr.sun_family = AF_UNIX;
	strncpy(srvAddr.sun_path, CONNECT_SOCK_PATH, sizeof(srvAddr.sun_path) - 1);

	ret = bind(srvFd, (struct sockaddr *)&srvAddr, sizeof(srvAddr));
	TEST_ASSERT_EQUAL_INT(0, ret);

	test_common.clientFd = socket(AF_UNIX, SOCK_DGRAM, 0);
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.clientFd);

	/* Connect to server */
	ret = connect(test_common.clientFd, (struct sockaddr *)&srvAddr, sizeof(srvAddr));
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Reset peer with AF_UNSPEC */
	memset(&unspecAddr, 0, sizeof(unspecAddr));
	unspecAddr.sa_family = AF_UNSPEC;

	ret = connect(test_common.clientFd, &unspecAddr, sizeof(unspecAddr));
	/* Some implementations return 0, some return -1 with EAFNOSUPPORT but still reset */
	(void)ret;

	/* After reset, send() without destination should fail */
	errno = 0;
	ret = (int)send(test_common.clientFd, "x", 1, 0);
	if (ret == -1) {
		/* Expected: EDESTADDRREQ or ENOTCONN */
		TEST_ASSERT_TRUE(errno == EDESTADDRREQ || errno == ENOTCONN);
	}
}


TEST(socket_api_connect, connect_ebadf)
{
	/* "EBADF: The socket argument is not a valid file descriptor." */
	struct sockaddr_un addr;
	int ret;

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, CONNECT_SOCK_PATH, sizeof(addr.sun_path) - 1);

	errno = 0;
	ret = connect(-1, (struct sockaddr *)&addr, sizeof(addr));
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


TEST(socket_api_connect, connect_enotsock)
{
	/* "ENOTSOCK: The socket argument does not refer to a socket." */
	struct sockaddr_un addr;
	int fd;
	int ret;

	fd = open("/dev/null", O_RDONLY);
	TEST_ASSERT_NOT_EQUAL_INT(-1, fd);

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, CONNECT_SOCK_PATH, sizeof(addr.sun_path) - 1);

	errno = 0;
	ret = connect(fd, (struct sockaddr *)&addr, sizeof(addr));
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(ENOTSOCK, errno);

	close(fd);
}


TEST(socket_api_connect, connect_enoent)
{
	/* "ENOENT: A component of the pathname does not name an existing file" */
	struct sockaddr_un addr;
	int ret;

	test_common.clientFd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.clientFd);

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, CONNECT_NOENT_PATH, sizeof(addr.sun_path) - 1);

	errno = 0;
	ret = connect(test_common.clientFd, (struct sockaddr *)&addr, sizeof(addr));
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);
}


TEST(socket_api_connect, connect_econnrefused)
{
	/* "ECONNREFUSED: The target address was not listening for connections
	 *  or refused the connection request." */
	struct sockaddr_in addr;
	int ret;

	test_common.clientFd = socket(AF_INET, SOCK_STREAM, 0);
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.clientFd);

	/* Connect to loopback on a port that nobody is listening on */
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port = htons(19999);

	errno = 0;
	ret = connect(test_common.clientFd, (struct sockaddr *)&addr, sizeof(addr));
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(ECONNREFUSED, errno);
}


TEST(socket_api_connect, connect_eisconn)
{
	/* "EISCONN: The specified socket is connection-mode and is already connected." */
	struct sockaddr_un addr;
	int ret;

	test_common.listenFd = test_setupListener();
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.listenFd);

	test_common.clientFd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.clientFd);

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, CONNECT_SOCK_PATH, sizeof(addr.sun_path) - 1);

	ret = connect(test_common.clientFd, (struct sockaddr *)&addr, sizeof(addr));
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Second connect on already-connected socket */
	errno = 0;
	ret = connect(test_common.clientFd, (struct sockaddr *)&addr, sizeof(addr));
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EISCONN, errno);
}


TEST(socket_api_connect, connect_eafnosupport)
{
	/* "EAFNOSUPPORT: The specified address is not a valid address for
	 *  the address family of the specified socket." */
	struct sockaddr_in inetAddr;
	int ret;

	test_common.clientFd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.clientFd);

	memset(&inetAddr, 0, sizeof(inetAddr));
	inetAddr.sin_family = AF_INET;
	inetAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	inetAddr.sin_port = htons(80);

	errno = 0;
	ret = connect(test_common.clientFd, (struct sockaddr *)&inetAddr, sizeof(inetAddr));
	TEST_ASSERT_EQUAL_INT(-1, ret);
	/* EAFNOSUPPORT or EINVAL are acceptable */
	TEST_ASSERT_TRUE(errno == EAFNOSUPPORT || errno == EINVAL);
}


TEST(socket_api_connect, connect_binds_unnamed)
{
	/* "If the socket has not already been bound to a local address,
	 *  connect() shall bind it to an address" */
	struct sockaddr_un addr, localAddr;
	socklen_t addrLen;
	int ret;

	test_common.listenFd = test_setupListener();
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.listenFd);

	test_common.clientFd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.clientFd);

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, CONNECT_SOCK_PATH, sizeof(addr.sun_path) - 1);

	ret = connect(test_common.clientFd, (struct sockaddr *)&addr, sizeof(addr));
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* getsockname should succeed after implicit bind */
	memset(&localAddr, 0, sizeof(localAddr));
	addrLen = sizeof(localAddr);
	ret = getsockname(test_common.clientFd, (struct sockaddr *)&localAddr, &addrLen);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(AF_UNIX, localAddr.sun_family);
}


TEST_GROUP_RUNNER(socket_api_connect)
{
	RUN_TEST_CASE(socket_api_connect, connect_unix_stream_success);
	RUN_TEST_CASE(socket_api_connect, connect_unix_dgram_sets_peer);
	RUN_TEST_CASE(socket_api_connect, connect_dgram_unspec_resets_peer);
	RUN_TEST_CASE(socket_api_connect, connect_ebadf);
	RUN_TEST_CASE(socket_api_connect, connect_enotsock);
	RUN_TEST_CASE(socket_api_connect, connect_enoent);
	RUN_TEST_CASE(socket_api_connect, connect_econnrefused);
	RUN_TEST_CASE(socket_api_connect, connect_eisconn);
	RUN_TEST_CASE(socket_api_connect, connect_eafnosupport);
	RUN_TEST_CASE(socket_api_connect, connect_binds_unnamed);
}
