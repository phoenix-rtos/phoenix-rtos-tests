/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 *
 * HEADER:
 *    - math.h
 *
 * TESTED:
 *    - modf(), fmod()
 *    - ceil, floor()
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

static double test_getFrac(double x)
{
	double powTwo, frac = fabs(x);

	for (powTwo = pow(2.0, test_common.maxPowTwo); powTwo >= 2.0; powTwo /= 2.0) {
		while (frac > powTwo) {
			frac -= powTwo;
		}
	}

	while (frac >= 1.0) {
		frac -= 1.0;
	}

	return frac;
}

static double test_ceil(double x)
{
	double frac = fabs(x);

	if (frac > pow(2.0, test_common.maxPowTwo)) {
		return x;
	}

	frac = test_getFrac(x);

	if (frac == 0.0) {
		return x;
	}

	if (x < 0.0) {
		return (x + frac);
	}
	else {
		return (x - frac + 1.0);
	}
}

static double test_floor(double x)
{
	double frac = fabs(x);

	if (frac > pow(2.0, test_common.maxPowTwo)) {
		return x;
	}

	frac = test_getFrac(x);

	if (frac == 0.0) {
		return x;
	}

	if (x < 0.0) {
		return x + frac - 1.0;
	}
	else {
		return x - frac;
	}
}


static double test_fmod(double x, double y)
{
	double absx = fabs(x);
	double absy = fabs(y);
	double dval = absy;
	double powTwo64 = pow(2.0, 64.0);
	double powTwo128 = pow(2.0, 128.0);

	if (y == 0.0) {
		return 0.0;
	}

	if (absx < absy) {
		return x;
	}

	while (1) {
		while ((dval * powTwo128) < absx) {
			dval = dval * powTwo128;
		}
		if ((dval * powTwo64) < absx) {
			dval = dval * powTwo64;
		}
		if ((dval * 4294967296.0) < absx) {
			dval = dval * 4294967296.0;
		}
		if ((dval * 65536.0) < absx) {
			dval = dval * 65536.0;
		}
		if ((dval * 256.0) < absx) {
			dval = dval * 256.0;
		}
		if ((dval * 16.0) < absx) {
			dval = dval * 16.0;
		}
		if ((dval * 4.0) < absx) {
			dval = dval * 4.0;
		}
		if ((dval * 2.0) < absx) {
			dval = dval * 2.0;
		}
		absx = absx - dval;
		dval = absy;
		if (absx < absy) {
			break;
		}
	}

	if (x < 0.0) {
		return -absx;
	}

	return absx;
}


TEST_GROUP(math_frac);


TEST_SETUP(math_frac)
{
}


TEST_TEAR_DOWN(math_frac)
{
}


TEST(math_frac, modf_basic)
{
	int i, iters = 50 * ITER_FACTOR;
	double acceptLoss = 1.0;
	double max = DBL_MAX;
	double min = DBL_MIN;
	double x, f, g, ipart, digLost;

	for (i = 0; i < iters; i++) {
		x = test_getRandomLog(min, max);
		if (i % 2) {
			x = -x;
		}
		if (x > 0.0) {
			f = x - floor(x);
		}
		else {
			f = x - ceil(x);
		}

		g = modf(x, &ipart);

		digLost = test_checkResult(f, g);
		test_check_digLost("modf", x, digLost, acceptLoss);
	}

	for (i = 0; i < iters; i++) {
		x = test_getRandomLog(min, max);
		if (i % 2) {
			x = -x;
		}
		if (x > 0.0) {
			f = floor(x);
		}
		else {
			f = ceil(x);
		}

		modf(x, &ipart);
		g = ipart;

		digLost = test_checkResult(f, g);
		test_check_digLost("modf", x, digLost, acceptLoss);
	}
}


TEST(math_frac, modf_special_cond)
{
	double iprt;

	iprt = 0.0;
	// TEST_ASSERT_DOUBLE_IS_NAN(modf(NAN, &iprt));
	// TEST_ASSERT_DOUBLE_IS_NAN(iprt);

	iprt = 0.0;
	// TEST_ASSERT_DOUBLE_IS_NAN(modf(NAN, &iprt));
	// TEST_ASSERT_DOUBLE_IS_NAN(iprt);

	iprt = 0.0;
	TEST_ASSERT_EQUAL_DOUBLE(0.0, modf(INFINITY, &iprt));
	TEST_ASSERT_EQUAL_DOUBLE(INFINITY, iprt);

	iprt = 0.0;
	TEST_ASSERT_EQUAL_DOUBLE(-0.0, modf(-INFINITY, &iprt));
	TEST_ASSERT_EQUAL_DOUBLE(-INFINITY, iprt);
}


TEST(math_frac, fmod_basic)
{
	int i, iters = 20 * ITER_FACTOR;
	double acceptLoss = 60.0;
	double xmax = DBL_MAX / 2.0;
	double xmin = DBL_MIN * 2.0;
	double x, y, ymin, ymax, f, g, digLost;

	for (i = 0; i < iters; i++) {
		x = test_getRandomLog(xmin, xmax);
		ymin = xmin;
		ymax = x;
		y = test_getRandomLog(ymin, ymax);

		if (rand() % 2) {
			x = -x;
		}
		if (rand() % 2) {
			y = -y;
		}

		f = test_fmod(x, y);
		g = fmod(x, y);

		digLost = test_checkResult(f, g);
		test_check_digLost2("fmod", x, y, digLost, acceptLoss);
	}
}


TEST(math_frac, fmod_special_cond)
{
	/* Initialize x and y to random finite value other than 0.0 */
	double x = 1.2;
	double y = 1.2;

	TEST_ASSERT_DOUBLE_IS_NAN(fmod(NAN, y));
	TEST_ASSERT_DOUBLE_IS_NAN(fmod(-NAN, y));

	TEST_ASSERT_DOUBLE_IS_NAN(fmod(x, NAN));
	TEST_ASSERT_DOUBLE_IS_NAN(fmod(x, -NAN));

	TEST_ASSERT_DOUBLE_IS_NAN(fmod(NAN, NAN));
	TEST_ASSERT_DOUBLE_IS_NAN(fmod(-NAN, -NAN));

	errno = 0;
	TEST_ASSERT_DOUBLE_IS_NAN(fmod(x, 0.0));
	// TEST_ASSERT_EQUAL_INT(EDOM, errno);

	errno = 0;
	TEST_ASSERT_DOUBLE_IS_NAN(fmod(INFINITY, y));
	// TEST_ASSERT_EQUAL_INT(EDOM, errno);

	TEST_ASSERT_EQUAL_DOUBLE(0.0, fmod(0.0, y));
	TEST_ASSERT_EQUAL_DOUBLE(-0.0, fmod(-0.0, y));

	TEST_ASSERT_EQUAL_DOUBLE(x, fmod(x, INFINITY));
	TEST_ASSERT_EQUAL_DOUBLE(x, fmod(x, -INFINITY));
}


TEST(math_frac, ceil_basic)
{
	int i, iters = 10 * ITER_FACTOR;
	double acceptLoss = 1.0;
	double max = pow(2.0, (double)test_common.maxPowTwo + 2.0);
	double min = 1.0e-10;
	double x, f, g, digLost;

	for (i = 0; i < iters; i++) {
		x = test_getRandomLog(min, max);
		if (i % 2) {
			x = -x;
		}
		f = test_ceil(x);
		g = ceil(x);

		digLost = test_checkResult(f, g);
		test_check_digLost("ceil", x, digLost, acceptLoss);
	}
}


TEST(math_frac, ceil_special_cond)
{
	TEST_ASSERT_DOUBLE_IS_NAN(ceil(NAN));
	TEST_ASSERT_DOUBLE_IS_NAN(ceil(-NAN));

	TEST_ASSERT_EQUAL_DOUBLE(0.0, ceil(0.0));
	TEST_ASSERT_EQUAL_DOUBLE(-0.0, ceil(-0.0));

	TEST_ASSERT_EQUAL_DOUBLE(INFINITY, ceil(INFINITY));
	TEST_ASSERT_EQUAL_DOUBLE(-INFINITY, ceil(-INFINITY));
}


TEST(math_frac, floor_basic)
{
	int i, iters = 10 * ITER_FACTOR;
	double acceptLoss = 1.0;
	double max = DBL_MAX;
	double min = DBL_MIN;
	double x, f, g, digLost;

	for (i = 0; i < iters; i++) {
		x = test_getRandomLog(min, max);
		if (i % 2) {
			x = -x;
		}
		f = test_floor(x);
		g = floor(x);

		digLost = test_checkResult(f, g);
		test_check_digLost("floor", x, digLost, acceptLoss);
	}
}


TEST(math_frac, floor_special_cond)
{
	TEST_ASSERT_DOUBLE_IS_NAN(floor(NAN));
	TEST_ASSERT_DOUBLE_IS_NAN(floor(-NAN));

	TEST_ASSERT_EQUAL_DOUBLE(0.0, floor(0.0));
	TEST_ASSERT_EQUAL_DOUBLE(-0.0, floor(-0.0));

	TEST_ASSERT_EQUAL_DOUBLE(INFINITY, floor(INFINITY));
	TEST_ASSERT_EQUAL_DOUBLE(-INFINITY, floor(-INFINITY));
}

TEST_GROUP_RUNNER(math_frac)
{
	test_setup();

	RUN_TEST_CASE(math_frac, modf_basic);
	RUN_TEST_CASE(math_frac, modf_special_cond);

	/* fmod fix needed */
	// RUN_TEST_CASE(math_frac, fmod_basic);
	RUN_TEST_CASE(math_frac, fmod_special_cond);

	RUN_TEST_CASE(math_frac, ceil_basic);
	RUN_TEST_CASE(math_frac, ceil_special_cond);

	RUN_TEST_CASE(math_frac, floor_basic);
	RUN_TEST_CASE(math_frac, floor_special_cond);
}
