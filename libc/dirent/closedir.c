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
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>

#include <unity_fixture.h>
#include "common.h"
#include "dirent_helper_functions.h"

#define MAIN_DIR "test_closedir"

TEST_GROUP(dirent_closedir);


TEST_SETUP(dirent_closedir)
{
	errno = 0;
	TEST_MKDIR_ASSERTED(MAIN_DIR, S_IRWXU);
	errno = 0;
	TEST_MKDIR_ASSERTED(MAIN_DIR "/dir1", S_IRUSR);
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
	TEST_ASSERT_EQUAL_INT(0, closedir(dp));
}


TEST(dirent_closedir, closing_non_empty_dir)
{
	DIR *dp = opendir(MAIN_DIR);

	TEST_ASSERT_NOT_NULL(dp);
	TEST_ASSERT_EQUAL_INT(0, closedir(dp));
}


TEST(dirent_closedir, preserving_content_after_closedir)
{
	char dirNames[7][10];
	ino_t inodes[7];
	struct dirent *info;
	DIR *dp1;
	int result = 0;
	errno = 0;


	TEST_MKDIR_ASSERTED(MAIN_DIR "/test_preserve", S_IRWXU);
	TEST_MKDIR_ASSERTED(MAIN_DIR "/test_preserve/B", S_IRUSR);
	TEST_MKDIR_ASSERTED(MAIN_DIR "/test_preserve/CC", S_IRUSR);
	TEST_MKDIR_ASSERTED(MAIN_DIR "/test_preserve/DDDD", S_IRUSR);
	TEST_MKDIR_ASSERTED(MAIN_DIR "/test_preserve/EEEEEE", S_IRUSR);


	dp1 = opendir(MAIN_DIR "/test_preserve");
	TEST_ASSERT_NOT_NULL(dp1);

	/*
	 *Create an array with names of dirs.
	 *Indexes will be used to associate name of each directory with a bit
	 */

	{
		int i = 0;
		while ((info = readdir(dp1)) != NULL) {
			inodes[i] = info->d_ino;
			strcpy(dirNames[i++], info->d_name);
		}
	}

	TEST_ASSERT_EQUAL_INT(0, closedir(dp1));
	DIR *dp2 = opendir(MAIN_DIR "/test_preserve");
	TEST_ASSERT_NOT_NULL(dp2);
	rewinddir(dp2);

	/*
	 * Map each directory to a bit.
	 * In case of fail set most left bit to 1.
	 * Assert every bit is high except the first two.
	 */

	/* Go through each entry */

	while ((info = readdir(dp2)) != NULL) {
		int found = 0;
		char name[10];
		strcpy(name, info->d_name);

		/* determine the index of given name */
		for (int i = 0; i < 7; ++i) {
			if (!strcmp(name, dirNames[i])) {
				TEST_ASSERT_EQUAL_INT64(inodes[i], info->d_ino);
				/* Set the index bit of found name */
				result |= 1 << i;

				found = 1;
			}
		}
		/* Name found, time to search for another name */
		if (found) {
			continue;
		}

		TEST_FAIL();
	}

	/* result variable should be 0b0011 1111, which is 63, or 3f */
	TEST_ASSERT_EQUAL_INT(0x3f, result);

	closedir(dp2);

	rmdir(MAIN_DIR "/test_preserve/B");
	rmdir(MAIN_DIR "/test_preserve/CC");
	rmdir(MAIN_DIR "/test_preserve/DDDD");
	rmdir(MAIN_DIR "/test_preserve/EEEEEE");
	rmdir(MAIN_DIR "/test_preserve");
}


TEST_GROUP_RUNNER(dirent_closedir)
{
	RUN_TEST_CASE(dirent_closedir, closing_empty_dir);
	RUN_TEST_CASE(dirent_closedir, closing_non_empty_dir);
	RUN_TEST_CASE(dirent_closedir, preserving_content_after_closedir);
}
