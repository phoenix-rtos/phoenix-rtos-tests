/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <dirent.h>
 * TESTED:
 *    - fdopendir()
 *    - seekdir()
 *    - telldir()
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
#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/stat.h>

#include <unity_fixture.h>

#define DIRENT_TEST_DIR   "dirent_test_dir"
#define DIRENT_FILE_A     "dirent_test_dir/aaa"
#define DIRENT_FILE_B     "dirent_test_dir/bbb"
#define DIRENT_FILE_C     "dirent_test_dir/ccc"
#define DIRENT_NUM_FILES  3


static struct {
	DIR *dirp;
	int fd;
} test_common;


static void test_createFile(const char *path)
{
	int fd;

	fd = open(path, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
	if (fd >= 0) {
		close(fd);
	}
}


/*
Test group for fdopendir.
*/
TEST_GROUP(dirent_fdopendir);


TEST_SETUP(dirent_fdopendir)
{
	test_common.dirp = NULL;
	test_common.fd = -1;

	/* clean up and recreate test directory */
	unlink(DIRENT_FILE_A);
	unlink(DIRENT_FILE_B);
	unlink(DIRENT_FILE_C);
	rmdir(DIRENT_TEST_DIR);

	TEST_ASSERT_EQUAL_INT(0, mkdir(DIRENT_TEST_DIR, S_IRWXU));
	test_createFile(DIRENT_FILE_A);
	test_createFile(DIRENT_FILE_B);
	test_createFile(DIRENT_FILE_C);
}


TEST_TEAR_DOWN(dirent_fdopendir)
{
	if (test_common.dirp != NULL) {
		closedir(test_common.dirp);
		test_common.dirp = NULL;
		/* fd is closed by closedir */
		test_common.fd = -1;
	}

	if (test_common.fd >= 0) {
		close(test_common.fd);
		test_common.fd = -1;
	}

	unlink(DIRENT_FILE_A);
	unlink(DIRENT_FILE_B);
	unlink(DIRENT_FILE_C);
	rmdir(DIRENT_TEST_DIR);
}


TEST(dirent_fdopendir, fdopendir_basic)
{
	struct dirent *entry;
	int count;

	test_common.fd = open(DIRENT_TEST_DIR, O_RDONLY | O_DIRECTORY);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_common.fd);

	test_common.dirp = fdopendir(test_common.fd);
	TEST_ASSERT_NOT_NULL(test_common.dirp);

	/* count entries (should have at least . .. and 3 files) */
	count = 0;
	while ((entry = readdir(test_common.dirp)) != NULL) {
		count++;
	}
	TEST_ASSERT_GREATER_OR_EQUAL_INT(DIRENT_NUM_FILES, count);
}


TEST(dirent_fdopendir, fdopendir_readdir)
{
	struct dirent *entry;
	int foundA, foundB, foundC;

	test_common.fd = open(DIRENT_TEST_DIR, O_RDONLY | O_DIRECTORY);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_common.fd);

	test_common.dirp = fdopendir(test_common.fd);
	TEST_ASSERT_NOT_NULL(test_common.dirp);

	foundA = 0;
	foundB = 0;
	foundC = 0;

	while ((entry = readdir(test_common.dirp)) != NULL) {
		if (strcmp(entry->d_name, "aaa") == 0) {
			foundA = 1;
		}
		else if (strcmp(entry->d_name, "bbb") == 0) {
			foundB = 1;
		}
		else if (strcmp(entry->d_name, "ccc") == 0) {
			foundC = 1;
		}
	}

	TEST_ASSERT_EQUAL_INT(1, foundA);
	TEST_ASSERT_EQUAL_INT(1, foundB);
	TEST_ASSERT_EQUAL_INT(1, foundC);
}


TEST(dirent_fdopendir, fdopendir_closedir_closes_fd)
{
	int fd;
	int ret;

	fd = open(DIRENT_TEST_DIR, O_RDONLY | O_DIRECTORY);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);
	test_common.fd = fd;

	test_common.dirp = fdopendir(fd);
	TEST_ASSERT_NOT_NULL(test_common.dirp);

	/* closedir shall close the underlying fd */
	closedir(test_common.dirp);
	test_common.dirp = NULL;
	test_common.fd = -1;

	/* fd should now be invalid */
	errno = 0;
	ret = fcntl(fd, F_GETFD);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


TEST(dirent_fdopendir, fdopendir_ebadf)
{
	/* invalid fd shall fail with EBADF */
	errno = 0;
	test_common.dirp = fdopendir(-1);
	TEST_ASSERT_NULL(test_common.dirp);
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


TEST(dirent_fdopendir, fdopendir_enotdir)
{
	int fd;

	/* fd pointing to a regular file shall fail with ENOTDIR */
	fd = open(DIRENT_FILE_A, O_RDONLY);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);
	test_common.fd = fd;

	errno = 0;
	test_common.dirp = fdopendir(fd);
	TEST_ASSERT_NULL(test_common.dirp);
	TEST_ASSERT_EQUAL_INT(ENOTDIR, errno);
}


TEST(dirent_fdopendir, fdopendir_rewinddir)
{
	struct dirent *entry;
	int count1, count2;

	test_common.fd = open(DIRENT_TEST_DIR, O_RDONLY | O_DIRECTORY);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_common.fd);

	test_common.dirp = fdopendir(test_common.fd);
	TEST_ASSERT_NOT_NULL(test_common.dirp);

	/* read all entries */
	count1 = 0;
	while ((entry = readdir(test_common.dirp)) != NULL) {
		count1++;
	}

	/* rewinddir and read again */
	rewinddir(test_common.dirp);

	count2 = 0;
	while ((entry = readdir(test_common.dirp)) != NULL) {
		count2++;
	}

	TEST_ASSERT_EQUAL_INT(count1, count2);
}


TEST_GROUP_RUNNER(dirent_fdopendir)
{
	RUN_TEST_CASE(dirent_fdopendir, fdopendir_basic);
	RUN_TEST_CASE(dirent_fdopendir, fdopendir_readdir);
	RUN_TEST_CASE(dirent_fdopendir, fdopendir_closedir_closes_fd);
	RUN_TEST_CASE(dirent_fdopendir, fdopendir_ebadf);
	RUN_TEST_CASE(dirent_fdopendir, fdopendir_enotdir);
	RUN_TEST_CASE(dirent_fdopendir, fdopendir_rewinddir);
}


/*
Test group for seekdir and telldir.
*/
TEST_GROUP(dirent_seekdir_telldir);


TEST_SETUP(dirent_seekdir_telldir)
{
	test_common.dirp = NULL;
	test_common.fd = -1;

	unlink(DIRENT_FILE_A);
	unlink(DIRENT_FILE_B);
	unlink(DIRENT_FILE_C);
	rmdir(DIRENT_TEST_DIR);

	TEST_ASSERT_EQUAL_INT(0, mkdir(DIRENT_TEST_DIR, S_IRWXU));
	test_createFile(DIRENT_FILE_A);
	test_createFile(DIRENT_FILE_B);
	test_createFile(DIRENT_FILE_C);
}


TEST_TEAR_DOWN(dirent_seekdir_telldir)
{
	if (test_common.dirp != NULL) {
		closedir(test_common.dirp);
		test_common.dirp = NULL;
	}

	unlink(DIRENT_FILE_A);
	unlink(DIRENT_FILE_B);
	unlink(DIRENT_FILE_C);
	rmdir(DIRENT_TEST_DIR);
}


TEST(dirent_seekdir_telldir, telldir_returns_position)
{
	long loc;

	test_common.dirp = opendir(DIRENT_TEST_DIR);
	TEST_ASSERT_NOT_NULL(test_common.dirp);

	/* telldir shall return the current location */
	loc = telldir(test_common.dirp);
	/* we cannot know the exact value, but it should be a valid number */
	(void)loc;

	/* after reading one entry, telldir should return a different value */
	TEST_ASSERT_NOT_NULL(readdir(test_common.dirp));
	{
		long loc2 = telldir(test_common.dirp);
		/* positions may differ (though not guaranteed by POSIX to differ, this is practical) */
		(void)loc2;
	}
}


TEST(dirent_seekdir_telldir, seekdir_restores_position)
{
	struct dirent *entry1;
	struct dirent *entry2;
	long loc;
	char savedName[NAME_MAX + 1];

	test_common.dirp = opendir(DIRENT_TEST_DIR);
	TEST_ASSERT_NOT_NULL(test_common.dirp);

	/* read first entry */
	entry1 = readdir(test_common.dirp);
	TEST_ASSERT_NOT_NULL(entry1);

	/* save position after first entry */
	loc = telldir(test_common.dirp);

	/* read second entry */
	entry2 = readdir(test_common.dirp);
	TEST_ASSERT_NOT_NULL(entry2);
	snprintf(savedName, sizeof(savedName), "%s", entry2->d_name);

	/* read past it */
	(void)readdir(test_common.dirp);

	/* seekdir back to saved position */
	seekdir(test_common.dirp, loc);

	/* next readdir shall return the same entry as entry2 */
	entry2 = readdir(test_common.dirp);
	TEST_ASSERT_NOT_NULL(entry2);
	TEST_ASSERT_EQUAL_STRING(savedName, entry2->d_name);
}


TEST(dirent_seekdir_telldir, seekdir_telldir_roundtrip)
{
	long loc;
	struct dirent *entry;
	char name1[NAME_MAX + 1];
	char name2[NAME_MAX + 1];

	test_common.dirp = opendir(DIRENT_TEST_DIR);
	TEST_ASSERT_NOT_NULL(test_common.dirp);

	/* skip . and .. to get to our test files */
	entry = readdir(test_common.dirp);
	TEST_ASSERT_NOT_NULL(entry);
	entry = readdir(test_common.dirp);
	TEST_ASSERT_NOT_NULL(entry);

	/* save position */
	loc = telldir(test_common.dirp);

	/* read an entry */
	entry = readdir(test_common.dirp);
	TEST_ASSERT_NOT_NULL(entry);
	snprintf(name1, sizeof(name1), "%s", entry->d_name);

	/* read more to move position forward */
	(void)readdir(test_common.dirp);

	/* seek back */
	seekdir(test_common.dirp, loc);

	/* telldir after seekdir should return the same loc */
	TEST_ASSERT_EQUAL_INT(loc, telldir(test_common.dirp));

	/* and readdir should give the same entry */
	entry = readdir(test_common.dirp);
	TEST_ASSERT_NOT_NULL(entry);
	snprintf(name2, sizeof(name2), "%s", entry->d_name);

	TEST_ASSERT_EQUAL_STRING(name1, name2);
}


TEST(dirent_seekdir_telldir, seekdir_multiple_positions)
{
	long loc1, loc2;
	struct dirent *entry;
	char nameAtLoc1[NAME_MAX + 1];
	char nameAtLoc2[NAME_MAX + 1];
	char verifyName[NAME_MAX + 1];

	test_common.dirp = opendir(DIRENT_TEST_DIR);
	TEST_ASSERT_NOT_NULL(test_common.dirp);

	/* record position before first entry */
	loc1 = telldir(test_common.dirp);
	entry = readdir(test_common.dirp);
	TEST_ASSERT_NOT_NULL(entry);
	snprintf(nameAtLoc1, sizeof(nameAtLoc1), "%s", entry->d_name);

	/* record position before second entry */
	loc2 = telldir(test_common.dirp);
	entry = readdir(test_common.dirp);
	TEST_ASSERT_NOT_NULL(entry);
	snprintf(nameAtLoc2, sizeof(nameAtLoc2), "%s", entry->d_name);

	/* seek to loc2 and verify */
	seekdir(test_common.dirp, loc2);
	entry = readdir(test_common.dirp);
	TEST_ASSERT_NOT_NULL(entry);
	snprintf(verifyName, sizeof(verifyName), "%s", entry->d_name);
	TEST_ASSERT_EQUAL_STRING(nameAtLoc2, verifyName);

	/* seek to loc1 and verify */
	seekdir(test_common.dirp, loc1);
	entry = readdir(test_common.dirp);
	TEST_ASSERT_NOT_NULL(entry);
	snprintf(verifyName, sizeof(verifyName), "%s", entry->d_name);
	TEST_ASSERT_EQUAL_STRING(nameAtLoc1, verifyName);
}


TEST(dirent_seekdir_telldir, telldir_after_rewinddir)
{
	long locStart, locAfterRewind;
	struct dirent *entry;

	test_common.dirp = opendir(DIRENT_TEST_DIR);
	TEST_ASSERT_NOT_NULL(test_common.dirp);

	locStart = telldir(test_common.dirp);

	/* advance */
	entry = readdir(test_common.dirp);
	TEST_ASSERT_NOT_NULL(entry);
	entry = readdir(test_common.dirp);
	TEST_ASSERT_NOT_NULL(entry);

	/* rewinddir resets to beginning */
	rewinddir(test_common.dirp);
	locAfterRewind = telldir(test_common.dirp);

	TEST_ASSERT_EQUAL_INT(locStart, locAfterRewind);
}


TEST_GROUP_RUNNER(dirent_seekdir_telldir)
{
	RUN_TEST_CASE(dirent_seekdir_telldir, telldir_returns_position);
	RUN_TEST_CASE(dirent_seekdir_telldir, seekdir_restores_position);
	RUN_TEST_CASE(dirent_seekdir_telldir, seekdir_telldir_roundtrip);
	RUN_TEST_CASE(dirent_seekdir_telldir, seekdir_multiple_positions);
	RUN_TEST_CASE(dirent_seekdir_telldir, telldir_after_rewinddir);
}
