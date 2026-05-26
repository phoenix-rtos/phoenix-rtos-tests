/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <sys/socket.h>
 * TESTED:
 *    - socketpair()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Lukasz Kruszynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <sys/socket.h>
#include <sys/resource.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include "unity_fixture.h"

#define SOCKETPAIR_MSG     "hello"
#define SOCKETPAIR_MSG_LEN 5
#define SOCKETPAIR_BUF_SZ  32

TEST_GROUP(socket_api_socketpair);

TEST_SETUP(socket_api_socketpair)
{
}

TEST_TEAR_DOWN(socket_api_socketpair)
{
}


TEST(socket_api_socketpair, stream_success)
{
	int sv[2];
	int ret;

	ret = socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_TRUE(sv[0] >= 0);
	TEST_ASSERT_TRUE(sv[1] >= 0);
	TEST_ASSERT_NOT_EQUAL_INT(sv[0], sv[1]);

	close(sv[0]);
	close(sv[1]);
}


TEST(socket_api_socketpair, dgram_success)
{
	int sv[2];
	int ret;

	ret = socketpair(AF_UNIX, SOCK_DGRAM, 0, sv);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_TRUE(sv[0] >= 0);
	TEST_ASSERT_TRUE(sv[1] >= 0);
	TEST_ASSERT_NOT_EQUAL_INT(sv[0], sv[1]);

	close(sv[0]);
	close(sv[1]);
}


TEST(socket_api_socketpair, bidirectional_stream)
{
	int sv[2];
	int ret;
	ssize_t n;
	char buf[SOCKETPAIR_BUF_SZ];

	ret = socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* sv[0] -> sv[1] */
	n = write(sv[0], SOCKETPAIR_MSG, SOCKETPAIR_MSG_LEN);
	TEST_ASSERT_EQUAL_INT(SOCKETPAIR_MSG_LEN, (int)n);

	memset(buf, 0, sizeof(buf));
	n = read(sv[1], buf, sizeof(buf));
	TEST_ASSERT_EQUAL_INT(SOCKETPAIR_MSG_LEN, (int)n);
	TEST_ASSERT_EQUAL_MEMORY(SOCKETPAIR_MSG, buf, SOCKETPAIR_MSG_LEN);

	/* sv[1] -> sv[0] */
	n = write(sv[1], SOCKETPAIR_MSG, SOCKETPAIR_MSG_LEN);
	TEST_ASSERT_EQUAL_INT(SOCKETPAIR_MSG_LEN, (int)n);

	memset(buf, 0, sizeof(buf));
	n = read(sv[0], buf, sizeof(buf));
	TEST_ASSERT_EQUAL_INT(SOCKETPAIR_MSG_LEN, (int)n);
	TEST_ASSERT_EQUAL_MEMORY(SOCKETPAIR_MSG, buf, SOCKETPAIR_MSG_LEN);

	close(sv[0]);
	close(sv[1]);
}


TEST(socket_api_socketpair, bidirectional_dgram)
{
	int sv[2];
	int ret;
	ssize_t n;
	char buf[SOCKETPAIR_BUF_SZ];

	ret = socketpair(AF_UNIX, SOCK_DGRAM, 0, sv);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* sv[0] -> sv[1] */
	n = write(sv[0], SOCKETPAIR_MSG, SOCKETPAIR_MSG_LEN);
	TEST_ASSERT_EQUAL_INT(SOCKETPAIR_MSG_LEN, (int)n);

	memset(buf, 0, sizeof(buf));
	n = read(sv[1], buf, sizeof(buf));
	TEST_ASSERT_EQUAL_INT(SOCKETPAIR_MSG_LEN, (int)n);
	TEST_ASSERT_EQUAL_MEMORY(SOCKETPAIR_MSG, buf, SOCKETPAIR_MSG_LEN);

	/* sv[1] -> sv[0] */
	n = write(sv[1], SOCKETPAIR_MSG, SOCKETPAIR_MSG_LEN);
	TEST_ASSERT_EQUAL_INT(SOCKETPAIR_MSG_LEN, (int)n);

	memset(buf, 0, sizeof(buf));
	n = read(sv[0], buf, sizeof(buf));
	TEST_ASSERT_EQUAL_INT(SOCKETPAIR_MSG_LEN, (int)n);
	TEST_ASSERT_EQUAL_MEMORY(SOCKETPAIR_MSG, buf, SOCKETPAIR_MSG_LEN);

	close(sv[0]);
	close(sv[1]);
}


TEST(socket_api_socketpair, fd_allocation)
{
	/* File descriptors shall be allocated as described in File Descriptor Allocation
	 * (lowest available) */
	int sv[2];
	int ret;
	int lowFd;

	/* Create a socket pair, close sv[0], then create again - new fd should reuse freed slot */
	ret = socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
	TEST_ASSERT_EQUAL_INT(0, ret);

	lowFd = (sv[0] < sv[1]) ? sv[0] : sv[1];
	close(sv[0]);
	close(sv[1]);

	/* After closing both, creating a new pair should allocate fds starting from lowFd or lower */
	ret = socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_TRUE(sv[0] <= lowFd || sv[1] <= lowFd);

	close(sv[0]);
	close(sv[1]);
}


TEST(socket_api_socketpair, eafnosupport_invalid_domain)
{
	int sv[2];
	int ret;

	errno = 0;
	ret = socketpair(-1, SOCK_STREAM, 0, sv);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EAFNOSUPPORT, errno);
}


TEST(socket_api_socketpair, eprotonosupport_invalid_protocol)
{
	int sv[2];
	int ret;

	errno = 0;
	ret = socketpair(AF_UNIX, SOCK_STREAM, 9999, sv);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EPROTONOSUPPORT, errno);
}


TEST(socket_api_socketpair, eopnotsupp_inet)
{
	int sv[2];
	int ret;

	/* AF_INET does not support socketpair */
	errno = 0;
	ret = socketpair(AF_INET, SOCK_STREAM, 0, sv);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	/* May return EAFNOSUPPORT or EOPNOTSUPP depending on implementation */
	TEST_ASSERT_TRUE(errno == EOPNOTSUPP || errno == EAFNOSUPPORT);
}


TEST(socket_api_socketpair, eprototype_invalid_type)
{
	int sv[2];
	int ret;

	errno = 0;
	ret = socketpair(AF_UNIX, -1, 0, sv);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	/* EPROTOTYPE: socket type not supported by protocol */
	TEST_ASSERT_TRUE(errno == EPROTOTYPE || errno == EINVAL);
}


TEST(socket_api_socketpair, vector_unmodified_on_error)
{
	int sv[2];
	int ret;

	sv[0] = -42;
	sv[1] = -42;

	ret = socketpair(-1, SOCK_STREAM, 0, sv);
	TEST_ASSERT_EQUAL_INT(-1, ret);

	/* On error, contents of socket_vector shall be left unmodified */
#ifndef __phoenix__
	TEST_IGNORE_MESSAGE("Linux kernel may modify socket_vector before detecting error");
#endif
	TEST_ASSERT_EQUAL_INT(-42, sv[0]);
	TEST_ASSERT_EQUAL_INT(-42, sv[1]);
}


TEST(socket_api_socketpair, seqpacket_success)
{
	int sv[2];
	int ret;

	ret = socketpair(AF_UNIX, SOCK_SEQPACKET, 0, sv);
	if (ret == -1 && errno == EPROTONOSUPPORT) {
		TEST_IGNORE_MESSAGE("SOCK_SEQPACKET not supported on this platform");
	}
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_TRUE(sv[0] >= 0);
	TEST_ASSERT_TRUE(sv[1] >= 0);

	close(sv[0]);
	close(sv[1]);
}


TEST(socket_api_socketpair, protocol_zero_default)
{
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1644");
#endif
	/* Specifying a protocol of 0 causes socketpair() to use an unspecified default
	 * protocol appropriate for the requested socket type */
	int sv[2];
	int ret;
	int optval;
	socklen_t optlen;

	ret = socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Verify the socket type is correct */
	optlen = sizeof(optval);
	ret = getsockopt(sv[0], SOL_SOCKET, SO_TYPE, &optval, &optlen);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(SOCK_STREAM, optval);

	close(sv[0]);
	close(sv[1]);
}


TEST(socket_api_socketpair, emfile_resource_exhaustion)
{
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("setrlimit(RLIMIT_NOFILE) not enforced");
#endif
	int sv[2];
	int ret;
	struct rlimit rl;
	struct rlimit oldrl;
	int dupfds[16];
	int ndups;
	int i;

	/* Lower RLIMIT_NOFILE to a small value to exhaust fd table quickly */
	ret = getrlimit(RLIMIT_NOFILE, &oldrl);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Set soft limit to current fd count + 1 so no room for a pair */
	rl.rlim_cur = 4;
	rl.rlim_max = oldrl.rlim_max;
	ret = setrlimit(RLIMIT_NOFILE, &rl);
	if (ret != 0) {
		/* If we can't set rlimit, exhaust by duping */
		TEST_IGNORE_MESSAGE("Cannot set RLIMIT_NOFILE, skipping EMFILE test");
	}

	/* Use up remaining slots by duping */
	ndups = 0;
	for (i = 0; i < 16; i++) {
		dupfds[i] = dup(STDIN_FILENO);
		if (dupfds[i] < 0) {
			break;
		}
		ndups++;
	}

	errno = 0;
	ret = socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EMFILE, errno);

	/* Cleanup */
	for (i = 0; i < ndups; i++) {
		close(dupfds[i]);
	}
	setrlimit(RLIMIT_NOFILE, &oldrl);
}


TEST_GROUP_RUNNER(socket_api_socketpair)
{
	RUN_TEST_CASE(socket_api_socketpair, stream_success);
	RUN_TEST_CASE(socket_api_socketpair, dgram_success);
	RUN_TEST_CASE(socket_api_socketpair, bidirectional_stream);
	RUN_TEST_CASE(socket_api_socketpair, bidirectional_dgram);
	RUN_TEST_CASE(socket_api_socketpair, fd_allocation);
	RUN_TEST_CASE(socket_api_socketpair, eafnosupport_invalid_domain);
	RUN_TEST_CASE(socket_api_socketpair, eprotonosupport_invalid_protocol);
	RUN_TEST_CASE(socket_api_socketpair, eopnotsupp_inet);
	RUN_TEST_CASE(socket_api_socketpair, eprototype_invalid_type);
	RUN_TEST_CASE(socket_api_socketpair, vector_unmodified_on_error);
	RUN_TEST_CASE(socket_api_socketpair, seqpacket_success);
	RUN_TEST_CASE(socket_api_socketpair, protocol_zero_default);
	RUN_TEST_CASE(socket_api_socketpair, emfile_resource_exhaustion);
}
