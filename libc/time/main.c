/*
 * Phoenix-RTOS
 *
 * test-libc-time
 *
 * Main entry point.
 *
 * Copyright 2023 Phoenix Systems
 * Author: Jacek Maksymowicz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "unity_fixture.h"

void runner(void)
{
	RUN_TEST_GROUP(time_mktime);
	RUN_TEST_GROUP(time_gmtime);
	RUN_TEST_GROUP(time_strftime);
}


int main(int argc, char *argv[])
{
	int failures = UnityMain(argc, (const char **)argv, runner);
	return (failures == 0) ? 0 : 1;
}
