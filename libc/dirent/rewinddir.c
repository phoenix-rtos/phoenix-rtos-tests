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
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <unity_fixture.h>

#include "common.h"

#define MAIN_DIR               "test_rewinddir"
#define MAIN_DIR_INIT_CONTENTS 4


TEST_GROUP(dirent_rewinddir);

TEST_SETUP(dirent_rewinddir)
{
	mkdir(MAIN_DIR, 0777);
	mkdir(MAIN_DIR "/dir1", 0777);
	mkdir(MAIN_DIR "/dir2", 0777);
	mkdir(MAIN_DIR "/dir2/nestDir", 0777);
}


TEST_TEAR_DOWN(dirent_rewinddir)
{
	rmdir(MAIN_DIR "/newdir");
	unlink(MAIN_DIR "/hardlink");
	unlink(MAIN_DIR "/symlink");
	remove(MAIN_DIR "/textfile.txt");
	rmdir(MAIN_DIR "/dir2/nestDir");
	rmdir(MAIN_DIR "/dir1");
	rmdir(MAIN_DIR "/dir2");
	rmdir(MAIN_DIR);
}


TEST(dirent_rewinddir, reset_dirstream_position)
{
	struct stat bufBefore, bufAfter;
	DIR *dp = opendir(MAIN_DIR);
	int counter1 = 0, counter2 = 0, counter3 = 0, counter4 = 0;

	TEST_ASSERT_NOT_NULL(dp);

	stat(MAIN_DIR, &bufBefore);

	while (readdir(dp))
		counter1++;

	rewinddir(dp);

	while (readdir(dp))
		counter2++;

	TEST_ASSERT_EQUAL_INT(counter1, counter2);
	TEST_ASSERT_EQUAL_INT(MAIN_DIR_INIT_CONTENTS, counter1);

	rewinddir(dp);

	stat(MAIN_DIR, &bufAfter);

	TEST_ASSERT_EQUAL(bufBefore.st_blksize, bufAfter.st_blksize);
	TEST_ASSERT_EQUAL(bufBefore.st_blocks, bufAfter.st_blocks);
	TEST_ASSERT_EQUAL(bufBefore.st_dev, bufAfter.st_dev);
	TEST_ASSERT_EQUAL(bufBefore.st_ino, bufAfter.st_ino);
	TEST_ASSERT_EQUAL(bufBefore.st_mode, bufAfter.st_mode);


	mkdir(MAIN_DIR "/newdir", 0777);
	fclose(fopen(MAIN_DIR "/textfile.txt", "w+"));
	TEST_ASSERT_LESS_OR_EQUAL(0, link(MAIN_DIR, MAIN_DIR "/hardlink"));
	TEST_ASSERT_EQUAL(0, symlink(MAIN_DIR "/newdir", MAIN_DIR "/symlink"));

	while (readdir(dp))
		counter3++;

	closedir(dp);
	dp = opendir(MAIN_DIR);

	while (readdir(dp)) {
		counter4++;
	}

	TEST_ASSERT_EQUAL_INT(MAIN_DIR_INIT_CONTENTS + 3, counter3);
	TEST_ASSERT_EQUAL_INT(counter3, counter4);

	closedir(dp);
}

TEST_GROUP_RUNNER(dirent_rewinddir)
{
	RUN_TEST_CASE(dirent_rewinddir, reset_dirstream_position);
}
