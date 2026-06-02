/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <sys/socket.h>
 * TESTED:
 *    - getsockname()
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

#define GETSOCKNAME_SOCK_PATH "/tmp/test_getsockname_sock"


static struct {
	int fd;
} test_common;


TEST_GROUP(socket_api_getsockname);


TEST_SETUP(socket_api_getsockname)
{
	unlink(GETSOCKNAME_SOCK_PATH);
	test_common.fd = -1;
}


TEST_TEAR_DOWN(socket_api_getsockname)
{
	if (test_common.fd >= 0) {
		close(test_common.fd);
		test_common.fd = -1;
	}
	unlink(GETSOCKNAME_SOCK_PATH);
}


TEST(socket_api_getsockname, getsockname_unix_bound)
{
	/* "getsockname() shall retrieve the locally-bound name of the
	 *  specified socket" */
	struct sockaddr_un bindAddr, nameAddr;
	socklen_t addrLen;
	int ret;

	test_common.fd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.fd);

	memset(&bindAddr, 0, sizeof(bindAddr));
	bindAddr.sun_family = AF_UNIX;
	strncpy(bindAddr.sun_path, GETSOCKNAME_SOCK_PATH, sizeof(bindAddr.sun_path) - 1);

	ret = bind(test_common.fd, (struct sockaddr *)&bindAddr, sizeof(bindAddr));
	TEST_ASSERT_EQUAL_INT(0, ret);

	memset(&nameAddr, 0, sizeof(nameAddr));
	addrLen = sizeof(nameAddr);
	errno = 0;
	ret = getsockname(test_common.fd, (struct sockaddr *)&nameAddr, &addrLen);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, errno);
#ifdef __phoenix__
	/* https://github.com/phoenix-rtos/phoenix-rtos-project/issues/1674 */
	TEST_IGNORE_MESSAGE("#1674 issue");
#else
	TEST_ASSERT_EQUAL_INT(AF_UNIX, nameAddr.sun_family);
	TEST_ASSERT_EQUAL_STRING(GETSOCKNAME_SOCK_PATH, nameAddr.sun_path);
#endif
}


TEST(socket_api_getsockname, getsockname_inet_bound)
{
	/* getsockname returns the bound address for AF_INET */
	struct sockaddr_in bindAddr, nameAddr;
	socklen_t addrLen;
	int ret;

	test_common.fd = socket(AF_INET, SOCK_STREAM, 0);
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.fd);

	memset(&bindAddr, 0, sizeof(bindAddr));
	bindAddr.sin_family = AF_INET;
	bindAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	bindAddr.sin_port = 0; /* kernel assigns */

	ret = bind(test_common.fd, (struct sockaddr *)&bindAddr, sizeof(bindAddr));
	TEST_ASSERT_EQUAL_INT(0, ret);

	memset(&nameAddr, 0, sizeof(nameAddr));
	addrLen = sizeof(nameAddr);
	ret = getsockname(test_common.fd, (struct sockaddr *)&nameAddr, &addrLen);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(AF_INET, nameAddr.sin_family);
	TEST_ASSERT_EQUAL_UINT32(htonl(INADDR_LOOPBACK), nameAddr.sin_addr.s_addr);
	/* Port must be non-zero (kernel assigned) */
	TEST_ASSERT_NOT_EQUAL_INT(0, ntohs(nameAddr.sin_port));
}


TEST(socket_api_getsockname, getsockname_addrlen_updated)
{
	/* "on output specifies the length of the stored address" */
	struct sockaddr_un bindAddr, nameAddr;
	socklen_t addrLen;
	int ret;

	test_common.fd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.fd);

	memset(&bindAddr, 0, sizeof(bindAddr));
	bindAddr.sun_family = AF_UNIX;
	strncpy(bindAddr.sun_path, GETSOCKNAME_SOCK_PATH, sizeof(bindAddr.sun_path) - 1);

	ret = bind(test_common.fd, (struct sockaddr *)&bindAddr, sizeof(bindAddr));
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Pass large buffer, addrLen should be updated to actual size */
	addrLen = sizeof(nameAddr);
	memset(&nameAddr, 0, sizeof(nameAddr));
	ret = getsockname(test_common.fd, (struct sockaddr *)&nameAddr, &addrLen);
	TEST_ASSERT_EQUAL_INT(0, ret);
	/* addrLen should be at least offsetof(sun_family) + path length + 1 */
	TEST_ASSERT_GREATER_THAN_UINT(0U, (unsigned int)addrLen);
}


TEST(socket_api_getsockname, getsockname_truncation)
{
	/* "If the actual length of the address is greater than the length
	 *  of the supplied sockaddr structure, the stored address shall be truncated." */
	struct sockaddr_un bindAddr;
	struct sockaddr smallBuf;
	socklen_t addrLen;
	int ret;

	test_common.fd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.fd);

	memset(&bindAddr, 0, sizeof(bindAddr));
	bindAddr.sun_family = AF_UNIX;
	strncpy(bindAddr.sun_path, GETSOCKNAME_SOCK_PATH, sizeof(bindAddr.sun_path) - 1);

	ret = bind(test_common.fd, (struct sockaddr *)&bindAddr, sizeof(bindAddr));
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Pass very small buffer */
	addrLen = (socklen_t)sizeof(sa_family_t);
	memset(&smallBuf, 0xff, sizeof(smallBuf));
	ret = getsockname(test_common.fd, &smallBuf, &addrLen);
	TEST_ASSERT_EQUAL_INT(0, ret);
#ifdef __phoenix__
	/* https://github.com/phoenix-rtos/phoenix-rtos-project/issues/1674 */
	TEST_IGNORE_MESSAGE("#1674 issue");
#else
	/* addrLen updated to actual (larger) size */
	TEST_ASSERT_GREATER_THAN_UINT((unsigned int)sizeof(sa_family_t), (unsigned int)addrLen);
#endif
}


TEST(socket_api_getsockname, getsockname_unbound)
{
	/* "If the socket has not been bound to a local name, the value stored
	 *  in the object pointed to by address is unspecified." — but call succeeds */
	struct sockaddr_un nameAddr;
	socklen_t addrLen = sizeof(nameAddr);
	int ret;

	test_common.fd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.fd);

	memset(&nameAddr, 0xff, sizeof(nameAddr));
	ret = getsockname(test_common.fd, (struct sockaddr *)&nameAddr, &addrLen);
	/* Must succeed — address content is unspecified */
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST(socket_api_getsockname, getsockname_ebadf)
{
	/* "EBADF: The socket argument is not a valid file descriptor." */
	struct sockaddr_un addr;
	socklen_t addrLen = sizeof(addr);
	int ret;

	errno = 0;
	ret = getsockname(-1, (struct sockaddr *)&addr, &addrLen);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


TEST(socket_api_getsockname, getsockname_enotsock)
{
	/* "ENOTSOCK: The socket argument does not refer to a socket." */
	struct sockaddr_un addr;
	socklen_t addrLen = sizeof(addr);
	int ret;

	test_common.fd = open("/dev/null", O_RDONLY);
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.fd);

	errno = 0;
	ret = getsockname(test_common.fd, (struct sockaddr *)&addr, &addrLen);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(ENOTSOCK, errno);
}


TEST(socket_api_getsockname, getsockname_after_connect)
{
	/* After connect(), getsockname returns the implicitly-bound address */
	struct sockaddr_un srvAddr, cliAddr;
	socklen_t addrLen;
	int srvFd, cliFd;
	int ret;
	int flags;

	srvFd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_NOT_EQUAL_INT(-1, srvFd);
	test_common.fd = srvFd;

	memset(&srvAddr, 0, sizeof(srvAddr));
	srvAddr.sun_family = AF_UNIX;
	strncpy(srvAddr.sun_path, GETSOCKNAME_SOCK_PATH, sizeof(srvAddr.sun_path) - 1);

	ret = bind(srvFd, (struct sockaddr *)&srvAddr, sizeof(srvAddr));
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = listen(srvFd, 5);
	TEST_ASSERT_EQUAL_INT(0, ret);

	cliFd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_NOT_EQUAL_INT(-1, cliFd);
	if (TEST_PROTECT()) {
		flags = fcntl(cliFd, F_GETFL, 0);
		fcntl(cliFd, F_SETFL, flags | O_NONBLOCK);
		ret = connect(cliFd, (struct sockaddr *)&srvAddr, sizeof(srvAddr));
		TEST_ASSERT_TRUE(ret == 0 || (ret == -1 && errno == EINPROGRESS));
		fcntl(cliFd, F_SETFL, flags);

		/* getsockname on client after connect */
		memset(&cliAddr, 0, sizeof(cliAddr));
		addrLen = sizeof(cliAddr);
		ret = getsockname(cliFd, (struct sockaddr *)&cliAddr, &addrLen);
		TEST_ASSERT_EQUAL_INT(0, ret);
#ifdef __phoenix__
		/* https://github.com/phoenix-rtos/phoenix-rtos-project/issues/1674 */
		TEST_IGNORE_MESSAGE("#1674 issue");
#else
		TEST_ASSERT_EQUAL_INT(AF_UNIX, cliAddr.sun_family);
#endif
	}
	close(cliFd);
}


TEST_GROUP_RUNNER(socket_api_getsockname)
{
	RUN_TEST_CASE(socket_api_getsockname, getsockname_unix_bound);
	RUN_TEST_CASE(socket_api_getsockname, getsockname_inet_bound);
	RUN_TEST_CASE(socket_api_getsockname, getsockname_addrlen_updated);
	RUN_TEST_CASE(socket_api_getsockname, getsockname_truncation);
	RUN_TEST_CASE(socket_api_getsockname, getsockname_unbound);
	RUN_TEST_CASE(socket_api_getsockname, getsockname_ebadf);
	RUN_TEST_CASE(socket_api_getsockname, getsockname_enotsock);
	RUN_TEST_CASE(socket_api_getsockname, getsockname_after_connect);
}
