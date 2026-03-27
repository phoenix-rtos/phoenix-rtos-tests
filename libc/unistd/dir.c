/*
 * Phoenix-RTOS
 *
 * Tests for *dir functions behaviour
 *
 * Copyright 2026 Phoenix Systems
 * Author: Michal Lach
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/msg.h>

#include "unity.h"
#include "unity_fixture.h"

#define TEST_DIR      "/test_dir"
#define TEST_FILENAME "/test_file"


TEST_GROUP(dirent);


TEST_SETUP(dirent)
{
	TEST_ASSERT_EQUAL_INT(0, mkdir(TEST_DIR, 0666));

	FILE *empty = fopen(TEST_FILENAME, "w+");
	TEST_ASSERT_NOT_NULL(empty);
	fclose(empty);
}


TEST_TEAR_DOWN(dirent)
{
	TEST_ASSERT_EQUAL_INT(0, rmdir(TEST_DIR));
	TEST_ASSERT_EQUAL_INT(0, remove(TEST_FILENAME));
}


TEST_GROUP_RUNNER(dirent)
{
	RUN_TEST_CASE(dirent, readdir_enoent);
	RUN_TEST_CASE(dirent, opendir_enodir);
	RUN_TEST_CASE(dirent, opendir_retainpos);
}


TEST(dirent, readdir_enoent)
{
	int saved;

	DIR *d = opendir(TEST_DIR);
	TEST_ASSERT_NOT_NULL(d);
	/* clang-format off */
	errno = 0;
	while (readdir(d) != NULL);
	/* clang-format on */
	saved = errno;

	TEST_ASSERT_EQUAL_INT(0, saved);
	TEST_ASSERT_EQUAL_INT(0, closedir(d));
}

TEST(dirent, opendir_enodir)
{
	int saved, fd;
	DIR *d;

	errno = 0;
	d = opendir(TEST_FILENAME);
	saved = errno;

	TEST_ASSERT_EQUAL_INT(ENOTDIR, saved);
	TEST_ASSERT_NULL(d);

	fd = open(TEST_FILENAME, 0);
	TEST_ASSERT_NOT_EQUAL_INT(-1, fd);

	d = fdopendir(fd);
	TEST_ASSERT_EQUAL_INT(ENOTDIR, saved);
	TEST_ASSERT_NULL(d);
}

TEST(dirent, opendir_retainpos)
{
	int fd;
	struct stat statbuf;
	struct dirent *dirbuf, *dirptr;
	DIR *dir;

	fd = open(TEST_DIR, O_RDONLY);

	TEST_ASSERT_NOT_EQUAL(-1, fd);
	TEST_ASSERT_NOT_EQUAL(-1, fstat(fd, &statbuf));

	dirbuf = calloc(1, sizeof(struct dirent) + NAME_MAX + 1);
	TEST_ASSERT_NOT_NULL(dirbuf);

	msg_t msg = {
		.type = mtReaddir,
		.oid = { .port = statbuf.st_dev, .id = statbuf.st_ino },
		.i.readdir.offs = 0,
		.o.data = dirbuf,
		.o.size = sizeof(struct dirent) + NAME_MAX + 1
	};

	TEST_ASSERT_EQUAL_INT(0, msgSend(statbuf.st_dev, &msg));
	TEST_ASSERT_GREATER_OR_EQUAL(0, msg.o.err);

	TEST_ASSERT_EQUAL_STRING(".", dirbuf->d_name);

	dir = fdopendir(fd);
	TEST_ASSERT_NOT_NULL(dir);
	dirptr = readdir(dir);
	TEST_ASSERT_NOT_NULL(dirptr);
	TEST_ASSERT_EQUAL_STRING("..", dirptr->d_name);

	free(dirbuf);
}

TEST(dirent, rewinddir)
{
	struct dirent *dirent;
	DIR *dir;

	errno = 0;
	dir = opendir(TEST_DIR);
	TEST_ASSERT_NOT_NULL(dir);
	TEST_ASSERT_EQUAL_INT(0, errno);

	dirent = readdir(dir);
	TEST_ASSERT_NOT_NULL(dirent);
	TEST_ASSERT_EQUAL_STRING(".", dirent->d_name);
	TEST_ASSERT_EQUAL_INT(0, errno);

	rewinddir(dir);
	TEST_ASSERT_EQUAL_INT(0, errno);

	dirent = readdir(dir);
	TEST_ASSERT_NOT_NULL(dirent);
	TEST_ASSERT_EQUAL_STRING(".", dirent->d_name);
	TEST_ASSERT_EQUAL_INT(0, errno);
}

TEST(dirent, seekntell)
{
	struct dirent *dirent;
	DIR *dir;
	long pos;

	errno = 0;
	dir = opendir(TEST_DIR);
	TEST_ASSERT_NOT_NULL(dir);
	TEST_ASSERT_EQUAL_INT(0, errno);

	dirent = readdir(dir);
	TEST_ASSERT_NOT_NULL(dirent);
	TEST_ASSERT_EQUAL_STRING(".", dirent->d_name);
	TEST_ASSERT_EQUAL_INT(0, errno);

	pos = telldir(dir);
	TEST_ASSERT_GREATER_OR_EQUAL(0, pos);
	TEST_ASSERT_EQUAL_INT(0, errno);

	rewinddir(dir);
	TEST_ASSERT_EQUAL_INT(0, errno);

	seekdir(dir, pos);

	dirent = readdir(dir);
	TEST_ASSERT_NOT_NULL(dirent);
	TEST_ASSERT_EQUAL_STRING("..", dirent->d_name);
	TEST_ASSERT_EQUAL_INT(0, errno);
}
