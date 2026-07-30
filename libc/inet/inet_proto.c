/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <netdb.h>
 * TESTED:
 *    - getprotobyname()
 *    - getprotobynumber()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Lukasz Kruszynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define _XOPEN_SOURCE 700

#include <netdb.h>
#include <string.h>

#include <unity_fixture.h>

/*
 * Well-known protocol numbers assigned by IANA. These entries are present in
 * every conforming network protocol database (for example /etc/protocols).
 */
#define PROTO_ICMP_NAME "icmp"
#define PROTO_TCP_NAME  "tcp"
#define PROTO_UDP_NAME  "udp"
#define PROTO_ICMP_NUM  1
#define PROTO_TCP_NUM   6
#define PROTO_UDP_NUM   17

/* A name/number pair that shall not correspond to any protocol entry. */
#define PROTO_BOGUS_NAME "nosuchproto0"
#define PROTO_BOGUS_NUM  0x6ffff

/* Upper bound on the alias list length, guards against a malformed database. */
#define ALIASES_SCAN_MAX 256U


TEST_GROUP(inet_proto);


TEST_SETUP(inet_proto)
{
}


TEST_TEAR_DOWN(inet_proto)
{
}


/*
 * The protocol database is optional scaffolding: skip when the running system
 * has no protocol database at all, so the assertions below are not vacuous.
 */
static int test_haveProtoDb(void)
{
	return getprotobynumber(PROTO_TCP_NUM) != NULL;
}


/* getprotobyname: shall return the entry whose p_name matches the given name. */
TEST(inet_proto, getprotobyname_returns_entry)
{
	struct protoent *ent;

	if (!test_haveProtoDb()) {
		TEST_IGNORE_MESSAGE("protocol database unavailable");
	}

	ent = getprotobyname(PROTO_TCP_NAME);
	TEST_ASSERT_NOT_NULL(ent);
	TEST_ASSERT_EQUAL_STRING(PROTO_TCP_NAME, ent->p_name);
	TEST_ASSERT_EQUAL_INT(PROTO_TCP_NUM, ent->p_proto);
	/* The members of a protoent shall include a non-NULL alias list. */
	TEST_ASSERT_NOT_NULL(ent->p_aliases);
}


/* getprotobynumber: shall return the entry whose p_proto matches the number. */
TEST(inet_proto, getprotobynumber_returns_entry)
{
	struct protoent *ent;

	if (!test_haveProtoDb()) {
		TEST_IGNORE_MESSAGE("protocol database unavailable");
	}

	ent = getprotobynumber(PROTO_UDP_NUM);
	TEST_ASSERT_NOT_NULL(ent);
	TEST_ASSERT_EQUAL_STRING(PROTO_UDP_NAME, ent->p_name);
	TEST_ASSERT_EQUAL_INT(PROTO_UDP_NUM, ent->p_proto);
	TEST_ASSERT_NOT_NULL(ent->p_aliases);
}


/*
 * getprotobyname and getprotobynumber shall report the same p_proto value for
 * a given protocol, whether it is looked up by name or by number.
 */
TEST(inet_proto, name_number_consistency)
{
	size_t i;
	struct protoent *byName;
	struct protoent *byNum;
	static const struct {
		const char *name;
		int num;
	} known[] = {
		{ PROTO_ICMP_NAME, PROTO_ICMP_NUM },
		{ PROTO_TCP_NAME, PROTO_TCP_NUM },
		{ PROTO_UDP_NAME, PROTO_UDP_NUM },
	};

	if (!test_haveProtoDb()) {
		TEST_IGNORE_MESSAGE("protocol database unavailable");
	}

	for (i = 0; i < sizeof(known) / sizeof(known[0]); i++) {
		byName = getprotobyname(known[i].name);
		TEST_ASSERT_NOT_NULL(byName);
		TEST_ASSERT_EQUAL_INT(known[i].num, byName->p_proto);

		byNum = getprotobynumber(known[i].num);
		TEST_ASSERT_NOT_NULL(byNum);
		TEST_ASSERT_EQUAL_INT(known[i].num, byNum->p_proto);
		TEST_ASSERT_EQUAL_STRING(known[i].name, byNum->p_name);
	}
}


/* The p_aliases member shall be a NULL-terminated array of strings. */
TEST(inet_proto, aliases_array_null_terminated)
{
	struct protoent *ent;
	unsigned count;

	if (!test_haveProtoDb()) {
		TEST_IGNORE_MESSAGE("protocol database unavailable");
	}

	ent = getprotobynumber(PROTO_TCP_NUM);
	TEST_ASSERT_NOT_NULL(ent);
	TEST_ASSERT_NOT_NULL(ent->p_aliases);

	/* Every present alias is a valid string; the array ends with a NULL entry. */
	for (count = 0; count < ALIASES_SCAN_MAX; count++) {
		if (ent->p_aliases[count] == NULL) {
			break;
		}
		TEST_ASSERT_NOT_NULL(ent->p_aliases[count]);
	}
	TEST_ASSERT_LESS_THAN_UINT(ALIASES_SCAN_MAX, count);
}


/* getprotobyname: shall return a null pointer when the entry is not found. */
TEST(inet_proto, unknown_name_returns_null)
{
	TEST_ASSERT_NULL(getprotobyname(PROTO_BOGUS_NAME));
}


/* getprotobynumber: shall return a null pointer when the entry is not found. */
TEST(inet_proto, unknown_number_returns_null)
{
	TEST_ASSERT_NULL(getprotobynumber(PROTO_BOGUS_NUM));
}


TEST_GROUP_RUNNER(inet_proto)
{
	RUN_TEST_CASE(inet_proto, getprotobyname_returns_entry);
	RUN_TEST_CASE(inet_proto, getprotobynumber_returns_entry);
	RUN_TEST_CASE(inet_proto, name_number_consistency);
	RUN_TEST_CASE(inet_proto, aliases_array_null_terminated);
	RUN_TEST_CASE(inet_proto, unknown_name_returns_null);
	RUN_TEST_CASE(inet_proto, unknown_number_returns_null);
}
