/*
 * Phoenix-RTOS
 *
 * test-libc-dirent
 *
 * Main entry point.
 *
 * Copyright 2023-2026 Phoenix Systems
 * Author: Arkadiusz Kozlowski, Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "unity_fixture.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* No need for forward declarations, RUN_TEST_GROUP does it by itself */
void runner(void)
{
	RUN_TEST_GROUP(dirent_opendir);
	RUN_TEST_GROUP(dirent_closedir);
	RUN_TEST_GROUP(dirent_readdir);
	RUN_TEST_GROUP(dirent_rewinddir);
	RUN_TEST_GROUP(dirent_fdopendir);
	RUN_TEST_GROUP(dirent_seekdir_telldir);
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
