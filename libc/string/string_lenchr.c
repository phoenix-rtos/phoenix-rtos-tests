/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 * 	- string.h
 * TESTED:
 * 	- strlen()
 * 	- strnlen()
 * 	- strchr()
 * 	- strrchr()
 * 	- memchr()
 *
 * Copyright 2023 Phoenix Systems
 * Author: Damian Modzelewski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <unity_fixture.h>


TEST_GROUP(string_len);
TEST_GROUP(string_chr);


TEST_SETUP(string_len)
{
}


TEST_TEAR_DOWN(string_len)
{
}


TEST(string_len, ascii)
{
	const char empty[] = "",
			   pangram[] = "The quick brown fox jumps over the lazy dog",
			   torn[] = "foo\0bar",
			   doubleNul[] = "\0\0abc",
			   specials[] = "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~",
			   whites[] = " \v\t\r\n";
	char asciiSet[128] = { 0 };
	int sz, i;

	TEST_ASSERT_EQUAL_INT(0, strlen(""));

	/* Pangram with a whole alphabet set */
	sz = sizeof(pangram) - 1;
	TEST_ASSERT_EQUAL_INT(sz, strlen(pangram));
	TEST_ASSERT_EQUAL_INT(sz - 1, strnlen(pangram, sz - 1));
	TEST_ASSERT_EQUAL_INT(sz, strnlen(pangram, sz));
	TEST_ASSERT_EQUAL_INT(sz, strnlen(pangram, sz + 1));

	/* Text with null character in the middle */
	sz = sizeof(torn) / 2 - 1;
	TEST_ASSERT_EQUAL_INT(sz, strlen(torn));
	TEST_ASSERT_EQUAL_INT(sz - 1, strnlen(torn, 3 - 1));
	TEST_ASSERT_EQUAL_INT(sz, strnlen(torn, 3));
	TEST_ASSERT_EQUAL_INT(sz, strnlen(torn, 3 + 1));

	/* End of string */
	sz = 0;
	TEST_ASSERT_EQUAL_INT(sz, strlen(doubleNul));
	TEST_ASSERT_EQUAL_INT(sz, strnlen(doubleNul, 0));
	TEST_ASSERT_EQUAL_INT(sz, strnlen(doubleNul, 1));
	TEST_ASSERT_EQUAL_INT(sz, strlen(doubleNul));
	TEST_ASSERT_EQUAL_INT(sz, strnlen(empty, 0));
	TEST_ASSERT_EQUAL_INT(sz, strnlen(empty, 1));

	/* Special characters */
	sz = sizeof(specials) - 1;
	TEST_ASSERT_EQUAL_INT(sz, strlen(specials));
	TEST_ASSERT_EQUAL_INT(sz - 1, strnlen(specials, sz - 1));
	TEST_ASSERT_EQUAL_INT(sz, strnlen(specials, sz));
	TEST_ASSERT_EQUAL_INT(sz, strnlen(specials, sz + 1));

	/* White spaces */
	sz = sizeof(whites) - 1;
	TEST_ASSERT_EQUAL_INT(sz, strlen(whites));
	TEST_ASSERT_EQUAL_INT(sz - 1, strnlen(whites, sz - 1));
	TEST_ASSERT_EQUAL_INT(sz, strnlen(whites, sz));
	TEST_ASSERT_EQUAL_INT(sz, strnlen(whites, sz + 1));

	/* Checking ascii charset */
	for (i = 1; i < sizeof(asciiSet); i++) {
		asciiSet[i - 1] = i;
	}

	sz = sizeof(asciiSet) - 1;
	TEST_ASSERT_EQUAL_INT(sz, strlen(asciiSet));
	TEST_ASSERT_EQUAL_INT(sz - 1, strnlen(asciiSet, sz - 1));
	TEST_ASSERT_EQUAL_INT(sz, strnlen(asciiSet, sz));
	TEST_ASSERT_EQUAL_INT(sz, strnlen(asciiSet, sz + 1));
}


TEST(string_len, not_ascii)
{
	const char notAsciiString[] = "♦♥♣♠◊⊗こんにちは❉❉⌨⌨⌨⌨⌨⌨⌨⌨❉❉";
	unsigned char notAsciiSet[129];
	int sz, i;

	/* Checking ability to read the chars out of ascii charset */
	sz = sizeof(notAsciiString) - 1;
	TEST_ASSERT_EQUAL_INT(sz, strlen(notAsciiString));
	TEST_ASSERT_EQUAL_INT(sz - 1, strnlen(notAsciiString, sz - 1));
	TEST_ASSERT_EQUAL_INT(sz, strnlen(notAsciiString, sz));
	TEST_ASSERT_EQUAL_INT(sz, strnlen(notAsciiString, sz + 1));

	/* Checking  out of ascii bytes */
	for (i = 128; i <= 255; i++) {
		notAsciiSet[i - 128] = i;
	}
	notAsciiSet[128] = 0;

	sz = sizeof(notAsciiSet) - 1;
	TEST_ASSERT_EQUAL_INT(sz, strlen((const char *)notAsciiSet));
	TEST_ASSERT_EQUAL_INT(sz - 1, strnlen((const char *)notAsciiSet, sz - 1));
	TEST_ASSERT_EQUAL_INT(sz, strnlen((const char *)notAsciiSet, sz));
	TEST_ASSERT_EQUAL_INT(sz, strnlen((const char *)notAsciiSet, sz + 1));
}


TEST(string_len, big)
{
	char bigstr[PATH_MAX] = { 0 };
	int sz, i;

	/* The length of string is not clearly restricted, so we test one of bigger value, which may be used  */
	for (i = 0; i < PATH_MAX - 1; i++) {
		bigstr[i] = 'A';
	}
	bigstr[i] = '\0';

	sz = sizeof(bigstr) - 1;
	TEST_ASSERT_EQUAL_INT(sz, strlen(bigstr));
	TEST_ASSERT_EQUAL_INT(sz - 1, strnlen(bigstr, sz - 1));
	TEST_ASSERT_EQUAL_INT(sz, strnlen(bigstr, sz));
	TEST_ASSERT_EQUAL_INT(sz, strnlen(bigstr, sz + 1));
}


/*
////////////////////////////////////////////////////////////////////////////////////
*/


TEST_SETUP(string_chr)
{
}


TEST_TEAR_DOWN(string_chr)
{
}


TEST(string_chr, basic)
{
	const char *lorem = "Lorem ipsum dolor sit amet, consectetur adipiscing elit y";
	int sz;

	sz = strlen(lorem);

	/* Check of proper working base */
	TEST_ASSERT_EQUAL_STRING(lorem, strchr(lorem, 'L'));
	TEST_ASSERT_EQUAL_STRING(lorem, strrchr(lorem, 'L'));
	TEST_ASSERT_EQUAL_STRING(lorem, memchr(lorem, 'L', sz));

	TEST_ASSERT_EQUAL_STRING("lor sit amet, consectetur adipiscing elit y", strchr(lorem, 'l'));
	TEST_ASSERT_EQUAL_STRING("lit y", strrchr(lorem, 'l'));
	TEST_ASSERT_EQUAL_STRING("lor sit amet, consectetur adipiscing elit y", memchr(lorem, 'l', sz));

	TEST_ASSERT_EQUAL_STRING(NULL, strchr(lorem, 'x'));
	TEST_ASSERT_EQUAL_STRING(NULL, strrchr(lorem, 'x'));
	TEST_ASSERT_EQUAL_STRING(NULL, memchr(lorem, 'x', sz));

	TEST_ASSERT_EQUAL_STRING(&lorem[sz], strchr(lorem, lorem[sz]));
	TEST_ASSERT_EQUAL_STRING(&lorem[sz], strrchr(lorem, lorem[sz]));
	TEST_ASSERT_EQUAL_STRING(&lorem[sz], memchr(lorem, lorem[sz], sz + 1));
}


TEST(string_chr, big)
{
	char str[PATH_MAX] = { 0 };

	/* Long string case */
	memset(str, 'a', PATH_MAX - 1);
	str[PATH_MAX - 5] = 'b';

	TEST_ASSERT_EQUAL_STRING("baaa", strchr(str, 'b'));
	TEST_ASSERT_EQUAL_STRING("baaa", strrchr(str, 'b'));
	TEST_ASSERT_EQUAL_STRING("baaa", memchr(str, 'b', sizeof(str)));

	str[PATH_MAX - 5] = 'a';
	str[5] = 'b';

	TEST_ASSERT_EQUAL_STRING(&str[5], strchr(str, 'b'));
	TEST_ASSERT_EQUAL_STRING(&str[5], strrchr(str, 'b'));
	TEST_ASSERT_EQUAL_STRING(&str[5], memchr(str, 'b', sizeof(str)));

	TEST_ASSERT_EQUAL_STRING(str, strchr(str, 'a'));
	TEST_ASSERT_EQUAL_STRING("a", strrchr(str, 'a'));
	TEST_ASSERT_EQUAL_STRING(str, memchr(str, 'a', sizeof(str)));

	TEST_ASSERT_EQUAL_STRING(NULL, strchr(str, 'x'));
	TEST_ASSERT_EQUAL_STRING(NULL, strrchr(str, 'x'));
	TEST_ASSERT_EQUAL_STRING(NULL, memchr(str, 'x', sizeof(str)));
}


TEST(string_chr, special)
{
	const char specials[] = "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";
	int sz;

	/*
	 * Testing did strchr don't stop if found special signs
	 * Getting first element from string contains only special characters
	 */

	sz = strlen(specials);

	TEST_ASSERT_EQUAL_STRING(specials, strchr(specials, specials[0]));
	TEST_ASSERT_EQUAL_STRING(specials, strrchr(specials, specials[0]));
	TEST_ASSERT_EQUAL_STRING(specials, memchr(specials, specials[0], strlen(specials)));

	/* Getting last element from string contains only special characters */
	TEST_ASSERT_EQUAL_STRING(&specials[sz], strchr(specials, specials[sz]));
	TEST_ASSERT_EQUAL_STRING(&specials[sz], strrchr(specials, specials[sz]));
	TEST_ASSERT_EQUAL_STRING(&specials[sz], memchr(specials, specials[sz], sz + 1));

	/* Getting middle element from string contains only special characters */
	TEST_ASSERT_EQUAL_STRING(&specials[sz / 2], strchr(specials, specials[sz / 2]));
	TEST_ASSERT_EQUAL_STRING(&specials[sz / 2], strrchr(specials, specials[sz / 2]));
	TEST_ASSERT_EQUAL_STRING(&specials[sz / 2], memchr(specials, specials[sz / 2], sz + 1));

	/* Getting element which is not into string */
	TEST_ASSERT_EQUAL_STRING(NULL, strchr(specials, 'I'));
	TEST_ASSERT_EQUAL_STRING(NULL, strrchr(specials, 'I'));
	TEST_ASSERT_EQUAL_STRING(NULL, memchr(specials, 'I', sz));
}

TEST(string_chr, ascii)
{
	int sz, i;
	char str[INT8_MAX] = { 0 };

	for (i = 1; i < sizeof(str); i++) {
		str[i - 1] = i;
	}

	sz = sizeof(str) - 1;

	TEST_ASSERT_EQUAL_STRING(str, strchr(str, str[0]));
	TEST_ASSERT_EQUAL_STRING(str, strrchr(str, str[0]));
	TEST_ASSERT_EQUAL_STRING(str, memchr(str, str[0], sz + 1));

	TEST_ASSERT_EQUAL_STRING(&str[sz / 2], strchr(str, str[sz / 2]));
	TEST_ASSERT_EQUAL_STRING(&str[sz / 2], strrchr(str, str[sz / 2]));
	TEST_ASSERT_EQUAL_STRING(&str[sz / 2], memchr(str, str[sz / 2], sz + 1));

	TEST_ASSERT_EQUAL_STRING(&str[sz], strchr(str, str[sz]));
	TEST_ASSERT_EQUAL_STRING(&str[sz], strrchr(str, str[sz]));
	TEST_ASSERT_EQUAL_STRING(&str[sz], memchr(str, str[sz], sz + 1));
}


TEST(string_chr, not_ascii)
{
	const char notAsciiString[] = "♥♣♠◊⊗こんにちは❉❉⌨⌨⌨⌨⌨⌨⌨⌨❉❉♦x";
	char notAsciiSet[129];
	int sz, i;

	/* Checking ability to read the chars out of ascii charset */
	sz = sizeof(notAsciiString) - 1;
	TEST_ASSERT_EQUAL_STRING("x", strchr(notAsciiString, 'x'));
	TEST_ASSERT_EQUAL_STRING("x", strrchr(notAsciiString, 'x'));
	TEST_ASSERT_EQUAL_STRING("x", memchr(notAsciiString, 'x', sz));

	/* Checking  out of ascii bytes */
	for (i = 128; i <= 255; i++) {
		notAsciiSet[i - 128] = i;
	}
	notAsciiSet[128] = 0;

	/* Testing capability of functions to hold and read not ascii set */
	sz = sizeof(notAsciiSet);
	TEST_ASSERT_EQUAL_STRING(notAsciiSet, strchr(notAsciiSet, notAsciiSet[0]));
	TEST_ASSERT_EQUAL_STRING(notAsciiSet, strrchr(notAsciiSet, notAsciiSet[0]));
	TEST_ASSERT_EQUAL_STRING(notAsciiSet, memchr(notAsciiSet, notAsciiSet[0], sz));

	TEST_ASSERT_EQUAL_STRING(&notAsciiSet[64], strchr(notAsciiSet, notAsciiSet[64]));
	TEST_ASSERT_EQUAL_STRING(&notAsciiSet[64], strrchr(notAsciiSet, notAsciiSet[64]));
	TEST_ASSERT_EQUAL_STRING(&notAsciiSet[64], memchr(notAsciiSet, notAsciiSet[64], sz));

	TEST_ASSERT_EQUAL_STRING(&notAsciiSet[sz - 1], strchr(notAsciiSet, notAsciiSet[sz - 1]));
	TEST_ASSERT_EQUAL_STRING(&notAsciiSet[sz - 1], strrchr(notAsciiSet, notAsciiSet[sz - 1]));
	TEST_ASSERT_EQUAL_STRING(&notAsciiSet[sz - 1], memchr(notAsciiSet, notAsciiSet[sz - 1], sz));
}


TEST(string_chr, torn)
{
	char *torn = "foo\0bar";
	int sz;

	/* Checking correct working of null terminating point */
	sz = strlen(torn) + 1;
	TEST_ASSERT_EQUAL_STRING("", strchr(torn, '\0'));
	TEST_ASSERT_EQUAL_STRING("", strrchr(torn, '\0'));
	TEST_ASSERT_EQUAL_STRING("", memchr(torn, '\0', sz));

	TEST_ASSERT_EQUAL_STRING("foo", strchr(torn, 'f'));
	TEST_ASSERT_EQUAL_STRING("foo", strrchr(torn, 'f'));
	TEST_ASSERT_EQUAL_STRING("foo", memchr(torn, 'f', sz));

	TEST_ASSERT_EQUAL_STRING(NULL, strchr(torn, 'b'));
	TEST_ASSERT_EQUAL_STRING(NULL, strrchr(torn, 'b'));
	TEST_ASSERT_EQUAL_STRING(NULL, memchr(torn, 'b', sz));
}


TEST(string_chr, whitespaces)
{
	const char *exp = "Ipsum",
			   *strWhites = "Lorem \n\t\e\r\b\v\f\\ Ipsum",
			   whites[] = " \n\t\e\r\b\v\f\\";
	int sz;

	/* Checking if white signs doesn't make any interference on functions output */
	sz = sizeof(whites) - 1;
	TEST_ASSERT_EQUAL_STRING(&whites[sz], strchr(whites, whites[sz]));
	TEST_ASSERT_EQUAL_STRING(&whites[sz], strrchr(whites, whites[sz]));
	TEST_ASSERT_EQUAL_STRING(&whites[sz], memchr(whites, whites[sz], sz + 1));

	TEST_ASSERT_EQUAL_STRING(&whites[sz / 2], strchr(whites, whites[sz / 2]));
	TEST_ASSERT_EQUAL_STRING(&whites[sz / 2], strrchr(whites, whites[sz / 2]));
	TEST_ASSERT_EQUAL_STRING(&whites[sz / 2], memchr(whites, whites[sz / 2], sz + 1));

	TEST_ASSERT_EQUAL_STRING(&whites[0], strchr(whites, whites[0]));
	TEST_ASSERT_EQUAL_STRING(&whites[0], strrchr(whites, whites[0]));
	TEST_ASSERT_EQUAL_STRING(&whites[0], memchr(whites, whites[0], sz + 1));

	TEST_ASSERT_EQUAL_STRING(NULL, strchr(whites, 'x'));
	TEST_ASSERT_EQUAL_STRING(NULL, strrchr(whites, 'x'));
	TEST_ASSERT_EQUAL_STRING(NULL, memchr(whites, 'x', sz));

	sz = strlen(strWhites);
	/* Testing if whitespaces interference output of functions */
	TEST_ASSERT_EQUAL_STRING(exp, strchr(strWhites, 'I'));
	TEST_ASSERT_EQUAL_STRING(exp, strrchr(strWhites, 'I'));
	TEST_ASSERT_EQUAL_STRING(exp, memchr(strWhites, 'I', sz));
}


TEST(string_chr, empty)
{
	int sz;

	/* Checking if we can get an empty string */
	sz = 1;
	TEST_ASSERT_EQUAL_STRING("", strchr("", '\0'));
	TEST_ASSERT_EQUAL_STRING("", strrchr("", '\0'));
	TEST_ASSERT_EQUAL_STRING("", memchr("", '\0', sz));
}


TEST(string_chr, memchr_size)
{
	char lorem[] = "Lorem";

	/* Testing capability of setting size in memchr */
	TEST_ASSERT_EQUAL_STRING("rem", memchr(lorem, 'r', 6));
	TEST_ASSERT_EQUAL_STRING("rem", memchr(lorem, 'r', 5));
	TEST_ASSERT_EQUAL_STRING("rem", memchr(lorem, 'r', 4));
	TEST_ASSERT_EQUAL_STRING("rem", memchr(lorem, 'r', 3));
	TEST_ASSERT_EQUAL_STRING(NULL, memchr(lorem, 'r', 2));
	TEST_ASSERT_EQUAL_STRING(NULL, memchr(lorem, 'r', 1));
	TEST_ASSERT_EQUAL_STRING(NULL, memchr(lorem, 'r', 0));
}


/*
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/


TEST_GROUP_RUNNER(string_len)
{
	RUN_TEST_CASE(string_len, ascii);
	RUN_TEST_CASE(string_len, not_ascii);
	RUN_TEST_CASE(string_len, big);
}


TEST_GROUP_RUNNER(string_chr)
{
	RUN_TEST_CASE(string_chr, basic);
	RUN_TEST_CASE(string_chr, big);
	RUN_TEST_CASE(string_chr, memchr_size);
	RUN_TEST_CASE(string_chr, special);
	RUN_TEST_CASE(string_chr, whitespaces);
	RUN_TEST_CASE(string_chr, empty);
	RUN_TEST_CASE(string_chr, ascii);
	RUN_TEST_CASE(string_chr, not_ascii);
	RUN_TEST_CASE(string_chr, torn);
}
