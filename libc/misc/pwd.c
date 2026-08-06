/*
 * Phoenix-RTOS
 *
 * libc/misc/pwd
 *
 * Tests for password-related functions
 *
 * Copyright 2021, 2026 Phoenix Systems
 * Author: Mateusz Niewiadomski, Julian Uziembło
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <stdbool.h>
#include <errno.h>
#include <limits.h>
#include <pwd.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include <unity_fixture.h>


static struct {
	char pw_name[NAME_MAX];
	char pw_gecos[128];
	char pw_dir[PATH_MAX];
	char pw_shell[PATH_MAX];
	char pw_passwd[128];
} commonBufs;


static struct passwd defaultPwBuf = {
	.pw_name = commonBufs.pw_name,
	.pw_passwd = commonBufs.pw_passwd,
	.pw_dir = commonBufs.pw_dir,
	.pw_shell = commonBufs.pw_shell,
	.pw_gecos = commonBufs.pw_gecos,
};


#ifdef __phoenix__
#define ROOT_WORKDIR "/"
#define ROOT_SHELL   "/bin/sh"
#else
#define ROOT_WORKDIR "/root"
#define ROOT_SHELL   "/bin/bash"
#endif

static bool ispasswdfile;


TEST_GROUP(pwd);


TEST_SETUP(pwd)
{
}


TEST_TEAR_DOWN(pwd)
{
	endpwent();
}


static void validateEntry(const struct passwd *pw)
{
	TEST_ASSERT_NOT_NULL(pw);
	TEST_ASSERT_NOT_NULL(pw->pw_name);
	TEST_ASSERT_GREATER_THAN(0, strlen(pw->pw_name));
	TEST_ASSERT_NOT_NULL(pw->pw_passwd);
	TEST_ASSERT_NOT_NULL(pw->pw_gecos);
	TEST_ASSERT_NOT_NULL(pw->pw_dir);
	TEST_ASSERT_NOT_NULL(pw->pw_shell);
}


static void assertPwdEnd(void)
{
	int saved = errno;
	endpwent();
	TEST_ASSERT_EQUAL(saved, errno);
}


static inline void strncpyNull(char **dst, char *src, size_t maxlen)
{
	if (src != NULL) {
		strncpy(*dst, src, maxlen - 1);
		(*dst)[maxlen - 1] = '\0';
	}
	else {
		*dst = NULL;
	}
}


static void pwdcpy(struct passwd *dst, const struct passwd *src)
{
	strncpyNull(&dst->pw_name, src->pw_name, sizeof(commonBufs.pw_name));
	strncpyNull(&dst->pw_passwd, src->pw_passwd, sizeof(commonBufs.pw_passwd));
	dst->pw_uid = src->pw_uid;
	dst->pw_gid = src->pw_gid;
	strncpyNull(&dst->pw_dir, src->pw_dir, sizeof(commonBufs.pw_dir));
	strncpyNull(&dst->pw_gecos, src->pw_gecos, sizeof(commonBufs.pw_gecos));
	strncpyNull(&dst->pw_shell, src->pw_shell, sizeof(commonBufs.pw_shell));
}


static void assertEqualPwd(struct passwd *pwd1, const struct passwd *pwd2)
{
	TEST_ASSERT_EQUAL_STRING(pwd1->pw_name, pwd2->pw_name);
	TEST_ASSERT_EQUAL_STRING(pwd1->pw_passwd, pwd2->pw_passwd);
	TEST_ASSERT_EQUAL(pwd1->pw_uid, pwd2->pw_uid);
	TEST_ASSERT_EQUAL(pwd1->pw_gid, pwd2->pw_gid);
	TEST_ASSERT_EQUAL_STRING(pwd1->pw_dir, pwd2->pw_dir);
	TEST_ASSERT_EQUAL_STRING(pwd1->pw_gecos, pwd2->pw_gecos);
	TEST_ASSERT_EQUAL_STRING(pwd1->pw_shell, pwd2->pw_shell);
}


TEST(pwd, getpwent_basic)
{
	if (!ispasswdfile) {
		TEST_IGNORE_MESSAGE("No /etc/passwd file!");
	}

	struct passwd *pw;
	int count = 0;

	setpwent();

	while ((pw = getpwent()) != NULL) {
		validateEntry(pw);

		count++;

		/* protect against broken infinite iteration */
		TEST_ASSERT_LESS_THAN(1000, count);
	}

	TEST_ASSERT_GREATER_THAN(0, count);

	assertPwdEnd();
}


TEST(pwd, setpwent)
{
	if (!ispasswdfile) {
		TEST_IGNORE_MESSAGE("No /etc/passwd file!");
	}

	struct passwd *ptr1, *ptr2, pwd = defaultPwBuf;

	setpwent();

	ptr1 = getpwent();

	TEST_ASSERT_NOT_NULL(ptr1);

	pwdcpy(&pwd, ptr1);

	/* advance stream */
	(void)getpwent();
	(void)getpwent();

	setpwent();

	ptr2 = getpwent();

	TEST_ASSERT_NOT_NULL(ptr2);

	TEST_ASSERT_EQUAL_PTR(ptr1, ptr2);
	assertEqualPwd(&pwd, ptr2);

	assertPwdEnd();
}


TEST(pwd, endpwent)
{
	if (!ispasswdfile) {
		TEST_IGNORE_MESSAGE("No /etc/passwd file!");
	}

	struct passwd *pw;

	setpwent();

	pw = getpwent();

	TEST_ASSERT_NOT_NULL(pw);

	endpwent();

	pw = getpwent();

	TEST_ASSERT_NOT_NULL(pw);

	validateEntry(pw);

	assertPwdEnd();
}


TEST(pwd, getpwent_eof)
{
	if (!ispasswdfile) {
		TEST_IGNORE_MESSAGE("No /etc/passwd file!");
	}

	struct passwd *pw;
	int limit = 0;

	setpwent();

	while ((pw = getpwent()) != NULL) {
		limit++;

		TEST_ASSERT_LESS_THAN(1000, limit);
	}

	/* EOF should remain EOF */
	pw = getpwent();

	TEST_ASSERT_NULL(pw);

	assertPwdEnd();
}


TEST(pwd, getpwnam_root)
{
	if (!ispasswdfile) {
		TEST_IGNORE_MESSAGE("No /etc/passwd file!");
	}

	struct passwd *pw;

	errno = 0;

	pw = getpwnam("root");

	TEST_ASSERT_NOT_NULL(pw);

	validateEntry(pw);

	TEST_ASSERT_EQUAL_STRING("root", pw->pw_name);
	TEST_ASSERT_EQUAL_INT(0, pw->pw_uid);
}


TEST(pwd, getpwnam_nonexistent)
{
	if (!ispasswdfile) {
		TEST_IGNORE_MESSAGE("No /etc/passwd file!");
	}

	struct passwd *pw;

	errno = 0;

	pw = getpwnam("phoenix_rtos_nonexistent_user_2137");

	TEST_ASSERT_NULL(pw);
}


TEST(pwd, getpwnam_consistent)
{
	if (!ispasswdfile) {
		TEST_IGNORE_MESSAGE("No /etc/passwd file!");
	}

	struct passwd *ptr1, *ptr2, pwd = defaultPwBuf;

	ptr1 = getpwnam("root");

	TEST_ASSERT_NOT_NULL(ptr1);

	pwdcpy(&pwd, ptr1);

	ptr2 = getpwnam("root");

	TEST_ASSERT_EQUAL_PTR(ptr1, ptr2);
	assertEqualPwd(&pwd, ptr2);
}


TEST(pwd, getpwnam_r_root)
{
	if (!ispasswdfile) {
		TEST_IGNORE_MESSAGE("No /etc/passwd file!");
	}

	struct passwd pwd;
	struct passwd *result = NULL;
	char buffer[1024];
	int ret;

	memset(&pwd, 0, sizeof(pwd));
	memset(buffer, 0, sizeof(buffer));

	errno = 0;

	ret = getpwnam_r("root", &pwd, buffer, sizeof(buffer), &result);

	TEST_ASSERT_EQUAL_INT(0, ret);

	TEST_ASSERT_NOT_NULL(result);
	TEST_ASSERT_EQUAL_PTR(&pwd, result);

	TEST_ASSERT_EQUAL_STRING("root", pwd.pw_name);
	TEST_ASSERT_EQUAL_INT(0, pwd.pw_uid);

	validateEntry(&pwd);
}


TEST(pwd, getpwnam_r_nonexistent)
{
	if (!ispasswdfile) {
		TEST_IGNORE_MESSAGE("No /etc/passwd file!");
	}

	struct passwd pwd;
	struct passwd *result = (struct passwd *)0xdeadbeef;
	char buffer[1024];
	int ret;

	memset(&pwd, 0, sizeof(pwd));
	memset(buffer, 0, sizeof(buffer));

	errno = 0;

	ret = getpwnam_r(
			"phoenix_rtos_nonexistent_user_2137",
			&pwd,
			buffer,
			sizeof(buffer),
			&result);

	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_NULL(result);
}


TEST(pwd, getpwnam_r_small_buf)
{
	if (!ispasswdfile) {
		TEST_IGNORE_MESSAGE("No /etc/passwd file!");
	}

	struct passwd pwd;
	struct passwd *result = NULL;
	char buffer[2];
	int ret;

	memset(&pwd, 0, sizeof(pwd));

	errno = 0;

	ret = getpwnam_r("root", &pwd, buffer, sizeof(buffer), &result);

	TEST_ASSERT_EQUAL_INT(ERANGE, ret);
	TEST_ASSERT_NULL(result);
}


TEST(pwd, getpwnam_r_consistent)
{
	if (!ispasswdfile) {
		TEST_IGNORE_MESSAGE("No /etc/passwd file!");
	}

	struct passwd pwd1;
	struct passwd pwd2;

	struct passwd *res1 = NULL;
	struct passwd *res2 = NULL;

	char buf1[1024];
	char buf2[1024];

	int ret;

	ret = getpwnam_r("root", &pwd1, buf1, sizeof(buf1), &res1);

	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_NOT_NULL(res1);

	ret = getpwnam_r("root", &pwd2, buf2, sizeof(buf2), &res2);

	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_NOT_NULL(res2);

	assertEqualPwd(&pwd1, &pwd2);
}


TEST(pwd, getpwuid_root)
{
	if (!ispasswdfile) {
		TEST_IGNORE_MESSAGE("No /etc/passwd file!");
	}

	struct passwd *pw;

	errno = 0;

	pw = getpwuid(0);

	TEST_ASSERT_NOT_NULL(pw);

	validateEntry(pw);

	TEST_ASSERT_EQUAL_INT(0, pw->pw_uid);
	TEST_ASSERT_EQUAL_STRING("root", pw->pw_name);
}


TEST(pwd, getpwuid_nonexistent)
{
	if (!ispasswdfile) {
		TEST_IGNORE_MESSAGE("No /etc/passwd file!");
	}

	struct passwd *pw;

	errno = 0;

	pw = getpwuid((uid_t)999999);

	TEST_ASSERT_NULL(pw);
}


TEST(pwd, getpwuid_matches_getpwnam)
{
	if (!ispasswdfile) {
		TEST_IGNORE_MESSAGE("No /etc/passwd file!");
	}

	struct passwd *by_name, *by_uid, pwd = defaultPwBuf;

	by_name = getpwnam("root");

	TEST_ASSERT_NOT_NULL(by_name);

	pwdcpy(&pwd, by_name);

	by_uid = getpwuid(by_name->pw_uid);

	TEST_ASSERT_NOT_NULL(by_uid);

	assertEqualPwd(&pwd, by_uid);
}


TEST(pwd, getpwuid_r_root)
{
	if (!ispasswdfile) {
		TEST_IGNORE_MESSAGE("No /etc/passwd file!");
	}

	struct passwd pwd;
	struct passwd *result = NULL;

	char buffer[1024];

	int ret;

	memset(&pwd, 0, sizeof(pwd));
	memset(buffer, 0, sizeof(buffer));

	errno = 0;

	ret = getpwuid_r(0, &pwd, buffer, sizeof(buffer), &result);

	TEST_ASSERT_EQUAL_INT(0, ret);

	TEST_ASSERT_NOT_NULL(result);
	TEST_ASSERT_EQUAL_PTR(&pwd, result);

	validateEntry(&pwd);

	TEST_ASSERT_EQUAL_INT(0, pwd.pw_uid);
	TEST_ASSERT_EQUAL_STRING("root", pwd.pw_name);
}


TEST(pwd, getpwuid_r_nonexistent)
{
	if (!ispasswdfile) {
		TEST_IGNORE_MESSAGE("No /etc/passwd file!");
	}

	struct passwd pwd;
	struct passwd *result = (struct passwd *)0xdeadbeef;

	char buffer[1024];

	int ret;

	memset(&pwd, 0, sizeof(pwd));
	memset(buffer, 0, sizeof(buffer));

	errno = 0;

	ret = getpwuid_r(
			(uid_t)999999,
			&pwd,
			buffer,
			sizeof(buffer),
			&result);

	TEST_ASSERT_EQUAL_INT(0, ret);

	TEST_ASSERT_NULL(result);
}


TEST(pwd, getpwuid_r_small_buf)
{
	if (!ispasswdfile) {
		TEST_IGNORE_MESSAGE("No /etc/passwd file!");
	}

	struct passwd pwd;
	struct passwd *result = NULL;

	char buffer[2];

	int ret;

	memset(&pwd, 0, sizeof(pwd));

	errno = 0;

	ret = getpwuid_r(0, &pwd, buffer, sizeof(buffer), &result);

	TEST_ASSERT_EQUAL_INT(ERANGE, ret);

	TEST_ASSERT_NULL(result);
}


TEST(pwd, getpwuid_r_consistent)
{
	if (!ispasswdfile) {
		TEST_IGNORE_MESSAGE("No /etc/passwd file!");
	}

	struct passwd pwd1;
	struct passwd pwd2;

	struct passwd *res1 = NULL;
	struct passwd *res2 = NULL;

	char buf1[1024];
	char buf2[1024];

	int ret;

	ret = getpwuid_r(0, &pwd1, buf1, sizeof(buf1), &res1);

	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_NOT_NULL(res1);

	ret = getpwuid_r(0, &pwd2, buf2, sizeof(buf2), &res2);

	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_NOT_NULL(res2);

	assertEqualPwd(&pwd1, &pwd2);
}


TEST(pwd, getpwuid_r_separate_buffers)
{
	if (!ispasswdfile) {
		TEST_IGNORE_MESSAGE("No /etc/passwd file!");
	}

	struct passwd pwd1;
	struct passwd pwd2;

	struct passwd *res1 = NULL;
	struct passwd *res2 = NULL;

	char buf1[1024];
	char buf2[1024];

	int ret;

	ret = getpwuid_r(0, &pwd1, buf1, sizeof(buf1), &res1);

	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_NOT_NULL(res1);

	ret = getpwuid_r(0, &pwd2, buf2, sizeof(buf2), &res2);

	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_NOT_NULL(res2);

	TEST_ASSERT_TRUE(pwd1.pw_name != pwd2.pw_name);
	TEST_ASSERT_TRUE(pwd1.pw_dir != pwd2.pw_dir);
	TEST_ASSERT_TRUE(pwd1.pw_shell != pwd2.pw_shell);
}


/* Get root account details by name */
TEST(pwd, getpwnam_getroot)
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
TEST(pwd, getpwuid_getroot)
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
TEST(pwd, getpwnam_getnull)
{
	TEST_ASSERT_NULL(getpwnam("loremipsum"));
	// FIXME: invalid test
	// TEST_ASSERT_NULL(getpwnam(NULL));
	/* TODO: add errno check */

	if (!ispasswdfile)
		TEST_IGNORE_MESSAGE("No /etc/passwd file!");
}


/* Try to reach for non-existing user data with getpwuid */
TEST(pwd, getpwuid_getnull)
{
	TEST_ASSERT_NULL(getpwuid((uid_t)-1));
	TEST_ASSERT_NULL(getpwuid((uid_t)65535));
	/* TODO: add errno check */

	if (!ispasswdfile)
		TEST_IGNORE_MESSAGE("No /etc/passwd file!");
}


/* Try to reach for root user data with no /etc/passwd file with getpwnam */
TEST(pwd, getpwnam_nopasswdfile)
{
	if (ispasswdfile)
		rename("/etc/passwd", "/etc/passwd_del");

	TEST_ASSERT_NULL(getpwnam("root"));
	/* TODO: add errno check */

	if (ispasswdfile)
		rename("/etc/passwd_del", "/etc/passwd");
}


/* Try to reach for root user data with no /etc/passwd file with getpwuid */
TEST(pwd, getpwuid_nopasswdfile)
{
	if (ispasswdfile)
		rename("/etc/passwd", "/etc/passwd_del");

	TEST_ASSERT_NULL(getpwuid((uid_t)0));
	/* TODO: add errno check */

	if (ispasswdfile)
		rename("/etc/passwd_del", "/etc/passwd");
}


TEST_GROUP_RUNNER(pwd)
{
	ispasswdfile = access("/etc/passwd", F_OK) == 0;

	RUN_TEST_CASE(pwd, getpwnam_getroot);
	RUN_TEST_CASE(pwd, getpwuid_getroot);
	RUN_TEST_CASE(pwd, getpwnam_getnull);
	RUN_TEST_CASE(pwd, getpwuid_getnull);
	/* we can't rename/delete the /etc/passwd file on host */
#ifdef __phoenix__
	RUN_TEST_CASE(pwd, getpwnam_nopasswdfile);
	RUN_TEST_CASE(pwd, getpwuid_nopasswdfile);
#endif
	RUN_TEST_CASE(pwd, getpwent_basic);
	RUN_TEST_CASE(pwd, setpwent);
	RUN_TEST_CASE(pwd, endpwent);
	RUN_TEST_CASE(pwd, getpwent_eof);

	RUN_TEST_CASE(pwd, getpwnam_root);
	RUN_TEST_CASE(pwd, getpwnam_nonexistent);
	RUN_TEST_CASE(pwd, getpwnam_consistent);

	RUN_TEST_CASE(pwd, getpwnam_r_root);
	RUN_TEST_CASE(pwd, getpwnam_r_nonexistent);
	RUN_TEST_CASE(pwd, getpwnam_r_small_buf);
	RUN_TEST_CASE(pwd, getpwnam_r_consistent);

	RUN_TEST_CASE(pwd, getpwuid_root);
	RUN_TEST_CASE(pwd, getpwuid_nonexistent);
	RUN_TEST_CASE(pwd, getpwuid_matches_getpwnam);

	RUN_TEST_CASE(pwd, getpwuid_r_root);
	RUN_TEST_CASE(pwd, getpwuid_r_nonexistent);
	RUN_TEST_CASE(pwd, getpwuid_r_small_buf);
	RUN_TEST_CASE(pwd, getpwuid_r_consistent);
	RUN_TEST_CASE(pwd, getpwuid_r_separate_buffers);
}
