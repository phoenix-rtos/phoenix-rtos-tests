/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <sys/stat.h>
 * TESTED:
 *    - mknod()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Lukasz Kruszynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define _XOPEN_SOURCE 700

#include <errno.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "common.h"
#include <unity_fixture.h>

#define MKNOD_FIFO_PATH    "mknod_fifo"
#define MKNOD_EXIST_PATH   "mknod_exist"
#define MKNOD_SYMLINK_PATH "mknod_symlink"
#define MKNOD_REGFILE_PATH "mknod_regfile"
#define MKNOD_CHARDEV_PATH "mknod_chardev"

#define PERM_BITS 0777
/* A single path component longer than any NAME_MAX / PATH_MAX limit. */
#define MKNOD_LONG_LEN 5000


static struct {
	mode_t oldUmask;
} test_common;


TEST_GROUP(fileops_mknod);


TEST_SETUP(fileops_mknod)
{
	test_common.oldUmask = umask(0);

	unlink(MKNOD_FIFO_PATH);
	unlink(MKNOD_EXIST_PATH);
	unlink(MKNOD_SYMLINK_PATH);
	unlink(MKNOD_REGFILE_PATH);
	unlink(MKNOD_CHARDEV_PATH);
}


TEST_TEAR_DOWN(fileops_mknod)
{
	umask(test_common.oldUmask);

	unlink(MKNOD_FIFO_PATH);
	unlink(MKNOD_EXIST_PATH);
	unlink(MKNOD_SYMLINK_PATH);
	unlink(MKNOD_REGFILE_PATH);
	unlink(MKNOD_CHARDEV_PATH);
}


/* The portable use of mknod() shall create a FIFO-special file. */
TEST(fileops_mknod, fifo_creates_fifo)
{
	struct stat st;

	errno = 0;
	TEST_ASSERT_EQUAL_INT(0, mknod(MKNOD_FIFO_PATH, S_IFIFO | 0644, 0));

	TEST_ASSERT_EQUAL_INT(0, stat(MKNOD_FIFO_PATH, &st));
	TEST_ASSERT_TRUE(S_ISFIFO(st.st_mode));
	/* umask is 0 in setup, so the permission bits are taken verbatim. */
	TEST_ASSERT_EQUAL_INT(0644, (int)(st.st_mode & PERM_BITS));
}


/* Permission bits of the new file shall be modified by the file mode creation mask. */
TEST(fileops_mknod, perms_masked_by_umask)
{
	TEST_IGNORE_MESSAGE("Unverified Failure");
	struct stat st;

	umask(022);

	TEST_ASSERT_EQUAL_INT(0, mknod(MKNOD_FIFO_PATH, S_IFIFO | 0666, 0));
	TEST_ASSERT_EQUAL_INT(0, stat(MKNOD_FIFO_PATH, &st));
	TEST_ASSERT_EQUAL_INT(0644, (int)(st.st_mode & PERM_BITS));
}


/*
 * mknod() shall mark the new file's timestamps and the containing directory's
 * modification time for update.
 */
TEST(fileops_mknod, marks_timestamps)
{
	struct stat st;
	struct stat dirSt;
	time_t before;

	before = time(NULL);
	TEST_ASSERT_NOT_EQUAL_INT((time_t)-1, before);

	TEST_ASSERT_EQUAL_INT(0, mknod(MKNOD_FIFO_PATH, S_IFIFO | 0644, 0));

	TEST_ASSERT_EQUAL_INT(0, stat(MKNOD_FIFO_PATH, &st));
	TEST_ASSERT_TRUE(st.st_mtime >= before);
	TEST_ASSERT_TRUE(st.st_ctime >= before);

	TEST_ASSERT_EQUAL_INT(0, stat(".", &dirSt));
	TEST_ASSERT_TRUE(dirSt.st_mtime >= before);
}


/* mknod() on an existing name shall fail with EEXIST. */
TEST(fileops_mknod, existing_file_eexist)
{
	create_file(MKNOD_EXIST_PATH, NULL);

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, mknod(MKNOD_EXIST_PATH, S_IFIFO | 0644, 0));
	TEST_ASSERT_EQUAL_INT(EEXIST, errno);
}


/* If path names a symbolic link, mknod() shall fail with EEXIST. */
TEST(fileops_mknod, symlink_eexist)
{
	TEST_IGNORE_MESSAGE("Unverified Failure");
	TEST_ASSERT_EQUAL_INT(0, symlink("mknod_dangling_target", MKNOD_SYMLINK_PATH));

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, mknod(MKNOD_SYMLINK_PATH, S_IFIFO | 0644, 0));
	TEST_ASSERT_EQUAL_INT(EEXIST, errno);
}


/* A missing path prefix component shall fail with ENOENT and create nothing. */
TEST(fileops_mknod, missing_prefix_enoent)
{
	struct stat st;

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, mknod("mknod_absent_dir_xyz/child", S_IFIFO | 0644, 0));
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);

	/* On failure the new file shall not have been created. */
	TEST_ASSERT_EQUAL_INT(-1, stat("mknod_absent_dir_xyz/child", &st));
}


/* A non-directory in the path prefix shall fail with ENOTDIR. */
TEST(fileops_mknod, file_prefix_enotdir)
{
	create_file(MKNOD_REGFILE_PATH, NULL);

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, mknod(MKNOD_REGFILE_PATH "/child", S_IFIFO | 0644, 0));
	TEST_ASSERT_EQUAL_INT(ENOTDIR, errno);
}


/* A path component longer than NAME_MAX shall fail with ENAMETOOLONG. */
TEST(fileops_mknod, long_component_enametoolong)
{
	static char longName[MKNOD_LONG_LEN];

	memset(longName, 'a', sizeof(longName) - 1);
	longName[sizeof(longName) - 1] = '\0';

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, mknod(longName, S_IFIFO | 0644, 0));
	TEST_ASSERT_EQUAL_INT(ENAMETOOLONG, errno);
}


/*
 * A process without appropriate privileges shall not create a file type other
 * than FIFO-special; the attempt shall fail with EPERM.
 */
TEST(fileops_mknod, non_fifo_unprivileged_eperm)
{
	if (geteuid() == 0) {
		TEST_IGNORE_MESSAGE("running as root: EPERM for non-FIFO mknod not observable");
	}

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, mknod(MKNOD_CHARDEV_PATH, S_IFCHR | 0644, 0));
	TEST_ASSERT_EQUAL_INT(EPERM, errno);
}


TEST_GROUP_RUNNER(fileops_mknod)
{
	RUN_TEST_CASE(fileops_mknod, fifo_creates_fifo);
	RUN_TEST_CASE(fileops_mknod, perms_masked_by_umask);
	RUN_TEST_CASE(fileops_mknod, marks_timestamps);
	RUN_TEST_CASE(fileops_mknod, existing_file_eexist);
	RUN_TEST_CASE(fileops_mknod, symlink_eexist);
	RUN_TEST_CASE(fileops_mknod, missing_prefix_enoent);
	RUN_TEST_CASE(fileops_mknod, file_prefix_enotdir);
	RUN_TEST_CASE(fileops_mknod, long_component_enametoolong);
	RUN_TEST_CASE(fileops_mknod, non_fifo_unprivileged_eperm);
}
