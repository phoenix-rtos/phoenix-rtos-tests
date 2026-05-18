/*
 * Phoenix-RTOS
 *
 * libc-tests
 *
 * Testing getpwuid and getpwnam edge cases
 *
 * Copyright 2021 Phoenix Systems
 * Author: Mateusz Niewiadomski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <pwd.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>

#include <unity_fixture.h>


/* "/" is a root dir and the supported shell is sh on Phoenix-RTOS */
#ifdef __phoenix__
#define ROOT_WORKDIR "/"
#define ROOT_SHELL   "/bin/sh"
#else
#define ROOT_WORKDIR "/root"
#define ROOT_SHELL   "/bin/bash"
#endif

static unsigned int ispasswdfile;


TEST_GROUP(getpwd);


TEST_SETUP(getpwd)
{
}


TEST_TEAR_DOWN(getpwd)
{
}


/* Try to reach for non-existing user data with getpwuid */
TEST(getpwd, getpwuid_getnull)
{
	TEST_ASSERT_NULL(getpwuid((uid_t)-1));
	TEST_ASSERT_NULL(getpwuid((uid_t)65535));
	/* TODO: add errno check */

	if (!ispasswdfile)
		TEST_IGNORE_MESSAGE("No /etc/passwd file!");
}


/* Try to reach for root user data with no /etc/passwd file with getpwnam */
TEST(getpwd, getpwnam_nopasswdfile)
{
	if (ispasswdfile)
		rename("/etc/passwd", "/etc/passwd_del");

	TEST_ASSERT_NULL(getpwnam("root"));
	/* TODO: add errno check */

	if (ispasswdfile)
		rename("/etc/passwd_del", "/etc/passwd");
}


/* Try to reach for root user data with no /etc/passwd file with getpwuid */
TEST(getpwd, getpwuid_nopasswdfile)
{
	if (ispasswdfile)
		rename("/etc/passwd", "/etc/passwd_del");

	TEST_ASSERT_NULL(getpwuid((uid_t)0));
	/* TODO: add errno check */

	if (ispasswdfile)
		rename("/etc/passwd_del", "/etc/passwd");
}


TEST_GROUP_RUNNER(getpwd)
{
	ispasswdfile = (access("/etc/passwd", F_OK) == 0) ? 1 : 0;

	RUN_TEST_CASE(getpwd, getpwuid_getnull);
	/* we can't rename/delete the /etc/passwd file on host */
#ifdef __phoenix__
	RUN_TEST_CASE(getpwd, getpwnam_nopasswdfile);
	RUN_TEST_CASE(getpwd, getpwuid_nopasswdfile);
#endif
}
