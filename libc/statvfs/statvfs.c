/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <sys/statvfs.h>
 * TESTED:
 *    - statvfs()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Lukasz Kruszynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <sys/statvfs.h>
#include <sys/stat.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

#include "unity_fixture.h"

#define STATVFS_TEST_DIR    "/tmp/test_statvfs_dir"
#define STATVFS_TEST_FILE   "/tmp/test_statvfs_file"
#define STATVFS_SYMLINK     "/tmp/test_statvfs_link"
#define STATVFS_NOPERM_DIR  "/tmp/test_statvfs_noperm"
#define STATVFS_NOPERM_FILE "/tmp/test_statvfs_noperm/inner"
#define STATVFS_LOOP_LINK_A "/tmp/test_statvfs_loop_a"
#define STATVFS_LOOP_LINK_B "/tmp/test_statvfs_loop_b"

static struct {
	int fd;
} test_common;

TEST_GROUP(statvfs_statvfs);

TEST_SETUP(statvfs_statvfs)
{
	test_common.fd = -1;
	unlink(STATVFS_TEST_FILE);
	unlink(STATVFS_SYMLINK);
	unlink(STATVFS_LOOP_LINK_A);
	unlink(STATVFS_LOOP_LINK_B);
	chmod(STATVFS_NOPERM_DIR, 0755);
	unlink(STATVFS_NOPERM_FILE);
	rmdir(STATVFS_NOPERM_DIR);
	rmdir(STATVFS_TEST_DIR);
}

TEST_TEAR_DOWN(statvfs_statvfs)
{
	if (test_common.fd >= 0) {
		close(test_common.fd);
		test_common.fd = -1;
	}
	unlink(STATVFS_TEST_FILE);
	unlink(STATVFS_SYMLINK);
	unlink(STATVFS_LOOP_LINK_A);
	unlink(STATVFS_LOOP_LINK_B);
	chmod(STATVFS_NOPERM_DIR, 0755);
	unlink(STATVFS_NOPERM_FILE);
	rmdir(STATVFS_NOPERM_DIR);
	rmdir(STATVFS_TEST_DIR);
}


TEST(statvfs_statvfs, statvfs_root_success)
{
	struct statvfs buf;
	int ret;

	memset(&buf, 0, sizeof(buf));
	ret = statvfs("/", &buf);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* block size must be positive */
	TEST_ASSERT_GREATER_THAN_UINT(0U, buf.f_bsize);
	/* fundamental block size must be positive */
	TEST_ASSERT_GREATER_THAN_UINT(0U, buf.f_frsize);
}


TEST(statvfs_statvfs, statvfs_regular_file)
{
	struct statvfs buf;
	int ret;

	test_common.fd = open(STATVFS_TEST_FILE, O_CREAT | O_WRONLY, 0644);
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.fd);

	memset(&buf, 0, sizeof(buf));
	ret = statvfs(STATVFS_TEST_FILE, &buf);
	TEST_ASSERT_EQUAL_INT(0, ret);

	TEST_ASSERT_GREATER_THAN_UINT(0U, buf.f_bsize);
	TEST_ASSERT_GREATER_THAN_UINT(0U, buf.f_frsize);
}


TEST(statvfs_statvfs, statvfs_directory)
{
	struct statvfs buf;
	int ret;

	ret = mkdir(STATVFS_TEST_DIR, 0755);
	TEST_ASSERT_EQUAL_INT(0, ret);

	memset(&buf, 0, sizeof(buf));
	ret = statvfs(STATVFS_TEST_DIR, &buf);
	TEST_ASSERT_EQUAL_INT(0, ret);

	TEST_ASSERT_GREATER_THAN_UINT(0U, buf.f_bsize);
}


TEST(statvfs_statvfs, statvfs_symlink)
{
	struct statvfs buf;
	int ret;

	test_common.fd = open(STATVFS_TEST_FILE, O_CREAT | O_WRONLY, 0644);
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.fd);

	ret = symlink(STATVFS_TEST_FILE, STATVFS_SYMLINK);
	TEST_ASSERT_EQUAL_INT(0, ret);

	memset(&buf, 0, sizeof(buf));
	ret = statvfs(STATVFS_SYMLINK, &buf);
	TEST_ASSERT_EQUAL_INT(0, ret);

	TEST_ASSERT_GREATER_THAN_UINT(0U, buf.f_bsize);
}


TEST(statvfs_statvfs, statvfs_struct_fields_consistent)
{
	struct statvfs buf;
	int ret;

	memset(&buf, 0, sizeof(buf));
	ret = statvfs("/", &buf);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* namemax should be positive */
	TEST_ASSERT_GREATER_THAN_UINT(0U, buf.f_namemax);
}


TEST(statvfs_statvfs, statvfs_same_fs_consistent)
{
	struct statvfs buf1;
	struct statvfs buf2;
	int ret;

	test_common.fd = open(STATVFS_TEST_FILE, O_CREAT | O_WRONLY, 0644);
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.fd);

	memset(&buf1, 0, sizeof(buf1));
	ret = statvfs("/tmp", &buf1);
	TEST_ASSERT_EQUAL_INT(0, ret);

	memset(&buf2, 0, sizeof(buf2));
	ret = statvfs(STATVFS_TEST_FILE, &buf2);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Two paths on same filesystem should report same block/fragment size */
	TEST_ASSERT_EQUAL_UINT(buf1.f_bsize, buf2.f_bsize);
	TEST_ASSERT_EQUAL_UINT(buf1.f_frsize, buf2.f_frsize);
	TEST_ASSERT_EQUAL_UINT(buf1.f_fsid, buf2.f_fsid);
}


TEST(statvfs_statvfs, statvfs_f_flag_readonly)
{
	struct statvfs buf;
	int ret;

	/* /proc is typically read-only; verify ST_RDONLY if available */
	memset(&buf, 0, sizeof(buf));
	ret = statvfs("/proc", &buf);
	if (ret == -1) {
		TEST_IGNORE_MESSAGE("/proc not available");
	}
	TEST_ASSERT_EQUAL_INT(0, ret);

	if ((buf.f_flag & ST_RDONLY) == 0) {
		TEST_IGNORE_MESSAGE("/proc not reported as read-only on this system");
	}
	TEST_ASSERT_EQUAL_UINT(ST_RDONLY, (buf.f_flag & ST_RDONLY));
}


TEST(statvfs_statvfs, statvfs_enoent_missing_file)
{
	struct statvfs buf;
	int ret;

	errno = 0;
	ret = statvfs("/tmp/nonexistent_statvfs_path_xyz", &buf);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);
}


TEST(statvfs_statvfs, statvfs_enoent_empty_string)
{
	struct statvfs buf;
	int ret;

	errno = 0;
	ret = statvfs("", &buf);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);
}


TEST(statvfs_statvfs, statvfs_enotdir_prefix_not_dir)
{
	struct statvfs buf;
	int ret;

	test_common.fd = open(STATVFS_TEST_FILE, O_CREAT | O_WRONLY, 0644);
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.fd);

	/* Path uses a regular file as a directory component */
	errno = 0;
	ret = statvfs(STATVFS_TEST_FILE "/child", &buf);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(ENOTDIR, errno);
}


TEST(statvfs_statvfs, statvfs_enametoolong)
{
	struct statvfs buf;
	int ret;
	int savedErrno;
	long noTrunc;
	static char longPath[PATH_MAX + 2];

	memset(longPath, 'a', sizeof(longPath) - 1);
	longPath[0] = '/';
	longPath[sizeof(longPath) - 1] = '\0';

	errno = 0;
	ret = statvfs(longPath, &buf);
	savedErrno = errno;
	TEST_ASSERT_EQUAL_INT(-1, ret);

	/*
	 * A component longer than NAME_MAX shall fail with ENAMETOOLONG when
	 * _POSIX_NO_TRUNC is in effect; otherwise the component is truncated and
	 * resolution fails with ENOENT.
	 */
	noTrunc = pathconf("/", _PC_NO_TRUNC);
	if (noTrunc > 0) {
		TEST_ASSERT_EQUAL_INT(ENAMETOOLONG, savedErrno);
	}
	else {
		TEST_ASSERT_TRUE((savedErrno == ENAMETOOLONG) || (savedErrno == ENOENT));
	}
}


TEST(statvfs_statvfs, statvfs_eacces_no_search_perm)
{
	struct statvfs buf;
	int ret;

	if (getuid() == 0) {
		TEST_IGNORE_MESSAGE("running as root - permission checks bypassed");
	}

	ret = mkdir(STATVFS_NOPERM_DIR, 0755);
	TEST_ASSERT_EQUAL_INT(0, ret);

	test_common.fd = open(STATVFS_NOPERM_FILE, O_CREAT | O_WRONLY, 0644);
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.fd);

	/* Remove search (execute) permission from the directory */
	ret = chmod(STATVFS_NOPERM_DIR, 0000);
	TEST_ASSERT_EQUAL_INT(0, ret);

	errno = 0;
	ret = statvfs(STATVFS_NOPERM_FILE, &buf);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EACCES, errno);
}


TEST(statvfs_statvfs, statvfs_eloop_symlink_loop)
{
	struct statvfs buf;
	int ret;

	/* Create circular symlink: a -> b, b -> a */
	ret = symlink(STATVFS_LOOP_LINK_B, STATVFS_LOOP_LINK_A);
	TEST_ASSERT_EQUAL_INT(0, ret);
	ret = symlink(STATVFS_LOOP_LINK_A, STATVFS_LOOP_LINK_B);
	TEST_ASSERT_EQUAL_INT(0, ret);

	errno = 0;
	ret = statvfs(STATVFS_LOOP_LINK_A, &buf);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(ELOOP, errno);
}


TEST_GROUP_RUNNER(statvfs_statvfs)
{
	RUN_TEST_CASE(statvfs_statvfs, statvfs_root_success);
	RUN_TEST_CASE(statvfs_statvfs, statvfs_regular_file);
	RUN_TEST_CASE(statvfs_statvfs, statvfs_directory);
	RUN_TEST_CASE(statvfs_statvfs, statvfs_symlink);
	RUN_TEST_CASE(statvfs_statvfs, statvfs_struct_fields_consistent);
	RUN_TEST_CASE(statvfs_statvfs, statvfs_same_fs_consistent);
	RUN_TEST_CASE(statvfs_statvfs, statvfs_f_flag_readonly);
	RUN_TEST_CASE(statvfs_statvfs, statvfs_enoent_missing_file);
	RUN_TEST_CASE(statvfs_statvfs, statvfs_enoent_empty_string);
	RUN_TEST_CASE(statvfs_statvfs, statvfs_enotdir_prefix_not_dir);
	RUN_TEST_CASE(statvfs_statvfs, statvfs_enametoolong);
	RUN_TEST_CASE(statvfs_statvfs, statvfs_eacces_no_search_perm);
	RUN_TEST_CASE(statvfs_statvfs, statvfs_eloop_symlink_loop);
}
