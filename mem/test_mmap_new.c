/*
 * Phoenix-RTOS
 *
 * phoenix-rtos-test
 *
 * mmap syscall tests
 *
 * Copyright 2024 Phoenix Systems
 * Author: Hubert Badocha
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

#include "unity_fixture.h"


static long page_size;
static const char *filename = "./mmap_testfile";


TEST_GROUP(test_mmap);


TEST_SETUP(test_mmap)
{
	page_size = sysconf(_SC_PAGESIZE);
}


TEST_TEAR_DOWN(test_mmap)
{
	unlink(filename);
}


TEST(test_mmap, len__zero)
{
	TEST_ASSERT_EQUAL(MAP_FAILED, mmap(NULL, 0, PROT_READ, MAP_ANONYMOUS, -1, 0));
}


TEST(test_mmap, len__not_page_aligned)
{
	unsigned char *area = mmap(NULL, page_size + 1, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	TEST_ASSERT_NOT_EQUAL(MAP_FAILED, area);

	TEST_ASSERT_EQUAL(0, munmap(area, page_size + 1));
}


TEST(test_mmap, len__not_page_aligned_file)
{
	const char *data = "ABCDEFGHIJKLMNOPQRSTUVWXYZ123456";  // 32 characters

	// Create file and write 32 characters
	int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0600);
	TEST_ASSERT_NOT_EQUAL(-1, fd);
	TEST_ASSERT_EQUAL(32, write(fd, data, 32));
	TEST_ASSERT_EQUAL(0, close(fd));

	// Map the first 16 bytes of the file
	fd = open(filename, O_RDONLY);
	TEST_ASSERT_NOT_EQUAL(-1, fd);
	void *map = mmap(NULL, 16, PROT_READ, MAP_PRIVATE, fd, 0);
	TEST_ASSERT_NOT_EQUAL(MAP_FAILED, map);

	TEST_ASSERT_EQUAL(0, munmap(map, 16));
	TEST_ASSERT_EQUAL(0, close(fd));
}


TEST_GROUP_RUNNER(test_mmap)
{
	RUN_TEST_CASE(test_mmap, len__not_page_aligned);
	RUN_TEST_CASE(test_mmap, len__not_page_aligned_file);
	RUN_TEST_CASE(test_mmap, len__zero);
}


static void runner(void)
{
	RUN_TEST_GROUP(test_mmap);
}


int main(int argc, char *argv[])
{
	return (UnityMain(argc, (const char **)argv, runner) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
