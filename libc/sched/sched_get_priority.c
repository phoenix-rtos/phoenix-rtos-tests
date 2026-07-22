/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - sched.h
 * TESTED:
 *    - sched_get_priority_max()
 *    - sched_get_priority_min()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <sched.h>

#include "unity_fixture.h"

/* Tests: sched_get_priority_max, sched_get_priority_min */
TEST_GROUP(sched_get_priority);

TEST_SETUP(sched_get_priority)
{
}

TEST_TEAR_DOWN(sched_get_priority)
{
}


TEST(sched_get_priority, max_sched_fifo)
{
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1686 issue");
#else
	int ret;

#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1639 issue - phoenix limitation");
#endif

	errno = 0;
	ret = sched_get_priority_max(SCHED_FIFO);
	TEST_ASSERT_GREATER_THAN_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(0, errno);
#endif
}


TEST(sched_get_priority, max_sched_rr)
{
	int ret;

	errno = 0;
	ret = sched_get_priority_max(SCHED_RR);
	TEST_ASSERT_GREATER_THAN_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(0, errno);
}


TEST(sched_get_priority, max_sched_other)
{
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1686 issue");
#else
	int ret;
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1639 issue - phoenix limitation");
#endif

	errno = 0;
	ret = sched_get_priority_max(SCHED_OTHER);
	TEST_ASSERT_GREATER_THAN_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(0, errno);
#endif
}


TEST(sched_get_priority, min_sched_fifo)
{
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1686 issue");
#else
	int ret;
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1639 issue - phoenix limitation");
#endif

	errno = 0;
	ret = sched_get_priority_min(SCHED_FIFO);
	TEST_ASSERT_GREATER_THAN_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(0, errno);
#endif
}


TEST(sched_get_priority, min_sched_rr)
{
	int ret;

	errno = 0;
	ret = sched_get_priority_min(SCHED_RR);
	TEST_ASSERT_GREATER_THAN_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(0, errno);
}


TEST(sched_get_priority, min_sched_other)
{
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1686 issue");
#else
	int ret;
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1639 issue - phoenix limitation");
#endif

	errno = 0;
	ret = sched_get_priority_min(SCHED_OTHER);
	TEST_ASSERT_GREATER_THAN_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(0, errno);
#endif
}


TEST(sched_get_priority, max_ge_min_all_policies)
{
	int maxVal, minVal;

#ifndef __phoenix__
	maxVal = sched_get_priority_max(SCHED_FIFO);
	minVal = sched_get_priority_min(SCHED_FIFO);
	TEST_ASSERT_GREATER_THAN_INT(-1, maxVal);
	TEST_ASSERT_GREATER_THAN_INT(-1, minVal);
	TEST_ASSERT_TRUE(maxVal >= minVal);
#endif

	maxVal = sched_get_priority_max(SCHED_RR);
	minVal = sched_get_priority_min(SCHED_RR);
	TEST_ASSERT_GREATER_THAN_INT(-1, maxVal);
	TEST_ASSERT_GREATER_THAN_INT(-1, minVal);
	TEST_ASSERT_TRUE(maxVal >= minVal);

#ifndef __phoenix__
	maxVal = sched_get_priority_max(SCHED_OTHER);
	minVal = sched_get_priority_min(SCHED_OTHER);
	TEST_ASSERT_GREATER_THAN_INT(-1, maxVal);
	TEST_ASSERT_GREATER_THAN_INT(-1, minVal);
	TEST_ASSERT_TRUE(maxVal >= minVal);
#endif
}


TEST(sched_get_priority, einval_invalid_policy)
{
	int ret;
	/* Test sched_get_priority_max with invalid policy */
	errno = 0;
	ret = sched_get_priority_max(-1);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);

	/* Test sched_get_priority_min with invalid policy */
	errno = 0;
	ret = sched_get_priority_min(-1);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);

	/* Test with a large invalid value */
	errno = 0;
	ret = sched_get_priority_max(9999);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);

	errno = 0;
	ret = sched_get_priority_min(9999);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST_GROUP_RUNNER(sched_get_priority)
{
	RUN_TEST_CASE(sched_get_priority, max_sched_fifo);
	RUN_TEST_CASE(sched_get_priority, max_sched_rr);
	RUN_TEST_CASE(sched_get_priority, max_sched_other);
	RUN_TEST_CASE(sched_get_priority, min_sched_fifo);
	RUN_TEST_CASE(sched_get_priority, min_sched_rr);
	RUN_TEST_CASE(sched_get_priority, min_sched_other);
	RUN_TEST_CASE(sched_get_priority, max_ge_min_all_policies);
	RUN_TEST_CASE(sched_get_priority, einval_invalid_policy);
}
