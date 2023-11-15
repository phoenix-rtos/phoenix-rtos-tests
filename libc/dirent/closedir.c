/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - dirent.h
 * TESTED:
 *    - closedir()
 *
 * Copyright 2023 Phoenix Systems
 * Author: Arkadiusz Kozlowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <string.h>

#include <unity_fixture.h>
#include "common.h"

#define MAIN_DIR "test_closedir"


TEST_GROUP(dirent_closedir);


TEST_SETUP(dirent_closedir)
{
	errno = 0;
	TEST_ASSERT_TRUE(mkdir(MAIN_DIR, 0777) != -1 || errno == EEXIST);
	TEST_ASSERT_TRUE(mkdir(MAIN_DIR "/dir1", 0777) != -1 || errno == EEXIST);
}


TEST_TEAR_DOWN(dirent_closedir)
{
	rmdir(MAIN_DIR "/dir1");
	rmdir(MAIN_DIR);
}


TEST(dirent_closedir, closing_empty_dir)
{
	DIR *dp = opendir(MAIN_DIR "/dir1");

	TEST_ASSERT_NOT_NULL(dp);
	TEST_ASSERT_EQUAL(0, closedir(dp));
}


TEST(dirent_closedir, closing_non_empty_dir)
{
	DIR *dp = opendir(MAIN_DIR);

	TEST_ASSERT_NOT_NULL(dp);
	TEST_ASSERT_EQUAL_INT(0, closedir(dp));
}


TEST_GROUP_RUNNER(dirent_closedir)
{
	RUN_TEST_CASE(dirent_closedir, closing_empty_dir);
	RUN_TEST_CASE(dirent_closedir, closing_non_empty_dir);
}
