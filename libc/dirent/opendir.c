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

#include <unity_fixture.h>

#include "common.h"
#include "dirent_helper_functions.h"

#define MAIN_DIR     "test_opendir"
#define THREAD_COUNT 4

typedef struct {
	char *selfLoop;
	char *mutualLoop;
	DIR *dirP;
	DIR *selfDP;
	DIR *mutualDP;
	DIR *dirs[4];
} test_ctx_t;

static test_ctx_t test_ctx;


static void *thread_opendir(void *arg)
{
	DIR *dp = opendir(MAIN_DIR);
	return (void *)dp;
}


TEST_GROUP(dirent_opendir);


TEST_SETUP(dirent_opendir)
{
	memset(&test_ctx, 0, sizeof(test_ctx));
	TEST_MKDIR_ASSERTED(MAIN_DIR, S_IRWXU);
}


TEST_TEAR_DOWN(dirent_opendir)
{
	for (int i = 0; i < 4; i++) {
		if (test_ctx.dirs[i] != NULL)
			closedir(test_ctx.dirs[i]);
	}
	if (test_ctx.dirP != NULL) {
		closedir(test_ctx.dirP);
	}
	if (test_ctx.selfDP != NULL) {
		closedir(test_ctx.selfDP);
	}
	if (test_ctx.mutualDP != NULL) {
		closedir(test_ctx.mutualDP);
	}
	unlink(MAIN_DIR "/notadir.txt");
	unlink(MAIN_DIR "/A/B");
	unlink(MAIN_DIR "/D1/S1");
	unlink(MAIN_DIR "/D2/S2");
	unlink(MAIN_DIR "/dangling_link");
	rmdir(MAIN_DIR "/empty_dir");
	rmdir(MAIN_DIR "/formerDir");
	rmdir(MAIN_DIR "/latterDir");
	rmdir(MAIN_DIR "/evenLatterDir");
	rmdir(MAIN_DIR "/ToBeDeleted");
	rmdir(MAIN_DIR "/newdir");
	rmdir(MAIN_DIR "/A");
	rmdir(MAIN_DIR "/D1");
	rmdir(MAIN_DIR "/D2");
	rmdir(MAIN_DIR "/nested");

	free(test_ctx.selfLoop);
	free(test_ctx.mutualLoop);
	rmdir(MAIN_DIR);
}


TEST(dirent_opendir, opening_empty_directory)
{
	TEST_MKDIR_ASSERTED(MAIN_DIR "/empty_dir", S_IRUSR);
	test_ctx.dirP = opendir(MAIN_DIR "/empty_dir");
	TEST_ASSERT_NOT_NULL(test_ctx.dirP);
}


TEST(dirent_opendir, opening_not_empty_directory)
{
	test_ctx.dirP = opendir(MAIN_DIR);
	TEST_ASSERT_NOT_NULL(test_ctx.dirP);
}


TEST(dirent_opendir, wrong_directory_name)
{
	errno = 0;
	test_ctx.dirP = opendir(MAIN_DIR "/not_existing_directory");
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);
	TEST_ASSERT_NULL(test_ctx.dirP);

	errno = 0;
	test_ctx.dirP = opendir("");
	TEST_ASSERT_NULL(test_ctx.dirP);
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);
}


TEST(dirent_opendir, not_a_directory)
{
	int fd = creat(MAIN_DIR "/notadir.txt", S_IRUSR);
	TEST_ASSERT_NOT_EQUAL_MESSAGE(-1, fd, "Failed to create notadir.txt");
	close(fd);

	errno = 0;
	test_ctx.dirP = opendir(MAIN_DIR "/notadir.txt");

	TEST_ASSERT_NULL(test_ctx.dirP);
	TEST_ASSERT_EQUAL_INT(ENOTDIR, errno);
}


TEST(dirent_opendir, creating_dirs_in_closed_and_open_directories)
{
	/* Create dir in closed directory */
	TEST_MKDIR_ASSERTED(MAIN_DIR "/formerDir", S_IRUSR);

	/* Create dir in opened directory, then close opened one */
	test_ctx.dirP = TEST_OPENDIR_ASSERTED(MAIN_DIR);
	test_ctx.dirs[0] = opendir(MAIN_DIR "/formerDir");
	TEST_ASSERT_NOT_NULL(test_ctx.dirs[0]);

	TEST_MKDIR_ASSERTED(MAIN_DIR "/latterDir", S_IRUSR);

	closedir(test_ctx.dirP);

	/* Assure that both dirs can be opened without problems */
	test_ctx.dirs[1] = opendir(MAIN_DIR "/formerDir");
	test_ctx.dirs[2] = opendir(MAIN_DIR "/latterDir");
	TEST_ASSERT_NOT_NULL(test_ctx.dirs[1]);
	TEST_ASSERT_NOT_NULL(test_ctx.dirs[2]);

	test_ctx.dirP = opendir(MAIN_DIR);
	TEST_ASSERT_NOT_NULL(test_ctx.dirP);

	TEST_MKDIR_ASSERTED(MAIN_DIR "/ToBeDeleted", S_IRUSR);
	TEST_ASSERT_EQUAL_INT(0, rmdir(MAIN_DIR "/ToBeDeleted"));
	TEST_ASSERT_EQUAL_INT(0, closedir(test_ctx.dirP));
	test_ctx.dirP = NULL;

	TEST_ASSERT_NULL(opendir(MAIN_DIR "/ToBeDeleted"));
	TEST_MKDIR_ASSERTED(MAIN_DIR "/evenLatterDir", S_IRUSR);
	test_ctx.dirs[3] = opendir(MAIN_DIR "/evenLatterDir");
	TEST_ASSERT_NOT_NULL(test_ctx.dirs[3]);

	for (int i = 0; i < 4; ++i) {
		TEST_ASSERT_NOT_NULL(test_ctx.dirs[i]);
	}
}


TEST(dirent_opendir, open_small_enough_number_of_directories)
{
	errno = 0;
	size_t num_of_dirs = 20;
	char dirPath[40];
	DIR *dirs[num_of_dirs];

	memset((void *)dirs, 0, sizeof(dirs));

	if (TEST_PROTECT()) {
		/* Create directories in batch */
		for (size_t i = 0; i < num_of_dirs; ++i) {
			snprintf(dirPath, sizeof(dirPath), MAIN_DIR "/%zu", i);
			TEST_MKDIR_ASSERTED(dirPath, S_IRUSR);
		}

		/* Open directories one by one */
		for (size_t i = 0; i < num_of_dirs; ++i) {
			snprintf(dirPath, sizeof(dirPath), MAIN_DIR "/%zu", i);

			dirs[i] = opendir(dirPath);
			TEST_ASSERT_NOT_NULL_MESSAGE(dirs[i], "Failed to open directory");
		}
	}

	for (size_t i = 0; i < num_of_dirs; ++i) {
		if (dirs[i] != NULL) {
			closedir(dirs[i]);
		}
		snprintf(dirPath, sizeof(dirPath), MAIN_DIR "/%zu", i);
		rmdir(dirPath);
	}
}


TEST(dirent_opendir, open_enough_dirs_to_force_error)
{
	/*issue #1610: https://github.com/issues/created?issue=phoenix-rtos%7Cphoenix-rtos-project%7C1610 */
	TEST_IGNORE_MESSAGE("#1610 issue");

	const int FD_OVER_LIMIT_MARGIN = 10;
	long max_open_dirs = 0;
	long max_fds = 0;
	char err_msg_buff[128];

#if defined(_SC_OPEN_MAX)
	max_fds = sysconf(_SC_OPEN_MAX);
#endif

	TEST_ASSERT_GREATER_THAN(0, max_fds);

	long max_fds_over_limit = max_fds + FD_OVER_LIMIT_MARGIN;

	DIR **dirs = (DIR **)calloc(max_fds_over_limit + 1, sizeof(DIR *));
	TEST_ASSERT_NOT_NULL(dirs);

	errno = 0;

	if (TEST_PROTECT()) {
		while (max_open_dirs < max_fds_over_limit) {
			dirs[max_open_dirs] = opendir(MAIN_DIR);
			if (dirs[max_open_dirs] == NULL) {
				break;
			}
			max_open_dirs++;
		}

		snprintf(err_msg_buff, sizeof(err_msg_buff), "Managed to open %ld dirs despite the %ld file descriptors limit", max_open_dirs, max_fds);
		TEST_ASSERT_LESS_OR_EQUAL_MESSAGE(max_fds, max_open_dirs, err_msg_buff);
		TEST_ASSERT_NULL(dirs[max_open_dirs]);
		TEST_ASSERT_TRUE_MESSAGE(errno == EMFILE || errno == ENFILE, "Expected errno to be EMFILE or ENFILE upon exhausting file descriptors");
	}

	for (long i = 0; i < max_open_dirs; ++i) {
		if (dirs[i] != NULL) {
			closedir(dirs[i]);
		}
	}

	free((void *)dirs);
}


TEST(dirent_opendir, open_same_dir_multiple_times)
{
	test_ctx.dirs[0] = opendir(MAIN_DIR);
	test_ctx.dirs[1] = opendir(MAIN_DIR);
	test_ctx.dirs[2] = opendir(MAIN_DIR);

	TEST_ASSERT_NOT_NULL(test_ctx.dirs[0]);
	TEST_ASSERT_NOT_NULL(test_ctx.dirs[1]);
	TEST_ASSERT_NOT_NULL(test_ctx.dirs[2]);

	TEST_ASSERT_NOT_EQUAL(test_ctx.dirs[0], test_ctx.dirs[1]);
	TEST_ASSERT_NOT_EQUAL(test_ctx.dirs[1], test_ctx.dirs[2]);
	TEST_ASSERT_NOT_EQUAL(test_ctx.dirs[0], test_ctx.dirs[2]);
}


TEST(dirent_opendir, open_inside_open_directory)
{
	TEST_MKDIR_ASSERTED(MAIN_DIR "/newdir", S_IRUSR);
	test_ctx.dirs[0] = opendir(MAIN_DIR);
	TEST_ASSERT_NOT_NULL(test_ctx.dirs[0]);

	test_ctx.dirs[1] = opendir(MAIN_DIR "/newdir");
	TEST_ASSERT_NOT_NULL(test_ctx.dirs[1]);
}


TEST(dirent_opendir, symlink_loop)
{
	const size_t PATH_BUFFER_SAFE_MARGIN = 10;
	long symloopMax = 0;
#if defined(SYMLOOP_MAX)
	symloopMax = SYMLOOP_MAX;
#elif defined(_SC_SYMLOOP_MAX)
	symloopMax = sysconf(_SC_SYMLOOP_MAX);
	if (symloopMax == -1) {
		/* symloopMax on linux is likely to be hardcoded to 40 */
		symloopMax = 40;
	}
#else
	TEST_IGNORE_MESSAGE("Neither SYMLOOP_MAX nor sysconf(_SC_SYMLOOP_MAX) is defined");
#endif

	TEST_MKDIR_ASSERTED(MAIN_DIR "/A", S_IRWXU);
	TEST_MKDIR_ASSERTED(MAIN_DIR "/D1", S_IRWXU);
	TEST_MKDIR_ASSERTED(MAIN_DIR "/D2", S_IRWXU);

	test_ctx.selfLoop = malloc(PATH_MAX);
	test_ctx.mutualLoop = malloc(PATH_MAX);
	char *selfLoop = test_ctx.selfLoop;
	char *mutualLoop = test_ctx.mutualLoop;

	TEST_ASSERT_NOT_NULL(selfLoop);
	TEST_ASSERT_NOT_NULL(mutualLoop);

	TEST_ASSERT_EQUAL_INT(0, symlink("../D2", MAIN_DIR "/D1/S1"));
	TEST_ASSERT_EQUAL_INT(0, symlink("../D1", MAIN_DIR "/D2/S2"));
	TEST_ASSERT_EQUAL_INT(0, symlink(".", MAIN_DIR "/A/B"));

	strcpy(selfLoop, MAIN_DIR "/A");
	strcpy(mutualLoop, MAIN_DIR "/D1");

	/* Create a path to a valid symloop; POSIX dictates loops up to SYMLOOP_MAX must succeed */
	for (int i = 0; i < 4; ++i) {
		strcat(selfLoop, "/B/B");
		strcat(mutualLoop, "/S1/S2");
	}

	errno = 0;
	test_ctx.selfDP = opendir(selfLoop);
	test_ctx.mutualDP = opendir(mutualLoop);
	DIR *selfDP = test_ctx.selfDP;
	DIR *mutualDP = test_ctx.mutualDP;

	TEST_ASSERT_NOT_NULL(selfDP);
	TEST_ASSERT_NOT_NULL(mutualDP);

	closedir(test_ctx.selfDP);
	test_ctx.selfDP = NULL;
	closedir(test_ctx.mutualDP);
	test_ctx.mutualDP = NULL;

	/* Add layers until path resolution depth forces an ELOOP error */
	for (int i = 0; i < symloopMax / 2 - 1; ++i) {
		if (strlen(selfLoop) + PATH_BUFFER_SAFE_MARGIN >= PATH_MAX ||
				strlen(mutualLoop) + PATH_BUFFER_SAFE_MARGIN >= PATH_MAX) {
			break;
		}
		strcat(selfLoop, "/B/B");
		strcat(mutualLoop, "/S1/S2");
	}

	errno = 0;
	test_ctx.selfDP = opendir(selfLoop);
	TEST_ASSERT_NULL(test_ctx.selfDP);
	TEST_ASSERT_EQUAL_INT(ELOOP, errno);
	test_ctx.selfDP = NULL;

	errno = 0;
	test_ctx.mutualDP = opendir(mutualLoop);
	TEST_ASSERT_NULL(test_ctx.mutualDP);
	TEST_ASSERT_EQUAL_INT(ELOOP, errno);
	test_ctx.mutualDP = NULL;
}


TEST(dirent_opendir, symlink_leading_nowhere)
{
	TEST_ASSERT_EQUAL_INT(0, symlink(MAIN_DIR "/does_not_exist", MAIN_DIR "/dangling_link"));

	errno = 0;
	test_ctx.dirP = opendir(MAIN_DIR "/dangling_link");

	TEST_ASSERT_NULL(test_ctx.dirP);
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);

	unlink(MAIN_DIR "/dangling_link");
}


TEST(dirent_opendir, symlink_to_file)
{
	int fd = -1;
	int created = 0;
	int linked = 0;

	if (TEST_PROTECT()) {
		fd = creat(MAIN_DIR "/target_file.txt", S_IRUSR);
		TEST_ASSERT_NOT_EQUAL_MESSAGE(-1, fd, "Failed to create target_file.txt");
		created = 1;
		close(fd);
		fd = -1;

		TEST_ASSERT_EQUAL_INT(0, symlink("target_file.txt", MAIN_DIR "/file_link"));
		linked = 1;

		errno = 0;
		test_ctx.dirP = opendir(MAIN_DIR "/file_link");

		TEST_ASSERT_NULL(test_ctx.dirP);
		TEST_ASSERT_EQUAL_INT(ENOTDIR, errno);
	}

	if (fd != -1) {
		close(fd);
	}
	if (linked) {
		unlink(MAIN_DIR "/file_link");
	}
	if (created) {
		remove(MAIN_DIR "/target_file.txt");
	}
}


TEST(dirent_opendir, trailing_slashes)
{
	errno = 0;
	test_ctx.dirP = opendir(MAIN_DIR "///");
	TEST_ASSERT_NOT_NULL(test_ctx.dirP);
	TEST_ASSERT_EQUAL_INT(0, closedir(test_ctx.dirP));


	TEST_MKDIR_ASSERTED(MAIN_DIR "/nested", S_IRWXU);
	errno = 0;
	test_ctx.dirP = opendir(MAIN_DIR "///nested///");
	TEST_ASSERT_NOT_NULL(test_ctx.dirP);
	TEST_ASSERT_EQUAL_INT(0, closedir(test_ctx.dirP));
	test_ctx.dirP = NULL;
}


TEST(dirent_opendir, traverse_path)
{
	TEST_MKDIR_ASSERTED(MAIN_DIR "/nested", S_IRWXU);

	errno = 0;
	test_ctx.dirP = opendir(MAIN_DIR "/nested/../nested/.");

	TEST_ASSERT_NOT_NULL(test_ctx.dirP);
	TEST_ASSERT_EQUAL_INT(0, closedir(test_ctx.dirP));
	test_ctx.dirP = NULL;
}


TEST(dirent_opendir, too_long_path)
{
	char *filename = malloc(PATH_MAX + 1);
	/* Add 2 for null terminator and slash */
	char *path = malloc(PATH_MAX + sizeof(MAIN_DIR) + 2);

	if (TEST_PROTECT()) {
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
	}
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
