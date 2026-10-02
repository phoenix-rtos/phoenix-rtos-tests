/*
 * Phoenix-RTOS
 *
 * libc/semaphore
 *
 * tests for POSIX semaphores
 *
 * Copyright 2026 Phoenix Systems
 * Author: Ziemowit Leszczynski
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>

#include "unity_fixture.h"


void runner(void)
{
	RUN_TEST_GROUP(sem_unnamed);
	RUN_TEST_GROUP(sem_named);
}


int main(int argc, char *argv[])
{
	return (UnityMain(argc, (const char **)argv, runner) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
