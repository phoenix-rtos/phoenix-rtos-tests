/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <sys/stat.h>
 *    - <unistd.h>
 *    - <fcntl.h>
 * TESTED:
 *    - fchdir()
 *    - fstatat()
 *    - mkdirat()
 *    - mkfifoat()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <limits.h>

#include "unity_fixture.h"

#define FILEOPS_TEST_DIR   "/tmp/test_fileops_dir"
#define FILEOPS_TEST_FILE  "/tmp/test_fileops_file"
#define FILEOPS_TEST_FIFO  "/tmp/test_fileops_fifo"
#define FILEOPS_SUBDIR     "test_fileops_subdir"
#define FILEOPS_SUBFIFO    "test_fileops_subfifo"

static struct {
	int dirFd;
	int fileFd;
	char origCwd[PATH_MAX];
} test_common;


/* ========================================================================= */
/* fchdir */
/* ========================================================================= */

TEST_GROUP(fileops_fchdir);

TEST_SETUP(fileops_fchdir)
{
	int ret;

	test_common.dirFd = -1;
	test_common.fileFd = -1;

	ret = (getcwd(test_common.origCwd, sizeof(test_common.origCwd)) != NULL) ? 0 : -1;
	TEST_ASSERT_EQUAL_INT(0, ret);

	unlink(FILEOPS_TEST_FILE);
	rmdir(FILEOPS_TEST_DIR);
	ret = mkdir(FILEOPS_TEST_DIR, 0755);
	TEST_ASSERT_EQUAL_INT(0, ret);
}

TEST_TEAR_DOWN(fileops_fchdir)
{
	if (test_common.dirFd >= 0) {
		close(test_common.dirFd);
		test_common.dirFd = -1;
	}
	if (test_common.fileFd >= 0) {
		close(test_common.fileFd);
		test_common.fileFd = -1;
	}
	if (chdir(test_common.origCwd) < 0) {
		/* best effort */
	}
	rmdir(FILEOPS_TEST_DIR);
	unlink(FILEOPS_TEST_FILE);
}


TEST(fileops_fchdir, fchdir_changes_cwd)
{
	int ret;
	char cwd[PATH_MAX];

	test_common.dirFd = open(FILEOPS_TEST_DIR, O_RDONLY | O_DIRECTORY);
	TEST_ASSERT_TRUE(test_common.dirFd >= 0);

	ret = fchdir(test_common.dirFd);
	TEST_ASSERT_EQUAL_INT(0, ret);

	TEST_ASSERT_NOT_NULL(getcwd(cwd, sizeof(cwd)));
	TEST_ASSERT_EQUAL_STRING(FILEOPS_TEST_DIR, cwd);
}


TEST(fileops_fchdir, fchdir_ebadf)
{
	int ret;

	errno = 0;
	ret = fchdir(-1);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


TEST(fileops_fchdir, fchdir_enotdir)
{
	int ret;

	test_common.fileFd = open(FILEOPS_TEST_FILE, O_RDWR | O_CREAT | O_TRUNC, 0644);
	TEST_ASSERT_TRUE(test_common.fileFd >= 0);

	errno = 0;
	ret = fchdir(test_common.fileFd);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(ENOTDIR, errno);
}


TEST(fileops_fchdir, fchdir_returns_zero_on_success)
{
	int ret;

	test_common.dirFd = open(FILEOPS_TEST_DIR, O_RDONLY | O_DIRECTORY);
	TEST_ASSERT_TRUE(test_common.dirFd >= 0);

	ret = fchdir(test_common.dirFd);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST_GROUP_RUNNER(fileops_fchdir)
{
	RUN_TEST_CASE(fileops_fchdir, fchdir_changes_cwd);
	RUN_TEST_CASE(fileops_fchdir, fchdir_ebadf);
	RUN_TEST_CASE(fileops_fchdir, fchdir_enotdir);
	RUN_TEST_CASE(fileops_fchdir, fchdir_returns_zero_on_success);
}


/* ========================================================================= */
/* fstatat */
/* ========================================================================= */

TEST_GROUP(fileops_fstatat);

TEST_SETUP(fileops_fstatat)
{
	test_common.dirFd = -1;
	test_common.fileFd = -1;
	unlink(FILEOPS_TEST_FILE);

	test_common.fileFd = open(FILEOPS_TEST_FILE, O_RDWR | O_CREAT | O_TRUNC, 0644);
	TEST_ASSERT_TRUE(test_common.fileFd >= 0);

	test_common.dirFd = open("/tmp", O_RDONLY | O_DIRECTORY);
	TEST_ASSERT_TRUE(test_common.dirFd >= 0);
}

TEST_TEAR_DOWN(fileops_fstatat)
{
	if (test_common.fileFd >= 0) {
		close(test_common.fileFd);
		test_common.fileFd = -1;
	}
	if (test_common.dirFd >= 0) {
		close(test_common.dirFd);
		test_common.dirFd = -1;
	}
	unlink(FILEOPS_TEST_FILE);
}


TEST(fileops_fstatat, fstatat_relative_path)
{
	int ret;
	struct stat st;

	ret = fstatat(test_common.dirFd, "test_fileops_file", &st, 0);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_TRUE(S_ISREG(st.st_mode));
}


TEST(fileops_fstatat, fstatat_at_fdcwd)
{
	int ret;
	struct stat st;

	ret = fstatat(AT_FDCWD, FILEOPS_TEST_FILE, &st, 0);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_TRUE(S_ISREG(st.st_mode));
}


TEST(fileops_fstatat, fstatat_absolute_path_with_fd)
{
	int ret;
	struct stat st;

	ret = fstatat(test_common.dirFd, FILEOPS_TEST_FILE, &st, 0);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_TRUE(S_ISREG(st.st_mode));
}


TEST(fileops_fstatat, fstatat_at_symlink_nofollow)
{
	int ret;
	struct stat st;
	const char *linkPath = "/tmp/test_fileops_link";

	unlink(linkPath);
	ret = symlink(FILEOPS_TEST_FILE, linkPath);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = fstatat(AT_FDCWD, linkPath, &st, AT_SYMLINK_NOFOLLOW);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_TRUE(S_ISLNK(st.st_mode));

	/* Without the flag, should follow the link */
	ret = fstatat(AT_FDCWD, linkPath, &st, 0);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_TRUE(S_ISREG(st.st_mode));

	unlink(linkPath);
}


TEST(fileops_fstatat, fstatat_enoent)
{
	int ret;
	struct stat st;

	errno = 0;
	ret = fstatat(test_common.dirFd, "nonexistent_fileops", &st, 0);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);
}


TEST(fileops_fstatat, fstatat_ebadf)
{
	int ret;
	struct stat st;

	errno = 0;
	ret = fstatat(-1, "test_fileops_file", &st, 0);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


TEST(fileops_fstatat, fstatat_enotdir_fd_not_dir)
{
	int ret;
	struct stat st;

	errno = 0;
	ret = fstatat(test_common.fileFd, "test_fileops_file", &st, 0);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(ENOTDIR, errno);
}


TEST_GROUP_RUNNER(fileops_fstatat)
{
	RUN_TEST_CASE(fileops_fstatat, fstatat_relative_path);
	RUN_TEST_CASE(fileops_fstatat, fstatat_at_fdcwd);
	RUN_TEST_CASE(fileops_fstatat, fstatat_absolute_path_with_fd);
	RUN_TEST_CASE(fileops_fstatat, fstatat_at_symlink_nofollow);
	RUN_TEST_CASE(fileops_fstatat, fstatat_enoent);
	RUN_TEST_CASE(fileops_fstatat, fstatat_ebadf);
	RUN_TEST_CASE(fileops_fstatat, fstatat_enotdir_fd_not_dir);
}


/* ========================================================================= */
/* mkdirat */
/* ========================================================================= */

TEST_GROUP(fileops_mkdirat);

TEST_SETUP(fileops_mkdirat)
{
	test_common.dirFd = -1;
	rmdir("/tmp/" FILEOPS_SUBDIR);

	test_common.dirFd = open("/tmp", O_RDONLY | O_DIRECTORY);
	TEST_ASSERT_TRUE(test_common.dirFd >= 0);
}

TEST_TEAR_DOWN(fileops_mkdirat)
{
	if (test_common.dirFd >= 0) {
		close(test_common.dirFd);
		test_common.dirFd = -1;
	}
	rmdir("/tmp/" FILEOPS_SUBDIR);
}


TEST(fileops_mkdirat, mkdirat_creates_directory)
{
	int ret;
	struct stat st;

	ret = mkdirat(test_common.dirFd, FILEOPS_SUBDIR, 0755);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = fstatat(test_common.dirFd, FILEOPS_SUBDIR, &st, 0);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_TRUE(S_ISDIR(st.st_mode));
}


TEST(fileops_mkdirat, mkdirat_permissions_applied)
{
	int ret;
	struct stat st;
	mode_t oldMask;

	/* Set umask to 0 so permissions are exact */
	oldMask = umask(0);

	ret = mkdirat(test_common.dirFd, FILEOPS_SUBDIR, 0750);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = fstatat(test_common.dirFd, FILEOPS_SUBDIR, &st, 0);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0750, st.st_mode & 0777);

	umask(oldMask);
}


TEST(fileops_mkdirat, mkdirat_at_fdcwd)
{
	int ret;
	struct stat st;
	const char *dirPath = "/tmp/" FILEOPS_SUBDIR;

	ret = mkdirat(AT_FDCWD, dirPath, 0755);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = stat(dirPath, &st);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_TRUE(S_ISDIR(st.st_mode));
}


TEST(fileops_mkdirat, mkdirat_eexist)
{
	int ret;

	ret = mkdirat(test_common.dirFd, FILEOPS_SUBDIR, 0755);
	TEST_ASSERT_EQUAL_INT(0, ret);

	errno = 0;
	ret = mkdirat(test_common.dirFd, FILEOPS_SUBDIR, 0755);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EEXIST, errno);
}


TEST(fileops_mkdirat, mkdirat_enoent_missing_component)
{
	int ret;

	errno = 0;
	ret = mkdirat(test_common.dirFd, "nonexistent_parent/subdir", 0755);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);
}


TEST(fileops_mkdirat, mkdirat_ebadf)
{
	int ret;

	errno = 0;
	ret = mkdirat(-1, FILEOPS_SUBDIR, 0755);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


TEST_GROUP_RUNNER(fileops_mkdirat)
{
	RUN_TEST_CASE(fileops_mkdirat, mkdirat_creates_directory);
	RUN_TEST_CASE(fileops_mkdirat, mkdirat_permissions_applied);
	RUN_TEST_CASE(fileops_mkdirat, mkdirat_at_fdcwd);
	RUN_TEST_CASE(fileops_mkdirat, mkdirat_eexist);
	RUN_TEST_CASE(fileops_mkdirat, mkdirat_enoent_missing_component);
	RUN_TEST_CASE(fileops_mkdirat, mkdirat_ebadf);
}


/* ========================================================================= */
/* mkfifoat */
/* ========================================================================= */

TEST_GROUP(fileops_mkfifoat);

TEST_SETUP(fileops_mkfifoat)
{
	test_common.dirFd = -1;
	unlink("/tmp/" FILEOPS_SUBFIFO);

	test_common.dirFd = open("/tmp", O_RDONLY | O_DIRECTORY);
	TEST_ASSERT_TRUE(test_common.dirFd >= 0);
}

TEST_TEAR_DOWN(fileops_mkfifoat)
{
	if (test_common.dirFd >= 0) {
		close(test_common.dirFd);
		test_common.dirFd = -1;
	}
	unlink("/tmp/" FILEOPS_SUBFIFO);
}


TEST(fileops_mkfifoat, mkfifoat_creates_fifo)
{
	int ret;
	struct stat st;

	ret = mkfifoat(test_common.dirFd, FILEOPS_SUBFIFO, 0644);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = fstatat(test_common.dirFd, FILEOPS_SUBFIFO, &st, 0);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_TRUE(S_ISFIFO(st.st_mode));
}


TEST(fileops_mkfifoat, mkfifoat_permissions_applied)
{
	int ret;
	struct stat st;
	mode_t oldMask;

	oldMask = umask(0);

	ret = mkfifoat(test_common.dirFd, FILEOPS_SUBFIFO, 0640);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = fstatat(test_common.dirFd, FILEOPS_SUBFIFO, &st, 0);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0640, st.st_mode & 0777);

	umask(oldMask);
}


TEST(fileops_mkfifoat, mkfifoat_at_fdcwd)
{
	int ret;
	struct stat st;
	const char *fifoPath = "/tmp/" FILEOPS_SUBFIFO;

	ret = mkfifoat(AT_FDCWD, fifoPath, 0644);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = stat(fifoPath, &st);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_TRUE(S_ISFIFO(st.st_mode));
}


TEST(fileops_mkfifoat, mkfifoat_eexist)
{
	int ret;

	ret = mkfifoat(test_common.dirFd, FILEOPS_SUBFIFO, 0644);
	TEST_ASSERT_EQUAL_INT(0, ret);

	errno = 0;
	ret = mkfifoat(test_common.dirFd, FILEOPS_SUBFIFO, 0644);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EEXIST, errno);
}


TEST(fileops_mkfifoat, mkfifoat_enoent)
{
	int ret;

	errno = 0;
	ret = mkfifoat(test_common.dirFd, "no_such_parent/fifo", 0644);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);
}


TEST(fileops_mkfifoat, mkfifoat_ebadf)
{
	int ret;

	errno = 0;
	ret = mkfifoat(-1, FILEOPS_SUBFIFO, 0644);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


TEST_GROUP_RUNNER(fileops_mkfifoat)
{
	RUN_TEST_CASE(fileops_mkfifoat, mkfifoat_creates_fifo);
	RUN_TEST_CASE(fileops_mkfifoat, mkfifoat_permissions_applied);
	RUN_TEST_CASE(fileops_mkfifoat, mkfifoat_at_fdcwd);
	RUN_TEST_CASE(fileops_mkfifoat, mkfifoat_eexist);
	RUN_TEST_CASE(fileops_mkfifoat, mkfifoat_enoent);
	RUN_TEST_CASE(fileops_mkfifoat, mkfifoat_ebadf);
}
