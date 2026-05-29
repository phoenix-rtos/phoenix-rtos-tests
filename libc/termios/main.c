/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <termios.h>
 *    - <unistd.h>
 * TESTED:
 *    - tcdrain()
 *    - tcflow()
 *    - tcflush()
 *    - tcgetsid()
 *    - tcsetattr()
 *    - tcgetpgrp()
 *    - tcsetpgrp()
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
	RUN_TEST_GROUP(termios_tcdrain);
	RUN_TEST_GROUP(termios_tcflow);
	RUN_TEST_GROUP(termios_tcflush);
	RUN_TEST_GROUP(termios_tcgetsid);
	RUN_TEST_GROUP(termios_tcsetattr);
	RUN_TEST_GROUP(termios_tcgetpgrp);
	RUN_TEST_GROUP(termios_tcsetpgrp);
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
