/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - sched.h
 * TESTED:
 *    - sched_get_priority_max()
 *    - sched_get_priority_min()
 *    - sched_yield()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Daniel Loew
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "unity_fixture.h"

void runner(void)
{
	RUN_TEST_GROUP(sched_get_priority);
	RUN_TEST_GROUP(sched_yield);
}

int main(int argc, char *argv[])
{
	const char *var = "POSIXLY_CORRECT";

	if (setenv(var, "y", 1) != 0) {
		fprintf(stderr, "Setting %s environment variable failed: %s\n", var, strerror(errno));
		return 1;
	}

	return (UnityMain(argc, (const char **)argv, runner) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
