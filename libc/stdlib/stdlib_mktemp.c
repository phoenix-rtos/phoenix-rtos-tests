/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <stdlib.h>
 * TESTED:
 *    - mkdtemp()
 *    - mkstemp()
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
#include <sys/stat.h>
#include <limits.h>
#include <fcntl.h>

#include <unity_fixture.h>

#define MKTEMP_DIR       "mktemp_testdir"
#define MKTEMP_TEMPLATE  "mktemp_testXXXXXX"
#define MKTEMP_PREFIX    "mktemp_test"
#define MKTEMP_SUFFIX_LEN 6


static struct {
	char tmpl[PATH_MAX];
	char *dirResult;
	int fd;
} test_common;


/*
Test group for mkdtemp.
*/
TEST_GROUP(stdlib_mkdtemp);


TEST_SETUP(stdlib_mkdtemp)
{
	test_common.dirResult = NULL;
	rmdir(MKTEMP_DIR);
}


TEST_TEAR_DOWN(stdlib_mkdtemp)
{
	if (test_common.dirResult != NULL) {
		rmdir(test_common.dirResult);
	}
}


TEST(stdlib_mkdtemp, mkdtemp_basic)
{
	TEST_IGNORE_MESSAGE("no urandom, todo: make it conditional");
	struct stat st;

	strncpy(test_common.tmpl, MKTEMP_TEMPLATE, sizeof(test_common.tmpl) - 1);
	test_common.tmpl[sizeof(test_common.tmpl) - 1] = '\0';

	test_common.dirResult = mkdtemp(test_common.tmpl);
	TEST_ASSERT_NOT_NULL(test_common.dirResult);

	/* return value shall be the same pointer as template */
	TEST_ASSERT_TRUE(test_common.dirResult == test_common.tmpl);

	/* directory shall exist */
	TEST_ASSERT_EQUAL_INT(0, stat(test_common.dirResult, &st));
	TEST_ASSERT_TRUE(S_ISDIR(st.st_mode));

	/* permissions shall be S_IRWXU (0700) */
	TEST_ASSERT_EQUAL_INT(S_IRWXU, st.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO));
}


TEST(stdlib_mkdtemp, mkdtemp_modifies_template)
{
	TEST_IGNORE_MESSAGE("no urandom, todo: make it conditional");
	const char *suffix;

	strncpy(test_common.tmpl, MKTEMP_TEMPLATE, sizeof(test_common.tmpl) - 1);
	test_common.tmpl[sizeof(test_common.tmpl) - 1] = '\0';

	test_common.dirResult = mkdtemp(test_common.tmpl);
	TEST_ASSERT_NOT_NULL(test_common.dirResult);

	/* prefix shall be preserved */
	TEST_ASSERT_EQUAL_INT(0, strncmp(test_common.dirResult, MKTEMP_PREFIX, strlen(MKTEMP_PREFIX)));

	/* suffix shall not be all X's anymore */
	suffix = test_common.dirResult + strlen(MKTEMP_PREFIX);
	TEST_ASSERT_EQUAL_INT(MKTEMP_SUFFIX_LEN, strlen(suffix));
	TEST_ASSERT_FALSE(strcmp(suffix, "XXXXXX") == 0);
}


TEST(stdlib_mkdtemp, mkdtemp_unique)
{
	TEST_IGNORE_MESSAGE("no urandom, todo: make it conditional");
	char tmpl2[PATH_MAX];
	char *dir2;

	strncpy(test_common.tmpl, MKTEMP_TEMPLATE, sizeof(test_common.tmpl) - 1);
	test_common.tmpl[sizeof(test_common.tmpl) - 1] = '\0';
	strncpy(tmpl2, MKTEMP_TEMPLATE, sizeof(tmpl2) - 1);
	tmpl2[sizeof(tmpl2) - 1] = '\0';

	test_common.dirResult = mkdtemp(test_common.tmpl);
	TEST_ASSERT_NOT_NULL(test_common.dirResult);

	dir2 = mkdtemp(tmpl2);
	TEST_ASSERT_NOT_NULL(dir2);

	/* two calls shall produce different names */
	TEST_ASSERT_FALSE(strcmp(test_common.dirResult, dir2) == 0);

	rmdir(dir2);
}


TEST(stdlib_mkdtemp, mkdtemp_einval)
{
	TEST_IGNORE_MESSAGE("no urandom, todo: make it conditional");
	/* template not ending in XXXXXX */
	strncpy(test_common.tmpl, "no_suffix_here", sizeof(test_common.tmpl) - 1);
	test_common.tmpl[sizeof(test_common.tmpl) - 1] = '\0';

	errno = 0;
	TEST_ASSERT_NULL(mkdtemp(test_common.tmpl));
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST(stdlib_mkdtemp, mkdtemp_enoent)
{
	TEST_IGNORE_MESSAGE("no urandom, todo: make it conditional");
	/* non-existing path prefix */
	strncpy(test_common.tmpl, "/nonexistent_dir_xyz/tmpXXXXXX", sizeof(test_common.tmpl) - 1);
	test_common.tmpl[sizeof(test_common.tmpl) - 1] = '\0';

	errno = 0;
	TEST_ASSERT_NULL(mkdtemp(test_common.tmpl));
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);
}


TEST(stdlib_mkdtemp, mkdtemp_enotdir)
{
	TEST_IGNORE_MESSAGE("no urandom, todo: make it conditional");
	int fd;

	/* create a regular file, then try to use it as path prefix */
	fd = open("mkdtemp_notdir_file", O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);
	close(fd);

	strncpy(test_common.tmpl, "mkdtemp_notdir_file/tmpXXXXXX", sizeof(test_common.tmpl) - 1);
	test_common.tmpl[sizeof(test_common.tmpl) - 1] = '\0';

	errno = 0;
	TEST_ASSERT_NULL(mkdtemp(test_common.tmpl));
	TEST_ASSERT_EQUAL_INT(ENOTDIR, errno);

	unlink("mkdtemp_notdir_file");
}


TEST(stdlib_mkdtemp, mkdtemp_enametoolong)
{
	TEST_IGNORE_MESSAGE("no urandom, todo: make it conditional");
	static char longTmpl[PATH_MAX + 16];

	memset(longTmpl, 'a', sizeof(longTmpl));
	/* place XXXXXX at the end */
	memcpy(longTmpl + sizeof(longTmpl) - 7, "XXXXXX", 6);
	longTmpl[sizeof(longTmpl) - 1] = '\0';

	errno = 0;
	TEST_ASSERT_NULL(mkdtemp(longTmpl));
	TEST_ASSERT_EQUAL_INT(ENAMETOOLONG, errno);
}


TEST_GROUP_RUNNER(stdlib_mkdtemp)
{
	RUN_TEST_CASE(stdlib_mkdtemp, mkdtemp_basic);
	RUN_TEST_CASE(stdlib_mkdtemp, mkdtemp_modifies_template);
	RUN_TEST_CASE(stdlib_mkdtemp, mkdtemp_unique);
	RUN_TEST_CASE(stdlib_mkdtemp, mkdtemp_einval);
	RUN_TEST_CASE(stdlib_mkdtemp, mkdtemp_enoent);
	RUN_TEST_CASE(stdlib_mkdtemp, mkdtemp_enotdir);
	RUN_TEST_CASE(stdlib_mkdtemp, mkdtemp_enametoolong);
}


/*
Test group for mkstemp.
*/
TEST_GROUP(stdlib_mkstemp);


TEST_SETUP(stdlib_mkstemp)
{
	test_common.fd = -1;
}


TEST_TEAR_DOWN(stdlib_mkstemp)
{
	if (test_common.fd >= 0) {
		close(test_common.fd);
		test_common.fd = -1;
	}

	unlink(test_common.tmpl);
}


TEST(stdlib_mkstemp, mkstemp_basic)
{
	TEST_IGNORE_MESSAGE("no urandom, todo: make it conditional");
	struct stat st;

	strncpy(test_common.tmpl, MKTEMP_TEMPLATE, sizeof(test_common.tmpl) - 1);
	test_common.tmpl[sizeof(test_common.tmpl) - 1] = '\0';

	test_common.fd = mkstemp(test_common.tmpl);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_common.fd);

	/* file shall exist */
	TEST_ASSERT_EQUAL_INT(0, stat(test_common.tmpl, &st));
	TEST_ASSERT_TRUE(S_ISREG(st.st_mode));

	/* permissions shall be S_IRUSR|S_IWUSR (0600) */
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1654 issue");
#else
	TEST_ASSERT_EQUAL_INT(S_IRUSR | S_IWUSR, st.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO));
#endif
}


TEST(stdlib_mkstemp, mkstemp_rdwr)
{
	TEST_IGNORE_MESSAGE("no urandom, todo: make it conditional");
	const char data[] = "hello";
	char buf[sizeof(data)];
	ssize_t n;

	/* fd shall be open for reading and writing */
	strncpy(test_common.tmpl, MKTEMP_TEMPLATE, sizeof(test_common.tmpl) - 1);
	test_common.tmpl[sizeof(test_common.tmpl) - 1] = '\0';

	test_common.fd = mkstemp(test_common.tmpl);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_common.fd);

	n = write(test_common.fd, data, strlen(data));
	TEST_ASSERT_EQUAL_INT((ssize_t)strlen(data), n);

	TEST_ASSERT_EQUAL_INT(0, lseek(test_common.fd, 0, SEEK_SET));

	memset(buf, 0, sizeof(buf));
	n = read(test_common.fd, buf, strlen(data));
	TEST_ASSERT_EQUAL_INT((ssize_t)strlen(data), n);
	TEST_ASSERT_EQUAL_MEMORY(data, buf, strlen(data));
}


TEST(stdlib_mkstemp, mkstemp_modifies_template)
{
	TEST_IGNORE_MESSAGE("no urandom, todo: make it conditional");
	const char *suffix;

	strncpy(test_common.tmpl, MKTEMP_TEMPLATE, sizeof(test_common.tmpl) - 1);
	test_common.tmpl[sizeof(test_common.tmpl) - 1] = '\0';

	test_common.fd = mkstemp(test_common.tmpl);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_common.fd);

	/* prefix preserved */
	TEST_ASSERT_EQUAL_INT(0, strncmp(test_common.tmpl, MKTEMP_PREFIX, strlen(MKTEMP_PREFIX)));

	/* suffix replaced */
	suffix = test_common.tmpl + strlen(MKTEMP_PREFIX);
	TEST_ASSERT_EQUAL_INT(MKTEMP_SUFFIX_LEN, strlen(suffix));
	TEST_ASSERT_FALSE(strcmp(suffix, "XXXXXX") == 0);
}


TEST(stdlib_mkstemp, mkstemp_unique)
{
	TEST_IGNORE_MESSAGE("no urandom, todo: make it conditional");
	char tmpl2[PATH_MAX];
	int fd2;

	strncpy(test_common.tmpl, MKTEMP_TEMPLATE, sizeof(test_common.tmpl) - 1);
	test_common.tmpl[sizeof(test_common.tmpl) - 1] = '\0';
	strncpy(tmpl2, MKTEMP_TEMPLATE, sizeof(tmpl2) - 1);
	tmpl2[sizeof(tmpl2) - 1] = '\0';

	test_common.fd = mkstemp(test_common.tmpl);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_common.fd);

	fd2 = mkstemp(tmpl2);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd2);

	/* names shall differ */
	TEST_ASSERT_FALSE(strcmp(test_common.tmpl, tmpl2) == 0);

	close(fd2);
	unlink(tmpl2);
}


TEST(stdlib_mkstemp, mkstemp_einval)
{
	TEST_IGNORE_MESSAGE("no urandom, todo: make it conditional");
	/* template not ending in XXXXXX */
	strncpy(test_common.tmpl, "no_suffix_here", sizeof(test_common.tmpl) - 1);
	test_common.tmpl[sizeof(test_common.tmpl) - 1] = '\0';

	errno = 0;
	test_common.fd = mkstemp(test_common.tmpl);
	TEST_ASSERT_EQUAL_INT(-1, test_common.fd);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
	test_common.fd = -1;
}


TEST(stdlib_mkstemp, mkstemp_enoent)
{
	TEST_IGNORE_MESSAGE("no urandom, todo: make it conditional");
	/* non-existing path prefix */
	strncpy(test_common.tmpl, "/nonexistent_dir_xyz/tmpXXXXXX", sizeof(test_common.tmpl) - 1);
	test_common.tmpl[sizeof(test_common.tmpl) - 1] = '\0';

	errno = 0;
	test_common.fd = mkstemp(test_common.tmpl);
	TEST_ASSERT_EQUAL_INT(-1, test_common.fd);
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);
	test_common.fd = -1;
}


TEST(stdlib_mkstemp, mkstemp_enotdir)
{
	TEST_IGNORE_MESSAGE("no urandom, todo: make it conditional");
	int fd;

	fd = open("mkstemp_notdir_file", O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);
	close(fd);

	strncpy(test_common.tmpl, "mkstemp_notdir_file/tmpXXXXXX", sizeof(test_common.tmpl) - 1);
	test_common.tmpl[sizeof(test_common.tmpl) - 1] = '\0';

	errno = 0;
	test_common.fd = mkstemp(test_common.tmpl);
	TEST_ASSERT_EQUAL_INT(-1, test_common.fd);
	TEST_ASSERT_EQUAL_INT(ENOTDIR, errno);
	test_common.fd = -1;

	unlink("mkstemp_notdir_file");
}


TEST(stdlib_mkstemp, mkstemp_enametoolong)
{
	TEST_IGNORE_MESSAGE("no urandom, todo: make it conditional");
	static char longTmpl[PATH_MAX + 16];

	memset(longTmpl, 'a', sizeof(longTmpl));
	memcpy(longTmpl + sizeof(longTmpl) - 7, "XXXXXX", 6);
	longTmpl[sizeof(longTmpl) - 1] = '\0';

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, mkstemp(longTmpl));
	TEST_ASSERT_EQUAL_INT(ENAMETOOLONG, errno);
}


TEST_GROUP_RUNNER(stdlib_mkstemp)
{
	RUN_TEST_CASE(stdlib_mkstemp, mkstemp_basic);
	RUN_TEST_CASE(stdlib_mkstemp, mkstemp_rdwr);
	RUN_TEST_CASE(stdlib_mkstemp, mkstemp_modifies_template);
	RUN_TEST_CASE(stdlib_mkstemp, mkstemp_unique);
	RUN_TEST_CASE(stdlib_mkstemp, mkstemp_einval);
	RUN_TEST_CASE(stdlib_mkstemp, mkstemp_enoent);
	RUN_TEST_CASE(stdlib_mkstemp, mkstemp_enotdir);
	RUN_TEST_CASE(stdlib_mkstemp, mkstemp_enametoolong);
}
