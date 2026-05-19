/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - fcntl.h
 * TESTED:
 *    - open()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>

#include "common.h"
#include "unity_fixture.h"

#define OPEN_TEST_FILE    "/tmp/test_open_file"
#define OPEN_TEST_FILE2   "/tmp/test_open_file2"
#define OPEN_TEST_DIR     "/tmp/test_open_dir"
#define OPEN_TEST_SYMLINK "/tmp/test_open_symlink"
#define OPEN_TEST_FIFO    "/tmp/test_open_fifo"
#define OPEN_TEST_DATA    "hello world"


static struct {
	int fd;
	int fd2;
} test_common;


TEST_GROUP(fcntl_open);


TEST_SETUP(fcntl_open)
{
	unlink(OPEN_TEST_FILE);
	unlink(OPEN_TEST_FILE2);
	unlink(OPEN_TEST_SYMLINK);
	unlink(OPEN_TEST_FIFO);
	rmdir(OPEN_TEST_DIR);

	test_common.fd = -1;
	test_common.fd2 = -1;
}


TEST_TEAR_DOWN(fcntl_open)
{
	if (test_common.fd >= 0) {
		close(test_common.fd);
	}
	if (test_common.fd2 >= 0) {
		close(test_common.fd2);
	}
	unlink(OPEN_TEST_FILE);
	unlink(OPEN_TEST_FILE2);
	unlink(OPEN_TEST_SYMLINK);
	unlink(OPEN_TEST_FIFO);
	rmdir(OPEN_TEST_DIR);
}


TEST(fcntl_open, open_rdonly_existing)
{
	ssize_t n;
	char buf[32];
	int ret;

	ret = _create_file(OPEN_TEST_FILE, OPEN_TEST_DATA);
	TEST_ASSERT_EQUAL_INT(0, ret);

	test_common.fd = open(OPEN_TEST_FILE, O_RDONLY);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_common.fd);

	n = read(test_common.fd, buf, sizeof(buf) - 1);
	TEST_ASSERT_EQUAL_INT((int)strlen(OPEN_TEST_DATA), (int)n);
	buf[n] = '\0';
	TEST_ASSERT_EQUAL_STRING(OPEN_TEST_DATA, buf);
}


TEST(fcntl_open, open_wronly_existing)
{
	ssize_t n;
	char buf[32];
	const char *newData = "overwrite";
	int ret;

	ret = _create_file(OPEN_TEST_FILE, OPEN_TEST_DATA);
	TEST_ASSERT_EQUAL_INT(0, ret);

	test_common.fd = open(OPEN_TEST_FILE, O_WRONLY);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_common.fd);

	n = write(test_common.fd, newData, strlen(newData));
	TEST_ASSERT_EQUAL_INT((int)strlen(newData), (int)n);

	/* verify cannot read from write-only fd */
	n = read(test_common.fd, buf, sizeof(buf));
	TEST_ASSERT_EQUAL_INT(-1, (int)n);
}


TEST(fcntl_open, open_rdwr_existing)
{
	ssize_t n;
	char buf[32];
	const char *newData = "rw";
	int ret;

	ret = _create_file(OPEN_TEST_FILE, OPEN_TEST_DATA);
	TEST_ASSERT_EQUAL_INT(0, ret);

	test_common.fd = open(OPEN_TEST_FILE, O_RDWR);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_common.fd);

	n = write(test_common.fd, newData, strlen(newData));
	TEST_ASSERT_EQUAL_INT((int)strlen(newData), (int)n);

	ret = (int)lseek(test_common.fd, 0, SEEK_SET);
	TEST_ASSERT_EQUAL_INT(0, ret);

	n = read(test_common.fd, buf, sizeof(buf) - 1);
	TEST_ASSERT_GREATER_THAN_INT(0, (int)n);
	buf[n] = '\0';
	/* first two chars overwritten */
	TEST_ASSERT_EQUAL_MEMORY(newData, buf, strlen(newData));
}


TEST(fcntl_open, open_creat_new_file)
{
	struct stat st;
	mode_t prevMask;
	int ret;

	prevMask = umask(0022);

	test_common.fd = open(OPEN_TEST_FILE, O_WRONLY | O_CREAT, 0666);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_common.fd);

	ret = fstat(test_common.fd, &st);
	TEST_ASSERT_EQUAL_INT(0, ret);
	/* 0666 & ~0022 = 0644 */
	TEST_ASSERT_EQUAL_INT(0644, (int)(st.st_mode & 0777));

	umask(prevMask);
}


TEST(fcntl_open, open_creat_existing_no_effect)
{
	struct stat st;
	int ret;

	ret = _create_file(OPEN_TEST_FILE, OPEN_TEST_DATA);
	TEST_ASSERT_EQUAL_INT(0, ret);

	test_common.fd = open(OPEN_TEST_FILE, O_RDONLY | O_CREAT, 0600);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_common.fd);

	ret = fstat(test_common.fd, &st);
	TEST_ASSERT_EQUAL_INT(0, ret);
	/* file should still have data - O_CREAT alone doesn't truncate */
	TEST_ASSERT_GREATER_THAN_INT(0, (int)st.st_size);
}


TEST(fcntl_open, open_creat_excl_new_file)
{
	test_common.fd = open(OPEN_TEST_FILE, O_WRONLY | O_CREAT | O_EXCL, 0644);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_common.fd);
}


TEST(fcntl_open, open_creat_excl_existing_eexist)
{
	int fd;
	int ret;

	ret = _create_file(OPEN_TEST_FILE, OPEN_TEST_DATA);
	TEST_ASSERT_EQUAL_INT(0, ret);

	errno = 0;
	fd = open(OPEN_TEST_FILE, O_WRONLY | O_CREAT | O_EXCL, 0644);
	TEST_ASSERT_EQUAL_INT(-1, fd);
	TEST_ASSERT_EQUAL_INT(EEXIST, errno);
}


TEST(fcntl_open, open_creat_excl_symlink_eexist)
{
	int fd;
	int ret;

	ret = symlink(OPEN_TEST_FILE, OPEN_TEST_SYMLINK);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* O_CREAT|O_EXCL on symlink shall fail with EEXIST regardless of target */
	errno = 0;
	fd = open(OPEN_TEST_SYMLINK, O_WRONLY | O_CREAT | O_EXCL, 0644);
	TEST_ASSERT_EQUAL_INT(-1, fd);
	TEST_ASSERT_EQUAL_INT(EEXIST, errno);
}


TEST(fcntl_open, open_trunc_existing)
{
	struct stat st;
	int ret;

	ret = _create_file(OPEN_TEST_FILE, OPEN_TEST_DATA);
	TEST_ASSERT_EQUAL_INT(0, ret);

	test_common.fd = open(OPEN_TEST_FILE, O_WRONLY | O_TRUNC);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_common.fd);

	ret = fstat(test_common.fd, &st);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, (int)st.st_size);
}


TEST(fcntl_open, open_append)
{
	ssize_t n;
	char buf[64];
	const char *extra = "XYZ";
	int ret;

	ret = _create_file(OPEN_TEST_FILE, OPEN_TEST_DATA);
	TEST_ASSERT_EQUAL_INT(0, ret);

	test_common.fd = open(OPEN_TEST_FILE, O_WRONLY | O_APPEND);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_common.fd);

	n = write(test_common.fd, extra, strlen(extra));
	TEST_ASSERT_EQUAL_INT((int)strlen(extra), (int)n);

	close(test_common.fd);
	test_common.fd = -1;

	/* verify appended content */
	test_common.fd = open(OPEN_TEST_FILE, O_RDONLY);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_common.fd);

	n = read(test_common.fd, buf, sizeof(buf) - 1);
	TEST_ASSERT_EQUAL_INT((int)(strlen(OPEN_TEST_DATA) + strlen(extra)), (int)n);
	buf[n] = '\0';
	TEST_ASSERT_EQUAL_STRING(OPEN_TEST_DATA "XYZ", buf);
}


TEST(fcntl_open, open_cloexec_flag)
{
	int flags;

	test_common.fd = open(OPEN_TEST_FILE, O_WRONLY | O_CREAT | O_CLOEXEC, 0644);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_common.fd);

	flags = fcntl(test_common.fd, F_GETFD);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, flags);
	TEST_ASSERT_EQUAL_INT(FD_CLOEXEC, flags & FD_CLOEXEC);
}


TEST(fcntl_open, open_no_cloexec_by_default)
{
	int flags;

	test_common.fd = open(OPEN_TEST_FILE, O_WRONLY | O_CREAT, 0644);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_common.fd);

	flags = fcntl(test_common.fd, F_GETFD);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, flags);
	TEST_ASSERT_EQUAL_INT(0, flags & FD_CLOEXEC);
}


TEST(fcntl_open, open_directory)
{
	int ret;

	ret = mkdir(OPEN_TEST_DIR, 0755);
	TEST_ASSERT_EQUAL_INT(0, ret);

	test_common.fd = open(OPEN_TEST_DIR, O_RDONLY | O_DIRECTORY);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_common.fd);
}


TEST(fcntl_open, open_directory_enotdir)
{
	int fd;
	int ret;

	ret = _create_file(OPEN_TEST_FILE, OPEN_TEST_DATA);
	TEST_ASSERT_EQUAL_INT(0, ret);

	errno = 0;
	fd = open(OPEN_TEST_FILE, O_RDONLY | O_DIRECTORY);
	TEST_ASSERT_EQUAL_INT(-1, fd);
	TEST_ASSERT_EQUAL_INT(ENOTDIR, errno);
}


TEST(fcntl_open, open_nofollow_eloop)
{
	int fd;
	int ret;

	ret = _create_file(OPEN_TEST_FILE, OPEN_TEST_DATA);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = symlink(OPEN_TEST_FILE, OPEN_TEST_SYMLINK);
	TEST_ASSERT_EQUAL_INT(0, ret);

	errno = 0;
	fd = open(OPEN_TEST_SYMLINK, O_RDONLY | O_NOFOLLOW);
	TEST_ASSERT_EQUAL_INT(-1, fd);
	TEST_ASSERT_EQUAL_INT(ELOOP, errno);
}


TEST(fcntl_open, open_enoent_no_creat)
{
	int fd;

	errno = 0;
	fd = open(OPEN_TEST_FILE, O_RDONLY);
	TEST_ASSERT_EQUAL_INT(-1, fd);
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);
}


TEST(fcntl_open, open_enoent_path_prefix)
{
	int fd;

	/* component of path prefix does not exist */
	errno = 0;
	fd = open("/tmp/nonexistent_dir_xyz/file", O_WRONLY | O_CREAT, 0644);
	TEST_ASSERT_EQUAL_INT(-1, fd);
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);
}


TEST(fcntl_open, open_enoent_empty_path)
{
	int fd;

	errno = 0;
	fd = open("", O_RDONLY);
	TEST_ASSERT_EQUAL_INT(-1, fd);
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);
}


TEST(fcntl_open, open_eisdir_wronly)
{
	int fd;
	int ret;

	ret = mkdir(OPEN_TEST_DIR, 0755);
	TEST_ASSERT_EQUAL_INT(0, ret);

	errno = 0;
	fd = open(OPEN_TEST_DIR, O_WRONLY);
	TEST_ASSERT_EQUAL_INT(-1, fd);
	TEST_ASSERT_EQUAL_INT(EISDIR, errno);
}


TEST(fcntl_open, open_eisdir_rdwr)
{
	int fd;
	int ret;

	ret = mkdir(OPEN_TEST_DIR, 0755);
	TEST_ASSERT_EQUAL_INT(0, ret);

	errno = 0;
	fd = open(OPEN_TEST_DIR, O_RDWR);
	TEST_ASSERT_EQUAL_INT(-1, fd);
	TEST_ASSERT_EQUAL_INT(EISDIR, errno);
}


TEST(fcntl_open, open_enotdir_prefix)
{
	int fd;
	int ret;

	ret = _create_file(OPEN_TEST_FILE, OPEN_TEST_DATA);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* path prefix component is a regular file, not a directory */
	errno = 0;
	fd = open(OPEN_TEST_FILE "/child", O_RDONLY);
	TEST_ASSERT_EQUAL_INT(-1, fd);
	TEST_ASSERT_EQUAL_INT(ENOTDIR, errno);
}


TEST(fcntl_open, open_enametoolong)
{
	static char longName[NAME_MAX + 2];
	int fd;

	memset(longName, 'a', NAME_MAX + 1);
	longName[NAME_MAX + 1] = '\0';

	errno = 0;
	fd = open(longName, O_RDONLY);
	TEST_ASSERT_EQUAL_INT(-1, fd);
	TEST_ASSERT_EQUAL_INT(ENAMETOOLONG, errno);
}


TEST(fcntl_open, open_offset_at_beginning)
{
	ssize_t n;
	char buf[4];
	int ret;

	ret = _create_file(OPEN_TEST_FILE, OPEN_TEST_DATA);
	TEST_ASSERT_EQUAL_INT(0, ret);

	test_common.fd = open(OPEN_TEST_FILE, O_RDONLY);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_common.fd);

	/* first read should start from offset 0 */
	n = read(test_common.fd, buf, 1);
	TEST_ASSERT_EQUAL_INT(1, (int)n);
	TEST_ASSERT_EQUAL_CHAR(OPEN_TEST_DATA[0], buf[0]);
}


TEST(fcntl_open, open_returns_lowest_fd)
{
	int fd1, fd2, fd3;
	int ret;

	ret = _create_file(OPEN_TEST_FILE, OPEN_TEST_DATA);
	TEST_ASSERT_EQUAL_INT(0, ret);

	fd1 = open(OPEN_TEST_FILE, O_RDONLY);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd1);

	fd2 = open(OPEN_TEST_FILE, O_RDONLY);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd2);
	TEST_ASSERT_GREATER_THAN_INT(fd1, fd2);

	/* close fd1, next open should reuse it */
	close(fd1);

	fd3 = open(OPEN_TEST_FILE, O_RDONLY);
	TEST_ASSERT_EQUAL_INT(fd1, fd3);

	close(fd2);
	close(fd3);
}


TEST(fcntl_open, open_nonblock_fifo_rdonly)
{
	int ret;

	ret = mkfifo(OPEN_TEST_FIFO, 0644);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* O_NONBLOCK + O_RDONLY on FIFO shall return without delay */
	test_common.fd = open(OPEN_TEST_FIFO, O_RDONLY | O_NONBLOCK);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_common.fd);
}


TEST(fcntl_open, open_nonblock_fifo_wronly_enxio)
{
	int fd;
	int ret;

	ret = mkfifo(OPEN_TEST_FIFO, 0644);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* O_NONBLOCK + O_WRONLY on FIFO with no reader shall fail with ENXIO */
	errno = 0;
	fd = open(OPEN_TEST_FIFO, O_WRONLY | O_NONBLOCK);
	TEST_ASSERT_EQUAL_INT(-1, fd);
	TEST_ASSERT_EQUAL_INT(ENXIO, errno);
}


TEST(fcntl_open, open_return_value_nonnegative)
{
	int ret;

	ret = _create_file(OPEN_TEST_FILE, OPEN_TEST_DATA);
	TEST_ASSERT_EQUAL_INT(0, ret);

	test_common.fd = open(OPEN_TEST_FILE, O_RDONLY);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_common.fd);
}


TEST(fcntl_open, open_failure_returns_minus_one)
{
	int fd;

	errno = 0;
	fd = open("/tmp/nonexistent_xyz_abc", O_RDONLY);
	TEST_ASSERT_EQUAL_INT(-1, fd);
	TEST_ASSERT_NOT_EQUAL_INT(0, errno);
}


TEST_GROUP_RUNNER(fcntl_open)
{
	RUN_TEST_CASE(fcntl_open, open_rdonly_existing);
	RUN_TEST_CASE(fcntl_open, open_wronly_existing);
	RUN_TEST_CASE(fcntl_open, open_rdwr_existing);
	RUN_TEST_CASE(fcntl_open, open_creat_new_file);
	RUN_TEST_CASE(fcntl_open, open_creat_existing_no_effect);
	RUN_TEST_CASE(fcntl_open, open_creat_excl_new_file);
	RUN_TEST_CASE(fcntl_open, open_creat_excl_existing_eexist);
	RUN_TEST_CASE(fcntl_open, open_creat_excl_symlink_eexist);
	RUN_TEST_CASE(fcntl_open, open_trunc_existing);
	RUN_TEST_CASE(fcntl_open, open_append);
	RUN_TEST_CASE(fcntl_open, open_cloexec_flag);
	RUN_TEST_CASE(fcntl_open, open_no_cloexec_by_default);
	RUN_TEST_CASE(fcntl_open, open_directory);
	RUN_TEST_CASE(fcntl_open, open_directory_enotdir);
	RUN_TEST_CASE(fcntl_open, open_nofollow_eloop);
	RUN_TEST_CASE(fcntl_open, open_enoent_no_creat);
	RUN_TEST_CASE(fcntl_open, open_enoent_path_prefix);
	RUN_TEST_CASE(fcntl_open, open_enoent_empty_path);
	RUN_TEST_CASE(fcntl_open, open_eisdir_wronly);
	RUN_TEST_CASE(fcntl_open, open_eisdir_rdwr);
	RUN_TEST_CASE(fcntl_open, open_enotdir_prefix);
	RUN_TEST_CASE(fcntl_open, open_enametoolong);
	RUN_TEST_CASE(fcntl_open, open_offset_at_beginning);
	RUN_TEST_CASE(fcntl_open, open_returns_lowest_fd);
	RUN_TEST_CASE(fcntl_open, open_nonblock_fifo_rdonly);
	RUN_TEST_CASE(fcntl_open, open_nonblock_fifo_wronly_enxio);
	RUN_TEST_CASE(fcntl_open, open_return_value_nonnegative);
	RUN_TEST_CASE(fcntl_open, open_failure_returns_minus_one);
}
