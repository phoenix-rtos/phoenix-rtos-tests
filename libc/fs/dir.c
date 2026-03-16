/*
 * Phoenix-RTOS
 *
 * libc-tests
 *
 * Testing directory related functions
 *
 * Copyright 2022 Phoenix Systems
 * Author: Mateusz Niewiadomski, Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unity_fixture.h>


TEST_GROUP(rmdir);

TEST_SETUP(rmdir)
{
}


TEST_TEAR_DOWN(rmdir)
{
}


TEST(rmdir, rmdir_empty)
{
	/* test removing empty directory */
	TEST_ASSERT_EQUAL_INT(0, mkdir("rmdir_empty-d", 0));
	TEST_ASSERT_EQUAL_INT(0, rmdir("rmdir_empty-d"));
}


TEST(rmdir, rmdir_nonexistent)
{
	/* test rmdir on nonexisting directory*/
	TEST_ASSERT_EQUAL_INT(-1, rmdir("rmdir_nonexistent-d"));
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);
}


TEST(rmdir, rmdir_toolongpath)
{
	char toolongpath[PATH_MAX + 16];

	memset(toolongpath, 'a', sizeof(toolongpath) - 1);
	toolongpath[sizeof(toolongpath) - 1] = '\0';

	/* test rmdir with too long path */
	TEST_ASSERT_EQUAL_INT(-1, rmdir(toolongpath));
	TEST_ASSERT_EQUAL_INT(ENAMETOOLONG, errno);
}


TEST(rmdir, rmdir_emptystring)
{
	/* test rmdir on empty string */
	TEST_ASSERT_EQUAL_INT(-1, rmdir(""));
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);
}


TEST(rmdir, rmdir_file)
{
	TEST_ASSERT_GREATER_THAN_INT(0, creat("rmdir_file-t", 0));

	/* test rmdir on file */
	TEST_ASSERT_EQUAL_INT(-1, rmdir("rmdir_file-t"));
	TEST_ASSERT_EQUAL_INT(ENOTDIR, errno);

	TEST_ASSERT_EQUAL_INT(0, unlink("rmdir_file-t"));
}


TEST(rmdir, rmdir_notempty)
{
	/* prepare not empty directory */
	TEST_ASSERT_EQUAL_INT(0, mkdir("rmdir_notempty-d", 0));
	TEST_ASSERT_GREATER_THAN_INT(0, creat("rmdir_notempty-d/rmdir_notempty-t", 0));

	/* test removing not empty directory */
	TEST_ASSERT_EQUAL_INT(-1, rmdir("rmdir_notempty-d"));
	TEST_ASSERT_EQUAL_INT(ENOTEMPTY, errno);

	/* clean up */
	TEST_ASSERT_EQUAL_INT(0, unlink("rmdir_notempty-d/rmdir_notempty-t"));
	TEST_ASSERT_EQUAL_INT(0, rmdir("rmdir_notempty-d"));
}


TEST_GROUP_RUNNER(rmdir)
{
	RUN_TEST_CASE(rmdir, rmdir_empty);
	RUN_TEST_CASE(rmdir, rmdir_nonexistent);
	RUN_TEST_CASE(rmdir, rmdir_toolongpath);
	RUN_TEST_CASE(rmdir, rmdir_emptystring);
	RUN_TEST_CASE(rmdir, rmdir_file);
	RUN_TEST_CASE(rmdir, rmdir_notempty);
}
