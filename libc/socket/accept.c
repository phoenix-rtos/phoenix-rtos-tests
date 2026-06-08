/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <sys/socket.h>
 * TESTED:
 *    - accept()
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
#include <sys/wait.h>
#include <signal.h>

#include "unity_fixture.h"

#define ACCEPT_SOCK_PATH "/tmp/test_accept_sock"
#define ACCEPT_BACKLOG   5


static struct {
	int listenFd;
	int acceptFd;
	int clientFd;
} test_common;


TEST_GROUP(socket_api_accept);


TEST_SETUP(socket_api_accept)
{
	unlink(ACCEPT_SOCK_PATH);
	test_common.listenFd = -1;
	test_common.acceptFd = -1;
	test_common.clientFd = -1;
}


TEST_TEAR_DOWN(socket_api_accept)
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
	unlink(ACCEPT_SOCK_PATH);
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
	strncpy(addr.sun_path, ACCEPT_SOCK_PATH, sizeof(addr.sun_path) - 1);

	ret = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
	if (ret != 0) {
		close(fd);
		return -1;
	}

	ret = listen(fd, ACCEPT_BACKLOG);
	if (ret != 0) {
		close(fd);
		return -1;
	}

	return fd;
}


static int test_connectClient(void)
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
	strncpy(addr.sun_path, ACCEPT_SOCK_PATH, sizeof(addr.sun_path) - 1);

	ret = connect(fd, (struct sockaddr *)&addr, sizeof(addr));
	if (ret != 0) {
		close(fd);
		return -1;
	}

	return fd;
}


TEST(socket_api_accept, accept_success)
{
	/* "accept() shall extract the first connection on the queue of pending
	 *  connections, create a new socket ... and allocate a new file descriptor" */
	int ret;

	test_common.listenFd = test_setupListener();
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.listenFd);

	test_common.clientFd = test_connectClient();
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.clientFd);

	test_common.acceptFd = accept(test_common.listenFd, NULL, NULL);
	TEST_ASSERT_TRUE(test_common.acceptFd >= 0);

	/* Accepted fd is different from listen fd */
	TEST_ASSERT_NOT_EQUAL_INT(test_common.listenFd, test_common.acceptFd);

	/* Verify connection works: send data through */
	const char msg[] = "hello";
	char buf[16];
	ret = (int)write(test_common.clientFd, msg, sizeof(msg));
	TEST_ASSERT_EQUAL_INT((int)sizeof(msg), ret);

	ret = (int)read(test_common.acceptFd, buf, sizeof(buf));
	TEST_ASSERT_EQUAL_INT((int)sizeof(msg), ret);
	TEST_ASSERT_EQUAL_STRING(msg, buf);
}


TEST(socket_api_accept, accept_with_address)
{
	/* "If address is not a null pointer, the address of the peer for
	 *  the accepted connection shall be stored" */
	struct sockaddr_un peerAddr;
	socklen_t addrLen = sizeof(peerAddr);

	test_common.listenFd = test_setupListener();
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.listenFd);

	test_common.clientFd = test_connectClient();
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.clientFd);

	memset(&peerAddr, 0xff, sizeof(peerAddr));
	test_common.acceptFd = accept(test_common.listenFd, (struct sockaddr *)&peerAddr, &addrLen);
	TEST_ASSERT_TRUE(test_common.acceptFd >= 0);

	/* For AF_UNIX, the address family should be AF_UNIX */
	TEST_ASSERT_EQUAL_INT(AF_UNIX, peerAddr.sun_family);
}


TEST(socket_api_accept, accept_null_address)
{
	/* "address: Either a null pointer ..." — must succeed with NULL */
	test_common.listenFd = test_setupListener();
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.listenFd);

	test_common.clientFd = test_connectClient();
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.clientFd);

	test_common.acceptFd = accept(test_common.listenFd, NULL, NULL);
	TEST_ASSERT_TRUE(test_common.acceptFd >= 0);
}


TEST(socket_api_accept, accept_original_still_accepts)
{
	/* "The original socket remains open and can accept more connections." */
	int secondClient;
	int secondAccept;

	test_common.listenFd = test_setupListener();
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.listenFd);

	test_common.clientFd = test_connectClient();
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.clientFd);

	test_common.acceptFd = accept(test_common.listenFd, NULL, NULL);
	TEST_ASSERT_TRUE(test_common.acceptFd >= 0);

	/* Second connection */
	secondClient = test_connectClient();
	TEST_ASSERT_NOT_EQUAL_INT(-1, secondClient);

	secondAccept = accept(test_common.listenFd, NULL, NULL);
	TEST_ASSERT_TRUE(secondAccept >= 0);

	close(secondClient);
	close(secondAccept);
}


TEST(socket_api_accept, accept_ebadf)
{
	/* "EBADF: The socket argument is not a valid file descriptor." */
	int ret;

	errno = 0;
	ret = accept(-1, NULL, NULL);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


TEST(socket_api_accept, accept_enotsock)
{
	/* "ENOTSOCK: The socket argument does not refer to a socket." */
	int fd;
	int ret;

	fd = open("/dev/null", O_RDONLY);
	TEST_ASSERT_NOT_EQUAL_INT(-1, fd);

	errno = 0;
	ret = accept(fd, NULL, NULL);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(ENOTSOCK, errno);

	close(fd);
}


TEST(socket_api_accept, accept_einval_not_listening)
{
	/* "EINVAL: The socket is not accepting connections." */
	int fd;
	int ret;

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_NOT_EQUAL_INT(-1, fd);

	/* Socket created but not listening */
	errno = 0;
	ret = accept(fd, NULL, NULL);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);

	close(fd);
}


TEST(socket_api_accept, accept_eagain_nonblock)
{
	/* "EAGAIN or EWOULDBLOCK: O_NONBLOCK is set ... and no connections
	 *  are present to be accepted." */
	int flags;
	int ret;

	test_common.listenFd = test_setupListener();
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.listenFd);

	/* Set non-blocking */
	flags = fcntl(test_common.listenFd, F_GETFL, 0);
	TEST_ASSERT_NOT_EQUAL_INT(-1, flags);
	TEST_ASSERT_EQUAL_INT(0, fcntl(test_common.listenFd, F_SETFL, flags | O_NONBLOCK));

	errno = 0;
	ret = accept(test_common.listenFd, NULL, NULL);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_TRUE(errno == EAGAIN || errno == EWOULDBLOCK);
}


TEST(socket_api_accept, accept_eopnotsupp_dgram)
{
	/* "EOPNOTSUPP: The socket type of the specified socket does not
	 *  support accepting connections." */
	int fd;
	int ret;

	fd = socket(AF_UNIX, SOCK_DGRAM, 0);
	TEST_ASSERT_NOT_EQUAL_INT(-1, fd);

	errno = 0;
	ret = accept(fd, NULL, NULL);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	/* EOPNOTSUPP or EINVAL are both acceptable depending on implementation */
	TEST_ASSERT_TRUE(errno == EOPNOTSUPP || errno == EINVAL);

	close(fd);
}


TEST_GROUP_RUNNER(socket_api_accept)
{
	RUN_TEST_CASE(socket_api_accept, accept_success);
	RUN_TEST_CASE(socket_api_accept, accept_with_address);
	RUN_TEST_CASE(socket_api_accept, accept_null_address);
	RUN_TEST_CASE(socket_api_accept, accept_original_still_accepts);
	RUN_TEST_CASE(socket_api_accept, accept_ebadf);
	RUN_TEST_CASE(socket_api_accept, accept_enotsock);
	RUN_TEST_CASE(socket_api_accept, accept_einval_not_listening);
	RUN_TEST_CASE(socket_api_accept, accept_eagain_nonblock);
	RUN_TEST_CASE(socket_api_accept, accept_eopnotsupp_dgram);
}
