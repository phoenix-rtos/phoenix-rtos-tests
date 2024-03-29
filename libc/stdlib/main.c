/*
 * Phoenix-RTOS
 *
 * test-libc-stdio
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

#include <stdlib.h>

#include "unity_fixture.h"

/* no need for forward declarations, RUN_TEST_GROUP does it by itself */
void runner(void)
{
	RUN_TEST_GROUP(stdlib_alloc);
	RUN_TEST_GROUP(stdlib_env);
	RUN_TEST_GROUP(stdlib_bsearch);
	RUN_TEST_GROUP(stdlib_strto);
}


int main(int argc, char *argv[])
{
	return (UnityMain(argc, (const char **)argv, runner) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
