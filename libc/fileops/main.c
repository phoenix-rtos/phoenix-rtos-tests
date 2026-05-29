/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <sys/stat.h>
 *    - <unistd.h>
 *    - <fcntl.h>
 * TESTED:
 *    - fchmod()
 *    - fchmodat()
 *    - fchdir()
 *    - fchown()
 *    - fdatasync()
 *    - fstatat()
 *    - futimens()
 *    - lockf()
 *    - mkdirat()
 *    - mkfifoat()
 *    - sync()
 *    - utimensat()
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
#include "unity_fixture.h"

void runner(void)
{
	RUN_TEST_GROUP(fileops_fchmod);
	RUN_TEST_GROUP(fileops_fchmodat);
	RUN_TEST_GROUP(fileops_fchdir);
	RUN_TEST_GROUP(fileops_fstatat);
	RUN_TEST_GROUP(fileops_mkdirat);
	RUN_TEST_GROUP(fileops_mkfifoat);
	RUN_TEST_GROUP(fileops_futimens);
	RUN_TEST_GROUP(fileops_utimensat);
	RUN_TEST_GROUP(fileops_fchown);
	RUN_TEST_GROUP(fileops_fdatasync);
	RUN_TEST_GROUP(fileops_lockf);
	RUN_TEST_GROUP(fileops_sync);
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
