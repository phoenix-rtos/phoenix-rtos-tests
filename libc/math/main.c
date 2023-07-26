/*
 * Phoenix-RTOS
 *
 * test-libc-math
 *
 * Main entry point.
 *
 * Copyright 2023 Phoenix Systems
 * Author: Mateusz Bloch
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "unity_fixture.h"

/* no need for forward declarations, RUN_TEST_GROUP does it by itself */
void runner(void)
{
	RUN_TEST_GROUP(math_mod);
	RUN_TEST_GROUP(math_exp);
	RUN_TEST_GROUP(math_power);
	RUN_TEST_GROUP(math_trig);
	RUN_TEST_GROUP(math_hyper);
}


int main(int argc, char *argv[])
{
	UnityMain(argc, (const char **)argv, runner);

	return 0;
}
