/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 *
 * HEADER:
 *    - math.h
 *
 * TESTED:
 *    - tan(), atan(), tanh(), atan2()
 *    - sin(), asin(), sinh()
 *    - cos(), acos(), cosh()
 *
 * Copyright 2023 Phoenix Systems
 * Author: Mateusz Bloch
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <math.h>
#include <limits.h>
#include <float.h>
#include <errno.h>
#include <unity_fixture.h>


#define M_SQRT3 1.73205080756887719318
#define DELTA   1e-05


static double dblSubnormal(void)
{
	union {
		double doubleValue;
		uint64_t integerValue;
	} representation;
	representation.integerValue = 0x000fffffffffffffULL;
	return representation.doubleValue;
}


TEST_GROUP(math_trig);
TEST_GROUP(math_hyper);


TEST_SETUP(math_trig)
{
}


TEST_TEAR_DOWN(math_trig)
{
}


TEST(math_trig, tan_basic)
{
	int i;
	int iterates = 2000;
	double start = -DBL_MAX;
	double end = DBL_MAX;
	double step = (end - start) / (iterates - 1);
	double x, y;

	for (i = 0; i <= iterates; i++) {
		x = start + i * step;
		y = tan(x / 2.0);
		TEST_ASSERT_EQUAL_DOUBLE(tan(x), (2.0 * y) / (1.0 + y * y));
	}
}


TEST(math_trig, tan_huge_x)
{
	// TEST_ASSERT_EQUAL_DOUBLE(-0.37362445398759902560, tan(1.0e+6)); // Sprawdź do którego bitu ma być uwzględnione
	// TEST_ASSERT_EQUAL_DOUBLE(0.37362445398759902560, tan(-1.0e+6)); // Sprawdź do którego bitu ma być uwzględnione

	TEST_ASSERT_DOUBLE_WITHIN(DELTA, 1e-06, tan(1.0e-6));
	TEST_ASSERT_DOUBLE_WITHIN(DELTA, -1e-06, tan(-1.0e-6));

	TEST_ASSERT_EQUAL_DOUBLE(DBL_MIN, tan(DBL_MIN));
}


TEST(math_trig, tan_special_cond)
{
	TEST_ASSERT_DOUBLE_IS_NAN(tan(NAN));

	TEST_ASSERT_EQUAL_DOUBLE(0, tan(0.0));
	TEST_ASSERT_EQUAL_DOUBLE(0, tan(-0.0));

	TEST_ASSERT_EQUAL_DOUBLE(dblSubnormal(), tan(dblSubnormal()));

	errno = 0;
	TEST_ASSERT_DOUBLE_IS_NAN(tan(INFINITY));
	TEST_ASSERT_EQUAL_INT(EDOM, errno);

	errno = 0;
	TEST_ASSERT_DOUBLE_IS_NAN(tan(-INFINITY));
	TEST_ASSERT_EQUAL_INT(EDOM, errno);
}


TEST(math_trig, atan_basic)
{
	int i;
	int iterates = 2000;
	double start = -1.0;
	double end = 1.0;
	double step = (end - start) / (iterates - 1);
	double x;

	for (i = 0; i <= iterates; i++) {
		x = start + i * step;
		TEST_ASSERT_EQUAL_DOUBLE(x, tan(atan(x)));
	}
}


TEST(math_trig, atan_huge_x)
{
	TEST_ASSERT_DOUBLE_WITHIN(DELTA, M_PI / 2, atan(1.0e+6));
	TEST_ASSERT_DOUBLE_WITHIN(DELTA, -M_PI / 2, atan(-1.0e+6));

	TEST_ASSERT_EQUAL_DOUBLE(1e-10, atan(1.0e-10));
	TEST_ASSERT_EQUAL_DOUBLE(-1e-10, atan(-1.0e-10));

	TEST_ASSERT_EQUAL_DOUBLE(M_PI_2, atan(DBL_MAX));
	TEST_ASSERT_EQUAL_DOUBLE(DBL_MIN, atan(DBL_MIN));
}


TEST(math_trig, atan_special_cond)
{
	TEST_ASSERT_DOUBLE_IS_NAN(atan(NAN));

	TEST_ASSERT_EQUAL_DOUBLE(0, atan(0.0));
	TEST_ASSERT_EQUAL_DOUBLE(0, atan(-0.0));

	TEST_ASSERT_EQUAL_DOUBLE(M_PI_2, atan(INFINITY));
	TEST_ASSERT_EQUAL_DOUBLE(-M_PI_2, atan(-INFINITY));

	TEST_ASSERT_EQUAL_DOUBLE(dblSubnormal(), atan(dblSubnormal()));
}


TEST(math_trig, sin_basic)
{
	int i;
	int iterates = 2000;
	double start = -DBL_MAX;
	double end = DBL_MAX;
	double step = (end - start) / (iterates - 1);
	double x, y;

	for (i = 0; i <= iterates; i++) {
		x = start + i * step;
		y = sinh(x / 3.0);
		TEST_ASSERT_EQUAL_DOUBLE(sinh(x), (3.0 * y) + (4.0 * y * y * y));
	}
}


TEST(math_trig, sin_huge_x)
{
	// TEST_ASSERT_EQUAL_DOUBLE(-0.34999350217129293616, sin(1.0e+6)); // Sprawdź do którego bitu ma być uwzględnione
	// TEST_ASSERT_EQUAL_DOUBLE(0.34999350217129293616, sin(-1.0e+6)); // Sprawdź do którego bitu ma być uwzględnione

	TEST_ASSERT_DOUBLE_WITHIN(DELTA, 1e-06, sin(1.0e-6));
	TEST_ASSERT_DOUBLE_WITHIN(DELTA, -1e-06, sin(-1.0e-6));

	TEST_ASSERT_EQUAL_DOUBLE(DBL_MIN, sin(DBL_MIN));
}


TEST(math_trig, sin_special_cond)
{
	TEST_ASSERT_DOUBLE_IS_NAN(sin(NAN));

	TEST_ASSERT_EQUAL_DOUBLE(0, sin(0.0));
	TEST_ASSERT_EQUAL_DOUBLE(0, sin(-0.0));

	TEST_ASSERT_EQUAL_DOUBLE(dblSubnormal(), sin(dblSubnormal()));

	errno = 0;
	TEST_ASSERT_DOUBLE_IS_NAN(sin(INFINITY));
	// TEST_ASSERT_EQUAL_INT(EDOM, errno);

	errno = 0;
	TEST_ASSERT_DOUBLE_IS_NAN(sin(-INFINITY));
	// TEST_ASSERT_EQUAL_INT(EDOM, errno);
}


TEST(math_trig, asin_basic)
{
	int i;
	int iterates = 2000;
	double start = -0.99;
	double end = 0.99;
	double step = (end - start) / (iterates - 1);
	double x;

	for (i = 0; i <= iterates; i++) {
		x = start + i * step;
		TEST_ASSERT_EQUAL_DOUBLE(x, sin(asin(x)));
	}
}


TEST(math_trig, asin_huge_x)
{
	errno = 0;
	TEST_ASSERT_DOUBLE_IS_NAN(asin(1.0e+6));
	TEST_ASSERT_EQUAL_INT(EDOM, errno);

	errno = 0;
	TEST_ASSERT_DOUBLE_IS_NAN(asin(-1.0e+6));
	TEST_ASSERT_EQUAL_INT(EDOM, errno);

	TEST_ASSERT_DOUBLE_WITHIN(DELTA, 1e-06, asin(1.0e-6));
	TEST_ASSERT_DOUBLE_WITHIN(DELTA, -1e-06, asin(-1.0e-6));

	errno = 0;
	TEST_ASSERT_DOUBLE_IS_NAN(asin(DBL_MAX));
	TEST_ASSERT_EQUAL_INT(EDOM, errno);

	// TEST_ASSERT_EQUAL_DOUBLE(DBL_MIN, asin(DBL_MIN));
}


TEST(math_trig, asin_special_cond)
{
	errno = 0;
	TEST_ASSERT_DOUBLE_IS_NAN(asin(M_PI));
	TEST_ASSERT_EQUAL_INT(EDOM, errno);

	errno = 0;
	TEST_ASSERT_DOUBLE_IS_NAN(asin(-M_PI));
	TEST_ASSERT_EQUAL_INT(EDOM, errno);

	TEST_ASSERT_DOUBLE_IS_NAN(asin(NAN));

	// TEST_ASSERT_EQUAL_DOUBLE(0, asin(0.0));
	// TEST_ASSERT_EQUAL_DOUBLE(0, asin(-0.0));

	// TEST_ASSERT_EQUAL_DOUBLE(dblSubnormal(), asin(dblSubnormal()));

	errno = 0;
	TEST_ASSERT_DOUBLE_IS_NAN(asin(INFINITY));
	TEST_ASSERT_EQUAL_INT(EDOM, errno);

	errno = 0;
	TEST_ASSERT_DOUBLE_IS_NAN(asin(-INFINITY));
	TEST_ASSERT_EQUAL_INT(EDOM, errno);
}


TEST(math_trig, cos_basic)
{
	int i;
	int iterates = 2000;
	double start = -DBL_MAX;
	double end = DBL_MAX;
	double step = (end - start) / (iterates - 1);
	double x, y;

	for (i = 0; i <= iterates; i++) {
		x = start + i * step;
		y = cos(x / 2.0);
		TEST_ASSERT_EQUAL_DOUBLE(cos(x), ((2.0 * y * y) - 1.0));
	}
}


TEST(math_trig, cos_huge_x)
{
	// TEST_ASSERT_EQUAL_DOUBLE(0.93675212753314474057, cos(1.0e+6)); // Sprawdź do którego bitu ma być uwzględnione
	// TEST_ASSERT_EQUAL_DOUBLE(0.93675212753314474057, cos(-1.0e+6)); // Sprawdź do którego bitu ma być uwzględnione

	TEST_ASSERT_DOUBLE_WITHIN(DELTA, 1, cos(1.0e-6));
	TEST_ASSERT_DOUBLE_WITHIN(DELTA, 1, cos(-1.0e-6));

	// TEST_ASSERT_DOUBLE_WITHIN(DELTA, -0.999987, cos(DBL_MAX)); Sprwadź !!!
	TEST_ASSERT_EQUAL_DOUBLE(1, cos(DBL_MIN));
}


TEST(math_trig, cos_special_cond)
{
	TEST_ASSERT_DOUBLE_IS_NAN(cos(NAN));

	TEST_ASSERT_EQUAL_DOUBLE(1, cos(0.0));
	TEST_ASSERT_EQUAL_DOUBLE(1, cos(-0.0));

	errno = 0;
	TEST_ASSERT_DOUBLE_IS_NAN(cos(INFINITY));
	// TEST_ASSERT_EQUAL_INT(EDOM, errno);

	errno = 0;
	TEST_ASSERT_DOUBLE_IS_NAN(cos(-INFINITY));
	// TEST_ASSERT_EQUAL_INT(EDOM, errno);
}


TEST(math_trig, acos_basic)
{
	int i;
	int iterates = 2000;
	double start = -0.99;
	double end = 0.99;
	double step = (end - start) / (iterates - 1);
	double x;

	for (i = 0; i <= iterates; i++) {
		x = start + i * step;
		TEST_ASSERT_EQUAL_DOUBLE(x, cos(acos(x)));
	}
}


TEST(math_trig, acos_huge_x)
{
	errno = 0;
	TEST_ASSERT_DOUBLE_IS_NAN(acos(1.0e+6));
	TEST_ASSERT_EQUAL_INT(EDOM, errno);

	errno = 0;
	TEST_ASSERT_DOUBLE_IS_NAN(acos(-1.0e+6));
	TEST_ASSERT_EQUAL_INT(EDOM, errno);

	TEST_ASSERT_EQUAL_DOUBLE(M_PI_2, acos(0.0));
	TEST_ASSERT_EQUAL_DOUBLE(M_PI_2, acos(-0.0));


	TEST_ASSERT_DOUBLE_WITHIN(DELTA, M_PI_2, acos(1.0e-6));
	TEST_ASSERT_DOUBLE_WITHIN(DELTA, M_PI_2, acos(-1.0e-6));

	errno = 0;
	TEST_ASSERT_DOUBLE_IS_NAN(acos(DBL_MAX));
	TEST_ASSERT_EQUAL_DOUBLE(M_PI_2, acos(DBL_MIN));
}


TEST(math_trig, acos_special_cond)
{
	errno = 0;
	TEST_ASSERT_DOUBLE_IS_NAN(acos(M_PI));
	TEST_ASSERT_EQUAL_INT(EDOM, errno);

	errno = 0;
	TEST_ASSERT_DOUBLE_IS_NAN(acos(-M_PI));
	TEST_ASSERT_EQUAL_INT(EDOM, errno);

	TEST_ASSERT_DOUBLE_IS_NAN(acos(NAN));

	errno = 0;
	TEST_ASSERT_DOUBLE_IS_NAN(acos(INFINITY));
	TEST_ASSERT_EQUAL_INT(EDOM, errno);

	errno = 0;
	TEST_ASSERT_DOUBLE_IS_NAN(acos(-INFINITY));
	TEST_ASSERT_EQUAL_INT(EDOM, errno);
}


TEST_SETUP(math_hyper)
{
}


TEST_TEAR_DOWN(math_hyper)
{
}


TEST(math_hyper, tanh_basic)
{
	int i;
	int iterates = 2000;
	double start = -DBL_MAX;
	double end = DBL_MAX;
	double step = (end - start) / (iterates - 1);
	double x, y;

	for (i = 0; i <= iterates; i++) {
		x = start + i * step;
		y = tanh(x / 2.0);
		TEST_ASSERT_EQUAL_DOUBLE(tanh(x), (2.0 * y) / (1.0 + y * y));
	}
}


TEST(math_hyper, tanh_huge_x)
{
	// TEST_ASSERT_EQUAL_DOUBLE(1, tanh(1.0e+6));
	// TEST_ASSERT_EQUAL_DOUBLE(-1, tanh(-1.0e+6));

	// TEST_ASSERT_EQUAL_DOUBLE(1e-10, tanh(1.0e-10)); // Sprawdź do którego bitu ma być uwzględnione
	// TEST_ASSERT_EQUAL_DOUBLE(-1e-10, tanh(-1.0e-10)); // Sprawdź do którego bitu ma być uwzględnione

	// TEST_ASSERT_EQUAL_DOUBLE(1, tanh(INT_MAX));
	// TEST_ASSERT_EQUAL_DOUBLE(-1, tanh(INT_MIN));

	// TEST_ASSERT_EQUAL_DOUBLE(1, tanh(DBL_MAX));
	// TEST_ASSERT_EQUAL_DOUBLE(DBL_MIN, tanh(DBL_MIN));
}


TEST(math_hyper, tanh_special_cond)
{
	// TEST_ASSERT_EQUAL_DOUBLE(NAN, tanh(NAN)); IMX, ZYNQ-QEMU

	TEST_ASSERT_EQUAL_DOUBLE(0, tanh(0.0));
	TEST_ASSERT_EQUAL_DOUBLE(0, tanh(-0.0));

	// TEST_ASSERT_EQUAL_DOUBLE(1, tanh(INFINITY));
	// TEST_ASSERT_EQUAL_DOUBLE(-1, tanh(-INFINITY));

	// TEST_ASSERT_EQUAL_DOUBLE(dblSubnormal(), tanh(dblSubnormal()));
}


TEST(math_hyper, sinh_basic)
{
	int i;
	int iterates = 2000;
	double start = -DBL_MAX;
	double end = DBL_MAX;
	double step = (end - start) / (iterates - 1);
	double x, y;

	for (i = 0; i <= iterates; i++) {
		x = start + i * step;
		y = sinh(x / 3.0);
		TEST_ASSERT_EQUAL_DOUBLE(sinh(x), (3.0 * y) + (4.0 * y * y * y));
	}
}


TEST(math_hyper, sinh_huge_x)
{
	TEST_ASSERT_EQUAL_DOUBLE(HUGE_VAL, sinh(1.0e+6));
	TEST_ASSERT_EQUAL_DOUBLE(-HUGE_VAL, sinh(-1.0e+6));

	TEST_ASSERT_DOUBLE_WITHIN(DELTA, 1e-06, sinh(1.0e-6));
	TEST_ASSERT_DOUBLE_WITHIN(DELTA, -1e-06, sinh(-1.0e-6));


	// TEST_ASSERT_EQUAL_DOUBLE(DBL_MIN, sinh(DBL_MIN));
	TEST_ASSERT_EQUAL_DOUBLE(HUGE_VAL, sinh(DBL_MAX));
}


TEST(math_hyper, sinh_special_cond)
{
	// TEST_ASSERT_DOUBLE_IS_NAN(sinh(NAN));

	TEST_ASSERT_EQUAL_DOUBLE(0, sinh(0.0));
	TEST_ASSERT_EQUAL_DOUBLE(0, sinh(-0.0));

	// TEST_ASSERT_EQUAL_DOUBLE(dblSubnormal(), sinh(dblSubnormal()));

	TEST_ASSERT_EQUAL_DOUBLE(INFINITY, sinh(INFINITY));

	TEST_ASSERT_EQUAL_DOUBLE(-INFINITY, sinh(-INFINITY));
}


TEST(math_hyper, cosh_basic)
{
	int i;
	int iterates = 2000;
	double start = -DBL_MAX;
	double end = DBL_MAX;
	double step = (end - start) / (iterates - 1);
	double x, y;

	for (i = 0; i <= iterates; i++) {
		x = start + i * step;
		y = cosh(x / 2.0);
		TEST_ASSERT_EQUAL_DOUBLE(cosh(x), ((2.0 * y * y) - 1.0));
	}
}


TEST(math_hyper, cosh_huge_x)
{
	TEST_ASSERT_EQUAL_DOUBLE(HUGE_VAL, cosh(1.0e+6));
	TEST_ASSERT_EQUAL_DOUBLE(HUGE_VAL, cosh(-1.0e+6));

	TEST_ASSERT_EQUAL_DOUBLE(1, cosh(1.0e-6));
	TEST_ASSERT_EQUAL_DOUBLE(1, cosh(-1.0e-6));

	TEST_ASSERT_EQUAL_DOUBLE(1, cosh(DBL_MIN));
	TEST_ASSERT_EQUAL_DOUBLE(HUGE_VAL, cosh(DBL_MAX));
}


TEST(math_hyper, cosh_special_cond)
{
	// TEST_ASSERT_DOUBLE_IS_NAN(cosh(NAN));

	TEST_ASSERT_EQUAL_DOUBLE(1.0, cosh(0.0));
	TEST_ASSERT_EQUAL_DOUBLE(1.0, cosh(-0.0));

	TEST_ASSERT_EQUAL_DOUBLE(INFINITY, cosh(INFINITY));

	TEST_ASSERT_EQUAL_DOUBLE(INFINITY, cosh(-INFINITY));
}


TEST_GROUP_RUNNER(math_trig)
{
	RUN_TEST_CASE(math_trig, tan_basic);
	RUN_TEST_CASE(math_trig, tan_huge_x);
	RUN_TEST_CASE(math_trig, tan_special_cond);

	RUN_TEST_CASE(math_trig, atan_basic);
	RUN_TEST_CASE(math_trig, atan_huge_x);
	RUN_TEST_CASE(math_trig, atan_special_cond);

	RUN_TEST_CASE(math_trig, sin_basic);
	RUN_TEST_CASE(math_trig, sin_huge_x);
	RUN_TEST_CASE(math_trig, sin_special_cond);

	RUN_TEST_CASE(math_trig, asin_basic);
	RUN_TEST_CASE(math_trig, asin_huge_x);
	RUN_TEST_CASE(math_trig, asin_special_cond);

	RUN_TEST_CASE(math_trig, cos_basic);
	RUN_TEST_CASE(math_trig, cos_huge_x);
	RUN_TEST_CASE(math_trig, cos_special_cond);

	RUN_TEST_CASE(math_trig, acos_basic);
	RUN_TEST_CASE(math_trig, acos_huge_x);
	RUN_TEST_CASE(math_trig, acos_special_cond);
}


TEST_GROUP_RUNNER(math_hyper)
{
	RUN_TEST_CASE(math_hyper, tanh_basic);
	RUN_TEST_CASE(math_hyper, tanh_huge_x);
	RUN_TEST_CASE(math_hyper, tanh_special_cond);

	RUN_TEST_CASE(math_hyper, sinh_basic);
	RUN_TEST_CASE(math_hyper, sinh_huge_x);
	RUN_TEST_CASE(math_hyper, sinh_special_cond);

	RUN_TEST_CASE(math_hyper, cosh_basic);
	RUN_TEST_CASE(math_hyper, cosh_huge_x);
	RUN_TEST_CASE(math_hyper, cosh_special_cond);
}
