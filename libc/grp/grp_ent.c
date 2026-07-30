/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <grp.h>
 * TESTED:
 *    - getgrent()
 *    - setgrent()
 *    - endgrent()
 *    - getgrnam_r()
 *    - getgrgid_r()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Lukasz Kruszynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define _XOPEN_SOURCE 700

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <grp.h>
#include <sys/types.h>

#include <unity_fixture.h>

/* Safety bound on database enumeration, guards against a broken cursor. */
#define GRP_SCAN_MAX      100000U
/* Buffer large enough for the small, memberless gid-0 group entry. */
#define GRP_FALLBACK_BUFSZ 1024
#define GRP_NAME_MAX      128

/* A group id and name that shall not correspond to any database entry. */
#define GRP_BOGUS_GID  ((gid_t)0x7ffffffeU)
#define GRP_BOGUS_NAME "no_such_group_zxq_98765"


/*
 * Fetch the name of the gid-0 group using the reentrant lookup. Returns 0 on
 * success, -1 if the entry is not available (no group database).
 */
static int test_gid0Name(char *out, size_t n)
{
	struct group grp;
	struct group *res = NULL;
	static char buf[GRP_FALLBACK_BUFSZ];

	if (getgrgid_r((gid_t)0, &grp, buf, sizeof(buf), &res) != 0) {
		return -1;
	}
	if (res == NULL) {
		return -1;
	}

	strncpy(out, grp.gr_name, n - 1);
	out[n - 1] = '\0';
	return 0;
}


/* ===== getgrent / setgrent / endgrent ===== */


TEST_GROUP(grp_getgrent);


TEST_SETUP(grp_getgrent)
{
}


TEST_TEAR_DOWN(grp_getgrent)
{
	endgrent();
}


/* getgrent() shall return successive broken-out group entries from the database. */
TEST(grp_getgrent, enumerates_entries)
{
	struct group *g;
	unsigned count = 0;
	int foundRoot = 0;

	setgrent();
	while ((g = getgrent()) != NULL) {
		TEST_ASSERT_NOT_NULL(g->gr_name);
		TEST_ASSERT_TRUE(g->gr_name[0] != '\0');
		TEST_ASSERT_NOT_NULL(g->gr_mem);
		if ((int)g->gr_gid == 0) {
			foundRoot = 1;
		}
		count++;
		if (count >= GRP_SCAN_MAX) {
			break;
		}
	}

	TEST_ASSERT_GREATER_THAN_UINT(0, count);
	TEST_ASSERT_TRUE_MESSAGE(foundRoot != 0, "group database has no gid 0 entry");
}


/* On end-of-file getgrent() shall return NULL and shall not change errno. */
TEST(grp_getgrent, eof_returns_null_without_errno)
{
	struct group *g;
	unsigned count = 0;

	setgrent();
	do {
		errno = 0;
		g = getgrent();
		count++;
	} while ((g != NULL) && (count < GRP_SCAN_MAX));

	TEST_ASSERT_NULL(g);
	TEST_ASSERT_EQUAL_INT(0, errno);
}


/* setgrent() shall rewind so the next getgrent() returns the first entry again. */
TEST(grp_getgrent, setgrent_rewinds)
{
	struct group *g;
	char firstName[GRP_NAME_MAX];
	gid_t firstGid;

	setgrent();
	g = getgrent();
	if (g == NULL) {
		TEST_IGNORE_MESSAGE("group database is empty");
	}
	strncpy(firstName, g->gr_name, sizeof(firstName) - 1);
	firstName[sizeof(firstName) - 1] = '\0';
	firstGid = g->gr_gid;

	/* Advance past the first entry. */
	(void)getgrent();
	(void)getgrent();

	setgrent();
	g = getgrent();
	TEST_ASSERT_NOT_NULL(g);
	TEST_ASSERT_EQUAL_STRING(firstName, g->gr_name);
	TEST_ASSERT_EQUAL_INT((int)firstGid, (int)g->gr_gid);
}


/* setgrent() and endgrent() shall not change errno when they succeed. */
TEST(grp_getgrent, setgrent_endgrent_preserve_errno)
{
	errno = 0;
	setgrent();
	TEST_ASSERT_EQUAL_INT(0, errno);

	errno = 0;
	endgrent();
	TEST_ASSERT_EQUAL_INT(0, errno);
}


/* ===== getgrnam_r ===== */


TEST_GROUP(grp_getgrnam_r);


TEST_SETUP(grp_getgrnam_r)
{
}


TEST_TEAR_DOWN(grp_getgrnam_r)
{
}


/* getgrnam_r() shall fill the caller structure for a matching name and succeed. */
TEST(grp_getgrnam_r, returns_entry_for_gid0_name)
{
	struct group grp;
	struct group *res = NULL;
	char name[GRP_NAME_MAX];
	static char buf[GRP_FALLBACK_BUFSZ];

	if (test_gid0Name(name, sizeof(name)) != 0) {
		TEST_IGNORE_MESSAGE("no gid 0 group available");
	}

	TEST_ASSERT_EQUAL_INT(0, getgrnam_r(name, &grp, buf, sizeof(buf), &res));
	TEST_ASSERT_EQUAL_PTR(&grp, res);
	TEST_ASSERT_EQUAL_STRING(name, grp.gr_name);
	TEST_ASSERT_EQUAL_INT(0, (int)grp.gr_gid);
	TEST_ASSERT_NOT_NULL(grp.gr_mem);
}


/* When the entry is not found getgrnam_r() shall return 0 with a NULL result. */
TEST(grp_getgrnam_r, not_found_returns_zero_null)
{
	struct group grp;
	struct group *res = &grp;
	static char buf[GRP_FALLBACK_BUFSZ];

	TEST_ASSERT_EQUAL_INT(0, getgrnam_r(GRP_BOGUS_NAME, &grp, buf, sizeof(buf), &res));
	TEST_ASSERT_NULL(res);
}


/* ===== getgrgid_r ===== */


TEST_GROUP(grp_getgrgid_r);


TEST_SETUP(grp_getgrgid_r)
{
}


TEST_TEAR_DOWN(grp_getgrgid_r)
{
}


/* getgrgid_r() shall fill the caller structure for a matching gid and succeed. */
TEST(grp_getgrgid_r, returns_entry_for_gid0)
{
	struct group grp;
	struct group *res = NULL;
	static char buf[GRP_FALLBACK_BUFSZ];

	TEST_ASSERT_EQUAL_INT(0, getgrgid_r((gid_t)0, &grp, buf, sizeof(buf), &res));
	if (res == NULL) {
		TEST_IGNORE_MESSAGE("no gid 0 group available");
	}

	TEST_ASSERT_EQUAL_PTR(&grp, res);
	TEST_ASSERT_EQUAL_INT(0, (int)grp.gr_gid);
	TEST_ASSERT_NOT_NULL(grp.gr_name);
	TEST_ASSERT_TRUE(grp.gr_name[0] != '\0');
	TEST_ASSERT_NOT_NULL(grp.gr_mem);
}


/* When the entry is not found getgrgid_r() shall return 0 with a NULL result. */
TEST(grp_getgrgid_r, not_found_returns_zero_null)
{
	struct group grp;
	struct group *res = &grp;
	static char buf[GRP_FALLBACK_BUFSZ];

	TEST_ASSERT_EQUAL_INT(0, getgrgid_r(GRP_BOGUS_GID, &grp, buf, sizeof(buf), &res));
	TEST_ASSERT_NULL(res);
}


TEST_GROUP_RUNNER(grp_getgrent)
{
	RUN_TEST_CASE(grp_getgrent, enumerates_entries);
	RUN_TEST_CASE(grp_getgrent, eof_returns_null_without_errno);
	RUN_TEST_CASE(grp_getgrent, setgrent_rewinds);
	RUN_TEST_CASE(grp_getgrent, setgrent_endgrent_preserve_errno);
}


TEST_GROUP_RUNNER(grp_getgrnam_r)
{
	RUN_TEST_CASE(grp_getgrnam_r, returns_entry_for_gid0_name);
	RUN_TEST_CASE(grp_getgrnam_r, not_found_returns_zero_null);
}


TEST_GROUP_RUNNER(grp_getgrgid_r)
{
	RUN_TEST_CASE(grp_getgrgid_r, returns_entry_for_gid0);
	RUN_TEST_CASE(grp_getgrgid_r, not_found_returns_zero_null);
}
