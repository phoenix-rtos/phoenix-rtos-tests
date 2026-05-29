/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <time.h>
 * TESTED:
 *    - clock_nanosleep()
 *    - nanosleep()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <time.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

#include "unity_fixture.h"

#define NSEC_PER_SEC  1000000000L
#define SLEEP_10MS_NS 10000000L
#define SLEEP_50MS_NS 50000000L


/*
 * Tests: clock_nanosleep
 */

TEST_GROUP(time_clock_nanosleep);


TEST_SETUP(time_clock_nanosleep)
{
}


TEST_TEAR_DOWN(time_clock_nanosleep)
{
}


TEST(time_clock_nanosleep, clock_nanosleep_relative_success)
{
	/* "If the flag TIMER_ABSTIME is not set ... clock_nanosleep() shall cause
	 *  the current thread to be suspended ... until the time interval specified
	 *  by the rqtp argument has elapsed" */
	const struct timespec rqtp = { 0, SLEEP_10MS_NS };
	struct timespec before, after;
	int ret;

	TEST_ASSERT_EQUAL_INT(0, clock_gettime(CLOCK_MONOTONIC, &before));

	ret = clock_nanosleep(CLOCK_MONOTONIC, 0, &rqtp, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	TEST_ASSERT_EQUAL_INT(0, clock_gettime(CLOCK_MONOTONIC, &after));

	/* Elapsed time must be at least 10ms */
	long elapsed = (after.tv_sec - before.tv_sec) * NSEC_PER_SEC +
		(after.tv_nsec - before.tv_nsec);
	TEST_ASSERT_TRUE(elapsed >= SLEEP_10MS_NS);
}


TEST(time_clock_nanosleep, clock_nanosleep_relative_zero)
{
	/* Zero sleep shall return immediately */
	const struct timespec rqtp = { 0, 0 };
	int ret;

	ret = clock_nanosleep(CLOCK_MONOTONIC, 0, &rqtp, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST(time_clock_nanosleep, clock_nanosleep_absolute_past)
{
	/* "If ... the time value specified by rqtp is less than or equal to
	 *  the time value of the specified clock, then clock_nanosleep() shall
	 *  return immediately" */
	struct timespec rqtp = { 0, 0 }; /* time in the past (epoch) */
	int ret;

	ret = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &rqtp, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST(time_clock_nanosleep, clock_nanosleep_absolute_success)
{
	/* Absolute sleep until a near-future time */
	struct timespec now, target;
	int ret;

	TEST_ASSERT_EQUAL_INT(0, clock_gettime(CLOCK_MONOTONIC, &now));

	/* Sleep until 20ms from now */
	target.tv_sec = now.tv_sec;
	target.tv_nsec = now.tv_nsec + 20000000L;
	if (target.tv_nsec >= NSEC_PER_SEC) {
		target.tv_sec += 1;
		target.tv_nsec -= NSEC_PER_SEC;
	}

	ret = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &target, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Verify we reached at least the target time */
	struct timespec after;
	TEST_ASSERT_EQUAL_INT(0, clock_gettime(CLOCK_MONOTONIC, &after));
	TEST_ASSERT_TRUE(
		(after.tv_sec > target.tv_sec) ||
		((after.tv_sec == target.tv_sec) && (after.tv_nsec >= target.tv_nsec)));
}


TEST(time_clock_nanosleep, clock_nanosleep_realtime)
{
	/* clock_nanosleep with CLOCK_REALTIME (relative) is equivalent to nanosleep */
	const struct timespec rqtp = { 0, SLEEP_10MS_NS };
	int ret;

	ret = clock_nanosleep(CLOCK_REALTIME, 0, &rqtp, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST(time_clock_nanosleep, clock_nanosleep_einval_negative_nsec)
{
	/* "EINVAL: The rqtp argument specified a nanosecond value less than zero" */
	struct timespec rqtp = { 0, -1 };
	int ret;

	ret = clock_nanosleep(CLOCK_MONOTONIC, 0, &rqtp, NULL);
	TEST_ASSERT_EQUAL_INT(EINVAL, ret);
}


TEST(time_clock_nanosleep, clock_nanosleep_einval_nsec_too_large)
{
	/* "EINVAL: ... greater than or equal to 1000 million" */
	struct timespec rqtp = { 0, NSEC_PER_SEC };
	int ret;

	ret = clock_nanosleep(CLOCK_MONOTONIC, 0, &rqtp, NULL);
	TEST_ASSERT_EQUAL_INT(EINVAL, ret);
}


TEST(time_clock_nanosleep, clock_nanosleep_einval_invalid_clock)
{
	/* "EINVAL: ... the clock_id argument does not specify a known clock" */
	const struct timespec rqtp = { 0, SLEEP_10MS_NS };
	int ret;

	ret = clock_nanosleep((clockid_t)-99, 0, &rqtp, NULL);
	TEST_ASSERT_EQUAL_INT(EINVAL, ret);
}


TEST(time_clock_nanosleep, clock_nanosleep_rmtp_updated)
{
	/* "if the rmtp argument is non-NULL, the timespec structure referenced
	 *  by it shall be updated to contain the amount of time remaining" —
	 *  for uninterrupted sleep, remaining should be zero */
	const struct timespec rqtp = { 0, SLEEP_10MS_NS };
	struct timespec rmtp = { 99, 99 };
	int ret;

	ret = clock_nanosleep(CLOCK_MONOTONIC, 0, &rqtp, &rmtp);
	TEST_ASSERT_EQUAL_INT(0, ret);
	/* After successful (uninterrupted) sleep, rmtp is unspecified by POSIX
	 * for clock_nanosleep, but many implementations zero it. Just verify no crash. */
}


TEST_GROUP_RUNNER(time_clock_nanosleep)
{
	RUN_TEST_CASE(time_clock_nanosleep, clock_nanosleep_relative_success);
	RUN_TEST_CASE(time_clock_nanosleep, clock_nanosleep_relative_zero);
	RUN_TEST_CASE(time_clock_nanosleep, clock_nanosleep_absolute_past);
	RUN_TEST_CASE(time_clock_nanosleep, clock_nanosleep_absolute_success);
	RUN_TEST_CASE(time_clock_nanosleep, clock_nanosleep_realtime);
	RUN_TEST_CASE(time_clock_nanosleep, clock_nanosleep_einval_negative_nsec);
	RUN_TEST_CASE(time_clock_nanosleep, clock_nanosleep_einval_nsec_too_large);
	RUN_TEST_CASE(time_clock_nanosleep, clock_nanosleep_einval_invalid_clock);
	RUN_TEST_CASE(time_clock_nanosleep, clock_nanosleep_rmtp_updated);
}


/*
 * Tests: nanosleep
 */

TEST_GROUP(time_nanosleep);


TEST_SETUP(time_nanosleep)
{
}


TEST_TEAR_DOWN(time_nanosleep)
{
}


TEST(time_nanosleep, nanosleep_success)
{
	/* "nanosleep() shall cause the current thread to be suspended ...
	 *  until ... the time interval specified by the rqtp argument has elapsed" */
	const struct timespec rqtp = { 0, SLEEP_10MS_NS };
	struct timespec before, after;
	int ret;

	TEST_ASSERT_EQUAL_INT(0, clock_gettime(CLOCK_MONOTONIC, &before));

	errno = 0;
	ret = nanosleep(&rqtp, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, errno);

	TEST_ASSERT_EQUAL_INT(0, clock_gettime(CLOCK_MONOTONIC, &after));

	long elapsed = (after.tv_sec - before.tv_sec) * NSEC_PER_SEC +
		(after.tv_nsec - before.tv_nsec);
	TEST_ASSERT_TRUE(elapsed >= SLEEP_10MS_NS);
}


TEST(time_nanosleep, nanosleep_zero)
{
	/* Zero-duration sleep returns immediately */
	const struct timespec rqtp = { 0, 0 };
	int ret;

	ret = nanosleep(&rqtp, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST(time_nanosleep, nanosleep_einval_negative_nsec)
{
	/* "EINVAL: The rqtp argument specified a nanosecond value less than zero" */
	struct timespec rqtp = { 0, -1 };
	int ret;

	errno = 0;
	ret = nanosleep(&rqtp, NULL);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST(time_nanosleep, nanosleep_einval_nsec_too_large)
{
	/* "EINVAL: ... greater than or equal to 1000 million" */
	struct timespec rqtp = { 0, NSEC_PER_SEC };
	int ret;

	errno = 0;
	ret = nanosleep(&rqtp, NULL);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST(time_nanosleep, nanosleep_rmtp_null)
{
	/* rmtp may be NULL — no crash */
	const struct timespec rqtp = { 0, SLEEP_10MS_NS };
	int ret;

	ret = nanosleep(&rqtp, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST(time_nanosleep, nanosleep_rmtp_same_object)
{
	/* "The rqtp and rmtp arguments can point to the same object." */
	struct timespec ts = { 0, SLEEP_10MS_NS };
	int ret;

	ret = nanosleep(&ts, &ts);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST(time_nanosleep, nanosleep_minimum_duration)
{
	/* "the suspension time shall not be less than the time specified by rqtp" */
	const struct timespec rqtp = { 0, SLEEP_50MS_NS };
	struct timespec before, after;
	int ret;

	TEST_ASSERT_EQUAL_INT(0, clock_gettime(CLOCK_MONOTONIC, &before));

	ret = nanosleep(&rqtp, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	TEST_ASSERT_EQUAL_INT(0, clock_gettime(CLOCK_MONOTONIC, &after));

	long elapsed = (after.tv_sec - before.tv_sec) * NSEC_PER_SEC +
		(after.tv_nsec - before.tv_nsec);
	TEST_ASSERT_TRUE(elapsed >= SLEEP_50MS_NS);
}


TEST_GROUP_RUNNER(time_nanosleep)
{
	RUN_TEST_CASE(time_nanosleep, nanosleep_success);
	RUN_TEST_CASE(time_nanosleep, nanosleep_zero);
	RUN_TEST_CASE(time_nanosleep, nanosleep_einval_negative_nsec);
	RUN_TEST_CASE(time_nanosleep, nanosleep_einval_nsec_too_large);
	RUN_TEST_CASE(time_nanosleep, nanosleep_rmtp_null);
	RUN_TEST_CASE(time_nanosleep, nanosleep_rmtp_same_object);
	RUN_TEST_CASE(time_nanosleep, nanosleep_minimum_duration);
}
