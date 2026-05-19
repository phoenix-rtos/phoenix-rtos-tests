/*
 * Phoenix-RTOS
 *
 * libc/posixsrv
 *
 * tests for posixsrv-related functionality
 *
 * Copyright 2026 Phoenix Systems
 * Author: Julian Uziembło
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <stdlib.h>

#include "unity_fixture.h"


void runner(void)
{
	RUN_TEST_GROUP(tmpfile);
}


int main(int argc, char *argv[])
{
	return (UnityMain(argc, (const char **)argv, runner) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
