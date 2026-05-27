/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <fcntl.h>
 * TESTED:
 *    - openat()
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
#include <fcntl.h>
#include <sys/stat.h>
#include <limits.h>

#include <unity_fixture.h>

#define OPENAT_TEST_DIR   "openat_test_dir"
#define OPENAT_TEST_FILE  "openat_file.tmp"
#define OPENAT_TEST_PATH  "openat_test_dir/openat_file.tmp"
#define OPENAT_NEWFILE    "openat_newfile.tmp"
#define OPENAT_NEWPATH    "openat_test_dir/openat_newfile.tmp"
#define OPENAT_SYMLINK    "openat_test_dir/openat_link"


static struct {
	int dirfd;
	int fd;
} test_common;


TEST_GROUP(fcntl_openat);


TEST_SETUP(fcntl_openat)
{
	(void)unlink(OPENAT_TEST_PATH);
	(void)unlink(OPENAT_NEWPATH);
	(void)unlink(OPENAT_SYMLINK);
	(void)rmdir(OPENAT_TEST_DIR);

	(void)mkdir(OPENAT_TEST_DIR, 0755);
	test_common.dirfd = open(OPENAT_TEST_DIR, O_RDONLY | O_DIRECTORY);
	test_common.fd = -1;
}


TEST_TEAR_DOWN(fcntl_openat)
{
	if (test_common.fd >= 0) {
		close(test_common.fd);
	}
	if (test_common.dirfd >= 0) {
		close(test_common.dirfd);
	}
	(void)unlink(OPENAT_TEST_PATH);
	(void)unlink(OPENAT_NEWPATH);
	(void)unlink(OPENAT_SYMLINK);
	(void)rmdir(OPENAT_TEST_DIR);
}


static void test_createFileInDir(void)
{
	int fd;

	fd = openat(test_common.dirfd, OPENAT_TEST_FILE, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
	if (fd >= 0) {
		close(fd);
	}
}


/* openat: create file relative to directory fd */
TEST(fcntl_openat, create_relative_to_dirfd)
{
	struct stat st;
	int ret;

	test_common.fd = openat(test_common.dirfd, OPENAT_NEWFILE, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
	TEST_ASSERT_TRUE(test_common.fd >= 0);

	/* Verify file was created in the correct directory */
	ret = stat(OPENAT_NEWPATH, &st);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


/* openat: open existing file for reading */
TEST(fcntl_openat, open_existing_rdonly)
{
	ssize_t n;
	char buf[8];

	test_createFileInDir();

	test_common.fd = openat(test_common.dirfd, OPENAT_TEST_FILE, O_RDONLY);
	TEST_ASSERT_TRUE(test_common.fd >= 0);

	/* Reading empty file returns 0 */
	n = read(test_common.fd, buf, sizeof(buf));
	TEST_ASSERT_EQUAL_INT(0, (int)n);
}


/* openat: open file for read-write */
TEST(fcntl_openat, open_rdwr)
{
	ssize_t n;
	char buf[8];
	const char *data = "test";

	test_common.fd = openat(test_common.dirfd, OPENAT_NEWFILE, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
	TEST_ASSERT_TRUE(test_common.fd >= 0);

	n = write(test_common.fd, data, 4);
	TEST_ASSERT_EQUAL_INT(4, (int)n);

	(void)lseek(test_common.fd, 0, SEEK_SET);

	n = read(test_common.fd, buf, sizeof(buf));
	TEST_ASSERT_EQUAL_INT(4, (int)n);
	TEST_ASSERT_EQUAL_MEMORY(data, buf, 4);
}


/* openat: O_CREAT | O_EXCL fails if file exists */
TEST(fcntl_openat, creat_excl_exists)
{
	int fd2;

	test_createFileInDir();

	errno = 0;
	fd2 = openat(test_common.dirfd, OPENAT_TEST_FILE, O_CREAT | O_EXCL | O_WRONLY, S_IRUSR | S_IWUSR);
	TEST_ASSERT_EQUAL_INT(-1, fd2);
	TEST_ASSERT_EQUAL_INT(EEXIST, errno);
}


/* openat: O_TRUNC truncates existing file */
TEST(fcntl_openat, trunc_existing)
{
	ssize_t n;
	struct stat st;
	int ret;

	test_common.fd = openat(test_common.dirfd, OPENAT_NEWFILE, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
	TEST_ASSERT_TRUE(test_common.fd >= 0);
	n = write(test_common.fd, "hello world", 11);
	TEST_ASSERT_EQUAL_INT(11, (int)n);
	close(test_common.fd);

	test_common.fd = openat(test_common.dirfd, OPENAT_NEWFILE, O_WRONLY | O_TRUNC);
	TEST_ASSERT_TRUE(test_common.fd >= 0);

	ret = fstat(test_common.fd, &st);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, (int)st.st_size);
}


/* openat: O_APPEND causes writes at end */
TEST(fcntl_openat, append_writes_at_end)
{
	ssize_t n;
	struct stat st;
	int ret;

	test_common.fd = openat(test_common.dirfd, OPENAT_NEWFILE, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
	TEST_ASSERT_TRUE(test_common.fd >= 0);
	n = write(test_common.fd, "abc", 3);
	TEST_ASSERT_EQUAL_INT(3, (int)n);
	close(test_common.fd);

	test_common.fd = openat(test_common.dirfd, OPENAT_NEWFILE, O_WRONLY | O_APPEND);
	TEST_ASSERT_TRUE(test_common.fd >= 0);
	n = write(test_common.fd, "de", 2);
	TEST_ASSERT_EQUAL_INT(2, (int)n);

	ret = fstat(test_common.fd, &st);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(5, (int)st.st_size);
}


/* openat: absolute path ignores dirfd */
TEST(fcntl_openat, absolute_path_ignores_dirfd)
{
	struct stat st;
	int ret;

	test_common.fd = openat(test_common.dirfd, "/dev/null", O_RDONLY);
	TEST_ASSERT_TRUE(test_common.fd >= 0);

	ret = fstat(test_common.fd, &st);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


/* openat: AT_FDCWD uses current working directory */
TEST(fcntl_openat, at_fdcwd_uses_cwd)
{
	struct stat st;
	int ret;

	test_common.fd = openat(AT_FDCWD, OPENAT_TEST_DIR, O_RDONLY | O_DIRECTORY);
	TEST_ASSERT_TRUE(test_common.fd >= 0);

	ret = fstat(test_common.fd, &st);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_TRUE(S_ISDIR(st.st_mode));
}


/* openat: O_CLOEXEC sets FD_CLOEXEC on new fd */
TEST(fcntl_openat, cloexec_sets_flag)
{
	int flags;

	test_createFileInDir();

	test_common.fd = openat(test_common.dirfd, OPENAT_TEST_FILE, O_RDONLY | O_CLOEXEC);
	TEST_ASSERT_TRUE(test_common.fd >= 0);

	flags = fcntl(test_common.fd, F_GETFD);
	TEST_ASSERT_TRUE(flags >= 0);
	TEST_ASSERT_EQUAL_INT(FD_CLOEXEC, flags & FD_CLOEXEC);
}


/* openat: O_NOFOLLOW fails on symlink with ELOOP */
TEST(fcntl_openat, nofollow_eloop_symlink)
{
	int ret;
	int fd2;

	test_createFileInDir();

	ret = symlink(OPENAT_TEST_FILE, OPENAT_SYMLINK);
	TEST_ASSERT_EQUAL_INT(0, ret);

	errno = 0;
	fd2 = openat(test_common.dirfd, "openat_link", O_RDONLY | O_NOFOLLOW);
	TEST_ASSERT_EQUAL_INT(-1, fd2);
	TEST_ASSERT_EQUAL_INT(ELOOP, errno);
}


/* openat: ENOENT when file does not exist and O_CREAT not set */
TEST(fcntl_openat, enoent_no_creat)
{
	int fd2;

	errno = 0;
	fd2 = openat(test_common.dirfd, "nonexistent_xyz", O_RDONLY);
	TEST_ASSERT_EQUAL_INT(-1, fd2);
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);
}


/* openat: ENOTDIR when dirfd is not a directory */
TEST(fcntl_openat, enotdir_not_directory_fd)
{
	int filefd;
	int fd2;

	test_createFileInDir();

	filefd = open(OPENAT_TEST_PATH, O_RDONLY);
	TEST_ASSERT_TRUE(filefd >= 0);

	errno = 0;
	fd2 = openat(filefd, "anything", O_RDONLY);
	TEST_ASSERT_EQUAL_INT(-1, fd2);
	TEST_ASSERT_EQUAL_INT(ENOTDIR, errno);

	close(filefd);
}


/* openat: EBADF when dirfd is invalid */
TEST(fcntl_openat, ebadf_invalid_dirfd)
{
	int fd2;

	errno = 0;
	fd2 = openat(-1, "file", O_RDONLY);
	TEST_ASSERT_EQUAL_INT(-1, fd2);
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


/* openat: file offset starts at 0 */
TEST(fcntl_openat, offset_starts_at_zero)
{
	off_t off;

	test_createFileInDir();

	test_common.fd = openat(test_common.dirfd, OPENAT_TEST_FILE, O_RDONLY);
	TEST_ASSERT_TRUE(test_common.fd >= 0);

	off = lseek(test_common.fd, 0, SEEK_CUR);
	TEST_ASSERT_EQUAL_INT(0, (int)off);
}


/* openat: O_DIRECTORY fails on regular file with ENOTDIR */
TEST(fcntl_openat, o_directory_on_file_enotdir)
{
	int fd2;

	test_createFileInDir();

	errno = 0;
	fd2 = openat(test_common.dirfd, OPENAT_TEST_FILE, O_RDONLY | O_DIRECTORY);
	TEST_ASSERT_EQUAL_INT(-1, fd2);
	TEST_ASSERT_EQUAL_INT(ENOTDIR, errno);
}


TEST_GROUP_RUNNER(fcntl_openat)
{
	RUN_TEST_CASE(fcntl_openat, create_relative_to_dirfd);
	RUN_TEST_CASE(fcntl_openat, open_existing_rdonly);
	RUN_TEST_CASE(fcntl_openat, open_rdwr);
	RUN_TEST_CASE(fcntl_openat, creat_excl_exists);
	RUN_TEST_CASE(fcntl_openat, trunc_existing);
	RUN_TEST_CASE(fcntl_openat, append_writes_at_end);
	RUN_TEST_CASE(fcntl_openat, absolute_path_ignores_dirfd);
	RUN_TEST_CASE(fcntl_openat, at_fdcwd_uses_cwd);
	RUN_TEST_CASE(fcntl_openat, cloexec_sets_flag);
	RUN_TEST_CASE(fcntl_openat, nofollow_eloop_symlink);
	RUN_TEST_CASE(fcntl_openat, enoent_no_creat);
	RUN_TEST_CASE(fcntl_openat, enotdir_not_directory_fd);
	RUN_TEST_CASE(fcntl_openat, ebadf_invalid_dirfd);
	RUN_TEST_CASE(fcntl_openat, offset_starts_at_zero);
	RUN_TEST_CASE(fcntl_openat, o_directory_on_file_enotdir);
}
