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
	RUN_TEST_GROUP(stdio_scanf_d);
	RUN_TEST_GROUP(stdio_scanf_i);
	RUN_TEST_GROUP(stdio_scanf_u);
	RUN_TEST_GROUP(stdio_scanf_o);
	RUN_TEST_GROUP(stdio_scanf_x);
	RUN_TEST_GROUP(stdio_scanf_aefg);
	RUN_TEST_GROUP(stdio_scanf_cspn);
	RUN_TEST_GROUP(stdio_scanf_squareBrackets);
	RUN_TEST_GROUP(stdio_scanf_rest);
}


int main(int argc, char *argv[])
{
	UnityMain(argc, (const char **)argv, runner);

	return 0;
}
