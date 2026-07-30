/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <sys/time.h>
 * TESTED:
 *    - gettimeofday()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Lukasz Kruszynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define _XOPEN_SOURCE 700

#include <sys/time.h>
#include <time.h>
#include <stdint.h>

#include <unity_fixture.h>

#define USEC_PER_SEC    1000000
/* Tolerated skew between gettimeofday() and time(), both read moments apart. */
#define TIME_DELTA_SEC  5


TEST_GROUP(time_gettimeofday);


TEST_SETUP(time_gettimeofday)
{
}


TEST_TEAR_DOWN(time_gettimeofday)
{
}


/*
 * gettimeofday() shall return 0 (no value is reserved to indicate an error) and
 * shall store a microsecond field within its defined [0, 1000000) range.
 */
TEST(time_gettimeofday, returns_zero_and_valid_usec)
{
	struct timeval tv;

	tv.tv_sec = -1;
	tv.tv_usec = -1;

	TEST_ASSERT_EQUAL_INT(0, gettimeofday(&tv, NULL));

	/* Seconds since the Epoch are positive for any present-day clock. */
	TEST_ASSERT_GREATER_THAN_INT(0, (int)tv.tv_sec);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, (int)tv.tv_usec);
	TEST_ASSERT_LESS_THAN_INT(USEC_PER_SEC, (int)tv.tv_usec);
}


/*
 * The seconds field shall express time since the Epoch, so it shall agree with
 * time() to within a small tolerance.
 */
TEST(time_gettimeofday, agrees_with_time)
{
	struct timeval tv;
	time_t now;

	TEST_ASSERT_EQUAL_INT(0, gettimeofday(&tv, NULL));
	now = time(NULL);
	TEST_ASSERT_NOT_EQUAL_INT((time_t)-1, now);

	TEST_ASSERT_INT64_WITHIN((int64_t)TIME_DELTA_SEC, (int64_t)now, (int64_t)tv.tv_sec);
}


/* Successive readings of the wall clock shall not move backwards. */
TEST(time_gettimeofday, nondecreasing)
{
	struct timeval first;
	struct timeval second;

	TEST_ASSERT_EQUAL_INT(0, gettimeofday(&first, NULL));
	TEST_ASSERT_EQUAL_INT(0, gettimeofday(&second, NULL));

	TEST_ASSERT_TRUE_MESSAGE(
		(second.tv_sec > first.tv_sec) ||
			((second.tv_sec == first.tv_sec) && (second.tv_usec >= first.tv_usec)),
		"wall clock moved backwards between calls");
}


TEST_GROUP_RUNNER(time_gettimeofday)
{
	RUN_TEST_CASE(time_gettimeofday, returns_zero_and_valid_usec);
	RUN_TEST_CASE(time_gettimeofday, agrees_with_time);
	RUN_TEST_CASE(time_gettimeofday, nondecreasing);
}
