/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - arpa/inet.h
 * TESTED:
 *    - inet_addr()
 *    - inet_ntoa()
 *    - inet_pton()
 *    - inet_ntop()
 *    - ntohs()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Lukasz Kruszynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <unity_fixture.h>

/* 127.0.0.1 in host byte order and its canonical dotted-decimal text. */
#define LOOPBACK_HOST 0x7f000001UL
#define LOOPBACK_STR  "127.0.0.1"

#define ADDR_ANY_STR   "0.0.0.0"
#define ADDR_BCAST_STR "255.255.255.255"

/* Value inet_addr() returns for a malformed string. */
#define INET_INVALID_RET ((in_addr_t)(-1))

#define IPV6_LOOPBACK_STR "::1"
#define IPV6_ANY_STR      "::"

/* Length of the binary IPv6 address forms used as expected values. */
#define IPV6_ADDR_LEN 16U

/* A 16-bit test value and its network-order (big-endian) byte layout. */
#define BYTEORDER_HOST_VALUE 0x1234U


static const uint8_t ipv6Loopback[IPV6_ADDR_LEN] = {
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 1
};

static const uint8_t ipv6Any[IPV6_ADDR_LEN] = { 0 };


TEST_GROUP(inet_addr);
TEST_GROUP(inet_ntoa);
TEST_GROUP(inet_pton);
TEST_GROUP(inet_ntop);
TEST_GROUP(inet_byteorder);


TEST_SETUP(inet_addr)
{
}


TEST_TEAR_DOWN(inet_addr)
{
}


TEST(inet_addr, addr_converts_dotted_decimal)
{
	/* Four-part dotted decimal converts to a network-order address. */
	TEST_ASSERT_EQUAL_UINT32(htonl(LOOPBACK_HOST), inet_addr(LOOPBACK_STR));
	TEST_ASSERT_EQUAL_UINT32(0U, inet_addr(ADDR_ANY_STR));
	TEST_ASSERT_EQUAL_UINT32(0xffffffffUL, inet_addr(ADDR_BCAST_STR));
}


TEST(inet_addr, addr_accepts_alternate_forms)
{
	const in_addr_t canonical = inet_addr(LOOPBACK_STR);

	/* Three-part, two-part and one-part forms resolve to the same address. */
	TEST_ASSERT_EQUAL_UINT32(canonical, inet_addr("127.0.1"));
	TEST_ASSERT_EQUAL_UINT32(canonical, inet_addr("127.1"));
	TEST_ASSERT_EQUAL_UINT32(canonical, inet_addr("2130706433"));

	/* Parts may be given in hexadecimal (0x) or octal (leading 0). */
	TEST_ASSERT_EQUAL_UINT32(canonical, inet_addr("0x7f.0.0.1"));
	TEST_ASSERT_EQUAL_UINT32(canonical, inet_addr("0177.0.0.1"));
}


TEST(inet_addr, addr_rejects_invalid)
{
	/* Malformed strings shall return (in_addr_t)(-1). */
	TEST_ASSERT_EQUAL_UINT32(INET_INVALID_RET, inet_addr("not-an-address"));
	TEST_ASSERT_EQUAL_UINT32(INET_INVALID_RET, inet_addr("256.0.0.1"));
	TEST_ASSERT_EQUAL_UINT32(INET_INVALID_RET, inet_addr("1.2.3.4.5"));
}


TEST_SETUP(inet_ntoa)
{
}


TEST_TEAR_DOWN(inet_ntoa)
{
}


TEST(inet_ntoa, ntoa_converts_to_dotted_decimal)
{
	struct in_addr in;

	/* A network-order address is rendered in Internet standard dot notation. */
	in.s_addr = htonl(LOOPBACK_HOST);
	TEST_ASSERT_EQUAL_STRING(LOOPBACK_STR, inet_ntoa(in));

	in.s_addr = htonl(0UL);
	TEST_ASSERT_EQUAL_STRING(ADDR_ANY_STR, inet_ntoa(in));

	in.s_addr = htonl(0xffffffffUL);
	TEST_ASSERT_EQUAL_STRING(ADDR_BCAST_STR, inet_ntoa(in));
}


TEST_SETUP(inet_pton)
{
}


TEST_TEAR_DOWN(inet_pton)
{
}


TEST(inet_pton, pton_ipv4_valid)
{
	struct in_addr addr;

	addr.s_addr = 0U;

	/* A valid dotted-decimal string converts, storing network byte order. */
	TEST_ASSERT_EQUAL_INT(1, inet_pton(AF_INET, LOOPBACK_STR, &addr));
	TEST_ASSERT_EQUAL_UINT32(htonl(LOOPBACK_HOST), addr.s_addr);
}


TEST(inet_pton, pton_ipv4_rejects_nonstandard)
{
	struct in_addr addr;

	/* Octal, hexadecimal and short forms accepted by inet_addr() are refused. */
	TEST_ASSERT_EQUAL_INT(0, inet_pton(AF_INET, "0x7f.0.0.1", &addr));
	TEST_ASSERT_EQUAL_INT(0, inet_pton(AF_INET, "127.1", &addr));
	TEST_ASSERT_EQUAL_INT(0, inet_pton(AF_INET, "256.0.0.1", &addr));
}


TEST(inet_pton, pton_ipv6_valid)
{
	struct in6_addr addr;

	TEST_ASSERT_EQUAL_INT(1, inet_pton(AF_INET6, IPV6_LOOPBACK_STR, &addr));
	TEST_ASSERT_EQUAL_MEMORY(ipv6Loopback, &addr, sizeof(ipv6Loopback));

	TEST_ASSERT_EQUAL_INT(1, inet_pton(AF_INET6, IPV6_ANY_STR, &addr));
	TEST_ASSERT_EQUAL_MEMORY(ipv6Any, &addr, sizeof(ipv6Any));
}


TEST(inet_pton, pton_ipv6_rejects_invalid)
{
	struct in6_addr addr;

	TEST_ASSERT_EQUAL_INT(0, inet_pton(AF_INET6, "1::2::3", &addr)); /* two "::" */
	TEST_ASSERT_EQUAL_INT(0, inet_pton(AF_INET6, "::g", &addr));     /* bad digit */
}


TEST(inet_pton, pton_unknown_family)
{
	struct in_addr addr;

	/* An unknown address family shall fail with -1 and errno EAFNOSUPPORT. */
	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, inet_pton(AF_UNIX, LOOPBACK_STR, &addr));
	TEST_ASSERT_EQUAL_INT(EAFNOSUPPORT, errno);
}


TEST_SETUP(inet_ntop)
{
}


TEST_TEAR_DOWN(inet_ntop)
{
}


TEST(inet_ntop, ntop_ipv4)
{
	struct in_addr addr;
	char buf[INET_ADDRSTRLEN];
	const char *ret;

	addr.s_addr = htonl(LOOPBACK_HOST);

	/* On success the return value is the destination buffer holding the text. */
	ret = inet_ntop(AF_INET, &addr, buf, sizeof(buf));
	TEST_ASSERT_EQUAL_PTR(buf, ret);
	TEST_ASSERT_EQUAL_STRING(LOOPBACK_STR, buf);
}


TEST(inet_ntop, ntop_ipv6)
{
	char buf[INET6_ADDRSTRLEN];
	const char *ret;

	ret = inet_ntop(AF_INET6, ipv6Loopback, buf, sizeof(buf));
	TEST_ASSERT_EQUAL_PTR(buf, ret);
	TEST_ASSERT_EQUAL_STRING(IPV6_LOOPBACK_STR, buf);
}


TEST(inet_ntop, ntop_buffer_too_small)
{
	struct in_addr addr;
	char buf[INET_ADDRSTRLEN];

	addr.s_addr = htonl(LOOPBACK_HOST);

	/* A result buffer too small shall fail with NULL and errno ENOSPC. */
	errno = 0;
	TEST_ASSERT_NULL(inet_ntop(AF_INET, &addr, buf, 1U));
	TEST_ASSERT_EQUAL_INT(ENOSPC, errno);
}


TEST(inet_ntop, ntop_unknown_family)
{
	struct in_addr addr;
	char buf[INET_ADDRSTRLEN];

	addr.s_addr = htonl(LOOPBACK_HOST);

	/* An unknown address family shall fail with NULL and errno EAFNOSUPPORT. */
	errno = 0;
	TEST_ASSERT_NULL(inet_ntop(AF_UNIX, &addr, buf, sizeof(buf)));
	TEST_ASSERT_EQUAL_INT(EAFNOSUPPORT, errno);
}


TEST_SETUP(inet_byteorder)
{
}


TEST_TEAR_DOWN(inet_byteorder)
{
}


/* Tests: ntohs, htons */
TEST(inet_byteorder, byteorder_ntohs_htons)
{
	const uint8_t netbytes[2] = { 0x12, 0x34 };
	uint16_t netval;

	memcpy(&netval, netbytes, sizeof(netval));

	/* Network order is big-endian, so the raw value denotes 0x1234 in host order. */
	TEST_ASSERT_EQUAL_UINT16(BYTEORDER_HOST_VALUE, ntohs(netval));
	TEST_ASSERT_EQUAL_UINT16(netval, htons(BYTEORDER_HOST_VALUE));

	/* ntohs and htons are mutual inverses across the 16-bit range. */
	TEST_ASSERT_EQUAL_UINT16(0xabcdU, ntohs(htons(0xabcdU)));
	TEST_ASSERT_EQUAL_UINT16(0U, ntohs(0U));
	TEST_ASSERT_EQUAL_UINT16(0xffffU, ntohs(0xffffU));
}


TEST_GROUP_RUNNER(inet_addr)
{
	RUN_TEST_CASE(inet_addr, addr_converts_dotted_decimal);
	RUN_TEST_CASE(inet_addr, addr_accepts_alternate_forms);
	RUN_TEST_CASE(inet_addr, addr_rejects_invalid);
}


TEST_GROUP_RUNNER(inet_ntoa)
{
	RUN_TEST_CASE(inet_ntoa, ntoa_converts_to_dotted_decimal);
}


TEST_GROUP_RUNNER(inet_pton)
{
	RUN_TEST_CASE(inet_pton, pton_ipv4_valid);
	RUN_TEST_CASE(inet_pton, pton_ipv4_rejects_nonstandard);
	RUN_TEST_CASE(inet_pton, pton_ipv6_valid);
	RUN_TEST_CASE(inet_pton, pton_ipv6_rejects_invalid);
	RUN_TEST_CASE(inet_pton, pton_unknown_family);
}


TEST_GROUP_RUNNER(inet_ntop)
{
	RUN_TEST_CASE(inet_ntop, ntop_ipv4);
	RUN_TEST_CASE(inet_ntop, ntop_ipv6);
	RUN_TEST_CASE(inet_ntop, ntop_buffer_too_small);
	RUN_TEST_CASE(inet_ntop, ntop_unknown_family);
}


TEST_GROUP_RUNNER(inet_byteorder)
{
	RUN_TEST_CASE(inet_byteorder, byteorder_ntohs_htons);
}
