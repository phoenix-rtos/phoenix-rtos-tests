/*
 * Phoenix-RTOS
 *
 * test-libc-math
 *
 * Libc math tests entry point.
 *
 * Copyright 2023 Phoenix Systems
 * Author: Adam Debek
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "common.h"

#include <unity_fixture.h>

struct test_common_s test_common;

void runner(void)
{
	RUN_TEST_GROUP(static_vector);
	RUN_TEST_GROUP(math_abs);
	RUN_TEST_GROUP(math_frac);
	RUN_TEST_GROUP(math_exp);
	RUN_TEST_GROUP(math_pow);
	RUN_TEST_GROUP(math_trig);
	RUN_TEST_GROUP(math_hyper);
}


int main(int argc, char *argv[])
{
	UnityMain(argc, (const char **)argv, runner);

	return 0;
}
