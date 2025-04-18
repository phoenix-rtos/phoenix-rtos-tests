/*
 * Phoenix-RTOS
 *
 * test-libc-stdio
 *
 * Main entry point.
 *
 * Copyright 2023 Phoenix Systems
 * Author: Mateusz Bloch
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
	RUN_TEST_GROUP(file);
	RUN_TEST_GROUP(stdio_fopenfclose);
	RUN_TEST_GROUP(stdio_line);
	RUN_TEST_GROUP(stdio_getput);
	RUN_TEST_GROUP(stdio_fileseek);
	RUN_TEST_GROUP(stdio_fileop);
	RUN_TEST_GROUP(stdio_bufs);
	RUN_TEST_GROUP(stdio_fread);
	RUN_TEST_GROUP(stdio_fwrite);
	RUN_TEST_GROUP(stdio_fflush);
}


int main(int argc, char *argv[])
{
	return (UnityMain(argc, (const char **)argv, runner) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
