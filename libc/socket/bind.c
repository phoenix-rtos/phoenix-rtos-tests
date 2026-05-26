/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <sys/socket.h>
 * TESTED:
 *    - bind()
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
#include <sys/stat.h>

#include "unity_fixture.h"

#define BIND_SOCK_PATH    "/tmp/test_bind_sock"
#define BIND_SOCK_PATH2   "/tmp/test_bind_sock2"
#define BIND_NOENT_PATH   "/tmp/test_bind_noent_dir/sock"
#define BIND_SYMLINK_PATH "/tmp/test_bind_symlink"


static struct {
	int fd;
	int fd2;
} test_common;


TEST_GROUP(socket_api_bind);


TEST_SETUP(socket_api_bind)
{
	unlink(BIND_SOCK_PATH);
	unlink(BIND_SOCK_PATH2);
	unlink(BIND_SYMLINK_PATH);
	test_common.fd = -1;
	test_common.fd2 = -1;
}


TEST_TEAR_DOWN(socket_api_bind)
{
	if (test_common.fd >= 0) {
		close(test_common.fd);
		test_common.fd = -1;
	}
	if (test_common.fd2 >= 0) {
		close(test_common.fd2);
		test_common.fd2 = -1;
	}
	unlink(BIND_SOCK_PATH);
	unlink(BIND_SOCK_PATH2);
	unlink(BIND_SYMLINK_PATH);
}


TEST(socket_api_bind, bind_unix_success)
{
	/* "bind() shall assign a local socket address address to a socket" */
	struct sockaddr_un addr;
	int ret;

	test_common.fd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.fd);

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, BIND_SOCK_PATH, sizeof(addr.sun_path) - 1);

	errno = 0;
	ret = bind(test_common.fd, (struct sockaddr *)&addr, sizeof(addr));
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, errno);

	/* Socket file should exist */
	struct stat st;
	TEST_ASSERT_EQUAL_INT(0, stat(BIND_SOCK_PATH, &st));
}


TEST(socket_api_bind, bind_unix_dgram_success)
{
	/* bind works for SOCK_DGRAM as well */
	struct sockaddr_un addr;
	int ret;

	test_common.fd = socket(AF_UNIX, SOCK_DGRAM, 0);
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.fd);

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, BIND_SOCK_PATH, sizeof(addr.sun_path) - 1);

	ret = bind(test_common.fd, (struct sockaddr *)&addr, sizeof(addr));
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST(socket_api_bind, bind_inet_success)
{
	/* bind to INADDR_LOOPBACK with port 0 (kernel assigns) */
	struct sockaddr_in addr;
	int ret;

	test_common.fd = socket(AF_INET, SOCK_STREAM, 0);
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.fd);

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port = 0;

	ret = bind(test_common.fd, (struct sockaddr *)&addr, sizeof(addr));
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST(socket_api_bind, bind_ebadf)
{
	/* "EBADF: The socket argument is not a valid file descriptor." */
	struct sockaddr_un addr;
	int ret;

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, BIND_SOCK_PATH, sizeof(addr.sun_path) - 1);

	errno = 0;
	ret = bind(-1, (struct sockaddr *)&addr, sizeof(addr));
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


TEST(socket_api_bind, bind_enotsock)
{
	/* "ENOTSOCK: The socket argument does not refer to a socket." */
	struct sockaddr_un addr;
	int ret;

	test_common.fd = open("/dev/null", O_RDONLY);
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.fd);

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, BIND_SOCK_PATH, sizeof(addr.sun_path) - 1);

	errno = 0;
	ret = bind(test_common.fd, (struct sockaddr *)&addr, sizeof(addr));
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(ENOTSOCK, errno);
}


TEST(socket_api_bind, bind_eaddrinuse)
{
	/* "EADDRINUSE: The specified address is already in use." */
	struct sockaddr_un addr;
	int ret;

	test_common.fd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.fd);

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, BIND_SOCK_PATH, sizeof(addr.sun_path) - 1);

	ret = bind(test_common.fd, (struct sockaddr *)&addr, sizeof(addr));
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Second bind to same path */
	test_common.fd2 = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.fd2);

	errno = 0;
	ret = bind(test_common.fd2, (struct sockaddr *)&addr, sizeof(addr));
	TEST_ASSERT_EQUAL_INT(-1, ret);
#ifdef __phoenix__
	/* https://github.com/phoenix-rtos/phoenix-rtos-project/issues/1667 */
	TEST_IGNORE_MESSAGE("#1667 issue");
#else
	TEST_ASSERT_EQUAL_INT(EADDRINUSE, errno);
#endif
}


TEST(socket_api_bind, bind_eaddrinuse_symlink)
{
	/* "If the address family of the socket is AF_UNIX and the pathname
	 *  in address names a symbolic link, bind() shall fail and set errno
	 *  to [EADDRINUSE]." */
	struct sockaddr_un addr;
	int ret;

	/* Create a symlink at the target path */
	ret = symlink("/tmp", BIND_SOCK_PATH);
	TEST_ASSERT_EQUAL_INT(0, ret);

	test_common.fd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.fd);

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, BIND_SOCK_PATH, sizeof(addr.sun_path) - 1);

	errno = 0;
	ret = bind(test_common.fd, (struct sockaddr *)&addr, sizeof(addr));
	TEST_ASSERT_EQUAL_INT(-1, ret);
#ifdef __phoenix__
	/* https://github.com/phoenix-rtos/phoenix-rtos-project/issues/1667 */
	TEST_IGNORE_MESSAGE("#1667 issue");
#else
	TEST_ASSERT_EQUAL_INT(EADDRINUSE, errno);
#endif
}


TEST(socket_api_bind, bind_einval_already_bound)
{
	/* "EINVAL: The socket is already bound to an address, and the protocol
	 *  does not support binding to a new address" */
	struct sockaddr_un addr1, addr2;
	int ret;

	test_common.fd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.fd);

	memset(&addr1, 0, sizeof(addr1));
	addr1.sun_family = AF_UNIX;
	strncpy(addr1.sun_path, BIND_SOCK_PATH, sizeof(addr1.sun_path) - 1);

	ret = bind(test_common.fd, (struct sockaddr *)&addr1, sizeof(addr1));
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Try to rebind to different address */
	memset(&addr2, 0, sizeof(addr2));
	addr2.sun_family = AF_UNIX;
	strncpy(addr2.sun_path, BIND_SOCK_PATH2, sizeof(addr2.sun_path) - 1);

	errno = 0;
	ret = bind(test_common.fd, (struct sockaddr *)&addr2, sizeof(addr2));
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST(socket_api_bind, bind_enoent_missing_dir)
{
	/* "ENOENT: A component of the path prefix ... does not name an existing file" */
	struct sockaddr_un addr;
	int ret;

	test_common.fd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.fd);

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, BIND_NOENT_PATH, sizeof(addr.sun_path) - 1);

	errno = 0;
	ret = bind(test_common.fd, (struct sockaddr *)&addr, sizeof(addr));
	TEST_ASSERT_EQUAL_INT(-1, ret);
#ifdef __phoenix__
	/* https://github.com/phoenix-rtos/phoenix-rtos-project/issues/1668 */
	TEST_IGNORE_MESSAGE("#1668 issue");
#else
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);
#endif
}


TEST(socket_api_bind, bind_eafnosupport)
{
	/* "EAFNOSUPPORT: The specified address is not a valid address for
	 *  the address family of the specified socket." */
	struct sockaddr_in inetAddr;
	int ret;

	/* AF_UNIX socket but AF_INET address */
	test_common.fd = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.fd);

	memset(&inetAddr, 0, sizeof(inetAddr));
	inetAddr.sin_family = AF_INET;
	inetAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	inetAddr.sin_port = htons(0);

	errno = 0;
	ret = bind(test_common.fd, (struct sockaddr *)&inetAddr, sizeof(inetAddr));
	TEST_ASSERT_EQUAL_INT(-1, ret);
	/* EAFNOSUPPORT or EINVAL are acceptable */
	TEST_ASSERT_TRUE(errno == EAFNOSUPPORT || errno == EINVAL);
}


TEST_GROUP_RUNNER(socket_api_bind)
{
	RUN_TEST_CASE(socket_api_bind, bind_unix_success);
	RUN_TEST_CASE(socket_api_bind, bind_unix_dgram_success);
	RUN_TEST_CASE(socket_api_bind, bind_inet_success);
	RUN_TEST_CASE(socket_api_bind, bind_ebadf);
	RUN_TEST_CASE(socket_api_bind, bind_enotsock);
	RUN_TEST_CASE(socket_api_bind, bind_eaddrinuse);
	RUN_TEST_CASE(socket_api_bind, bind_eaddrinuse_symlink);
	RUN_TEST_CASE(socket_api_bind, bind_einval_already_bound);
	RUN_TEST_CASE(socket_api_bind, bind_enoent_missing_dir);
	RUN_TEST_CASE(socket_api_bind, bind_eafnosupport);
}
