/*
 * Phoenix-RTOS
 *
 * Meterfs opening and closing tests group
 *
 * Copyright 2021, 2023 Phoenix Systems
 * Author: Tomasz Korniluk, Hubert Badocha
 *
 *
 * %LICENSE%
 */

#include "common.h"

static struct {
	int fds[255];
	char buff[21];
} common;


static file_fsInfo_t fsInfo;


TEST_GROUP(meterfs_openclose);


TEST_SETUP(meterfs_openclose)
{
	(void)memset(common.buff, 0, sizeof(common.buff));
	TEST_ASSERT_EQUAL(0, file_devInfo(&fsInfo));
}


TEST_TEAR_DOWN(meterfs_openclose)
{
	TEST_ASSERT_EQUAL(0, file_eraseAll());
}


/* Test case of opening and closing non existing files. */
TEST(meterfs_openclose, no_files)
{
	size_t i;

	for (i = 0; i < sizeof(common.fds) / sizeof(common.fds[0]); ++i) {
		(void)snprintf(common.buff, sizeof(common.buff), "/file%zu", i);
		TEST_ASSERT_EQUAL_MESSAGE(-ENOENT, file_open(common.buff), common.buff);
		TEST_ASSERT_EQUAL_MESSAGE(-ENOENT, file_close(i), common.buff);
		(void)memset(common.buff, 0, sizeof(common.buff));
	}
}


/* Test case of opening and closing existing files. */
TEST(meterfs_openclose, existing_files)
{
	size_t i, fileCount = sizeof(common.fds) / sizeof(common.fds[0]);
	if (fileCount > fsInfo.fileLimit) {
		fileCount = fsInfo.fileLimit;
	}

	for (i = 0; i < fileCount; ++i) {
		(void)snprintf(common.buff, sizeof(common.buff), "file%zu", i);
		TEST_ASSERT_EQUAL_MESSAGE(0, file_allocate(common.buff, 2, 2000, 20), common.buff);
		(void)memset(common.buff, 0, sizeof(common.buff));
	}

	for (i = 0; i < fileCount; ++i) {
		(void)snprintf(common.buff, sizeof(common.buff), "/file%zu", i);
		common.fds[i] = file_open(common.buff);
		TEST_ASSERT_GREATER_OR_EQUAL_MESSAGE(0, common.fds[i], common.buff);
		(void)memset(common.buff, 0, sizeof(common.buff));
	}

	for (i = 0; i < fileCount; ++i) {
		(void)snprintf(common.buff, sizeof(common.buff), "file%zu", i);
		TEST_ASSERT_EQUAL_MESSAGE(0, file_close(common.fds[i]), common.buff);
		(void)memset(common.buff, 0, sizeof(common.buff));
	}
}


TEST_GROUP_RUNNER(meterfs_openclose)
{
	RUN_TEST_CASE(meterfs_openclose, no_files);
	RUN_TEST_CASE(meterfs_openclose, existing_files);
}


void runner(void)
{
	RUN_TEST_GROUP(meterfs_openclose);
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
