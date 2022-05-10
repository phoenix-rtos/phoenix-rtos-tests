/*
 * Phoenix-RTOS
 *
 * phoenix-rtos-tests
 *
 * simple casting test
 *
 * Copyright 2021 Phoenix Systems
 * Author: Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <libgen.h>
#include <signal.h>
#include <sys/wait.h>

#include "unity_fixture.h"

TEST_GROUP(test_cast);

TEST_SETUP(test_cast)
{
}

TEST_TEAR_DOWN(test_cast)
{
}

TEST(test_cast, cast_assign)
{
	unsigned int u;
	int ran;

	srand(7);
	ran = rand();

	printf("0\n");
	u = (unsigned int)2 + ((double)ran / (double)2147483647) * 200000;
	printf("1\n");
	printf("%u\n", u);
}

TEST_GROUP_RUNNER(test_cast)
{
	RUN_TEST_CASE(test_cast, cast_assign);
}

void runner(void)
{
	RUN_TEST_GROUP(test_cast);
}

int main(int argc, char *argv[])
{
	UnityMain(argc, (const char **)argv, runner);
	printf("Test completed\n");
	return 0;
}
