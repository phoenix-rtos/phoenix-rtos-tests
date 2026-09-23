/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <unistd.h>
 *    - <fcntl.h>
 * TESTED:
 *    - symlinkat()
 *    - unlinkat()
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
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <limits.h>
#include <stdint.h>

#include "common.h"
#include "fileops_at.h"
#include "unity_fixture.h"

#define LN_DIR          "/tmp/test_fileops_link"
#define LN_FILE_NAME    "file"
#define LN_SUBDIR_NAME  "sub"
#define LN_FULL_NAME    "full"
#define LN_INNER_NAME   "inner"
#define LN_NEW_NAME     "new"
#define LN_NEW2_NAME    "new2"
#define LN_LINK_NAME    "link"
#define LN_DIRLINK_NAME "dirlink"
#define LN_LOOPA_NAME   "loopa"
#define LN_LOOPB_NAME   "loopb"
#define LN_HARD_NAME    "hard"
#define LN_MISSING_NAME "missing"

#define LN_FILE    LN_DIR "/" LN_FILE_NAME
#define LN_SUBDIR  LN_DIR "/" LN_SUBDIR_NAME
#define LN_FULL    LN_DIR "/" LN_FULL_NAME
#define LN_INNER   LN_FULL "/" LN_INNER_NAME
#define LN_NEW     LN_DIR "/" LN_NEW_NAME
#define LN_NEW2    LN_DIR "/" LN_NEW2_NAME
#define LN_LINK    LN_DIR "/" LN_LINK_NAME
#define LN_DIRLINK LN_DIR "/" LN_DIRLINK_NAME
#define LN_LOOPA   LN_DIR "/" LN_LOOPA_NAME
#define LN_LOOPB   LN_DIR "/" LN_LOOPB_NAME
#define LN_HARD    LN_DIR "/" LN_HARD_NAME

#define LN_SUB_NEW_REL  LN_SUBDIR_NAME "/" LN_NEW_NAME
#define LN_SUB_NEW      LN_DIR "/" LN_SUB_NEW_REL
#define LN_SUB_FILE_REL LN_SUBDIR_NAME "/" LN_FILE_NAME
#define LN_SUB_FILE     LN_DIR "/" LN_SUB_FILE_REL

#define LN_DATA   "link test data"
#define LN_TARGET "some_target"

/* strings that are not valid/existing pathnames; symlinkat() shall store them verbatim */
#define LN_ODD_TARGET    "a//b/./../c///"
#define LN_ABS_TARGET    "/no/such/dir/at/all"
#define LN_LONG_TGT_LEN  255
#define LN_NAME_TOO_LONG (NAME_MAX + 2)
/* upper bound for allocating a path1 longer than SYMLINK_MAX */
#define LN_SYMLINK_MAX_CAP 65536L

#define LN_DIR_MODE     0755
#define LN_RO_DIR_MODE  0555
#define LN_NOX_DIR_MODE 0666
#define LN_OLD_TIME     1000000
#define LN_BUF_SIZE     512
#define LN_WAIT_NS      10000000L
#define LN_WAIT_TRIES   300


static struct {
	test_atCtx_t at;
	int fd;
	char *heapBuf;
	char buf[LN_BUF_SIZE];
	char longName[LN_NAME_TOO_LONG + 1];
} test_common;


static void test_removeTree(void)
{
	chmod(LN_SUBDIR, LN_DIR_MODE);
	chmod(LN_FULL, LN_DIR_MODE);
	chmod(LN_DIR, LN_DIR_MODE);

	unlink(LN_FILE);
	unlink(LN_NEW);
	unlink(LN_NEW2);
	unlink(LN_LINK);
	unlink(LN_DIRLINK);
	unlink(LN_LOOPA);
	unlink(LN_LOOPB);
	unlink(LN_HARD);
	unlink(LN_SUB_NEW);
	unlink(LN_SUB_FILE);
	unlink(LN_INNER);
	rmdir(LN_SUBDIR);
	rmdir(LN_FULL);
	rmdir(LN_DIR);
}


static void test_createTree(void)
{
	test_atInit(&test_common.at);
	test_common.fd = -1;
	test_common.heapBuf = NULL;

	memset(test_common.longName, 'n', LN_NAME_TOO_LONG);
	test_common.longName[LN_NAME_TOO_LONG] = '\0';

	test_removeTree();

	TEST_ASSERT_EQUAL_INT(0, mkdir(LN_DIR, LN_DIR_MODE));
	TEST_ASSERT_EQUAL_INT(0, mkdir(LN_SUBDIR, LN_DIR_MODE));
	TEST_ASSERT_EQUAL_INT(0, mkdir(LN_FULL, LN_DIR_MODE));
	create_file(LN_FILE, LN_DATA);
	create_file(LN_INNER, NULL);
	TEST_ASSERT_EQUAL_INT(0, symlink(LN_FILE_NAME, LN_LINK));
	TEST_ASSERT_EQUAL_INT(0, symlink(LN_SUBDIR_NAME, LN_DIRLINK));
	TEST_ASSERT_EQUAL_INT(0, symlink(LN_LOOPB_NAME, LN_LOOPA));
	TEST_ASSERT_EQUAL_INT(0, symlink(LN_LOOPA_NAME, LN_LOOPB));

	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_atOpenDir(&test_common.at, LN_DIR));
}


static void test_destroyTree(void)
{
	if (test_common.fd >= 0) {
		close(test_common.fd);
		test_common.fd = -1;
	}
	free(test_common.heapBuf);
	test_common.heapBuf = NULL;
	test_atRelease(&test_common.at);
	test_removeTree();
}


static int test_exists(const char *path)
{
	struct stat st;

	return (lstat(path, &st) == 0) ? 1 : 0;
}


/* path shall be a symbolic link whose content is exactly expected */
static void test_checkLink(const char *path, const char *expected)
{
	struct stat st;
	ssize_t ret;

	TEST_ASSERT_EQUAL_INT(0, lstat(path, &st));
	TEST_ASSERT_TRUE(S_ISLNK(st.st_mode));

	ret = readlink(path, test_common.buf, sizeof(test_common.buf));
	TEST_ASSERT_EQUAL_INT((ssize_t)strlen(expected), ret);
	TEST_ASSERT_EQUAL_MEMORY(expected, test_common.buf, strlen(expected));
}


/* Busy-waits (with short sleeps) until the wall clock passes second t. */
static void test_waitNextSecond(time_t t)
{
	const struct timespec delay = { 0, LN_WAIT_NS };
	int tries = 0;

	while ((time(NULL) <= t) && (tries < LN_WAIT_TRIES)) {
		(void)nanosleep(&delay, NULL);
		tries++;
	}
	TEST_ASSERT_GREATER_THAN_INT64((int64_t)t, (int64_t)time(NULL));
}


static void test_symlinkatFails(int fd, const char *path2, int expErrno)
{
	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, symlinkat(LN_TARGET, fd, path2));
	TEST_ASSERT_EQUAL_INT(expErrno, errno);
}


static void test_unlinkatFails(int fd, const char *path, int flag, int expErrno)
{
	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, unlinkat(fd, path, flag));
	TEST_ASSERT_EQUAL_INT(expErrno, errno);
}


/* ========================================================================= */
/* symlinkat */
/* ========================================================================= */

TEST_GROUP(fileops_symlinkat);


TEST_SETUP(fileops_symlinkat)
{
	test_createTree();
}


TEST_TEAR_DOWN(fileops_symlinkat)
{
	test_destroyTree();
}


/* Relative path2 is created relative to fd (not cwd); link contains path1; returns 0. */
TEST(fileops_symlinkat, symlinkat_relative_to_fd)
{
	TEST_ASSERT_EQUAL_INT(0, test_atChdir(&test_common.at, LN_SUBDIR));

	TEST_ASSERT_EQUAL_INT(0, symlinkat(LN_TARGET, test_common.at.dirFd, LN_NEW_NAME));

	test_checkLink(LN_NEW, LN_TARGET);
	TEST_ASSERT_FALSE(test_exists(LN_SUB_NEW));
}


/* A link to an existing file resolves to that file. */
TEST(fileops_symlinkat, symlinkat_existing_target_resolves)
{
	struct stat st;

	TEST_ASSERT_EQUAL_INT(0, symlinkat(LN_FILE_NAME, test_common.at.dirFd, LN_NEW_NAME));

	test_checkLink(LN_NEW, LN_FILE_NAME);
	TEST_ASSERT_EQUAL_INT(0, stat(LN_NEW, &st));
	TEST_ASSERT_TRUE(S_ISREG(st.st_mode));
	check_file_contents(LN_DATA, LN_NEW);
}


/* path1 is treated only as a string, not validated as a pathname. */
TEST(fileops_symlinkat, symlinkat_path1_not_validated)
{
	static char longTarget[LN_LONG_TGT_LEN + 1];

	TEST_ASSERT_EQUAL_INT(0, symlinkat(LN_ODD_TARGET, test_common.at.dirFd, LN_NEW_NAME));
	test_checkLink(LN_NEW, LN_ODD_TARGET);
	TEST_ASSERT_EQUAL_INT(0, unlink(LN_NEW));

	TEST_ASSERT_EQUAL_INT(0, symlinkat(LN_ABS_TARGET, test_common.at.dirFd, LN_NEW_NAME));
	test_checkLink(LN_NEW, LN_ABS_TARGET);
	TEST_ASSERT_EQUAL_INT(0, unlink(LN_NEW));

	/* a single component longer than NAME_MAX is still just a string */
	TEST_ASSERT_EQUAL_INT(0, symlinkat(test_common.longName, test_common.at.dirFd, LN_NEW_NAME));
	test_checkLink(LN_NEW, test_common.longName);
	TEST_ASSERT_EQUAL_INT(0, unlink(LN_NEW));

	memset(longTarget, 't', LN_LONG_TGT_LEN);
	longTarget[LN_LONG_TGT_LEN] = '\0';
	TEST_ASSERT_EQUAL_INT(0, symlinkat(longTarget, test_common.at.dirFd, LN_NEW_NAME));
	test_checkLink(LN_NEW, longTarget);
}


/* An absolute path2 is used regardless of fd (even an invalid or non-directory fd). */
TEST(fileops_symlinkat, symlinkat_absolute_ignores_fd)
{
	TEST_ASSERT_EQUAL_INT(0, symlinkat(LN_TARGET, -1, LN_NEW));
	test_checkLink(LN_NEW, LN_TARGET);

	test_common.fd = open(LN_FILE, O_RDONLY);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_common.fd);
	TEST_ASSERT_EQUAL_INT(0, symlinkat(LN_TARGET, test_common.fd, LN_NEW2));
	test_checkLink(LN_NEW2, LN_TARGET);
}


/* AT_FDCWD: path2 is relative to cwd and behaviour is identical to symlink(). */
TEST(fileops_symlinkat, symlinkat_at_fdcwd)
{
	TEST_ASSERT_EQUAL_INT(0, test_atChdir(&test_common.at, LN_DIR));

	TEST_ASSERT_EQUAL_INT(0, symlinkat(LN_TARGET, AT_FDCWD, LN_NEW_NAME));
	TEST_ASSERT_EQUAL_INT(0, symlink(LN_TARGET, LN_NEW2_NAME));
	test_checkLink(LN_NEW, LN_TARGET);
	test_checkLink(LN_NEW2, LN_TARGET);

	/* errors are identical as well */
	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, symlinkat(LN_TARGET, AT_FDCWD, LN_FILE_NAME));
	TEST_ASSERT_EQUAL_INT(EEXIST, errno);
	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, symlink(LN_TARGET, LN_FILE_NAME));
	TEST_ASSERT_EQUAL_INT(EEXIST, errno);
}


/* Link uid is the effective uid; gid is the parent directory gid or the effective gid. */
TEST(fileops_symlinkat, symlinkat_ownership)
{
	struct stat st;
	struct stat dirSt;

	TEST_ASSERT_EQUAL_INT(0, symlinkat(LN_TARGET, test_common.at.dirFd, LN_NEW_NAME));

	TEST_ASSERT_EQUAL_INT(0, lstat(LN_NEW, &st));
	TEST_ASSERT_EQUAL_INT(0, stat(LN_DIR, &dirSt));
	TEST_ASSERT_EQUAL_UINT(geteuid(), st.st_uid);
	TEST_ASSERT_TRUE((st.st_gid == dirSt.st_gid) || (st.st_gid == getegid()));
}


/* The link's atime/mtime/ctime and the parent directory's mtime/ctime are marked for update. */
TEST(fileops_symlinkat, symlinkat_marks_timestamps)
{
	const struct timespec oldTimes[2] = { { LN_OLD_TIME, 0 }, { LN_OLD_TIME, 0 } };
	struct stat st;
	time_t before;

	TEST_ASSERT_EQUAL_INT(0, utimensat(AT_FDCWD, LN_DIR, oldTimes, 0));
	before = time(NULL);
	TEST_ASSERT_NOT_EQUAL_INT64((int64_t)-1, (int64_t)before);

	TEST_ASSERT_EQUAL_INT(0, symlinkat(LN_TARGET, test_common.at.dirFd, LN_NEW_NAME));

	TEST_ASSERT_EQUAL_INT(0, lstat(LN_NEW, &st));
	TEST_ASSERT_GREATER_OR_EQUAL_INT64((int64_t)before, (int64_t)st.st_atime);
	TEST_ASSERT_GREATER_OR_EQUAL_INT64((int64_t)before, (int64_t)st.st_mtime);
	TEST_ASSERT_GREATER_OR_EQUAL_INT64((int64_t)before, (int64_t)st.st_ctime);

	TEST_ASSERT_EQUAL_INT(0, stat(LN_DIR, &st));
	TEST_ASSERT_GREATER_OR_EQUAL_INT64((int64_t)before, (int64_t)st.st_mtime);
	TEST_ASSERT_GREATER_OR_EQUAL_INT64((int64_t)before, (int64_t)st.st_ctime);
}


/* EEXIST: path2 names an existing regular file, directory or symbolic link; it is unaffected. */
TEST(fileops_symlinkat, symlinkat_eexist)
{
	struct stat st;

	test_symlinkatFails(test_common.at.dirFd, LN_FILE_NAME, EEXIST);
	check_file_contents(LN_DATA, LN_FILE);

	test_symlinkatFails(test_common.at.dirFd, LN_SUBDIR_NAME, EEXIST);
	TEST_ASSERT_EQUAL_INT(0, lstat(LN_SUBDIR, &st));
	TEST_ASSERT_TRUE(S_ISDIR(st.st_mode));

	test_symlinkatFails(test_common.at.dirFd, LN_LINK_NAME, EEXIST);
	test_checkLink(LN_LINK, LN_FILE_NAME);

	/* a symbolic link whose resolution loops is still an existing name */
	test_symlinkatFails(test_common.at.dirFd, LN_LOOPA_NAME, EEXIST);
	test_checkLink(LN_LOOPA, LN_LOOPB_NAME);
}


/* ENOENT: a component of the path prefix does not exist, or path2 is empty. */
TEST(fileops_symlinkat, symlinkat_enoent)
{
	test_symlinkatFails(test_common.at.dirFd, LN_MISSING_NAME "/" LN_NEW_NAME, ENOENT);
	test_symlinkatFails(test_common.at.dirFd, "", ENOENT);
	test_symlinkatFails(AT_FDCWD, "", ENOENT);
}


/* Trailing slash: ENOENT or ENOTDIR; ENOENT shall not occur if the name without slashes exists. */
TEST(fileops_symlinkat, symlinkat_trailing_slash)
{
	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, symlinkat(LN_TARGET, test_common.at.dirFd, LN_NEW_NAME "/"));
	TEST_ASSERT_TRUE((errno == ENOENT) || (errno == ENOTDIR));
	TEST_ASSERT_FALSE(test_exists(LN_NEW));

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, symlinkat(LN_TARGET, test_common.at.dirFd, LN_FILE_NAME "/"));
	TEST_ASSERT_NOT_EQUAL_INT(ENOENT, errno);
	check_file_contents(LN_DATA, LN_FILE);

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, symlinkat(LN_TARGET, test_common.at.dirFd, LN_SUBDIR_NAME "/"));
	TEST_ASSERT_NOT_EQUAL_INT(ENOENT, errno);
}


/* ENOTDIR: a component of the path prefix is a regular file. */
TEST(fileops_symlinkat, symlinkat_enotdir_prefix)
{
	test_symlinkatFails(test_common.at.dirFd, LN_FILE_NAME "/" LN_NEW_NAME, ENOTDIR);
	test_symlinkatFails(AT_FDCWD, LN_FILE "/" LN_NEW_NAME, ENOTDIR);
}


/* ELOOP: a loop exists in symbolic links encountered while resolving path2. */
TEST(fileops_symlinkat, symlinkat_eloop)
{
	test_symlinkatFails(test_common.at.dirFd, LN_LOOPA_NAME "/" LN_NEW_NAME, ELOOP);
}


/* ENAMETOOLONG: a component of path2 is longer than NAME_MAX. */
TEST(fileops_symlinkat, symlinkat_enametoolong_path2)
{
	test_symlinkatFails(test_common.at.dirFd, test_common.longName, ENAMETOOLONG);
}


/* ENAMETOOLONG: path1 is longer than SYMLINK_MAX. */
TEST(fileops_symlinkat, symlinkat_enametoolong_path1)
{
	long symlinkMax;

	errno = 0;
	symlinkMax = pathconf(LN_DIR, _PC_SYMLINK_MAX);
	if (symlinkMax < 0) {
		TEST_IGNORE_MESSAGE("SYMLINK_MAX is indeterminate for this file system");
	}
	if (symlinkMax >= LN_SYMLINK_MAX_CAP) {
		TEST_IGNORE_MESSAGE("SYMLINK_MAX too large to build path1 exceeding it");
	}

	test_common.heapBuf = malloc((size_t)symlinkMax + 2U);
	TEST_ASSERT_NOT_NULL(test_common.heapBuf);
	memset(test_common.heapBuf, 't', (size_t)symlinkMax + 1U);
	test_common.heapBuf[symlinkMax + 1] = '\0';

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, symlinkat(test_common.heapBuf, test_common.at.dirFd, LN_NEW_NAME));
	TEST_ASSERT_EQUAL_INT(ENAMETOOLONG, errno);
	TEST_ASSERT_FALSE(test_exists(LN_NEW));
}


/* EACCES: write permission is denied in the directory where the link would be created. */
TEST(fileops_symlinkat, symlinkat_eacces_no_write)
{
	if (geteuid() == 0) {
		TEST_IGNORE_MESSAGE("running as root: permission checks are bypassed");
	}

	TEST_ASSERT_EQUAL_INT(0, chmod(LN_SUBDIR, LN_RO_DIR_MODE));
	test_symlinkatFails(test_common.at.dirFd, LN_SUB_NEW_REL, EACCES);
	TEST_ASSERT_EQUAL_INT(0, chmod(LN_SUBDIR, LN_DIR_MODE));
	TEST_ASSERT_FALSE(test_exists(LN_SUB_NEW));
}


/* EACCES: search permission is denied for a component of the path prefix of path2. */
TEST(fileops_symlinkat, symlinkat_eacces_no_search)
{
	if (geteuid() == 0) {
		TEST_IGNORE_MESSAGE("running as root: permission checks are bypassed");
	}

	TEST_ASSERT_EQUAL_INT(0, chmod(LN_SUBDIR, LN_NOX_DIR_MODE));
	test_symlinkatFails(test_common.at.dirFd, LN_SUB_NEW_REL, EACCES);
	TEST_ASSERT_EQUAL_INT(0, chmod(LN_SUBDIR, LN_DIR_MODE));
	TEST_ASSERT_FALSE(test_exists(LN_SUB_NEW));
}


/* EACCES: fd not opened with O_SEARCH and the directory's current permissions deny search. */
TEST(fileops_symlinkat, symlinkat_eacces_fd_no_search)
{
	if (geteuid() == 0) {
		TEST_IGNORE_MESSAGE("running as root: permission checks are bypassed");
	}

	TEST_ASSERT_EQUAL_INT(0, chmod(LN_DIR, LN_NOX_DIR_MODE));
	test_symlinkatFails(test_common.at.dirFd, LN_NEW_NAME, EACCES);
	TEST_ASSERT_EQUAL_INT(0, chmod(LN_DIR, LN_DIR_MODE));
	TEST_ASSERT_FALSE(test_exists(LN_NEW));
}


/* fd opened with O_SEARCH: no search-permission check against the directory's current permissions. */
TEST(fileops_symlinkat, symlinkat_o_search_no_check)
{
#ifndef O_SEARCH
	TEST_IGNORE_MESSAGE("O_SEARCH not defined by this libc");
#else
	int ret;

	if (geteuid() == 0) {
		TEST_IGNORE_MESSAGE("running as root: permission checks are bypassed");
	}

	test_common.fd = open(LN_DIR, O_SEARCH | O_DIRECTORY);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_common.fd);
	TEST_ASSERT_EQUAL_INT(0, chmod(LN_DIR, LN_NOX_DIR_MODE));

	ret = symlinkat(LN_TARGET, test_common.fd, LN_NEW_NAME);
	TEST_ASSERT_EQUAL_INT(0, chmod(LN_DIR, LN_DIR_MODE));
	TEST_ASSERT_EQUAL_INT(0, ret);
	test_checkLink(LN_NEW, LN_TARGET);
#endif
}


/* EBADF: relative path2 and fd is neither AT_FDCWD nor a valid descriptor. */
TEST(fileops_symlinkat, symlinkat_ebadf)
{
	int fd;

	test_symlinkatFails(-1, LN_NEW_NAME, EBADF);

	fd = test_atClosedFd(LN_DIR);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);
	test_symlinkatFails(fd, LN_NEW_NAME, EBADF);

	TEST_ASSERT_FALSE(test_exists(LN_NEW));
}


/* ENOTDIR: relative path2 and fd refers to a non-directory file. */
TEST(fileops_symlinkat, symlinkat_enotdir_fd_not_dir)
{
	test_common.fd = open(LN_FILE, O_RDONLY);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_common.fd);

	test_symlinkatFails(test_common.fd, LN_NEW_NAME, ENOTDIR);
}


TEST_GROUP_RUNNER(fileops_symlinkat)
{
	RUN_TEST_CASE(fileops_symlinkat, symlinkat_relative_to_fd);
	RUN_TEST_CASE(fileops_symlinkat, symlinkat_existing_target_resolves);
	RUN_TEST_CASE(fileops_symlinkat, symlinkat_path1_not_validated);
	RUN_TEST_CASE(fileops_symlinkat, symlinkat_absolute_ignores_fd);
	RUN_TEST_CASE(fileops_symlinkat, symlinkat_at_fdcwd);
	RUN_TEST_CASE(fileops_symlinkat, symlinkat_ownership);
	RUN_TEST_CASE(fileops_symlinkat, symlinkat_marks_timestamps);
	RUN_TEST_CASE(fileops_symlinkat, symlinkat_eexist);
	RUN_TEST_CASE(fileops_symlinkat, symlinkat_enoent);
	RUN_TEST_CASE(fileops_symlinkat, symlinkat_trailing_slash);
	RUN_TEST_CASE(fileops_symlinkat, symlinkat_enotdir_prefix);
	RUN_TEST_CASE(fileops_symlinkat, symlinkat_eloop);
	RUN_TEST_CASE(fileops_symlinkat, symlinkat_enametoolong_path2);
	RUN_TEST_CASE(fileops_symlinkat, symlinkat_enametoolong_path1);
	RUN_TEST_CASE(fileops_symlinkat, symlinkat_eacces_no_write);
	RUN_TEST_CASE(fileops_symlinkat, symlinkat_eacces_no_search);
	RUN_TEST_CASE(fileops_symlinkat, symlinkat_eacces_fd_no_search);
	RUN_TEST_CASE(fileops_symlinkat, symlinkat_o_search_no_check);
	RUN_TEST_CASE(fileops_symlinkat, symlinkat_ebadf);
	RUN_TEST_CASE(fileops_symlinkat, symlinkat_enotdir_fd_not_dir);
}


/* ========================================================================= */
/* unlinkat */
/* ========================================================================= */

TEST_GROUP(fileops_unlinkat);


TEST_SETUP(fileops_unlinkat)
{
	test_createTree();
}


TEST_TEAR_DOWN(fileops_unlinkat)
{
	test_destroyTree();
}


/* Relative path is resolved against fd (not cwd); the link is removed; returns 0. */
TEST(fileops_unlinkat, unlinkat_removes_file_relative_to_fd)
{
	create_file(LN_SUB_FILE, NULL);
	TEST_ASSERT_EQUAL_INT(0, test_atChdir(&test_common.at, LN_SUBDIR));

	TEST_ASSERT_EQUAL_INT(0, unlinkat(test_common.at.dirFd, LN_FILE_NAME, 0));

	TEST_ASSERT_FALSE(test_exists(LN_FILE));
	TEST_ASSERT_TRUE(test_exists(LN_SUB_FILE));
}


/* A symbolic link is removed itself; the file or directory it names is not affected. */
TEST(fileops_unlinkat, unlinkat_symlink_not_followed)
{
	struct stat st;

	TEST_ASSERT_EQUAL_INT(0, unlinkat(test_common.at.dirFd, LN_LINK_NAME, 0));
	TEST_ASSERT_FALSE(test_exists(LN_LINK));
	check_file_contents(LN_DATA, LN_FILE);

	TEST_ASSERT_EQUAL_INT(0, unlinkat(test_common.at.dirFd, LN_DIRLINK_NAME, 0));
	TEST_ASSERT_FALSE(test_exists(LN_DIRLINK));
	TEST_ASSERT_EQUAL_INT(0, lstat(LN_SUBDIR, &st));
	TEST_ASSERT_TRUE(S_ISDIR(st.st_mode));

	/* a dangling (looping) symlink can be removed too */
	TEST_ASSERT_EQUAL_INT(0, unlinkat(test_common.at.dirFd, LN_LOOPA_NAME, 0));
	TEST_ASSERT_FALSE(test_exists(LN_LOOPA));
	test_checkLink(LN_LOOPB, LN_LOOPA_NAME);
}


/* The link count is decremented and, if non-zero, the file's ctime is marked for update. */
TEST(fileops_unlinkat, unlinkat_decrements_link_count)
{
	struct stat before;
	struct stat after;

	TEST_ASSERT_EQUAL_INT(0, link(LN_FILE, LN_HARD));
	TEST_ASSERT_EQUAL_INT(0, stat(LN_FILE, &before));
	TEST_ASSERT_EQUAL_UINT(2U, (unsigned int)before.st_nlink);

	test_waitNextSecond(before.st_ctime);
	TEST_ASSERT_EQUAL_INT(0, unlinkat(test_common.at.dirFd, LN_HARD_NAME, 0));

	TEST_ASSERT_FALSE(test_exists(LN_HARD));
	TEST_ASSERT_EQUAL_INT(0, stat(LN_FILE, &after));
	TEST_ASSERT_EQUAL_UINT(1U, (unsigned int)after.st_nlink);
	TEST_ASSERT_GREATER_THAN_INT64((int64_t)before.st_ctime, (int64_t)after.st_ctime);
	check_file_contents(LN_DATA, LN_FILE);
}


/* Removing the last link of an open file: the name is gone, contents stay accessible until close. */
TEST(fileops_unlinkat, unlinkat_open_file_stays_accessible)
{
	struct stat st;
	ssize_t ret;

	test_common.fd = open(LN_FILE, O_RDONLY);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_common.fd);

	TEST_ASSERT_EQUAL_INT(0, unlinkat(test_common.at.dirFd, LN_FILE_NAME, 0));

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, open(LN_FILE, O_RDONLY));
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);

	TEST_ASSERT_EQUAL_INT(0, fstat(test_common.fd, &st));
	TEST_ASSERT_EQUAL_UINT(0U, (unsigned int)st.st_nlink);

	ret = pread(test_common.fd, test_common.buf, sizeof(test_common.buf), 0);
	TEST_ASSERT_EQUAL_INT((ssize_t)strlen(LN_DATA), ret);
	TEST_ASSERT_EQUAL_MEMORY(LN_DATA, test_common.buf, strlen(LN_DATA));
}


/* The parent directory's mtime and ctime are marked for update. */
TEST(fileops_unlinkat, unlinkat_marks_parent_timestamps)
{
	const struct timespec oldTimes[2] = { { LN_OLD_TIME, 0 }, { LN_OLD_TIME, 0 } };
	struct stat st;
	time_t before;

	TEST_ASSERT_EQUAL_INT(0, utimensat(AT_FDCWD, LN_DIR, oldTimes, 0));
	before = time(NULL);
	TEST_ASSERT_NOT_EQUAL_INT64((int64_t)-1, (int64_t)before);

	TEST_ASSERT_EQUAL_INT(0, unlinkat(test_common.at.dirFd, LN_FILE_NAME, 0));

	TEST_ASSERT_EQUAL_INT(0, stat(LN_DIR, &st));
	TEST_ASSERT_GREATER_OR_EQUAL_INT64((int64_t)before, (int64_t)st.st_mtime);
	TEST_ASSERT_GREATER_OR_EQUAL_INT64((int64_t)before, (int64_t)st.st_ctime);
}


/* AT_REMOVEDIR removes an empty directory, like rmdir(). */
TEST(fileops_unlinkat, unlinkat_removedir_empty)
{
	TEST_ASSERT_EQUAL_INT(0, unlinkat(test_common.at.dirFd, LN_SUBDIR_NAME, AT_REMOVEDIR));
	TEST_ASSERT_FALSE(test_exists(LN_SUBDIR));
}


/* An absolute path is used regardless of fd (even an invalid or non-directory fd). */
TEST(fileops_unlinkat, unlinkat_absolute_ignores_fd)
{
	TEST_ASSERT_EQUAL_INT(0, unlinkat(-1, LN_LINK, 0));
	TEST_ASSERT_FALSE(test_exists(LN_LINK));

	test_common.fd = open(LN_FILE, O_RDONLY);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_common.fd);
	TEST_ASSERT_EQUAL_INT(0, unlinkat(test_common.fd, LN_SUBDIR, AT_REMOVEDIR));
	TEST_ASSERT_FALSE(test_exists(LN_SUBDIR));
}


/* AT_FDCWD: relative path uses cwd; identical to unlink() or rmdir() depending on AT_REMOVEDIR. */
TEST(fileops_unlinkat, unlinkat_at_fdcwd)
{
	TEST_ASSERT_EQUAL_INT(0, test_atChdir(&test_common.at, LN_DIR));

	TEST_ASSERT_EQUAL_INT(0, unlinkat(AT_FDCWD, LN_FILE_NAME, 0));
	TEST_ASSERT_FALSE(test_exists(LN_FILE));

	TEST_ASSERT_EQUAL_INT(0, unlinkat(AT_FDCWD, LN_SUBDIR_NAME, AT_REMOVEDIR));
	TEST_ASSERT_FALSE(test_exists(LN_SUBDIR));

	/* errors are identical as well */
	test_unlinkatFails(AT_FDCWD, LN_MISSING_NAME, 0, ENOENT);
	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, unlink(LN_MISSING_NAME));
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);

	test_unlinkatFails(AT_FDCWD, LN_LINK_NAME, AT_REMOVEDIR, ENOTDIR);
	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, rmdir(LN_LINK_NAME));
	TEST_ASSERT_EQUAL_INT(ENOTDIR, errno);
}


/* EPERM: path names a directory and AT_REMOVEDIR is not set; the directory is not removed. */
TEST(fileops_unlinkat, unlinkat_directory_eperm)
{
	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, unlinkat(test_common.at.dirFd, LN_SUBDIR_NAME, 0));
	TEST_ASSERT_TRUE(test_exists(LN_SUBDIR));
#ifndef __phoenix__
	TEST_IGNORE_MESSAGE("host-pc bug: Linux unlink() on a directory fails with EISDIR instead of EPERM");
#endif
	TEST_ASSERT_EQUAL_INT(EPERM, errno);
}


/* EEXIST or ENOTEMPTY: AT_REMOVEDIR on a non-empty directory; nothing is removed. */
TEST(fileops_unlinkat, unlinkat_removedir_not_empty)
{
	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, unlinkat(test_common.at.dirFd, LN_FULL_NAME, AT_REMOVEDIR));
	TEST_ASSERT_TRUE((errno == EEXIST) || (errno == ENOTEMPTY));
	TEST_ASSERT_TRUE(test_exists(LN_INNER));
}


/* ENOTDIR: AT_REMOVEDIR and path does not name a directory (regular file or symlink to a directory). */
TEST(fileops_unlinkat, unlinkat_removedir_enotdir)
{
	test_unlinkatFails(test_common.at.dirFd, LN_FILE_NAME, AT_REMOVEDIR, ENOTDIR);
	check_file_contents(LN_DATA, LN_FILE);

	test_unlinkatFails(test_common.at.dirFd, LN_DIRLINK_NAME, AT_REMOVEDIR, ENOTDIR);
	test_checkLink(LN_DIRLINK, LN_SUBDIR_NAME);
	TEST_ASSERT_TRUE(test_exists(LN_SUBDIR));
}


/* ENOENT: path does not exist, a prefix component does not exist, or path is empty. */
TEST(fileops_unlinkat, unlinkat_enoent)
{
	test_unlinkatFails(test_common.at.dirFd, LN_MISSING_NAME, 0, ENOENT);
	test_unlinkatFails(test_common.at.dirFd, LN_MISSING_NAME "/" LN_FILE_NAME, 0, ENOENT);
	test_unlinkatFails(test_common.at.dirFd, "", 0, ENOENT);
	test_unlinkatFails(test_common.at.dirFd, "", AT_REMOVEDIR, ENOENT);
}


/* ENOTDIR: a prefix component is a regular file, or path ends with '/' and names a regular file. */
TEST(fileops_unlinkat, unlinkat_enotdir_path)
{
	test_unlinkatFails(test_common.at.dirFd, LN_FILE_NAME "/" LN_NEW_NAME, 0, ENOTDIR);
	test_unlinkatFails(test_common.at.dirFd, LN_FILE_NAME "/", 0, ENOTDIR);
	check_file_contents(LN_DATA, LN_FILE);
}


/* ELOOP: a loop exists in symbolic links encountered during path resolution. */
TEST(fileops_unlinkat, unlinkat_eloop)
{
	test_unlinkatFails(test_common.at.dirFd, LN_LOOPA_NAME "/" LN_FILE_NAME, 0, ELOOP);
}


/* ENAMETOOLONG: a path component is longer than NAME_MAX. */
TEST(fileops_unlinkat, unlinkat_enametoolong)
{
	test_unlinkatFails(test_common.at.dirFd, test_common.longName, 0, ENAMETOOLONG);
	test_unlinkatFails(test_common.at.dirFd, test_common.longName, AT_REMOVEDIR, ENAMETOOLONG);
}


/* EACCES: write permission is denied on the directory containing the entry; nothing is removed. */
TEST(fileops_unlinkat, unlinkat_eacces_no_write)
{
	if (geteuid() == 0) {
		TEST_IGNORE_MESSAGE("running as root: permission checks are bypassed");
	}

	create_file(LN_SUB_FILE, NULL);
	TEST_ASSERT_EQUAL_INT(0, chmod(LN_SUBDIR, LN_RO_DIR_MODE));
	test_unlinkatFails(test_common.at.dirFd, LN_SUB_FILE_REL, 0, EACCES);
	TEST_ASSERT_EQUAL_INT(0, chmod(LN_SUBDIR, LN_DIR_MODE));
	TEST_ASSERT_TRUE(test_exists(LN_SUB_FILE));
}


/* EACCES: search permission is denied for a component of the path prefix; nothing is removed. */
TEST(fileops_unlinkat, unlinkat_eacces_no_search)
{
	if (geteuid() == 0) {
		TEST_IGNORE_MESSAGE("running as root: permission checks are bypassed");
	}

	create_file(LN_SUB_FILE, NULL);
	TEST_ASSERT_EQUAL_INT(0, chmod(LN_SUBDIR, LN_NOX_DIR_MODE));
	test_unlinkatFails(test_common.at.dirFd, LN_SUB_FILE_REL, 0, EACCES);
	TEST_ASSERT_EQUAL_INT(0, chmod(LN_SUBDIR, LN_DIR_MODE));
	TEST_ASSERT_TRUE(test_exists(LN_SUB_FILE));
}


/* EACCES: fd not opened with O_SEARCH and the directory's current permissions deny search. */
TEST(fileops_unlinkat, unlinkat_eacces_fd_no_search)
{
	if (geteuid() == 0) {
		TEST_IGNORE_MESSAGE("running as root: permission checks are bypassed");
	}

	TEST_ASSERT_EQUAL_INT(0, chmod(LN_DIR, LN_NOX_DIR_MODE));
	test_unlinkatFails(test_common.at.dirFd, LN_FILE_NAME, 0, EACCES);
	TEST_ASSERT_EQUAL_INT(0, chmod(LN_DIR, LN_DIR_MODE));
	TEST_ASSERT_TRUE(test_exists(LN_FILE));
}


/* fd opened with O_SEARCH: no search-permission check against the directory's current permissions. */
TEST(fileops_unlinkat, unlinkat_o_search_no_check)
{
#ifndef O_SEARCH
	TEST_IGNORE_MESSAGE("O_SEARCH not defined by this libc");
#else
	int ret;

	if (geteuid() == 0) {
		TEST_IGNORE_MESSAGE("running as root: permission checks are bypassed");
	}

	test_common.fd = open(LN_DIR, O_SEARCH | O_DIRECTORY);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_common.fd);
	TEST_ASSERT_EQUAL_INT(0, chmod(LN_DIR, LN_NOX_DIR_MODE));

	ret = unlinkat(test_common.fd, LN_FILE_NAME, 0);
	TEST_ASSERT_EQUAL_INT(0, chmod(LN_DIR, LN_DIR_MODE));
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_FALSE(test_exists(LN_FILE));
#endif
}


/* EBADF: relative path and fd is neither AT_FDCWD nor a valid descriptor. */
TEST(fileops_unlinkat, unlinkat_ebadf)
{
	int fd;

	test_unlinkatFails(-1, LN_FILE_NAME, 0, EBADF);

	fd = test_atClosedFd(LN_DIR);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);
	test_unlinkatFails(fd, LN_FILE_NAME, 0, EBADF);
	test_unlinkatFails(fd, LN_SUBDIR_NAME, AT_REMOVEDIR, EBADF);

	TEST_ASSERT_TRUE(test_exists(LN_FILE));
	TEST_ASSERT_TRUE(test_exists(LN_SUBDIR));
}


/* ENOTDIR: relative path and fd refers to a non-directory file. */
TEST(fileops_unlinkat, unlinkat_enotdir_fd_not_dir)
{
	test_common.fd = open(LN_FILE, O_RDONLY);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_common.fd);

	test_unlinkatFails(test_common.fd, LN_FILE_NAME, 0, ENOTDIR);
	test_unlinkatFails(test_common.fd, LN_SUBDIR_NAME, AT_REMOVEDIR, ENOTDIR);
	TEST_ASSERT_TRUE(test_exists(LN_FILE));
}


TEST_GROUP_RUNNER(fileops_unlinkat)
{
	RUN_TEST_CASE(fileops_unlinkat, unlinkat_removes_file_relative_to_fd);
	RUN_TEST_CASE(fileops_unlinkat, unlinkat_symlink_not_followed);
	RUN_TEST_CASE(fileops_unlinkat, unlinkat_decrements_link_count);
	RUN_TEST_CASE(fileops_unlinkat, unlinkat_open_file_stays_accessible);
	RUN_TEST_CASE(fileops_unlinkat, unlinkat_marks_parent_timestamps);
	RUN_TEST_CASE(fileops_unlinkat, unlinkat_removedir_empty);
	RUN_TEST_CASE(fileops_unlinkat, unlinkat_absolute_ignores_fd);
	RUN_TEST_CASE(fileops_unlinkat, unlinkat_at_fdcwd);
	RUN_TEST_CASE(fileops_unlinkat, unlinkat_directory_eperm);
	RUN_TEST_CASE(fileops_unlinkat, unlinkat_removedir_not_empty);
	RUN_TEST_CASE(fileops_unlinkat, unlinkat_removedir_enotdir);
	RUN_TEST_CASE(fileops_unlinkat, unlinkat_enoent);
	RUN_TEST_CASE(fileops_unlinkat, unlinkat_enotdir_path);
	RUN_TEST_CASE(fileops_unlinkat, unlinkat_eloop);
	RUN_TEST_CASE(fileops_unlinkat, unlinkat_enametoolong);
	RUN_TEST_CASE(fileops_unlinkat, unlinkat_eacces_no_write);
	RUN_TEST_CASE(fileops_unlinkat, unlinkat_eacces_no_search);
	RUN_TEST_CASE(fileops_unlinkat, unlinkat_eacces_fd_no_search);
	RUN_TEST_CASE(fileops_unlinkat, unlinkat_o_search_no_check);
	RUN_TEST_CASE(fileops_unlinkat, unlinkat_ebadf);
	RUN_TEST_CASE(fileops_unlinkat, unlinkat_enotdir_fd_not_dir);
}
