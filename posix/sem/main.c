/*
 * Phoenix-RTOS
 *
 * test-posix-sem
 *
 * POSIX semaphore tests.
 *
 * Copyright 2026 Phoenix Systems
 * Author: Michal Lach
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <stdlib.h>

#include <unity_fixture.h>


void runner(void)
{
	RUN_TEST_GROUP(named);
}

int main(int argc, char *argv[])
{
	return (UnityMain(argc, (const char **)argv, runner) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
