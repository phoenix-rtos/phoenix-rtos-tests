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


#define PAGES 4


static long page_size;


TEST_GROUP(test_mprotect);


TEST_SETUP(test_mprotect)
{
	page_size = sysconf(_SC_PAGESIZE);
}


TEST_TEAR_DOWN(test_mprotect)
{
}


TEST(test_mprotect, test_mprotect_singlecore)
{
	unsigned char *area = mmap(NULL, page_size * PAGES, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	TEST_ASSERT(area != MAP_FAILED);

	for (int page = 0; page < PAGES; page++) {
		area[page * page_size] = 0x42;
	}

	TEST_ASSERT_EQUAL(0, mprotect(area, page_size * PAGES, PROT_READ));

	for (int page = 0; page < PAGES; page++) {
		TEST_ASSERT_EQUAL(0x42, area[page * page_size]);
	}

	TEST_ASSERT_EQUAL(0, mprotect(area, page_size * PAGES, PROT_READ | PROT_WRITE));

	for (int page = 0; page < PAGES; page++) {
		area[(page * page_size) + 0x6] = 0x9;
		TEST_ASSERT_EQUAL(0x9, area[(page * page_size) + 0x6]);
	}

	TEST_ASSERT_EQUAL(0, munmap(area, page_size * PAGES));
}

TEST_GROUP_RUNNER(test_mprotect)
{
	RUN_TEST_CASE(test_mprotect, test_mprotect_singlecore);
}


static void runner(void)
{
	RUN_TEST_GROUP(test_mprotect);
}


int main(int argc, char *argv[])
{
	return (UnityMain(argc, (const char **)argv, runner) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
