/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - dirent.h
 * TESTED:
 *    - opendir()
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
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>

#include <unity_fixture.h>

#include "common.h"
#include "dirent_helper_functions.h"

#define MAIN_DIR     "test_opendir"
#define THREAD_COUNT 4

static int test_create_directories(int num_of_dirs)
{

	char dirPath[40];
	DIR *dirs[num_of_dirs];
	int opened_dirs = -1;
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


static void *thread_opendir(void *arg)
{
	DIR *dp = opendir(MAIN_DIR);
	return (void *)dp;
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


TEST(dirent_opendir, open_enough_dirs_to_force_error)
{
	/*issue #1610: https://github.com/issues/created?issue=phoenix-rtos%7Cphoenix-rtos-project%7C1610 */
	TEST_IGNORE_MESSAGE("#1610 issue");

	DIR **dirs;
	long max_open_dirs = 0;
	long max_fds = 0;
	char err_msg_buff[128];

#if defined(_SC_OPEN_MAX)
	max_fds = sysconf(_SC_OPEN_MAX);
#endif

	TEST_ASSERT_GREATER_THAN(0, max_fds);

	long max_fds_over_limit = max_fds + 10;

	dirs = (DIR **)calloc(max_fds_over_limit + 1, sizeof(DIR *));
	TEST_ASSERT_NOT_NULL(dirs);

	errno = 0;


	while (max_open_dirs < max_fds_over_limit) {
		dirs[max_open_dirs] = opendir(MAIN_DIR);
		if (dirs[max_open_dirs] == NULL) {
			break;
		}
		max_open_dirs++;
	}

	if (max_open_dirs > max_fds) {
		for (long i = 0; i < max_open_dirs; ++i) {
			closedir(dirs[i]);
		}
		free((void *)dirs);
		snprintf(err_msg_buff, sizeof(err_msg_buff), "Managed to open %ld dirs despite the %ld file descriptors limit", max_open_dirs, max_fds);
		TEST_FAIL_MESSAGE(err_msg_buff);
	}

	TEST_ASSERT_NULL(dirs[max_open_dirs]);

	if (errno != EMFILE && errno != ENFILE) {
		for (long i = 0; i < max_open_dirs; ++i) {
			closedir(dirs[i]);
		}
		free((void *)dirs);
		TEST_FAIL_MESSAGE("Expected errno to be EMFILE or ENFILE upon exhausting file descriptors");
	}

	for (long i = 0; i < max_open_dirs; ++i) {
		TEST_ASSERT_EQUAL_INT(0, closedir(dirs[i]));
	}

	free((void *)dirs);
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


TEST(dirent_opendir, open_inside_open_directory)
{
	TEST_MKDIR_ASSERTED(MAIN_DIR "/newdir", S_IRUSR);
	DIR *dp1, *dp2;
	dp1 = opendir(MAIN_DIR);
	TEST_ASSERT_NOT_NULL(dp1);

	dp2 = opendir(MAIN_DIR "/newdir");
	TEST_ASSERT_NOT_NULL(dp2);

	closedir(dp1);
	closedir(dp2);
	rmdir(MAIN_DIR "/newdir");
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

	TEST_MKDIR_ASSERTED(MAIN_DIR "/A", S_IRWXU);
	TEST_MKDIR_ASSERTED(MAIN_DIR "/D1", S_IRWXU);
	TEST_MKDIR_ASSERTED(MAIN_DIR "/D2", S_IRWXU);

	/* SYMLOOP_MAX probably won't be bigger than 40*/
	char *selfLoop = malloc(PATH_MAX);
	char *mutualLoop = malloc(PATH_MAX);

	TEST_ASSERT_NOT_NULL(selfLoop);
	TEST_ASSERT_NOT_NULL(mutualLoop);

	TEST_ASSERT_EQUAL_INT(0, symlink("../D2", MAIN_DIR "/D1/S1"));
	TEST_ASSERT_EQUAL_INT(0, symlink("../D1", MAIN_DIR "/D2/S2"));
	TEST_ASSERT_EQUAL_INT(0, symlink(".", MAIN_DIR "/A/B"));

	strcpy(selfLoop, MAIN_DIR "/A");
	strcpy(mutualLoop, MAIN_DIR "/D1");

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


	/* Add a few layers of symloops, so it is too deep (selfLoop is not empty at this point) */
	for (int i = 0; i < symloopMax / 2 - 1; ++i) {
		if (strlen(selfLoop) + 10 >= PATH_MAX || strlen(mutualLoop) + 10 >= PATH_MAX) {
			break;
		}
		strcat(selfLoop, "/B/B");
		strcat(mutualLoop, "/S1/S2");
	}

	errno = 0;
	TEST_ASSERT_NULL(opendir(selfLoop));
	TEST_ASSERT_EQUAL_INT(ELOOP, errno);

	errno = 0;
	TEST_ASSERT_NULL(opendir(mutualLoop));
	TEST_ASSERT_EQUAL_INT(ELOOP, errno);

	unlink(MAIN_DIR "/A/B");
	unlink(MAIN_DIR "/D1/S1");
	unlink(MAIN_DIR "/D2/S2");

	rmdir(MAIN_DIR "/A");
	rmdir(MAIN_DIR "/D1");
	rmdir(MAIN_DIR "/D2");

	free(selfLoop);
	free(mutualLoop);
}


TEST(dirent_opendir, symlink_leading_nowhere)
{
	TEST_ASSERT_EQUAL_INT(0, symlink(MAIN_DIR "/does_not_exist", MAIN_DIR "/dangling_link"));

	errno = 0;
	DIR *dp = opendir(MAIN_DIR "/dangling_link");

	TEST_ASSERT_NULL(dp);
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);

	unlink(MAIN_DIR "/dangling_link");
}


TEST(dirent_opendir, symlink_to_file)
{
	close(creat(MAIN_DIR "/target_file.txt", S_IRUSR));
	TEST_ASSERT_EQUAL_INT(0, symlink("target_file.txt", MAIN_DIR "/file_link"));

	errno = 0;
	DIR *dp = opendir(MAIN_DIR "/file_link");

	TEST_ASSERT_NULL(dp);
	TEST_ASSERT_EQUAL_INT(ENOTDIR, errno);

	unlink(MAIN_DIR "/file_link");
	remove(MAIN_DIR "/target_file.txt");
}


TEST(dirent_opendir, trailing_slashes)
{
	DIR *dp;
	errno = 0;
	dp = opendir(MAIN_DIR "///");
	TEST_ASSERT_NOT_NULL(dp);
	TEST_ASSERT_EQUAL_INT(0, closedir(dp));


	TEST_MKDIR_ASSERTED(MAIN_DIR "/nested", S_IRWXU);
	errno = 0;
	dp = opendir(MAIN_DIR "///nested///");
	TEST_ASSERT_NOT_NULL(dp);
	TEST_ASSERT_EQUAL_INT(0, closedir(dp));

	rmdir(MAIN_DIR "/nested");
}


TEST(dirent_opendir, traverse_path)
{
	DIR *dp;
	TEST_MKDIR_ASSERTED(MAIN_DIR "/nested", S_IRWXU);

	errno = 0;
	dp = opendir(MAIN_DIR "/nested/../nested/.");

	TEST_ASSERT_NOT_NULL(dp);
	TEST_ASSERT_EQUAL_INT(0, closedir(dp));

	rmdir(MAIN_DIR "/nested");
}


TEST(dirent_opendir, too_long_path)
{
	char *filename = malloc(PATH_MAX + 1);
	/* Add 2 for null terminator and slash */
	char *path = malloc(PATH_MAX + sizeof(MAIN_DIR) + 2);

	TEST_ASSERT_NOT_NULL(filename);
	TEST_ASSERT_NOT_NULL(path);

	memset(filename, 'a', PATH_MAX);
	filename[PATH_MAX] = '\0';
	strcpy(path, MAIN_DIR);

	strcat(path, "/");
	strcat(path, filename);

	errno = 0;
	TEST_ASSERT_NULL(opendir(path));
	TEST_ASSERT_EQUAL_INT(ENAMETOOLONG, errno);
	free(filename);
	free(path);
}


TEST(dirent_opendir, thread_safety)
{
	pthread_t threads[THREAD_COUNT];
	DIR *results[THREAD_COUNT];
	pthread_attr_t attr;

	size_t safe_stack_size = 8192;
#ifdef PTHREAD_STACK_MIN
	if (safe_stack_size < PTHREAD_STACK_MIN) {
		safe_stack_size = PTHREAD_STACK_MIN;
	}
#endif

	TEST_ASSERT_EQUAL_INT(0, pthread_attr_init(&attr));
	TEST_ASSERT_EQUAL_INT(0, pthread_attr_setstacksize(&attr, safe_stack_size));

	for (int i = 0; i < THREAD_COUNT; ++i) {
		TEST_ASSERT_EQUAL_INT(0, pthread_create(&threads[i], &attr, thread_opendir, NULL));
	}

	pthread_attr_destroy(&attr);

	for (int i = 0; i < THREAD_COUNT; ++i) {
		TEST_ASSERT_EQUAL_INT(0, pthread_join(threads[i], (void **)&results[i]));
	}

	for (int i = 0; i < THREAD_COUNT; ++i) {
		TEST_ASSERT_NOT_NULL(results[i]);

		for (int j = i + 1; j < THREAD_COUNT; ++j) {
			TEST_ASSERT_NOT_EQUAL(results[i], results[j]);
		}

		TEST_ASSERT_EQUAL_INT(0, closedir(results[i]));
	}
}


TEST_GROUP_RUNNER(dirent_opendir)
{
	RUN_TEST_CASE(dirent_opendir, opening_empty_directory);
	RUN_TEST_CASE(dirent_opendir, opening_not_empty_directory);
	RUN_TEST_CASE(dirent_opendir, wrong_directory_name);
	RUN_TEST_CASE(dirent_opendir, not_a_directory);
	RUN_TEST_CASE(dirent_opendir, creating_dirs_in_closed_and_open_directories);
	RUN_TEST_CASE(dirent_opendir, open_small_enough_number_of_directories);
	RUN_TEST_CASE(dirent_opendir, open_enough_dirs_to_force_error);
	RUN_TEST_CASE(dirent_opendir, open_same_dir_multiple_times);
	RUN_TEST_CASE(dirent_opendir, open_inside_open_directory);
	RUN_TEST_CASE(dirent_opendir, symlink_loop);
	RUN_TEST_CASE(dirent_opendir, symlink_leading_nowhere);
	RUN_TEST_CASE(dirent_opendir, symlink_to_file);
	RUN_TEST_CASE(dirent_opendir, too_long_path);
	RUN_TEST_CASE(dirent_opendir, trailing_slashes);
	RUN_TEST_CASE(dirent_opendir, traverse_path);
	RUN_TEST_CASE(dirent_opendir, thread_safety);
}
