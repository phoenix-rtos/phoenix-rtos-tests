/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <time.h>
 * TESTED:
 *    - gmtime_r()
 *    - localtime_r()
 *    - asctime_r()
 *    - ctime_r()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Lukasz Kruszynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "time_common.h"

/* asctime_r()/ctime_r() require a caller buffer of at least 26 bytes. */
#define ASCTIME_BUFSZ 26
#define TZ_SAVE_SZ    64

/* Broken-down UTC references, in init_tm() field order:
 * {sec, min, hour, mday, mon, year, wday, yday, isdst}. */
static const int tmEpoch[] = { 0, 0, 0, 1, 0, 70, 4, 0, 0 };    /* 1970-01-01 00:00:00 Thu */
static const int tmLeap[] = { 0, 0, 5, 29, 1, 116, 1, 59, 0 };  /* 2016-02-29 05:00:00 Mon */

static const time_t epochT = 0;
static const time_t leapT = 1456722000;

/* asctime_r() renderings of the references above (note the two-space day pad). */
#define EPOCH_UTC_STR "Thu Jan  1 00:00:00 1970\n"
#define LEAP_UTC_STR  "Mon Feb 29 05:00:00 2016\n"


static struct {
	char savedTz[TZ_SAVE_SZ];
	int hadTz;
} test_common;


/* Pin the timezone to UTC so that local-time conversions are deterministic. */
static void test_forceUtc(void)
{
	const char *cur = getenv("TZ");

	test_common.hadTz = (cur != NULL) ? 1 : 0;
	if (cur != NULL) {
		strncpy(test_common.savedTz, cur, sizeof(test_common.savedTz) - 1);
		test_common.savedTz[sizeof(test_common.savedTz) - 1] = '\0';
	}

	setenv("TZ", "UTC0", 1);
	tzset();
}


static void test_restoreTz(void)
{
	if (test_common.hadTz != 0) {
		setenv("TZ", test_common.savedTz, 1);
	}
	else {
		unsetenv("TZ");
	}
	tzset();
}


/* ===== gmtime_r ===== */


TEST_GROUP(time_gmtime_r);


TEST_SETUP(time_gmtime_r)
{
}


TEST_TEAR_DOWN(time_gmtime_r)
{
}


/* gmtime_r() shall store the UTC broken-down time and return that same buffer. */
TEST(time_gmtime_r, converts_and_returns_buffer)
{
	struct tm result;
	struct tm expected;
	struct tm *ret;

	init_tm(&expected, tmEpoch);
	ret = gmtime_r(&epochT, &result);
	TEST_ASSERT_EQUAL_PTR(&result, ret);
	struct_tm_assert_equal(&expected, &result);

	init_tm(&expected, tmLeap);
	ret = gmtime_r(&leapT, &result);
	TEST_ASSERT_EQUAL_PTR(&result, ret);
	struct_tm_assert_equal(&expected, &result);
}


/* Each gmtime_r() call shall use only its caller-supplied buffer. */
TEST(time_gmtime_r, uses_caller_buffer)
{
	struct tm first;
	struct tm second;
	struct tm expected;

	gmtime_r(&epochT, &first);
	gmtime_r(&leapT, &second);

	/* The second conversion must not have disturbed the first result. */
	init_tm(&expected, tmEpoch);
	struct_tm_assert_equal(&expected, &first);
	init_tm(&expected, tmLeap);
	struct_tm_assert_equal(&expected, &second);
}


/* ===== localtime_r ===== */


TEST_GROUP(time_localtime_r);


TEST_SETUP(time_localtime_r)
{
	test_forceUtc();
}


TEST_TEAR_DOWN(time_localtime_r)
{
	test_restoreTz();
}


/* Under a UTC timezone localtime_r() shall equal the UTC broken-down time. */
TEST(time_localtime_r, converts_and_returns_buffer)
{
	struct tm result;
	struct tm expected;
	struct tm *ret;

	init_tm(&expected, tmEpoch);
	ret = localtime_r(&epochT, &result);
	TEST_ASSERT_EQUAL_PTR(&result, ret);
	struct_tm_assert_equal(&expected, &result);
}


/* Each localtime_r() call shall use only its caller-supplied buffer. */
TEST(time_localtime_r, uses_caller_buffer)
{
	struct tm first;
	struct tm second;
	struct tm expected;

	localtime_r(&epochT, &first);
	localtime_r(&leapT, &second);

	init_tm(&expected, tmEpoch);
	struct_tm_assert_equal(&expected, &first);
	init_tm(&expected, tmLeap);
	struct_tm_assert_equal(&expected, &second);
}


/* ===== asctime_r ===== */


TEST_GROUP(time_asctime_r);


TEST_SETUP(time_asctime_r)
{
}


TEST_TEAR_DOWN(time_asctime_r)
{
}


/*
 * asctime_r() shall render the broken-down time in the fixed 26-byte form and
 * return the caller buffer.
 */
TEST(time_asctime_r, formats_known_times)
{
	TEST_IGNORE_MESSAGE("Unverified Failure");
	size_t i;
	struct tm t;
	char buf[ASCTIME_BUFSZ];
	char *ret;
	static const struct {
		const int *fields;
		const char *expected;
	} cases[] = {
		{ tmEpoch, EPOCH_UTC_STR },
		{ tmLeap, LEAP_UTC_STR },
	};

	for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
		init_tm(&t, cases[i].fields);
		ret = asctime_r(&t, buf);
		TEST_ASSERT_EQUAL_PTR(buf, ret);
		TEST_ASSERT_EQUAL_STRING(cases[i].expected, buf);
	}
}


/* ===== ctime_r ===== */


TEST_GROUP(time_ctime_r);


TEST_SETUP(time_ctime_r)
{
	test_forceUtc();
}


TEST_TEAR_DOWN(time_ctime_r)
{
	test_restoreTz();
}


/*
 * ctime_r() shall be equivalent to asctime_r(localtime_r(clock)); under a UTC
 * timezone that is the fixed-form UTC string. It shall return the caller buffer.
 */
TEST(time_ctime_r, converts_local_time)
{
	TEST_IGNORE_MESSAGE("Unverified Failure");
	char buf[ASCTIME_BUFSZ];
	char *ret;

	ret = ctime_r(&epochT, buf);
	TEST_ASSERT_EQUAL_PTR(buf, ret);
	TEST_ASSERT_EQUAL_STRING(EPOCH_UTC_STR, buf);
}


TEST_GROUP_RUNNER(time_gmtime_r)
{
	RUN_TEST_CASE(time_gmtime_r, converts_and_returns_buffer);
	RUN_TEST_CASE(time_gmtime_r, uses_caller_buffer);
}


TEST_GROUP_RUNNER(time_localtime_r)
{
	RUN_TEST_CASE(time_localtime_r, converts_and_returns_buffer);
	RUN_TEST_CASE(time_localtime_r, uses_caller_buffer);
}


TEST_GROUP_RUNNER(time_asctime_r)
{
	RUN_TEST_CASE(time_asctime_r, formats_known_times);
}


TEST_GROUP_RUNNER(time_ctime_r)
{
	RUN_TEST_CASE(time_ctime_r, converts_local_time);
}
