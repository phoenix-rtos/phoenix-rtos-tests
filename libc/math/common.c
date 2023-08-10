/*
 * Phoenix-RTOS
 *
 * libc-tests
 *
 * Math tests common part.
 *
 * Copyright 2023 Phoenix Systems
 * Author: Adam Debek
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "common.h"

#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <float.h>

double test_getRandomLog(double min, double max)
{
	if (min == 0.0) {
		min = DBL_MIN;
	}
	double a = log(min);
	double b = log(max);
	double r = (double)rand() / (double)RAND_MAX;

	return exp(a + r * (b - a));
}


double test_getRandomLogSubnrm()
{
	/* Values passed to function refer to subnormals range */
	return test_getRandomLog(5.0e-324, 2.0e-308);
}


void test_setup(void)
{
	double num = M_PI;
	const double one = 1.0;
	const double zero = 0.0;
	const double half = 0.5;

	test_common.maxPowTwoPi = 0;
	test_common.maxPowTwo = 0;
	test_common.dblSignif = 0;

	/* Initialize seed for test */
	srand(time(NULL));

	/* Initialize maxPowTwoPi value */
	do {
		test_common.maxPowTwoPi++;
		num *= 2.0;
	} while (num - M_PI != num);

	num = 1.0;
	/* Initialize maxPowTwo value */
	do {
		test_common.maxPowTwo++;
		num *= 2.0;
	} while (num + half != num);

	num = 1.0;
	/* Initialize digSignif value */
	do {
		test_common.dblSignif++;
		num *= 2.0;
	} while ((((num + one) - num) - one) == zero);
}


double test_checkResult(double f, double g)
{
	double digSignif, digLost, diff;

	if (g - f != g) {
		if (g != 0.0) {
			diff = fabs((f - g) / g);
		}
		else {
			diff = f;
		}
	}
	else {
		if (f != 0.0) {
			diff = fabs((f - g) / f);
		}
		else {
			diff = g;
		}
	}

	if (diff > 0.0) {
		digSignif = log2(diff);
	}
	else {
		digSignif = -999.0;
	}

	if (digSignif + test_common.dblSignif < 0.0) {
		digLost = 0.0;
	}
	else {
		digLost = digSignif + test_common.dblSignif;
	}

	return digLost;
}

void test_check_digLost(char *fun, double x, double digLost, double acceptLoss)
{
	char errStr[150];

	if (digLost > acceptLoss) {
		sprintf(errStr, "%s"
						" (%g) gave lost of %.f significant digits of base 2 where "
						"maximal acceptable loss is %.f signicant digits of base 2",
			fun, x, digLost, acceptLoss);
		TEST_FAIL_MESSAGE(errStr);
	}
}


void test_check_digLost2(char *fun, double x, double y, double digLost, double acceptLoss)
{
	char errStr[150];

	if (digLost > acceptLoss) {
		sprintf(errStr, "%s"
						" (%g, %g) gave lost of %.f significant digits of base 2 where "
						"maximal acceptable loss is %.f signicant digits of base 2",
			fun, x, y, digLost, acceptLoss);
		TEST_FAIL_MESSAGE(errStr);
	}
}


void test_log_vector_check(double arg, double expected, double actual, double epsilon)
{
	char errStr[100];
	double diff = expected - actual;

	if (diff < 0.0) {
		diff = -diff;
	}

	if (epsilon < 0.0) {
		epsilon = -epsilon;
	}

	if (diff > epsilon) {
		sprintf(errStr, "Log(%g): expected value: %g actual value: %g epsilon: %g",
			arg, expected, actual, epsilon);
		TEST_FAIL_MESSAGE(errStr);
	}
}


void test_exp_vector_check(double arg, double expected, double actual, double epsilon)
{
	char errStr[100];
	double diff = expected - actual;

	if (diff < 0.0) {
		diff = -diff;
	}

	if (epsilon < 0.0) {
		epsilon = -epsilon;
	}

	if (diff > epsilon) {
		sprintf(errStr, "Exp(%g): expected value: %g actual value: %g epsilon: %g",
			arg, expected, actual, epsilon);
		TEST_FAIL_MESSAGE(errStr);
	}
}
