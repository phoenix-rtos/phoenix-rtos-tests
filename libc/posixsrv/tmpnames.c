/*
 * Phoenix-RTOS
 *
 * libc/posixsrv
 *
 * tests for tmpnam()/tempnam()
 * these tests are in posixsrv dir as they use
 * /dev/urandom for random name generation
 *
 * Copyright 2026 Phoenix Systems
 * Author: Julian Uziembło
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <sys/stat.h>

#include "unity_fixture.h"


static char *name_alloc, *name_alloc2;


TEST_GROUP(tmpnames);


TEST_SETUP(tmpnames)
{
	name_alloc = NULL;
	name_alloc2 = NULL;
}


TEST_TEAR_DOWN(tmpnames)
{
	if (name_alloc != NULL) {
		free(name_alloc);
		name_alloc = NULL;
	}

	if (name_alloc2 != NULL) {
		free(name_alloc2);
		name_alloc2 = NULL;
	}
}


TEST(tmpnames, tmpnam_null)
{
	char *name;

	name = tmpnam(NULL);

	TEST_ASSERT_NOT_NULL(name);
	TEST_ASSERT_GREATER_THAN(0, strlen(name));
}


TEST(tmpnames, tmpnam_buf)
{
	char buf[L_tmpnam];
	char *ret;

	memset(buf, 0xaa, sizeof(buf));

	errno = 0;
	ret = tmpnam(buf);

	TEST_ASSERT_EQUAL_PTR(buf, ret);
	TEST_ASSERT_GREATER_THAN(0, strlen(buf));
}


TEST(tmpnames, tmpnam_multiple)
{
	char a[L_tmpnam];
	char b[L_tmpnam];

	TEST_ASSERT_NOT_NULL(tmpnam(a));
	TEST_ASSERT_NOT_NULL(tmpnam(b));

	TEST_ASSERT_NOT_EQUAL(0, strcmp(a, b));
}


TEST(tmpnames, tmpnam_unique)
{
	char buf[L_tmpnam];

	TEST_ASSERT_NOT_NULL(tmpnam(buf));

	errno = 0;

	TEST_ASSERT_EQUAL(-1, access(buf, F_OK));
	TEST_ASSERT_EQUAL(ENOENT, errno);
}


TEST(tmpnames, tmpnam_internal_buf)
{
	char *a;
	char *b;
	char saved[L_tmpnam];

	a = tmpnam(NULL);
	TEST_ASSERT_NOT_NULL(a);

	strcpy(saved, a);

	b = tmpnam(NULL);
	TEST_ASSERT_NOT_NULL(b);

	/*
	 * POSIX allows same static buffer to be reused,
	 * so verify old contents changed or new name differs.
	 */
	TEST_ASSERT_NOT_EQUAL(0, strcmp(saved, b));
}


TEST(tmpnames, tmpnam_many_unique)
{
	static char names[128][L_tmpnam];
	int i, j;

	for (i = 0; i < 128; ++i) {
		TEST_ASSERT_NOT_NULL(tmpnam(names[i]));
	}

	for (i = 0; i < 128; ++i) {
		for (j = i + 1; j < 128; ++j) {
			TEST_ASSERT_NOT_EQUAL(0, strcmp(names[i], names[j]));
		}
	}
}


TEST(tmpnames, tempnam_basic)
{
	name_alloc = tempnam(NULL, NULL);

	TEST_ASSERT_NOT_NULL(name_alloc);
	TEST_ASSERT_GREATER_THAN(0, strlen(name_alloc));
}


TEST(tmpnames, tempnam_prefix)
{
	const char *prefix = "abcd_";

	name_alloc = tempnam(NULL, prefix);

	TEST_ASSERT_NOT_NULL(name_alloc);

	/* POSIX does not require exact placement */
	TEST_ASSERT_NOT_NULL(strstr(name_alloc, prefix));
}


TEST(tmpnames, tempnam_unique)
{
	name_alloc = tempnam(NULL, "tst");

	TEST_ASSERT_NOT_NULL(name_alloc);

	errno = 0;

	TEST_ASSERT_EQUAL(-1, access(name_alloc, F_OK));
	TEST_ASSERT_EQUAL(ENOENT, errno);
}


TEST(tmpnames, tempnam_dir_exist)
{
	/* ensure the directory exists */
	mkdir("/tmp", 0777);

	name_alloc = tempnam("/tmp", "abc");

	TEST_ASSERT_NOT_NULL(name_alloc);

	TEST_ASSERT_EQUAL(0, strncmp(name_alloc, "/tmp/", 5));
}


TEST(tmpnames, tempnam_dir_nonexistent)
{
	name_alloc = tempnam("/definitely/nonexistent/path", "abc");

	TEST_ASSERT_NOT_NULL(name_alloc);

	TEST_ASSERT_GREATER_THAN(0, strlen(name_alloc));
}


TEST(tmpnames, tempnam_multiple)
{
	name_alloc = tempnam(NULL, "u");
	name_alloc2 = tempnam(NULL, "u");

	TEST_ASSERT_NOT_NULL(name_alloc);
	TEST_ASSERT_NOT_NULL(name_alloc2);

	TEST_ASSERT_NOT_EQUAL(0, strcmp(name_alloc, name_alloc2));
}


TEST(tmpnames, tempnam_empty_prefix)
{
	name_alloc = tempnam(NULL, "");

	TEST_ASSERT_NOT_NULL(name_alloc);
	TEST_ASSERT_GREATER_THAN(0, strlen(name_alloc));
}


TEST_GROUP_RUNNER(tmpnames)
{
	RUN_TEST_CASE(tmpnames, tmpnam_null);
	RUN_TEST_CASE(tmpnames, tmpnam_buf);
	RUN_TEST_CASE(tmpnames, tmpnam_multiple);
	RUN_TEST_CASE(tmpnames, tmpnam_unique);
	RUN_TEST_CASE(tmpnames, tmpnam_internal_buf);
	RUN_TEST_CASE(tmpnames, tmpnam_many_unique);

	RUN_TEST_CASE(tmpnames, tempnam_basic);
	RUN_TEST_CASE(tmpnames, tempnam_prefix);
	RUN_TEST_CASE(tmpnames, tempnam_unique);
	RUN_TEST_CASE(tmpnames, tempnam_dir_exist);
	RUN_TEST_CASE(tmpnames, tempnam_dir_nonexistent);
	RUN_TEST_CASE(tmpnames, tempnam_multiple);
	RUN_TEST_CASE(tmpnames, tempnam_empty_prefix);
}
