/*
 * Phoenix-RTOS
 *
 * libphoenix
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - dirent.h
 * TESTED:
 *    - opendir()
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
#include <errno.h>

#include <unity_fixture.h>

#include "common.h"
#include "dirent_helper_functions.h"

#define MAIN_DIR "test_opendir"


TEST_GROUP(dirent_opendir);


TEST_SETUP(dirent_opendir)
{

	test_mkdir_asserted(MAIN_DIR, 0777);
	test_mkdir_asserted(MAIN_DIR "/dir_without_read_perm", 0000);
	FILE *fptr;
	if ((fptr = fopen(MAIN_DIR "/notadir.txt", "w")) != NULL) {
		fprintf(fptr, "Some file contents");
		fclose(fptr);
	}
}


TEST_TEAR_DOWN(dirent_opendir)
{
	remove(MAIN_DIR "/notadir.txt");
	chmod(MAIN_DIR "/dir_without_read_perm", 0777);
	rmdir(MAIN_DIR "/dir_without_read_perm");
	rmdir(MAIN_DIR);
}


TEST(dirent_opendir, opening_empty_directory)
{
	DIR *dp;
	test_mkdir_asserted(MAIN_DIR "/empty_dir", 0777);
	dp = opendir(MAIN_DIR "/empty_dir");
	TEST_ASSERT_NOT_NULL(dp);
	closedir(dp);
	rmdir(MAIN_DIR "/empty_dir");
}


TEST(dirent_opendir, opening_not_empty_directory)
{
	DIR *dp;
	dp = opendir(MAIN_DIR);
	TEST_ASSERT_NOT_NULL(dp);
	closedir(dp);
}


TEST(dirent_opendir, no_read_permission)
{
	char unreadable[] = MAIN_DIR "/dir_without_read_perm";
	char readable[] = MAIN_DIR "/dir_without_read_perm/readable_dir";
	DIR *dirPtr;

	chmod(unreadable, 0700);
	test_mkdir_asserted(readable, 0777);
	chmod(unreadable, 0000);

	if ((dirPtr = opendir(unreadable)) != NULL) {
		closedir(dirPtr);
		rmdir(readable);
		TEST_IGNORE_MESSAGE("Opened a file without any permissions");
	}

	/* Try to read from locked directory */
	errno = 0;
	dirPtr = opendir(unreadable);
	TEST_ASSERT_EQUAL_INT(EACCES, errno);
	TEST_ASSERT_NULL(dirPtr);

	/* Try to read from available directory inside locked directory */
	errno = 0;
	dirPtr = opendir(readable);
	TEST_ASSERT_EQUAL_INT(EACCES, errno);
	TEST_ASSERT_NULL(dirPtr);

	/* No execute permission */
	chmod(unreadable, 0600);
	errno = 0;
	dirPtr = opendir(readable);
	TEST_ASSERT_EQUAL_INT(EACCES, errno);
	TEST_ASSERT_NULL(dirPtr);

	/* No read permission */
	chmod(unreadable, 0300);
	errno = 0;
	dirPtr = opendir(unreadable);
	TEST_ASSERT_EQUAL_INT(EACCES, errno);
	TEST_ASSERT_NULL(dirPtr);


	errno = 0;
	chmod(unreadable, 0700);
	rmdir(readable);
	chmod(unreadable, 0000);
}


TEST(dirent_opendir, wrong_directory_name)
{
	errno = 0;
	DIR *dirPtr = opendir(MAIN_DIR "/not_existing_directory");
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);
	TEST_ASSERT_NULL(dirPtr);

	errno = 0;
	dirPtr = opendir("");
	TEST_ASSERT_NULL(dirPtr);
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);
}


TEST(dirent_opendir, not_a_directory)
{
	errno = 0;
	DIR *dirPtr = opendir(MAIN_DIR "/notadir.txt");
	TEST_ASSERT_EQUAL_INT(ENOTDIR, errno);
	TEST_ASSERT_NULL(dirPtr);
}


TEST(dirent_opendir, creating_dirs_in_closed_and_open_directories)
{
	/* Create dir in closed directory */
	DIR *dirs[4];
	test_mkdir_asserted(MAIN_DIR "/formerDir", 0777);

	/* Create dir in opened directory, then close opened one */
	DIR *dp = opendir(MAIN_DIR);
	TEST_ASSERT_NOT_NULL(dirs[0] = opendir(MAIN_DIR "/formerDir"));

	test_mkdir_asserted(MAIN_DIR "/latterDir", 0777);

	closedir(dp);

	/* Assure that both dirs can be opened without problems */
	TEST_ASSERT_NOT_NULL(dirs[1] = opendir(MAIN_DIR "/formerDir"));
	TEST_ASSERT_NOT_NULL(dirs[2] = opendir(MAIN_DIR "/latterDir"));
	dp = opendir(MAIN_DIR);
	closedir(dp);
	test_mkdir_asserted(MAIN_DIR "/evenLatterDir", 0777);
	TEST_ASSERT_NOT_NULL(dirs[3] = opendir(MAIN_DIR "/evenLatterDir"));

	for (int i = 0; i < 4; ++i)
		closedir(dirs[i]);

	rmdir(MAIN_DIR "/formerDir");
	rmdir(MAIN_DIR "/latterDir");
	rmdir(MAIN_DIR "/evenLatterDir");
}


TEST(dirent_opendir, open_too_many_directories)
{
#ifdef OPEN_MAX
	int dir_amount = OPEN_MAX + 100;
	errno = 0;

	TEST_ASSERT_EQUAL_INT(-1, test_create_directories(dir_amount));
	TEST_ASSERT_EQUAL_INT(EMFILE, errno);
#else
	TEST_IGNORE_MESSAGE("OPEN_MAX not defined");
#endif
}


TEST(dirent_opendir, open_small_enough_number_of_directories)
{
	errno = 0;

	TEST_ASSERT_EQUAL_INT(0, test_create_directories(20));
}


TEST(dirent_opendir, open_same_dir_multiple_times)
{
	DIR *dp1, *dp2, *dp3;

	dp1 = opendir(MAIN_DIR);
	dp2 = opendir(MAIN_DIR);
	dp3 = opendir(MAIN_DIR);

	TEST_ASSERT_NOT_EQUAL(dp1, dp2);
	TEST_ASSERT_NOT_EQUAL(dp2, dp3);
	TEST_ASSERT_NOT_EQUAL(dp1, dp3);

	closedir(dp1);
	closedir(dp2);
	closedir(dp3);
}


TEST(dirent_opendir, symlink_loop)
{
#ifndef SYMLOOP_MAX
#define SYMLOOP_MAX 8
#define SYMLOOP_NOT_DEFINED
#endif

	test_mkdir_asserted("A", 0777);

	TEST_ASSERT_EQUAL(0, symlink("A", "D"));
	TEST_ASSERT_EQUAL(0, symlink(".", "A/D"));

	char loopPath[SYMLOOP_MAX * 2 + 16];
	strcpy(loopPath, "A/");

	/* Create a path to barely valid symloop */
	for (int i = 0; i < SYMLOOP_MAX / 2 - 1; ++i) {
		strcat(loopPath, "D/D/");
	}


	DIR *dp = opendir(loopPath);
	TEST_ASSERT_NOT_NULL(dp);
	rmdir(loopPath);
	closedir(dp);

/* Check for actually defined symloop */
#ifndef SYMLOOP_NOT_DEFINED

	/* Add a few layers of symloops, so it is too deep */
	for (int i = 0; i < 4; ++i) {
		strcat(loopPath, "D/D/");
	}

	errno = 0;

	TEST_ASSERT_NULL(opendir(loopPath));
	TEST_ASSERT_EQUAL_INT(ELOOP, errno);

#endif

	unlink("A/D");
	unlink("D");
	rmdir("A/D");
	rmdir("A");
}


TEST(dirent_opendir, opening_inside_open_directory)
{
	test_mkdir_asserted(MAIN_DIR "/newdir", 0777);
	DIR *dp1, *dp2;
	dp1 = opendir(MAIN_DIR);
	TEST_ASSERT_NOT_NULL(dp2 = opendir(MAIN_DIR "/newdir"));
	closedir(dp1);
	closedir(dp2);
	rmdir(MAIN_DIR "/newdir");
}


TEST(dirent_opendir, too_long_path)
{
	int size = PATH_MAX;
	char filename[size + 1];
	char path[size + strlen(MAIN_DIR) + 100];


	memset(filename, 'a', size - strlen(MAIN_DIR) - 3);
	strcpy(path, MAIN_DIR);
	strcat(path, "/");
	strcat(path, filename);
	mkdir(path, 0777);

	errno = 0;
	TEST_ASSERT_NULL(opendir(path));
	TEST_ASSERT_EQUAL_INT(ENAMETOOLONG, errno);
}


TEST(dirent_opendir, preserving_content_after_closedir)
{
	test_mkdir_asserted("test_preserve", 0777);
	test_mkdir_asserted("test_preserve/B", 0777);
	test_mkdir_asserted("test_preserve/CC", 0777);
	test_mkdir_asserted("test_preserve/DDDD", 0777);
	test_mkdir_asserted("test_preserve/EEEEEE", 0777);

	char dirNames[7][10];
	ino_t inodes[7];
	struct dirent *info;
	DIR *dp1 = opendir("test_preserve");
	char result = 0;

	/*
	 *Create an array with names of dirs.
	 *Indexes will be used to associate name of each directory with a bit
	 */
	{
		int i = 0;
		while ((info = readdir(dp1)) != NULL) {
			inodes[i] = info->d_ino;
			strcpy(dirNames[i++], info->d_name);
		}
	}

	closedir(dp1);
	DIR *dp2 = opendir("test_preserve");
	rewinddir(dp2);

	/*
	 * Map each directory to a bit.
	 * In case of fail set most left bit to 1.
	 * Assert every bit is high except the first two.
	 */

	/* Go through each entry */

	while ((info = readdir(dp2)) != NULL) {

		int found = 0;
		char name[10];
		strcpy(name, info->d_name);

		/* determine the index of given name */
		for (int i = 0; i < 7; ++i) {

			if (!strcmp(name, dirNames[i])) {

				TEST_ASSERT_EQUAL_INT64(inodes[i], info->d_ino);

				/* Set the index bit of found name */
				result |= 1 << i;

				found = 1;
			}
		}

		/* Name found, time to search for another name */
		if (found)
			continue;

		TEST_FAIL();
	}

	/* result variable should be 0b00111111, which is 63 */
	TEST_ASSERT_EQUAL_INT(63, result);

	closedir(dp2);

	rmdir("test_preserve/B");
	rmdir("test_preserve/CC");
	rmdir("test_preserve/DDDD");
	rmdir("test_preserve/EEEEEE");
	rmdir("test_preserve");
}


TEST_GROUP_RUNNER(dirent_opendir)
{
	RUN_TEST_CASE(dirent_opendir, opening_empty_directory);
	RUN_TEST_CASE(dirent_opendir, opening_not_empty_directory);
	RUN_TEST_CASE(dirent_opendir, no_read_permission);
	RUN_TEST_CASE(dirent_opendir, wrong_directory_name);
	RUN_TEST_CASE(dirent_opendir, not_a_directory);
	RUN_TEST_CASE(dirent_opendir, symlink_loop);
	RUN_TEST_CASE(dirent_opendir, too_long_path);
	RUN_TEST_CASE(dirent_opendir, opening_inside_open_directory);
	RUN_TEST_CASE(dirent_opendir, open_small_enough_number_of_directories);
	RUN_TEST_CASE(dirent_opendir, open_too_many_directories);
	RUN_TEST_CASE(dirent_opendir, preserving_content_after_closedir);
	RUN_TEST_CASE(dirent_opendir, open_same_dir_multiple_times);
	RUN_TEST_CASE(dirent_opendir, creating_dirs_in_closed_and_open_directories);
}
