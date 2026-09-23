/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <unistd.h>
 *    - <fcntl.h>
 * TESTED:
 *    - readlink()
 *    - readlinkat()
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
#include <stdint.h>
#include <stdio.h>

#include "common.h"
#include "fileops_at.h"
#include "unity_fixture.h"

#define RL_DIR          "/tmp/test_fileops_readlink"
#define RL_FILE_NAME    "file"
#define RL_SUBDIR_NAME  "sub"
#define RL_LINK_NAME    "link"
#define RL_DANGLE_NAME  "dangle"
#define RL_DIRLINK_NAME "dirlink"
#define RL_LONG_NAME    "long"
#define RL_LOOPA_NAME   "loopa"
#define RL_LOOPB_NAME   "loopb"
#define RL_MISSING_NAME "missing"

#define RL_FILE    RL_DIR "/" RL_FILE_NAME
#define RL_SUBDIR  RL_DIR "/" RL_SUBDIR_NAME
#define RL_LINK    RL_DIR "/" RL_LINK_NAME
#define RL_DANGLE  RL_DIR "/" RL_DANGLE_NAME
#define RL_DIRLINK RL_DIR "/" RL_DIRLINK_NAME
#define RL_LONG    RL_DIR "/" RL_LONG_NAME
#define RL_LOOPA   RL_DIR "/" RL_LOOPA_NAME
#define RL_LOOPB   RL_DIR "/" RL_LOOPB_NAME
#define RL_MISSING RL_DIR "/" RL_MISSING_NAME

/* symlink contents (RL_LINK -> RL_FILE_NAME, RL_DIRLINK -> RL_SUBDIR_NAME) */
#define RL_DANGLE_TARGET "no_such_target"

/* long symlink content length (well below any SYMLINK_MAX, still > typical short buffers) */
#define RL_LONG_LEN 255
/* a path component longer than NAME_MAX */
#define RL_NAME_TOO_LONG_LEN (NAME_MAX + 2)

#define RL_BUF_SIZE    512
#define RL_SENTINEL    0x5a
#define RL_SHORT_BUF   3
#define RL_DIR_MODE    0755
#define RL_OLD_TIME    1000000


static struct {
	test_atCtx_t at;
	char buf[RL_BUF_SIZE];
	char longContent[RL_LONG_LEN + 1];
} test_common;


static void test_removeTree(void)
{
	chmod(RL_SUBDIR, RL_DIR_MODE);
	chmod(RL_DIR, RL_DIR_MODE);

	unlink(RL_FILE);
	unlink(RL_LINK);
	unlink(RL_DANGLE);
	unlink(RL_DIRLINK);
	unlink(RL_LONG);
	unlink(RL_LOOPA);
	unlink(RL_LOOPB);
	unlink(RL_SUBDIR "/" RL_LINK_NAME);
	rmdir(RL_SUBDIR);
	rmdir(RL_DIR);
}


static void test_createTree(void)
{
	test_atInit(&test_common.at);

	memset(test_common.longContent, 'l', RL_LONG_LEN);
	test_common.longContent[RL_LONG_LEN] = '\0';

	test_removeTree();

	TEST_ASSERT_EQUAL_INT(0, mkdir(RL_DIR, RL_DIR_MODE));
	TEST_ASSERT_EQUAL_INT(0, mkdir(RL_SUBDIR, RL_DIR_MODE));
	create_file(RL_FILE, NULL);
	TEST_ASSERT_EQUAL_INT(0, symlink(RL_FILE_NAME, RL_LINK));
	TEST_ASSERT_EQUAL_INT(0, symlink(RL_DANGLE_TARGET, RL_DANGLE));
	TEST_ASSERT_EQUAL_INT(0, symlink(RL_SUBDIR_NAME, RL_DIRLINK));
	TEST_ASSERT_EQUAL_INT(0, symlink(test_common.longContent, RL_LONG));
	TEST_ASSERT_EQUAL_INT(0, symlink(RL_LOOPB_NAME, RL_LOOPA));
	TEST_ASSERT_EQUAL_INT(0, symlink(RL_LOOPA_NAME, RL_LOOPB));

	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_atOpenDir(&test_common.at, RL_DIR));
}


static void test_destroyTree(void)
{
	test_atRelease(&test_common.at);
	test_removeTree();
}


static void test_resetBuf(void)
{
	memset(test_common.buf, RL_SENTINEL, sizeof(test_common.buf));
}


static int test_bufUnchanged(size_t from)
{
	size_t i;

	for (i = from; i < sizeof(test_common.buf); i++) {
		if ((unsigned char)test_common.buf[i] != RL_SENTINEL) {
			return 0;
		}
	}
	return 1;
}


/*
 * Both readlink(absPath) and readlinkat(dirFd, relPath) shall fail with
 * expErrno, return -1 and leave the buffer unchanged.
 */
static void test_checkError(const char *absPath, const char *relPath, int expErrno)
{
	ssize_t ret;

	test_resetBuf();
	errno = 0;
	ret = readlink(absPath, test_common.buf, sizeof(test_common.buf));
	TEST_ASSERT_EQUAL_INT_MESSAGE(-1, ret, "readlink");
	TEST_ASSERT_EQUAL_INT_MESSAGE(expErrno, errno, "readlink");
	TEST_ASSERT_TRUE_MESSAGE(test_bufUnchanged(0), "readlink modified buf");

	test_resetBuf();
	errno = 0;
	ret = readlinkat(test_common.at.dirFd, relPath, test_common.buf, sizeof(test_common.buf));
	TEST_ASSERT_EQUAL_INT_MESSAGE(-1, ret, "readlinkat");
	TEST_ASSERT_EQUAL_INT_MESSAGE(expErrno, errno, "readlinkat");
	TEST_ASSERT_TRUE_MESSAGE(test_bufUnchanged(0), "readlinkat modified buf");
}


/* Both functions shall place the link content in buf and return its length. */
static void test_checkContent(const char *absPath, const char *relPath, const char *expected)
{
	const size_t len = strlen(expected);
	ssize_t ret;

	test_resetBuf();
	ret = readlink(absPath, test_common.buf, sizeof(test_common.buf));
	TEST_ASSERT_EQUAL_INT_MESSAGE((ssize_t)len, ret, "readlink");
	TEST_ASSERT_EQUAL_MEMORY(expected, test_common.buf, len);

	test_resetBuf();
	ret = readlinkat(test_common.at.dirFd, relPath, test_common.buf, sizeof(test_common.buf));
	TEST_ASSERT_EQUAL_INT_MESSAGE((ssize_t)len, ret, "readlinkat");
	TEST_ASSERT_EQUAL_MEMORY(expected, test_common.buf, len);
}


/* ========================================================================= */
/* Tests: readlink, readlinkat (common requirements) */
/* ========================================================================= */

TEST_GROUP(fileops_readlink);


TEST_SETUP(fileops_readlink)
{
	test_createTree();
}


TEST_TEAR_DOWN(fileops_readlink)
{
	test_destroyTree();
}


/* Content of a link to an existing regular file is returned; count excludes any NUL. */
TEST(fileops_readlink, readlink_content_file_target)
{
	test_checkContent(RL_LINK, RL_LINK_NAME, RL_FILE_NAME);
}


/* Content of a link to a directory is returned (the link itself is not followed). */
TEST(fileops_readlink, readlink_content_dir_target)
{
	test_checkContent(RL_DIRLINK, RL_DIRLINK_NAME, RL_SUBDIR_NAME);
}


/* Content of a dangling link is returned. */
TEST(fileops_readlink, readlink_content_dangling)
{
	test_checkContent(RL_DANGLE, RL_DANGLE_NAME, RL_DANGLE_TARGET);
}


/* Long link content is returned completely. */
TEST(fileops_readlink, readlink_content_long)
{
	test_checkContent(RL_LONG, RL_LONG_NAME, test_common.longContent);
}


/* A symbolic link in the path prefix is followed; the final link is read, not followed. */
TEST(fileops_readlink, readlink_prefix_through_symlink)
{
	TEST_ASSERT_EQUAL_INT(0, symlink(RL_DANGLE_TARGET, RL_SUBDIR "/" RL_LINK_NAME));

	test_checkContent(RL_DIRLINK "/" RL_LINK_NAME, RL_DIRLINK_NAME "/" RL_LINK_NAME, RL_DANGLE_TARGET);

	TEST_ASSERT_EQUAL_INT(0, unlink(RL_SUBDIR "/" RL_LINK_NAME));
}


/* If buf is too small, the first bufsize bytes are placed in buf; nothing is written past it. */
TEST(fileops_readlink, readlink_truncated_to_bufsize)
{
	ssize_t ret;

	test_resetBuf();
	ret = readlink(RL_DANGLE, test_common.buf, RL_SHORT_BUF);
	TEST_ASSERT_EQUAL_INT(RL_SHORT_BUF, ret);
	TEST_ASSERT_EQUAL_MEMORY(RL_DANGLE_TARGET, test_common.buf, RL_SHORT_BUF);
	TEST_ASSERT_TRUE(test_bufUnchanged(RL_SHORT_BUF));

	test_resetBuf();
	ret = readlinkat(test_common.at.dirFd, RL_DANGLE_NAME, test_common.buf, RL_SHORT_BUF);
	TEST_ASSERT_EQUAL_INT(RL_SHORT_BUF, ret);
	TEST_ASSERT_EQUAL_MEMORY(RL_DANGLE_TARGET, test_common.buf, RL_SHORT_BUF);
	TEST_ASSERT_TRUE(test_bufUnchanged(RL_SHORT_BUF));
}


/* bufsize of exactly the link length: whole content is returned, no NUL appended. */
TEST(fileops_readlink, readlink_exact_bufsize)
{
	const size_t len = strlen(RL_DANGLE_TARGET);
	ssize_t ret;

	test_resetBuf();
	ret = readlink(RL_DANGLE, test_common.buf, len);
	TEST_ASSERT_EQUAL_INT((ssize_t)len, ret);
	TEST_ASSERT_EQUAL_MEMORY(RL_DANGLE_TARGET, test_common.buf, len);
	TEST_ASSERT_TRUE(test_bufUnchanged(len));

	test_resetBuf();
	ret = readlinkat(test_common.at.dirFd, RL_DANGLE_NAME, test_common.buf, len);
	TEST_ASSERT_EQUAL_INT((ssize_t)len, ret);
	TEST_ASSERT_EQUAL_MEMORY(RL_DANGLE_TARGET, test_common.buf, len);
	TEST_ASSERT_TRUE(test_bufUnchanged(len));
}


/* bufsize of 1: exactly one byte is placed in buf. */
TEST(fileops_readlink, readlink_bufsize_one)
{
	ssize_t ret;

	test_resetBuf();
	ret = readlink(RL_LONG, test_common.buf, 1);
	TEST_ASSERT_EQUAL_INT(1, ret);
	TEST_ASSERT_EQUAL_CHAR(test_common.longContent[0], test_common.buf[0]);
	TEST_ASSERT_TRUE(test_bufUnchanged(1));

	test_resetBuf();
	ret = readlinkat(test_common.at.dirFd, RL_LONG_NAME, test_common.buf, 1);
	TEST_ASSERT_EQUAL_INT(1, ret);
	TEST_ASSERT_EQUAL_CHAR(test_common.longContent[0], test_common.buf[0]);
	TEST_ASSERT_TRUE(test_bufUnchanged(1));
}


/* Successful completion shall mark the last data access timestamp of the link for update. */
TEST(fileops_readlink, readlink_marks_atime)
{
	const struct timespec oldTimes[2] = { { RL_OLD_TIME, 0 }, { RL_OLD_TIME, 0 } };
	struct stat st;

	TEST_ASSERT_EQUAL_INT(0, utimensat(AT_FDCWD, RL_DANGLE, oldTimes, AT_SYMLINK_NOFOLLOW));
	TEST_ASSERT_EQUAL_INT(0, lstat(RL_DANGLE, &st));
	TEST_ASSERT_EQUAL_INT64(RL_OLD_TIME, (int64_t)st.st_atime);

	TEST_ASSERT_EQUAL_INT((ssize_t)strlen(RL_DANGLE_TARGET), readlink(RL_DANGLE, test_common.buf, sizeof(test_common.buf)));
	TEST_ASSERT_EQUAL_INT(0, lstat(RL_DANGLE, &st));
	TEST_ASSERT_GREATER_THAN_INT64(RL_OLD_TIME, (int64_t)st.st_atime);

	TEST_ASSERT_EQUAL_INT(0, utimensat(AT_FDCWD, RL_DANGLE, oldTimes, AT_SYMLINK_NOFOLLOW));
	TEST_ASSERT_EQUAL_INT((ssize_t)strlen(RL_DANGLE_TARGET), readlinkat(test_common.at.dirFd, RL_DANGLE_NAME, test_common.buf, sizeof(test_common.buf)));
	TEST_ASSERT_EQUAL_INT(0, lstat(RL_DANGLE, &st));
	TEST_ASSERT_GREATER_THAN_INT64(RL_OLD_TIME, (int64_t)st.st_atime);
}


/* EINVAL: path names a regular file (not a symbolic link). */
TEST(fileops_readlink, readlink_einval_regular_file)
{
	test_checkError(RL_FILE, RL_FILE_NAME, EINVAL);
}


/* EINVAL: path names a directory (not a symbolic link). */
TEST(fileops_readlink, readlink_einval_directory)
{
	test_checkError(RL_SUBDIR, RL_SUBDIR_NAME, EINVAL);
}


/* ENOENT: a component of path does not name an existing file. */
TEST(fileops_readlink, readlink_enoent_missing)
{
	test_checkError(RL_MISSING, RL_MISSING_NAME, ENOENT);
	test_checkError(RL_MISSING "/" RL_LINK_NAME, RL_MISSING_NAME "/" RL_LINK_NAME, ENOENT);
}


/* ENOENT: path is an empty string. */
TEST(fileops_readlink, readlink_enoent_empty_path)
{
	test_checkError("", "", ENOENT);
}


/* ENOTDIR: a component of the path prefix is a regular file. */
TEST(fileops_readlink, readlink_enotdir_prefix_file)
{
	test_checkError(RL_FILE "/" RL_LINK_NAME, RL_FILE_NAME "/" RL_LINK_NAME, ENOTDIR);
}


/* ENOTDIR: path ends with a trailing slash and names a regular file. */
TEST(fileops_readlink, readlink_enotdir_trailing_slash)
{
	test_checkError(RL_FILE "/", RL_FILE_NAME "/", ENOTDIR);
}


/* ELOOP: a loop exists in symbolic links encountered during path resolution. */
TEST(fileops_readlink, readlink_eloop)
{
	test_checkError(RL_LOOPA "/" RL_LINK_NAME, RL_LOOPA_NAME "/" RL_LINK_NAME, ELOOP);
}


/* ENAMETOOLONG: a path component is longer than NAME_MAX. */
TEST(fileops_readlink, readlink_enametoolong)
{
	static char longName[RL_NAME_TOO_LONG_LEN + 1];
	static char longPath[sizeof(RL_DIR "/") + RL_NAME_TOO_LONG_LEN];

	memset(longName, 'n', RL_NAME_TOO_LONG_LEN);
	longName[RL_NAME_TOO_LONG_LEN] = '\0';
	(void)snprintf(longPath, sizeof(longPath), "%s/%s", RL_DIR, longName);

	test_checkError(longPath, longName, ENAMETOOLONG);
}


/* EACCES: search permission is denied for a component of the path prefix. */
TEST(fileops_readlink, readlink_eacces_prefix)
{
	if (geteuid() == 0) {
		TEST_IGNORE_MESSAGE("running as root: search permission checks are bypassed");
	}

	TEST_ASSERT_EQUAL_INT(0, symlink(RL_DANGLE_TARGET, RL_SUBDIR "/" RL_LINK_NAME));
	TEST_ASSERT_EQUAL_INT(0, chmod(RL_SUBDIR, 0));

	test_checkError(RL_SUBDIR "/" RL_LINK_NAME, RL_SUBDIR_NAME "/" RL_LINK_NAME, EACCES);

	TEST_ASSERT_EQUAL_INT(0, chmod(RL_SUBDIR, RL_DIR_MODE));
	TEST_ASSERT_EQUAL_INT(0, unlink(RL_SUBDIR "/" RL_LINK_NAME));
}


TEST_GROUP_RUNNER(fileops_readlink)
{
	RUN_TEST_CASE(fileops_readlink, readlink_content_file_target);
	RUN_TEST_CASE(fileops_readlink, readlink_content_dir_target);
	RUN_TEST_CASE(fileops_readlink, readlink_content_dangling);
	RUN_TEST_CASE(fileops_readlink, readlink_content_long);
	RUN_TEST_CASE(fileops_readlink, readlink_prefix_through_symlink);
	RUN_TEST_CASE(fileops_readlink, readlink_truncated_to_bufsize);
	RUN_TEST_CASE(fileops_readlink, readlink_exact_bufsize);
	RUN_TEST_CASE(fileops_readlink, readlink_bufsize_one);
	RUN_TEST_CASE(fileops_readlink, readlink_marks_atime);
	RUN_TEST_CASE(fileops_readlink, readlink_einval_regular_file);
	RUN_TEST_CASE(fileops_readlink, readlink_einval_directory);
	RUN_TEST_CASE(fileops_readlink, readlink_enoent_missing);
	RUN_TEST_CASE(fileops_readlink, readlink_enoent_empty_path);
	RUN_TEST_CASE(fileops_readlink, readlink_enotdir_prefix_file);
	RUN_TEST_CASE(fileops_readlink, readlink_enotdir_trailing_slash);
	RUN_TEST_CASE(fileops_readlink, readlink_eloop);
	RUN_TEST_CASE(fileops_readlink, readlink_enametoolong);
	RUN_TEST_CASE(fileops_readlink, readlink_eacces_prefix);
}


/* ========================================================================= */
/* readlinkat (fd-specific requirements) */
/* ========================================================================= */

TEST_GROUP(fileops_readlinkat);


TEST_SETUP(fileops_readlinkat)
{
	test_createTree();
}


TEST_TEAR_DOWN(fileops_readlinkat)
{
	test_destroyTree();
}


/* A relative path is resolved against fd, not against the current working directory. */
TEST(fileops_readlinkat, readlinkat_relative_to_fd)
{
	ssize_t ret;

	TEST_ASSERT_EQUAL_INT(0, test_atChdir(&test_common.at, RL_SUBDIR));

	/* RL_DANGLE_NAME exists in RL_DIR but not in the cwd (RL_SUBDIR) */
	test_resetBuf();
	ret = readlinkat(test_common.at.dirFd, RL_DANGLE_NAME, test_common.buf, sizeof(test_common.buf));
	TEST_ASSERT_EQUAL_INT((ssize_t)strlen(RL_DANGLE_TARGET), ret);
	TEST_ASSERT_EQUAL_MEMORY(RL_DANGLE_TARGET, test_common.buf, strlen(RL_DANGLE_TARGET));
}


/* An absolute path is resolved independently of fd (even an invalid or non-directory fd). */
TEST(fileops_readlinkat, readlinkat_absolute_ignores_fd)
{
	ssize_t ret;
	int fileFd;

	test_resetBuf();
	ret = readlinkat(-1, RL_DANGLE, test_common.buf, sizeof(test_common.buf));
	TEST_ASSERT_EQUAL_INT((ssize_t)strlen(RL_DANGLE_TARGET), ret);
	TEST_ASSERT_EQUAL_MEMORY(RL_DANGLE_TARGET, test_common.buf, strlen(RL_DANGLE_TARGET));

	fileFd = open(RL_FILE, O_RDONLY);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fileFd);

	test_resetBuf();
	ret = readlinkat(fileFd, RL_DANGLE, test_common.buf, sizeof(test_common.buf));
	close(fileFd);
	TEST_ASSERT_EQUAL_INT((ssize_t)strlen(RL_DANGLE_TARGET), ret);
	TEST_ASSERT_EQUAL_MEMORY(RL_DANGLE_TARGET, test_common.buf, strlen(RL_DANGLE_TARGET));
}


/* AT_FDCWD: relative path is resolved against the current working directory, like readlink(). */
TEST(fileops_readlinkat, readlinkat_at_fdcwd)
{
	static char buf2[RL_BUF_SIZE];
	ssize_t ret;
	ssize_t ret2;

	TEST_ASSERT_EQUAL_INT(0, test_atChdir(&test_common.at, RL_DIR));

	test_resetBuf();
	ret = readlinkat(AT_FDCWD, RL_DANGLE_NAME, test_common.buf, sizeof(test_common.buf));
	ret2 = readlink(RL_DANGLE_NAME, buf2, sizeof(buf2));
	TEST_ASSERT_EQUAL_INT((ssize_t)strlen(RL_DANGLE_TARGET), ret);
	TEST_ASSERT_EQUAL_INT(ret2, ret);
	TEST_ASSERT_EQUAL_MEMORY(RL_DANGLE_TARGET, test_common.buf, strlen(RL_DANGLE_TARGET));
	TEST_ASSERT_EQUAL_MEMORY(buf2, test_common.buf, strlen(RL_DANGLE_TARGET));

	/* errors are identical as well */
	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, readlinkat(AT_FDCWD, RL_FILE_NAME, test_common.buf, sizeof(test_common.buf)));
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


/* EBADF: relative path and fd is neither AT_FDCWD nor a valid descriptor. */
TEST(fileops_readlinkat, readlinkat_ebadf)
{
	int fd;

	test_resetBuf();
	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, readlinkat(-1, RL_DANGLE_NAME, test_common.buf, sizeof(test_common.buf)));
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
	TEST_ASSERT_TRUE(test_bufUnchanged(0));

	/* a descriptor that was valid but has been closed */
	fd = test_atClosedFd(RL_DIR);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, readlinkat(fd, RL_DANGLE_NAME, test_common.buf, sizeof(test_common.buf)));
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
	TEST_ASSERT_TRUE(test_bufUnchanged(0));
}


/* ENOTDIR: relative path and fd refers to a non-directory file. */
TEST(fileops_readlinkat, readlinkat_enotdir_fd_not_dir)
{
	int fileFd;
	ssize_t ret;

	fileFd = open(RL_FILE, O_RDONLY);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fileFd);

	test_resetBuf();
	errno = 0;
	ret = readlinkat(fileFd, RL_DANGLE_NAME, test_common.buf, sizeof(test_common.buf));
	close(fileFd);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(ENOTDIR, errno);
	TEST_ASSERT_TRUE(test_bufUnchanged(0));
}


/* EACCES: fd not opened with O_SEARCH and the directory's current permissions deny search. */
TEST(fileops_readlinkat, readlinkat_eacces_fd_no_search)
{
	ssize_t ret;

	if (geteuid() == 0) {
		TEST_IGNORE_MESSAGE("running as root: search permission checks are bypassed");
	}

	/* dirFd was opened with search permission; permissions are revoked afterwards */
	TEST_ASSERT_EQUAL_INT(0, chmod(RL_DIR, 0));

	test_resetBuf();
	errno = 0;
	ret = readlinkat(test_common.at.dirFd, RL_DANGLE_NAME, test_common.buf, sizeof(test_common.buf));
	TEST_ASSERT_EQUAL_INT(0, chmod(RL_DIR, RL_DIR_MODE));
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EACCES, errno);
	TEST_ASSERT_TRUE(test_bufUnchanged(0));
}


/* fd opened with O_SEARCH: no search-permission check against the directory's current permissions. */
TEST(fileops_readlinkat, readlinkat_o_search_no_check)
{
#ifndef O_SEARCH
	TEST_IGNORE_MESSAGE("O_SEARCH not defined by this libc");
#else
	int searchFd;
	ssize_t ret;

	if (geteuid() == 0) {
		TEST_IGNORE_MESSAGE("running as root: search permission checks are bypassed");
	}

	searchFd = open(RL_DIR, O_SEARCH | O_DIRECTORY);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, searchFd);
	TEST_ASSERT_EQUAL_INT(0, chmod(RL_DIR, 0));

	test_resetBuf();
	ret = readlinkat(searchFd, RL_DANGLE_NAME, test_common.buf, sizeof(test_common.buf));
	TEST_ASSERT_EQUAL_INT(0, chmod(RL_DIR, RL_DIR_MODE));
	close(searchFd);
	TEST_ASSERT_EQUAL_INT((ssize_t)strlen(RL_DANGLE_TARGET), ret);
	TEST_ASSERT_EQUAL_MEMORY(RL_DANGLE_TARGET, test_common.buf, strlen(RL_DANGLE_TARGET));
#endif
}


TEST_GROUP_RUNNER(fileops_readlinkat)
{
	RUN_TEST_CASE(fileops_readlinkat, readlinkat_relative_to_fd);
	RUN_TEST_CASE(fileops_readlinkat, readlinkat_absolute_ignores_fd);
	RUN_TEST_CASE(fileops_readlinkat, readlinkat_at_fdcwd);
	RUN_TEST_CASE(fileops_readlinkat, readlinkat_ebadf);
	RUN_TEST_CASE(fileops_readlinkat, readlinkat_enotdir_fd_not_dir);
	RUN_TEST_CASE(fileops_readlinkat, readlinkat_eacces_fd_no_search);
	RUN_TEST_CASE(fileops_readlinkat, readlinkat_o_search_no_check);
}
