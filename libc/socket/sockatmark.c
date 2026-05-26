/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <sys/socket.h>
 * TESTED:
 *    - sockatmark()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include "unity_fixture.h"

TEST_GROUP(socket_api_sockatmark);

static int testSock;

TEST_SETUP(socket_api_sockatmark)
{
	testSock = -1;
}

TEST_TEAR_DOWN(socket_api_sockatmark)
{
	if (testSock >= 0) {
		close(testSock);
		testSock = -1;
	}
}


TEST(socket_api_sockatmark, sockatmark_not_at_mark)
{

	testSock = socket(AF_INET, SOCK_STREAM, 0);
	TEST_ASSERT_TRUE(testSock >= 0);

#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("sockatmark is not implemented");
#else
	int ret;

	/* On a freshly created socket with no OOB data, sockatmark should return 0 */
	ret = sockatmark(testSock);
	TEST_ASSERT_EQUAL_INT(0, ret);
#endif
}


TEST(socket_api_sockatmark, sockatmark_ebadf)
{
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("sockatmark is not implemented");
#else
	int ret;

	errno = 0;
	ret = sockatmark(-1);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
#endif
}


TEST(socket_api_sockatmark, sockatmark_enotty_not_socket)
{
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("sockatmark is not implemented");
#else
	int ret;
	int fd;

	fd = open("/dev/null", O_RDONLY);
	TEST_ASSERT_TRUE(fd >= 0);

	if (TEST_PROTECT()) {
		errno = 0;
		ret = sockatmark(fd);
		TEST_ASSERT_EQUAL_INT(-1, ret);
		/* POSIX specifies ENOTTY when fd is not a socket */
		TEST_ASSERT_EQUAL_INT(ENOTTY, errno);
	}
	close(fd);
#endif
}


TEST(socket_api_sockatmark, sockatmark_udp_socket)
{
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("sockatmark is not implemented");
#else
	int ret;

	/* UDP doesn't support OOB; sockatmark should still not crash */
	testSock = socket(AF_INET, SOCK_DGRAM, 0);
	TEST_ASSERT_TRUE(testSock >= 0);

	ret = sockatmark(testSock);
	/* Implementation-defined for non-stream sockets, just verify no crash */
	TEST_ASSERT_TRUE(ret == 0 || ret == -1);
#endif
}


TEST_GROUP_RUNNER(socket_api_sockatmark)
{
	RUN_TEST_CASE(socket_api_sockatmark, sockatmark_not_at_mark);
	RUN_TEST_CASE(socket_api_sockatmark, sockatmark_ebadf);
	RUN_TEST_CASE(socket_api_sockatmark, sockatmark_enotty_not_socket);
	RUN_TEST_CASE(socket_api_sockatmark, sockatmark_udp_socket);
}
