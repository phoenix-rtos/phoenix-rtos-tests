/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <sys/socket.h>
 * TESTED:
 *    - getpeername()
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

#define GETPEERNAME_SOCK_PATH "/tmp/test_getpeername_sock"
#define GETPEERNAME_BACKLOG   5


static struct {
	int listenFd;
	int clientFd;
	int acceptFd;
	int genericFd;
} test_common;


TEST_GROUP(socket_api_getpeername);


TEST_SETUP(socket_api_getpeername)
{
	unlink(GETPEERNAME_SOCK_PATH);
	test_common.listenFd = -1;
	test_common.clientFd = -1;
	test_common.acceptFd = -1;
	test_common.genericFd = -1;
}


TEST_TEAR_DOWN(socket_api_getpeername)
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
	if (test_common.genericFd >= 0) {
		close(test_common.genericFd);
		test_common.genericFd = -1;
	}
	unlink(GETPEERNAME_SOCK_PATH);
}


static int test_setupConnectedPair(void)
{
	struct sockaddr_un addr;
	int ret;
	int flags;

	test_common.listenFd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (test_common.listenFd < 0) {
		return -1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, GETPEERNAME_SOCK_PATH, sizeof(addr.sun_path) - 1);

	ret = bind(test_common.listenFd, (struct sockaddr *)&addr, sizeof(addr));
	if (ret != 0) {
		return -1;
	}

	ret = listen(test_common.listenFd, GETPEERNAME_BACKLOG);
	if (ret != 0) {
		return -1;
	}

	test_common.clientFd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (test_common.clientFd < 0) {
		return -1;
	}

	flags = fcntl(test_common.clientFd, F_GETFL, 0);
	fcntl(test_common.clientFd, F_SETFL, flags | O_NONBLOCK);

	ret = connect(test_common.clientFd, (struct sockaddr *)&addr, sizeof(addr));
	if (ret != 0 && errno != EINPROGRESS) {
		return -1;
	}

	test_common.acceptFd = accept(test_common.listenFd, NULL, NULL);
	if (test_common.acceptFd < 0) {
		return -1;
	}

	fcntl(test_common.clientFd, F_SETFL, flags);

	return 0;
}


TEST(socket_api_getpeername, getpeername_success_accepted)
{
	/* "getpeername() shall retrieve the peer address of the specified socket" */
	struct sockaddr_un peerAddr;
	socklen_t addrLen = sizeof(peerAddr);
	int ret;

	TEST_ASSERT_EQUAL_INT(0, test_setupConnectedPair());

	/* On the accepted socket, peer is the client */
	memset(&peerAddr, 0, sizeof(peerAddr));
	ret = getpeername(test_common.acceptFd, (struct sockaddr *)&peerAddr, &addrLen);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST(socket_api_getpeername, getpeername_success_client)
{
	/* On the client socket, peer is the server address */
	struct sockaddr_un peerAddr;
	socklen_t addrLen = sizeof(peerAddr);
	int ret;

	TEST_ASSERT_EQUAL_INT(0, test_setupConnectedPair());

	memset(&peerAddr, 0, sizeof(peerAddr));
	ret = getpeername(test_common.clientFd, (struct sockaddr *)&peerAddr, &addrLen);
	TEST_ASSERT_EQUAL_INT(0, ret);
#ifdef __phoenix__
	/* https://github.com/phoenix-rtos/phoenix-rtos-project/issues/1672 */
	TEST_IGNORE_MESSAGE("#1672 issue");
#else
	TEST_ASSERT_EQUAL_INT(AF_UNIX, peerAddr.sun_family);
	/* The server's path should be in the peer address */
	TEST_ASSERT_EQUAL_STRING(GETPEERNAME_SOCK_PATH, peerAddr.sun_path);
#endif
}


TEST(socket_api_getpeername, getpeername_truncation)
{
	/* "If the actual length of the address is greater than the length
	 *  of the supplied sockaddr structure, the stored address shall be truncated." */
	struct sockaddr_un peerAddr;
	socklen_t addrLen;
	int ret;

	TEST_ASSERT_EQUAL_INT(0, test_setupConnectedPair());

	/* Provide a small buffer */
	addrLen = (socklen_t)sizeof(sa_family_t);
	memset(&peerAddr, 0xff, sizeof(peerAddr));
	ret = getpeername(test_common.clientFd, (struct sockaddr *)&peerAddr, &addrLen);
	TEST_ASSERT_EQUAL_INT(0, ret);
#ifdef __phoenix__
	/* https://github.com/phoenix-rtos/phoenix-rtos-project/issues/1672 */
	TEST_IGNORE_MESSAGE("#1672 issue");
#else
	/* addrLen should be updated to actual length (larger than what we passed) */
	TEST_ASSERT_GREATER_THAN_UINT((unsigned int)sizeof(sa_family_t), (unsigned int)addrLen);
#endif
}


TEST(socket_api_getpeername, getpeername_ebadf)
{
	/* "EBADF: The socket argument is not a valid file descriptor." */
	struct sockaddr_un addr;
	socklen_t addrLen = sizeof(addr);
	int ret;

	errno = 0;
	ret = getpeername(-1, (struct sockaddr *)&addr, &addrLen);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


TEST(socket_api_getpeername, getpeername_enotsock)
{
	/* "ENOTSOCK: The socket argument does not refer to a socket." */
	struct sockaddr_un addr;
	socklen_t addrLen = sizeof(addr);
	int ret;

	test_common.genericFd = open("/dev/null", O_RDONLY);
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.genericFd);

	errno = 0;
	ret = getpeername(test_common.genericFd, (struct sockaddr *)&addr, &addrLen);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(ENOTSOCK, errno);
}


TEST(socket_api_getpeername, getpeername_enotconn)
{
	/* "ENOTCONN: The socket is not connected or otherwise has not had
	 *  the peer pre-specified." */
	struct sockaddr_un addr;
	socklen_t addrLen = sizeof(addr);
	int ret;

	test_common.genericFd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.genericFd);

	errno = 0;
	ret = getpeername(test_common.genericFd, (struct sockaddr *)&addr, &addrLen);
#ifdef __phoenix__
	(void)ret;
	/* https://github.com/phoenix-rtos/phoenix-rtos-project/issues/1673 */
	TEST_IGNORE_MESSAGE("#1673 issue");
#else
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(ENOTCONN, errno);
#endif
}


TEST(socket_api_getpeername, getpeername_inet_loopback)
{
	/* Verify getpeername works for AF_INET connected sockets */
	struct sockaddr_in srvAddr, peerAddr;
	socklen_t addrLen;
	int srvFd, cliFd, accFd;
	int ret;
	int flags;

	srvFd = socket(AF_INET, SOCK_STREAM, 0);
	TEST_ASSERT_NOT_EQUAL_INT(-1, srvFd);
	test_common.listenFd = srvFd;

	memset(&srvAddr, 0, sizeof(srvAddr));
	srvAddr.sin_family = AF_INET;
	srvAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	srvAddr.sin_port = 0;

	ret = bind(srvFd, (struct sockaddr *)&srvAddr, sizeof(srvAddr));
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = listen(srvFd, GETPEERNAME_BACKLOG);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Find assigned port */
	addrLen = sizeof(srvAddr);
	ret = getsockname(srvFd, (struct sockaddr *)&srvAddr, &addrLen);
	TEST_ASSERT_EQUAL_INT(0, ret);

	cliFd = socket(AF_INET, SOCK_STREAM, 0);
	TEST_ASSERT_NOT_EQUAL_INT(-1, cliFd);
	test_common.clientFd = cliFd;

	flags = fcntl(cliFd, F_GETFL, 0);
	fcntl(cliFd, F_SETFL, flags | O_NONBLOCK);

	ret = connect(cliFd, (struct sockaddr *)&srvAddr, sizeof(srvAddr));
	TEST_ASSERT_TRUE(ret == 0 || (ret == -1 && errno == EINPROGRESS));

	accFd = accept(srvFd, NULL, NULL);
	TEST_ASSERT_TRUE(accFd >= 0);
	test_common.acceptFd = accFd;

	fcntl(cliFd, F_SETFL, flags);

	/* getpeername on client should return server address */
	memset(&peerAddr, 0, sizeof(peerAddr));
	addrLen = sizeof(peerAddr);
	ret = getpeername(cliFd, (struct sockaddr *)&peerAddr, &addrLen);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(AF_INET, peerAddr.sin_family);
	TEST_ASSERT_EQUAL_UINT32(htonl(INADDR_LOOPBACK), peerAddr.sin_addr.s_addr);
	TEST_ASSERT_EQUAL_INT(srvAddr.sin_port, peerAddr.sin_port);
}


TEST_GROUP_RUNNER(socket_api_getpeername)
{
	RUN_TEST_CASE(socket_api_getpeername, getpeername_success_accepted);
	RUN_TEST_CASE(socket_api_getpeername, getpeername_success_client);
	RUN_TEST_CASE(socket_api_getpeername, getpeername_truncation);
	RUN_TEST_CASE(socket_api_getpeername, getpeername_ebadf);
	RUN_TEST_CASE(socket_api_getpeername, getpeername_enotsock);
	RUN_TEST_CASE(socket_api_getpeername, getpeername_enotconn);
	RUN_TEST_CASE(socket_api_getpeername, getpeername_inet_loopback);
}
