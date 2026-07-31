/*
 * Phoenix-RTOS
 *
 * test-libc-fcntl
 *
 * Main entry point.
 *
 * Copyright 2026 Phoenix Systems
 * Author: Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "common.h"
#include "unity_fixture.h"

void runner(void)
{
	RUN_TEST_GROUP(fcntl_open);
	RUN_TEST_GROUP(fcntl_creat);
	RUN_TEST_GROUP(fcntl_fcntl);
	RUN_TEST_GROUP(fcntl_openat);
}

int main(int argc, char *argv[])
{
	const char *var = "POSIXLY_CORRECT";

	if (setenv(var, "y", 1) != 0) {
		fprintf(stderr, "Setting %s environment variable failed: %s\n", var, strerror(errno));
		return 1;
	}

	/* /tmp may not be present on dummyfs targets, create it to make libc tests common */
	if (libc_createDirIfMissing("/tmp") < 0) {
		unsetenv(var);
		return 1;
	}

	return (UnityMain(argc, (const char **)argv, runner) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
