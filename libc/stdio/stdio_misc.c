/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <stdio.h>
 * TESTED:
 *    - getc_unlocked()
 *    - getchar_unlocked()
 *    - putc_unlocked()
 *    - putchar_unlocked()
 *    - renameat()
 *    - tempnam()
 *    - tmpnam()
 *    - vdprintf()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "unity_fixture.h"

#define TEST_FILE_A    "test_stdio_misc_a"
#define TEST_FILE_B    "test_stdio_misc_b"
#define TEST_DIR       "test_stdio_misc_dir"
#define BUF_SIZE       256


/* Tests: getc_unlocked, putc_unlocked, getchar_unlocked, putchar_unlocked */
TEST_GROUP(stdio_unlocked);

TEST_SETUP(stdio_unlocked)
{
	unlink(TEST_FILE_A);
}

TEST_TEAR_DOWN(stdio_unlocked)
{
	unlink(TEST_FILE_A);
}


TEST(stdio_unlocked, getc_unlocked_reads_chars)
{
	FILE *fp;
	int ch;
	const char *data = "abc";

	fp = fopen(TEST_FILE_A, "w");
	TEST_ASSERT_NOT_NULL(fp);
	fputs(data, fp);
	fclose(fp);
	fp = NULL;

	fp = fopen(TEST_FILE_A, "r");
	TEST_ASSERT_NOT_NULL(fp);

	flockfile(fp);
	ch = getc_unlocked(fp);
	TEST_ASSERT_EQUAL_INT('a', ch);
	ch = getc_unlocked(fp);
	TEST_ASSERT_EQUAL_INT('b', ch);
	ch = getc_unlocked(fp);
	TEST_ASSERT_EQUAL_INT('c', ch);
	ch = getc_unlocked(fp);
	TEST_ASSERT_EQUAL_INT(EOF, ch);
	funlockfile(fp);

	fclose(fp);
}


TEST(stdio_unlocked, putc_unlocked_writes_chars)
{
	FILE *fp;
	int ret;
	char buf[16];
	size_t n;

	fp = fopen(TEST_FILE_A, "w");
	TEST_ASSERT_NOT_NULL(fp);

	flockfile(fp);
	ret = putc_unlocked('X', fp);
	TEST_ASSERT_EQUAL_INT('X', ret);
	ret = putc_unlocked('Y', fp);
	TEST_ASSERT_EQUAL_INT('Y', ret);
	funlockfile(fp);

	fclose(fp);
	fp = NULL;

	fp = fopen(TEST_FILE_A, "r");
	TEST_ASSERT_NOT_NULL(fp);
	n = fread(buf, 1, sizeof(buf) - 1, fp);
	buf[n] = '\0';
	TEST_ASSERT_EQUAL_STRING("XY", buf);
	fclose(fp);
}


TEST(stdio_unlocked, getc_unlocked_eof_sets_indicator)
{
	FILE *fp;
	int ch;

	fp = fopen(TEST_FILE_A, "w");
	TEST_ASSERT_NOT_NULL(fp);
	fclose(fp);
	fp = NULL;

	/* Empty file */
	fp = fopen(TEST_FILE_A, "r");
	TEST_ASSERT_NOT_NULL(fp);

	flockfile(fp);
	ch = getc_unlocked(fp);
	TEST_ASSERT_EQUAL_INT(EOF, ch);
	TEST_ASSERT_TRUE(feof(fp) != 0);
	funlockfile(fp);

	fclose(fp);
}


TEST(stdio_unlocked, putc_unlocked_returns_char_written)
{
	FILE *fp;
	int ret;

	fp = fopen(TEST_FILE_A, "w");
	TEST_ASSERT_NOT_NULL(fp);

	flockfile(fp);
	ret = putc_unlocked(0x41, fp);
	TEST_ASSERT_EQUAL_INT(0x41, ret);
	funlockfile(fp);

	fclose(fp);
}


TEST_GROUP_RUNNER(stdio_unlocked)
{
	RUN_TEST_CASE(stdio_unlocked, getc_unlocked_reads_chars);
	RUN_TEST_CASE(stdio_unlocked, putc_unlocked_writes_chars);
	RUN_TEST_CASE(stdio_unlocked, getc_unlocked_eof_sets_indicator);
	RUN_TEST_CASE(stdio_unlocked, putc_unlocked_returns_char_written);
}


/* Tests: renameat */
TEST_GROUP(stdio_renameat);

TEST_SETUP(stdio_renameat)
{
	unlink(TEST_FILE_A);
	unlink(TEST_FILE_B);
}

TEST_TEAR_DOWN(stdio_renameat)
{
	unlink(TEST_FILE_A);
	unlink(TEST_FILE_B);
}


TEST(stdio_renameat, renameat_at_fdcwd)
{
	FILE *fp;
	int ret;
	struct stat st;

	fp = fopen(TEST_FILE_A, "w");
	TEST_ASSERT_NOT_NULL(fp);
	fputs("hello", fp);
	fclose(fp);
	fp = NULL;

	errno = 0;
	ret = renameat(AT_FDCWD, TEST_FILE_A, AT_FDCWD, TEST_FILE_B);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, errno);

	/* Old name should not exist */
	TEST_ASSERT_EQUAL_INT(-1, stat(TEST_FILE_A, &st));
	/* New name should exist */
	TEST_ASSERT_EQUAL_INT(0, stat(TEST_FILE_B, &st));
}


TEST(stdio_renameat, renameat_contents_preserved)
{
	FILE *fp;
	int ret;
	char buf[32];
	size_t n;

	fp = fopen(TEST_FILE_A, "w");
	TEST_ASSERT_NOT_NULL(fp);
	fputs("test data", fp);
	fclose(fp);
	fp = NULL;

	ret = renameat(AT_FDCWD, TEST_FILE_A, AT_FDCWD, TEST_FILE_B);
	TEST_ASSERT_EQUAL_INT(0, ret);

	fp = fopen(TEST_FILE_B, "r");
	TEST_ASSERT_NOT_NULL(fp);
	n = fread(buf, 1, sizeof(buf) - 1, fp);
	buf[n] = '\0';
	TEST_ASSERT_EQUAL_STRING("test data", buf);
	fclose(fp);
}


TEST(stdio_renameat, renameat_enoent_source_missing)
{
	int ret;

	errno = 0;
	ret = renameat(AT_FDCWD, "nonexistent_file_xyzzy", AT_FDCWD, TEST_FILE_B);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);
}


TEST(stdio_renameat, renameat_overwrites_existing)
{
	FILE *fp;
	int ret;
	char buf[32];
	size_t n;

	fp = fopen(TEST_FILE_A, "w");
	TEST_ASSERT_NOT_NULL(fp);
	fputs("new content", fp);
	fclose(fp);
	fp = NULL;

	fp = fopen(TEST_FILE_B, "w");
	TEST_ASSERT_NOT_NULL(fp);
	fputs("old content", fp);
	fclose(fp);
	fp = NULL;

	ret = renameat(AT_FDCWD, TEST_FILE_A, AT_FDCWD, TEST_FILE_B);
	TEST_ASSERT_EQUAL_INT(0, ret);

	fp = fopen(TEST_FILE_B, "r");
	TEST_ASSERT_NOT_NULL(fp);
	n = fread(buf, 1, sizeof(buf) - 1, fp);
	buf[n] = '\0';
	TEST_ASSERT_EQUAL_STRING("new content", buf);
	fclose(fp);
}


TEST(stdio_renameat, renameat_with_fd)
{
	FILE *fp;
	int ret;
	int dirfd;
	struct stat st;

	fp = fopen(TEST_FILE_A, "w");
	TEST_ASSERT_NOT_NULL(fp);
	fputs("data", fp);
	fclose(fp);
	fp = NULL;

	dirfd = open(".", O_RDONLY | O_DIRECTORY);
	TEST_ASSERT_GREATER_THAN_INT(-1, dirfd);

	errno = 0;
	ret = renameat(dirfd, TEST_FILE_A, dirfd, TEST_FILE_B);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, errno);

	TEST_ASSERT_EQUAL_INT(-1, stat(TEST_FILE_A, &st));
	TEST_ASSERT_EQUAL_INT(0, stat(TEST_FILE_B, &st));

	close(dirfd);
}


TEST_GROUP_RUNNER(stdio_renameat)
{
	RUN_TEST_CASE(stdio_renameat, renameat_at_fdcwd);
	RUN_TEST_CASE(stdio_renameat, renameat_contents_preserved);
	RUN_TEST_CASE(stdio_renameat, renameat_enoent_source_missing);
	RUN_TEST_CASE(stdio_renameat, renameat_overwrites_existing);
	RUN_TEST_CASE(stdio_renameat, renameat_with_fd);
}


/* Tests: tempnam, tmpnam */
TEST_GROUP(stdio_tmpnam);

TEST_SETUP(stdio_tmpnam)
{
}

TEST_TEAR_DOWN(stdio_tmpnam)
{
}


TEST(stdio_tmpnam, tmpnam_null_returns_string)
{
	char *result;

	result = tmpnam(NULL);
	TEST_ASSERT_NOT_NULL(result);
	TEST_ASSERT_GREATER_THAN_INT(0, (int)strlen(result));
}


TEST(stdio_tmpnam, tmpnam_with_buffer)
{
	char buf[L_tmpnam + 1];
	char *result;

	memset(buf, 0, sizeof(buf));
	result = tmpnam(buf);
	TEST_ASSERT_NOT_NULL(result);
	TEST_ASSERT_TRUE(result == buf);
	TEST_ASSERT_GREATER_THAN_INT(0, (int)strlen(buf));
}


TEST(stdio_tmpnam, tmpnam_unique_names)
{
	char buf1[L_tmpnam + 1];
	char buf2[L_tmpnam + 1];

	TEST_ASSERT_NOT_NULL(tmpnam(buf1));
	TEST_ASSERT_NOT_NULL(tmpnam(buf2));
	/* Each call should generate a different string */
	TEST_ASSERT_TRUE(strcmp(buf1, buf2) != 0);
}


TEST(stdio_tmpnam, tempnam_basic)
{
	char *result;

	result = tempnam(NULL, NULL);
	TEST_ASSERT_NOT_NULL(result);
	TEST_ASSERT_GREATER_THAN_INT(0, (int)strlen(result));
	free(result);
}


TEST(stdio_tmpnam, tempnam_with_dir)
{
	char *result;

	result = tempnam("/tmp", NULL);
	TEST_ASSERT_NOT_NULL(result);
	TEST_ASSERT_GREATER_THAN_INT(0, (int)strlen(result));
	/* Should start with the given directory */
	TEST_ASSERT_EQUAL_INT(0, strncmp(result, "/tmp", 4));
	free(result);
}


TEST(stdio_tmpnam, tempnam_with_prefix)
{
	char *result;
	const char *prefix = "tst";

	result = tempnam("/tmp", prefix);
	TEST_ASSERT_NOT_NULL(result);
	/* The prefix should appear in the filename portion */
	TEST_ASSERT_NOT_NULL(strstr(result, prefix));
	free(result);
}


TEST(stdio_tmpnam, tempnam_unique_names)
{
	char *result1;
	char *result2;

	result1 = tempnam(NULL, "a");
	TEST_ASSERT_NOT_NULL(result1);

	result2 = tempnam(NULL, "a");
	TEST_ASSERT_NOT_NULL(result2);

	TEST_ASSERT_TRUE(strcmp(result1, result2) != 0);

	free(result1);
	free(result2);
}


TEST_GROUP_RUNNER(stdio_tmpnam)
{
	RUN_TEST_CASE(stdio_tmpnam, tmpnam_null_returns_string);
	RUN_TEST_CASE(stdio_tmpnam, tmpnam_with_buffer);
	RUN_TEST_CASE(stdio_tmpnam, tmpnam_unique_names);
	RUN_TEST_CASE(stdio_tmpnam, tempnam_basic);
	RUN_TEST_CASE(stdio_tmpnam, tempnam_with_dir);
	RUN_TEST_CASE(stdio_tmpnam, tempnam_with_prefix);
	RUN_TEST_CASE(stdio_tmpnam, tempnam_unique_names);
}


/* Tests: vdprintf */
TEST_GROUP(stdio_vdprintf);

TEST_SETUP(stdio_vdprintf)
{
	unlink(TEST_FILE_A);
}

TEST_TEAR_DOWN(stdio_vdprintf)
{
	unlink(TEST_FILE_A);
}


static int test_vdprintfHelper(int fd, const char *fmt, ...)
{
	va_list ap;
	int ret;

	va_start(ap, fmt);
	ret = vdprintf(fd, fmt, ap);
	va_end(ap);
	return ret;
}


TEST(stdio_vdprintf, basic_string)
{
	int fd;
	int ret;
	char buf[BUF_SIZE];
	ssize_t n;

	fd = open(TEST_FILE_A, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	TEST_ASSERT_GREATER_THAN_INT(-1, fd);

	ret = test_vdprintfHelper(fd, "hello %s", "world");
	TEST_ASSERT_EQUAL_INT(11, ret);
	close(fd);

	fd = open(TEST_FILE_A, O_RDONLY);
	TEST_ASSERT_GREATER_THAN_INT(-1, fd);
	n = read(fd, buf, sizeof(buf) - 1);
	TEST_ASSERT_EQUAL_INT(11, (int)n);
	buf[n] = '\0';
	TEST_ASSERT_EQUAL_STRING("hello world", buf);
	close(fd);
}


TEST(stdio_vdprintf, format_int)
{
	int fd;
	int ret;
	char buf[BUF_SIZE];
	ssize_t n;

	fd = open(TEST_FILE_A, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	TEST_ASSERT_GREATER_THAN_INT(-1, fd);

	ret = test_vdprintfHelper(fd, "%d+%d=%d", 1, 2, 3);
	TEST_ASSERT_EQUAL_INT(5, ret);
	close(fd);

	fd = open(TEST_FILE_A, O_RDONLY);
	TEST_ASSERT_GREATER_THAN_INT(-1, fd);
	n = read(fd, buf, sizeof(buf) - 1);
	buf[n] = '\0';
	TEST_ASSERT_EQUAL_STRING("1+2=3", buf);
	close(fd);
}


TEST(stdio_vdprintf, returns_char_count)
{
	int fd;
	int ret;

	fd = open(TEST_FILE_A, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	TEST_ASSERT_GREATER_THAN_INT(-1, fd);

	ret = test_vdprintfHelper(fd, "");
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = test_vdprintfHelper(fd, "a");
	TEST_ASSERT_EQUAL_INT(1, ret);

	ret = test_vdprintfHelper(fd, "%05d", 42);
	TEST_ASSERT_EQUAL_INT(5, ret);

	close(fd);
}


TEST(stdio_vdprintf, ebadf_invalid_fd)
{
	int ret;

	errno = 0;
	ret = test_vdprintfHelper(-1, "test");
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


TEST_GROUP_RUNNER(stdio_vdprintf)
{
	RUN_TEST_CASE(stdio_vdprintf, basic_string);
	RUN_TEST_CASE(stdio_vdprintf, format_int);
	RUN_TEST_CASE(stdio_vdprintf, returns_char_count);
	RUN_TEST_CASE(stdio_vdprintf, ebadf_invalid_fd);
}
