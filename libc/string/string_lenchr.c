/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - string.h
 * TESTED:
 *    - strlen()
 *    - strnlen()
 * 	- strchr()
 * 	- strrchr()
 *    - strchr()
 *    - strrchr()
 *    - memchr()
 *
 * Copyright 2023 Phoenix Systems
 * Author: Damian Modzelewski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <unity_fixture.h>

#include "testdata.h"

#define BUFF_SIZE 128

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
			   whites[] = " \v\t\r\n",
			   *asciiSet;
	int sz;

	TEST_ASSERT_EQUAL_INT(0, strlen(""));
	asciiSet = testdata_createCharStr(BUFF_SIZE + 1);

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


	sz = BUFF_SIZE;
	TEST_ASSERT_EQUAL_INT(sz, strlen(asciiSet));
	TEST_ASSERT_EQUAL_INT(sz - 1, strnlen(asciiSet, sz - 1));
	TEST_ASSERT_EQUAL_INT(sz, strnlen(asciiSet, sz));
	TEST_ASSERT_EQUAL_INT(sz, strnlen(asciiSet, sz + 1));

	free((void *)asciiSet);
}


TEST(string_len, not_ascii)
{
	unsigned char charSet[BUFF_SIZE] = { 0 };
	int sz, i;

	/* Checking  out of ASCII bytes */
	for (i = sizeof(charSet); i < sizeof(charSet) * 2 - 1; i++) {
		charSet[i - sizeof(charSet)] = i;
	}

	sz = sizeof(charSet) - 1;
	TEST_ASSERT_EQUAL_INT(sz, strlen((const char *)charSet));
	TEST_ASSERT_EQUAL_INT(sz - 1, strnlen((const char *)charSet, sz - 1));
	TEST_ASSERT_EQUAL_INT(sz, strnlen((const char *)charSet, sz));
	TEST_ASSERT_EQUAL_INT(sz, strnlen((const char *)charSet, sz + 1));
}


TEST(string_len, big)
{
	char bigStr[PATH_MAX] = { 0 };
	int sz;

	/* The length of the string is not restricted, so we test one of bigger value, which may be used  */
	memset(bigStr, 1, sizeof(bigStr) - 1);

	sz = sizeof(bigStr) - 1;
	TEST_ASSERT_EQUAL_INT(sz, strlen(bigStr));
	TEST_ASSERT_EQUAL_INT(sz - 1, strnlen(bigStr, sz - 1));
	TEST_ASSERT_EQUAL_INT(sz, strnlen(bigStr, sz));
	TEST_ASSERT_EQUAL_INT(sz, strnlen(bigStr, sz + 1));
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
	TEST_ASSERT_EQUAL_PTR(lorem, strchr(lorem, 'L'));
	TEST_ASSERT_EQUAL_PTR(lorem, strrchr(lorem, 'L'));
	TEST_ASSERT_EQUAL_PTR(lorem, memchr(lorem, 'L', sz));

	TEST_ASSERT_EQUAL_PTR(&lorem[14], strchr(lorem, 'l'));
	TEST_ASSERT_EQUAL_PTR(&lorem[52], strrchr(lorem, 'l'));
	TEST_ASSERT_EQUAL_PTR(&lorem[14], memchr(lorem, 'l', sz));

	TEST_ASSERT_EQUAL_PTR(NULL, strchr(lorem, 'x'));
	TEST_ASSERT_EQUAL_PTR(NULL, strrchr(lorem, 'x'));
	TEST_ASSERT_EQUAL_PTR(NULL, memchr(lorem, 'x', sz));

	TEST_ASSERT_EQUAL_PTR(&lorem[sz], strchr(lorem, lorem[sz]));
	TEST_ASSERT_EQUAL_PTR(&lorem[sz], strrchr(lorem, lorem[sz]));
	TEST_ASSERT_EQUAL_PTR(&lorem[sz], memchr(lorem, lorem[sz], sz + 1));
}


TEST(string_chr, big)
{
	char str[PATH_MAX] = { 0 };

	/* Long string case */
	memset(str, 'a', sizeof(str) - 1);
	str[PATH_MAX - 5] = 'b';

	TEST_ASSERT_EQUAL_PTR(&str[PATH_MAX - 5], strchr(str, 'b'));
	TEST_ASSERT_EQUAL_PTR(&str[PATH_MAX - 5], strrchr(str, 'b'));
	TEST_ASSERT_EQUAL_PTR(&str[PATH_MAX - 5], memchr(str, 'b', sizeof(str)));

	str[PATH_MAX - 5] = 'a';
	str[5] = 'b';

	TEST_ASSERT_EQUAL_PTR(&str[5], strchr(str, 'b'));
	TEST_ASSERT_EQUAL_PTR(&str[5], strrchr(str, 'b'));
	TEST_ASSERT_EQUAL_PTR(&str[5], memchr(str, 'b', sizeof(str)));

	TEST_ASSERT_EQUAL_PTR(str, strchr(str, 'a'));
	/*
	 * In this scenario we must point to 2 places before the end of
	 * the size of 'str' because the first of them is a NULL terminator
	 * and the second place is where intentionally strrchr will point
	 * because it returns the last byte before NULL
	 */
	TEST_ASSERT_EQUAL_PTR(&str[sizeof(str) - 2], strrchr(str, 'a'));
	TEST_ASSERT_EQUAL_PTR(str, memchr(str, 'a', sizeof(str)));

	TEST_ASSERT_EQUAL_PTR(NULL, strchr(str, 'x'));
	TEST_ASSERT_EQUAL_PTR(NULL, strrchr(str, 'x'));
	TEST_ASSERT_EQUAL_PTR(NULL, memchr(str, 'x', sizeof(str)));
}


TEST(string_chr, special)
{
	const char specials[] = "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";
	int sz;

	/*
	 * Testing did strchr don't stop if found special signs
	 * Getting the first element from the string contains only special characters
	 */

	sz = strlen(specials);

	TEST_ASSERT_EQUAL_PTR(specials, strchr(specials, specials[0]));
	TEST_ASSERT_EQUAL_PTR(specials, strrchr(specials, specials[0]));
	TEST_ASSERT_EQUAL_PTR(specials, memchr(specials, specials[0], strlen(specials)));

	/* Getting last element from string contains only special characters */
	TEST_ASSERT_EQUAL_PTR(&specials[sz], strchr(specials, specials[sz]));
	TEST_ASSERT_EQUAL_PTR(&specials[sz], strrchr(specials, specials[sz]));
	TEST_ASSERT_EQUAL_PTR(&specials[sz], memchr(specials, specials[sz], sz + 1));

	/* Getting middle element from string contains only special characters */
	TEST_ASSERT_EQUAL_PTR(&specials[sz / 2], strchr(specials, specials[sz / 2]));
	TEST_ASSERT_EQUAL_PTR(&specials[sz / 2], strrchr(specials, specials[sz / 2]));
	TEST_ASSERT_EQUAL_PTR(&specials[sz / 2], memchr(specials, specials[sz / 2], sz + 1));

	/* Getting element which is not into string */
	TEST_ASSERT_EQUAL_PTR(NULL, strchr(specials, 'I'));
	TEST_ASSERT_EQUAL_PTR(NULL, strrchr(specials, 'I'));
	TEST_ASSERT_EQUAL_PTR(NULL, memchr(specials, 'I', sz));
}


TEST(string_chr, ascii)
{
	int sz;
	char *asciiStr;

	asciiStr = testdata_createCharStr(INT8_MAX + 1);
	sz = INT8_MAX;


	TEST_ASSERT_EQUAL_PTR(asciiStr, strchr(asciiStr, asciiStr[0]));
	/* In this scenario we need to use second place of array because testdata create string with 0 and replace it with 1 */
	TEST_ASSERT_EQUAL_PTR(&asciiStr[1], strrchr(asciiStr, asciiStr[0]));
	TEST_ASSERT_EQUAL_PTR(asciiStr, memchr(asciiStr, asciiStr[0], sz + 1));

	TEST_ASSERT_EQUAL_PTR(&asciiStr[sz / 2], strchr(asciiStr, asciiStr[sz / 2]));
	TEST_ASSERT_EQUAL_PTR(&asciiStr[sz / 2], strrchr(asciiStr, asciiStr[sz / 2]));
	TEST_ASSERT_EQUAL_PTR(&asciiStr[sz / 2], memchr(asciiStr, asciiStr[sz / 2], sz + 1));

	TEST_ASSERT_EQUAL_PTR(&asciiStr[sz], strchr(asciiStr, asciiStr[sz]));
	TEST_ASSERT_EQUAL_PTR(&asciiStr[sz], strrchr(asciiStr, asciiStr[sz]));
	TEST_ASSERT_EQUAL_PTR(&asciiStr[sz], memchr(asciiStr, asciiStr[sz], sz + 1));

	free((void *)asciiStr);
}


TEST(string_chr, not_ascii)
{
	char charSet[BUFF_SIZE] = { 0 };
	int sz, i;

	/* Checking  out of ASCII bytes */
	for (i = sizeof(charSet); i < sizeof(charSet) * 2 - 1; i++) {
		charSet[i - sizeof(charSet)] = i;
	}

	/* Testing capability of functions to hold and read not ascii set */
	sz = sizeof(charSet);
	TEST_ASSERT_EQUAL_PTR(charSet, strchr(charSet, charSet[0]));
	TEST_ASSERT_EQUAL_PTR(charSet, strrchr(charSet, charSet[0]));
	TEST_ASSERT_EQUAL_PTR(charSet, memchr(charSet, charSet[0], sz));

	TEST_ASSERT_EQUAL_PTR(&charSet[64], strchr(charSet, charSet[64]));
	TEST_ASSERT_EQUAL_PTR(&charSet[64], strrchr(charSet, charSet[64]));
	TEST_ASSERT_EQUAL_PTR(&charSet[64], memchr(charSet, charSet[64], sz));

	TEST_ASSERT_EQUAL_PTR(&charSet[sz - 1], strchr(charSet, charSet[sz - 1]));
	TEST_ASSERT_EQUAL_PTR(&charSet[sz - 1], strrchr(charSet, charSet[sz - 1]));
	TEST_ASSERT_EQUAL_PTR(&charSet[sz - 1], memchr(charSet, charSet[sz - 1], sz));
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

	TEST_ASSERT_EQUAL_PTR(NULL, strchr(torn, 'b'));
	TEST_ASSERT_EQUAL_PTR(NULL, strrchr(torn, 'b'));
	TEST_ASSERT_EQUAL_PTR(NULL, memchr(torn, 'b', sz));
}


TEST(string_chr, whitespaces)
{
	const char *exp = "Ipsum",
			   *strWhites = "Lorem \n\t\e\r\b\v\f\\ Ipsum",
			   whites[] = " \n\t\e\r\b\v\f\\";
	int sz, i;

	/* Checking if white signs don't make any interference on functions output */
	sz = sizeof(whites) - 1;

	for (i = 0; i < sz; i++) {
		TEST_ASSERT_EQUAL_PTR(&whites[i], strchr(whites, whites[i]));
		TEST_ASSERT_EQUAL_PTR(&whites[i], strrchr(whites, whites[i]));
		TEST_ASSERT_EQUAL_PTR(&whites[i], memchr(whites, whites[i], i + 1));
	}

	TEST_ASSERT_EQUAL_PTR(NULL, strchr(whites, 'x'));
	TEST_ASSERT_EQUAL_PTR(NULL, strrchr(whites, 'x'));
	TEST_ASSERT_EQUAL_PTR(NULL, memchr(whites, 'x', sz));

	sz = strlen(strWhites);
	/* Testing if whitespaces interference output of functions */
	TEST_ASSERT_EQUAL_STRING(exp, strchr(strWhites, 'I'));
	TEST_ASSERT_EQUAL_STRING(exp, strrchr(strWhites, 'I'));
	TEST_ASSERT_EQUAL_STRING(exp, memchr(strWhites, 'I', sz));
}


TEST(string_chr, empty)
{
	/* Checking if we can get an empty string */
	TEST_ASSERT_EQUAL_STRING("", strchr("", '\0'));
	TEST_ASSERT_EQUAL_STRING("", strrchr("", '\0'));
	TEST_ASSERT_EQUAL_STRING("", memchr("", '\0', 1));
}


TEST(string_chr, memchr_size)
{
	char charSet[BUFF_SIZE] = { 0 };
	int i;

	for (i = 0; i < sizeof(charSet); i++) {

		charSet[i] = i;

		/*
		 * In this case, we search the ASCII set where the stop point
		 * is always found and we check if memchr always returns the
		 * correct address
		 */
		TEST_ASSERT_EQUAL_PTR(&charSet[i], memchr(charSet, charSet[i], i + 1));
		/*
		 * In this case, the size of the search is the same as the place
		 * where is search char placed and memchr never meets
		 * the criteria to find it
		 */
		TEST_ASSERT_EQUAL_PTR(NULL, memchr(charSet, charSet[i], i));
	}
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
