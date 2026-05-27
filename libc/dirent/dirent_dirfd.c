/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <dirent.h>
 * TESTED:
 *    - dirfd()
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
#include <limits.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <unity_fixture.h>

#define DIRFD_TEST_DIR "dirfd_test_dir"

#ifndef __phoenix__

static struct {
	DIR *dirp;
} test_common;


TEST_GROUP(dirent_dirfd);


TEST_SETUP(dirent_dirfd)
{
	(void)mkdir(DIRFD_TEST_DIR, 0755);
	test_common.dirp = NULL;
}


TEST_TEAR_DOWN(dirent_dirfd)
{
	if (test_common.dirp != NULL) {
		closedir(test_common.dirp);
	}
	rmdir(DIRFD_TEST_DIR);
}


/* dirfd: shall return a non-negative file descriptor for a valid DIR stream */
TEST(dirent_dirfd, returns_valid_fd)
{
	int fd;

	test_common.dirp = opendir(DIRFD_TEST_DIR);
	TEST_ASSERT_NOT_NULL(test_common.dirp);

	fd = dirfd(test_common.dirp);
	TEST_ASSERT_TRUE(fd >= 0);
}


/* dirfd: returned fd shall refer to the same directory (usable with fchdir) */
TEST(dirent_dirfd, fd_refers_to_same_directory)
{
	int fd;
	int ret;
	char cwd[PATH_MAX];
	char expected[PATH_MAX];
	char *res;

	/* Get absolute path of test directory */
	res = realpath(DIRFD_TEST_DIR, expected);
	TEST_ASSERT_NOT_NULL(res);

	test_common.dirp = opendir(DIRFD_TEST_DIR);
	TEST_ASSERT_NOT_NULL(test_common.dirp);

	fd = dirfd(test_common.dirp);
	TEST_ASSERT_TRUE(fd >= 0);

	/* Save current dir */
	res = getcwd(cwd, sizeof(cwd));
	TEST_ASSERT_NOT_NULL(res);

	/* fchdir using the fd from dirfd — should go to the same directory */
	ret = fchdir(fd);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Verify we are in the expected directory */
	char newcwd[PATH_MAX];
	res = getcwd(newcwd, sizeof(newcwd));
	TEST_ASSERT_NOT_NULL(res);
	TEST_ASSERT_EQUAL_STRING(expected, newcwd);

	/* Restore original directory */
	ret = chdir(cwd);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


/* dirfd: returned fd is consistent across multiple calls on same DIR */
TEST(dirent_dirfd, consistent_fd_across_calls)
{
	int fd1;
	int fd2;

	test_common.dirp = opendir(DIRFD_TEST_DIR);
	TEST_ASSERT_NOT_NULL(test_common.dirp);

	fd1 = dirfd(test_common.dirp);
	TEST_ASSERT_TRUE(fd1 >= 0);

	fd2 = dirfd(test_common.dirp);
	TEST_ASSERT_TRUE(fd2 >= 0);

	TEST_ASSERT_EQUAL_INT(fd1, fd2);
}


/* dirfd: fd remains valid after readdir operations */
TEST(dirent_dirfd, fd_valid_after_readdir)
{
	int fd;
	struct stat st;
	int ret;

	test_common.dirp = opendir(DIRFD_TEST_DIR);
	TEST_ASSERT_NOT_NULL(test_common.dirp);

	/* Perform some readdir operations */
	(void)readdir(test_common.dirp);
	(void)readdir(test_common.dirp);

	fd = dirfd(test_common.dirp);
	TEST_ASSERT_TRUE(fd >= 0);

	/* Verify fd is still usable */
	ret = fstat(fd, &st);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_TRUE(S_ISDIR(st.st_mode));
}


/* dirfd: fd remains valid after rewinddir */
TEST(dirent_dirfd, fd_valid_after_rewinddir)
{
	int fd;
	struct stat st;
	int ret;

	test_common.dirp = opendir(DIRFD_TEST_DIR);
	TEST_ASSERT_NOT_NULL(test_common.dirp);

	(void)readdir(test_common.dirp);
	rewinddir(test_common.dirp);

	fd = dirfd(test_common.dirp);
	TEST_ASSERT_TRUE(fd >= 0);

	ret = fstat(fd, &st);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_TRUE(S_ISDIR(st.st_mode));
}


/* dirfd: fd from fdopendir matches the original fd */
TEST(dirent_dirfd, fdopendir_returns_same_fd)
{
	int origFd;
	int gotFd;

	origFd = open(DIRFD_TEST_DIR, O_RDONLY | O_DIRECTORY);
	TEST_ASSERT_TRUE(origFd >= 0);

	test_common.dirp = fdopendir(origFd);
	TEST_ASSERT_NOT_NULL(test_common.dirp);

	gotFd = dirfd(test_common.dirp);
	TEST_ASSERT_EQUAL_INT(origFd, gotFd);
}


TEST_GROUP_RUNNER(dirent_dirfd)
{
	RUN_TEST_CASE(dirent_dirfd, returns_valid_fd);
	RUN_TEST_CASE(dirent_dirfd, fd_refers_to_same_directory);
	RUN_TEST_CASE(dirent_dirfd, consistent_fd_across_calls);
	RUN_TEST_CASE(dirent_dirfd, fd_valid_after_readdir);
	RUN_TEST_CASE(dirent_dirfd, fd_valid_after_rewinddir);
	RUN_TEST_CASE(dirent_dirfd, fdopendir_returns_same_fd);
}

#else
TEST_GROUP_UNIMPLEMENTED(dirent_dirfd, "dirfd not implemented")
#endif
