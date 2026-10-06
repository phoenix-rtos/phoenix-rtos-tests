/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <stdlib.h>
 * TESTED:
 *    - mkdtemp()
 *    - mkstemp()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>
#include <fcntl.h>

#include <unity_fixture.h>

#define MKTEMP_DIR       "mktemp_testdir"
#define MKTEMP_TEMPLATE  "mktemp_testXXXXXX"
#define MKTEMP_PREFIX    "mktemp_test"
#define MKTEMP_SUFFIX_LEN 6


static struct {
	char tmpl[PATH_MAX];
	char *dirResult;
	int fd;
} test_common;


TEST_GROUP(stdlib_mkdtemp);


TEST_SETUP(stdlib_mkdtemp)
{
	test_common.dirResult = NULL;
	rmdir(MKTEMP_DIR);
}


TEST_TEAR_DOWN(stdlib_mkdtemp)
{
	if (test_common.dirResult != NULL) {
		rmdir(test_common.dirResult);
	}
}


TEST(stdlib_mkdtemp, mkdtemp_einval)
{
	/* template not ending in XXXXXX */
	strncpy(test_common.tmpl, "no_suffix_here", sizeof(test_common.tmpl) - 1);
	test_common.tmpl[sizeof(test_common.tmpl) - 1] = '\0';

	errno = 0;
	TEST_ASSERT_NULL(mkdtemp(test_common.tmpl));
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST(stdlib_mkdtemp, mkdtemp_enoent)
{
	/* non-existing path prefix */
	strncpy(test_common.tmpl, "/nonexistent_dir_xyz/tmpXXXXXX", sizeof(test_common.tmpl) - 1);
	test_common.tmpl[sizeof(test_common.tmpl) - 1] = '\0';

	errno = 0;
	TEST_ASSERT_NULL(mkdtemp(test_common.tmpl));
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);
}


TEST_GROUP_RUNNER(stdlib_mkdtemp)
{
	RUN_TEST_CASE(stdlib_mkdtemp, mkdtemp_einval);
	RUN_TEST_CASE(stdlib_mkdtemp, mkdtemp_enoent);
}

