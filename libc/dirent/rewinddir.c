/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - dirent.h
 * TESTED:
 *    - rewinddir()
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

#include <string.h>
#include <fcntl.h>
#include <stdlib.h>

#include <unity_fixture.h>
#include "dirent_helper_functions.h"

#include "common.h"

#define MAIN_DIR               "test_rewinddir"
#define MAIN_DIR_INIT_CONTENTS 4


TEST_GROUP(dirent_rewinddir);

TEST_SETUP(dirent_rewinddir)
{
	TEST_MKDIR_ASSERTED(MAIN_DIR, S_IRWXU);
	TEST_MKDIR_ASSERTED(MAIN_DIR "/dir1", S_IRUSR);
	TEST_MKDIR_ASSERTED(MAIN_DIR "/dir2", S_IRWXU);
	TEST_MKDIR_ASSERTED(MAIN_DIR "/dir2/nestDir", S_IRUSR);
}


TEST_TEAR_DOWN(dirent_rewinddir)
{
	rmdir(MAIN_DIR "/dir2/nestDir");
	rmdir(MAIN_DIR "/dir1");
	rmdir(MAIN_DIR "/dir2");
	rmdir(MAIN_DIR);
}


TEST(dirent_rewinddir, rewinddir_basic)
{
	struct stat bufBefore, bufAfter;
	DIR *dp = opendir(MAIN_DIR);
	int counter1, counter2;
	counter1 = counter2 = 0;


	TEST_ASSERT_NOT_NULL(dp);

	stat(MAIN_DIR, &bufBefore);

	while (readdir(dp)) {
		counter1++;
	}

	rewinddir(dp);

	while (readdir(dp)) {
		counter2++;
	}

	TEST_ASSERT_EQUAL_INT(counter1, counter2);
	TEST_ASSERT_EQUAL_INT(MAIN_DIR_INIT_CONTENTS, counter1);

	rewinddir(dp);

	stat(MAIN_DIR, &bufAfter);

	TEST_ASSERT_EQUAL(bufBefore.st_blksize, bufAfter.st_blksize);
	TEST_ASSERT_EQUAL(bufBefore.st_blocks, bufAfter.st_blocks);
	TEST_ASSERT_EQUAL(bufBefore.st_dev, bufAfter.st_dev);
	TEST_ASSERT_EQUAL(bufBefore.st_ino, bufAfter.st_ino);
	TEST_ASSERT_EQUAL(bufBefore.st_mode, bufAfter.st_mode);

	closedir(dp);
}


TEST(dirent_rewinddir, directory_contents_change)
{
	DIR *dp = opendir(MAIN_DIR);
	int counter1, counter2, counter3;
	counter1 = counter2 = counter3 = 0;

	while (readdir(dp)) {
		counter1++;
	}

	TEST_ASSERT_EQUAL_INT(MAIN_DIR_INIT_CONTENTS, counter1);

	mkdir(MAIN_DIR "/newdir", S_IRUSR);
	close(creat(MAIN_DIR "/textfile.txt", S_IRUSR | S_IWUSR));
	TEST_ASSERT_EQUAL_INT(0, link(MAIN_DIR "/textfile.txt", MAIN_DIR "/hardlink"));
	TEST_ASSERT_EQUAL_INT(0, symlink(MAIN_DIR "/newdir", MAIN_DIR "/symlink"));

	rewinddir(dp);

	while (readdir(dp)) {
		counter2++;
	}

	TEST_ASSERT_EQUAL_INT(MAIN_DIR_INIT_CONTENTS + 4, counter2);

	rmdir(MAIN_DIR "/newdir");
	unlink(MAIN_DIR "/hardlink");
	unlink(MAIN_DIR "/symlink");
	remove(MAIN_DIR "/textfile.txt");
	rewinddir(dp);

	while (readdir(dp)) {
		counter3++;
	}

	TEST_ASSERT_EQUAL_INT(MAIN_DIR_INIT_CONTENTS, counter3);
	closedir(dp);
}

TEST(dirent_rewinddir, rewinddir_mid_stream)
{
	DIR *dp = opendir(MAIN_DIR);
	struct dirent *info;
	char first_entry[NAME_MAX + 1];

	TEST_ASSERT_NOT_NULL(dp);

	info = readdir(dp);
	TEST_ASSERT_NOT_NULL(info);
	snprintf(first_entry, sizeof(first_entry), "%s", info->d_name);

	TEST_ASSERT_NOT_NULL(readdir(dp));

	errno = 0;
	rewinddir(dp);
	TEST_ASSERT_EQUAL_INT(0, errno);

	info = readdir(dp);
	TEST_ASSERT_NOT_NULL(info);
	TEST_ASSERT_EQUAL_STRING(first_entry, info->d_name);

	closedir(dp);
}


TEST(dirent_rewinddir, rewinddir_multiple_consecutive)
{
	DIR *dp = opendir(MAIN_DIR);
	struct dirent *info;
	char first_entry[NAME_MAX + 1];

	TEST_ASSERT_NOT_NULL(dp);

	info = readdir(dp);
	TEST_ASSERT_NOT_NULL(info);
	snprintf(first_entry, sizeof(first_entry), "%s", info->d_name);

	TEST_ASSERT_NOT_NULL(readdir(dp));
	TEST_ASSERT_NOT_NULL(readdir(dp));

	for (int i = 0; i < 10; i++) {
		rewinddir(dp);
	}

	info = readdir(dp);
	TEST_ASSERT_NOT_NULL(info);
	TEST_ASSERT_EQUAL_STRING(first_entry, info->d_name);

	closedir(dp);
}


TEST(dirent_rewinddir, rewinddir_independent_streams)
{
	DIR *dp1 = opendir(MAIN_DIR);
	DIR *dp2 = opendir(MAIN_DIR);
	struct dirent *info2;

	TEST_ASSERT_NOT_NULL(dp1);
	TEST_ASSERT_NOT_NULL(dp2);

	info2 = readdir(dp2);
	TEST_ASSERT_NOT_NULL(info2);

	info2 = readdir(dp2);
	TEST_ASSERT_NOT_NULL(info2);
	char second_entry[NAME_MAX + 1];
	snprintf(second_entry, sizeof(second_entry), "%s", info2->d_name);

	while (readdir(dp1) != NULL) { }
	rewinddir(dp1);

	info2 = readdir(dp2);
	TEST_ASSERT_NOT_NULL(info2);

	TEST_ASSERT_NOT_EQUAL(0, strcmp(second_entry, info2->d_name));

	closedir(dp1);
	closedir(dp2);
}


TEST(dirent_rewinddir, rewind_empty_dir)
{
	TEST_MKDIR_ASSERTED(MAIN_DIR "/emptydir", 0700);
	DIR *dp = opendir(MAIN_DIR "/emptydir");
	TEST_ASSERT_NOT_NULL(dp);

	int count1 = 0, count2 = 0;

	while (readdir(dp) != NULL) {
		count1++;
	}

	rewinddir(dp);

	while (readdir(dp) != NULL) {
		count2++;
	}

	TEST_ASSERT_EQUAL_INT(count1, count2);
	TEST_ASSERT_EQUAL_INT(2, count1);

	closedir(dp);
	rmdir(MAIN_DIR "/emptydir");
}


TEST(dirent_rewinddir, rewind_large_dir_pagination)
{
	const int NUM_FILES = 40;
	char *path = malloc(PATH_MAX);
	TEST_ASSERT_NOT_NULL(path);

	TEST_MKDIR_ASSERTED(MAIN_DIR "/pagedir", 0700);

	for (int i = 0; i < NUM_FILES; i++) {
		snprintf(path, PATH_MAX, MAIN_DIR "/pagedir/file_%d.txt", i);
		close(creat(path, S_IRUSR));
	}

	DIR *dp = opendir(MAIN_DIR "/pagedir");
	TEST_ASSERT_NOT_NULL(dp);

	struct dirent *info = readdir(dp);
	TEST_ASSERT_NOT_NULL(info);
	char first_entry[NAME_MAX + 1];
	snprintf(first_entry, sizeof(first_entry), "%s", info->d_name);

	while (readdir(dp) != NULL) { }

	rewinddir(dp);

	info = readdir(dp);
	TEST_ASSERT_NOT_NULL(info);
	TEST_ASSERT_EQUAL_STRING(first_entry, info->d_name);

	closedir(dp);

	for (int i = 0; i < NUM_FILES; i++) {
		snprintf(path, PATH_MAX, MAIN_DIR "/pagedir/file_%d.txt", i);
		remove(path);
	}
	rmdir(MAIN_DIR "/pagedir");
	free(path);
}


TEST(dirent_rewinddir, rewind_unlinked_dir)
{
	TEST_MKDIR_ASSERTED(MAIN_DIR "/ghostdir", 0700);
	DIR *dp = opendir(MAIN_DIR "/ghostdir");
	TEST_ASSERT_NOT_NULL(dp);

	TEST_ASSERT_EQUAL_INT(0, rmdir(MAIN_DIR "/ghostdir"));

	errno = 0;

	while (readdir(dp) != NULL) { }

	TEST_ASSERT_EQUAL_INT(errno, 0);
	rewinddir(dp);

	void *ret = readdir(dp);
	(void)ret;
	closedir(dp);
}


TEST_GROUP_RUNNER(dirent_rewinddir)
{
	RUN_TEST_CASE(dirent_rewinddir, rewinddir_basic);
	RUN_TEST_CASE(dirent_rewinddir, directory_contents_change);
	RUN_TEST_CASE(dirent_rewinddir, rewinddir_mid_stream);
	RUN_TEST_CASE(dirent_rewinddir, rewinddir_multiple_consecutive);
	RUN_TEST_CASE(dirent_rewinddir, rewinddir_independent_streams);
	RUN_TEST_CASE(dirent_rewinddir, rewind_empty_dir);
	RUN_TEST_CASE(dirent_rewinddir, rewind_large_dir_pagination);
	RUN_TEST_CASE(dirent_rewinddir, rewind_unlinked_dir);
}
