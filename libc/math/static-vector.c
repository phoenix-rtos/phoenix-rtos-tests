/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 *
 * Pre-test using static vector
 *
 * Copyright 2023 Phoenix Systems
 * Author: Adam Debek
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

/* Define needed to include static-vector-data.h */
#define _TEST_STATIC_VECTOR_DATA_H
#include <math.h>
#include <limits.h>
#include <float.h>
#include <stdlib.h>

#include "common.h"
#include "static-vector-data.h"
#include <unity_fixture.h>


/* Number of static vectors */
#define logN (sizeof(static_vectors_log) / sizeof(static_vectors_log[0]))
#define expN (sizeof(static_vectors_exp) / sizeof(static_vectors_exp[0]))

/*
 * Log() and exp() are used to generate random values (used almost in all tests),
 * for that reason some pre-tests with static vectors have to be run
 */

TEST_GROUP(static_vector);


TEST_SETUP(static_vector)
{
}


TEST_TEAR_DOWN(static_vector)
{
}


TEST(static_vector, log)
{
	int i;

	for (i = 0; i < logN; i++) {
		test_exp_vector_check(static_vectors_log[i][0], static_vectors_log[i][1],
			log(static_vectors_log[i][0]), static_vectors_log[i][1] / 100.0);
	}
}


TEST(static_vector, exp)
{
	int i;

	for (i = 0; i < expN; i++) {
		test_exp_vector_check(static_vectors_exp[i][0], static_vectors_exp[i][1],
			exp(static_vectors_exp[i][0]), static_vectors_exp[i][1] / 100.0);
	}
}


TEST_GROUP_RUNNER(static_vector)
{
	RUN_TEST_CASE(static_vector, log);
	RUN_TEST_CASE(static_vector, exp);
}
