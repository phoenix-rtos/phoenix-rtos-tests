/*
 * Phoenix-RTOS
 *
 * test-libc-signal
 *
 * Main entry point.
 *
 * Copyright 2025 Phoenix Systems
 * Author: Marek Bialowas
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <stdlib.h>

#include <unity_fixture.h>


/* no need for forward declarations, RUN_TEST_GROUP does it by itself */
void runner(void)
{
	// RUN_TEST_GROUP(mask);
	// RUN_TEST_GROUP(handler);
	RUN_TEST_GROUP(sigaction);
	// RUN_TEST_GROUP(sigsuspend);
}


int main(int argc, char *argv[])
{
	return (UnityMain(argc, (const char **)argv, runner) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
