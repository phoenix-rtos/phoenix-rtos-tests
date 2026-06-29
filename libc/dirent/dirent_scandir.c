/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <dirent.h>
 * TESTED:
 *    - scandir()
 *    - alphasort()
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
#include <sys/stat.h>

#include <unity_fixture.h>

#define SCANDIR_TEST_DIR     "scandir_test_dir"
#define SCANDIR_FILE_APPLE   "scandir_test_dir/apple"
#define SCANDIR_FILE_BANANA  "scandir_test_dir/banana"
#define SCANDIR_FILE_CHERRY  "scandir_test_dir/cherry"
#define SCANDIR_NUM_FILES    3
#define SCANDIR_SUBDIR       "scandir_test_dir/subdir"

#ifndef __phoenix__

static struct {
	struct dirent **namelist;
	int count;
} test_common;


static void test_createFile(const char *path)
{
	int fd;

	fd = open(path, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
	if (fd >= 0) {
		close(fd);
	}
}


static void test_freeNamelist(void)
{
	int i;

	if (test_common.namelist != NULL) {
		for (i = 0; i < test_common.count; i++) {
			free(test_common.namelist[i]);
		}
		free(test_common.namelist);
		test_common.namelist = NULL;
		test_common.count = 0;
	}
}


TEST_GROUP(dirent_scandir);


TEST_SETUP(dirent_scandir)
{
	(void)unlink(SCANDIR_FILE_APPLE);
	(void)unlink(SCANDIR_FILE_BANANA);
	(void)unlink(SCANDIR_FILE_CHERRY);
	(void)rmdir(SCANDIR_SUBDIR);
	(void)rmdir(SCANDIR_TEST_DIR);

	(void)mkdir(SCANDIR_TEST_DIR, 0755);
	test_common.namelist = NULL;
	test_common.count = 0;
}


TEST_TEAR_DOWN(dirent_scandir)
{
	test_freeNamelist();
	(void)unlink(SCANDIR_FILE_APPLE);
	(void)unlink(SCANDIR_FILE_BANANA);
	(void)unlink(SCANDIR_FILE_CHERRY);
	(void)rmdir(SCANDIR_SUBDIR);
	(void)rmdir(SCANDIR_TEST_DIR);
}


/* Filter that selects only regular entries (excludes "." and "..") */
static int test_filterNoDots(const struct dirent *entry)
{
	if ((strcmp(entry->d_name, ".") == 0) || (strcmp(entry->d_name, "..") == 0)) {
		return 0;
	}
	return 1;
}


/* scandir: shall return number of entries in the array on success */
TEST(dirent_scandir, returns_entry_count)
{
	test_createFile(SCANDIR_FILE_APPLE);
	test_createFile(SCANDIR_FILE_BANANA);
	test_createFile(SCANDIR_FILE_CHERRY);

	test_common.count = scandir(SCANDIR_TEST_DIR, &test_common.namelist, test_filterNoDots, alphasort);
	TEST_ASSERT_EQUAL_INT(SCANDIR_NUM_FILES, test_common.count);
	TEST_ASSERT_NOT_NULL(test_common.namelist);
}


/* scandir: entries sorted with alphasort shall be in alphabetical order */
TEST(dirent_scandir, alphasort_order)
{
	test_createFile(SCANDIR_FILE_CHERRY);
	test_createFile(SCANDIR_FILE_APPLE);
	test_createFile(SCANDIR_FILE_BANANA);

	test_common.count = scandir(SCANDIR_TEST_DIR, &test_common.namelist, test_filterNoDots, alphasort);
	TEST_ASSERT_EQUAL_INT(SCANDIR_NUM_FILES, test_common.count);
	TEST_ASSERT_EQUAL_STRING("apple", test_common.namelist[0]->d_name);
	TEST_ASSERT_EQUAL_STRING("banana", test_common.namelist[1]->d_name);
	TEST_ASSERT_EQUAL_STRING("cherry", test_common.namelist[2]->d_name);
}


/* scandir: NULL sel shall select all entries (including . and ..) */
TEST(dirent_scandir, null_filter_selects_all)
{
	test_createFile(SCANDIR_FILE_APPLE);

	test_common.count = scandir(SCANDIR_TEST_DIR, &test_common.namelist, NULL, alphasort);
	/* At minimum: ".", "..", "apple" */
	TEST_ASSERT_TRUE(test_common.count >= 3);
}


/* scandir: filter that rejects all entries returns 0 */
static int test_filterRejectAll(const struct dirent *entry)
{
	(void)entry;
	return 0;
}


TEST(dirent_scandir, filter_rejects_all)
{
	test_createFile(SCANDIR_FILE_APPLE);
	test_createFile(SCANDIR_FILE_BANANA);

	test_common.count = scandir(SCANDIR_TEST_DIR, &test_common.namelist, test_filterRejectAll, alphasort);
	TEST_ASSERT_EQUAL_INT(0, test_common.count);
}


/* scandir: empty directory returns 0 with filter excluding dots */
TEST(dirent_scandir, empty_dir_no_files)
{
	test_common.count = scandir(SCANDIR_TEST_DIR, &test_common.namelist, test_filterNoDots, alphasort);
	TEST_ASSERT_EQUAL_INT(0, test_common.count);
}


/* scandir: NULL compar — entries are still returned (order is unspecified) */
TEST(dirent_scandir, null_compar)
{
	test_createFile(SCANDIR_FILE_APPLE);
	test_createFile(SCANDIR_FILE_BANANA);

	test_common.count = scandir(SCANDIR_TEST_DIR, &test_common.namelist, test_filterNoDots, NULL);
	TEST_ASSERT_EQUAL_INT(2, test_common.count);
	TEST_ASSERT_NOT_NULL(test_common.namelist);
}


/* scandir: shall include subdirectories in results */
TEST(dirent_scandir, includes_subdirectories)
{
	int ret;
	int found = 0;
	int i;

	ret = mkdir(SCANDIR_SUBDIR, 0755);
	TEST_ASSERT_EQUAL_INT(0, ret);

	test_common.count = scandir(SCANDIR_TEST_DIR, &test_common.namelist, test_filterNoDots, alphasort);
	TEST_ASSERT_TRUE(test_common.count >= 1);

	for (i = 0; i < test_common.count; i++) {
		if (strcmp(test_common.namelist[i]->d_name, "subdir") == 0) {
			found = 1;
			break;
		}
	}
	TEST_ASSERT_EQUAL_INT(1, found);
}


/* scandir: ENOENT when directory does not exist */
TEST(dirent_scandir, enoent_nonexistent_dir)
{
	errno = 0;
	test_common.count = scandir("/tmp/scandir_nonexistent_dir_xyz", &test_common.namelist, NULL, NULL);
	TEST_ASSERT_EQUAL_INT(-1, test_common.count);
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);
}


/* scandir: ENOTDIR when path is a regular file */
TEST(dirent_scandir, enotdir_regular_file)
{
	test_createFile(SCANDIR_FILE_APPLE);

	errno = 0;
	test_common.count = scandir(SCANDIR_FILE_APPLE, &test_common.namelist, NULL, NULL);
	TEST_ASSERT_EQUAL_INT(-1, test_common.count);
	TEST_ASSERT_EQUAL_INT(ENOTDIR, errno);
}


/* scandir: ENOENT for empty string path */
TEST(dirent_scandir, enoent_empty_string)
{
	errno = 0;
	test_common.count = scandir("", &test_common.namelist, NULL, NULL);
	TEST_ASSERT_EQUAL_INT(-1, test_common.count);
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);
}


/* alphasort: returns negative when first < second */
TEST(dirent_scandir, alphasort_negative)
{
	struct dirent d1;
	struct dirent d2;
	const struct dirent *p1 = &d1;
	const struct dirent *p2 = &d2;
	int res;

	strncpy(d1.d_name, "aaa", sizeof(d1.d_name));
	strncpy(d2.d_name, "bbb", sizeof(d2.d_name));

	res = alphasort(&p1, &p2);
	TEST_ASSERT_TRUE(res < 0);
}


/* alphasort: returns zero for equal names */
TEST(dirent_scandir, alphasort_zero)
{
	struct dirent d1;
	struct dirent d2;
	const struct dirent *p1 = &d1;
	const struct dirent *p2 = &d2;
	int res;

	strncpy(d1.d_name, "same", sizeof(d1.d_name));
	strncpy(d2.d_name, "same", sizeof(d2.d_name));

	res = alphasort(&p1, &p2);
	TEST_ASSERT_EQUAL_INT(0, res);
}


/* alphasort: returns positive when first > second */
TEST(dirent_scandir, alphasort_positive)
{
	struct dirent d1;
	struct dirent d2;
	const struct dirent *p1 = &d1;
	const struct dirent *p2 = &d2;
	int res;

	strncpy(d1.d_name, "zzz", sizeof(d1.d_name));
	strncpy(d2.d_name, "aaa", sizeof(d2.d_name));

	res = alphasort(&p1, &p2);
	TEST_ASSERT_TRUE(res > 0);
}


TEST_GROUP_RUNNER(dirent_scandir)
{
	RUN_TEST_CASE(dirent_scandir, returns_entry_count);
	RUN_TEST_CASE(dirent_scandir, alphasort_order);
	RUN_TEST_CASE(dirent_scandir, null_filter_selects_all);
	RUN_TEST_CASE(dirent_scandir, filter_rejects_all);
	RUN_TEST_CASE(dirent_scandir, empty_dir_no_files);
	RUN_TEST_CASE(dirent_scandir, null_compar);
	RUN_TEST_CASE(dirent_scandir, includes_subdirectories);
	RUN_TEST_CASE(dirent_scandir, enoent_nonexistent_dir);
	RUN_TEST_CASE(dirent_scandir, enotdir_regular_file);
	RUN_TEST_CASE(dirent_scandir, enoent_empty_string);
	RUN_TEST_CASE(dirent_scandir, alphasort_negative);
	RUN_TEST_CASE(dirent_scandir, alphasort_zero);
	RUN_TEST_CASE(dirent_scandir, alphasort_positive);
}

#else
TEST_GROUP_UNIMPLEMENTED(dirent_scandir, "scandir not implemented")
#endif
