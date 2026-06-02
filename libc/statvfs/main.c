/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <sys/statvfs.h>
 * TESTED:
 *    - statvfs()
 *    - fstatvfs()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Lukasz Kruszynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include "unity_fixture.h"

extern void runner(void);

/* create directory unless it exists */
static int libc_createDirIfMissing(const char *path)
{
	struct stat buffer;

	if (stat(path, &buffer) != 0) {
		if (errno == ENOENT) {
			if (mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0) {
				fprintf(stderr, "Creating %s directory by mkdir failed: %s\n", path, strerror(errno));
				return -1;
			}
		}
		else {
			fprintf(stderr, "stat() on %s directory failed: %s\n", path, strerror(errno));
			return -1;
		}
	}

	return 0;
}

void runner(void)
{
	RUN_TEST_GROUP(statvfs_statvfs);
	RUN_TEST_GROUP(statvfs_fstatvfs);
}

int main(int argc, char *argv[])
{
	const char *var = "POSIXLY_CORRECT";

	if (setenv(var, "y", 1) != 0) {
		fprintf(stderr, "Setting %s environment variable failed: %s\n", var, strerror(errno));
		return 1;
	}

	if (libc_createDirIfMissing("/tmp") < 0) {
		unsetenv(var);
		return 1;
	}

	return (UnityMain(argc, (const char **)argv, runner) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
