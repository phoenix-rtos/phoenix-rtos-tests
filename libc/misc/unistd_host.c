/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <unistd.h>
 * TESTED:
 *    - gethostid()
 *    - gethostname()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <errno.h>

#include "unity_fixture.h"

/* HOST_NAME_MAX may not be defined on all systems; POSIX guarantees at least 255 */
#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 255
#endif

#define HOSTNAME_BUF_SIZE (HOST_NAME_MAX + 1)


/*
 * Tests: gethostid
 */

TEST_GROUP(unistd_gethostid);


TEST_SETUP(unistd_gethostid)
{
}


TEST_TEAR_DOWN(unistd_gethostid)
{
}


TEST(unistd_gethostid, gethostid_returns_value)
{
	/* "gethostid() shall retrieve a 32-bit identifier for the current host" */
	/* No errors are defined — function always succeeds */
	long id = gethostid();
	/* The value is unspecified but the call must succeed. Verify it fits in 32 bits. */
	TEST_ASSERT_EQUAL_INT(id, (long)(int32_t)id);
}


TEST(unistd_gethostid, gethostid_consistent)
{
	/* Two consecutive calls shall return the same identifier */
	long id1 = gethostid();
	long id2 = gethostid();
	TEST_ASSERT_EQUAL_INT(id1, id2);
}


TEST_GROUP_RUNNER(unistd_gethostid)
{
	RUN_TEST_CASE(unistd_gethostid, gethostid_returns_value);
	RUN_TEST_CASE(unistd_gethostid, gethostid_consistent);
}


/*
 * Tests: gethostname
 */

TEST_GROUP(unistd_gethostname);


TEST_SETUP(unistd_gethostname)
{
}


TEST_TEAR_DOWN(unistd_gethostname)
{
}


TEST(unistd_gethostname, gethostname_success)
{
	TEST_IGNORE_MESSAGE("ISSUE TO BE REPORTED");
	/* "gethostname() shall return the standard host name for the current machine" */
	char buf[HOSTNAME_BUF_SIZE];
	int ret;

	memset(buf, 0xff, sizeof(buf));
	ret = gethostname(buf, sizeof(buf));
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* returned name shall be null-terminated when buffer is sufficient */
	TEST_ASSERT_NOT_NULL(memchr(buf, '\0', sizeof(buf)));

	/* name must not be empty */
	TEST_ASSERT_GREATER_THAN_INT(0, (int)strlen(buf));
}


TEST(unistd_gethostname, gethostname_consistent)
{
	/* Two calls return the same hostname */
	char buf1[HOSTNAME_BUF_SIZE];
	char buf2[HOSTNAME_BUF_SIZE];

	TEST_ASSERT_EQUAL_INT(0, gethostname(buf1, sizeof(buf1)));
	TEST_ASSERT_EQUAL_INT(0, gethostname(buf2, sizeof(buf2)));
	TEST_ASSERT_EQUAL_STRING(buf1, buf2);
}


TEST(unistd_gethostname, gethostname_max_length)
{
	/* "Host names are limited to {HOST_NAME_MAX} bytes" */
	char buf[HOSTNAME_BUF_SIZE];

	TEST_ASSERT_EQUAL_INT(0, gethostname(buf, sizeof(buf)));
	TEST_ASSERT_TRUE(strlen(buf) <= (size_t)HOST_NAME_MAX);
}


TEST(unistd_gethostname, gethostname_truncation)
{
	/* "if namelen is an insufficient length to hold the host name,
	 *  then the returned name shall be truncated" */
	char full[HOSTNAME_BUF_SIZE];
	char small[2];
	int ret;

	TEST_ASSERT_EQUAL_INT(0, gethostname(full, sizeof(full)));

	/* If hostname is at least 2 chars, a 2-byte buffer triggers truncation */
	if (strlen(full) >= 2) {
		memset(small, 0xff, sizeof(small));
		ret = gethostname(small, sizeof(small));
#ifndef __phoenix__
		/* glibc returns -1/ENAMETOOLONG instead of truncating */
		if (ret == -1 && errno == ENAMETOOLONG) {
			TEST_IGNORE_MESSAGE("host-pc bug: glibc returns ENAMETOOLONG instead of truncating");
		}
#endif
		/* Return value is 0 on success (truncation is not an error per spec) */
		TEST_ASSERT_EQUAL_INT(0, ret);
		/* First byte must match */
		TEST_ASSERT_EQUAL_CHAR(full[0], small[0]);
	}
	else {
		TEST_IGNORE_MESSAGE("hostname too short to test truncation");
	}
}


TEST(unistd_gethostname, gethostname_exact_length)
{
	/* Buffer exactly large enough for hostname + NUL */
	char full[HOSTNAME_BUF_SIZE];
	size_t len;
	int ret;

	TEST_ASSERT_EQUAL_INT(0, gethostname(full, sizeof(full)));
	len = strlen(full);

	if (len > 0) {
		static char exact[HOSTNAME_BUF_SIZE];
		memset(exact, 0xff, sizeof(exact));
		ret = gethostname(exact, len + 1);
		TEST_ASSERT_EQUAL_INT(0, ret);
		TEST_ASSERT_EQUAL_STRING(full, exact);
	}
}


TEST_GROUP_RUNNER(unistd_gethostname)
{
	RUN_TEST_CASE(unistd_gethostname, gethostname_success);
	RUN_TEST_CASE(unistd_gethostname, gethostname_consistent);
	RUN_TEST_CASE(unistd_gethostname, gethostname_max_length);
	RUN_TEST_CASE(unistd_gethostname, gethostname_truncation);
	RUN_TEST_CASE(unistd_gethostname, gethostname_exact_length);
}
