/*
 * Phoenix-RTOS
 *
 * phoenix-rtos-test
 *
 * mprotect syscall tests
 *
 * Copyright 2023 Phoenix Systems
 * Author: Hubert Badocha
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <sys/mman.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

#include "unity_fixture.h"


TEST_GROUP(test_mprotect_fault);


TEST_SETUP(test_mprotect_fault)
{
}


TEST_TEAR_DOWN(test_mprotect_fault)
{
}


TEST(test_mprotect_fault, unit)
{
	long pageSize = sysconf(_SC_PAGESIZE);
	long totalSize = 4 * pageSize;
	volatile unsigned char *area = mmap(NULL, totalSize, PROT_READ | PROT_WRITE, MAP_ANONYMOUS, -1, 0);
	TEST_ASSERT(area != MAP_FAILED);

	TEST_ASSERT_EQUAL(0, mprotect((void *)area, totalSize, PROT_READ));

	area[0] = 0x42; /* Should generate fault. */
	TEST_ASSERT_NOT_EQUAL(0x42, area[0]);
}


TEST_GROUP_RUNNER(test_mprotect_fault)
{
	RUN_TEST_CASE(test_mprotect_fault, unit);
}


static void runner(void)
{
	RUN_TEST_GROUP(test_mprotect_fault);
}


int main(int argc, char *argv[])
{
	return (UnityMain(argc, (const char **)argv, runner) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
