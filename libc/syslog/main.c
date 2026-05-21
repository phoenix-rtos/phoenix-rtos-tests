/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <syslog.h>
 * TESTED:
 *    - syslog()
 *    - setlogmask()
 *    - openlog()
 *    - closelog()
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
#include "unity_fixture.h"


void runner(void)
{
	RUN_TEST_GROUP(syslog_setlogmask);
	RUN_TEST_GROUP(syslog_syslog);
	RUN_TEST_GROUP(syslog_openlog);
	RUN_TEST_GROUP(syslog_closelog);
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
