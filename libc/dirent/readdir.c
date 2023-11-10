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
#include <signal.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include <unity_fixture.h>

#include "common.h"

#define MAIN_DIR            "test_readdir"
#define INO_T_TEST_MAX_DIRS 10

int d_ino_in(ino_t arg, ino_t *arr)
{
	for (int i = 0; i < INO_T_TEST_MAX_DIRS; ++i)
		if (arg == arr[i])
			return i + 1;
	return 0;
}


TEST_GROUP(dirent_readdir);

TEST_SETUP(dirent_readdir)
{
	mkdir(MAIN_DIR, 0777);

	mkdir(MAIN_DIR "/dir1", 0777);
	mkdir(MAIN_DIR "/dir2", 0777);
	mkdir(MAIN_DIR "/dir3", 0777);

	mkdir(MAIN_DIR "/dir1/nest1", 0777);
	mkdir(MAIN_DIR "/dir1/nest2", 0777);

	mkdir(MAIN_DIR "/dir2/nest1", 0777);
	mkdir(MAIN_DIR "/dir2/nest2", 0777);

	FILE *files[] = {
		fopen(MAIN_DIR "/file1.txt", "w+"),
		fopen(MAIN_DIR "/file2.dat", "w+"),
		fopen(MAIN_DIR "/file3.json", "w+")
	};

	if (files[0]) {
		fprintf(files[0], "Some data");
		fprintf(files[1], "Some other data");


		fclose(files[0]);
		fclose(files[1]);
		fclose(files[2]);
	}
}


TEST_TEAR_DOWN(dirent_readdir)
{
	rmdir(MAIN_DIR "/dir1/nest1");
	rmdir(MAIN_DIR "/dir1/nest2");

	rmdir(MAIN_DIR "/dir2/nest1");
	rmdir(MAIN_DIR "/dir2/nest2");

	rmdir(MAIN_DIR "/dir1");
	rmdir(MAIN_DIR "/dir2");
	rmdir(MAIN_DIR "/dir3");

	remove(MAIN_DIR "/file1.txt");
	remove(MAIN_DIR "/file2.dat");
	remove(MAIN_DIR "/file3.json");

	rmdir(MAIN_DIR);
}


TEST(dirent_readdir, long_name_directory_check)
{
	errno = 0;
	DIR *dp = opendir(MAIN_DIR);
	struct dirent *info;

	if (!dp)
		TEST_FAIL_MESSAGE(strerror(errno));


	char longDirName[NAME_MAX + 1] = { 0 };
	longDirName[NAME_MAX] = '\0';

	memset(longDirName, 'a', NAME_MAX);
	char longDirPath[NAME_MAX + 2 + sizeof(MAIN_DIR)];
	sprintf(longDirPath, MAIN_DIR "/%s", longDirName);
	mkdir(longDirPath, 0777);

	int passed = 0;

	while (info = readdir(dp)) {
		if (!strcmp(longDirName, info->d_name)) {
			passed = 1;
		}
	}

	closedir(dp);
	rmdir(longDirPath);

	passed ? TEST_PASS() : TEST_FAIL();
}


TEST(dirent_readdir, basic_listing_count)
{
	DIR *dp = opendir(MAIN_DIR);
	int entry_counter = 0;
	struct dirent *info;

	if (!dp)
		TEST_FAIL_MESSAGE(strerror(errno));

	while (info = readdir(dp))
		entry_counter++;

	/* 6 files from setup, and both . and .. directories */
	TEST_ASSERT_EQUAL(8, entry_counter);

	closedir(dp);
}


TEST(dirent_readdir, reading_in_parent_and_child)
{

	DIR *dp1, *dp2;

	dp1 = opendir(MAIN_DIR "/dir1");
	dp2 = opendir(MAIN_DIR "/dir2");


	TEST_ASSERT_NOT_NULL(readdir(dp1));
	TEST_ASSERT_NOT_NULL(readdir(dp2));

	/* After first readdir is done, removing contents from directory shall not influence output from readdir */

	pid_t cid = fork();
	/* Since there are two different dir streams, there is no reading from the same stream in two processes */

	if (cid) {
		/* Check for parent */
		TEST_ASSERT_NOT_NULL(readdir(dp1));
		closedir(dp1);
		closedir(dp2);
	}

	else {
		/* Check for child */
		TEST_ASSERT_NOT_NULL(readdir(dp2));
		closedir(dp1);
		closedir(dp2);
		kill(getpid(), SIGTERM);
	}
}


TEST(dirent_readdir, symlink_inode_correct_number)
{
	mkdir("test", 0777);
	mkdir("a1", 0777);
	mkdir("a2", 0777);
	symlink("test", "a3");

	ino_t n1, n2;

	struct dirent *info;
	DIR *dp = opendir(".");

	while (info = readdir(dp)) {
		if (!strcmp("test", info->d_name)) {
			n1 = info->d_ino;
		}

		if (!strcmp("a3", info->d_name)) {
			n2 = info->d_ino;
		}
	}

	closedir(dp);

	TEST_ASSERT_NOT_EQUAL(n1, n2);

	rmdir("test");
	rmdir("a1");
	rmdir("a2");
	unlink("a3");
}


TEST(dirent_readdir, distinct_inode_nums)
{
	ino_t *inode_arr = calloc(INO_T_TEST_MAX_DIRS, sizeof(ino_t));
	DIR *dp = opendir(MAIN_DIR);
	struct dirent *info;
	int inode_counter = 0;

	/* assert distinct inodes */
	while (info = readdir(dp)) {
		TEST_ASSERT_EQUAL(0, d_ino_in(info->d_ino, inode_arr));
		inode_arr[inode_counter++] = info->d_ino;
	}
	closedir(dp);
	free(inode_arr);
}


TEST(dirent_readdir, same_file_reading_by_two_pointers)
{
	DIR *dp1, *dp2;
	dp1 = opendir(MAIN_DIR);
	dp2 = opendir(MAIN_DIR);

	int counter = 2;

	readdir(dp1);
	readdir(dp1);

	errno = 0;

	while (readdir(dp2))
		continue;

	TEST_ASSERT_EQUAL(0, errno);

	while (readdir(dp1))
		counter++;

	TEST_ASSERT_EQUAL(0, errno);

	TEST_ASSERT_EQUAL(8, counter);

	closedir(dp1);
	closedir(dp2);
}


TEST(dirent_readdir, reading_closed_dir_streams)
{
	DIR *dp = NULL;

	dp = opendir(MAIN_DIR);
	closedir(dp);
	errno = 0;
	TEST_ASSERT_NULL(readdir(dp));
	TEST_ASSERT_EQUAL(EBADF, errno);
	errno = 0;
}


TEST(dirent_readdir, correct_dirent_names)
{
	struct dirent *info;
	char filename_bits = 0;
	DIR *dp = opendir(MAIN_DIR);

	while (info = readdir(dp)) {

		/* Set corresponding bit of filename_bits each time info->d_name is encountered */
		if (!strcmp(info->d_name, "dir1"))
			filename_bits |= 1;

		if (!strcmp(info->d_name, "file1.txt"))
			filename_bits |= 2;

		if (!strcmp(info->d_name, "file2.dat"))
			filename_bits |= 4;

		if (!strcmp(info->d_name, "notExistingFile.jpg"))
			filename_bits |= 8;

		if (!strcmp(info->d_name, "file3.json"))
			filename_bits |= 16;
	}

	TEST_ASSERT_EQUAL(0b10111, filename_bits);
	closedir(dp);
}


TEST_GROUP_RUNNER(dirent_readdir)
{
	RUN_TEST_CASE(dirent_readdir, basic_listing_count);
	RUN_TEST_CASE(dirent_readdir, correct_dirent_names);
	RUN_TEST_CASE(dirent_readdir, distinct_inode_nums);
	RUN_TEST_CASE(dirent_readdir, symlink_inode_correct_number);
	RUN_TEST_CASE(dirent_readdir, same_file_reading_by_two_pointers);
	RUN_TEST_CASE(dirent_readdir, reading_in_parent_and_child);
	RUN_TEST_CASE(dirent_readdir, reading_closed_dir_streams)
	RUN_TEST_CASE(dirent_readdir, long_name_directory_check);
}
