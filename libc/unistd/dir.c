/*
 * Phoenix-RTOS
 *
 * Tests for *dir functions behaviour
 *
 * Copyright 2026 Phoenix Systems
 * Author: Michal Lach
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "unity_fixture.h"

#define TEST_DIR "/test_dir"


TEST_GROUP(readdir);


TEST_SETUP(readdir)
{
	TEST_ASSERT_EQUAL_INT(0, mkdir(TEST_DIR, 0666));
}


TEST_TEAR_DOWN(readdir)
{
	TEST_ASSERT_EQUAL_INT(0, rmdir(TEST_DIR));
}


TEST_GROUP_RUNNER(readdir)
{
	RUN_TEST_CASE(readdir, enoent);
}


TEST(readdir, enoent)
{
	int saved;

	DIR *d = opendir(TEST_DIR);
	TEST_ASSERT_NOT_NULL(d);
	/* clang-format off */
	errno = 0;
	while (readdir(d));
	/* clang-format on */
	saved = errno;

	TEST_ASSERT_EQUAL_INT(0, saved);
	TEST_ASSERT_EQUAL_INT(0, closedir(d));
}
