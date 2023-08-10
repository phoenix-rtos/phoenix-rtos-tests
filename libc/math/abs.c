/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 *
 * HEADER:
 *    - math.h
 *
 * TESTED:
 *    - fabs()
 *
 * Copyright 2023 Phoenix Systems
 * Author: Adam Debek
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <math.h>
#include <limits.h>
#include <float.h>
#include <time.h>
#include <stdlib.h>

#include "common.h"
#include <unity_fixture.h>

TEST_GROUP(math_abs);


TEST_SETUP(math_abs)
{
}


TEST_TEAR_DOWN(math_abs)
{
}

TEST(math_abs, fabs_basic)
{
	int i, iters = 50 * ITER_FACTOR;
	double max = DBL_MAX;
	double min = 0.0;
	double x;

	for (i = 0; i < iters; i++) {
		x = test_getRandomLog(min, max);

		TEST_ASSERT_EQUAL_DOUBLE(x, fabs(x));
		TEST_ASSERT_EQUAL_DOUBLE(x, fabs(-x));
	}
}


TEST(math_abs, fabs_max_min)
{
	TEST_ASSERT_EQUAL_DOUBLE(DBL_MIN, fabs(DBL_MIN));
	TEST_ASSERT_EQUAL_DOUBLE(DBL_MIN, fabs(-DBL_MIN));

	TEST_ASSERT_EQUAL_DOUBLE(DBL_MAX, fabs(DBL_MAX));
	TEST_ASSERT_EQUAL_DOUBLE(DBL_MAX, fabs(-DBL_MAX));
}


TEST(math_abs, fabs_special_cond)
{
	TEST_ASSERT_DOUBLE_IS_NAN(fabs(NAN));
	TEST_ASSERT_DOUBLE_IS_NAN(fabs(-NAN));

	TEST_ASSERT_EQUAL_DOUBLE(0.0, fabs(0.0));
	TEST_ASSERT_EQUAL_DOUBLE(0.0, fabs(-0.0));

	TEST_ASSERT_EQUAL_DOUBLE(INFINITY, fabs(INFINITY));
	TEST_ASSERT_EQUAL_DOUBLE(INFINITY, fabs(-INFINITY));
}

TEST_GROUP_RUNNER(math_abs)
{
	test_setup();

	RUN_TEST_CASE(math_abs, fabs_basic);
	RUN_TEST_CASE(math_abs, fabs_max_min);
	RUN_TEST_CASE(math_abs, fabs_special_cond);
}
