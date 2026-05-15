/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - dirent.h
 * TESTED:
 *    - readdir()
 *
 * Copyright 2023-2026 Phoenix Systems
 * Authors: Arkadiusz Kozlowski, Lukasz Kruszynski
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
#include <fcntl.h>
#include <stdlib.h>

#include <unity_fixture.h>

#include "common.h"
#include "dirent_helper_functions.h"

#define MAIN_DIR            "test_readdir"
#define INO_T_TEST_MAX_DIRS 10


int d_ino_in(ino_t arg, ino_t *arr, int n)
{
	for (int i = 0; i < n; ++i) {
		if (arg == arr[i]) {
			return i;
		}
	}
	return -1;
}


TEST_GROUP(dirent_readdir);

TEST_SETUP(dirent_readdir)
{
	TEST_MKDIR_ASSERTED(MAIN_DIR, 0700);

	mkdir(MAIN_DIR "/dir1", S_IRUSR | S_IWUSR | S_IXUSR);
	mkdir(MAIN_DIR "/dir2", S_IRUSR | S_IWUSR | S_IXUSR);

	mkdir(MAIN_DIR "/dir1/nest1", S_IRUSR);
	mkdir(MAIN_DIR "/dir1/nest2", S_IRUSR);

	mkdir(MAIN_DIR "/dir2/nest1", S_IRUSR);
	mkdir(MAIN_DIR "/dir2/nest2", S_IRUSR);

	int files[] = {
		creat(MAIN_DIR "/file1.txt", S_IRUSR),
		creat(MAIN_DIR "/file2.dat", S_IRUSR),
		creat(MAIN_DIR "/file3.json", S_IRUSR)
	};

	close(files[0]);
	close(files[1]);
	close(files[2]);
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
	/* issue #1615: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/1615 */
	TEST_IGNORE_MESSAGE("#1615 issue");
	DIR *dp;
	struct dirent *info;
	char longDirName[NAME_MAX + 1];
	char longDirPath[NAME_MAX + 2 + sizeof(MAIN_DIR)];

	memset(longDirName, 'a', NAME_MAX);
	longDirName[NAME_MAX] = '\0';
	snprintf(longDirPath, sizeof(longDirPath), MAIN_DIR "/%s", longDirName);

	errno = 0;
	TEST_MKDIR_ASSERTED(longDirPath, S_IRUSR);

	dp = TEST_OPENDIR_ASSERTED(MAIN_DIR);
	TEST_ASSERT_NOT_NULL(dp);

	while ((info = readdir(dp)) != NULL) {
		if (info->d_name[0] == 'a') {

			if (strcmp(longDirName, info->d_name) != 0) {
				closedir(dp);
				rmdir(longDirPath);
				TEST_FAIL();
			}
			else {
				TEST_ASSERT_EQUAL_INT(NAME_MAX, strlen(info->d_name));
				closedir(dp);
				rmdir(longDirPath);
				TEST_PASS();
			}
		}
	}

	closedir(dp);
	rmdir(longDirPath);

	TEST_FAIL();
}


TEST(dirent_readdir, basic_listing_count)
{
	DIR *dp = TEST_OPENDIR_ASSERTED(MAIN_DIR);
	int entry_counter = 0;
	struct dirent *info;

	while ((info = readdir(dp)) != NULL) {
		entry_counter++;
	}

	/* 5 files from setup, and both . and .. directories */
	TEST_ASSERT_EQUAL_INT(7, entry_counter);

	closedir(dp);
}


TEST(dirent_readdir, reading_in_parent_and_child)
{
	DIR *dp1, *dp2;

	dp1 = TEST_OPENDIR_ASSERTED(MAIN_DIR "/dir1");
	dp2 = TEST_OPENDIR_ASSERTED(MAIN_DIR "/dir2");

	TEST_ASSERT_NOT_NULL(readdir(dp1));
	TEST_ASSERT_NOT_NULL(readdir(dp2));

	pid_t pid = fork();

	if (pid == -1) {
		closedir(dp1);
		closedir(dp2);
		TEST_IGNORE_MESSAGE("Fork failed");
	}

	/* Since there are two different dir streams, there is no reading from the same stream in two processes */
	if (pid > 0) {
		int cresult;
		/* Check for parent */
		TEST_ASSERT_NOT_NULL(readdir(dp1));
		rewinddir(dp1);
		TEST_ASSERT_NOT_NULL(readdir(dp1));
		closedir(dp1);
		closedir(dp2);
		wait(&cresult);
		TEST_ASSERT_TRUE(WIFEXITED(cresult));
		TEST_ASSERT_EQUAL_INT(EXIT_SUCCESS, WEXITSTATUS(cresult));
	}

	else {
		/* Check for child */
		int status = EXIT_SUCCESS;
		if (!readdir(dp2)) {
			status = EXIT_FAILURE;
		}

		rewinddir(dp2);

		if (!readdir(dp2)) {
			status = EXIT_FAILURE;
		}

		closedir(dp1);
		closedir(dp2);
		exit(status);
	}
}


TEST(dirent_readdir, hardlink_inode_correct_number)
{
	const char *originalFilePath = MAIN_DIR "/original_file.txt";
	const char *linkFilePath = MAIN_DIR "/linked_file.txt";

	fclose(fopen(originalFilePath, "w+"));

	TEST_ASSERT_EQUAL_INT(0, link(originalFilePath, linkFilePath));

	DIR *dp = TEST_OPENDIR_ASSERTED(MAIN_DIR);
	struct dirent *info;
	ino_t orig_ino = 0;
	ino_t link_ino = 0;
	int files_found = 0;

	while ((info = readdir(dp)) != NULL) {
		if (!strcmp(info->d_name, "original_file.txt")) {
			orig_ino = info->d_ino;
			files_found++;
		}
		else if (!strcmp(info->d_name, "linked_file.txt")) {
			link_ino = info->d_ino;
			files_found++;
		}
	}

	closedir(dp);

	TEST_ASSERT_EQUAL_INT(2, files_found);
	TEST_ASSERT_EQUAL_UINT64(orig_ino, link_ino);

	remove(originalFilePath);
	unlink(linkFilePath);
}


TEST(dirent_readdir, distinct_inode_nums)
{
	ino_t inode_arr[INO_T_TEST_MAX_DIRS];
	DIR *dp = TEST_OPENDIR_ASSERTED(MAIN_DIR);
	struct dirent *info;
	int inode_counter = 0;

	/* Assert distinct inodes */
	while ((info = readdir(dp)) != NULL) {
		TEST_ASSERT_LESS_THAN_INT(
				INO_T_TEST_MAX_DIRS,
				inode_counter);

		/* Pass inode_counter as 'n' to avoid reading garbage/uninitialized memory */
		TEST_ASSERT_EQUAL_INT(-1, d_ino_in(info->d_ino, inode_arr, inode_counter));
		inode_arr[inode_counter++] = info->d_ino;
	}

	closedir(dp);
}


TEST(dirent_readdir, same_file_reading_by_two_pointers)
{
	DIR *dp1, *dp2;
	dp1 = TEST_OPENDIR_ASSERTED(MAIN_DIR);
	dp2 = TEST_OPENDIR_ASSERTED(MAIN_DIR);

	int counter = 2;

	TEST_ASSERT_NOT_EQUAL(NULL, readdir(dp1));
	TEST_ASSERT_NOT_EQUAL(NULL, readdir(dp1));

	errno = 0;

	while (readdir(dp2) != NULL) {
		continue;
	}

	TEST_ASSERT_EQUAL_INT(0, errno);

	while (readdir(dp1) != NULL) {
		counter++;
	}

	TEST_ASSERT_EQUAL_INT(0, errno);

	TEST_ASSERT_EQUAL_INT(7, counter);

	closedir(dp1);
	closedir(dp2);
}


TEST(dirent_readdir, correct_dirent_names)
{
	struct dirent *info;
	int filename_bits = 0;
	DIR *dp = TEST_OPENDIR_ASSERTED(MAIN_DIR);

	while ((info = readdir(dp)) != NULL) {

		/* Set corresponding bit of filename_bits each time info->d_name is encountered */
		if (!strcmp(info->d_name, "dir1")) {

			filename_bits |= 1;
		}

		else if (!strcmp(info->d_name, "file1.txt")) {
			filename_bits |= 2;
		}

		else if (!strcmp(info->d_name, "file2.dat")) {
			filename_bits |= 4;
		}

		else if (!strcmp(info->d_name, ".")) {
			filename_bits |= 8;
		}

		else if (!strcmp(info->d_name, "file3.json")) {
			filename_bits |= 16;
		}

		else if (!strcmp(info->d_name, "..")) {
			filename_bits |= 32;
		}

		else if (!strcmp(info->d_name, "dir2")) {
			filename_bits |= 64;
		}

		else
			TEST_FAIL_MESSAGE(info->d_name);
	}

	TEST_ASSERT_EQUAL_INT(0x7f, filename_bits);
	closedir(dp);
}


TEST(dirent_readdir, read_past_end_of_stream)
{
	DIR *dp = TEST_OPENDIR_ASSERTED(MAIN_DIR);

	while (readdir(dp) != NULL) {
		continue;
	}

	errno = 0;
	for (int i = 0; i < 10; i++) {
		TEST_ASSERT_NULL(readdir(dp));
		TEST_ASSERT_EQUAL_INT(0, errno);
	}

	closedir(dp);
}


TEST(dirent_readdir, large_directory_pagination)
{
	const int NUM_FILES = 40;
	char *path = malloc(PATH_MAX);
	TEST_ASSERT_NOT_NULL(path);

	TEST_MKDIR_ASSERTED(MAIN_DIR "/stress_dir", 0700);

	for (int i = 0; i < NUM_FILES; i++) {
		sprintf(path, MAIN_DIR "/stress_dir/file_%d.txt", i);
		int fd = creat(path, S_IRUSR);
		TEST_ASSERT_NOT_EQUAL(-1, fd);
		close(fd);
	}

	DIR *dp = TEST_OPENDIR_ASSERTED(MAIN_DIR "/stress_dir");
	int count = 0;

	while (readdir(dp) != NULL) {
		count++;
	}

	closedir(dp);

	for (int i = 0; i < NUM_FILES; i++) {
		snprintf(path, PATH_MAX, MAIN_DIR "/stress_dir/file_%d.txt", i);
		remove(path);
	}
	rmdir(MAIN_DIR "/stress_dir");
	free(path);

	/* NUM_FILES + "." + ".." */
	TEST_ASSERT_EQUAL_INT(NUM_FILES + 2, count);
}


TEST(dirent_readdir, unlink_during_iteration)
{
	DIR *dp;
	struct dirent *info;
	const char *victim_path = MAIN_DIR "/victim.txt";

	int fd = creat(victim_path, S_IRUSR);
	TEST_ASSERT_NOT_EQUAL(-1, fd);
	close(fd);

	dp = TEST_OPENDIR_ASSERTED(MAIN_DIR);
	TEST_ASSERT_NOT_NULL(dp);

	info = readdir(dp);
	TEST_ASSERT_NOT_NULL(info);

	TEST_ASSERT_EQUAL_INT(0, remove(victim_path));

	errno = 0;
	while ((info = readdir(dp)) != NULL) {
		continue;
	}

	TEST_ASSERT_EQUAL_INT(0, errno);

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
	RUN_TEST_CASE(dirent_readdir, read_past_end_of_stream);
	RUN_TEST_CASE(dirent_readdir, large_directory_pagination);
	RUN_TEST_CASE(dirent_readdir, unlink_during_iteration);
}
