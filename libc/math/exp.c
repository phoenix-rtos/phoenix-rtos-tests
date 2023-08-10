/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 *
 * HEADER:
 *    - math.h
 *
 * TESTED:
 *    - exp(), frexp(), ldexp()
 *    - log(), log2(), log10()
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
#include <errno.h>
#include <stdlib.h>

#include "common.h"
#include <unity_fixture.h>

TEST_GROUP(math_exp);


TEST_SETUP(math_exp)
{
}


TEST_TEAR_DOWN(math_exp)
{
}


TEST(math_exp, exp_basic)
{
	int i, iters = 100 * ITER_FACTOR;
	double acceptLoss = 50.0;
	double xmax = log(DBL_MAX);
	double xmin = 1.0;
	double x, y, ymin, ymax, f, g, digLost;

	for (i = 0; i < iters; i++) {
		x = test_getRandomLog(xmin, xmax);
		ymin = -xmax;
		ymax = xmax - x;

		y = (double)rand() / RAND_MAX * (ymax - ymin) + ymin;
		/* Using exponent addition rule */
		f = exp(x) * exp(y);
		g = exp(x + y);

		digLost = test_checkResult(f, g);
		test_check_digLost2("exp", x, y, digLost, acceptLoss);
	}
}


TEST(math_exp, exp_special_cond)
{
	// TEST_ASSERT_DOUBLE_IS_NAN(exp(NAN));
	// TEST_ASSERT_DOUBLE_IS_NAN(exp(-NAN));

	TEST_ASSERT_EQUAL_DOUBLE(1.0, exp(0.0));
	TEST_ASSERT_EQUAL_DOUBLE(1.0, exp(-0.0));

	TEST_ASSERT_EQUAL_DOUBLE(INFINITY, exp(INFINITY));
	TEST_ASSERT_EQUAL_DOUBLE(0.0, exp(-INFINITY));
}


TEST(math_exp, frexp_basic)
{
	int i, e, iters = 10 * ITER_FACTOR;
	double acceptLoss = 1.0;
	double max = DBL_MAX / 2.0;
	double min = DBL_MIN;
	double x, f, g, y, digLost;

	for (i = 0; i < iters; i++) {
		x = test_getRandomLog(min, max);

		if (i % 2) {
			x = -x;
		}

		y = frexp(x, &e);

		if (fabs(y) >= 1.0 || fabs(y) < 0.5) {
			char errStr[100];
			sprintf(errStr, "frexp(%g, int *exp) returned %g - value out of range <0.5, 1)", x, y);
			TEST_FAIL_MESSAGE(errStr);
		}

		if (e > 0.0) {
			f = y * pow(2.0, e);
		}
		else {
			f = y / pow(2.0, -e);
		}

		g = x;

		digLost = test_checkResult(f, g);
		test_check_digLost("frexp", x, digLost, acceptLoss);
	}
}


TEST(math_exp, frexp_special_cond)
{
	int exp;

	// TEST_ASSERT_DOUBLE_IS_NAN(frexp(NAN, &exp));
	// TEST_ASSERT_DOUBLE_IS_NAN(frexp(-NAN, &exp));

	TEST_ASSERT_EQUAL_DOUBLE(0.0, frexp(0.0, &exp));
	TEST_ASSERT_EQUAL_DOUBLE(-0.0, frexp(-0.0, &exp));

	// TEST_ASSERT_EQUAL_DOUBLE(INFINITY, frexp(INFINITY, &exp));
	// TEST_ASSERT_EQUAL_DOUBLE(-INFINITY, frexp(-INFINITY, &exp));
}


TEST(math_exp, ldexp_basic)
{
	int i, e, iters = 10 * ITER_FACTOR;
	double acceptLoss = 1.0;
	double max = DBL_MAX / 2.0;
	double min = DBL_MIN;
	double x, f, g, y, digLost;

	for (i = 0; i < iters; i++) {
		x = test_getRandomLog(min, max);

		if (i % 2) {
			x = -x;
		}

		y = frexp(x, &e);
		f = ldexp(y, e);
		g = x;

		digLost = test_checkResult(f, g);
		test_check_digLost("ldexp", x, digLost, acceptLoss);
	}
}


TEST(math_exp, ldexp_special_cond)
{
	/* Initialize x and exp to random finite value other than 0.0 */
	double x = 1.2;
	int exp = 2;

	// TEST_ASSERT_DOUBLE_IS_NAN(ldexp(NAN, exp));
	// TEST_ASSERT_DOUBLE_IS_NAN(ldexp(-NAN, exp));

	TEST_ASSERT_EQUAL_DOUBLE(0.0, ldexp(0.0, exp));
	TEST_ASSERT_EQUAL_DOUBLE(-0.0, ldexp(-0.0, exp));

	TEST_ASSERT_EQUAL_DOUBLE(INFINITY, ldexp(INFINITY, exp));
	TEST_ASSERT_EQUAL_DOUBLE(-INFINITY, ldexp(-INFINITY, exp));

	TEST_ASSERT_EQUAL_DOUBLE(x, ldexp(x, 0));
}


TEST(math_exp, log_basic)
{
	int i, iters = 20 * ITER_FACTOR;
	double acceptLoss = 50.0;
	double xmax = log(DBL_MAX);
	double xmin = 1.0e-20;
	double x, y, ymin, ymax, f, g, digLost;

	for (i = 0; i < iters; i++) {
		x = test_getRandomLog(xmin, xmax);
		ymin = xmin;
		ymax = xmax - x;

		y = (double)rand() / RAND_MAX * (ymax - ymin) + ymin;
		/* Using logarithm properties */
		f = log(x) + log(y);
		g = log(x * y);

		digLost = test_checkResult(f, g);
		test_check_digLost("log", x * y, digLost, acceptLoss);
	}
}


TEST(math_exp, log_special_cond)
{
	// TEST_ASSERT_DOUBLE_IS_NAN(log(NAN));
	// TEST_ASSERT_DOUBLE_IS_NAN(log(-NAN));

	errno = 0;
	TEST_ASSERT_EQUAL_DOUBLE(-HUGE_VAL, log(0.0));
	TEST_ASSERT_EQUAL_INT(ERANGE, errno);

	errno = 0;
	TEST_ASSERT_EQUAL_DOUBLE(-HUGE_VAL, log(-0.0));
	TEST_ASSERT_EQUAL_INT(ERANGE, errno);

	// TEST_ASSERT_EQUAL_DOUBLE(INFINITY, log(INFINITY));

	TEST_ASSERT_EQUAL_DOUBLE(0.0, log(1.0));

	errno = 0;
	TEST_ASSERT_DOUBLE_IS_NAN(log(-1.0));
	TEST_ASSERT_EQUAL_INT(EDOM, errno);
}


TEST(math_exp, log2_basic)
{
	int i, iters = 20 * ITER_FACTOR;
	double acceptLoss = 50.0;
	double xmax = log(DBL_MAX);
	double xmin = 1.0e-10;
	double x, y, ymin, ymax, f, g, digLost;

	for (i = 0; i < iters; i++) {
		x = test_getRandomLog(xmin, xmax);
		ymin = xmin;
		ymax = xmax - x;

		y = (double)rand() / RAND_MAX * (ymax - ymin) + ymin;
		f = log2(x) + log2(y);
		g = log2(x * y);

		digLost = test_checkResult(f, g);
		test_check_digLost("log2", x * y, digLost, acceptLoss);
	}
}


TEST(math_exp, log2_special_cond)
{
	// TEST_ASSERT_DOUBLE_IS_NAN(log2(NAN));
	// TEST_ASSERT_DOUBLE_IS_NAN(log2(-NAN));

	errno = 0;
	TEST_ASSERT_EQUAL_DOUBLE(-HUGE_VAL, log2(0.0));
	TEST_ASSERT_EQUAL_INT(ERANGE, errno);

	errno = 0;
	TEST_ASSERT_EQUAL_DOUBLE(-HUGE_VAL, log2(-0.0));
	TEST_ASSERT_EQUAL_INT(ERANGE, errno);

	// TEST_ASSERT_EQUAL_DOUBLE(INFINITY, log2(INFINITY));

	TEST_ASSERT_EQUAL_DOUBLE(0.0, log2(1.0));

	errno = 0;
	TEST_ASSERT_DOUBLE_IS_NAN(log2(-1.0));
	TEST_ASSERT_EQUAL_INT(EDOM, errno);
}


TEST(math_exp, log10_basic)
{
	int i, iters = 20 * ITER_FACTOR;
	double acceptLoss = 50.0;
	double xmax = log(DBL_MAX);
	double xmin = 1.0e-10;
	double x, y, ymin, ymax, f, g, digLost;

	for (i = 0; i < iters; i++) {
		x = test_getRandomLog(xmin, xmax);
		ymin = xmin;
		ymax = xmax - x;

		y = (double)rand() / RAND_MAX * (ymax - ymin) + ymin;
		f = log10(x) + log10(y);
		g = log10(x * y);

		digLost = test_checkResult(f, g);
		test_check_digLost("log10", x * y, digLost, acceptLoss);
	}
}


TEST(math_exp, log10_special_cond)
{
	// TEST_ASSERT_DOUBLE_IS_NAN(log10(NAN));
	// TEST_ASSERT_DOUBLE_IS_NAN(log10(-NAN));

	errno = 0;
	TEST_ASSERT_EQUAL_DOUBLE(-HUGE_VAL, log10(0.0));
	TEST_ASSERT_EQUAL_INT(ERANGE, errno);

	errno = 0;
	TEST_ASSERT_EQUAL_DOUBLE(-HUGE_VAL, log10(-0.0));
	TEST_ASSERT_EQUAL_INT(ERANGE, errno);

	// TEST_ASSERT_EQUAL_DOUBLE(INFINITY, log10(INFINITY));

	TEST_ASSERT_EQUAL_DOUBLE(0.0, log10(1.0));

	errno = 0;
	TEST_ASSERT_DOUBLE_IS_NAN(log10(-1.0));
	TEST_ASSERT_EQUAL_INT(EDOM, errno);
}


TEST_GROUP_RUNNER(math_exp)
{
	test_setup();

	RUN_TEST_CASE(math_exp, exp_basic);
	RUN_TEST_CASE(math_exp, exp_special_cond);

	RUN_TEST_CASE(math_exp, frexp_basic);
	RUN_TEST_CASE(math_exp, frexp_special_cond);

	RUN_TEST_CASE(math_exp, ldexp_basic);
	RUN_TEST_CASE(math_exp, ldexp_special_cond);

	RUN_TEST_CASE(math_exp, log_basic);
	RUN_TEST_CASE(math_exp, log_special_cond);

	RUN_TEST_CASE(math_exp, log2_basic);
	RUN_TEST_CASE(math_exp, log2_special_cond);

	RUN_TEST_CASE(math_exp, log10_basic);
	RUN_TEST_CASE(math_exp, log10_special_cond);
}
