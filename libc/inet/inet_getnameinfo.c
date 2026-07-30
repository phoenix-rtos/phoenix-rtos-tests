/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <netdb.h>
 * TESTED:
 *    - getnameinfo()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Lukasz Kruszynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define _XOPEN_SOURCE 700

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

#include <unity_fixture.h>

#define LOOPBACK_V4 "127.0.0.1"
#define SERVICE_STR "80"
#define SERVICE_NUM 80

#define NODE_BUFSZ 64
#define SERV_BUFSZ 16

/* An address family value that shall not be recognised by getnameinfo(). */
#define BOGUS_FAMILY 0x7f


TEST_GROUP(inet_getnameinfo);


TEST_SETUP(inet_getnameinfo)
{
}


TEST_TEAR_DOWN(inet_getnameinfo)
{
}


/* Fill in a socket address for the IPv4 loopback host and the test service. */
static void test_makeLoopbackV4(struct sockaddr_in *sin)
{
	memset(sin, 0, sizeof(*sin));
	sin->sin_family = AF_INET;
	sin->sin_port = htons(SERVICE_NUM);
	sin->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
}


/*
 * With NI_NUMERICHOST and NI_NUMERICSERV the numeric forms of the address and
 * the port shall be returned as null-terminated strings, and the call succeeds.
 */
TEST(inet_getnameinfo, numeric_host_and_service)
{
	struct sockaddr_in sin;
	char node[NODE_BUFSZ];
	char service[SERV_BUFSZ];

	test_makeLoopbackV4(&sin);

	TEST_ASSERT_EQUAL_INT(0, getnameinfo((const struct sockaddr *)&sin, sizeof(sin),
		node, sizeof(node), service, sizeof(service), NI_NUMERICHOST | NI_NUMERICSERV));

	TEST_ASSERT_EQUAL_STRING(LOOPBACK_V4, node);
	TEST_ASSERT_EQUAL_STRING(SERVICE_STR, service);
}


/* A null service argument shall suppress the service name; the node is returned. */
TEST(inet_getnameinfo, node_only)
{
	struct sockaddr_in sin;
	char node[NODE_BUFSZ];

	test_makeLoopbackV4(&sin);

	TEST_ASSERT_EQUAL_INT(0, getnameinfo((const struct sockaddr *)&sin, sizeof(sin),
		node, sizeof(node), NULL, 0, NI_NUMERICHOST | NI_NUMERICSERV));

	TEST_ASSERT_EQUAL_STRING(LOOPBACK_V4, node);
}


/* A null node argument shall suppress the node name; the service is returned. */
TEST(inet_getnameinfo, service_only)
{
	struct sockaddr_in sin;
	char service[SERV_BUFSZ];

	test_makeLoopbackV4(&sin);

	TEST_ASSERT_EQUAL_INT(0, getnameinfo((const struct sockaddr *)&sin, sizeof(sin),
		NULL, 0, service, sizeof(service), NI_NUMERICHOST | NI_NUMERICSERV));

	TEST_ASSERT_EQUAL_STRING(SERVICE_STR, service);
}


/* A node buffer too small to hold the result shall fail with EAI_OVERFLOW. */
TEST(inet_getnameinfo, node_overflow)
{
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1727 issue");
#else
	struct sockaddr_in sin;
	char node[4];
	char service[SERV_BUFSZ];

	test_makeLoopbackV4(&sin);

	TEST_ASSERT_EQUAL_INT(EAI_OVERFLOW, getnameinfo((const struct sockaddr *)&sin, sizeof(sin),
		node, sizeof(node), service, sizeof(service), NI_NUMERICHOST | NI_NUMERICSERV));
#endif
}


/* A service buffer too small to hold the result shall fail with EAI_OVERFLOW. */
TEST(inet_getnameinfo, service_overflow)
{
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1727 issue");
#else
	struct sockaddr_in sin;
	char node[NODE_BUFSZ];
	char service[1];

	test_makeLoopbackV4(&sin);

	TEST_ASSERT_EQUAL_INT(EAI_OVERFLOW, getnameinfo((const struct sockaddr *)&sin, sizeof(sin),
		node, sizeof(node), service, sizeof(service), NI_NUMERICHOST | NI_NUMERICSERV));
#endif
}


/* An unrecognised address family shall fail with EAI_FAMILY. */
TEST(inet_getnameinfo, bad_family)
{
	struct sockaddr sa;
	char node[NODE_BUFSZ];

	memset(&sa, 0, sizeof(sa));
	sa.sa_family = BOGUS_FAMILY;

	TEST_ASSERT_EQUAL_INT(EAI_FAMILY, getnameinfo(&sa, sizeof(sa),
		node, sizeof(node), NULL, 0, NI_NUMERICHOST));
}


TEST_GROUP_RUNNER(inet_getnameinfo)
{
	RUN_TEST_CASE(inet_getnameinfo, numeric_host_and_service);
	RUN_TEST_CASE(inet_getnameinfo, node_only);
	RUN_TEST_CASE(inet_getnameinfo, service_only);
	RUN_TEST_CASE(inet_getnameinfo, node_overflow);
	RUN_TEST_CASE(inet_getnameinfo, service_overflow);
	RUN_TEST_CASE(inet_getnameinfo, bad_family);
}
