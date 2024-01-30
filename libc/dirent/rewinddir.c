/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - dirent.h
 * TESTED:
 *    - rewinddir()
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

#include <string.h>
#include <fcntl.h>

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
	TEST_ASSERT_LESS_OR_EQUAL_INT(0, link(MAIN_DIR, MAIN_DIR "/hardlink"));
	TEST_ASSERT_EQUAL_INT(0, symlink(MAIN_DIR "/newdir", MAIN_DIR "/symlink"));

	rewinddir(dp);

	while (readdir(dp)) {
		counter2++;
	}

	TEST_ASSERT_EQUAL_INT(MAIN_DIR_INIT_CONTENTS + 3, counter2);

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


TEST_GROUP_RUNNER(dirent_rewinddir)
{
	RUN_TEST_CASE(dirent_rewinddir, rewinddir_basic);
	RUN_TEST_CASE(dirent_rewinddir, directory_contents_change);
}
