/*
 * Phoenix-RTOS
 *
 * libc-tests
 *
 * Testing unistd.h file/fs/directory related functions
 *
 * Copyright 2022 Phoenix Systems
 * Author: 
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <unity_fixture.h>

#include "common.h"


FILE *fd;
char buf[50];
const char filename[] = "unistd_fsdir_file";
const char directoryname[] = "unistd_fsdir_directory";
int err;
char toolongpath[PATH_MAX + 16];

TEST_GROUP(unistd_fsdir);

TEST_SETUP(unistd_fsdir)
{
	TEST_ASSERT_EQUAL_INT(0, chdir("/"));
	TEST_ASSERT_NOT_NULL(getcwd(buf, 50));
	TEST_ASSERT_EQUAL_STRING("/", buf);
	memset(buf, 0, 50);
	fd = fopen(filename, "w");
	fclose(fd);

	memset(toolongpath, 'a', PATH_MAX + 15);
	toolongpath[PATH_MAX + 15] = '\0';
}


TEST_TEAR_DOWN(unistd_fsdir)
{
	TEST_ASSERT_EQUAL_INT(0, remove(filename));
}


TEST(unistd_fsdir, fsdir_getcwd)
{
	TEST_ASSERT_NOT_NULL(getcwd(buf, 50));
	TEST_ASSERT_EQUAL_STRING("/", buf);

	TEST_ASSERT_NULL(getcwd(buf, 0));
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);

	TEST_ASSERT_NULL(getcwd(buf, 1));
	TEST_ASSERT_EQUAL_INT(ERANGE, errno);
}


TEST(unistd_fsdir, fsdir_chdir)
{
	/* test chdir to some directory */
	TEST_ASSERT_EQUAL_INT(0, chdir("/dev"));
	TEST_ASSERT_NOT_NULL(getcwd(buf, 50));
	TEST_ASSERT_EQUAL_STRING("/dev", buf);

	/* test chdir back to root */
	TEST_ASSERT_EQUAL_INT(0, chdir("/"));
	TEST_ASSERT_NOT_NULL(getcwd(buf, 50));
	TEST_ASSERT_EQUAL_STRING("/", buf);

	/* test chdir with too long path */
	TEST_ASSERT_EQUAL_INT(-1, chdir(toolongpath));
	TEST_ASSERT_EQUAL_INT(ENAMETOOLONG, errno);

	/* test chdir to nonexisting directory */
	TEST_ASSERT_EQUAL_INT(-1, chdir("/not_existing_directory"));
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);

	/*
		<posix incompliant> wrong errno setting if:
		- path is empty
		- path points to file

		https://github.com/phoenix-rtos/phoenix-rtos-project/issues/287
	*/
	TEST_IGNORE();

	/* test chdir to empty string */
	TEST_ASSERT_EQUAL_INT(-1, chdir(""));
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);

	/* test chdir to file */
	TEST_ASSERT_EQUAL_INT(-1, chdir(filename));
	TEST_ASSERT_EQUAL_INT(ENOTDIR, errno);
}


TEST(unistd_fsdir, fsdir_rmdir)
{
	/* test removing empty directory */
	TEST_ASSERT_EQUAL_INT(0, mkdir(directoryname, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
	TEST_ASSERT_EQUAL_INT(0, rmdir(directoryname));

	/* test rmdir on nonexisting directory*/
	TEST_ASSERT_EQUAL_INT(-1, rmdir("/not_existing_directory"));
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);

	/* test rmdir with too long path */
	TEST_ASSERT_EQUAL_INT(-1, rmdir(toolongpath));
	TEST_ASSERT_EQUAL_INT(ENAMETOOLONG, errno);

	/*
		<posix incompliant> wrong errno setting if:
		- path is empty (wrong errno)
		- path points to file (file is removed!)
		- removing not empty directory (wrong errno)

		https://github.com/phoenix-rtos/phoenix-rtos-project/issues/288
	*/
	TEST_IGNORE();

	/* test rmdir on empty string */
	TEST_ASSERT_EQUAL_INT(-1, rmdir(""));
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);

	/* test rmdir on file */
	TEST_ASSERT_EQUAL_INT(-1, rmdir(filename));
	TEST_ASSERT_EQUAL_INT(ENOTDIR, errno);

	/* test removing not empty directory */
	TEST_ASSERT_EQUAL_INT(0, mkdir(directoryname, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
	chdir(directoryname);
	fd = fopen(filename, "w");
	chdir("/");
	TEST_ASSERT_EQUAL_INT(-1, rmdir(directoryname));
	TEST_ASSERT_EQUAL_INT(ENOTEMPTY, errno);
	chdir(directoryname);
	remove(filename);
	chdir("/");
	TEST_ASSERT_EQUAL_INT(0, rmdir(directoryname));
}


TEST(unistd_fsdir, fsdir_fchdir)
{
	/*
		Declared but unimplemented in libphoenix
		https://github.com/phoenix-rtos/phoenix-rtos-project/issues/280
	*/
	TEST_IGNORE();
}


TEST(unistd_fsdir, fsdir_fchown)
{
	/*
		Unimplemented in libphoenix
		https://github.com/phoenix-rtos/phoenix-rtos-project/issues/280
	*/
	TEST_IGNORE();
}


TEST_GROUP_RUNNER(unistd_fsdir)
{
	RUN_TEST_CASE(unistd_fsdir, fsdir_getcwd);
	RUN_TEST_CASE(unistd_fsdir, fsdir_chdir);
	RUN_TEST_CASE(unistd_fsdir, fsdir_rmdir);
	RUN_TEST_CASE(unistd_fsdir, fsdir_fchdir);
	RUN_TEST_CASE(unistd_fsdir, fsdir_fchown);
}
