/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - net/if.h
 *    - netdb.h
 * TESTED:
 *    - if_nametoindex()
 *    - if_indextoname()
 *    - gai_strerror()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Lukasz Kruszynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <sys/socket.h>
#include <net/if.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>

#include <unity_fixture.h>

/* Highest interface index probed while searching for a real interface. */
#define IF_SCAN_MAX 64U

/* An index/name pair that shall not correspond to any interface. */
#define IF_BOGUS_INDEX 0x7fffffffU
#define IF_BOGUS_NAME  "nosuchif0"

/* A value that is not one of the EAI_* codes defined in <netdb.h>. */
#define GAI_BOGUS_CODE 0x7fffffff


TEST_GROUP(inet_if);
TEST_GROUP(inet_gai);


TEST_SETUP(inet_if)
{
}


TEST_TEAR_DOWN(inet_if)
{
}


/* Tests: if_nametoindex, if_indextoname */
TEST(inet_if, if_name_index_roundtrip)
{
	unsigned idx;
	unsigned found = 0U;
	char name[IF_NAMESIZE];
	char *ret = NULL;

	/* Discover a real interface using only the functions under test. */
	for (idx = 1U; idx <= IF_SCAN_MAX; idx++) {
		ret = if_indextoname(idx, name);
		if (ret != NULL) {
			found = idx;
			break;
		}
	}

	if (found == 0U) {
		TEST_IGNORE_MESSAGE("no network interface available to test");
	}
	else {
		/* if_indextoname() returns the ifname argument it was given. */
		TEST_ASSERT_EQUAL_PTR(name, ret);
		/* The reverse mapping shall yield the original index. */
		TEST_ASSERT_EQUAL_UINT(found, if_nametoindex(name));
	}
}


TEST(inet_if, if_nametoindex_unknown_is_zero)
{
	/* A name that is not an interface shall map to index zero. */
	TEST_ASSERT_EQUAL_UINT(0U, if_nametoindex(IF_BOGUS_NAME));
}


TEST(inet_if, if_indextoname_unknown_fails)
{
	char name[IF_NAMESIZE];

	/* A non-existent interface index shall fail with NULL and errno ENXIO. */
	errno = 0;
	TEST_ASSERT_NULL(if_indextoname(IF_BOGUS_INDEX, name));
	TEST_ASSERT_EQUAL_INT(ENXIO, errno);
}


TEST_SETUP(inet_gai)
{
}


TEST_TEAR_DOWN(inet_gai)
{
}


TEST(inet_gai, gai_strerror_known_codes)
{
	size_t i;
	const char *msg;
	static const int codes[] = {
		EAI_AGAIN, EAI_BADFLAGS, EAI_FAIL, EAI_FAMILY, EAI_MEMORY,
		EAI_NONAME, EAI_OVERFLOW, EAI_SERVICE, EAI_SOCKTYPE, EAI_SYSTEM
	};

	/* Every documented error code maps to a non-empty description string. */
	for (i = 0; i < sizeof(codes) / sizeof(codes[0]); i++) {
		msg = gai_strerror(codes[i]);
		TEST_ASSERT_NOT_NULL(msg);
		TEST_ASSERT_TRUE(msg[0] != '\0');
	}
}


TEST(inet_gai, gai_strerror_unknown_code)
{
	/* An unrecognised value shall still yield a non-empty descriptive string. */
	const char *msg = gai_strerror(GAI_BOGUS_CODE);

	TEST_ASSERT_NOT_NULL(msg);
	TEST_ASSERT_TRUE(msg[0] != '\0');
}


TEST_GROUP_RUNNER(inet_if)
{
	RUN_TEST_CASE(inet_if, if_name_index_roundtrip);
	RUN_TEST_CASE(inet_if, if_nametoindex_unknown_is_zero);
	RUN_TEST_CASE(inet_if, if_indextoname_unknown_fails);
}


TEST_GROUP_RUNNER(inet_gai)
{
	RUN_TEST_CASE(inet_gai, gai_strerror_known_codes);
	RUN_TEST_CASE(inet_gai, gai_strerror_unknown_code);
}
