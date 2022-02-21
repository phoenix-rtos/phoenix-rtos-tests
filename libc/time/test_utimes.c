/*
 * Phoenix-RTOS
 *
 * phoenix-rtos-tests
 *
 * utimes tests
 *
 * Copyright 2022 Phoenix Systems
 * Author: Ziemowit Leszczynski
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


#define FILENAME "/var/tmp/utimes"
#define LINKNAME "/var/tmp/utimes_link"
#define LOOP_CNT 10


typedef enum {
	test_set_null,
	test_set_now,
	test_set_past
} test_t;


TEST_GROUP(test_utimes);


TEST_SETUP(test_utimes)
{
	int fd;

	if ((fd = creat(FILENAME, 0644)) < 0)
		TEST_FAIL_MESSAGE(strerror(errno));
	close(fd);
}


TEST_TEAR_DOWN(test_utimes)
{
	unlink(FILENAME);
}


void test_utimes(const char *filename, test_t test)
{
	struct timeval tv[2];
	struct stat statbuf;
	int64_t adiff_sec, mdiff_sec;
	// int64_t adiff_nsec, mdiff_nsec;

	if (test != test_set_past) {
		if (gettimeofday(&tv[0], NULL) < 0)
			TEST_FAIL_MESSAGE(strerror(errno));
		tv[1] = tv[0];
	}
	else {
		tv[0].tv_sec = random();
		tv[1].tv_sec = random();
		tv[0].tv_usec = random() % 1000;
		tv[1].tv_usec = random() % 1000;
	}

	if (utimes(filename, test == test_set_null ? NULL : tv) < 0)
		TEST_FAIL_MESSAGE(strerror(errno));

	if (stat(FILENAME, &statbuf) < 0)
		TEST_FAIL_MESSAGE(strerror(errno));

	adiff_sec = tv[0].tv_sec - statbuf.st_atim.tv_sec;
	// adiff_nsec = tv[0].tv_usec * 1000 - statbuf.st_atim.tv_nsec;
	mdiff_sec = tv[1].tv_sec - statbuf.st_mtim.tv_sec;
	// mdiff_nsec = tv[1].tv_usec * 1000 - statbuf.st_mtim.tv_nsec;

	/* FIXME: only second accuracy is supported */
	if (test != test_set_past) {
		TEST_ASSERT_LESS_THAN(2, adiff_sec);
		TEST_ASSERT_LESS_THAN(2, mdiff_sec);
	}
	else {
		TEST_ASSERT_EQUAL_INT(0, adiff_sec);
		// TEST_ASSERT_EQUAL_INT(0, adiff_nsec);
		TEST_ASSERT_EQUAL_INT(0, mdiff_sec);
		// TEST_ASSERT_EQUAL_INT(0, mdiff_nsec);
	}
}


TEST(test_utimes, set_null)
{
	unsigned int i;

	for (i = 0; i < LOOP_CNT; ++i) {
		usleep(random() % 500000);
		test_utimes(FILENAME, test_set_null);
	}
}


TEST(test_utimes, set_now)
{
	unsigned int i;

	for (i = 0; i < LOOP_CNT; ++i) {
		usleep(random() % 500000);
		test_utimes(FILENAME, test_set_now);
	}
}


TEST(test_utimes, set_past)
{
	unsigned int i;

	for (i = 0; i < LOOP_CNT; ++i) {
		test_utimes(FILENAME, test_set_past);
	}
}


TEST_GROUP(test_futimes);


TEST_SETUP(test_futimes)
{
	int fd;

	if ((fd = creat(FILENAME, 0644)) < 0)
		TEST_FAIL_MESSAGE(strerror(errno));
	close(fd);
}


TEST_TEAR_DOWN(test_futimes)
{
	unlink(FILENAME);
}


void test_futimes(const char *filename, test_t test)
{
	int fd;
	struct timeval tv[2];
	struct stat statbuf;
	int64_t adiff_sec, mdiff_sec;
	// int64_t adiff_nsec, mdiff_nsec;

	if ((fd = open(FILENAME, O_RDONLY)) < 0)
		TEST_FAIL_MESSAGE(strerror(errno));

	if (test != test_set_past) {
		if (gettimeofday(&tv[0], NULL) < 0)
			TEST_FAIL_MESSAGE(strerror(errno));
		tv[1] = tv[0];
	}
	else {
		tv[0].tv_sec = random();
		tv[1].tv_sec = random();
		tv[0].tv_usec = random() % 1000;
		tv[1].tv_usec = random() % 1000;
	}

	if (futimes(fd, test == test_set_null ? NULL : tv) < 0)
		TEST_FAIL_MESSAGE(strerror(errno));

	if (stat(FILENAME, &statbuf) < 0)
		TEST_FAIL_MESSAGE(strerror(errno));

	close(fd);

	adiff_sec = tv[0].tv_sec - statbuf.st_atim.tv_sec;
	// adiff_nsec = tv[0].tv_usec * 1000 - statbuf.st_atim.tv_nsec;
	mdiff_sec = tv[1].tv_sec - statbuf.st_mtim.tv_sec;
	// mdiff_nsec = tv[1].tv_usec * 1000 - statbuf.st_mtim.tv_nsec;

	/* FIXME: only second accuracy is supported */
	if (test != test_set_past) {
		TEST_ASSERT_LESS_THAN_INT(2, adiff_sec);
		TEST_ASSERT_LESS_THAN_INT(2, mdiff_sec);
	}
	else {
		TEST_ASSERT_EQUAL_INT(0, adiff_sec);
		// TEST_ASSERT_EQUAL_INT(0, adiff_nsec);
		TEST_ASSERT_EQUAL_INT(0, mdiff_sec);
		// TEST_ASSERT_EQUAL_INT(0, mdiff_nsec);
	}
}


TEST(test_futimes, set_null)
{
	unsigned int i;

	for (i = 0; i < LOOP_CNT; ++i) {
		usleep(random() % 500000);
		test_futimes(FILENAME, test_set_null);
	}
}


TEST(test_futimes, set_now)
{
	unsigned int i;

	for (i = 0; i < LOOP_CNT; ++i) {
		usleep(random() % 500000);
		test_futimes(FILENAME, test_set_now);
	}
}


TEST(test_futimes, set_past)
{
	unsigned int i;

	for (i = 0; i < LOOP_CNT; ++i) {
		test_futimes(FILENAME, test_set_past);
	}
}


TEST_GROUP(test_lutimes);


TEST_SETUP(test_lutimes)
{
	int fd;

	if ((fd = creat(FILENAME, 0644)) < 0)
		TEST_FAIL_MESSAGE(strerror(errno));
	close(fd);

	if (symlink(FILENAME, LINKNAME) < 0)
		TEST_FAIL_MESSAGE(strerror(errno));
}


TEST_TEAR_DOWN(test_lutimes)
{
	unlink(LINKNAME);
	unlink(FILENAME);
}


void test_lutimes(const char *filename, test_t test)
{
	struct timeval tv[2];
	struct stat statbuf;
	int64_t adiff_sec, mdiff_sec;
	// int64_t adiff_nsec, mdiff_nsec;

	if (test != test_set_past) {
		if (gettimeofday(&tv[0], NULL) < 0)
			TEST_FAIL_MESSAGE(strerror(errno));
		tv[1] = tv[0];
	}
	else {
		tv[0].tv_sec = random();
		tv[1].tv_sec = random();
		tv[0].tv_usec = random() % 1000;
		tv[1].tv_usec = random() % 1000;
	}

	if (lutimes(filename, test == test_set_null ? NULL : tv) < 0)
		TEST_FAIL_MESSAGE(strerror(errno));

	if (lstat(FILENAME, &statbuf) < 0)
		TEST_FAIL_MESSAGE(strerror(errno));

	adiff_sec = tv[0].tv_sec - statbuf.st_atim.tv_sec;
	// adiff_nsec = tv[0].tv_usec * 1000 - statbuf.st_atim.tv_nsec;
	mdiff_sec = tv[1].tv_sec - statbuf.st_mtim.tv_sec;
	// mdiff_nsec = tv[1].tv_usec * 1000 - statbuf.st_mtim.tv_nsec;

	/* FIXME: only second accuracy is supported */
	if (test != test_set_past) {
		TEST_ASSERT_LESS_THAN_INT(2, adiff_sec);
		TEST_ASSERT_LESS_THAN_INT(2, mdiff_sec);
	}
	else {
		TEST_ASSERT_EQUAL_INT(0, adiff_sec);
		// TEST_ASSERT_EQUAL_INT(0, adiff_nsec);
		TEST_ASSERT_EQUAL_INT(0, mdiff_sec);
		// TEST_ASSERT_EQUAL_INT(0, mdiff_nsec);
	}
}


TEST(test_lutimes, set_null)
{
	unsigned int i;

	for (i = 0; i < LOOP_CNT; ++i) {
		usleep(random() % 500000);
		test_lutimes(FILENAME, test_set_null);
	}
}


TEST(test_lutimes, set_now)
{
	unsigned int i;

	for (i = 0; i < LOOP_CNT; ++i) {
		usleep(random() % 500000);
		test_lutimes(FILENAME, test_set_now);
	}
}


TEST(test_lutimes, set_past)
{
	unsigned int i;

	for (i = 0; i < LOOP_CNT; ++i) {
		test_lutimes(FILENAME, test_set_past);
	}
}


TEST_GROUP_RUNNER(test_utimes)
{
	RUN_TEST_CASE(test_utimes, set_null);
	RUN_TEST_CASE(test_utimes, set_now);
	RUN_TEST_CASE(test_utimes, set_past);
}


TEST_GROUP_RUNNER(test_futimes)
{
	RUN_TEST_CASE(test_futimes, set_null);
	RUN_TEST_CASE(test_futimes, set_now);
	RUN_TEST_CASE(test_futimes, set_past);
}


TEST_GROUP_RUNNER(test_lutimes)
{
	RUN_TEST_CASE(test_lutimes, set_null);
	RUN_TEST_CASE(test_lutimes, set_now);
	RUN_TEST_CASE(test_lutimes, set_past);
}


void runner(void)
{
	RUN_TEST_GROUP(test_utimes);
	RUN_TEST_GROUP(test_futimes);
	RUN_TEST_GROUP(test_lutimes);
}


int main(int argc, char *argv[])
{
	UnityMain(argc, (const char **)argv, runner);
	return 0;
}
