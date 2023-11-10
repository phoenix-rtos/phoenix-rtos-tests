/*
 * Phoenix-RTOS
 *
 * libphoenix
 *
 * test/test_strftime.c
 *
 * Copyright 2023 Phoenix Systems
 * Author: Arkadiusz Kozlowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <limits.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include <unity_fixture.h>

#include "common.h"

#define MAIN_DIR "test_rewinddir"


TEST_GROUP(dirent_rewinddir);

TEST_SETUP(dirent_rewinddir)
{
	mkdir(MAIN_DIR, 0777);
	mkdir(MAIN_DIR "/dir1", 0777);
	mkdir(MAIN_DIR "/dir2", 0777);
}


TEST_TEAR_DOWN(dirent_rewinddir)
{
	rmdir(MAIN_DIR "/dir1");
	rmdir(MAIN_DIR "/dir2");
	rmdir(MAIN_DIR);
}


TEST(dirent_rewinddir, reset_dirstream_position)
{
	DIR *dp = opendir(MAIN_DIR);
	int counter1 = 0, counter2 = 0, counter3 = 0, counter4 = 0;


	while (readdir(dp))
		counter1++;

	rewinddir(dp);

	while (readdir(dp))
		counter2++;

	TEST_ASSERT_EQUAL(counter1, counter2);
	TEST_ASSERT_EQUAL(4, counter1);

	mkdir(MAIN_DIR "/newdir", 0777);

	rewinddir(dp);
	while (readdir(dp))
		counter3++;

	closedir(dp);
	dp = opendir(MAIN_DIR);

	while (readdir(dp)) {
		counter4++;
	}

	TEST_ASSERT_EQUAL(5, counter3);
	TEST_ASSERT_EQUAL(counter3, counter4);

	closedir(dp);
	rmdir(MAIN_DIR "/newdir");
}

TEST_GROUP_RUNNER(dirent_rewinddir)
{

	RUN_TEST_CASE(dirent_rewinddir, reset_dirstream_position);
}
