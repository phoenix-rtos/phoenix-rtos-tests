/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <grp.h>
 * TESTED:
 *    - getgrgid()
 *    - getgrnam()
 *    - getgrid_r()
 *    - getgrnam_r()
 *    - getgrent()
 *    - setgrent()
 *    - endgrent()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Damian Loewnau, Julian Uziembło
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <grp.h>
#include <sys/types.h>

#include <unity_fixture.h>

#define GRP_TEST_BUFSIZE 1024


/* ===== getgrgid group ===== */


TEST_GROUP(grp_getgrgid);


TEST_SETUP(grp_getgrgid)
{
}


TEST_TEAR_DOWN(grp_getgrgid)
{
}


/* getgrgid: shall return pointer to group struct for valid gid (root group, gid 0) */
TEST(grp_getgrgid, returns_group_for_gid_zero)
{
	struct group *grp;

	errno = 0;
	grp = getgrgid(0);
	TEST_ASSERT_NOT_NULL(grp);
	TEST_ASSERT_EQUAL_INT(0, (int)grp->gr_gid);
	TEST_ASSERT_NOT_NULL(grp->gr_name);
	TEST_ASSERT_TRUE(strlen(grp->gr_name) > 0);
}


/* getgrgid: shall return pointer with correct gr_gid for current process group */
TEST(grp_getgrgid, returns_group_for_current_gid)
{
	struct group *grp;
	gid_t gid;

	gid = getgid();

	errno = 0;
	grp = getgrgid(gid);
	TEST_ASSERT_NOT_NULL(grp);
	TEST_ASSERT_EQUAL_INT((int)gid, (int)grp->gr_gid);
	TEST_ASSERT_NOT_NULL(grp->gr_name);
}


/* getgrgid: gr_mem shall be a non-NULL pointer (array of member names) */
TEST(grp_getgrgid, gr_mem_not_null)
{
	struct group *grp;

	grp = getgrgid(0);
	TEST_ASSERT_NOT_NULL(grp);
	TEST_ASSERT_NOT_NULL(grp->gr_mem);
}


/* getgrgid: shall return NULL for nonexistent gid, errno unchanged */
TEST(grp_getgrgid, returns_null_nonexistent_gid)
{
	struct group *grp;

	errno = 0;
	grp = getgrgid(65534U);
	if (grp == NULL) {
		/* POSIX: if not found, errno shall not be changed */
		TEST_ASSERT_EQUAL_INT(0, errno);
	}
	/* Some systems may have gid 65534 (nobody) — if found, just verify structure */
	if (grp != NULL) {
		TEST_ASSERT_EQUAL_INT(65534, (int)grp->gr_gid);
	}
}


/* getgrgid: shall return NULL for very large nonexistent gid */
TEST(grp_getgrgid, returns_null_very_large_gid)
{
	struct group *grp;

	errno = 0;
	grp = getgrgid((gid_t)99999U);
	if (grp == NULL) {
		TEST_ASSERT_EQUAL_INT(0, errno);
	}
}


/* getgrgid: subsequent call may overwrite previous result */
TEST(grp_getgrgid, subsequent_call_may_overwrite)
{
	struct group *grp1;
	struct group *grp2;
	gid_t gid;
	char name1[128];

	grp1 = getgrgid(0);
	TEST_ASSERT_NOT_NULL(grp1);
	strncpy(name1, grp1->gr_name, sizeof(name1) - 1);
	name1[sizeof(name1) - 1] = '\0';

	gid = getgid();
	grp2 = getgrgid(gid);
	/* grp1 pointer may now be invalid — only verify grp2 */
	if (grp2 != NULL) {
		TEST_ASSERT_EQUAL_INT((int)gid, (int)grp2->gr_gid);
	}
	/* Confirm that we successfully read name1 before the overwrite */
	TEST_ASSERT_TRUE(strlen(name1) > 0);
}


TEST_GROUP_RUNNER(grp_getgrgid)
{
	RUN_TEST_CASE(grp_getgrgid, returns_group_for_gid_zero);
	RUN_TEST_CASE(grp_getgrgid, returns_group_for_current_gid);
	RUN_TEST_CASE(grp_getgrgid, gr_mem_not_null);
	RUN_TEST_CASE(grp_getgrgid, returns_null_nonexistent_gid);
	RUN_TEST_CASE(grp_getgrgid, returns_null_very_large_gid);
	RUN_TEST_CASE(grp_getgrgid, subsequent_call_may_overwrite);
}


/* ===== getgrnam group ===== */


TEST_GROUP(grp_getgrnam);


TEST_SETUP(grp_getgrnam)
{
}


TEST_TEAR_DOWN(grp_getgrnam)
{
}


/* getgrnam: shall return pointer to group struct for "root" (or equivalent gid-0 group) */
TEST(grp_getgrnam, returns_group_for_root)
{
	struct group *grp;
	struct group *grp0;
	const char *name;

	/* Get name of gid 0 group first (may be "root", "wheel", etc.) */
	grp0 = getgrgid(0);
	TEST_ASSERT_NOT_NULL(grp0);
	name = grp0->gr_name;

	errno = 0;
	grp = getgrnam(name);
	TEST_ASSERT_NOT_NULL(grp);
	TEST_ASSERT_EQUAL_STRING(name, grp->gr_name);
	TEST_ASSERT_EQUAL_INT(0, (int)grp->gr_gid);
}


/* getgrnam: shall return correct structure for current process group name */
TEST(grp_getgrnam, returns_group_for_current_group)
{
	struct group *grpById;
	struct group *grpByName;
	gid_t gid;
	char name[128];

	gid = getgid();
	grpById = getgrgid(gid);
	TEST_ASSERT_NOT_NULL(grpById);
	strncpy(name, grpById->gr_name, sizeof(name) - 1);
	name[sizeof(name) - 1] = '\0';

	errno = 0;
	grpByName = getgrnam(name);
	TEST_ASSERT_NOT_NULL(grpByName);
	TEST_ASSERT_EQUAL_STRING(name, grpByName->gr_name);
	TEST_ASSERT_EQUAL_INT((int)gid, (int)grpByName->gr_gid);
}


/* getgrnam: gr_mem shall be a non-NULL pointer */
TEST(grp_getgrnam, gr_mem_not_null)
{
	struct group *grp0;
	struct group *grp;

	grp0 = getgrgid(0);
	TEST_ASSERT_NOT_NULL(grp0);

	grp = getgrnam(grp0->gr_name);
	TEST_ASSERT_NOT_NULL(grp);
	TEST_ASSERT_NOT_NULL(grp->gr_mem);
}


/* getgrnam: shall return NULL for nonexistent name, errno unchanged */
TEST(grp_getgrnam, returns_null_nonexistent_name)
{
	struct group *grp;

	errno = 0;
	grp = getgrnam("totally_nonexistent_group_xyz_12345");
	TEST_ASSERT_NULL(grp);
	TEST_ASSERT_EQUAL_INT(0, errno);
}


/* getgrnam: empty string shall return NULL */
TEST(grp_getgrnam, returns_null_empty_string)
{
	struct group *grp;

	errno = 0;
	grp = getgrnam("");
	TEST_ASSERT_NULL(grp);
	TEST_ASSERT_EQUAL_INT(0, errno);
}


/* getgrnam: subsequent call may overwrite previous result */
TEST(grp_getgrnam, subsequent_call_may_overwrite)
{
	struct group *grp0;
	struct group *grp1;
	struct group *grp2;
	char name0[128];
	gid_t gid;
	char nameGid[128];

	grp0 = getgrgid(0);
	TEST_ASSERT_NOT_NULL(grp0);
	strncpy(name0, grp0->gr_name, sizeof(name0) - 1);
	name0[sizeof(name0) - 1] = '\0';

	gid = getgid();
	grp0 = getgrgid(gid);
	TEST_ASSERT_NOT_NULL(grp0);
	strncpy(nameGid, grp0->gr_name, sizeof(nameGid) - 1);
	nameGid[sizeof(nameGid) - 1] = '\0';

	grp1 = getgrnam(name0);
	TEST_ASSERT_NOT_NULL(grp1);

	grp2 = getgrnam(nameGid);
	/* grp1 may be invalidated now — only verify grp2 */
	if (grp2 != NULL) {
		TEST_ASSERT_EQUAL_STRING(nameGid, grp2->gr_name);
	}
}


TEST_GROUP_RUNNER(grp_getgrnam)
{
	RUN_TEST_CASE(grp_getgrnam, returns_group_for_root);
	RUN_TEST_CASE(grp_getgrnam, returns_group_for_current_group);
	RUN_TEST_CASE(grp_getgrnam, gr_mem_not_null);
	RUN_TEST_CASE(grp_getgrnam, returns_null_nonexistent_name);
	RUN_TEST_CASE(grp_getgrnam, returns_null_empty_string);
	RUN_TEST_CASE(grp_getgrnam, subsequent_call_may_overwrite);
}


/* ===== getgrgid_r group ===== */

TEST_GROUP(grp_getgrgid_r);


TEST_SETUP(grp_getgrgid_r)
{
}


TEST_TEAR_DOWN(grp_getgrgid_r)
{
}


/* getgrgid_r: shall return group for gid 0 */
TEST(grp_getgrgid_r, returns_group_for_gid_zero)
{
	struct group grp;
	struct group *result;
	char buf[GRP_TEST_BUFSIZE];
	int ret;

	ret = getgrgid_r(0, &grp, buf, sizeof(buf), &result);

	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_NOT_NULL(result);
	TEST_ASSERT_EQUAL_INT(0, (int)grp.gr_gid);
	TEST_ASSERT_NOT_NULL(grp.gr_name);
	TEST_ASSERT_TRUE(strlen(grp.gr_name) > 0);
	TEST_ASSERT_NOT_NULL(grp.gr_mem);
}


/* getgrgid_r: shall return current process group */
TEST(grp_getgrgid_r, returns_current_gid)
{
	struct group grp;
	struct group *result;
	char buf[GRP_TEST_BUFSIZE];
	gid_t gid;
	int ret;

	gid = getgid();

	ret = getgrgid_r(gid, &grp, buf, sizeof(buf), &result);

	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_NOT_NULL(result);
	TEST_ASSERT_EQUAL_INT((int)gid, (int)grp.gr_gid);
}


/* getgrgid_r: nonexistent gid shall return not found */
TEST(grp_getgrgid_r, nonexistent_gid)
{
	struct group grp;
	struct group *result;
	char buf[GRP_TEST_BUFSIZE];
	int ret;

	ret = getgrgid_r((gid_t)99999U, &grp, buf, sizeof(buf), &result);

	TEST_ASSERT_TRUE((ret == 0) || (ret == ENOENT));
	TEST_ASSERT_NULL(result);
}


/* getgrgid_r: too small buffer shall fail with ERANGE */
TEST(grp_getgrgid_r, small_buffer)
{
	struct group grp;
	struct group *result;
	char buf[4];
	int ret;

	ret = getgrgid_r(0, &grp, buf, sizeof(buf), &result);

	TEST_ASSERT_EQUAL_INT(ERANGE, ret);
	TEST_ASSERT_NULL(result);
}


TEST_GROUP_RUNNER(grp_getgrgid_r)
{
	RUN_TEST_CASE(grp_getgrgid_r, returns_group_for_gid_zero);
	RUN_TEST_CASE(grp_getgrgid_r, returns_current_gid);
	RUN_TEST_CASE(grp_getgrgid_r, nonexistent_gid);
	RUN_TEST_CASE(grp_getgrgid_r, small_buffer);
}


/* ===== getgrnam_r group ===== */

TEST_GROUP(grp_getgrnam_r);


TEST_SETUP(grp_getgrnam_r)
{
}


TEST_TEAR_DOWN(grp_getgrnam_r)
{
}


/* getgrnam_r: shall return group for gid0 name */
TEST(grp_getgrnam_r, returns_root_group)
{
	struct group grp0;
	struct group grp;
	struct group *result;
	char buf0[GRP_TEST_BUFSIZE];
	char buf[GRP_TEST_BUFSIZE];
	int ret;

	TEST_ASSERT_EQUAL_INT(0, getgrgid_r(0, &grp0, buf0, sizeof(buf0), &result));
	TEST_ASSERT_NOT_NULL(result);
	TEST_ASSERT_NOT_NULL(grp0.gr_name);

	ret = getgrnam_r(grp0.gr_name, &grp, buf, sizeof(buf), &result);

	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_NOT_NULL(result);
	TEST_ASSERT_EQUAL_INT(0, (int)grp.gr_gid);
	TEST_ASSERT_EQUAL_STRING(grp0.gr_name, grp.gr_name);
}


/* getgrnam_r: nonexistent name */
TEST(grp_getgrnam_r, nonexistent_name)
{
	struct group grp;
	struct group *result;
	char buf[GRP_TEST_BUFSIZE];
	int ret;

	ret = getgrnam_r("definitely_not_existing_group", &grp, buf, sizeof(buf), &result);

	TEST_ASSERT_TRUE((ret == 0) || (ret == ENOENT));
	TEST_ASSERT_NULL(result);
}


/* getgrnam_r: too small buffer */
TEST(grp_getgrnam_r, small_buffer)
{
	struct group grp;
	struct group *result;
	char buf[4];
	int ret;

	ret = getgrnam_r("root", &grp, buf, sizeof(buf), &result);

	TEST_ASSERT_EQUAL_INT(ERANGE, ret);
	TEST_ASSERT_NULL(result);
}


TEST_GROUP_RUNNER(grp_getgrnam_r)
{
	RUN_TEST_CASE(grp_getgrnam_r, returns_root_group);
	RUN_TEST_CASE(grp_getgrnam_r, nonexistent_name);
	RUN_TEST_CASE(grp_getgrnam_r, small_buffer);
}

/* ===== getgrent group ===== */

TEST_GROUP(grp_getgrent);


TEST_SETUP(grp_getgrent)
{
	setgrent();
}


TEST_TEAR_DOWN(grp_getgrent)
{
	endgrent();
}


/* getgrent: shall return at least one group */
TEST(grp_getgrent, first_entry_exists)
{
	struct group *grp;

	grp = getgrent();

	TEST_ASSERT_NOT_NULL(grp);
	TEST_ASSERT_NOT_NULL(grp->gr_name);
	TEST_ASSERT_NOT_NULL(grp->gr_mem);
}


/* setgrent: shall rewind enumeration */
TEST(grp_getgrent, setgrent_rewinds)
{
	struct group *grp1;
	struct group *grp2;
	char first[128];
	int count = 0;

	grp1 = getgrent();
	TEST_ASSERT_NOT_NULL(grp1);
	TEST_ASSERT_NOT_NULL(grp1->gr_name);

	strncpy(first, grp1->gr_name, sizeof(first) - 1);
	first[sizeof(first) - 1] = '\0';

	while (getgrent() != NULL) {
		/* avoid infinite loop if implementation is broken */
		TEST_ASSERT_TRUE(count++ < 10000);
	}

	setgrent();

	grp2 = getgrent();

	TEST_ASSERT_NOT_NULL(grp2);
	TEST_ASSERT_EQUAL_STRING(first, grp2->gr_name);
}


/* getgrent: iteration eventually reaches end */
TEST(grp_getgrent, iteration_reaches_end)
{
	struct group *grp;
	int count = 0;

	while ((grp = getgrent()) != NULL) {
		TEST_ASSERT_NOT_NULL(grp->gr_name);

		/* avoid infinite loop if implementation is broken */
		TEST_ASSERT_TRUE(count++ < 10000);
	}

	TEST_ASSERT_TRUE(count > 0);
}


/* endgrent: shall allow reopening enumeration */
TEST(grp_getgrent, endgrent_then_setgrent)
{
	struct group *grp;

	endgrent();

	setgrent();

	grp = getgrent();

	TEST_ASSERT_NOT_NULL(grp);
}


TEST_GROUP_RUNNER(grp_getgrent)
{
	RUN_TEST_CASE(grp_getgrent, first_entry_exists);
	RUN_TEST_CASE(grp_getgrent, setgrent_rewinds);
	RUN_TEST_CASE(grp_getgrent, iteration_reaches_end);
	RUN_TEST_CASE(grp_getgrent, endgrent_then_setgrent);
}
