/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - strings.h
 * TESTED:
 *    - ffs()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Lukasz Kruszynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <strings.h>
#include <limits.h>

#include <unity_fixture.h>


TEST_GROUP(string_ffs);


TEST_SETUP(string_ffs)
{
}


TEST_TEAR_DOWN(string_ffs)
{
}


TEST(string_ffs, ffs_zero_returns_zero)
{
	/* If i is 0, ffs() shall return 0. */
	TEST_ASSERT_EQUAL_INT(0, ffs(0));
}


TEST(string_ffs, ffs_returns_least_significant_index)
{
	/* Bits are numbered from one at the least significant bit. */
	TEST_ASSERT_EQUAL_INT(1, ffs(0x1));  /* 0b0001 */
	TEST_ASSERT_EQUAL_INT(2, ffs(0x2));  /* 0b0010 */
	TEST_ASSERT_EQUAL_INT(3, ffs(0x4));  /* 0b0100 */

	/* With several bits set, the index of the lowest set bit is returned. */
	TEST_ASSERT_EQUAL_INT(1, ffs(0x3));  /* 0b0011 */
	TEST_ASSERT_EQUAL_INT(2, ffs(0x6));  /* 0b0110 */
	TEST_ASSERT_EQUAL_INT(3, ffs(0xc));  /* 0b1100 */
}


TEST(string_ffs, ffs_finds_high_and_sign_bit)
{
	TEST_ASSERT_EQUAL_INT(8, ffs(0x80));   /* first set bit at position 8 */
	TEST_ASSERT_EQUAL_INT(9, ffs(0x100));  /* first set bit at position 9 */

	/* INT_MIN has only the sign bit set: its index equals the width in bits. */
	TEST_ASSERT_EQUAL_INT((int)(sizeof(int) * CHAR_BIT), ffs(INT_MIN));
}


TEST(string_ffs, ffs_handles_negative_values)
{
	/* -1 has all bits set, so the least significant set bit is at index 1. */
	TEST_ASSERT_EQUAL_INT(1, ffs(-1));

	/* -2 clears only the least significant bit, so the result is 2. */
	TEST_ASSERT_EQUAL_INT(2, ffs(-2));
}


TEST_GROUP_RUNNER(string_ffs)
{
	RUN_TEST_CASE(string_ffs, ffs_zero_returns_zero);
	RUN_TEST_CASE(string_ffs, ffs_returns_least_significant_index);
	RUN_TEST_CASE(string_ffs, ffs_finds_high_and_sign_bit);
	RUN_TEST_CASE(string_ffs, ffs_handles_negative_values);
}
