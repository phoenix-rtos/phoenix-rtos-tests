/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - ctype.h
 * TESTED:
 *    - isalnum(), isalpha(), isascii(), isblank(),
 *    - iscntrl(), isdigit(), isgraph(), islower(),
 *    - isprint(), ispunct(), isspace(), isupper(),
 *    - isxdigit(), toascii(), tolower(), toupper(),
 *    - _toupper(), _tolower()
 *
 * Copyright 2023 Phoenix Systems
 * Author: Mateusz Bloch
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include <unity_fixture.h>


#define MIN_VALUE 0
#define MAX_VALUE 255

/*
Test group for:
* isalnum, isalpha, isascii,
* isblank, iscntrl, isdigit
* isgraph, islower, isprint
* ispunct, isspace, isupper
* isxdigit, toascii, tolower
* toupper, _tolower, _toupper
*/


TEST_GROUP(ctype);


TEST_SETUP(ctype)
{
}


TEST_TEAR_DOWN(ctype)
{
}


TEST(ctype, isalnum)
{
	for (int i = MIN_VALUE; i <= MAX_VALUE; i++) {
		if ((i >= '0' && i <= '9') || (i >= 'A' && i <= 'Z') || (i >= 'a' && i <= 'z')) {
			TEST_ASSERT_TRUE(isalnum(i));
		}
		else {
			TEST_ASSERT_FALSE(isalnum(i));
		}
	}

	TEST_ASSERT_FALSE(isalnum(EOF));
}


TEST(ctype, isalpha)
{
	for (int i = MIN_VALUE; i <= MAX_VALUE; i++) {
		if ((i >= 'A' && i <= 'Z') || (i >= 'a' && i <= 'z')) {
			TEST_ASSERT_TRUE(isalpha(i));
		}
		else {
			TEST_ASSERT_FALSE(isalpha(i));
		}
	}

	TEST_ASSERT_FALSE(isalpha(EOF));
}


TEST(ctype, isascii)
{
	for (int i = MIN_VALUE; i <= MAX_VALUE; i++) {
		if (i >= 0 && i <= 127) {
			TEST_ASSERT_TRUE(isascii(i));
		}
		else {
			TEST_ASSERT_FALSE(isascii(i));
		}
	}

	TEST_ASSERT_FALSE(isascii(EOF));
}


TEST(ctype, isblank)
{
	for (int i = MIN_VALUE; i <= MAX_VALUE; i++) {
		if (i == ' ' || i == '\t') {
			TEST_ASSERT_TRUE(isblank(i));
		}
		else {
			TEST_ASSERT_FALSE(isblank(i));
		}
	}

	TEST_ASSERT_FALSE(isblank(EOF));
}


TEST(ctype, iscntrl)
{
	for (int i = MIN_VALUE; i <= MAX_VALUE; i++) {
		if ((i >= 0 && i <= 31) || i == 127) {
			TEST_ASSERT_TRUE(iscntrl(i));
		}
		else {
			TEST_ASSERT_FALSE(iscntrl(i));
		}
	}

	TEST_ASSERT_FALSE(iscntrl(EOF));
}


TEST(ctype, isdigit)
{
	for (int i = MIN_VALUE; i <= MAX_VALUE; i++) {
		if (i >= '0' && i <= '9') {
			TEST_ASSERT_TRUE(isdigit(i));
		}
		else {
			TEST_ASSERT_FALSE(isdigit(i));
		}
	}

	TEST_ASSERT_FALSE(isdigit(EOF));
}


TEST(ctype, isgraph)
{
	for (int i = MIN_VALUE; i <= MAX_VALUE; i++) {
		if (i >= '!' && i <= '~') {
			TEST_ASSERT_TRUE(isgraph(i));
		}
		else {
			TEST_ASSERT_FALSE(isgraph(i));
		}
	}

	TEST_ASSERT_FALSE(isgraph(EOF));
}


TEST(ctype, islower)
{
	for (int i = MIN_VALUE; i <= MAX_VALUE; i++) {
		if (i >= 'a' && i <= 'z') {
			TEST_ASSERT_TRUE(islower(i));
		}
		else {
			TEST_ASSERT_FALSE(islower(i));
		}
	}

	TEST_ASSERT_FALSE(islower(EOF));
}


TEST(ctype, isprint)
{
	for (int i = MIN_VALUE; i <= MAX_VALUE; i++) {
		if (i >= ' ' && i <= '~') {
			TEST_ASSERT_TRUE(isprint(i));
		}
		else {
			TEST_ASSERT_FALSE(isprint(i));
		}
	}

	TEST_ASSERT_FALSE(isprint(EOF));
}


TEST(ctype, ispunct)
{
	for (int i = MIN_VALUE; i <= MAX_VALUE; i++) {
		if ((i >= '!' && i <= '/') || (i >= ':' && i <= '@') ||
			(i >= '[' && i <= '`') || (i >= '{' && i <= '~')) {
			TEST_ASSERT_TRUE(ispunct(i));
		}
		else {
			TEST_ASSERT_FALSE(ispunct(i));
		}
	}

	TEST_ASSERT_FALSE(ispunct(EOF));
}


TEST(ctype, isspace)
{
	for (int i = MIN_VALUE; i <= MAX_VALUE; i++) {
		if ((i >= 9 && i <= 13) || i == 32) {
			TEST_ASSERT_TRUE(isspace(i));
		}
		else {
			TEST_ASSERT_FALSE(isspace(i));
		}
	}

	TEST_ASSERT_FALSE(isspace(EOF));
}


TEST(ctype, isupper)
{
	for (int i = MIN_VALUE; i <= MAX_VALUE; i++) {
		if (i >= 'A' && i <= 'Z') {
			TEST_ASSERT_TRUE(isupper(i));
		}
		else {
			TEST_ASSERT_FALSE(isupper(i));
		}
	}

	TEST_ASSERT_FALSE(isupper(EOF));
}


TEST(ctype, isxdigit)
{
	for (int i = MIN_VALUE; i <= MAX_VALUE; i++) {
		if ((i >= '0' && i <= '9') || (i >= 'A' && i <= 'F') || (i >= 'a' && i <= 'f')) {
			TEST_ASSERT_TRUE(isxdigit(i));
		}
		else {
			TEST_ASSERT_FALSE(isxdigit(i));
		}
	}

	TEST_ASSERT_FALSE(isxdigit(EOF));
}


TEST(ctype, toascii)
{
/* toascii() not yet implemented in Phoenix-RTOS */
#ifdef __phoenix__
	TEST_IGNORE();
#else
	for (int i = MIN_VALUE; i <= MAX_VALUE; i++) {
		TEST_ASSERT_EQUAL_INT(i & 0x7f, toascii(i));
	}

	TEST_ASSERT_EQUAL_INT(EOF, toupper(EOF));
#endif
}


TEST(ctype, tolower)
{
	for (int i = MIN_VALUE; i <= MAX_VALUE; i++) {
		if (i >= 'A' && i <= 'Z') {
			TEST_ASSERT_EQUAL_INT(i + 32, tolower(i));
		}
		else {
			TEST_ASSERT_EQUAL_INT(i, tolower(i));
		}
	}

	TEST_ASSERT_EQUAL_INT(EOF, tolower(EOF));
}


TEST(ctype, toupper)
{
	for (int i = MIN_VALUE; i <= MAX_VALUE; i++) {
		if (i >= 'a' && i <= 'z') {
			TEST_ASSERT_EQUAL_INT(i - 32, toupper(i));
		}
		else {
			TEST_ASSERT_EQUAL_INT(i, toupper(i));
		}
	}

	TEST_ASSERT_EQUAL_INT(EOF, toupper(EOF));
}


TEST(ctype, _tolower)
{
/* _tolower() not yet implemented in Phoenix-RTOS */
#ifdef __phoenix__
	TEST_IGNORE();
#else
	for (int i = MIN_VALUE; i <= MAX_VALUE; i++) {
		if (i >= 'A' && i <= 'Z') {
			TEST_ASSERT_EQUAL_INT(i + 32, _tolower(i));
		}
		else {
			TEST_ASSERT_EQUAL_INT(i, _tolower(i));
		}
	}

	TEST_ASSERT_EQUAL_INT(EOF, _tolower(EOF));
#endif
}


TEST(ctype, _toupper)
{
/* _toupper() not yet implemented in Phoenix-RTOS */
#ifdef __phoenix__
	TEST_IGNORE();
#else
	for (int i = MIN_VALUE; i <= MAX_VALUE; i++) {
		if (i >= 'a' && i <= 'z') {
			TEST_ASSERT_EQUAL_INT(i - 32, _toupper(i));
		}
		else {
			TEST_ASSERT_EQUAL_INT(i, _toupper(i));
		}
	}

	TEST_ASSERT_EQUAL_INT(EOF, _toupper(EOF));
#endif
}


TEST_GROUP_RUNNER(ctype)
{
	RUN_TEST_CASE(ctype, isalnum);
	RUN_TEST_CASE(ctype, isalpha);
	RUN_TEST_CASE(ctype, isascii);
	RUN_TEST_CASE(ctype, isblank);
	RUN_TEST_CASE(ctype, iscntrl);
	RUN_TEST_CASE(ctype, isdigit);
	RUN_TEST_CASE(ctype, isgraph);
	RUN_TEST_CASE(ctype, islower);
	RUN_TEST_CASE(ctype, isprint);
	RUN_TEST_CASE(ctype, ispunct);
	RUN_TEST_CASE(ctype, isspace);
	RUN_TEST_CASE(ctype, isupper);
	RUN_TEST_CASE(ctype, isxdigit);
	RUN_TEST_CASE(ctype, toascii);
	RUN_TEST_CASE(ctype, tolower);
	RUN_TEST_CASE(ctype, toupper);
	RUN_TEST_CASE(ctype, _tolower);
	RUN_TEST_CASE(ctype, _toupper);
}
