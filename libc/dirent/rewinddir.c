#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include <unity_fixture.h>

#include "common.h"

#define MAIN_DIR "tested_files_rewinddir"


TEST_GROUP(rewinddir);

TEST_SETUP(rewinddir)
{
	mkdir(MAIN_DIR, 0777);
	mkdir(MAIN_DIR "/dir1", 0777);
	mkdir(MAIN_DIR "/dir2", 0777);
}


TEST_TEAR_DOWN(rewinddir)
{
	rmdir(MAIN_DIR "/dir1");
	rmdir(MAIN_DIR "/dir2");
	rmdir(MAIN_DIR);
}


TEST(rewinddir, reset_dirstream_position)
{
	DIR *dp = opendir(MAIN_DIR);
	int counter1 = 0, counter2 = 0, counter3 = 0;

	while (readdir(dp))
		counter1++;
	rewinddir(dp);


	while (readdir(dp))
		counter2++;

	TEST_ASSERT_EQUAL(counter1, counter2);

	mkdir(MAIN_DIR "/newdir", 0777);

	rewinddir(dp);
	while (readdir(dp))
		counter3++;

	TEST_ASSERT_NOT_EQUAL(counter1, counter3);
	rmdir(MAIN_DIR "/newdir");
	closedir(dp);
}

TEST_GROUP_RUNNER(rewinddir)
{

	RUN_TEST_CASE(rewinddir, reset_dirstream_position);
}