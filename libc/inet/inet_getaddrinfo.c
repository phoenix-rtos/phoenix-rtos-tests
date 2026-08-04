/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <netdb.h>
 * TESTED:
 *    - getaddrinfo()
 *    - freeaddrinfo()
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
#define LIST_SCAN_MAX 4096U

/* An address family value that shall not be recognised by getaddrinfo(). */
#define BOGUS_FAMILY 0x7f


static struct {
	struct addrinfo *res;
} test_common;


TEST_GROUP(inet_getaddrinfo);


TEST_SETUP(inet_getaddrinfo)
{
#ifdef __TARGET_AARCH64A53
	TEST_IGNORE_MESSAGE("issue to investigate");
#endif
	test_common.res = NULL;
}


TEST_TEAR_DOWN(inet_getaddrinfo)
{
	if (test_common.res != NULL) {
		freeaddrinfo(test_common.res);
		test_common.res = NULL;
	}
}


/* Walk the returned list and assert it is NULL-terminated within a bound. */
static void test_assertListTerminated(const struct addrinfo *list)
{
	unsigned count;

	for (count = 0; count < LIST_SCAN_MAX; count++) {
		if (list == NULL) {
			break;
		}
		list = list->ai_next;
	}
	TEST_ASSERT_LESS_THAN_UINT(LIST_SCAN_MAX, count);
}


/*
 * A numeric IPv4 host and numeric service shall resolve, without any name
 * service, to a single AF_INET socket address carrying that address and port.
 */
TEST(inet_getaddrinfo, numeric_ipv4)
{
	struct addrinfo hints = { 0 };
	const struct sockaddr_in *sin;

	hints.ai_family = AF_INET;
	hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV;

	TEST_ASSERT_EQUAL_INT(0, getaddrinfo(LOOPBACK_V4, SERVICE_STR, &hints, &test_common.res));
	TEST_ASSERT_NOT_NULL(test_common.res);
	test_assertListTerminated(test_common.res);

	/* ai_family shall be usable to create the socket and match the request. */
	TEST_ASSERT_EQUAL_INT(AF_INET, test_common.res->ai_family);
	TEST_ASSERT_NOT_NULL(test_common.res->ai_addr);
	TEST_ASSERT_GREATER_OR_EQUAL_UINT(sizeof(struct sockaddr_in), test_common.res->ai_addrlen);

	sin = (const struct sockaddr_in *)test_common.res->ai_addr;
	TEST_ASSERT_EQUAL_INT(AF_INET, sin->sin_family);
	TEST_ASSERT_EQUAL_HEX32(htonl(INADDR_LOOPBACK), sin->sin_addr.s_addr);
	TEST_ASSERT_EQUAL_HEX16(htons(SERVICE_NUM), sin->sin_port);
}


/*
 * With AI_PASSIVE and a null nodename the IPv4 address portion shall be set to
 * INADDR_ANY, suitable for binding a listening socket.
 */
TEST(inet_getaddrinfo, passive_wildcard)
{
	struct addrinfo hints = { 0 };
	const struct sockaddr_in *sin;

	hints.ai_family = AF_INET;
	hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;

	TEST_ASSERT_EQUAL_INT(0, getaddrinfo(NULL, SERVICE_STR, &hints, &test_common.res));
	TEST_ASSERT_NOT_NULL(test_common.res);

	sin = (const struct sockaddr_in *)test_common.res->ai_addr;
	TEST_ASSERT_EQUAL_HEX32(htonl(INADDR_ANY), sin->sin_addr.s_addr);
	TEST_ASSERT_EQUAL_HEX16(htons(SERVICE_NUM), sin->sin_port);
}


/*
 * Without AI_PASSIVE and with a null nodename the IPv4 address portion shall be
 * set to the loopback address, suitable for a connect().
 */
TEST(inet_getaddrinfo, loopback_default)
{
	struct addrinfo hints = { 0 };
	const struct sockaddr_in *sin;

	hints.ai_family = AF_INET;
	hints.ai_flags = AI_NUMERICSERV;

	TEST_ASSERT_EQUAL_INT(0, getaddrinfo(NULL, SERVICE_STR, &hints, &test_common.res));
	TEST_ASSERT_NOT_NULL(test_common.res);

	sin = (const struct sockaddr_in *)test_common.res->ai_addr;
	TEST_ASSERT_EQUAL_HEX32(htonl(INADDR_LOOPBACK), sin->sin_addr.s_addr);
}


/* When servname is null, only network-level addresses (port zero) are returned. */
TEST(inet_getaddrinfo, null_service_zero_port)
{
	struct addrinfo hints = { 0 };
	const struct sockaddr_in *sin;

	hints.ai_family = AF_INET;
	hints.ai_flags = AI_NUMERICHOST;

	TEST_ASSERT_EQUAL_INT(0, getaddrinfo(LOOPBACK_V4, NULL, &hints, &test_common.res));
	TEST_ASSERT_NOT_NULL(test_common.res);

	sin = (const struct sockaddr_in *)test_common.res->ai_addr;
	TEST_ASSERT_EQUAL_HEX16(0, sin->sin_port);
}


/* A non-zero ai_socktype in hints shall limit results to that socket type. */
TEST(inet_getaddrinfo, socktype_filter)
{
	struct addrinfo hints = { 0 };
	const struct addrinfo *cur;

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV;

	TEST_ASSERT_EQUAL_INT(0, getaddrinfo(LOOPBACK_V4, SERVICE_STR, &hints, &test_common.res));
	TEST_ASSERT_NOT_NULL(test_common.res);

	for (cur = test_common.res; cur != NULL; cur = cur->ai_next) {
		TEST_ASSERT_EQUAL_INT(SOCK_DGRAM, cur->ai_socktype);
	}
}


/*
 * With AI_CANONNAME and a numeric host, no name translation is performed, so
 * ai_canonname shall refer to a string with the same contents as nodename.
 */
TEST(inet_getaddrinfo, canonname_numeric)
{
	struct addrinfo hints = { 0 };

	hints.ai_family = AF_INET;
	hints.ai_flags = AI_NUMERICHOST | AI_CANONNAME;

	TEST_ASSERT_EQUAL_INT(0, getaddrinfo(LOOPBACK_V4, NULL, &hints, &test_common.res));
	TEST_ASSERT_NOT_NULL(test_common.res);
	TEST_ASSERT_NOT_NULL(test_common.res->ai_canonname);
	TEST_ASSERT_EQUAL_STRING(LOOPBACK_V4, test_common.res->ai_canonname);
}


/* AI_NUMERICHOST with a non-numeric nodename shall fail with EAI_NONAME. */
TEST(inet_getaddrinfo, numerichost_nonnumeric_noname)
{
	struct addrinfo hints = { 0 };

	hints.ai_family = AF_INET;
	hints.ai_flags = AI_NUMERICHOST;

	TEST_ASSERT_EQUAL_INT(EAI_NONAME, getaddrinfo("localhost", SERVICE_STR, &hints, &test_common.res));
}


/* AI_NUMERICSERV with a non-numeric servname shall fail with EAI_NONAME. */
TEST(inet_getaddrinfo, numericserv_nonnumeric_noname)
{
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1726 issue");
#else
	struct addrinfo hints = { 0 };

	hints.ai_family = AF_INET;
	hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV;

	TEST_ASSERT_EQUAL_INT(EAI_NONAME, getaddrinfo(LOOPBACK_V4, "http", &hints, &test_common.res));
#endif
}


/* Neither nodename nor servname supplied shall fail with EAI_NONAME. */
TEST(inet_getaddrinfo, both_null_noname)
{
	struct addrinfo hints = { 0 };

	hints.ai_family = AF_INET;

	TEST_ASSERT_EQUAL_INT(EAI_NONAME, getaddrinfo(NULL, NULL, &hints, &test_common.res));
}


/* An unrecognised address family in the hints shall fail with EAI_FAMILY. */
TEST(inet_getaddrinfo, bad_family)
{
	struct addrinfo hints = { 0 };

	hints.ai_family = BOGUS_FAMILY;
	hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV;

	TEST_ASSERT_EQUAL_INT(EAI_FAMILY, getaddrinfo(LOOPBACK_V4, SERVICE_STR, &hints, &test_common.res));
}


/* A numeric IPv6 host shall resolve to an AF_INET6 loopback socket address. */
TEST(inet_getaddrinfo, numeric_ipv6)
{
	struct addrinfo hints = { 0 };
	const struct sockaddr_in6 *sin6;
	int ret;

	hints.ai_family = AF_INET6;
	hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV;

	ret = getaddrinfo("::1", SERVICE_STR, &hints, &test_common.res);
	if (ret != 0) {
		TEST_IGNORE_MESSAGE("IPv6 unavailable on this system");
	}

	TEST_ASSERT_NOT_NULL(test_common.res);
	TEST_ASSERT_EQUAL_INT(AF_INET6, test_common.res->ai_family);
	TEST_ASSERT_EQUAL_INT(sizeof(struct sockaddr_in6), test_common.res->ai_addrlen);

	sin6 = (const struct sockaddr_in6 *)test_common.res->ai_addr;
	TEST_ASSERT_EQUAL_INT(AF_INET6, sin6->sin6_family);
	TEST_ASSERT_EQUAL_HEX16(htons(SERVICE_NUM), sin6->sin6_port);
	TEST_ASSERT_EQUAL_MEMORY(&in6addr_loopback, &sin6->sin6_addr, sizeof(struct in6_addr));
}


/*
 * freeaddrinfo shall support freeing an arbitrary sublist of a list returned by
 * getaddrinfo, in addition to the whole list.
 */
TEST(inet_getaddrinfo, freeaddrinfo_frees_sublist)
{
	struct addrinfo hints = { 0 };
	struct addrinfo *head;
	struct addrinfo *tail;

	hints.ai_family = AF_INET;
	hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV;

	TEST_ASSERT_EQUAL_INT(0, getaddrinfo(LOOPBACK_V4, SERVICE_STR, &hints, &head));
	TEST_ASSERT_NOT_NULL(head);

	/* Detach and free the tail sublist, then free the remaining head node. */
	tail = head->ai_next;
	if (tail != NULL) {
		head->ai_next = NULL;
		freeaddrinfo(tail);
	}
	freeaddrinfo(head);

	/* Nothing left for teardown to free. */
	test_common.res = NULL;
}


TEST_GROUP_RUNNER(inet_getaddrinfo)
{
	RUN_TEST_CASE(inet_getaddrinfo, numeric_ipv4);
	RUN_TEST_CASE(inet_getaddrinfo, passive_wildcard);
	RUN_TEST_CASE(inet_getaddrinfo, loopback_default);
	RUN_TEST_CASE(inet_getaddrinfo, null_service_zero_port);
	RUN_TEST_CASE(inet_getaddrinfo, socktype_filter);
	RUN_TEST_CASE(inet_getaddrinfo, canonname_numeric);
	RUN_TEST_CASE(inet_getaddrinfo, numerichost_nonnumeric_noname);
	RUN_TEST_CASE(inet_getaddrinfo, numericserv_nonnumeric_noname);
	RUN_TEST_CASE(inet_getaddrinfo, both_null_noname);
	RUN_TEST_CASE(inet_getaddrinfo, bad_family);
	RUN_TEST_CASE(inet_getaddrinfo, numeric_ipv6);
	RUN_TEST_CASE(inet_getaddrinfo, freeaddrinfo_frees_sublist);
}
