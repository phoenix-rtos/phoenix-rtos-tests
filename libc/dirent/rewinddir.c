/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - dirent.h
 * TESTED:
 *    - rewinddir()
 *
 * Copyright 2023-2026 Phoenix Systems
 * Authors: Arkadiusz Kozlowski, Lukasz Kruszynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <stdio.h>
#include <unistd.h>
#include <dirent.h>

#include <string.h>
#include <fcntl.h>
#include <stdlib.h>

#include <unity_fixture.h>
#include "dirent_helper_functions.h"

#include "common.h"

#define MAIN_DIR               "test_rewinddir"
#define MAIN_DIR_INIT_CONTENTS 4

typedef struct {
	DIR *dp1;
	DIR *dp2;
} test_ctx_t;

static test_ctx_t test_ctx;

TEST_GROUP(dirent_rewinddir);

TEST_SETUP(dirent_rewinddir)
{
	memset(&test_ctx, 0, sizeof(test_ctx));
	TEST_MKDIR_ASSERTED(MAIN_DIR, S_IRWXU);
	TEST_MKDIR_ASSERTED(MAIN_DIR "/dir1", S_IRUSR);
	TEST_MKDIR_ASSERTED(MAIN_DIR "/dir2", S_IRWXU);
	TEST_MKDIR_ASSERTED(MAIN_DIR "/dir2/nestDir", S_IRUSR);
}


TEST_TEAR_DOWN(dirent_rewinddir)
{
	if (test_ctx.dp1 != NULL) {
		closedir(test_ctx.dp1);
	}
	if (test_ctx.dp2 != NULL) {
		closedir(test_ctx.dp2);
	}
	rmdir(MAIN_DIR "/dir2/nestDir");
	rmdir(MAIN_DIR "/dir1");
	rmdir(MAIN_DIR "/dir2");
	rmdir(MAIN_DIR);
}


TEST(dirent_rewinddir, rewinddir_basic)
{
	struct stat bufBefore, bufAfter;
	test_ctx.dp1 = TEST_OPENDIR_ASSERTED(MAIN_DIR);
	DIR *dp = test_ctx.dp1;
	int counter1, counter2;
	counter1 = counter2 = 0;

	TEST_ASSERT_NOT_NULL(dp);

	TEST_ASSERT_EQUAL_INT(0, stat(MAIN_DIR, &bufBefore));

	while (readdir(dp)) {
		counter1++;
	}

	rewinddir(dp);

	while (readdir(dp)) {
		counter2++;
	}

	TEST_ASSERT_EQUAL_INT(counter1, counter2);
	TEST_ASSERT_EQUAL_INT(MAIN_DIR_INIT_CONTENTS, counter1);

	rewinddir(dp);

	TEST_ASSERT_EQUAL_INT(0, stat(MAIN_DIR, &bufAfter));

	TEST_ASSERT_EQUAL(bufBefore.st_blksize, bufAfter.st_blksize);
	TEST_ASSERT_EQUAL(bufBefore.st_blocks, bufAfter.st_blocks);
	TEST_ASSERT_EQUAL(bufBefore.st_dev, bufAfter.st_dev);
	TEST_ASSERT_EQUAL(bufBefore.st_ino, bufAfter.st_ino);
	TEST_ASSERT_EQUAL(bufBefore.st_mode, bufAfter.st_mode);
}


TEST(dirent_rewinddir, directory_contents_change)
{
	test_ctx.dp1 = TEST_OPENDIR_ASSERTED(MAIN_DIR);
	DIR *dp = test_ctx.dp1;
	int counter1 = 0, counter2 = 0, counter3 = 0;
	int fd = -1;

	if (TEST_PROTECT()) {
		while (readdir(dp)) {
			counter1++;
		}

		TEST_ASSERT_EQUAL_INT(MAIN_DIR_INIT_CONTENTS, counter1);
		TEST_ASSERT_EQUAL_INT(0, mkdir(MAIN_DIR "/newdir", S_IRUSR));

		fd = creat(MAIN_DIR "/textfile.txt", S_IRUSR | S_IWUSR);
		TEST_ASSERT_NOT_EQUAL_INT(-1, fd);
		close(fd);
		TEST_ASSERT_EQUAL_INT(0, link(MAIN_DIR "/textfile.txt", MAIN_DIR "/hardlink"));
		TEST_ASSERT_EQUAL_INT(0, symlink(MAIN_DIR "/newdir", MAIN_DIR "/symlink"));

		rewinddir(dp);

		while (readdir(dp)) {
			counter2++;
		}

		TEST_ASSERT_EQUAL_INT(MAIN_DIR_INIT_CONTENTS + 4, counter2);

		rmdir(MAIN_DIR "/newdir");
		unlink(MAIN_DIR "/hardlink");
		unlink(MAIN_DIR "/symlink");
		remove(MAIN_DIR "/textfile.txt");
		rewinddir(dp);

		while (readdir(dp)) {
			counter3++;
		}

		TEST_ASSERT_EQUAL_INT(MAIN_DIR_INIT_CONTENTS, counter3);
	}
	/* Ensure a cleanup, ignore fails */
	rmdir(MAIN_DIR "/newdir");
	unlink(MAIN_DIR "/hardlink");
	unlink(MAIN_DIR "/symlink");
	remove(MAIN_DIR "/textfile.txt");
}

TEST(dirent_rewinddir, rewinddir_mid_stream)
{
	test_ctx.dp1 = TEST_OPENDIR_ASSERTED(MAIN_DIR);
	DIR *dp = test_ctx.dp1;
	struct dirent *info;
	char first_entry[NAME_MAX + 1];

	info = readdir(dp);
	TEST_ASSERT_NOT_NULL(info);
	snprintf(first_entry, sizeof(first_entry), "%s", info->d_name);

	TEST_ASSERT_NOT_NULL(readdir(dp));

	errno = 0;
	rewinddir(dp);
	TEST_ASSERT_EQUAL_INT(0, errno);

	info = readdir(dp);
	TEST_ASSERT_NOT_NULL(info);
	TEST_ASSERT_EQUAL_STRING(first_entry, info->d_name);
}


TEST(dirent_rewinddir, rewinddir_multiple_consecutive)
{
	test_ctx.dp1 = TEST_OPENDIR_ASSERTED(MAIN_DIR);
	DIR *dp = test_ctx.dp1;
	struct dirent *info;
	char first_entry[NAME_MAX + 1];

	info = readdir(dp);
	TEST_ASSERT_NOT_NULL(info);
	snprintf(first_entry, sizeof(first_entry), "%s", info->d_name);

	TEST_ASSERT_NOT_NULL(readdir(dp));
	TEST_ASSERT_NOT_NULL(readdir(dp));

	for (int i = 0; i < 10; i++) {
		rewinddir(dp);
	}

	info = readdir(dp);
	TEST_ASSERT_NOT_NULL(info);
	TEST_ASSERT_EQUAL_STRING(first_entry, info->d_name);
}


TEST(dirent_rewinddir, rewinddir_independent_streams)
{
	char seq1[MAIN_DIR_INIT_CONTENTS][NAME_MAX + 1];
	char seq2[MAIN_DIR_INIT_CONTENTS][NAME_MAX + 1];
	struct dirent *entry;
	int n1 = 0;
	int n2 = 0;
	int i, k;

	test_ctx.dp1 = TEST_OPENDIR_ASSERTED(MAIN_DIR);
	test_ctx.dp2 = TEST_OPENDIR_ASSERTED(MAIN_DIR);
	DIR *dp1 = test_ctx.dp1;
	DIR *dp2 = test_ctx.dp2;

	TEST_ASSERT_NOT_NULL(dp1);
	TEST_ASSERT_NOT_NULL(dp2);

	errno = 0;
	entry = readdir(dp1);
	while (entry != NULL) {
		TEST_ASSERT_LESS_THAN_INT(MAIN_DIR_INIT_CONTENTS, n1);
		snprintf(seq1[n1], NAME_MAX + 1, "%s", entry->d_name);
		n1++;
		errno = 0;
		entry = readdir(dp1);
	}
	TEST_ASSERT_EQUAL_INT(0, errno);

	errno = 0;
	entry = readdir(dp2);
	while (entry != NULL) {
		TEST_ASSERT_LESS_THAN_INT(MAIN_DIR_INIT_CONTENTS, n2);
		snprintf(seq2[n2], NAME_MAX + 1, "%s", entry->d_name);
		n2++;
		errno = 0;
		entry = readdir(dp2);
	}
	TEST_ASSERT_EQUAL_INT(0, errno);

	TEST_ASSERT_EQUAL_INT(MAIN_DIR_INIT_CONTENTS, n1);
	TEST_ASSERT_EQUAL_INT(MAIN_DIR_INIT_CONTENTS, n2);

	rewinddir(dp1);
	rewinddir(dp2);

	/* Position dp2 at a known mid-stream offset just before the operation
	 * under test. */
	k = n2 / 2;
	for (i = 0; i < k; i++) {
		entry = readdir(dp2);
		TEST_ASSERT_NOT_NULL(entry);
		TEST_ASSERT_EQUAL_STRING(seq2[i], entry->d_name);
	}

	rewinddir(dp1);

	/* dp2 must still be exactly at offset k and yield its full unchanged
	 * tail followed by end-of-directory. */
	for (i = k; i < n2; i++) {
		errno = 0;
		entry = readdir(dp2);
		TEST_ASSERT_NOT_NULL(entry);
		TEST_ASSERT_EQUAL_STRING(seq2[i], entry->d_name);
	}
	errno = 0;
	TEST_ASSERT_NULL(readdir(dp2));
	TEST_ASSERT_EQUAL_INT(0, errno);

	/* dp1 itself was actually rewound to the start. */
	errno = 0;
	entry = readdir(dp1);
	TEST_ASSERT_NOT_NULL(entry);
	TEST_ASSERT_EQUAL_STRING(seq1[0], entry->d_name);

	rewinddir(dp1);
	rewinddir(dp2);

	/* Position dp1 at a known mid-stream offset just before the operation
	 * under test. */
	k = n1 / 2;
	for (i = 0; i < k; i++) {
		entry = readdir(dp1);
		TEST_ASSERT_NOT_NULL(entry);
		TEST_ASSERT_EQUAL_STRING(seq1[i], entry->d_name);
	}

	rewinddir(dp2);

	/* dp1 must still be exactly at offset k and yield its full unchanged
	 * tail followed by end-of-directory. */
	for (i = k; i < n1; i++) {
		errno = 0;
		entry = readdir(dp1);
		TEST_ASSERT_NOT_NULL(entry);
		TEST_ASSERT_EQUAL_STRING(seq1[i], entry->d_name);
	}
	errno = 0;
	TEST_ASSERT_NULL(readdir(dp1));
	TEST_ASSERT_EQUAL_INT(0, errno);

	errno = 0;
	entry = readdir(dp2);
	TEST_ASSERT_NOT_NULL(entry);
	TEST_ASSERT_EQUAL_STRING(seq2[0], entry->d_name);
}


TEST(dirent_rewinddir, rewind_empty_dir)
{
	int count1 = 0, count2 = 0;
	int dir_created = 0;

	if (TEST_PROTECT()) {
		TEST_MKDIR_ASSERTED(MAIN_DIR "/emptydir", 0700);
		dir_created = 1;

		test_ctx.dp1 = TEST_OPENDIR_ASSERTED(MAIN_DIR "/emptydir");
		DIR *dp = test_ctx.dp1;

		while (readdir(dp) != NULL) {
			count1++;
		}

		rewinddir(dp);

		while (readdir(dp) != NULL) {
			count2++;
		}

		TEST_ASSERT_EQUAL_INT(count1, count2);
		TEST_ASSERT_EQUAL_INT(2, count1);
	}

	if (test_ctx.dp1 != NULL) {
		closedir(test_ctx.dp1);
		test_ctx.dp1 = NULL;
	}

	if (dir_created) {
		rmdir(MAIN_DIR "/emptydir");
	}
}


TEST(dirent_rewinddir, rewind_large_dir_pagination)
{
	const int NUM_FILES = 40;
	char path[PATH_MAX];
	int dir_created = 0;
	int files_created = 0;
	DIR *dp = NULL;
	struct dirent *info = NULL;
	char first_entry[NAME_MAX + 1];
	int i;
	int fd;

	memset(path, 0, sizeof(path));
	memset(first_entry, 0, sizeof(first_entry));

	if (TEST_PROTECT()) {
		TEST_MKDIR_ASSERTED(MAIN_DIR "/pagedir", 0700);
		dir_created = 1;

		for (i = 0; i < NUM_FILES; i++) {
			snprintf(path, PATH_MAX, MAIN_DIR "/pagedir/file_%d.txt", i);
			fd = creat(path, S_IRUSR);
			TEST_ASSERT_NOT_EQUAL_INT(-1, fd);
			close(fd);
			files_created++;
		}

		test_ctx.dp1 = TEST_OPENDIR_ASSERTED(MAIN_DIR "/pagedir");
		dp = test_ctx.dp1;

		info = readdir(dp);
		TEST_ASSERT_NOT_NULL(info);
		snprintf(first_entry, sizeof(first_entry), "%s", info->d_name);

		while (readdir(dp) != NULL) { }

		rewinddir(dp);

		info = readdir(dp);
		TEST_ASSERT_NOT_NULL(info);
		TEST_ASSERT_EQUAL_STRING(first_entry, info->d_name);
	}

	if (test_ctx.dp1 != NULL) {
		closedir(test_ctx.dp1);
		test_ctx.dp1 = NULL;
	}

	if (dir_created) {
		for (i = 0; i < files_created; i++) {
			snprintf(path, PATH_MAX, MAIN_DIR "/pagedir/file_%d.txt", i);
			remove(path);
		}
		rmdir(MAIN_DIR "/pagedir");
	}
}


TEST(dirent_rewinddir, rewind_unlinked_dir)
{
	/* issue #1663 https://github.com/phoenix-rtos/phoenix-rtos-project/issues/1663 */
	TEST_IGNORE_MESSAGE("#1663 issue");
	DIR *dp = NULL;
	void *ret = NULL;
	int dir_created = 0;

	if (TEST_PROTECT()) {
		TEST_MKDIR_ASSERTED(MAIN_DIR "/ghostdir", 0700);
		dir_created = 1;

		test_ctx.dp1 = TEST_OPENDIR_ASSERTED(MAIN_DIR "/ghostdir");
		dp = test_ctx.dp1;

		TEST_ASSERT_EQUAL_INT(0, rmdir(MAIN_DIR "/ghostdir"));
		dir_created = 0;

		errno = 0;
		while (readdir(dp) != NULL) { }
		TEST_ASSERT_EQUAL_INT(0, errno);

		rewinddir(dp);

		ret = readdir(dp);
		/* POSIX mandates . and .. are removed on rmdir, so this must be NULL */
		TEST_ASSERT_NULL(ret);
	}

	if (dir_created) {
		rmdir(MAIN_DIR "/ghostdir");
	}
}


TEST_GROUP_RUNNER(dirent_rewinddir)
{
	RUN_TEST_CASE(dirent_rewinddir, rewinddir_basic);
	RUN_TEST_CASE(dirent_rewinddir, directory_contents_change);
	RUN_TEST_CASE(dirent_rewinddir, rewinddir_mid_stream);
	RUN_TEST_CASE(dirent_rewinddir, rewinddir_multiple_consecutive);
	RUN_TEST_CASE(dirent_rewinddir, rewinddir_independent_streams);
	RUN_TEST_CASE(dirent_rewinddir, rewind_empty_dir);
	RUN_TEST_CASE(dirent_rewinddir, rewind_large_dir_pagination);
	RUN_TEST_CASE(dirent_rewinddir, rewind_unlinked_dir);
}
