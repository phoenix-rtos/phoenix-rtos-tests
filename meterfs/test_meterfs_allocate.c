/*
 * Phoenix-RTOS
 *
 * Meterfs allocating tests group
 *
 * Copyright 2021, 2023 Phoenix Systems
 * Author: Tomasz Korniluk, Hubert Badocha
 *
 *
 * %LICENSE%
 */

#include "common.h"


static file_fsInfo_t fsInfo;


TEST_GROUP(meterfs_allocate);


TEST_SETUP(meterfs_allocate)
{
	TEST_ASSERT_EQUAL(0, file_devInfo(&fsInfo));
}


TEST_TEAR_DOWN(meterfs_allocate)
{
	TEST_ASSERT_EQUAL(0, file_eraseAll());
}


/*
 * Test case of allocating file bigger than flash size.
 */
TEST(meterfs_allocate, big_file)
{
	file_info_t info = { ((fsInfo.sz + fsInfo.sectorsz) / fsInfo.sectorsz) + 1u, fsInfo.sz + fsInfo.sectorsz, fsInfo.sectorsz / 100u, 0 };

	TEST_ASSERT_EQUAL(-EINVAL, file_allocate("file0", info.sectors, info.filesz, info.recordsz));
}


/* Test case of allocating more files than allowed. */
TEST(meterfs_allocate, many_files)
{
	size_t i, availableSectors;
	char fileName[32];

	availableSectors = (fsInfo.sz / fsInfo.sectorsz) - 1u;

	for (i = 0; i < (fsInfo.fileLimit + 10u); ++i) {
		(void)memset(fileName, 0, sizeof(fileName));
		(void)snprintf(fileName, sizeof(fileName), "file%zu", i);
		if ((i < fsInfo.fileLimit) && (availableSectors >= 2u)) {
			TEST_ASSERT_EQUAL_MESSAGE(0, file_allocate(fileName, 2, fsInfo.sectorsz / 2u, fsInfo.sectorsz / 200u), fileName);
			availableSectors -= 2u;
		}
		else {
			TEST_ASSERT_EQUAL_MESSAGE(-ENOMEM, file_allocate(fileName, 2, fsInfo.sectorsz / 2u, fsInfo.sectorsz / 200u), fileName);
		}
	}
}


/* Test case of allocating files with not allowed name length. */
TEST(meterfs_allocate, file_name_len)
{
	TEST_ASSERT_EQUAL(-EINVAL, file_allocate("file01234", 2, fsInfo.sectorsz / 2u, fsInfo.sectorsz / 100u));
	TEST_ASSERT_EQUAL(0, file_allocate("file0123", 2, fsInfo.sectorsz / 2u, fsInfo.sectorsz / 100u));
	TEST_ASSERT_EQUAL(-EINVAL, file_allocate("", 2, fsInfo.sectorsz / 2u, fsInfo.sectorsz / 100u));
}


/* Test case of allocating file with records bigger than flash sector size. */
TEST(meterfs_allocate, big_record)
{
	TEST_ASSERT_EQUAL(0, file_allocate("file0", 6, fsInfo.sectorsz * 5u, fsInfo.sectorsz + 1u));
}


/* Test case of allocating files with various initialization arguments. */
TEST(meterfs_allocate, var_init_args)
{
	TEST_ASSERT_EQUAL(-EINVAL, file_allocate("file0", 0, 0, 0));
	TEST_ASSERT_EQUAL(-EINVAL, file_allocate("file1", 0, fsInfo.sectorsz / 2u, fsInfo.sectorsz / 100u));
	TEST_ASSERT_EQUAL(-EINVAL, file_allocate("file2", 1, fsInfo.sectorsz / 2u, fsInfo.sectorsz / 100u));
	TEST_ASSERT_EQUAL(-EINVAL, file_allocate("file3", 2, fsInfo.sectorsz / 100u, fsInfo.sectorsz / 2u));
	TEST_ASSERT_EQUAL(-EINVAL, file_allocate("file4", 3, fsInfo.sectorsz * 100u, fsInfo.sectorsz / 100u));
	TEST_ASSERT_EQUAL(-EINVAL, file_allocate("file5", 7, fsInfo.sectorsz / 2u, 0));

	TEST_ASSERT_EQUAL(0, file_allocate("file6", 4, fsInfo.sectorsz / 100u, fsInfo.sectorsz / 100u));
	TEST_ASSERT_EQUAL(0, file_allocate("file7", 6, fsInfo.sectorsz / 2u, fsInfo.sectorsz / 10u));
	TEST_ASSERT_EQUAL(0, file_allocate("file8", 8, fsInfo.sectorsz / 100u, fsInfo.sectorsz / 200u));
	TEST_ASSERT_EQUAL(0, file_allocate("file9", 12, fsInfo.sectorsz / 200u, fsInfo.sectorsz / 400u));
	TEST_ASSERT_EQUAL(0, file_allocate("file10", 10, fsInfo.sectorsz / 2u, fsInfo.sectorsz / 100u));
	TEST_ASSERT_EQUAL(0, file_allocate("file11", 9, fsInfo.sectorsz / 2u, fsInfo.sectorsz / 100u));
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
	if (argc != 2) {
		(void)printf("Usage: %s /meterfs/mount/path\n", argv[0]);
		return 1;
	}
	if (file_init(argv[1]) != 0) {
		(void)printf("Failed to initialize test\n");
		return 1;
	}
	if (file_eraseAll() != 0) {
		(void)printf("Failed to format meterfs partition\n");
		return 1;
	}

	UnityMain(argc, (const char **)argv, runner);

	return 0;
}
