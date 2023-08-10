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

#include <stdio.h>

#include "unity_fixture.h"

#ifndef _TEST_MATH_COMMON_H
#define _TEST_MATH_COMMON_H

#ifdef __CPU_STM32L4X6
#define ITER_FACTOR 7
#else
#define ITER_FACTOR 100
#endif /* __CPU_STM32L4X6 */

struct test_common_s {
	int maxPowTwoPi;
	int maxPowTwo;
	int dblSignif;
};

extern struct test_common_s test_common;

double test_getRandomLog(double min, double max);

double test_getRandomLogSubnrm(void);

double test_checkResult(double f, double g);

void test_setup(void);

void test_check_digLost(char *fun, double x, double digLost, double acceptLoss);

void test_check_digLost2(char *fun, double x, double y, double digLost, double acceptLoss);

void test_log_vector_check(double arg, double expected, double actual, double epsilon);

void test_exp_vector_check(double arg, double expected, double actual, double epsilon);

#endif /* TEST_MATH_COMMON_H */
