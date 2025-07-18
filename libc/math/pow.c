/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 *
 * HEADER:
 *    - math.h
 *
 * TESTED:
 *    - sqrt(), pow()
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
#include <errno.h>

#include "common.h"
#include <unity_fixture.h>

TEST_GROUP(math_pow);


TEST_SETUP(math_pow)
{
}


TEST_TEAR_DOWN(math_pow)
{
}

TEST(math_pow, sqrt_basic)
{
	int i, iters = 100 * ITER_FACTOR;
	int digLost, acceptLoss = 55;
	double max = DBL_MAX;
	double min = 0.0;
	double x, y, f, g;

	for (i = 0; i < iters; i++) {
		x = test_getRandomLog(min, max);
		y = sqrt(x);
		f = y * y;
		g = x;

		digLost = test_checkResult(f, g);
		test_check_digLost("sqrt", x, digLost, acceptLoss);
	}

	// Dedicated subnormals loop
	iters = 20 * ITER_FACTOR;
	acceptLoss = 60;

	for (i = 0; i < iters; i++) {
		x = test_getRandomLogSubnrm();
		y = sqrt(x);
		f = y * y;
		g = x;

		digLost = test_checkResult(f, g);
		test_check_digLost("sqrt", x, digLost, acceptLoss);
	}
}


TEST(math_pow, sqrt_special_val)
{
	errno = 0;
	TEST_ASSERT_DOUBLE_IS_NAN(sqrt(-1.0));
	TEST_ASSERT_EQUAL_INT(EDOM, errno);

	TEST_ASSERT_DOUBLE_IS_NAN(sqrt(NAN));

	TEST_ASSERT_DOUBLE_IS_ZERO(sqrt(0.0));
	TEST_ASSERT_DOUBLE_IS_NEG_ZERO(sqrt(-0.0));

	TEST_ASSERT_DOUBLE_IS_INF(sqrt(INFINITY));

	errno = 0;
	TEST_ASSERT_DOUBLE_IS_NAN(sqrt(-INFINITY));
	TEST_ASSERT_EQUAL_INT(EDOM, errno);
}

TEST(math_pow, pow_basic)
{
	int i, iters = 100 * ITER_FACTOR;
	int digLost, acceptLoss = 50;
	double ymax = log(DBL_MAX / 2.0);
	double ymin = 1.0e-15;
	double x, xmin, xmax, y, f, t, g;

	for (i = 0; i < iters; i++) {
		y = test_getRandomLog(ymin, ymax);
		xmin = exp(log(DBL_MIN * 2.0) / y);
		xmax = exp(log(DBL_MAX / 2.0) / y);
		x = test_getRandomLog(xmin, xmax);
		t = pow(x, y / 2.0);
		f = t * t;
		g = pow(x, y);

		digLost = test_checkResult(f, g);
		test_check_digLost2("pow", x, y, digLost, acceptLoss);
	}

	ymin = 1.0;

	for (i = 0; i < iters; i++) {
		y = ceil(test_getRandomLog(ymin, ymax));
		xmin = exp(log(DBL_MIN * 2.0) / y);
		xmax = exp(log(DBL_MAX / 2.0) / y);
		x = test_getRandomLog(xmin, xmax);
		y = -y;
		t = pow(x, y / 2.0);
		f = t * t;
		g = pow(x, y);

		digLost = test_checkResult(f, g);
		test_check_digLost2("pow", x, y, digLost, acceptLoss);
	}
}

TEST(math_pow, pow_special_val)
{
	/* Initialize x and y to random finite value other than 0.0 */
	double x = 1.2;
	double y = 1.2;

	TEST_ASSERT_DOUBLE_IS_NAN(pow(NAN, y));
	TEST_ASSERT_DOUBLE_IS_NAN(pow(x, NAN));
	TEST_ASSERT_DOUBLE_IS_NAN(pow(NAN, NAN));

	TEST_ASSERT_EQUAL_DOUBLE(1.0, pow(1.0, NAN));
	TEST_ASSERT_EQUAL_DOUBLE(1.0, pow(1.0, y));

	TEST_ASSERT_EQUAL_DOUBLE(1.0, pow(NAN, 0.0));
	TEST_ASSERT_EQUAL_DOUBLE(1.0, pow(NAN, -0.0));

	TEST_ASSERT_EQUAL_DOUBLE(1.0, pow(x, 0.0));
	TEST_ASSERT_EQUAL_DOUBLE(1.0, pow(x, -0.0));

	TEST_ASSERT_DOUBLE_IS_ZERO(pow(0.0, 3.0));
	TEST_ASSERT_DOUBLE_IS_NEG_ZERO(pow(-0.0, 3.0));

	TEST_ASSERT_DOUBLE_IS_ZERO(pow(0.0, 2.0));
	TEST_ASSERT_DOUBLE_IS_ZERO(pow(-0.0, 2.0));
	TEST_ASSERT_DOUBLE_IS_ZERO(pow(0.0, 2.5));
	TEST_ASSERT_DOUBLE_IS_ZERO(pow(-0.0, 2.5));

	TEST_ASSERT_EQUAL_DOUBLE(1.0, pow(-1.0, INFINITY));
	TEST_ASSERT_EQUAL_DOUBLE(1.0, pow(-1.0, -INFINITY));

	TEST_ASSERT_DOUBLE_IS_INF(pow(0.3, -INFINITY));
	TEST_ASSERT_DOUBLE_IS_INF(pow(-0.3, -INFINITY));

	TEST_ASSERT_DOUBLE_IS_ZERO(pow(1.3, -INFINITY));
	TEST_ASSERT_DOUBLE_IS_ZERO(pow(-1.3, -INFINITY));

	TEST_ASSERT_DOUBLE_IS_ZERO(pow(0.3, INFINITY));
	TEST_ASSERT_DOUBLE_IS_ZERO(pow(-0.3, INFINITY));

	TEST_ASSERT_DOUBLE_IS_INF(pow(1.3, INFINITY));
	TEST_ASSERT_DOUBLE_IS_INF(pow(-1.3, INFINITY));

	TEST_ASSERT_DOUBLE_IS_NEG_ZERO(pow(-INFINITY, -3.0));

	TEST_ASSERT_DOUBLE_IS_ZERO(pow(-INFINITY, -2.0));
	TEST_ASSERT_DOUBLE_IS_ZERO(pow(-INFINITY, -2.5));

	TEST_ASSERT_DOUBLE_IS_NEG_INF(pow(-INFINITY, 3.0));

	TEST_ASSERT_DOUBLE_IS_INF(pow(-INFINITY, 2.0));
	TEST_ASSERT_DOUBLE_IS_INF(pow(-INFINITY, 2.5));

	TEST_ASSERT_DOUBLE_IS_ZERO(pow(INFINITY, -y));
	TEST_ASSERT_DOUBLE_IS_INF(pow(INFINITY, y));

	errno = 0;
	TEST_ASSERT_EQUAL_DOUBLE(HUGE_VAL, pow(0.0, -3.0));
	TEST_ASSERT_EQUAL_INT(ERANGE, errno);

	errno = 0;
	TEST_ASSERT_EQUAL_DOUBLE(-HUGE_VAL, pow(-0.0, -3.0));
	TEST_ASSERT_EQUAL_INT(ERANGE, errno);

	errno = 0;
	TEST_ASSERT_EQUAL_DOUBLE(HUGE_VAL, pow(0.0, -2.0));
	TEST_ASSERT_EQUAL_INT(ERANGE, errno);

	errno = 0;
	TEST_ASSERT_EQUAL_DOUBLE(HUGE_VAL, pow(-0.0, -1.5));
	TEST_ASSERT_EQUAL_INT(ERANGE, errno);

	errno = 0;
	TEST_ASSERT_DOUBLE_IS_INF(pow(1e308, 2.0));
	TEST_ASSERT_EQUAL_INT(ERANGE, errno);
}

TEST_GROUP_RUNNER(math_pow)
{
	test_setup();

	RUN_TEST_CASE(math_pow, sqrt_basic);
	RUN_TEST_CASE(math_pow, sqrt_special_val);

	RUN_TEST_CASE(math_pow, pow_basic);
	RUN_TEST_CASE(math_pow, pow_special_val);
}
