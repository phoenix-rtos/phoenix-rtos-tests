/*
 * Phoenix-RTOS
 *
 * libc-tests
 *
 * Testing procenv related functions
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
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unity_fixture.h>

#include "common.h"


#define FNAME   "procenv_file"
#define DIRNAME "procenv_directory"

static FILE *filep;
static char testWorkDir[PATH_MAX];
static char buf[PATH_MAX];
static char toolongpath[PATH_MAX + 16];

TEST_GROUP(procenv);

TEST_SETUP(procenv)
{
	/* clear buffer */
	memset(buf, 0, sizeof(buf));

	/* save the test working diectory */
	TEST_ASSERT_NOT_NULL(getcwd(testWorkDir, sizeof(testWorkDir)));

	/* clear/create file */
	filep = fopen(FNAME, "w");
	if (filep != NULL)
		fclose(filep);

	/* set too long path */
	memset(toolongpath, 'a', sizeof(toolongpath) - 1);
	toolongpath[sizeof(toolongpath) - 1] = '\0';
}


TEST_TEAR_DOWN(procenv)
{
	/* go back to the test working directory */
	TEST_ASSERT_EQUAL_INT(0, chdir(testWorkDir));
	TEST_ASSERT_EQUAL_INT(0, remove(FNAME));
}


TEST(procenv, getcwd)
{
	/* assumption that chdir("/") works fine when returning 0 */
	TEST_ASSERT_EQUAL_INT(0, chdir("/"));

	TEST_ASSERT_NOT_NULL(getcwd(buf, sizeof(buf)));
	TEST_ASSERT_EQUAL_STRING("/", buf);

	TEST_ASSERT_NULL(getcwd(buf, 0));
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);

	TEST_ASSERT_NULL(getcwd(buf, 1));
	TEST_ASSERT_EQUAL_INT(ERANGE, errno);
}


TEST(procenv, chdir_absroot)
{
	/* test chdir to root */
	TEST_ASSERT_EQUAL_INT(0, chdir("/"));
	TEST_ASSERT_NOT_NULL(getcwd(buf, sizeof(buf)));
	TEST_ASSERT_EQUAL_STRING("/", buf);

	/* go back to the test working directory and assert it */
	TEST_ASSERT_EQUAL_INT(0, chdir(testWorkDir));
	TEST_ASSERT_NOT_NULL(getcwd(buf, sizeof(buf)));
	TEST_ASSERT_EQUAL_STRING(testWorkDir, buf);
}


TEST(procenv, chdir_absdev)
{
	/* test chdir to some directory */
	TEST_ASSERT_EQUAL_INT(0, chdir("/dev"));
	TEST_ASSERT_NOT_NULL(getcwd(buf, sizeof(buf)));
	TEST_ASSERT_EQUAL_STRING("/dev", buf);

	/* go back to the test working directory and assert it */
	TEST_ASSERT_EQUAL_INT(0, chdir(testWorkDir));
	TEST_ASSERT_NOT_NULL(getcwd(buf, sizeof(buf)));
	TEST_ASSERT_EQUAL_STRING(testWorkDir, buf);
}


TEST(procenv, chdir_rel)
{
	char absPath[PATH_MAX];
	size_t slen;

	strncpy(absPath, testWorkDir, sizeof(absPath));

	slen = strlen(absPath);
	TEST_ASSERT_GREATER_OR_EQUAL(slen + (size_t)sizeof(DIRNAME) + 2, (size_t)sizeof(absPath));

	if (absPath[slen - 1] != '/') {
		absPath[slen++] = '/';
		absPath[slen] = '\0';
	}
	strcpy(absPath + slen, DIRNAME);

	TEST_ASSERT_EQUAL_INT(0, mkdir(DIRNAME, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));

	/* test chdir to some directory */
	TEST_ASSERT_EQUAL_INT(0, chdir(DIRNAME));
	TEST_ASSERT_NOT_NULL(getcwd(buf, sizeof(buf)));
	TEST_ASSERT_EQUAL_STRING(absPath, buf);

	/* test chdir to cwd */
	TEST_ASSERT_EQUAL_INT(0, chdir("."));
	TEST_ASSERT_NOT_NULL(getcwd(buf, sizeof(buf)));
	TEST_ASSERT_EQUAL_STRING(absPath, buf);

	/* test chdir back to working directory */
	TEST_ASSERT_EQUAL_INT(0, chdir(".."));
	TEST_ASSERT_NOT_NULL(getcwd(buf, sizeof(buf)));
	TEST_ASSERT_EQUAL_STRING(testWorkDir, buf);

	TEST_ASSERT_EQUAL_INT(0, rmdir(DIRNAME));
}


TEST(procenv, chdir_toolongpath)
{
	/* test chdir with too long path */
	TEST_ASSERT_EQUAL_INT(-1, chdir(toolongpath));
	TEST_ASSERT_EQUAL_INT(ENAMETOOLONG, errno);
}


TEST(procenv, chdir_nonexistent)
{
	/* test chdir to nonexisting directory */
	TEST_ASSERT_EQUAL_INT(-1, chdir("not_existing_directory"));
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);
}


TEST(procenv, chdir_emptystring)
{
	/* test chdir to empty string */
	TEST_ASSERT_EQUAL_INT(-1, chdir(""));
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);
}


TEST(procenv, chdir_tofile)
{
	/* test chdir to file */
	TEST_ASSERT_EQUAL_INT(-1, chdir(FNAME));
	TEST_ASSERT_EQUAL_INT(ENOTDIR, errno);
}


IGNORE_TEST(procenv, fchdir)
{
	/*
		Declared but unimplemented in libphoenix
		https://github.com/phoenix-rtos/phoenix-rtos-project/issues/280
	*/
}


TEST_GROUP_RUNNER(procenv)
{
	RUN_TEST_CASE(procenv, getcwd);

	RUN_TEST_CASE(procenv, chdir_absroot)
	RUN_TEST_CASE(procenv, chdir_absdev)
	RUN_TEST_CASE(procenv, chdir_rel)
	RUN_TEST_CASE(procenv, chdir_toolongpath);
	RUN_TEST_CASE(procenv, chdir_nonexistent);
	RUN_TEST_CASE(procenv, chdir_emptystring);
	RUN_TEST_CASE(procenv, chdir_tofile);

	RUN_TEST_CASE(procenv, fchdir);
}
