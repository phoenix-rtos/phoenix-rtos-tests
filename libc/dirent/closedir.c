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
#include <pthread.h>
#include <signal.h>

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>

#include <unity_fixture.h>

#include "common.h"

#define MAIN_DIR          "test_closedir"
#define INTERRUPT_DIR_NUM 256


TEST_GROUP(dirent_closedir);


TEST_SETUP(dirent_closedir)
{
	mkdir(MAIN_DIR, 0777);
	mkdir(MAIN_DIR "/dir1", 0777);
}


TEST_TEAR_DOWN(dirent_closedir)
{
	errno = 0;
	rmdir(MAIN_DIR "/dir1");
	rmdir(MAIN_DIR);
}


TEST(dirent_closedir, closing_empty_dir)
{
	errno = 0;
	DIR *dp = opendir(MAIN_DIR "/dir1");

	TEST_ASSERT_NOT_NULL(dp);
	TEST_ASSERT_EQUAL(0, closedir(dp));
}


TEST(dirent_closedir, closing_non_empty_dir)
{
	errno = 0;
	DIR *dp = opendir(MAIN_DIR);

	TEST_ASSERT_NOT_NULL(dp);
	TEST_ASSERT_EQUAL(0, closedir(dp));
}


// TEST(dirent_closedir, closing_nonexisting_stream)
// {
// 	DIR *dp;
// 	errno = 0;
// 	TEST_ASSERT_EQUAL(-1, closedir(dp));
// 	TEST_ASSERT_EQUAL(EBADF, errno);
// 	errno = 0;
// }


// TEST(dirent_closedir, closing_closed_directory)
// {
// 	DIR *dirPtr = opendir(MAIN_DIR "/dir1");
// 	errno = 0;

// 	closedir(dirPtr);
// 	TEST_ASSERT_EQUAL(-1, closedir(dirPtr));
// 	TEST_ASSERT_EQUAL(EBADF, errno);
// }


// TEST(dirent_closedir, interrupt_with_sigkill)
// {
// 	errno = 0;

// 	DIR *dirs[INTERRUPT_DIR_NUM];
// 	char *name[8];

// 	for (int i = 0; i < INTERRUPT_DIR_NUM; ++i) {
// 		sprintf(name, MAIN_DIR "/%d", i);
// 		dirs[i] = opendir(name);
// 	}

// 	pid_t childID = fork();
// 	if (childID) {
// 		for (int i = 0; i < INTERRUPT_DIR_NUM; ++i) {
// 			closedir(dirs[i]);
// 			continue;
// 		}
// 	}

// 	kill(childID, SIGKILL);
// 	TEST_ASSERT_EQUAL_INT(EINTR, errno);
// }


// TEST(dirent_closedir, interrupt_with_sigterm)
// {
// 	errno = 0;

// 	DIR *dirs[INTERRUPT_DIR_NUM];
// 	for (int i = 0; i < INTERRUPT_DIR_NUM; ++i) {
// 		char name[32];
// 		sprintf(name, MAIN_DIR "/%d", i);
// 		mkdir(name, 0777);
// 		dirs[i] = opendir(name);
// 	}

// 	pid_t childID = fork();

// 	if (!childID) {
// 		for (int i = 0; i < INTERRUPT_DIR_NUM; ++i) {
// 			closedir(dirs[i]);
// 		}
// 	}

// 	kill(childID, SIGKILL);
// 	TEST_ASSERT_EQUAL_INT(EINTR, errno);
// }

// TEST(dirent_closedir, interrupt_with_thread)
// {
// 	DIR *dirs[INTERRUPT_DIR_NUM];
// 	pthread_t worker;

// 	for (int i = 0; i < INTERRUPT_DIR_NUM; ++i) {
// 		char name[32];
// 		sprintf(name, MAIN_DIR "/%d", i);
// 		mkdir(name, 0777);
// 		dirs[i] = opendir(name);
// 	}

// 	pthread_create(&worker, NULL, &close_dirs, NULL);
// 	puts("Process created\n");
// 	errno = 0;
// 	usleep(100);
// 	pthread_kill(worker, SIGINT);
// 	puts("Process killed\n");
// 	printf("%d", errno);
// 	pthread_join(worker, NULL);

// 	return 0;
// }


TEST_GROUP_RUNNER(dirent_closedir)
{
	RUN_TEST_CASE(dirent_closedir, closing_empty_dir);
	RUN_TEST_CASE(dirent_closedir, closing_non_empty_dir);


	/* Calling AddressSanitizer and other scary behaviour */
	// RUN_TEST_CASE(dirent_closedir, closing_nonexisting_stream);
	// RUN_TEST_CASE(dirent_closedir, closing_closed_directory);

	/* Doesn't really want to cooperate */
	// RUN_TEST_CASE(dirent_closedir, interrupt_with_sigkill);
	// RUN_TEST_CASE(dirent_closedir, interrupt_with_sigterm);
	// RUN_TEST_CASE(dirent_closedir, interrupt_with_thread);
}
