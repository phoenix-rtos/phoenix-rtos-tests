/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - stdlib.h
 * TESTED:
 *    - rand_r()
 *    - random()
 *    - srandom()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Lukasz Kruszynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>
#include <limits.h>

#include <unity_fixture.h>

/* Number of successive draws used to probe range and reproducibility. */
#define RANDOM_SEQ_LEN 64

/* Upper bound of random() output, per POSIX: [0, 2^31 - 1]. */
#define RANDOM_MAX 2147483647L

/* Arbitrary non-trivial seed used for reproducibility checks. */
#define TEST_SEED 12345U


TEST_GROUP(stdlib_rand_r);
TEST_GROUP(stdlib_random);


TEST_SETUP(stdlib_rand_r)
{
}


TEST_TEAR_DOWN(stdlib_rand_r)
{
}


TEST(stdlib_rand_r, rand_r_range)
{
	int i;
	unsigned seed = TEST_SEED;

	/* Every value returned shall lie within [0, RAND_MAX]. */
	for (i = 0; i < RANDOM_SEQ_LEN; i++) {
		int val = rand_r(&seed);
		TEST_ASSERT_GREATER_OR_EQUAL_INT(0, val);
		TEST_ASSERT_LESS_OR_EQUAL_INT(RAND_MAX, val);
	}
}


TEST(stdlib_rand_r, rand_r_reproducible)
{
	int i;
	unsigned seedA = TEST_SEED;
	unsigned seedB = TEST_SEED;

	/*
	 * Same initial seed value, with the seed object not modified by the
	 * caller between calls, shall yield the same sequence.
	 */
	for (i = 0; i < RANDOM_SEQ_LEN; i++) {
		TEST_ASSERT_EQUAL_INT(rand_r(&seedA), rand_r(&seedB));
	}
}


TEST(stdlib_rand_r, rand_r_updates_seed)
{
	unsigned seed = TEST_SEED;

	/* The call updates the object pointed to by seed to carry state. */
	(void)rand_r(&seed);
	TEST_ASSERT_NOT_EQUAL_UINT(TEST_SEED, seed);
}


TEST_SETUP(stdlib_random)
{
}


TEST_TEAR_DOWN(stdlib_random)
{
}


TEST(stdlib_random, random_range)
{
	int i;

	srandom(TEST_SEED);

	/* random() shall return values in the range [0, 2^31 - 1]. */
	for (i = 0; i < RANDOM_SEQ_LEN; i++) {
		long val = random();
		TEST_ASSERT_GREATER_OR_EQUAL_INT(0, val);
		TEST_ASSERT_LESS_OR_EQUAL_INT(RANDOM_MAX, val);
	}
}


TEST(stdlib_random, random_reproducible)
{
	int i;
	static long first[RANDOM_SEQ_LEN];

	/* Seeding with a value, then re-seeding with it, repeats the sequence. */
	srandom(TEST_SEED);
	for (i = 0; i < RANDOM_SEQ_LEN; i++) {
		first[i] = random();
	}

	srandom(TEST_SEED);
	for (i = 0; i < RANDOM_SEQ_LEN; i++) {
		TEST_ASSERT_EQUAL_INT(first[i], random());
	}
}


TEST(stdlib_random, random_seed_one_reproducible)
{
	int i;
	static long first[RANDOM_SEQ_LEN];

	/*
	 * Seed value 1 is the default sequence; re-seeding with 1 shall
	 * reproduce it.
	 */
	srandom(1U);
	for (i = 0; i < RANDOM_SEQ_LEN; i++) {
		first[i] = random();
	}

	srandom(1U);
	for (i = 0; i < RANDOM_SEQ_LEN; i++) {
		TEST_ASSERT_EQUAL_INT(first[i], random());
	}
}


TEST_GROUP_RUNNER(stdlib_rand_r)
{
	RUN_TEST_CASE(stdlib_rand_r, rand_r_range);
	RUN_TEST_CASE(stdlib_rand_r, rand_r_reproducible);
	RUN_TEST_CASE(stdlib_rand_r, rand_r_updates_seed);
}


TEST_GROUP_RUNNER(stdlib_random)
{
	RUN_TEST_CASE(stdlib_random, random_range);
	RUN_TEST_CASE(stdlib_random, random_reproducible);
	RUN_TEST_CASE(stdlib_random, random_seed_one_reproducible);
}
