/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <time.h>
 * TESTED:
 *    - clock_gettime()
 *    - clock_settime()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Damian LoewnauLoewnauLoewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <time.h>
#include <errno.h>
#include <unistd.h>

#include "unity_fixture.h"

#define NSEC_PER_SEC 1000000000L


/*
 * Tests: clock_gettime
 */

TEST_GROUP(time_clock_gettime);


TEST_SETUP(time_clock_gettime)
{
}


TEST_TEAR_DOWN(time_clock_gettime)
{
}


TEST(time_clock_gettime, clock_gettime_realtime_success)
{
	/* "clock_gettime() shall return the current value tp for the specified clock" */
	struct timespec tp;
	int ret;

	errno = 0;
	ret = clock_gettime(CLOCK_REALTIME, &tp);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, errno);

	/* tv_sec must be positive (after epoch) */
	TEST_ASSERT_GREATER_THAN_INT(0, (int)tp.tv_sec);

	/* tv_nsec must be in [0, 999999999] */
	TEST_ASSERT_TRUE(tp.tv_nsec >= 0);
	TEST_ASSERT_TRUE(tp.tv_nsec < NSEC_PER_SEC);
}


TEST(time_clock_gettime, clock_gettime_monotonic_success)
{
	/* CLOCK_MONOTONIC must be supported */
	struct timespec tp;
	int ret;

	errno = 0;
	ret = clock_gettime(CLOCK_MONOTONIC, &tp);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, errno);

	/* tv_nsec must be in valid range */
	TEST_ASSERT_TRUE(tp.tv_nsec >= 0);
	TEST_ASSERT_TRUE(tp.tv_nsec < NSEC_PER_SEC);
}


TEST(time_clock_gettime, clock_gettime_monotonic_nondecreasing)
{
	/* CLOCK_MONOTONIC shall never go backwards */
	struct timespec tp1, tp2;
	int ret;

	ret = clock_gettime(CLOCK_MONOTONIC, &tp1);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = clock_gettime(CLOCK_MONOTONIC, &tp2);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* tp2 >= tp1 */
	TEST_ASSERT_TRUE(
		(tp2.tv_sec > tp1.tv_sec) ||
		((tp2.tv_sec == tp1.tv_sec) && (tp2.tv_nsec >= tp1.tv_nsec)));
}


TEST(time_clock_gettime, clock_gettime_realtime_advances)
{
	/* CLOCK_REALTIME shall advance over time */
	struct timespec tp1, tp2;
	struct timespec delay = { 0, 10000000L }; /* 10ms */
	int ret;

	ret = clock_gettime(CLOCK_REALTIME, &tp1);
	TEST_ASSERT_EQUAL_INT(0, ret);

	nanosleep(&delay, NULL);

	ret = clock_gettime(CLOCK_REALTIME, &tp2);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* tp2 must be strictly greater than tp1 */
	TEST_ASSERT_TRUE(
		(tp2.tv_sec > tp1.tv_sec) ||
		((tp2.tv_sec == tp1.tv_sec) && (tp2.tv_nsec > tp1.tv_nsec)));
}


TEST(time_clock_gettime, clock_gettime_einval_invalid_clock)
{
	/* "EINVAL: The clock_id argument does not specify a known clock." */
	struct timespec tp;
	int ret;

	errno = 0;
	ret = clock_gettime((clockid_t)-99, &tp);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST(time_clock_gettime, clock_gettime_nsec_range)
{
	/* tv_nsec shall always be in [0, 999999999] for any valid clock */
	struct timespec tp;
	int ret;
	int i;

	for (i = 0; i < 100; i++) {
		ret = clock_gettime(CLOCK_REALTIME, &tp);
		TEST_ASSERT_EQUAL_INT(0, ret);
		TEST_ASSERT_TRUE(tp.tv_nsec >= 0);
		TEST_ASSERT_TRUE(tp.tv_nsec < NSEC_PER_SEC);
	}
}


TEST_GROUP_RUNNER(time_clock_gettime)
{
	RUN_TEST_CASE(time_clock_gettime, clock_gettime_realtime_success);
	RUN_TEST_CASE(time_clock_gettime, clock_gettime_monotonic_success);
	RUN_TEST_CASE(time_clock_gettime, clock_gettime_monotonic_nondecreasing);
	RUN_TEST_CASE(time_clock_gettime, clock_gettime_realtime_advances);
	RUN_TEST_CASE(time_clock_gettime, clock_gettime_einval_invalid_clock);
	RUN_TEST_CASE(time_clock_gettime, clock_gettime_nsec_range);
}


/*
 * Tests: clock_settime
 */

TEST_GROUP(time_clock_settime);

static struct {
	struct timespec saved;
	int savedValid;
} test_settime;


TEST_SETUP(time_clock_settime)
{
	/* Save current time to restore after tests that modify it */
	test_settime.savedValid = (clock_gettime(CLOCK_REALTIME, &test_settime.saved) == 0) ? 1 : 0;
}


TEST_TEAR_DOWN(time_clock_settime)
{
	/* Restore time if we managed to save it */
	if (test_settime.savedValid != 0) {
		struct timespec now;
		if (clock_gettime(CLOCK_REALTIME, &now) == 0) {
			/* Only restore if time was significantly changed */
			long diff = now.tv_sec - test_settime.saved.tv_sec;
			if (diff < -2 || diff > 2) {
				clock_settime(CLOCK_REALTIME, &test_settime.saved);
			}
		}
	}
}


TEST(time_clock_settime, clock_settime_einval_invalid_clock)
{
	/* "EINVAL: The clock_id argument does not specify a known clock." */
	struct timespec tp = { 1000000, 0 };
	int ret;

	errno = 0;
	ret = clock_settime((clockid_t)-99, &tp);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST(time_clock_settime, clock_settime_einval_negative_nsec)
{
	/* "EINVAL: The tp argument specified a nanosecond value less than zero" */
	struct timespec tp;
	int ret;

	tp.tv_sec = 1000000;
	tp.tv_nsec = -1;

	errno = 0;
	ret = clock_settime(CLOCK_REALTIME, &tp);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST(time_clock_settime, clock_settime_einval_nsec_too_large)
{
	/* "EINVAL: The tp argument specified a nanosecond value ...
	 *  greater than or equal to 1000 million." */
	struct timespec tp;
	int ret;

	tp.tv_sec = 1000000;
	tp.tv_nsec = NSEC_PER_SEC;

	errno = 0;
	ret = clock_settime(CLOCK_REALTIME, &tp);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST(time_clock_settime, clock_settime_einval_monotonic)
{
	/* "EINVAL: The value of the clock_id argument is CLOCK_MONOTONIC." */
	struct timespec tp = { 1000000, 0 };
	int ret;

	errno = 0;
	ret = clock_settime(CLOCK_MONOTONIC, &tp);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST(time_clock_settime, clock_settime_realtime_success)
{
	/* "clock_settime() shall set the specified clock" — requires privileges */
	struct timespec tp, readBack;
	int ret;

	/* Get current time and add 1 second */
	ret = clock_gettime(CLOCK_REALTIME, &tp);
	TEST_ASSERT_EQUAL_INT(0, ret);

	tp.tv_sec += 1;
	tp.tv_nsec = 0;

	errno = 0;
	ret = clock_settime(CLOCK_REALTIME, &tp);
	if (ret == -1 && errno == EPERM) {
		TEST_IGNORE_MESSAGE("insufficient privileges to set CLOCK_REALTIME");
	}
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Read back — should be close to what we set */
	ret = clock_gettime(CLOCK_REALTIME, &readBack);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Allow 1 second tolerance */
	TEST_ASSERT_INT_WITHIN(1, (int)tp.tv_sec, (int)readBack.tv_sec);
}


TEST_GROUP_RUNNER(time_clock_settime)
{
	RUN_TEST_CASE(time_clock_settime, clock_settime_einval_invalid_clock);
	RUN_TEST_CASE(time_clock_settime, clock_settime_einval_negative_nsec);
	RUN_TEST_CASE(time_clock_settime, clock_settime_einval_nsec_too_large);
	RUN_TEST_CASE(time_clock_settime, clock_settime_einval_monotonic);
	RUN_TEST_CASE(time_clock_settime, clock_settime_realtime_success);
}
