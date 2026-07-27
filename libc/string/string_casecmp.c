/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - strings.h
 * TESTED:
 *    - strcasecmp()
 *    - strncasecmp()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Lukasz Kruszynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <strings.h>
#include <string.h>

#include <unity_fixture.h>

/* Case-differing strings that shall compare equal (interior mixed case). */
#define CI_EQUAL_A "MixedCase"
#define CI_EQUAL_B "mIXEDcASE"

#define EMPTY_STR ""

/* ORDER_LESSER lowercases to "apple", which is byte-less-than "banana". */
#define ORDER_LESSER  "APPLE"
#define ORDER_GREATER "banana"

/* A proper prefix compares less than the longer string. */
#define PREFIX_SHORT "abc"
#define PREFIX_LONG  "abcd"

/* Equal within the first BOUND_PREFIX_LEN bytes, differ afterwards. */
#define BOUND_SAME_PREFIX_A "abcXXX"
#define BOUND_SAME_PREFIX_B "ABCyyy"
#define BOUND_PREFIX_LEN    3U


TEST_GROUP(string_casecmp);


TEST_SETUP(string_casecmp)
{
}


TEST_TEAR_DOWN(string_casecmp)
{
}


TEST(string_casecmp, casecmp_equal_ignoring_case)
{
	/* Strings differing only in case shall compare equal in the POSIX locale. */
	TEST_ASSERT_EQUAL_INT(0, strcasecmp(CI_EQUAL_A, CI_EQUAL_B));
	TEST_ASSERT_EQUAL_INT(0, strncasecmp(CI_EQUAL_A, CI_EQUAL_B, strlen(CI_EQUAL_A)));

	/* Two empty strings compare equal. */
	TEST_ASSERT_EQUAL_INT(0, strcasecmp(EMPTY_STR, EMPTY_STR));
	TEST_ASSERT_EQUAL_INT(0, strncasecmp(EMPTY_STR, EMPTY_STR, 1U));
}


TEST(string_casecmp, casecmp_orders_by_char_value)
{
	/* When unequal, the sign reflects the comparison of the lowercased bytes. */
	TEST_ASSERT_LESS_THAN_INT(0, strcasecmp(ORDER_LESSER, ORDER_GREATER));
	TEST_ASSERT_GREATER_THAN_INT(0, strcasecmp(ORDER_GREATER, ORDER_LESSER));

	TEST_ASSERT_LESS_THAN_INT(0, strncasecmp(ORDER_LESSER, ORDER_GREATER, strlen(ORDER_LESSER)));
	TEST_ASSERT_GREATER_THAN_INT(0, strncasecmp(ORDER_GREATER, ORDER_LESSER, strlen(ORDER_LESSER)));
}


TEST(string_casecmp, casecmp_prefix_is_less)
{
	/* A string that is a proper prefix of the other compares less than it. */
	TEST_ASSERT_LESS_THAN_INT(0, strcasecmp(PREFIX_SHORT, PREFIX_LONG));
	TEST_ASSERT_GREATER_THAN_INT(0, strcasecmp(PREFIX_LONG, PREFIX_SHORT));
}


TEST(string_casecmp, ncasecmp_zero_length_is_equal)
{
	/* With n == 0 no bytes are compared, so the result shall be 0. */
	TEST_ASSERT_EQUAL_INT(0, strncasecmp(ORDER_LESSER, ORDER_GREATER, 0U));
}


TEST(string_casecmp, ncasecmp_ignores_bytes_beyond_n)
{
	/* Bytes at or beyond index n do not affect the result. */
	TEST_ASSERT_EQUAL_INT(0, strncasecmp(BOUND_SAME_PREFIX_A, BOUND_SAME_PREFIX_B, BOUND_PREFIX_LEN));
}


TEST_GROUP_RUNNER(string_casecmp)
{
	RUN_TEST_CASE(string_casecmp, casecmp_equal_ignoring_case);
	RUN_TEST_CASE(string_casecmp, casecmp_orders_by_char_value);
	RUN_TEST_CASE(string_casecmp, casecmp_prefix_is_less);
	RUN_TEST_CASE(string_casecmp, ncasecmp_zero_length_is_equal);
	RUN_TEST_CASE(string_casecmp, ncasecmp_ignores_bytes_beyond_n);
}
