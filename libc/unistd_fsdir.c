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


#define FNAME   "unistd_fsdir_file"
#define DIRNAME "unistd_fsdir_directory"

static FILE *filep;
static char buf[50];
static char toolongpath[PATH_MAX + 16];

TEST_GROUP(unistd_fsdir);

TEST_SETUP(unistd_fsdir)
{
	/* change to and assert we are in root */
	TEST_ASSERT_EQUAL_INT(0, chdir("/"));
	TEST_ASSERT_NOT_NULL(getcwd(buf, sizeof(buf)));
	TEST_ASSERT_EQUAL_STRING("/", buf);

	/* clear buffer */
	memset(buf, 0, sizeof(buf));

	/* clear/create file */
	filep = fopen(FNAME, "w");
	if (filep != NULL)
		fclose(filep);

	/* set too long path */
	memset(toolongpath, 'a', sizeof(toolongpath) - 1);
	toolongpath[sizeof(toolongpath) - 1] = '\0';
}


TEST_TEAR_DOWN(unistd_fsdir)
{
	TEST_ASSERT_EQUAL_INT(0, remove(FNAME));
}


TEST(unistd_fsdir, getcwd)
{
	TEST_ASSERT_NOT_NULL(getcwd(buf, sizeof(buf)));
	TEST_ASSERT_EQUAL_STRING("/", buf);

	TEST_ASSERT_NULL(getcwd(buf, 0));
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);

	TEST_ASSERT_NULL(getcwd(buf, 1));
	TEST_ASSERT_EQUAL_INT(ERANGE, errno);
}


TEST(unistd_fsdir, chdir_abs)
{
	/* test chdir to some directory */
	TEST_ASSERT_EQUAL_INT(0, chdir("/dev"));
	TEST_ASSERT_NOT_NULL(getcwd(buf, sizeof(buf)));
	TEST_ASSERT_EQUAL_STRING("/dev", buf);

	/* test chdir to cwd */
	TEST_ASSERT_EQUAL_INT(0, chdir("/dev"));
	TEST_ASSERT_NOT_NULL(getcwd(buf, sizeof(buf)));
	TEST_ASSERT_EQUAL_STRING("/dev", buf);

	/* test chdir back to root */
	TEST_ASSERT_EQUAL_INT(0, chdir("/"));
	TEST_ASSERT_NOT_NULL(getcwd(buf, sizeof(buf)));
	TEST_ASSERT_EQUAL_STRING("/", buf);
}


TEST(unistd_fsdir, chdir_rel)
{
	/* test chdir to some directory */
	TEST_ASSERT_EQUAL_INT(0, chdir("dev"));
	TEST_ASSERT_NOT_NULL(getcwd(buf, sizeof(buf)));
	TEST_ASSERT_EQUAL_STRING("/dev", buf);

	/* test chdir to cwd */
	TEST_ASSERT_EQUAL_INT(0, chdir("."));
	TEST_ASSERT_NOT_NULL(getcwd(buf, sizeof(buf)));
	TEST_ASSERT_EQUAL_STRING("/dev", buf);

	/* test chdir back to root */
	TEST_ASSERT_EQUAL_INT(0, chdir(".."));
	TEST_ASSERT_NOT_NULL(getcwd(buf, sizeof(buf)));
	TEST_ASSERT_EQUAL_STRING("/", buf);
}


TEST(unistd_fsdir, chdir_toolongpath)
{
	/* test chdir with too long path */
	TEST_ASSERT_EQUAL_INT(-1, chdir(toolongpath));
	TEST_ASSERT_EQUAL_INT(ENAMETOOLONG, errno);
}


TEST(unistd_fsdir, chdir_nonexistent)
{
	/* test chdir to nonexisting directory */
	TEST_ASSERT_EQUAL_INT(-1, chdir("/not_existing_directory"));
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);
}


TEST(unistd_fsdir, chdir_emptystring)
{
	/* test chdir to empty string */
	TEST_ASSERT_EQUAL_INT(-1, chdir(""));
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);
}


TEST(unistd_fsdir, chdir_tofile)
{
	/* test chdir to file */
	TEST_ASSERT_EQUAL_INT(-1, chdir(FNAME));
	TEST_ASSERT_EQUAL_INT(ENOTDIR, errno);
}


TEST(unistd_fsdir, rmdir_empty)
{
	/* test removing empty directory */
	TEST_ASSERT_EQUAL_INT(0, mkdir(DIRNAME, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
	TEST_ASSERT_EQUAL_INT(0, rmdir(DIRNAME));
}


TEST(unistd_fsdir, rmdir_nonexistent)
{
	/* test rmdir on nonexisting directory*/
	TEST_ASSERT_EQUAL_INT(-1, rmdir("/not_existing_directory"));
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);
}


TEST(unistd_fsdir, rmdir_toolongpath)
{
	/* test rmdir with too long path */
	TEST_ASSERT_EQUAL_INT(-1, rmdir(toolongpath));
	TEST_ASSERT_EQUAL_INT(ENAMETOOLONG, errno);
}


TEST(unistd_fsdir, rmdir_emptystring)
{
	/* test rmdir on empty string */
	TEST_ASSERT_EQUAL_INT(-1, rmdir(""));
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);
}


TEST(unistd_fsdir, rmdir_file)
{
	/* test rmdir on file */
	TEST_ASSERT_EQUAL_INT(-1, rmdir(FNAME));
	TEST_ASSERT_EQUAL_INT(ENOTDIR, errno);
}


TEST(unistd_fsdir, rmdir_notempty)
{
	/* prepare not empty directory */
	TEST_ASSERT_EQUAL_INT(0, mkdir(DIRNAME, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
	TEST_ASSERT_EQUAL_INT(0, chdir(DIRNAME));
	filep = fopen(FNAME, "w");
	TEST_ASSERT_NOT_NULL(filep);
	TEST_ASSERT_EQUAL_INT(0, fclose(filep));
	TEST_ASSERT_EQUAL_INT(0, chdir("/"));

	/* test removing not empty directory */
	TEST_ASSERT_EQUAL_INT(-1, rmdir(DIRNAME));
	TEST_ASSERT_EQUAL_INT(ENOTEMPTY, errno);

	/* cleanup */
	TEST_ASSERT_EQUAL_INT(0, chdir(DIRNAME));
	TEST_ASSERT_EQUAL_INT(0, remove(FNAME));
	TEST_ASSERT_EQUAL_INT(0, chdir("/"));
	TEST_ASSERT_EQUAL_INT(0, rmdir(DIRNAME));
}


TEST(unistd_fsdir, fchdir)
{
	/*
		Declared but unimplemented in libphoenix
		https://github.com/phoenix-rtos/phoenix-rtos-project/issues/280
	*/
	TEST_IGNORE();
}


TEST(unistd_fsdir, fchown)
{
	/*
		Unimplemented in libphoenix
		https://github.com/phoenix-rtos/phoenix-rtos-project/issues/280
	*/
	TEST_IGNORE();
}


TEST_GROUP_RUNNER(unistd_fsdir)
{
	RUN_TEST_CASE(unistd_fsdir, getcwd);

	RUN_TEST_CASE(unistd_fsdir, chdir_abs)
	RUN_TEST_CASE(unistd_fsdir, chdir_rel)
	RUN_TEST_CASE(unistd_fsdir, chdir_toolongpath);
	RUN_TEST_CASE(unistd_fsdir, chdir_nonexistent);
	RUN_TEST_CASE(unistd_fsdir, chdir_emptystring);
	RUN_TEST_CASE(unistd_fsdir, chdir_tofile);

	RUN_TEST_CASE(unistd_fsdir, rmdir_empty);
	RUN_TEST_CASE(unistd_fsdir, rmdir_nonexistent);
	RUN_TEST_CASE(unistd_fsdir, rmdir_toolongpath);
	RUN_TEST_CASE(unistd_fsdir, rmdir_emptystring);
	RUN_TEST_CASE(unistd_fsdir, rmdir_file);
	RUN_TEST_CASE(unistd_fsdir, rmdir_notempty);

	RUN_TEST_CASE(unistd_fsdir, fchdir);
	RUN_TEST_CASE(unistd_fsdir, fchown);
}
