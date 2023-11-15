/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - dirent.h
 * TESTED:
 *    - readdir()
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
#include <signal.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <string.h>

#include <unity_fixture.h>

#include "common.h"
#include "dirent_helper_functions.h"

#define MAIN_DIR            "test_readdir"
#define INO_T_TEST_MAX_DIRS 10


TEST_GROUP(dirent_readdir);

TEST_SETUP(dirent_readdir)
{
	mkdir(MAIN_DIR, 0777);

	mkdir(MAIN_DIR "/dir1", 0777);
	mkdir(MAIN_DIR "/dir2", 0777);

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

	remove(MAIN_DIR "/file1.txt");
	remove(MAIN_DIR "/file2.dat");
	remove(MAIN_DIR "/file3.json");

	rmdir(MAIN_DIR);
}


TEST(dirent_readdir, long_name_directory_check)
{
	DIR *dp = test_opendir_asserted(MAIN_DIR);
	struct dirent *info;
	char longDirName[NAME_MAX];
	char longDirPath[NAME_MAX + 2 + sizeof(MAIN_DIR)];

	longDirName[NAME_MAX - 1] = '\0';

	TEST_ASSERT_NOT_NULL(dp);

	memset(longDirName, 'a', NAME_MAX - 1);
	sprintf(longDirPath, MAIN_DIR "/%s", longDirName);

	errno = 0;
	if (mkdir(longDirPath, 0777) == -1 && errno != EEXIST)
		TEST_IGNORE_MESSAGE(strerror(errno));

	while ((info = readdir(dp)) != NULL) {
		if (!strcmp(longDirName, info->d_name)) {
			TEST_ASSERT_EQUAL_INT(NAME_MAX - 1, strlen(info->d_name));
			closedir(dp);
			rmdir(longDirPath);
			TEST_PASS();
		}
	}

	closedir(dp);
	rmdir(longDirPath);
	TEST_FAIL();
}


TEST(dirent_readdir, basic_listing_count)
{
	DIR *dp = test_opendir_asserted(MAIN_DIR);
	int entry_counter = 0;
	struct dirent *info;

	if (!dp)
		TEST_FAIL_MESSAGE(strerror(errno));

	while ((info = readdir(dp)) != NULL)
		entry_counter++;

	/* 5 files from setup, and both . and .. directories */
	TEST_ASSERT_EQUAL_INT(7, entry_counter);

	closedir(dp);
}


TEST(dirent_readdir, reading_in_parent_and_child)
{
	DIR *dp1, *dp2;

	dp1 = test_opendir_asserted(MAIN_DIR "/dir1");
	dp2 = test_opendir_asserted(MAIN_DIR "/dir2");


	TEST_ASSERT_NOT_NULL(readdir(dp1));
	TEST_ASSERT_NOT_NULL(readdir(dp2));

	/* After first readdir is done, removing contents from directory shall not influence output from readdir */

	pid_t pid = fork();
	/* Since there are two different dir streams, there is no reading from the same stream in two processes */

	TEST_ASSERT_NOT_EQUAL_INT(-1, pid);

	if (pid) {
		/* Check for parent */
		TEST_ASSERT_NOT_NULL(readdir(dp1));
		rewinddir(dp1);
		TEST_ASSERT_NOT_NULL(readdir(dp1));

		closedir(dp1);
		closedir(dp2);
		wait(NULL);
	}

	else {
		/* Check for child */
		TEST_ASSERT_NOT_NULL(readdir(dp2));
		rewinddir(dp2);
		TEST_ASSERT_NOT_NULL(readdir(dp2));

		closedir(dp1);
		closedir(dp2);
		exit(EXIT_SUCCESS);
	}
}


TEST(dirent_readdir, hardlink_inode_correct_number)
{

	const char *originalFilePath = MAIN_DIR "/original_file.txt";

	const char *linkFilePath = MAIN_DIR "/linked_file.txt";

	/* Create the file */
	fclose(fopen(originalFilePath, "w+"));

	int fileLinkResult = link(originalFilePath, linkFilePath);

	TEST_ASSERT_EQUAL_INT(0, fileLinkResult);

	struct stat originalFileStat, linkFileStat;

	stat(originalFilePath, &originalFileStat);
	stat(linkFilePath, &linkFileStat);

	TEST_ASSERT_EQUAL_UINT64(originalFileStat.st_ino, linkFileStat.st_ino);

	remove(originalFilePath);
	unlink(linkFilePath);
}


TEST(dirent_readdir, distinct_inode_nums)
{
	ino_t inode_arr[INO_T_TEST_MAX_DIRS];
	DIR *dp = test_opendir_asserted(MAIN_DIR);
	struct dirent *info;
	int inode_counter = 0;

	/* Set all array members to -1 since all inode id's are positive */
	for (int i = 0; i < INO_T_TEST_MAX_DIRS; ++i) {
		inode_arr[i] = -1;
	}

	/* Assert distinct inodes */
	while ((info = readdir(dp)) != NULL) {
		TEST_ASSERT_EQUAL_INT(-1, d_ino_in(info->d_ino, inode_arr));
		inode_arr[inode_counter++] = info->d_ino;
	}

	closedir(dp);
}


TEST(dirent_readdir, same_file_reading_by_two_pointers)
{
	DIR *dp1, *dp2;
	dp1 = test_opendir_asserted(MAIN_DIR);
	dp2 = test_opendir_asserted(MAIN_DIR);

	int counter = 2;

	readdir(dp1);
	readdir(dp1);

	errno = 0;

	while (readdir(dp2) != NULL)
		continue;

	TEST_ASSERT_EQUAL_INT(0, errno);

	while (readdir(dp1) != NULL)
		counter++;

	TEST_ASSERT_EQUAL_INT(0, errno);

	TEST_ASSERT_EQUAL_INT(7, counter);

	closedir(dp1);
	closedir(dp2);
}


TEST(dirent_readdir, correct_dirent_names)
{
	struct dirent *info;
	int filename_bits = 0;
	DIR *dp = test_opendir_asserted(MAIN_DIR);

	while ((info = readdir(dp)) != NULL) {

		/* Set corresponding bit of filename_bits each time info->d_name is encountered */
		if (!strcmp(info->d_name, "dir1"))
			filename_bits |= 1;

		else if (!strcmp(info->d_name, "file1.txt"))
			filename_bits |= 2;

		else if (!strcmp(info->d_name, "file2.dat"))
			filename_bits |= 4;

		else if (!strcmp(info->d_name, "."))
			filename_bits |= 8;

		else if (!strcmp(info->d_name, "file3.json"))
			filename_bits |= 16;

		else if (!strcmp(info->d_name, ".."))
			filename_bits |= 32;

		else if (!strcmp(info->d_name, "dir2"))
			filename_bits |= 64;

		else
			TEST_FAIL_MESSAGE(info->d_name);
	}

	TEST_ASSERT_EQUAL_INT(0x7f, filename_bits);
	closedir(dp);
}


TEST_GROUP_RUNNER(dirent_readdir)
{
	RUN_TEST_CASE(dirent_readdir, basic_listing_count);
	RUN_TEST_CASE(dirent_readdir, correct_dirent_names);
	RUN_TEST_CASE(dirent_readdir, distinct_inode_nums);
	RUN_TEST_CASE(dirent_readdir, hardlink_inode_correct_number);
	RUN_TEST_CASE(dirent_readdir, same_file_reading_by_two_pointers);
	RUN_TEST_CASE(dirent_readdir, reading_in_parent_and_child);
	RUN_TEST_CASE(dirent_readdir, long_name_directory_check);
}
