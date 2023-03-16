/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - string.h
 * TESTED:
 *    - strlen()
 *    - strnlen()
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


TEST_SETUP(string_len)
{
}


TEST_TEAR_DOWN(string_len)
{
}


TEST(string_len, ascii)
{
	const char empty[] = "";
	const char pangram[] = "The quick brown fox jumps over the lazy dog";
	const char torn[] = "foo\0bar";
	const char doubleNul[] = "\0\0abc";
	const char specials[] = "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";
	const char whites[] = " \v\t\r\n";
	char asciiSet[128] = { 0 };
	int sz, i;

	TEST_ASSERT_EQUAL_INT(0, strlen(""));

	/* pangram with a whole alphabet set */
	sz = sizeof(pangram) - 1;
	TEST_ASSERT_EQUAL_INT(sz, strlen(pangram));
	TEST_ASSERT_EQUAL_INT(sz - 1, strnlen(pangram, sz - 1));
	TEST_ASSERT_EQUAL_INT(sz, strnlen(pangram, sz));
	TEST_ASSERT_EQUAL_INT(sz, strnlen(pangram, sz + 1));

	/* text with null character in the middle*/
	sz = (sizeof(torn) / 2) - 1;
	TEST_ASSERT_EQUAL_INT(sz, strlen(torn));
	TEST_ASSERT_EQUAL_INT(sz - 1, strnlen(torn, 3 - 1));
	TEST_ASSERT_EQUAL_INT(sz, strnlen(torn, 3));
	TEST_ASSERT_EQUAL_INT(sz, strnlen(torn, 3 + 1));

	sz = 0;
	/* end of string */
	TEST_ASSERT_EQUAL_INT(sz, strlen(doubleNul));
	TEST_ASSERT_EQUAL_INT(sz, strnlen(doubleNul, 0));
	TEST_ASSERT_EQUAL_INT(sz, strnlen(doubleNul, 1));
	TEST_ASSERT_EQUAL_INT(sz, strlen(doubleNul));
	TEST_ASSERT_EQUAL_INT(sz, strnlen(empty, 0));
	TEST_ASSERT_EQUAL_INT(sz, strnlen(empty, 1));

	/* special characters */
	sz = sizeof(specials) - 1;
	TEST_ASSERT_EQUAL_INT(sz, strlen(specials));
	TEST_ASSERT_EQUAL_INT(sz - 1, strnlen(specials, sz - 1));
	TEST_ASSERT_EQUAL_INT(sz, strnlen(specials, sz));
	TEST_ASSERT_EQUAL_INT(sz, strnlen(specials, sz + 1));

	/* white spaces */
	sz = sizeof(whites) - 1;
	TEST_ASSERT_EQUAL_INT(sz, strlen(whites));
	TEST_ASSERT_EQUAL_INT(sz - 1, strnlen(whites, sz - 1));
	TEST_ASSERT_EQUAL_INT(sz, strnlen(whites, sz));
	TEST_ASSERT_EQUAL_INT(sz, strnlen(whites, sz + 1));

	/*Checking ascii charset*/
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

	/* Checking  out of ASCII bytes */
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


TEST_GROUP_RUNNER(string_len)
{
	RUN_TEST_CASE(string_len, ascii);
	RUN_TEST_CASE(string_len, not_ascii);
	RUN_TEST_CASE(string_len, big);
}
