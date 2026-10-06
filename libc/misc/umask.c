/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <sys/stat.h>
 * TESTED:
 *    - umask()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Lukasz Kruszynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "unity_fixture.h"

#define UMASK_TEST_FILE "/tmp/test_umask_file"
#define UMASK_TEST_DIR  "/tmp/test_umask_dir"
#define UMASK_TEST_FIFO "/tmp/test_umask_fifo"

TEST_GROUP(misc_umask);

TEST_SETUP(misc_umask)
{
	unlink(UMASK_TEST_FILE);
	unlink(UMASK_TEST_FIFO);
	rmdir(UMASK_TEST_DIR);
}

TEST_TEAR_DOWN(misc_umask)
{
	unlink(UMASK_TEST_FILE);
	unlink(UMASK_TEST_FIFO);
	rmdir(UMASK_TEST_DIR);
}


TEST(misc_umask, umask_set_zero)
{
	mode_t prev;
	mode_t ret;

	prev = umask(0);

	ret = umask(prev);
	TEST_ASSERT_EQUAL_INT(0, (ret & 0777));
}


TEST(misc_umask, umask_affects_open_creat_zero_mask)
{
	mode_t prev;
	int fd;
	struct stat st;
	int ret;

	prev = umask(0);

	fd = open(UMASK_TEST_FILE, O_CREAT | O_WRONLY, 0777);
	TEST_ASSERT_NOT_EQUAL_INT(-1, fd);
	close(fd);

	ret = stat(UMASK_TEST_FILE, &st);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* 0777 & ~0 = 0777 */
	TEST_ASSERT_EQUAL_INT(0777, (st.st_mode & 0777));

	umask(prev);
}


TEST(misc_umask, umask_no_errors_defined)
{
	mode_t prev;

	/*
	 * POSIX: "No errors are defined."
	 * umask always succeeds; verify errno is not modified.
	 */
	errno = 0;
	prev = umask(022);
	TEST_ASSERT_EQUAL_INT(0, errno);

	errno = 0;
	umask(prev);
	TEST_ASSERT_EQUAL_INT(0, errno);
}


TEST_GROUP_RUNNER(misc_umask)
{
	RUN_TEST_CASE(misc_umask, umask_set_zero);
	RUN_TEST_CASE(misc_umask, umask_affects_open_creat_zero_mask);
	RUN_TEST_CASE(misc_umask, umask_no_errors_defined);
}
