/*
 * Phoenix-RTOS
 *
 * libc-tests
 *
 * Testing path resolving (with symlink support)
 *
 * Copyright 2021 Phoenix Systems
 * Author: Marek Bialowas
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

/* make compilable against glibc (all these tests were run on host against libc/linux kernel) */
#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <unity_fixture.h>

#include "common.h"


static char testWorkDir[PATH_MAX];

/* Tests verifying correctness of resolving paths in libphoenix.
 * The same tests can be compiled for host-generic-pc to verify our assumptions against glibc.
 */

TEST_GROUP(resolve_path);

TEST_SETUP(resolve_path)
{
	/* save the test working diectory */
	TEST_ASSERT_NOT_NULL(getcwd(testWorkDir, sizeof(testWorkDir)));
}


TEST_TEAR_DOWN(resolve_path)
{
	/* go back to the test working directory and assert it */
	TEST_ASSERT_EQUAL_INT(0, chdir(testWorkDir));

	/* TODO: all tests should use common test dir to be recursively removed here */
}


TEST(resolve_path, canonicalize_abs_simple)
{
	TEST_ASSERT_NULL(canonicalize_file_name(""));

	check_and_free_str("/", canonicalize_file_name("/"));
	check_and_free_str("/", canonicalize_file_name("/."));
	check_and_free_str("/", canonicalize_file_name("//"));
	check_and_free_str("/", canonicalize_file_name("///"));
	check_and_free_str("/", canonicalize_file_name("/./"));

	check_and_free_str("/", canonicalize_file_name("/.."));
	check_and_free_str("/", canonicalize_file_name("/../../."));

	check_and_free_str("/etc", canonicalize_file_name("/etc"));
	check_and_free_str("/etc", canonicalize_file_name("/etc//"));
	check_and_free_str("/etc", canonicalize_file_name("/etc/."));

	check_and_free_str("/", canonicalize_file_name("/etc/.."));
	check_and_free_str("/", canonicalize_file_name("/etc/../."));
	check_and_free_str("/etc", canonicalize_file_name("/etc/../etc/"));
	check_and_free_str("/etc", canonicalize_file_name("//etc/..//../etc/"));
}


TEST(resolve_path, canonicalize_pwd_simple)
{
	/* don't care about current dir */
	TEST_ASSERT_EQUAL_INT(0, chdir("/etc"));

	check_and_free_str("/etc", canonicalize_file_name("."));
	check_and_free_str("/", canonicalize_file_name(".."));
	check_and_free_str("/", canonicalize_file_name("./.."));
	check_and_free_str("/", canonicalize_file_name("../."));

	check_and_free_str("/etc/passwd", canonicalize_file_name("passwd"));
	check_and_free_str("/etc/passwd", canonicalize_file_name("./passwd"));

	check_and_free_str("/etc", canonicalize_file_name("../etc"));
}


/* note canonicalize_path_name(path) == realpath(path, NULL) so no need for separate tests */
TEST(resolve_path, realpath_abs_noalloc)
{
	char result[PATH_MAX];

	TEST_ASSERT_EQUAL_STRING("/", realpath("/", result));
	TEST_ASSERT_EQUAL_STRING("/", realpath("/.", result));
	TEST_ASSERT_EQUAL_STRING("/", realpath("//", result));
	TEST_ASSERT_EQUAL_STRING("/", realpath("///", result));
	TEST_ASSERT_EQUAL_STRING("/", realpath("/./", result));

	TEST_ASSERT_EQUAL_STRING("/", realpath("/..", result));
	TEST_ASSERT_EQUAL_STRING("/", realpath("/../../.", result));

	TEST_ASSERT_EQUAL_STRING("/etc", realpath("/etc", result));
	TEST_ASSERT_EQUAL_STRING("/etc", realpath("/etc//", result));
	TEST_ASSERT_EQUAL_STRING("/etc", realpath("/etc/.", result));

	TEST_ASSERT_EQUAL_STRING("/", realpath("/etc/..", result));
	TEST_ASSERT_EQUAL_STRING("/", realpath("/etc/../.", result));
	TEST_ASSERT_EQUAL_STRING("/etc", realpath("/etc/../etc/", result));
	TEST_ASSERT_EQUAL_STRING("/etc", realpath("//etc/..//../etc/", result));
}


TEST(resolve_path, realpath_pwd_noalloc)
{
	char result[PATH_MAX];

	/* don't care about current dir */
	TEST_ASSERT_EQUAL_INT(0, chdir("/etc"));

	TEST_ASSERT_EQUAL_STRING("/etc", realpath(".", result));
	TEST_ASSERT_EQUAL_STRING("/", realpath("..", result));
	TEST_ASSERT_EQUAL_STRING("/", realpath("./..", result));
	TEST_ASSERT_EQUAL_STRING("/", realpath("../.", result));

	TEST_ASSERT_EQUAL_STRING("/etc/passwd", realpath("passwd", result));
	TEST_ASSERT_EQUAL_STRING("/etc/passwd", realpath("./passwd", result));

	TEST_ASSERT_EQUAL_STRING("/etc", realpath("../etc", result));
}


/* failures due to different reasons */
TEST(resolve_path, realpath_errno)
{
	char toolong[PATH_MAX + 10];

	check_null_and_errno(EINVAL, realpath(NULL, NULL));
	check_null_and_errno(ENOENT, realpath("/asdfasdf", NULL));

	memset(toolong, 'a', sizeof(toolong));
	toolong[0] = '/';
	toolong[PATH_MAX + 9] = 0;

	check_null_and_errno(ENAMETOOLONG, realpath(toolong, NULL));
	check_null_and_errno(ENOTDIR, realpath("/etc/passwd/fake", NULL));
	/* ELOOP tested by symlink_loop */
}


/* FIXME: this will fail due to not respecting NAME_MAX */
IGNORE_TEST(resolve_path, realpath_max_path)
{
	const char *tmp_prefix = "/tmp/";
	char path[PATH_MAX]; /* note: PATH_MAX includes \0 */

	memset(path, 'a', sizeof(path));
	memcpy(path, tmp_prefix, strlen(tmp_prefix));
	path[sizeof(path) - 1] = 0;
	printf("path: %s\n", path);

	create_file(path, NULL);

	check_and_free_str(path, realpath(path, NULL));
	/* FIXME: unlink */
}


#ifdef __phoenix__
/* testing resolve_path -> phoenix-specific API */
TEST(resolve_path, missing_leaf)
{
	check_null_and_errno(ENOENT, resolve_path("/etc/missing_file", NULL, 1, 0));
	check_and_free_str("/etc/missing_file", resolve_path("/etc/missing_file", NULL, 1, 1));

	check_and_free_str("/x", resolve_path("/x", NULL, 1, 1));
	check_and_free_str("/etc/x", resolve_path("/etc/x", NULL, 1, 1));

	check_null_and_errno(ENOENT, resolve_path("/etc/missing_dir/.", NULL, 1, 0));
	check_null_and_errno(ENOENT, resolve_path("/etc/missing_dir/.", NULL, 1, 1));
}

TEST(resolve_path, missing_branch)
{
	check_null_and_errno(ENOENT, resolve_path("/etc/missing_dir/missing_file", NULL, 1, 0));
	check_null_and_errno(ENOENT, resolve_path("/etc/x/missing_file", NULL, 1, 0));
}
#endif

static const char *file_contents = "real_data";

/* TODO: get prefix as a cmdline param to test various filesystems */
static const char *prefix = "/tmp";

TEST(resolve_path, symlink_abs)
{
	char path[PATH_MAX] = { 0 };
	char sympath[PATH_MAX] = { 0 };
	char buf[PATH_MAX] = { 0 };

	strcat(path, prefix);
	strcat(path, "/");
	strcat(path, "real_file");

	create_file(path, file_contents);

	strcat(sympath, prefix);
	strcat(sympath, "/");
	strcat(sympath, "symlink"); /* note: name also hardcoded below */

	/* create "absolute path" symlink */
	unlink(sympath);
	if (symlink(path, sympath) < 0)
		TEST_FAIL_MESSAGE(strerror(errno));

	/* resolve symlink */
	check_and_free_str(path, canonicalize_file_name(sympath));

	/* get symlink value directly */
	int symlen = readlink(sympath, buf, sizeof(buf));
	TEST_ASSERT_EQUAL_INT(strlen(path), symlen);
	TEST_ASSERT_EQUAL_STRING(path, buf);

	/* access file by symlink */
	check_file_contents(file_contents, sympath);

	/* access symmlink relatively to cwd */
	TEST_ASSERT_EQUAL_INT(0, chdir(prefix));
	check_and_free_str(path, canonicalize_file_name("symlink"));
	check_and_free_str(path, canonicalize_file_name("./symlink"));


	/* TODO: lstat, stat */

	/* cleanup - WARN: if failed - not reached */
	unlink(path);
	unlink(sympath);
}


TEST(resolve_path, symlink_relative)
{
	char path[PATH_MAX] = { 0 };
	char sympath[PATH_MAX] = { 0 };
	char buf[PATH_MAX] = { 0 };
	char *abspath;

	/* CWD = / */
	TEST_ASSERT_EQUAL_INT(0, chdir("/"));

	/* ../[prefix]/real_file */
	strcat(path, "..");
	strcat(path, prefix);
	strcat(path, "/");
	strcat(path, "real_file");
	abspath = path + 2;

	create_file(path, file_contents);

	/* ../[prefix]/symlink */
	strcat(sympath, "..");
	strcat(sympath, prefix);
	strcat(sympath, "/");
	strcat(sympath, "symlink"); /* note: name also hardcoded below */

	/* create "relative path" symlink */
	unlink(sympath);
	if (symlink(path, sympath) < 0)
		TEST_FAIL_MESSAGE(strerror(errno));

	/* resolve symlink */
	check_and_free_str(abspath, canonicalize_file_name(sympath));

	/* get symlink value directly */
	int symlen = readlink(sympath, buf, sizeof(buf));
	TEST_ASSERT_EQUAL_INT(strlen(path), symlen);
	TEST_ASSERT_EQUAL_STRING(path, buf);

	/* access file by symlink */
	check_file_contents(file_contents, sympath);

	/* access symmlink relatively to cwd */
	TEST_ASSERT_EQUAL_INT(0, chdir(prefix));
	check_and_free_str(abspath, canonicalize_file_name("symlink"));
	check_and_free_str(abspath, canonicalize_file_name("./symlink"));


	/* TODO: lstat, stat */

	/* cleanup - WARN: if failed - not reached */
	TEST_ASSERT_EQUAL_INT(0, chdir("/"));
	unlink(path);
	unlink(sympath);
}


/* create file by symlink */
TEST(resolve_path, symlink_create_file)
{
	char path[PATH_MAX] = { 0 };
	char sympath[PATH_MAX] = { 0 };
	char buf[PATH_MAX] = { 0 };

	strcat(path, prefix);
	strcat(path, "/");
	strcat(path, "real_file");

	strcat(sympath, prefix);
	strcat(sympath, "/");
	strcat(sympath, "symlink"); /* note: name also hardcoded below */

	/* create "absolute path" symlink to non-existing file */
	unlink(path);
	unlink(sympath);
	if (symlink(path, sympath) < 0)
		TEST_FAIL_MESSAGE(strerror(errno));

	/* resolve symlink */
	check_null_and_errno(ENOENT, canonicalize_file_name(sympath));

	/* get symlink value directly */
	int symlen = readlink(sympath, buf, sizeof(buf));
	TEST_ASSERT_EQUAL_INT(strlen(path), symlen);
	TEST_ASSERT_EQUAL_STRING(path, buf);

	/* create file by symlink and check contents by realpath */
	create_file(sympath, file_contents);
	check_and_free_str(path, canonicalize_file_name(sympath));
	check_file_contents(file_contents, path);

	/* cleanup - WARN: if failed - not reached */
	unlink(path);
	unlink(sympath);
}


TEST(resolve_path, symlink_dir)
{
	char path[PATH_MAX] = { 0 };
	char sympath[PATH_MAX] = { 0 };
	char buf[PATH_MAX] = { 0 };

	TEST_ASSERT_EQUAL_INT(0, chdir(prefix));

	/* FIXME: crappy way of cleaning, write recursive rmdir in TEST_TEAR_DOWN() */
	unlink("real_dir/real_file");
	rmdir("real_dir");
	unlink("symlink");

	TEST_ASSERT_EQUAL_INT(0, mkdir("real_dir", 0755));

	strcat(path, prefix);
	strcat(path, "/");
	strcat(path, "real_dir");
	strcat(path, "/");
	strcat(path, "real_file");

	create_file(path, file_contents);

	strcat(sympath, prefix);
	strcat(sympath, "/");
	strcat(sympath, "symlink");

	/* create relative symlink to real_dir */
	if (symlink("real_dir", sympath) < 0)
		TEST_FAIL_MESSAGE(strerror(errno));

	/* verify access to the file by symlink dir */
	strcat(sympath, "/");
	strcat(sympath, "real_file");
	check_and_free_str(path, canonicalize_file_name(sympath));
	check_file_contents(file_contents, sympath);
	*strrchr(sympath, '/') = 0; /* strip file name */

	/* test if we can cd into symlink */
	TEST_ASSERT_EQUAL_INT(0, chdir("symlink"));
	check_file_contents(file_contents, "real_file");

	/* unlink file inside symlink */
	TEST_ASSERT_EQUAL_INT(0, unlink("real_file"));
	*strrchr(path, '/') = 0; /* strip file name */

	// FIXME: 'PWD' should be maintained by shell, not by libc
#if 0
	/* test getcwd inside symlink - should have original symlink */
	char *pwd = getenv("PWD");
	TEST_ASSERT_EQUAL_STRING(sympath, pwd);
#endif

	/* test getcwd inside symlink - should have symlink resolved */
	TEST_ASSERT_NOT_NULL(getcwd(buf, sizeof(buf)));
	TEST_ASSERT_EQUAL_STRING(path, buf);

	/* test cd '..' from symlink */
	TEST_ASSERT_EQUAL_INT(0, chdir(".."));
	TEST_ASSERT_NOT_NULL(getcwd(buf, sizeof(buf)));
	TEST_ASSERT_EQUAL_STRING(prefix, buf);

	/* cleanup - WARN: if failed - not reached */
	TEST_ASSERT_EQUAL_INT(0, rmdir("real_dir"));
	TEST_ASSERT_EQUAL_INT(0, unlink(sympath));
}


/* check if (simple) symlink loop will exit eventually */
TEST(resolve_path, symlink_loop)
{
	/* CWD = prefix */
	TEST_ASSERT_EQUAL_INT(0, chdir(prefix));

	unlink("symlink1");
	unlink("symlink2");

	if (symlink("symlink1", "symlink2") < 0)
		TEST_FAIL_MESSAGE(strerror(errno));

	if (symlink("symlink2", "symlink1") < 0)
		TEST_FAIL_MESSAGE(strerror(errno));

	/* check direct path resolving */
	check_null_and_errno(ELOOP, canonicalize_file_name("symlink1"));
	check_null_and_errno(ELOOP, canonicalize_file_name("./symlink2"));

	/* check indirect - file access */
	check_file_open_errno(ELOOP, "symlink1");
	check_file_open_errno(ELOOP, "./symlink2");

	/* check indirect - try to cd */
	TEST_ASSERT_EQUAL_INT(-1, chdir("symlink1"));
	TEST_ASSERT_EQUAL_INT(ELOOP, errno);

	/* cleanup - WARN: if failed - not reached */
	unlink("symlink1");
	unlink("symlink2");
}


/* check if "mv symlink xxx" changes name of the symlink and not the target file */
TEST(resolve_path, symlink_rename)
{
	/* CWD = prefix */
	TEST_ASSERT_EQUAL_INT(0, chdir(prefix));

	unlink("symlink_old");
	unlink("symlink_new");
	unlink("real_file");

	create_file("real_file", file_contents);

	if (symlink("real_file", "symlink_old") < 0)
		TEST_FAIL_MESSAGE(strerror(errno));

	check_file_contents(file_contents, "symlink_old");

	if (rename("symlink_old", "symlink_new") < 0)
		TEST_FAIL_MESSAGE(strerror(errno));

	check_file_contents(file_contents, "real_file");
	check_file_open_errno(ENOENT, "symlink_old");
	check_file_contents(file_contents, "symlink_new");

	/* cleanup - WARN: if failed - not reached */
	TEST_ASSERT_EQUAL_INT(0, unlink("symlink_new"));
	TEST_ASSERT_EQUAL_INT(0, unlink("real_file"));
}


/* check if path temporarily longer than PATH_MAX while resolving won't crash */
TEST(resolve_path, symlink_long_resolution)
{
	char symNameLong[NAME_MAX / 2];
	char resolved[PATH_MAX + 1];
	char basePath[PATH_MAX - 5] = "symShort/";
	int basePathLen = strlen(basePath);
	const char symNameShort[] = "symShort";
	const char pathSegment[] = "dev/../";
	const int pathSegmentLen = sizeof(pathSegment) - 1;

	while (basePathLen + pathSegmentLen < sizeof(basePath) - 5) {
		strcat(basePath, pathSegment);
		basePathLen += pathSegmentLen;
	}
	strcat(basePath, "dev");

	memset(symNameLong, 'a', sizeof(symNameLong));
	symNameLong[sizeof(symNameLong) - 1] = 0;

	unlink(symNameShort);
	unlink(symNameLong);

	TEST_ASSERT_EQUAL_INT(0, symlink("/", symNameLong));
	TEST_ASSERT_EQUAL_INT(0, symlink(symNameLong, symNameShort));

	errno = 0;

	/* As it's described in the `realpath()` doc the function MAY fail in such case */
	if (realpath(basePath, resolved) != NULL) {
		TEST_ASSERT_EQUAL_STRING("/dev", resolved);
	}
	else {
		TEST_ASSERT_EQUAL_INT(ENAMETOOLONG, errno);
	}

	/* cleanup */
	unlink(symNameShort);
	unlink(symNameLong);
}


TEST_GROUP_RUNNER(resolve_path)
{
	RUN_TEST_CASE(resolve_path, canonicalize_abs_simple);
	RUN_TEST_CASE(resolve_path, canonicalize_pwd_simple);

	RUN_TEST_CASE(resolve_path, realpath_abs_noalloc);
	RUN_TEST_CASE(resolve_path, realpath_pwd_noalloc);

	RUN_TEST_CASE(resolve_path, realpath_errno);
	RUN_TEST_CASE(resolve_path, realpath_max_path);

#ifdef __phoenix__
	RUN_TEST_CASE(resolve_path, missing_leaf);
	RUN_TEST_CASE(resolve_path, missing_branch);
#endif

	RUN_TEST_CASE(resolve_path, symlink_abs);
	RUN_TEST_CASE(resolve_path, symlink_relative);
	RUN_TEST_CASE(resolve_path, symlink_create_file);
	RUN_TEST_CASE(resolve_path, symlink_dir);
	RUN_TEST_CASE(resolve_path, symlink_loop);
	RUN_TEST_CASE(resolve_path, symlink_rename);
	RUN_TEST_CASE(resolve_path, symlink_long_resolution);
}
