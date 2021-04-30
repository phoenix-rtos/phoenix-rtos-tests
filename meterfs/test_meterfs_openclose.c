/*
 * Phoenix-RTOS
 *
 * Meterfs opening and closing tests group
 *
 * Copyright 2021 Phoenix Systems
 * Author: Tomasz Korniluk
 *
 *
 * %LICENSE%
 */

#include "common.h"

static struct {
	int fds[255];
	char buff[21];
} common;

TEST_GROUP(meterfs_openclose);


TEST_SETUP(meterfs_openclose)
{
	memset(common.buff, 0, sizeof(common.buff));
}


TEST_TEAR_DOWN(meterfs_openclose)
{
	TEST_ASSERT_EQUAL(0, file_eraseAll());
}


/* Test case of opening and closing non existing files. */
TEST(meterfs_openclose, no_files)
{
	int i;

	for (i = 0; i < sizeof(common.fds) / sizeof(common.fds[0]); ++i) {
		snprintf(common.buff, sizeof(common.buff), "/file%d", i);
		TEST_ASSERT_EQUAL_MESSAGE(-ENOENT, file_open(common.buff), common.buff);
		TEST_ASSERT_EQUAL_MESSAGE(-ENOENT, file_close(i), common.buff);
		memset(common.buff, 0, sizeof(common.buff));
	}
}


/* Test case of opening and closing existing files. */
TEST(meterfs_openclose, existing_files)
{
	int i;

	for (i = 0; i < sizeof(common.fds) / sizeof(common.fds[0]); ++i) {
		snprintf(common.buff, sizeof(common.buff), "file%d", i);
		TEST_ASSERT_EQUAL_MESSAGE(0, file_allocate(common.buff, 2, 2000, 20), common.buff);
		memset(common.buff, 0, sizeof(common.buff));
	}

	for (i = 0; i < sizeof(common.fds) / sizeof(common.fds[0]); ++i) {
		snprintf(common.buff, sizeof(common.buff), "/file%d", i);
		common.fds[i] = file_open(common.buff);
		TEST_ASSERT_GREATER_OR_EQUAL_MESSAGE(0, common.fds[i], common.buff);
		memset(common.buff, 0, sizeof(common.buff));
	}

	for (i = 0; i < sizeof(common.fds) / sizeof(common.fds[0]); ++i) {
		snprintf(common.buff, sizeof(common.buff), "file%d", i);
		TEST_ASSERT_EQUAL_MESSAGE(0, file_close(common.fds[i]), common.buff);
		memset(common.buff, 0, sizeof(common.buff));
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
	file_init(argv[1]);
	TEST_ASSERT_EQUAL(0, file_eraseAll());

	UnityMain(argc, (const char**)argv, runner);

	return 0;
}
