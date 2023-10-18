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

#define MAIN_DIR "tested_files_readdir"


TEST_GROUP(readdir);


TEST_SETUP(readdir)
{
	mkdir(MAIN_DIR, 0777);
	mkdir(MAIN_DIR "/dir1", 0777);
	mkdir(MAIN_DIR "/dir2", 0777);
	mkdir(MAIN_DIR "/dir3", 0777);

	mkdir(MAIN_DIR "/dir1/nest1", 0777);
	mkdir(MAIN_DIR "/dir1/nest2", 0777);


	FILE *files[] = {
		fopen(MAIN_DIR "/file1.txt", "w+"),
		fopen(MAIN_DIR "/file2.dat", "w+"),
		fopen(MAIN_DIR "/file3.json", "w+")
	};

	fclose(files[0]);
	fclose(files[1]);
	fclose(files[2]);
}


TEST_TEAR_DOWN(readdir)
{
	rmdir(MAIN_DIR "/dir1/nest1");
	rmdir(MAIN_DIR "/dir1/nest2");
	rmdir(MAIN_DIR "/dir1");
	rmdir(MAIN_DIR "/dir2");
	rmdir(MAIN_DIR "/dir3");
	remove(MAIN_DIR "/file1.txt");
	remove(MAIN_DIR "/file2.dat");
	remove(MAIN_DIR "/file3.json");
	rmdir(MAIN_DIR);
}


TEST(readdir, correct_dirent_names)
{
	struct dirent *info;
	char filename_bits = 0;
	DIR *dp = opendir(MAIN_DIR);

	while (info = readdir(dp)) {

		/* Set bits of filename_bits when each name is encountered */
		if (strcmp(info->d_name, "dir1"))
			filename_bits |= 1;

		if (strcmp(info->d_name, "file1.txt"))
			filename_bits |= 2;

		if (strcmp(info->d_name, "file2.dat"))
			filename_bits |= 4;

		if (strcmp(info->d_name, "file3.json"))
			filename_bits |= 8;
	}

	TEST_ASSERT_EQUAL(15, filename_bits & 15);
	closedir(dp);
}


TEST(readdir, basic_listing_count)
{
	DIR *dp = opendir(MAIN_DIR);
	int entry_counter = 0;

	while (readdir(dp)) {
		entry_counter++;
	}


	TEST_ASSERT_EQUAL(8, entry_counter);

	closedir(dp);
}


TEST_GROUP_RUNNER(readdir)
{
	RUN_TEST_CASE(readdir, basic_listing_count);
	RUN_TEST_CASE(readdir, correct_dirent_names);
}
