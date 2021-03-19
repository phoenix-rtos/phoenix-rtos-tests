/*
 * Phoenix-RTOS
 *
 * Meterfs allocating tests group
 *
 * Copyright 2021 Phoenix Systems
 * Author: Tomasz Korniluk
 *
 *
 * %LICENSE%
 */

#include "common.h"

TEST_GROUP(meterfs_allocate);


TEST_SETUP(meterfs_allocate)
{
}


TEST_TEAR_DOWN(meterfs_allocate)
{
	TEST_ASSERT_EQUAL(0, file_eraseAll());
}


/* 
 * Test case of allocating file bigger than flash size.
 * NOTE: Not valid for hw target. Now there is no way to obtain/parametrize flash size in test.
 */
TEST(meterfs_allocate, big_file)
{
	uint32_t flashsz = 4 * 1024 * 1024;
	uint32_t sectorsz = 4096;
	file_info_t info = { ((flashsz + sectorsz) / sectorsz) + 1, flashsz + sectorsz, 20, 0 };

	TEST_ASSERT_EQUAL(-EINVAL, file_allocate("file0", info.sectors, info.filesz, info.recordsz));
}


/* Test case of allocating more files than allowed. */
TEST(meterfs_allocate, many_files)
{
	int i;
	char fileName[9];

	for (i = 0; i < (255 + 10); ++i) {
		snprintf(fileName, sizeof(fileName), "file%d", i);
		if (i < 255)
			TEST_ASSERT_EQUAL_MESSAGE(0, file_allocate(fileName, 2, 2000, 20), fileName);
		else
			TEST_ASSERT_EQUAL_MESSAGE(-ENOMEM, file_allocate(fileName, 2, 2000, 20), fileName);
		memset(fileName, 0, sizeof(fileName));
	}
}


/* Test case of allocating files with not allowed name length. */
TEST(meterfs_allocate, file_name_len)
{
	TEST_ASSERT_EQUAL(-EINVAL, file_allocate("file01234", 2, 2000, 20));
	TEST_ASSERT_EQUAL(0, file_allocate("file0123", 2, 2000, 20));
	TEST_ASSERT_EQUAL(-EINVAL, file_allocate("", 2, 2000, 20));
}


/* Test case of allocating file with records bigger than flash sector size. */
TEST(meterfs_allocate, big_record)
{
	TEST_ASSERT_EQUAL(0, file_allocate("file0", 6, 20000, 5000));
}


/* Test case of allocating files with various initialization arguments. */
TEST(meterfs_allocate, var_init_args)
{
	TEST_ASSERT_EQUAL(-EINVAL, file_allocate("file0", 0, 0, 0));
	TEST_ASSERT_EQUAL(-EINVAL, file_allocate("file1", 0, 2000, 20));
	TEST_ASSERT_EQUAL(-EINVAL, file_allocate("file2", 1, 2000, 20));
	TEST_ASSERT_EQUAL(-EINVAL, file_allocate("file3", 2, 20, 200));
	TEST_ASSERT_EQUAL(-EINVAL, file_allocate("file4", 3, 2000000, 20));
	TEST_ASSERT_EQUAL(-EINVAL, file_allocate("file5", 7, 2000, 0));

	TEST_ASSERT_EQUAL(0, file_allocate("file6", 4, 20, 20));	
	TEST_ASSERT_EQUAL(0, file_allocate("file7", 6, 2000, 20));
	TEST_ASSERT_EQUAL(0, file_allocate("file8", 8, 10, 5));
	TEST_ASSERT_EQUAL(0, file_allocate("file9", 12, 10, 5));
	TEST_ASSERT_EQUAL(0, file_allocate("file10", 10, 2000, 10));
	TEST_ASSERT_EQUAL(0, file_allocate("file11", 9, 2000, 20));
}


TEST_GROUP_RUNNER(meterfs_allocate)
{
	RUN_TEST_CASE(meterfs_allocate, big_file);
	RUN_TEST_CASE(meterfs_allocate, many_files);
	RUN_TEST_CASE(meterfs_allocate, file_name_len);
	RUN_TEST_CASE(meterfs_allocate, big_record);
	RUN_TEST_CASE(meterfs_allocate, var_init_args);
}


void runner(void)
{
	RUN_TEST_GROUP(meterfs_allocate);
}


int main(int argc, char *argv[])
{
	file_init(argv[1]);
	TEST_ASSERT_EQUAL(0, file_eraseAll());

	UnityMain(argc, (const char**)argv, runner);

	return 0;
}
