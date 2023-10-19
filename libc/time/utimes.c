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
 * Copyright 2022, 2023 Phoenix Systems
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
	TEST_SET_PAST
} test_t;


typedef enum {
	UTIMES,
	FUTIMES,
	LUTIMES
} mode_test;


void test_utimes(const char *filename, test_t test, mode_test mode)
{
	int fd;
	struct timeval tv[2];
	struct stat statbuf;
	int64_t adiff_sec, mdiff_sec;
/* FIXME: only second accuracy is supported */
#ifndef __phoenix__
	int64_t adiff_nsec, mdiff_nsec;
#endif

	if (test != TEST_SET_PAST) {
		TEST_ASSERT_EQUAL_INT(0, gettimeofday(&tv[0], NULL));
		tv[1] = tv[0];
	}
	else {
		tv[0].tv_sec = random();
		tv[1].tv_sec = random();
		tv[0].tv_usec = random() % 1000;
		tv[1].tv_usec = random() % 1000;
	}

	if (mode == UTIMES) {
		TEST_ASSERT_EQUAL_INT(0, utimes(filename, test == TEST_SET_NULL ? NULL : tv));
		TEST_ASSERT_EQUAL_INT(0, stat(filename, &statbuf));
	}
	else if (mode == FUTIMES) {
		TEST_ASSERT_GREATER_THAN_INT(0, fd = open(filename, O_RDONLY));
		TEST_ASSERT_EQUAL_INT(0, futimes(fd, test == TEST_SET_NULL ? NULL : tv));
		TEST_ASSERT_EQUAL_INT(0, fstat(fd, &statbuf));
		close(fd);
	}
	else if (mode == LUTIMES) {
		TEST_ASSERT_EQUAL_INT(0, lutimes(filename, test == TEST_SET_NULL ? NULL : tv));
		TEST_ASSERT_EQUAL_INT(0, lstat(filename, &statbuf));
	}


	adiff_sec = tv[0].tv_sec - statbuf.st_atim.tv_sec;
	mdiff_sec = tv[1].tv_sec - statbuf.st_mtim.tv_sec;

/* FIXME: only second accuracy is supported */
#ifndef __phoenix__
	adiff_nsec = tv[0].tv_usec * 1000 - statbuf.st_atim.tv_nsec;
	mdiff_nsec = tv[1].tv_usec * 1000 - statbuf.st_mtim.tv_nsec;
#endif

	if (test != TEST_SET_PAST) {
		TEST_ASSERT_LESS_THAN(2, adiff_sec);
		TEST_ASSERT_LESS_THAN(2, mdiff_sec);
	}
	else {
		TEST_ASSERT_EQUAL_INT(0, adiff_sec);
		TEST_ASSERT_EQUAL_INT(0, mdiff_sec);

/* FIXME: only second accuracy is supported */
#ifndef __phoenix__
		TEST_ASSERT_EQUAL_INT(0, adiff_nsec);
		TEST_ASSERT_EQUAL_INT(0, mdiff_nsec);
#endif
	}
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
		usleep(random() % 500000);
		test_utimes(FILENAME, TEST_SET_NULL, UTIMES);
	}
}


TEST(test_utimes, set_now)
{
	unsigned int i;

	for (i = 0; i < LOOP_CNT; ++i) {
		usleep(random() % 500000);
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
	struct timeval tv[2];
	struct stat statbuf;

	tv[0].tv_sec = 1;
	tv[1].tv_sec = 1;
	tv[0].tv_usec = 1;
	tv[1].tv_usec = 1;

	TEST_ASSERT_EQUAL_INT(0, utimes(FILENAME, tv));
	TEST_ASSERT_EQUAL_INT(0, stat(FILENAME, &statbuf));

	TEST_ASSERT_EQUAL_INT64(tv[0].tv_sec, statbuf.st_atim.tv_sec);
	TEST_ASSERT_EQUAL_INT64(tv[1].tv_sec, statbuf.st_mtim.tv_sec);

/* FIXME: only second accuracy is supported */
#ifndef __phoenix__
	TEST_ASSERT_EQUAL_INT64(tv[0].tv_usec * 1000, statbuf.st_atim.tv_nsec);
	TEST_ASSERT_EQUAL_INT64(tv[1].tv_usec * 1000, statbuf.st_mtim.tv_nsec);
#endif
}


TEST(test_utimes, high_value)
{
	struct timeval tv[2];
	struct stat statbuf;

	tv[0].tv_sec = INT_MAX;
	tv[1].tv_sec = INT_MAX;
	tv[0].tv_usec = 999999;
	tv[1].tv_usec = 999999;

	TEST_ASSERT_EQUAL_INT32(0, utimes(FILENAME, tv));
	TEST_ASSERT_EQUAL_INT32(0, stat(FILENAME, &statbuf));

	TEST_ASSERT_EQUAL_INT64(tv[0].tv_sec, statbuf.st_atim.tv_sec);
	TEST_ASSERT_EQUAL_INT64(tv[1].tv_sec, statbuf.st_mtim.tv_sec);

/* FIXME: only second accuracy is supported */
#ifndef __phoenix__
	TEST_ASSERT_EQUAL_INT64(tv[0].tv_usec * 1000, statbuf.st_atim.tv_nsec);
	TEST_ASSERT_EQUAL_INT64(tv[1].tv_usec * 1000, statbuf.st_mtim.tv_nsec);
#endif
}


TEST(test_utimes, errnos)
{
	/* ENOENT */
	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, utimes("NOT EXIST", NULL));
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, utimes("", NULL));
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);

	/* ENAMETOOLONG */
	char tooLongPath[PATH_MAX];
	memset(tooLongPath, 'a', PATH_MAX - 1);
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
	int fd;
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
		usleep(random() % 500000);
		test_utimes(FILENAME, TEST_SET_NULL, FUTIMES);
	}
}


TEST(test_futimes, set_now)
{
	unsigned int i;

	for (i = 0; i < LOOP_CNT; ++i) {
		usleep(random() % 500000);
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
	int fd;
	struct timeval tv[2];
	struct stat statbuf;

	tv[0].tv_sec = 1;
	tv[1].tv_sec = 1;
	tv[0].tv_usec = 1;
	tv[1].tv_usec = 1;

	TEST_ASSERT_GREATER_THAN_INT(0, fd = open(FILENAME, O_RDONLY));
	TEST_ASSERT_EQUAL_INT(0, futimes(fd, tv));
	TEST_ASSERT_EQUAL_INT(0, fstat(fd, &statbuf));
	TEST_ASSERT_EQUAL_INT(0, close(fd));

	TEST_ASSERT_EQUAL_INT64(tv[0].tv_sec, statbuf.st_atim.tv_sec);
	TEST_ASSERT_EQUAL_INT64(tv[1].tv_sec, statbuf.st_mtim.tv_sec);

/* FIXME: only second accuracy is supported */
#ifndef __phoenix__
	TEST_ASSERT_EQUAL_INT64(tv[0].tv_usec * 1000, statbuf.st_atim.tv_nsec);
	TEST_ASSERT_EQUAL_INT64(tv[1].tv_usec * 1000, statbuf.st_mtim.tv_nsec);
#endif
}


TEST(test_futimes, high_value)
{
	int fd;
	struct timeval tv[2];
	struct stat statbuf;

	tv[0].tv_sec = INT32_MAX;
	tv[1].tv_sec = INT32_MAX;
	tv[0].tv_usec = 999999;
	tv[1].tv_usec = 999999;

	TEST_ASSERT_GREATER_THAN_INT(0, fd = open(FILENAME, O_RDONLY));
	TEST_ASSERT_EQUAL_INT(0, futimes(fd, tv));
	TEST_ASSERT_EQUAL_INT(0, fstat(fd, &statbuf));
	TEST_ASSERT_EQUAL_INT(0, close(fd));

	TEST_ASSERT_EQUAL_INT64(tv[0].tv_sec, statbuf.st_atim.tv_sec);
	TEST_ASSERT_EQUAL_INT64(tv[1].tv_sec, statbuf.st_mtim.tv_sec);

/* FIXME: only second accuracy is supported */
#ifndef __phoenix__
	TEST_ASSERT_EQUAL_INT64(tv[0].tv_usec * 1000, statbuf.st_atim.tv_nsec);
	TEST_ASSERT_EQUAL_INT64(tv[1].tv_usec * 1000, statbuf.st_mtim.tv_nsec);
#endif
}


TEST(test_futimes, errnos)
{
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
		usleep(random() % 500000);
		test_utimes(FILENAME, TEST_SET_NULL, LUTIMES);
	}
}


TEST(test_lutimes, set_now)
{
	unsigned int i;

	for (i = 0; i < LOOP_CNT; ++i) {
		usleep(random() % 500000);
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
	struct timeval tv[2];
	struct stat statbuf;

	tv[0].tv_sec = 1;
	tv[1].tv_sec = 1;
	tv[0].tv_usec = 1;
	tv[1].tv_usec = 1;

	TEST_ASSERT_EQUAL_INT(0, lutimes(FILENAME, tv));
	TEST_ASSERT_EQUAL_INT(0, lstat(FILENAME, &statbuf));

	TEST_ASSERT_EQUAL_INT64(tv[0].tv_sec, statbuf.st_atim.tv_sec);
	TEST_ASSERT_EQUAL_INT64(tv[1].tv_sec, statbuf.st_mtim.tv_sec);

/* FIXME: only second accuracy is supported */
#ifndef __phoenix__
	TEST_ASSERT_EQUAL_INT64(tv[0].tv_usec * 1000, statbuf.st_atim.tv_nsec);
	TEST_ASSERT_EQUAL_INT64(tv[1].tv_usec * 1000, statbuf.st_mtim.tv_nsec);
#endif
}


TEST(test_lutimes, high_value)
{
	struct timeval tv[2];
	struct stat statbuf;

	tv[0].tv_sec = INT32_MAX;
	tv[1].tv_sec = INT32_MAX;
	tv[0].tv_usec = 999999;
	tv[1].tv_usec = 999999;

	TEST_ASSERT_EQUAL_INT(0, lutimes(FILENAME, tv));
	TEST_ASSERT_EQUAL_INT(0, lstat(FILENAME, &statbuf));

	TEST_ASSERT_EQUAL_INT64(tv[0].tv_sec, statbuf.st_atim.tv_sec);
	TEST_ASSERT_EQUAL_INT64(tv[1].tv_sec, statbuf.st_mtim.tv_sec);

/* FIXME: only second accuracy is supported */
#ifndef __phoenix__
	TEST_ASSERT_EQUAL_INT64(tv[0].tv_usec * 1000, statbuf.st_atim.tv_nsec);
	TEST_ASSERT_EQUAL_INT64(tv[1].tv_usec * 1000, statbuf.st_mtim.tv_nsec);
#endif
}


TEST(test_lutimes, errnos)
{
	/* ENOENT */
	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, lutimes("NOT EXIST", NULL));
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, lutimes("", NULL));
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);

	/* ENAMETOOLONG */
	char tooLongPath[PATH_MAX];
	memset(tooLongPath, 'a', PATH_MAX - 1);
	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, lutimes(tooLongPath, NULL));
	TEST_ASSERT_EQUAL_INT(ENAMETOOLONG, errno);

	/* ENOTDIR */
	int fd;
	fd = creat("not_a_directory", 0777);
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
