/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 *
 * HEADER:
 *    - math.h
 *
 * TESTED:
 *    - sin(), cos(), tan()
 *    - asin(), acos(), atan(), atan2()
 *    - sinh(), cosh(), tanh()
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
#include <time.h>

#include "common.h"
#include <unity_fixture.h>


TEST_GROUP(math_trig);
TEST_GROUP(math_hyper);


TEST_SETUP(math_trig)
{
}


TEST_TEAR_DOWN(math_trig)
{
}


TEST(math_trig, sin_basic)
{
	int i, iters = 50 * ITER_FACTOR;
	double acceptLoss = 35.0;
	double max = M_PI_2;
	double min = -max;
	double step = max / iters - min / iters;
	double xstart = min;
	double u, x, y, f, g, digLost;

	for (i = 0; i < iters; i++) {
		u = (double)rand() / (double)RAND_MAX;
		x = step * u + xstart;
		/* Using sinus trigonometric identity */
		y = sin(x / 3.0);
		f = (3.0 * y) - (4.0 * y * y * y);
		g = sin(x);

		digLost = test_checkResult(f, g);
		test_check_digLost("sin", x, digLost, acceptLoss);

		xstart += step;
	}

	max = 1024.0;
	min = -max;
	step = max / iters - min / iters;
	xstart = min;

	for (i = 0; i < iters; i++) {
		u = (double)rand() / (double)RAND_MAX;
		x = step * u + xstart;
		y = sin(x / 3.0);
		f = (3.0 * y) - (4.0 * y * y * y);
		g = sin(x);

		digLost = test_checkResult(f, g);
		test_check_digLost("sin", x, digLost, acceptLoss);

		xstart += step;
	}
}


TEST(math_trig, sin_normalize_special_case)
{
	double acceptLoss = 60.0;
	double digLost, x, f, g;

	x = M_PI * test_common.maxPowTwoPi;
	f = 2.0 * sin(x) * cos(x);
	g = sin(2.0 * x);

	TEST_ASSERT_DOUBLE_IS_NOT_NAN(f);
	TEST_ASSERT_DOUBLE_IS_NOT_NAN(g);

	digLost = test_checkResult(f, g);
	test_check_digLost("sin", x, digLost, acceptLoss);
}


TEST(math_trig, sin_special_cond)
{
	TEST_ASSERT_DOUBLE_IS_NAN(sin(NAN));
	TEST_ASSERT_DOUBLE_IS_NAN(sin(-NAN));

	TEST_ASSERT_EQUAL_DOUBLE(0.0, sin(0.0));
	TEST_ASSERT_EQUAL_DOUBLE(-0.0, sin(-0.0));

	errno = 0;
	TEST_ASSERT_DOUBLE_IS_NAN(sin(INFINITY));
	// TEST_ASSERT_EQUAL_INT(EDOM, errno);

	errno = 0;
	TEST_ASSERT_DOUBLE_IS_NAN(sin(-INFINITY));
	// TEST_ASSERT_EQUAL_INT(EDOM, errno);
}


TEST(math_trig, cos_basic)
{
	int i, iters = 50 * ITER_FACTOR;
	double acceptLoss = 35.0;
	double max = M_PI;
	double min = 0.0;
	double step = max / iters - min / iters;
	double xstart = min;
	double u, x, y, f, g, digLost;

	for (i = 0; i < iters; i++) {
		u = (double)rand() / (double)RAND_MAX;
		x = step * u + xstart;
		/* Using cosinus trigonometric identity */
		y = cos(x / 2.0);
		f = (2.0 * y * y) - 1.0;
		g = cos(x);

		digLost = test_checkResult(f, g);
		test_check_digLost("cos", x, digLost, acceptLoss);

		xstart += step;
	}

	max = 1024.0;
	min = -max;
	step = max / iters - min / iters;
	xstart = min;

	for (i = 0; i < iters; i++) {
		u = (double)rand() / (double)RAND_MAX;
		x = step * u + xstart;
		y = cos(x / 2.0);
		f = (2.0 * y * y) - 1.0;
		g = cos(x);

		digLost = test_checkResult(f, g);
		test_check_digLost("cos", x, digLost, acceptLoss);

		xstart += step;
	}
}


TEST(math_trig, cos_normalize_special_case)
{
	double acceptLoss = 60.0;
	double digLost, x, y, f, g;

	x = M_PI * pow(2.0, test_common.maxPowTwoPi);
	y = cos(x / 2.0);
	f = cos(2.0 * y * y) - 1.0;
	g = cos(x);

	TEST_ASSERT_DOUBLE_IS_NOT_NAN(f);
	TEST_ASSERT_DOUBLE_IS_NOT_NAN(g);

	digLost = test_checkResult(f, g);
	test_check_digLost("cos", x, digLost, acceptLoss);
}


TEST(math_trig, cos_special_cond)
{
	TEST_ASSERT_DOUBLE_IS_NAN(cos(NAN));
	TEST_ASSERT_DOUBLE_IS_NAN(cos(-NAN));

	TEST_ASSERT_EQUAL_DOUBLE(1.0, cos(0.0));
	TEST_ASSERT_EQUAL_DOUBLE(1.0, cos(-0.0));

	errno = 0;
	TEST_ASSERT_DOUBLE_IS_NAN(cos(INFINITY));
	// TEST_ASSERT_EQUAL_INT(EDOM, errno);

	errno = 0;
	TEST_ASSERT_DOUBLE_IS_NAN(cos(-INFINITY));
	// TEST_ASSERT_EQUAL_INT(EDOM, errno);
}


TEST(math_trig, tan_basic)
{
	int i, ferr, gerr, iters = 50 * ITER_FACTOR;
	double acceptLoss = 35.0;
	double max = M_PI_2 - 0.01;
	double min = -max;
	double step = max / iters - min / iters;
	double xstart = min;
	double u, x, y, f, g, digLost;

	for (i = 0; i < iters; i++) {
		u = (double)rand() / (double)RAND_MAX;
		x = step * u + xstart;
		/* Using tangens trigonometric identity */
		errno = 0;
		y = tan(x / 2.0);
		ferr = errno;
		f = (2.0 * y) / (1.0 - y * y);
		errno = 0;
		g = tan(x);
		gerr = errno;

		if (gerr != EDOM && ferr != EDOM) {
			digLost = test_checkResult(f, g);
			test_check_digLost("tan", x, digLost, acceptLoss);
		}

		xstart += step;
	}

	max = 1024.0;
	min = -max;
	step = max / iters - min / iters;
	xstart = min;

	for (i = 0; i < iters; i++) {
		u = (double)rand() / (double)RAND_MAX;
		x = step * u + xstart;
		y = tan(x / 2.0);
		f = (2.0 * y) / (1.0 - y * y);
		g = tan(x);

		digLost = test_checkResult(f, g);
		test_check_digLost("tan", x, digLost, acceptLoss);

		xstart += step;
	}
}


TEST(math_trig, tan_normalize_special_case)
{
	double acceptLoss = 60.0;
	double digLost, x, y, f, g;

	x = M_PI * pow(2.0, test_common.maxPowTwoPi);
	y = tan(x / 2.0);
	f = (2.0 * y) / (1.0 - y * y);
	g = tan(x);

	TEST_ASSERT_DOUBLE_IS_NOT_NAN(f);
	TEST_ASSERT_DOUBLE_IS_NOT_NAN(g);

	digLost = test_checkResult(f, g);
	test_check_digLost("tan", x, digLost, acceptLoss);
}


TEST(math_trig, tan_special_cond)
{
	TEST_ASSERT_DOUBLE_IS_NAN(tan(NAN));
	TEST_ASSERT_DOUBLE_IS_NAN(tan(-NAN));

	TEST_ASSERT_EQUAL_DOUBLE(0.0, tan(0.0));
	TEST_ASSERT_EQUAL_DOUBLE(-0.0, tan(-0.0));

	errno = 0;
	TEST_ASSERT_DOUBLE_IS_NAN(tan(INFINITY));
	TEST_ASSERT_EQUAL_INT(EDOM, errno);

	errno = 0;
	TEST_ASSERT_DOUBLE_IS_NAN(tan(-INFINITY));
	TEST_ASSERT_EQUAL_INT(EDOM, errno);
}


TEST(math_trig, asin_basic)
{
	int i, iters = 50 * ITER_FACTOR;
	double acceptLoss = 35.0;
	double max = 1.0;
	double min = -max;
	double step = max / iters - min / iters;
	double xstart = min;
	double u, x, f, g, digLost;

	for (i = 0; i < iters; i++) {
		u = (double)rand() / (double)RAND_MAX;
		x = step * u + xstart;
		f = sin(asin(x));
		g = x;

		digLost = test_checkResult(f, g);
		test_check_digLost("asin", x, digLost, acceptLoss);

		xstart += step;
	}
}


TEST(math_trig, asin_special_cond)
{
	TEST_ASSERT_DOUBLE_IS_NAN(asin(NAN));
	TEST_ASSERT_DOUBLE_IS_NAN(asin(-NAN));

	errno = 0;
	TEST_ASSERT_DOUBLE_IS_NAN(asin(1.03));
	TEST_ASSERT_EQUAL_INT(EDOM, errno);

	errno = 0;
	TEST_ASSERT_DOUBLE_IS_NAN(asin(-1.03));
	TEST_ASSERT_EQUAL_INT(EDOM, errno);

	// TEST_ASSERT_EQUAL_DOUBLE(0.0, asin(0.0));
	// TEST_ASSERT_EQUAL_DOUBLE(-0.0, asin(-0.0));

	errno = 0;
	TEST_ASSERT_DOUBLE_IS_NAN(asin(INFINITY));
	TEST_ASSERT_EQUAL_INT(EDOM, errno);

	errno = 0;
	TEST_ASSERT_DOUBLE_IS_NAN(asin(-INFINITY));
	TEST_ASSERT_EQUAL_INT(EDOM, errno);
}


TEST(math_trig, acos_basic)
{
	int i, iters = 50 * ITER_FACTOR;
	double acceptLoss = 35.0;
	double max = 1.0;
	double min = -max;
	double step = max / iters - min / iters;
	double xstart = min;
	double u, x, f, g, digLost;

	for (i = 0; i < iters; i++) {
		u = (double)rand() / (double)RAND_MAX;
		x = step * u + xstart;
		f = cos(acos(x));
		g = x;

		digLost = test_checkResult(f, g);
		test_check_digLost("acos", x, digLost, acceptLoss);

		xstart += step;
	}
}


TEST(math_trig, acos_special_cond)
{
	TEST_ASSERT_DOUBLE_IS_NAN(acos(NAN));
	TEST_ASSERT_DOUBLE_IS_NAN(acos(-NAN));

	errno = 0;
	TEST_ASSERT_DOUBLE_IS_NAN(acos(1.03));
	TEST_ASSERT_EQUAL_INT(EDOM, errno);

	errno = 0;
	TEST_ASSERT_DOUBLE_IS_NAN(acos(-1.03));
	TEST_ASSERT_EQUAL_INT(EDOM, errno);

	TEST_ASSERT_EQUAL_DOUBLE(0.0, acos(1.0));

	errno = 0;
	TEST_ASSERT_DOUBLE_IS_NAN(acos(INFINITY));
	TEST_ASSERT_EQUAL_INT(EDOM, errno);

	errno = 0;
	TEST_ASSERT_DOUBLE_IS_NAN(acos(-INFINITY));
	TEST_ASSERT_EQUAL_INT(EDOM, errno);
}


TEST(math_trig, atan_basic)
{
	int i, iters = 50 * ITER_FACTOR;
	double acceptLoss = 35.0;
	double max = 25.0;
	double min = 0.0;
	double x, f, g, digLost;

	for (i = 0; i < iters; i++) {
		x = test_getRandomLog(min, max);
		if (i % 2 == 0) {
			x = -x;
		}

		f = tan(atan(x));
		g = x;

		digLost = test_checkResult(f, g);
		test_check_digLost("atan", x, digLost, acceptLoss);
	}
}


TEST(math_trig, atan_special_cond)
{
	TEST_ASSERT_DOUBLE_IS_NAN(atan(NAN));
	TEST_ASSERT_DOUBLE_IS_NAN(atan(-NAN));

	TEST_ASSERT_EQUAL_DOUBLE(0.0, atan(0.0));
	TEST_ASSERT_EQUAL_DOUBLE(-0.0, atan(-0.0));

	TEST_ASSERT_EQUAL_DOUBLE(M_PI_2, atan(INFINITY));
	TEST_ASSERT_EQUAL_DOUBLE(-M_PI_2, atan(-INFINITY));
}


TEST(math_trig, atan2_basic)
{
	int i, iters = 50 * ITER_FACTOR;
	double acceptLoss = 35.0;
	double max = 25.0;
	double min = 0.01;
	double at, x, y, f, g, digLost;

	for (i = 0; i < iters; i++) {
		x = test_getRandomLog(min, max);
		y = test_getRandomLog(min, max);
		if (i % 2 == 0) {
			x = -x;
		}

		at = atan(y / x);
		if (x > 0) {
			f = at;
		}
		else if (y < 0) {
			f = at - M_PI;
		}
		else {
			f = at + M_PI;
		}

		g = atan2(y, x);

		digLost = test_checkResult(f, g);
		test_check_digLost2("atan2", y, x, digLost, acceptLoss);
	}
}


TEST(math_trig, atan2_special_cond)
{
	/* Initialize x and y to random finite value other than 0.0 */
	double x = 1.2;
	double y = 1.2;

	TEST_ASSERT_DOUBLE_IS_NAN(atan2(NAN, x));
	TEST_ASSERT_DOUBLE_IS_NAN(atan2(-NAN, x));

	// TEST_ASSERT_DOUBLE_IS_NAN(atan2(y, NAN));
	// TEST_ASSERT_DOUBLE_IS_NAN(atan2(y, -NAN));

	TEST_ASSERT_DOUBLE_IS_NAN(atan2(NAN, NAN));
	TEST_ASSERT_DOUBLE_IS_NAN(atan2(-NAN, -NAN));

	// TEST_ASSERT_EQUAL_DOUBLE(M_PI, atan2(0.0, -x));
	// TEST_ASSERT_EQUAL_DOUBLE(-M_PI, atan2(-0.0, -x));

	TEST_ASSERT_EQUAL_DOUBLE(0.0, atan2(0.0, x));
	TEST_ASSERT_EQUAL_DOUBLE(-0.0, atan2(-0.0, x));

	// TEST_ASSERT_EQUAL_DOUBLE(-M_PI_2, atan2(-y, 0.0));
	// TEST_ASSERT_EQUAL_DOUBLE(-M_PI_2, atan2(-y, -0.0));

	// TEST_ASSERT_EQUAL_DOUBLE(M_PI_2, atan2(y, 0.0));
	// TEST_ASSERT_EQUAL_DOUBLE(M_PI_2, atan2(y, -0.0));

	// errno = 0;
	// TEST_ASSERT_NOT_EQUAL_DOUBLE(INFINITY, atan2(y, 0.0));
	// TEST_ASSERT_NOT_EQUAL_INT(ERANGE, errno);

	// errno = 0;
	// TEST_ASSERT_EQUAL_DOUBLE(M_PI, atan2(0.0, -0.0));
	// TEST_ASSERT_NOT_EQUAL_INT(EDOM, errno);

	// errno = 0;
	// TEST_ASSERT_EQUAL_DOUBLE(-M_PI, atan2(-0.0, -0.0));
	// TEST_ASSERT_NOT_EQUAL_INT(EDOM, errno);

	// errno = 0;
	// TEST_ASSERT_EQUAL_DOUBLE(0.0, atan2(0.0, 0.0));
	// TEST_ASSERT_NOT_EQUAL_INT(EDOM, errno);

	// errno = 0;
	// TEST_ASSERT_EQUAL_DOUBLE(-0.0, atan2(-0.0, 0.0));
	// TEST_ASSERT_NOT_EQUAL_INT(EDOM, errno);

	TEST_ASSERT_EQUAL_DOUBLE(0.0, atan2(0.0, x));
	TEST_ASSERT_EQUAL_DOUBLE(-0.0, atan2(-0.0, x));

	TEST_ASSERT_EQUAL_DOUBLE(0.0, atan2(0.0, x));
	TEST_ASSERT_EQUAL_DOUBLE(-0.0, atan2(-0.0, x));

	TEST_ASSERT_EQUAL_DOUBLE(M_PI, atan2(y, -INFINITY));
	TEST_ASSERT_EQUAL_DOUBLE(-M_PI, atan2(-y, -INFINITY));

	TEST_ASSERT_EQUAL_DOUBLE(0.0, atan2(y, INFINITY));
	TEST_ASSERT_EQUAL_DOUBLE(-0.0, atan2(-y, INFINITY));

	TEST_ASSERT_EQUAL_DOUBLE(M_PI_2, atan2(INFINITY, x));
	TEST_ASSERT_EQUAL_DOUBLE(-M_PI_2, atan2(-INFINITY, x));

	// TEST_ASSERT_EQUAL_DOUBLE(M_PI_4 * 3, atan2(INFINITY, -INFINITY));
	// TEST_ASSERT_EQUAL_DOUBLE(-M_PI_4 * 3, atan2(-INFINITY, -INFINITY));

	// TEST_ASSERT_EQUAL_DOUBLE(M_PI_4, atan2(INFINITY, INFINITY));
	// TEST_ASSERT_EQUAL_DOUBLE(-M_PI_4, atan2(-INFINITY, INFINITY));
}


TEST_SETUP(math_hyper)
{
}


TEST_TEAR_DOWN(math_hyper)
{
}

TEST(math_hyper, sinh_basic)
{
	int i, iters = 50 * ITER_FACTOR;
	double acceptLoss = 50.0;
	double max = log(DBL_MAX);
	double min = 1.0e-10;
	double x, y, f, g, digLost;

	for (i = 0; i < iters; i++) {
		x = test_getRandomLog(min, max);
		if (i % 2 == 0) {
			x = -x;
		}
		/* Using sinus trigonometric identity, which is true for hyperbolic sinus too */
		y = sinh(x / 3.0);
		f = (3.0 * y) + (4.0 * y * y * y);
		g = sinh(x);

		digLost = test_checkResult(f, g);
		test_check_digLost("sinh", x, digLost, acceptLoss);
	}
}


TEST(math_hyper, sinh_special_cond)
{
	// TEST_ASSERT_DOUBLE_IS_NAN(sinh(NAN));
	// TEST_ASSERT_DOUBLE_IS_NAN(sinh(-NAN));

	TEST_ASSERT_EQUAL_DOUBLE(0.0, sinh(0.0));
	TEST_ASSERT_EQUAL_DOUBLE(-0.0, sinh(-0.0));

	TEST_ASSERT_EQUAL_DOUBLE(INFINITY, sinh(INFINITY));
	TEST_ASSERT_EQUAL_DOUBLE(-INFINITY, sinh(-INFINITY));

	errno = 0;
	TEST_ASSERT_EQUAL_DOUBLE(INFINITY, sinh(log(DBL_MAX) * 1.1));
	// TEST_ASSERT_EQUAL_INT(ERANGE, errno);
}


TEST(math_hyper, cosh_basic)
{
	int i, iters = 50 * ITER_FACTOR;
	double acceptLoss = 50.0;
	double max = log(DBL_MAX);
	double min = 1.0e-10;
	double x, y, f, g, digLost;

	for (i = 0; i < iters; i++) {
		x = test_getRandomLog(min, max);
		if (i % 2 == 0) {
			x = -x;
		}
		/* Using cosinus trigonometric identity, which is true for hyperbolic cosinus too */
		y = cosh(x / 2.0);
		f = (2.0 * y * y) - 1.0;
		g = cosh(x);

		digLost = test_checkResult(f, g);
		test_check_digLost("cosh", x, digLost, acceptLoss);
	}
}


TEST(math_hyper, cosh_special_cond)
{
	// TEST_ASSERT_DOUBLE_IS_NAN(cosh(NAN));
	// TEST_ASSERT_DOUBLE_IS_NAN(cosh(-NAN));

	TEST_ASSERT_EQUAL_DOUBLE(1.0, cosh(0.0));
	TEST_ASSERT_EQUAL_DOUBLE(1.0, cosh(-0.0));

	TEST_ASSERT_EQUAL_DOUBLE(INFINITY, cosh(INFINITY));
	TEST_ASSERT_EQUAL_DOUBLE(INFINITY, cosh(-INFINITY));
}


TEST(math_hyper, tanh_basic)
{
	int i, iters = 50 * ITER_FACTOR;
	double acceptLoss = 50.0;
	double max = log(DBL_MAX);
	double min = 1.0e-10;
	double x, y, f, g, digLost;

	for (i = 0; i < iters; i++) {
		x = test_getRandomLog(min, max);
		if (i % 2 == 0) {
			x = -x;
		}
		/* Using tangens trigonometric identity, which is true for hyperbolic tangens too */
		y = tanh(x / 2.0);
		f = (2.0 * y) / (1.0 + y * y);
		g = tanh(x);

		digLost = test_checkResult(f, g);
		test_check_digLost("tanh", x, digLost, acceptLoss);
	}
}


TEST(math_hyper, tanh_special_cond)
{
	// TEST_ASSERT_DOUBLE_IS_NAN(tanh(NAN));
	// TEST_ASSERT_DOUBLE_IS_NAN(tanh(-NAN));

	TEST_ASSERT_EQUAL_DOUBLE(0.0, tanh(0.0));
	TEST_ASSERT_EQUAL_DOUBLE(-0.0, tanh(-0.0));

	// TEST_ASSERT_EQUAL_DOUBLE(1.0, tanh(INFINITY));
	// TEST_ASSERT_EQUAL_DOUBLE(-1.0, tanh(-INFINITY));
}


TEST_GROUP_RUNNER(math_trig)
{
	test_setup();

	RUN_TEST_CASE(math_trig, sin_basic);
	RUN_TEST_CASE(math_trig, sin_normalize_special_case);
	RUN_TEST_CASE(math_trig, sin_special_cond);

	RUN_TEST_CASE(math_trig, cos_basic);
	RUN_TEST_CASE(math_trig, cos_normalize_special_case);
	RUN_TEST_CASE(math_trig, cos_special_cond);

	RUN_TEST_CASE(math_trig, tan_basic);
	RUN_TEST_CASE(math_trig, tan_normalize_special_case);
	RUN_TEST_CASE(math_trig, tan_special_cond);

	RUN_TEST_CASE(math_trig, asin_basic);
	RUN_TEST_CASE(math_trig, asin_special_cond);

	RUN_TEST_CASE(math_trig, acos_basic);
	RUN_TEST_CASE(math_trig, acos_special_cond);

	RUN_TEST_CASE(math_trig, atan_basic);
	RUN_TEST_CASE(math_trig, atan_special_cond);

	RUN_TEST_CASE(math_trig, atan2_basic);
	RUN_TEST_CASE(math_trig, atan2_special_cond);
}


TEST_GROUP_RUNNER(math_hyper)
{
	test_setup();

	RUN_TEST_CASE(math_hyper, sinh_basic);
	RUN_TEST_CASE(math_hyper, sinh_special_cond);

	RUN_TEST_CASE(math_hyper, cosh_basic);
	RUN_TEST_CASE(math_hyper, cosh_special_cond);

	RUN_TEST_CASE(math_hyper, tanh_basic);
	RUN_TEST_CASE(math_hyper, tanh_special_cond);
}
