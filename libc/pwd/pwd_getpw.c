/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <pwd.h>
 * TESTED:
 *    - endpwent()
 *    - getpwent()
 *    - getpwnam_r()
 *    - getpwuid_r()
 *    - setpwent()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "unity_fixture.h"

#define PWD_BUF_SIZE 1024

/* Tests: endpwent, getpwent, setpwent */
TEST_GROUP(pwd_getpwent);

TEST_SETUP(pwd_getpwent)
{
	endpwent();
}

TEST_TEAR_DOWN(pwd_getpwent)
{
	endpwent();
}


TEST(pwd_getpwent, getpwent_returns_first_entry)
{
	struct passwd *pw;

	pw = getpwent();
	TEST_ASSERT_NOT_NULL(pw);
	TEST_ASSERT_NOT_NULL(pw->pw_name);
	TEST_ASSERT_NOT_NULL(pw->pw_dir);
	TEST_ASSERT_NOT_NULL(pw->pw_shell);
}


TEST(pwd_getpwent, getpwent_iterates_multiple_entries)
{
	struct passwd *pw1;
	struct passwd *pw2;
	const char *name1;
	static char nameBuf[PWD_BUF_SIZE];

	pw1 = getpwent();
	TEST_ASSERT_NOT_NULL(pw1);
	name1 = pw1->pw_name;
	TEST_ASSERT_NOT_NULL(name1);
	strncpy(nameBuf, name1, sizeof(nameBuf) - 1);
	nameBuf[sizeof(nameBuf) - 1] = '\0';

	pw2 = getpwent();
	/* The user database size is implementation-defined; a second call may
	 * return another entry or NULL if only one entry exists. */
	if (pw2 != NULL) {
		TEST_ASSERT_NOT_NULL(pw2->pw_name);
		TEST_ASSERT_NOT_EQUAL_INT(0, strcmp(nameBuf, pw2->pw_name));
	}
}


TEST(pwd_getpwent, getpwent_returns_null_at_end)
{
	struct passwd *pw;
	int count = 0;

	/* Iterate through all entries */
	while ((pw = getpwent()) != NULL) {
		count++;
		/* Safety limit to avoid infinite loop */
		if (count > 10000) {
			break;
		}
	}

	TEST_ASSERT_GREATER_THAN_INT(0, count);
	/* After exhausting the database, getpwent returns NULL */
	pw = getpwent();
	TEST_ASSERT_NULL(pw);
}


TEST(pwd_getpwent, setpwent_rewinds_database)
{
	struct passwd *pw1;
	struct passwd *pw2;
	static char nameBuf[PWD_BUF_SIZE];

	pw1 = getpwent();
	TEST_ASSERT_NOT_NULL(pw1);
	TEST_ASSERT_NOT_NULL(pw1->pw_name);
	strncpy(nameBuf, pw1->pw_name, sizeof(nameBuf) - 1);
	nameBuf[sizeof(nameBuf) - 1] = '\0';

	/* Advance one more entry */
	(void)getpwent();

	/* Rewind */
	setpwent();

	pw2 = getpwent();
	TEST_ASSERT_NOT_NULL(pw2);
	TEST_ASSERT_NOT_NULL(pw2->pw_name);
	/* After rewind, first entry should be the same */
	TEST_ASSERT_EQUAL_STRING(nameBuf, pw2->pw_name);
}


TEST(pwd_getpwent, endpwent_allows_fresh_iteration)
{
	struct passwd *pw1;
	struct passwd *pw2;
	static char nameBuf[PWD_BUF_SIZE];

	pw1 = getpwent();
	TEST_ASSERT_NOT_NULL(pw1);
	TEST_ASSERT_NOT_NULL(pw1->pw_name);
	strncpy(nameBuf, pw1->pw_name, sizeof(nameBuf) - 1);
	nameBuf[sizeof(nameBuf) - 1] = '\0';

	endpwent();

	/* After endpwent + getpwent, should start from beginning */
	pw2 = getpwent();
	TEST_ASSERT_NOT_NULL(pw2);
	TEST_ASSERT_NOT_NULL(pw2->pw_name);
	TEST_ASSERT_EQUAL_STRING(nameBuf, pw2->pw_name);
}


TEST(pwd_getpwent, getpwent_fields_valid)
{
	struct passwd *pw;

	pw = getpwent();
	TEST_ASSERT_NOT_NULL(pw);

	/* pw_name shall not be empty */
	TEST_ASSERT_NOT_NULL(pw->pw_name);
	TEST_ASSERT_GREATER_THAN_INT(0, (int)strlen(pw->pw_name));

	/* pw_dir and pw_shell shall not be NULL */
	TEST_ASSERT_NOT_NULL(pw->pw_dir);
	TEST_ASSERT_NOT_NULL(pw->pw_shell);
}


TEST(pwd_getpwent, setpwent_no_errno_on_success)
{
	errno = 0;
	setpwent();
	TEST_ASSERT_EQUAL_INT(0, errno);
}


TEST(pwd_getpwent, endpwent_no_errno_on_success)
{
#ifndef __phoenix__
	/* Linux is not POSIX compliant here */
	TEST_IGNORE();
#endif
	/* Open the database first */
	(void)getpwent();
	errno = 0;
	endpwent();
	TEST_ASSERT_EQUAL_INT(0, errno);
}


TEST_GROUP_RUNNER(pwd_getpwent)
{
	RUN_TEST_CASE(pwd_getpwent, getpwent_returns_first_entry);
	RUN_TEST_CASE(pwd_getpwent, getpwent_iterates_multiple_entries);
	RUN_TEST_CASE(pwd_getpwent, getpwent_returns_null_at_end);
	RUN_TEST_CASE(pwd_getpwent, setpwent_rewinds_database);
	RUN_TEST_CASE(pwd_getpwent, endpwent_allows_fresh_iteration);
	RUN_TEST_CASE(pwd_getpwent, getpwent_fields_valid);
	RUN_TEST_CASE(pwd_getpwent, setpwent_no_errno_on_success);
	RUN_TEST_CASE(pwd_getpwent, endpwent_no_errno_on_success);
}


TEST_GROUP(pwd_getpwnam_r);

TEST_SETUP(pwd_getpwnam_r)
{
}

TEST_TEAR_DOWN(pwd_getpwnam_r)
{
}


TEST(pwd_getpwnam_r, getpwnam_r_finds_root)
{
	struct passwd pwd;
	struct passwd *result = NULL;
	static char buf[PWD_BUF_SIZE];
	int ret;

	ret = getpwnam_r("root", &pwd, buf, sizeof(buf), &result);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_NOT_NULL(result);
	TEST_ASSERT_EQUAL_STRING("root", result->pw_name);
	TEST_ASSERT_EQUAL_UINT(0U, result->pw_uid);
}


TEST(pwd_getpwnam_r, getpwnam_r_not_found)
{
#ifndef __phoenix__
	/* Linux is not POSIX compliant here */
	TEST_IGNORE();
#endif
	struct passwd pwd;
	struct passwd *result = NULL;
	static char buf[PWD_BUF_SIZE];
	int ret;

	ret = getpwnam_r("nonexistent_user_xyzzy_12345", &pwd, buf, sizeof(buf), &result);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_NULL(result);
}


TEST(pwd_getpwnam_r, getpwnam_r_buffer_too_small)
{
	struct passwd pwd;
	struct passwd *result = NULL;
	char buf[2];
	int ret;

	ret = getpwnam_r("root", &pwd, buf, sizeof(buf), &result);
	TEST_ASSERT_EQUAL_INT(ERANGE, ret);
	TEST_ASSERT_NULL(result);
}


TEST(pwd_getpwnam_r, getpwnam_r_fields_populated)
{
	struct passwd pwd;
	struct passwd *result = NULL;
	static char buf[PWD_BUF_SIZE];
	int ret;

	ret = getpwnam_r("root", &pwd, buf, sizeof(buf), &result);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_NOT_NULL(result);
	TEST_ASSERT_NOT_NULL(result->pw_name);
	TEST_ASSERT_NOT_NULL(result->pw_dir);
	TEST_ASSERT_NOT_NULL(result->pw_shell);
	TEST_ASSERT_EQUAL_UINT(0U, result->pw_uid);
	TEST_ASSERT_EQUAL_UINT(0U, result->pw_gid);
}


TEST(pwd_getpwnam_r, getpwnam_r_current_user)
{
	struct passwd pwd;
	struct passwd *result = NULL;
	static char buf[PWD_BUF_SIZE];
	int ret;
	uid_t myUid;
	struct passwd *myPw;

	/* Get current user's name via getpwuid */
	myUid = getuid();
	myPw = getpwuid(myUid);
	TEST_ASSERT_NOT_NULL(myPw);
	TEST_ASSERT_NOT_NULL(myPw->pw_name);

	ret = getpwnam_r(myPw->pw_name, &pwd, buf, sizeof(buf), &result);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_NOT_NULL(result);
	TEST_ASSERT_EQUAL_STRING(myPw->pw_name, result->pw_name);
	TEST_ASSERT_EQUAL_UINT((unsigned int)myUid, (unsigned int)result->pw_uid);
}


TEST_GROUP_RUNNER(pwd_getpwnam_r)
{
	RUN_TEST_CASE(pwd_getpwnam_r, getpwnam_r_finds_root);
	RUN_TEST_CASE(pwd_getpwnam_r, getpwnam_r_not_found);
	RUN_TEST_CASE(pwd_getpwnam_r, getpwnam_r_buffer_too_small);
	RUN_TEST_CASE(pwd_getpwnam_r, getpwnam_r_fields_populated);
	RUN_TEST_CASE(pwd_getpwnam_r, getpwnam_r_current_user);
}


TEST_GROUP(pwd_getpwuid_r);

TEST_SETUP(pwd_getpwuid_r)
{
}

TEST_TEAR_DOWN(pwd_getpwuid_r)
{
}


TEST(pwd_getpwuid_r, getpwuid_r_finds_root)
{
	struct passwd pwd;
	struct passwd *result = NULL;
	static char buf[PWD_BUF_SIZE];
	int ret;

	ret = getpwuid_r(0, &pwd, buf, sizeof(buf), &result);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_NOT_NULL(result);
	TEST_ASSERT_EQUAL_STRING("root", result->pw_name);
	TEST_ASSERT_EQUAL_UINT(0U, result->pw_uid);
}


TEST(pwd_getpwuid_r, getpwuid_r_not_found)
{
#ifndef __phoenix__
	/* Linux is not POSIX compliant here */
	TEST_IGNORE();
#endif

	struct passwd pwd;
	struct passwd *result = NULL;
	static char buf[PWD_BUF_SIZE];
	int ret;

	/* Use an unlikely UID */
	ret = getpwuid_r(99999, &pwd, buf, sizeof(buf), &result);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_NULL(result);
}


TEST(pwd_getpwuid_r, getpwuid_r_buffer_too_small)
{
	struct passwd pwd;
	struct passwd *result = NULL;
	char buf[2];
	int ret;

	ret = getpwuid_r(0, &pwd, buf, sizeof(buf), &result);
	TEST_ASSERT_EQUAL_INT(ERANGE, ret);
	TEST_ASSERT_NULL(result);
}


TEST(pwd_getpwuid_r, getpwuid_r_current_user)
{
	struct passwd pwd;
	struct passwd *result = NULL;
	static char buf[PWD_BUF_SIZE];
	int ret;
	uid_t myUid;

	myUid = getuid();
	ret = getpwuid_r(myUid, &pwd, buf, sizeof(buf), &result);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_NOT_NULL(result);
	TEST_ASSERT_EQUAL_UINT((unsigned int)myUid, (unsigned int)result->pw_uid);
	TEST_ASSERT_NOT_NULL(result->pw_name);
	TEST_ASSERT_GREATER_THAN_INT(0, (int)strlen(result->pw_name));
}


TEST(pwd_getpwuid_r, getpwuid_r_fields_populated)
{
	struct passwd pwd;
	struct passwd *result = NULL;
	static char buf[PWD_BUF_SIZE];
	int ret;

	ret = getpwuid_r(0, &pwd, buf, sizeof(buf), &result);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_NOT_NULL(result);
	TEST_ASSERT_NOT_NULL(result->pw_name);
	TEST_ASSERT_NOT_NULL(result->pw_dir);
	TEST_ASSERT_NOT_NULL(result->pw_shell);
	TEST_ASSERT_EQUAL_UINT(0U, result->pw_gid);
}


TEST_GROUP_RUNNER(pwd_getpwuid_r)
{
	RUN_TEST_CASE(pwd_getpwuid_r, getpwuid_r_finds_root);
	RUN_TEST_CASE(pwd_getpwuid_r, getpwuid_r_not_found);
	RUN_TEST_CASE(pwd_getpwuid_r, getpwuid_r_buffer_too_small);
	RUN_TEST_CASE(pwd_getpwuid_r, getpwuid_r_current_user);
	RUN_TEST_CASE(pwd_getpwuid_r, getpwuid_r_fields_populated);
}
