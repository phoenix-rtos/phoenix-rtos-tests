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
#include <errno.h>
#include <string.h>

#include <unity_fixture.h>

#include "common.h"

#define MAIN_DIR "tested_files_opendir"

#ifndef OPEN_MAX
#define OPEN_MAX 1024
#endif

#ifndef SYMLOOP_MAX
#define SYMLOOP_MAX 20
#endif

int create_directories(int num_of_dirs, char identifier)
{
	char buf[30];
	DIR *dirs[OPEN_MAX];
	int loop_counter;

	for (int i = 0; i < num_of_dirs; ++i) {
		sprintf(buf, MAIN_DIR "/%d%c", i, identifier);
		mkdir(buf, 0777);

		/* Guard clause that skips current take if dir was opened,
		Upon failing it proceeds to cleanup */
		if (dirs[i] = opendir(buf))
			continue;

		loop_counter = i;
		for (int j = 0; j <= i; j++) {
			if (dirs[j])
				closedir(dirs[j]);
			sprintf(buf, MAIN_DIR "/%d%c", j, identifier);
			rmdir(buf);
		}
		return -1;
	}

	for (int j = 0; j < num_of_dirs; j++) {
		sprintf(buf, MAIN_DIR "/%d%c", j, identifier);
		rmdir(buf);
	}

	return 0;
}


TEST_GROUP(opendir);


TEST_SETUP(opendir)
{

	mkdir(MAIN_DIR, 0777);
	mkdir(MAIN_DIR "/dir_without_read_perm", 0000);
	FILE *fptr;
	fptr = fopen(MAIN_DIR "/notadir.txt", "a");
	fprintf(fptr, "Some file contents");
	fclose(fptr);
}


TEST_TEAR_DOWN(opendir)
{
	errno = 0;
	remove(MAIN_DIR "/notadir.txt");
	chmod(MAIN_DIR "/dir_without_read_perm", 0777);
	rmdir(MAIN_DIR "/dir_without_read_perm");
	rmdir(MAIN_DIR);
}


TEST(opendir, opening_not_empty_directory)
{
	TEST_ASSERT_NOT_NULL(opendir(MAIN_DIR));
}


TEST(opendir, no_read_permission)
{
	errno = 0;
	DIR *dirPtr = opendir(MAIN_DIR "/dir_without_read_perm");
	TEST_ASSERT_EQUAL_INT(EACCES, errno);
	TEST_ASSERT_NULL(dirPtr);
}


TEST(opendir, wrong_directory_name)
{
	errno = 0;
	DIR *dirPtr = opendir(MAIN_DIR "/not_existing_directory");
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);
	TEST_ASSERT_NULL(dirPtr);
}


TEST(opendir, not_a_directory)
{
	errno = 0;
	DIR *dirPtr = opendir(MAIN_DIR "/notadir.txt");
	TEST_ASSERT_EQUAL_INT(ENOTDIR, errno);
	TEST_ASSERT_NULL(dirPtr);
}


TEST(opendir, open_n_directories)
{
	errno = 0;
	int dir_amount = OPEN_MAX + 100;


	TEST_ASSERT_EQUAL(-1, create_directories(dir_amount, 'd'));
}


TEST(opendir, direct_symlink)
{
	mkdir("A", 0777);

	symlink("A", "D");
	symlink(".", "A/D");

	char loopPath[SYMLOOP_MAX * 4 + 16];
	strcpy(loopPath, "A/");

	for (int i = 0; i < SYMLOOP_MAX - 2; ++i) {
		strcat(loopPath, "D/D/");
	}

	TEST_ASSERT_NOT_NULL(opendir(loopPath));
	rmdir(loopPath);

	for (int i = 0; i < 4; ++i) {
		strcat(loopPath, "D/D/");
	}

	errno = 0;
	TEST_ASSERT_NULL(opendir(loopPath));
	TEST_ASSERT_EQUAL_INT(ELOOP, errno);

	unlink("A/D");
	unlink("D");
	unlink("A");
	rmdir("A/D");
	rmdir("A");
}


TEST(opendir, too_long_path)
{
	int size = PATH_MAX;
	char filename[size + 1];
	char path[size + strlen(MAIN_DIR) + 100];


	memset(filename, 'a', size - strlen(MAIN_DIR) - 3);
	strcpy(path, MAIN_DIR);
	strcat(path, "/");
	strcat(path, filename);
	mkdir(path, 0777);

	TEST_ASSERT_EQUAL_PTR(NULL, opendir(path));
}


TEST_GROUP_RUNNER(opendir)
{
	RUN_TEST_CASE(opendir, no_read_permission);
	RUN_TEST_CASE(opendir, wrong_directory_name);
	RUN_TEST_CASE(opendir, not_a_directory);
	RUN_TEST_CASE(opendir, direct_symlink);
	RUN_TEST_CASE(opendir, too_long_path);
	RUN_TEST_CASE(opendir, open_n_directories);
}