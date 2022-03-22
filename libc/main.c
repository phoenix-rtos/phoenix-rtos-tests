/*
 * Phoenix-RTOS
 *
 * libc-tests
 *
 * Main entry point.
 *
 * Copyright 2021, 2022 Phoenix Systems
 * Author: Marek Bialowas, Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <stdlib.h>
#include "unity_fixture.h"

/* no need for forward declarations, RUN_TEST_GROUP does it by itself */
void runner(void)
{
	setenv("POSIXLY_CORRECT", "y", 1);

	// RUN_TEST_GROUP(stdio_fopenfclose);
	// RUN_TEST_GROUP(stdio_line);
	// RUN_TEST_GROUP(stdio_getput);
	// RUN_TEST_GROUP(stdio_fileseek);
	// RUN_TEST_GROUP(stdio_fileop);
	// RUN_TEST_GROUP(stdio_bufs);

	// RUN_TEST_GROUP(getpwd);
	// RUN_TEST_GROUP(resolve_path);
	// RUN_TEST_GROUP(file);
	// RUN_TEST_GROUP(unistd_getopt);
	// RUN_TEST_GROUP(unistd_uids);
	// RUN_TEST_GROUP(string_strlcpy);
	// RUN_TEST_GROUP(string_strlcat);
	// RUN_TEST_GROUP(unistd_fsdir);
	RUN_TEST_GROUP(unistd_file);
	// RUN_TEST_GROUP(wchar_wcscmp);
	// RUN_TEST_GROUP(test_pthread_cond);

	unsetenv("POSIXLY_CORRECT");
}


int main(int argc, char *argv[])
{
	UnityMain(argc, (const char **)argv, runner);
	return 0;
}
