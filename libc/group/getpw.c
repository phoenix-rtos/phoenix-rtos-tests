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


/* Get root account details by name */
TEST(getpwd, getpwnam_getroot)
{
	struct passwd *pw = getpwnam("root");

	if (ispasswdfile) {
		TEST_ASSERT_NOT_NULL(pw);
		TEST_ASSERT_EQUAL_STRING("root", pw->pw_name);
		/* no password hash content checking, only its nullability and size>0 */
		TEST_ASSERT_NOT_NULL(pw->pw_passwd);
		TEST_ASSERT_GREATER_THAN(0, strlen(pw->pw_passwd));
		TEST_ASSERT_EQUAL_INT(0, pw->pw_uid);
		TEST_ASSERT_EQUAL_INT(0, pw->pw_gid);
		TEST_ASSERT_EQUAL_STRING("root", pw->pw_gecos);
		TEST_ASSERT_EQUAL_STRING(ROOT_WORKDIR, pw->pw_dir);
		TEST_ASSERT_EQUAL_STRING(ROOT_SHELL, pw->pw_shell);
	}
	else {
		TEST_ASSERT_NULL(pw);
		TEST_IGNORE_MESSAGE("No /etc/passwd file!");
	}
}


/* Get root account details by uid */
TEST(getpwd, getpwuid_getroot)
{
	struct passwd *pw = getpwuid((uid_t)0);
	if (ispasswdfile) {
		TEST_ASSERT_NOT_NULL(pw);

		TEST_ASSERT_EQUAL_STRING("root", pw->pw_name);
		/* no password hash content checking, only its nullability and size>0 */
		TEST_ASSERT_NOT_NULL(pw->pw_passwd);
		TEST_ASSERT_GREATER_THAN(0, strlen(pw->pw_passwd));
		TEST_ASSERT_EQUAL_INT(0, pw->pw_uid);
		TEST_ASSERT_EQUAL_INT(0, pw->pw_gid);
		TEST_ASSERT_EQUAL_STRING("root", pw->pw_gecos);
		TEST_ASSERT_EQUAL_STRING(ROOT_WORKDIR, pw->pw_dir);
		TEST_ASSERT_EQUAL_STRING(ROOT_SHELL, pw->pw_shell);
	}
	else {
		TEST_ASSERT_NULL(pw);
		TEST_IGNORE_MESSAGE("No /etc/passwd file!");
	}
}


/* Try to reach for non-existing user data with getpwnam */
TEST(getpwd, getpwnam_getnull)
{
	TEST_ASSERT_NULL(getpwnam("loremipsum"));
	// FIXME: invalid test
	//TEST_ASSERT_NULL(getpwnam(NULL));
	/* TODO: add errno check */

	if (!ispasswdfile)
		TEST_IGNORE_MESSAGE("No /etc/passwd file!");
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

	RUN_TEST_CASE(getpwd, getpwnam_getroot);
	RUN_TEST_CASE(getpwd, getpwuid_getroot);
	RUN_TEST_CASE(getpwd, getpwnam_getnull);
	RUN_TEST_CASE(getpwd, getpwuid_getnull);
	/* we can't rename/delete the /etc/passwd file on host */
#ifdef __phoenix__
	RUN_TEST_CASE(getpwd, getpwnam_nopasswdfile);
	RUN_TEST_CASE(getpwd, getpwuid_nopasswdfile);
#endif
}
