/*
 * Phoenix-RTOS
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
#include <errno.h>
#include <fcntl.h>

#include <unity_fixture.h>

#include "common.h"
#include "dirent_helper_functions.h"

#define MAIN_DIR "test_opendir"

static int test_create_directories(int num_of_dirs)
{

	char dirPath[40];
	DIR *dirs[num_of_dirs];
	int opened_dirs = 0;
	int result = 0;

	/* Create directories in batch */
	for (int i = 0; i < num_of_dirs; ++i) {

		sprintf(dirPath, MAIN_DIR "/%d", i);

		TEST_MKDIR_ASSERTED(dirPath, S_IRUSR);
	}

	/* Open directories one by one, until one of them fails */
	for (int i = 0; i < num_of_dirs; ++i) {

		sprintf(dirPath, MAIN_DIR "/%d", i);

		/*
		 * Guard clause that skips current take if dir was opened,
		 * Upon failing it proceeds to cleanup
		 */
		if ((dirs[i] = opendir(dirPath)) != NULL) {
			opened_dirs = i;
			continue;
		}

		result = -1;
		break;
	}

	for (int i = 0; i <= opened_dirs; ++i) {
		TEST_ASSERT_EQUAL_INT(0, closedir(dirs[i]));
	}


	for (int i = 0; i < num_of_dirs; ++i) {
		sprintf(dirPath, MAIN_DIR "/%d", i);
		rmdir(dirPath);
	}

	return result;
}


TEST_GROUP(dirent_opendir);


TEST_SETUP(dirent_opendir)
{
	TEST_MKDIR_ASSERTED(MAIN_DIR, S_IRWXU);
}


TEST_TEAR_DOWN(dirent_opendir)
{
	rmdir(MAIN_DIR);
}


TEST(dirent_opendir, opening_empty_directory)
{
	DIR *dp;
	TEST_MKDIR_ASSERTED(MAIN_DIR "/empty_dir", S_IRUSR);
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
	TEST_IGNORE_MESSAGE("#937 issue");

	char unreadable[] = MAIN_DIR "/dir_without_read_perm";
	char readable[] = MAIN_DIR "/dir_without_read_perm/readable_dir";
	DIR *dirPtr;

	TEST_MKDIR_ASSERTED(unreadable, 0000);

	chmod(unreadable, S_IRWXU);
	TEST_MKDIR_ASSERTED(readable, S_IRUSR | S_IWUSR);
	chmod(unreadable, 0000);

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

	/* No execute permission in parent*/
	chmod(unreadable, S_IRUSR | S_IWUSR);
	errno = 0;
	dirPtr = opendir(readable);
	TEST_ASSERT_EQUAL_INT(EACCES, errno);
	TEST_ASSERT_NULL(dirPtr);

	/* No read permission */
	chmod(unreadable, S_IWUSR | S_IXUSR);
	errno = 0;
	dirPtr = opendir(unreadable);
	TEST_ASSERT_EQUAL_INT(EACCES, errno);
	TEST_ASSERT_NULL(dirPtr);


	errno = 0;
	chmod(unreadable, S_IRWXU);
	rmdir(readable);
	rmdir(unreadable);
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
	close(creat(MAIN_DIR "/notadir.txt", S_IRUSR));
	errno = 0;
	DIR *dirPtr = opendir(MAIN_DIR "/notadir.txt");
	TEST_ASSERT_EQUAL_INT(ENOTDIR, errno);
	TEST_ASSERT_NULL(dirPtr);
	remove(MAIN_DIR "/notadir.txt");
}


TEST(dirent_opendir, creating_dirs_in_closed_and_open_directories)
{
	/* Create dir in closed directory */
	DIR *dirs[4];
	TEST_MKDIR_ASSERTED(MAIN_DIR "/formerDir", S_IRUSR);

	/* Create dir in opened directory, then close opened one */
	DIR *dp = TEST_OPENDIR_ASSERTED(MAIN_DIR);
	TEST_ASSERT_NOT_NULL(dirs[0] = opendir(MAIN_DIR "/formerDir"));

	TEST_MKDIR_ASSERTED(MAIN_DIR "/latterDir", S_IRUSR);

	closedir(dp);

	/* Assure that both dirs can be opened without problems */
	TEST_ASSERT_NOT_NULL(dirs[1] = opendir(MAIN_DIR "/formerDir"));
	TEST_ASSERT_NOT_NULL(dirs[2] = opendir(MAIN_DIR "/latterDir"));

	dp = opendir(MAIN_DIR);
	TEST_ASSERT_NOT_NULL(dp);

	TEST_MKDIR_ASSERTED("ToBeDeleted", S_IRUSR);
	TEST_ASSERT_EQUAL_INT(0, rmdir("ToBeDeleted"));
	TEST_ASSERT_EQUAL_INT(0, closedir(dp));

	TEST_ASSERT_NULL(opendir("ToBeDeleted"));
	TEST_MKDIR_ASSERTED(MAIN_DIR "/evenLatterDir", S_IRUSR);
	TEST_ASSERT_NOT_NULL(dirs[3] = opendir(MAIN_DIR "/evenLatterDir"));

	for (int i = 0; i < 4; ++i) {
		TEST_ASSERT_NOT_NULL(dirs[i]);
		closedir(dirs[i]);
	}

	rmdir(MAIN_DIR "/formerDir");
	rmdir(MAIN_DIR "/latterDir");
	rmdir(MAIN_DIR "/evenLatterDir");
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
	int symloopMax = 0;
#if defined(SYMLOOP_MAX)
	symloopMax = SYMLOOP_MAX;
#elif defined(_SC_SYMLOOP_MAX)
	if ((symloopMax = sysconf(_SC_SYMLOOP_MAX)) == -1) {
		TEST_IGNORE_MESSAGE("sysconf() doesn't recognize _SC_SYMLOOP_MAX");
	}
#else
	TEST_IGNORE_MESSAGE("Neither SYMLOOP_MAX nor sysconf(_SC_SYMLOOP_MAX) is defined");
#endif

	TEST_MKDIR_ASSERTED("A", S_IRWXU);
	TEST_MKDIR_ASSERTED("D1", S_IRWXU);
	TEST_MKDIR_ASSERTED("D2", S_IRWXU);

	/* SYMLOOP_MAX probably won't be bigger than 40*/
	char selfLoop[40 * 2 + 16];
	char mutualLoop[40 * 4 + 32];

	TEST_ASSERT_EQUAL_INT(0, symlink("../D2", "D1/S1"));
	TEST_ASSERT_EQUAL_INT(0, symlink("../D1", "D2/S2"));
	TEST_ASSERT_EQUAL_INT(0, symlink(".", "A/B"));

	strcpy(selfLoop, "A");
	strcpy(mutualLoop, "D1");

	/* Create a path to a valid symloop */
	/* Posix says that symloops up to */
	for (int i = 0; i < 4; ++i) {
		strcat(selfLoop, "/B/B");
		strcat(mutualLoop, "/S1/S2");
	}

	errno = 0;
	DIR *selfDP = opendir(selfLoop);
	DIR *mutualDP = opendir(mutualLoop);

	TEST_ASSERT_NOT_NULL(selfDP);
	TEST_ASSERT_NOT_NULL(mutualDP);

	closedir(selfDP);
	closedir(mutualDP);


	/* Add a few layers of symloops, so it is too deep (selfLoop is not empty at this point)*/
	for (int i = 0; i < symloopMax / 2 - 1; ++i) {
		strcat(selfLoop, "/B/B");
		strcat(mutualLoop, "/S1/S2");
	}

	errno = 0;
	TEST_ASSERT_NULL(opendir(selfLoop));
	TEST_ASSERT_EQUAL_INT(ELOOP, errno);

	errno = 0;
	TEST_ASSERT_NULL(opendir(mutualLoop));
	TEST_ASSERT_EQUAL_INT(ELOOP, errno);


	unlink("A/B");
	unlink("D1/S1");
	unlink("D2/S2");

	rmdir("A");
	rmdir("D1");
	rmdir("D2");
}


TEST(dirent_opendir, opening_inside_open_directory)
{
	TEST_MKDIR_ASSERTED(MAIN_DIR "/newdir", S_IRUSR);
	DIR *dp1, *dp2;
	dp1 = opendir(MAIN_DIR);
	TEST_ASSERT_NOT_NULL(dp2 = opendir(MAIN_DIR "/newdir"));
	closedir(dp1);
	closedir(dp2);
	rmdir(MAIN_DIR "/newdir");
}


TEST(dirent_opendir, too_long_path)
{
	char filename[PATH_MAX + 1];
	/* Add 2 for null terminator and slash */
	char path[PATH_MAX + strlen(MAIN_DIR) + 2];

	memset(filename, 'a', PATH_MAX);
	filename[PATH_MAX] = '\0';
	strcpy(path, MAIN_DIR);

	strcat(path, "/");
	strcat(path, filename);

	errno = 0;
	TEST_ASSERT_NULL(opendir(path));
	TEST_ASSERT_EQUAL_INT(ENAMETOOLONG, errno);
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
	RUN_TEST_CASE(dirent_opendir, creating_dirs_in_closed_and_open_directories);
	RUN_TEST_CASE(dirent_opendir, opening_inside_open_directory);
	RUN_TEST_CASE(dirent_opendir, open_small_enough_number_of_directories);
	RUN_TEST_CASE(dirent_opendir, open_same_dir_multiple_times);
}
