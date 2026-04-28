/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - dirent.h
 * TESTED:
 *    - closedir()
 *
 * Copyright 2023-2026 Phoenix Systems
 * Authors: Arkadiusz Kozlowski, Lukasz Kruszynski
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
#include <stdlib.h>

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


TEST(dirent_closedir, closes_empty_dir)
{
	DIR *dp = opendir(MAIN_DIR "/dir1");

	TEST_ASSERT_NOT_NULL(dp);
	TEST_ASSERT_EQUAL_INT(0, closedir(dp));
}


TEST(dirent_closedir, closes_non_empty_dir)
{
	DIR *dp = opendir(MAIN_DIR);

	TEST_ASSERT_NOT_NULL(dp);
	TEST_ASSERT_EQUAL_INT(0, closedir(dp));
}


TEST(dirent_closedir, preserves_content_after_use)
{
	char(*dirNames)[NAME_MAX + 1] = malloc(10 * sizeof(*dirNames));
	TEST_ASSERT_NOT_NULL(dirNames);
	ino_t inodes[10];
	struct dirent *info;
	DIR *dp1;
	int result = 0;
	int num_entries = 0;
	errno = 0;

	TEST_MKDIR_ASSERTED(MAIN_DIR "/test_preserve", S_IRWXU);
	TEST_MKDIR_ASSERTED(MAIN_DIR "/test_preserve/B", S_IRUSR);
	TEST_MKDIR_ASSERTED(MAIN_DIR "/test_preserve/CC", S_IRUSR);
	TEST_MKDIR_ASSERTED(MAIN_DIR "/test_preserve/DDDD", S_IRUSR);
	TEST_MKDIR_ASSERTED(MAIN_DIR "/test_preserve/EEEEEE", S_IRUSR);

	dp1 = opendir(MAIN_DIR "/test_preserve");
	TEST_ASSERT_NOT_NULL(dp1);

	while ((info = readdir(dp1)) != NULL && num_entries < 10) {
		inodes[num_entries] = info->d_ino;
		strcpy(dirNames[num_entries++], info->d_name);
	}

	TEST_ASSERT_EQUAL_INT(0, closedir(dp1));

	DIR *dp2 = opendir(MAIN_DIR "/test_preserve");
	TEST_ASSERT_NOT_NULL(dp2);

	while ((info = readdir(dp2)) != NULL) {
		int found = 0;

		for (int i = 0; i < num_entries; ++i) {
			if (!strcmp(info->d_name, dirNames[i])) {
				TEST_ASSERT_EQUAL_INT64(inodes[i], info->d_ino);
				/* Set the index bit of found name */
				result |= (1 << i);
				found = 1;
				break;
			}
		}
		TEST_ASSERT_EQUAL_INT(1, found);
	}

	/* Dynamically calculate the expected bitmask (e.g., 6 entries = (1<<6)-1 = 63 = 0x3f) */
	int expected_mask = (1 << num_entries) - 1;
	TEST_ASSERT_EQUAL_INT(expected_mask, result);

	closedir(dp2);
	free(dirNames);

	rmdir(MAIN_DIR "/test_preserve/B");
	rmdir(MAIN_DIR "/test_preserve/CC");
	rmdir(MAIN_DIR "/test_preserve/DDDD");
	rmdir(MAIN_DIR "/test_preserve/EEEEEE");
	rmdir(MAIN_DIR "/test_preserve");
}


TEST(dirent_closedir, releases_file_descriptor)
{
	DIR *dp;

	long fd_limit = sysconf(_SC_OPEN_MAX);

	if (fd_limit <= 0) {
		/* value high enough to cause a potential leak */
		fd_limit = 2000;
	}

	long loop_count = fd_limit + 10;

	for (long i = 0; i < loop_count; i++) {
		dp = opendir(MAIN_DIR);
		TEST_ASSERT_NOT_NULL(dp);
		TEST_ASSERT_EQUAL_INT(0, closedir(dp));
	}
}


TEST_GROUP_RUNNER(dirent_closedir)
{
	RUN_TEST_CASE(dirent_closedir, closes_empty_dir);
	RUN_TEST_CASE(dirent_closedir, closes_non_empty_dir);
	RUN_TEST_CASE(dirent_closedir, preserves_content_after_use);
	RUN_TEST_CASE(dirent_closedir, releases_file_descriptor);
}
