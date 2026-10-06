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


TEST_GROUP(unistd_gethostname);


TEST_SETUP(unistd_gethostname)
{
}


TEST_TEAR_DOWN(unistd_gethostname)
{
}


TEST(unistd_gethostname, gethostname_max_length)
{
	/* "Host names are limited to {HOST_NAME_MAX} bytes" */
	char buf[HOSTNAME_BUF_SIZE];

	TEST_ASSERT_EQUAL_INT(0, gethostname(buf, sizeof(buf)));
	TEST_ASSERT_TRUE(strlen(buf) <= (size_t)HOST_NAME_MAX);
}


TEST_GROUP_RUNNER(unistd_gethostname)
{
	RUN_TEST_CASE(unistd_gethostname, gethostname_max_length);
}
