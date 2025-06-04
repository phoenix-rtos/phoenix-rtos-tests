/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - sys/time.h
 *
 * TESTED:
 *    - utimes()
 *    - futimes()
 *    - lutimes()
 *
 * Copyright 2022-2025 Phoenix Systems
 * Author: Ziemowit Leszczynski, Mateusz Bloch
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>

#include "unity_fixture.h"


static const char *const FILENAME = "utimes";
static const char *const LINKNAME = "utimes_link";
static const unsigned int LOOP_CNT = 10;


typedef enum {
	TEST_SET_NULL,
	TEST_SET_NOW,
	TEST_SET_PAST,
	TEST_SET_ONE,
	TEST_SET_HIGH_VALUE
} test_t;


typedef enum {
	UTIMES,
	FUTIMES,
	LUTIMES
} mode_test;


/* Function to set time values based on the test case */
static void set_time_values(struct timeval tv[2], test_t test)
{
	switch (test) {
		case TEST_SET_NULL:
		case TEST_SET_NOW:
			TEST_ASSERT_EQUAL_INT(0, gettimeofday(&tv[0], NULL));
			tv[1] = tv[0];
			break;
		case TEST_SET_PAST:
			tv[0].tv_sec = random();
			tv[1].tv_sec = random();
			tv[0].tv_usec = random() % 1000000;
			tv[1].tv_usec = random() % 1000000;
			break;
		case TEST_SET_ONE:
			tv[0].tv_sec = 1;
			tv[1].tv_sec = 1;
			tv[0].tv_usec = 1;
			tv[1].tv_usec = 1;
			break;
		case TEST_SET_HIGH_VALUE:
			tv[0].tv_sec = INT32_MAX;
			tv[1].tv_sec = INT32_MAX;
			tv[0].tv_usec = 999999;
			tv[1].tv_usec = 999999;
			break;
		default:
			TEST_FAIL_MESSAGE("Invalid test case");
			break;
	}
}

/* Function to perform utimes/futimes/lutimes based on the mode */
static void perform_utimes(const char *filename, struct timeval tv[2], test_t test, mode_test mode, struct stat *statbuf, char *message)
{
	int fd;

	if (mode == UTIMES) {
		strcpy(message, "utimes() failed");
		TEST_ASSERT_EQUAL_INT_MESSAGE(0, utimes(filename, test == TEST_SET_NULL ? NULL : tv), message);
		TEST_ASSERT_EQUAL_INT_MESSAGE(0, stat(filename, statbuf), message);
	}
	else if (mode == FUTIMES) {
		strcpy(message, "futimes() failed");
		TEST_ASSERT_GREATER_THAN_INT(0, fd = open(filename, O_RDONLY));
		TEST_ASSERT_EQUAL_INT_MESSAGE(0, futimes(fd, test == TEST_SET_NULL ? NULL : tv), message);
		TEST_ASSERT_EQUAL_INT_MESSAGE(0, fstat(fd, statbuf), message);
		close(fd);
	}
	else if (mode == LUTIMES) {
		strcpy(message, "lutimes() failed");
		TEST_ASSERT_EQUAL_INT_MESSAGE(0, lutimes(filename, test == TEST_SET_NULL ? NULL : tv), message);
		TEST_ASSERT_EQUAL_INT_MESSAGE(0, lstat(filename, statbuf), message);
	}
	else {
		TEST_FAIL_MESSAGE("Invalid mode");
	}
}

/* Function to validate the time values after performing utimes/futimes/lutimes */
static void validate_time_values(const struct timeval tv[2], const struct stat *statbuf, test_t test, char *message)
{
	int64_t adiff_sec, mdiff_sec;
#ifndef __phoenix__
	int64_t adiff_nsec, mdiff_nsec;
#endif

	adiff_sec = abs(statbuf->st_atim.tv_sec - tv[0].tv_sec);
	mdiff_sec = abs(statbuf->st_mtim.tv_sec - tv[1].tv_sec);

#ifndef __phoenix__
	adiff_nsec = tv[0].tv_usec * 1000 - statbuf->st_atim.tv_nsec;
	mdiff_nsec = tv[1].tv_usec * 1000 - statbuf->st_mtim.tv_nsec;
#endif

	switch (test) {
		case TEST_SET_NULL:
		case TEST_SET_NOW:
			TEST_ASSERT_LESS_OR_EQUAL_MESSAGE(1, adiff_sec, message);
			TEST_ASSERT_LESS_OR_EQUAL_MESSAGE(1, mdiff_sec, message);
			break;
		case TEST_SET_PAST:
		case TEST_SET_ONE:
		case TEST_SET_HIGH_VALUE:
			TEST_ASSERT_EQUAL_INT64_MESSAGE(0, adiff_sec, message);
			TEST_ASSERT_EQUAL_INT64_MESSAGE(0, mdiff_sec, message);
#ifndef __phoenix__
			TEST_ASSERT_EQUAL_INT64_MESSAGE(0, adiff_nsec, message);
			TEST_ASSERT_EQUAL_INT64_MESSAGE(0, mdiff_nsec, message);
#endif
			break;
		default:
			TEST_FAIL_MESSAGE("Invalid test case");
			break;
	}
}

/* Main test function that uses the smaller helper functions */
static void test_utimes(const char *filename, test_t test, mode_test mode)
{
	char message[100];
	struct timeval tv[2];
	struct stat statbuf;

	set_time_values(tv, test);
	perform_utimes(filename, tv, test, mode, &statbuf, message);
	validate_time_values(tv, &statbuf, test, message);
}


TEST_GROUP(test_utimes);


TEST_SETUP(test_utimes)
{
	int fd;
	TEST_ASSERT_GREATER_THAN_INT(0, fd = creat(FILENAME, 0644));
	close(fd);
}


TEST_TEAR_DOWN(test_utimes)
{
	unlink(FILENAME);
}


TEST(test_utimes, set_null)
{
	unsigned int i;

	for (i = 0; i < LOOP_CNT; ++i) {
		usleep(random() % 100000);
		test_utimes(FILENAME, TEST_SET_NULL, UTIMES);
	}
}


TEST(test_utimes, set_now)
{
	unsigned int i;

	for (i = 0; i < LOOP_CNT; ++i) {
		usleep(random() % 100000);
		test_utimes(FILENAME, TEST_SET_NOW, UTIMES);
	}
}


TEST(test_utimes, set_past)
{
	unsigned int i;

	for (i = 0; i < LOOP_CNT; ++i) {
		test_utimes(FILENAME, TEST_SET_PAST, UTIMES);
	}
}


TEST(test_utimes, one)
{
	test_utimes(FILENAME, TEST_SET_ONE, UTIMES);
}

TEST(test_utimes, high_value)
{
	test_utimes(FILENAME, TEST_SET_HIGH_VALUE, UTIMES);
}


TEST(test_utimes, errnos)
{
	int fd;

	/* TODO: EACCESS and EPERM not tested due to all programs on Phoenix are executed as root*/

	/* ENOENT */
	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, utimes("NOT EXIST", NULL));
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, utimes("", NULL));
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);

	/* ENAMETOOLONG */
	char tooLongPath[PATH_MAX + 2] = { 0 };
	memset(tooLongPath, 'a', PATH_MAX + 1);
	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, utimes(tooLongPath, NULL));
	TEST_ASSERT_EQUAL_INT(ENAMETOOLONG, errno);

	/* ELOOP */
	TEST_ASSERT_EQUAL_INT(0, symlink("loop_symlink", "loop_symlink"));
	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, utimes("loop_symlink", NULL));
	TEST_ASSERT_EQUAL_INT(ELOOP, errno);
	unlink("loop_symlink");

	/* ENOTDIR */
	fd = creat("not_a_directory", 0777);
	close(fd);
	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, utimes("not_a_directory/file.txt", NULL));
	TEST_ASSERT_EQUAL_INT(ENOTDIR, errno);
	remove("not_a_directory");
}


TEST_GROUP(test_futimes);


TEST_SETUP(test_futimes)
{
	int fd;
	TEST_ASSERT_GREATER_THAN_INT(0, fd = creat(FILENAME, 0644));
	close(fd);
}


TEST_TEAR_DOWN(test_futimes)
{
	unlink(FILENAME);
}


TEST(test_futimes, set_null)
{
	unsigned int i;

	for (i = 0; i < LOOP_CNT; ++i) {
		usleep(random() % 100000);
		test_utimes(FILENAME, TEST_SET_NULL, FUTIMES);
	}
}


TEST(test_futimes, set_now)
{
	unsigned int i;

	for (i = 0; i < LOOP_CNT; ++i) {
		usleep(random() % 100000);
		test_utimes(FILENAME, TEST_SET_NOW, FUTIMES);
	}
}


TEST(test_futimes, set_past)
{
	unsigned int i;

	for (i = 0; i < LOOP_CNT; ++i) {
		test_utimes(FILENAME, TEST_SET_PAST, FUTIMES);
	}
}


TEST(test_futimes, one)
{
	test_utimes(FILENAME, TEST_SET_ONE, FUTIMES);
}

TEST(test_futimes, high_value)
{
	test_utimes(FILENAME, TEST_SET_HIGH_VALUE, FUTIMES);
}


TEST(test_futimes, errnos)
{
	/* TODO: EACCESS and EPERM not tested due to all programs on Phoenix are executed as root*/

	/* EBADF */
	int fd = -1;

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, futimes(fd, NULL));
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


TEST_GROUP(test_lutimes);


TEST_SETUP(test_lutimes)
{
	int fd;

	TEST_ASSERT_GREATER_THAN_INT(0, fd = creat(FILENAME, 0644));
	close(fd);

	TEST_ASSERT_EQUAL_INT(0, symlink(FILENAME, LINKNAME));
}


TEST_TEAR_DOWN(test_lutimes)
{
	unlink(LINKNAME);
	unlink(FILENAME);
}


TEST(test_lutimes, set_null)
{
	unsigned int i;

	for (i = 0; i < LOOP_CNT; ++i) {
		usleep(random() % 100000);
		test_utimes(FILENAME, TEST_SET_NULL, LUTIMES);
	}
}


TEST(test_lutimes, set_now)
{
	unsigned int i;

	for (i = 0; i < LOOP_CNT; ++i) {
		usleep(random() % 100000);
		test_utimes(FILENAME, TEST_SET_NOW, LUTIMES);
	}
}


TEST(test_lutimes, set_past)
{
	unsigned int i;

	for (i = 0; i < LOOP_CNT; ++i) {
		test_utimes(FILENAME, TEST_SET_PAST, LUTIMES);
	}
}


TEST(test_lutimes, one)
{
	test_utimes(FILENAME, TEST_SET_ONE, LUTIMES);
}

TEST(test_lutimes, high_value)
{
	test_utimes(FILENAME, TEST_SET_HIGH_VALUE, LUTIMES);
}


TEST(test_lutimes, errnos)
{
	/* TODO: EACCESS and EPERM not tested due to all programs on Phoenix are executed as root*/


	/* ENOENT */
	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, lutimes("NOT EXIST", NULL));
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, lutimes("", NULL));
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);

	/* ENAMETOOLONG */
	char tooLongPath[PATH_MAX + 2] = { 0 };
	memset(tooLongPath, 'a', PATH_MAX + 1);
	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, lutimes(tooLongPath, NULL));
	TEST_ASSERT_EQUAL_INT(ENAMETOOLONG, errno);

	/* ENOTDIR */
	int fd;
	fd = creat("not_a_directory", 0777);
	TEST_ASSERT_GREATER_THAN_INT(0, fd);
	close(fd);
	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, lutimes("not_a_directory/file.txt", NULL));

/* On Phoenix is set wrong errno (?) */
#ifndef __phoenix
	TEST_ASSERT_EQUAL_INT(ENOTDIR, errno);
#endif

	remove("not_a_directory");
}


TEST_GROUP_RUNNER(test_utimes)
{
	RUN_TEST_CASE(test_utimes, set_null);
	RUN_TEST_CASE(test_utimes, set_now);
	RUN_TEST_CASE(test_utimes, set_past);
	RUN_TEST_CASE(test_utimes, one);
	RUN_TEST_CASE(test_utimes, high_value);
	RUN_TEST_CASE(test_utimes, errnos);
}


TEST_GROUP_RUNNER(test_futimes)
{
	RUN_TEST_CASE(test_futimes, set_null);
	RUN_TEST_CASE(test_futimes, set_now);
	RUN_TEST_CASE(test_futimes, set_past);
	RUN_TEST_CASE(test_futimes, one);
	RUN_TEST_CASE(test_futimes, high_value);
	RUN_TEST_CASE(test_futimes, errnos);
}


TEST_GROUP_RUNNER(test_lutimes)
{
	RUN_TEST_CASE(test_lutimes, set_null);
	RUN_TEST_CASE(test_lutimes, set_now);
	RUN_TEST_CASE(test_lutimes, set_past);
	RUN_TEST_CASE(test_lutimes, one);
	RUN_TEST_CASE(test_lutimes, high_value);
	RUN_TEST_CASE(test_lutimes, errnos);
}
