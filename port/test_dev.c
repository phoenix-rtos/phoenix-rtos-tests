/*
 * Phoenix-RTOS
 *
 * test_dev
 *
 * tests for `create_dev()` and `destroy_dev()` utility functions
 *
 * Copyright 2026 Phoenix Systems
 * Author: Julian Uziembło
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <posix/utils.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <unistd.h>

#include "unity_fixture.h"

#define DEV_PREFIX      "/dev"
#define SIMPLE_DEV_NAME "simple"
#define DIR1            "a"
#define DIR2            "b"
#define DIR3            "c"
#define COMPLEX_PATH    DIR1 "/" DIR2 "/" DIR3

#define TEST_PATH(oid, devpath, fullpath) \
	do { \
		oid_t _odev; \
		TEST_ASSERT_EQUAL_MESSAGE(0, create_dev(&(oid), (devpath)), "create_dev failed"); \
		TEST_ASSERT_EQUAL_MESSAGE(0, lookup((fullpath), NULL, &_odev), "lookup failed"); \
		TEST_ASSERT_EQUAL_MEMORY_MESSAGE(&(oid), &_odev, sizeof(_odev), "oid is not the same"); \
		TEST_ASSERT_EQUAL_MESSAGE(0, destroy_dev((devpath)), "destroy_dev failed"); \
		TEST_ASSERT_LESS_THAN_MESSAGE(0, lookup((fullpath), NULL, &_odev), "lookup succeeded - should have failed"); \
	} while (0)


static uint32_t port;


TEST_GROUP(test_dev);


TEST_SETUP(test_dev)
{
}


TEST_TEAR_DOWN(test_dev)
{
}


/* TODO: revise these tests for `create_dev` once
   the function is refactored; YT: RTOS-1262 */


TEST(test_dev, simple_relpath)
{
	__attribute__((unused)) oid_t tmp;
	if (lookup("devfs", NULL, &tmp) < 0 && lookup("/dev", NULL, &tmp) < 0) {
		TEST_IGNORE_MESSAGE("relpath doesn't work on this target");
	}
	oid_t oid = { .port = port, .id = 0 };
	const char *relpath = SIMPLE_DEV_NAME;
	const char *fullpath = DEV_PREFIX "/" SIMPLE_DEV_NAME;

	/* through only dev name */
	TEST_PATH(oid, relpath, fullpath);
}


TEST(test_dev, simple_fullpath)
{
	oid_t oid = { .port = port, .id = 0 };
	const char *fullpath = DEV_PREFIX "/" SIMPLE_DEV_NAME;

	/* through full path */
	TEST_PATH(oid, fullpath, fullpath);
}


TEST(test_dev, complex_relpath)
{
	__attribute__((unused)) oid_t tmp;
	if (lookup("devfs", NULL, &tmp) < 0 && lookup("/dev", NULL, &tmp) < 0) {
		TEST_IGNORE_MESSAGE("relpath doesn't work on this target");
	}
	oid_t oid = { .port = port, .id = 0 };
	const char *relpath = COMPLEX_PATH "/" SIMPLE_DEV_NAME;
	const char *fullpath = DEV_PREFIX "/" COMPLEX_PATH "/" SIMPLE_DEV_NAME;

	/* through only dev name */
	TEST_PATH(oid, relpath, fullpath);

	/* cleanup */
	TEST_ASSERT_EQUAL(0, rmdir(DEV_PREFIX "/" DIR1 "/" DIR2 "/" DIR3));
	TEST_ASSERT_LESS_THAN(0, lookup(DEV_PREFIX "/" DIR1 "/" DIR2 "/" DIR3, NULL, &oid));
	TEST_ASSERT_EQUAL(0, rmdir(DEV_PREFIX "/" DIR1 "/" DIR2));
	TEST_ASSERT_LESS_THAN(0, lookup(DEV_PREFIX "/" DIR1 "/" DIR2, NULL, &oid));
	TEST_ASSERT_EQUAL(0, rmdir(DEV_PREFIX "/" DIR1));
	TEST_ASSERT_LESS_THAN(0, lookup(DEV_PREFIX "/" DIR1, NULL, &oid));
}


TEST(test_dev, complex_fullpath)
{
	oid_t oid = { .port = port, .id = 0 };
	const char *fullpath = DEV_PREFIX "/" COMPLEX_PATH "/" SIMPLE_DEV_NAME;

	/* through full path */
	TEST_PATH(oid, fullpath, fullpath);

	/* cleanup */
	TEST_ASSERT_EQUAL(0, rmdir(DEV_PREFIX "/" DIR1 "/" DIR2 "/" DIR3));
	TEST_ASSERT_LESS_THAN(0, lookup(DEV_PREFIX "/" DIR1 "/" DIR2 "/" DIR3, NULL, &oid));
	TEST_ASSERT_EQUAL(0, rmdir(DEV_PREFIX "/" DIR1 "/" DIR2));
	TEST_ASSERT_LESS_THAN(0, lookup(DEV_PREFIX "/" DIR1 "/" DIR2, NULL, &oid));
	TEST_ASSERT_EQUAL(0, rmdir(DEV_PREFIX "/" DIR1));
	TEST_ASSERT_LESS_THAN(0, lookup(DEV_PREFIX "/" DIR1, NULL, &oid));
}


TEST_GROUP_RUNNER(test_dev)
{
	RUN_TEST_CASE(test_dev, simple_relpath);
	RUN_TEST_CASE(test_dev, simple_fullpath);
	RUN_TEST_CASE(test_dev, complex_relpath);
	RUN_TEST_CASE(test_dev, complex_fullpath);
}

static void runner(void)
{
	RUN_TEST_GROUP(test_dev);
}


int main(int argc, char *argv[])
{
	int err = portCreate(&port);
	if (err < 0) {
		fprintf(stderr, "Could not create port: %s (%d)\n", strerror(-err), -err);
		return EXIT_FAILURE;
	}
	err = (UnityMain(argc, (const char **)argv, runner) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
	portDestroy(port);
	return err;
}
