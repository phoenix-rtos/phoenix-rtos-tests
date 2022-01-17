/*
 * Phoenix-RTOS
 *
 * libc-tests
 *
 * Main entry point.
 *
 * Copyright 2021 Phoenix Systems
 * Author: Marek Bialowas
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "unity_fixture.h"

/* no need for forward declarations, RUN_TEST_GROUP does it by itself */
void runner(void)
{
	RUN_TEST_GROUP(getpwd);
	RUN_TEST_GROUP(resolve_path);
	RUN_TEST_GROUP(file);
	RUN_TEST_GROUP(unistd_getopt);
}


int main(int argc, char *argv[])
{
	UnityMain(argc, (const char **)argv, runner);
	return 0;
}
