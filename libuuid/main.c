/*
 * Phoenix-RTOS
 *
 * libuuid-tests
 *
 * Main entry point.
 *
 * Copyright 2022 Phoenix Systems
 * Author: Damian Loewnau
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
	RUN_TEST_GROUP(generate);
	RUN_TEST_GROUP(clear);
	RUN_TEST_GROUP(parse);
}


int main(int argc, char *argv[])
{
	return (UnityMain(argc, (const char **)argv, runner) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
