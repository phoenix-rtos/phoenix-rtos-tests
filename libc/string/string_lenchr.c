/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - string.h
 * TESTED:
 *    - strlen()
 *    - strnlen()
 *    - strcspn()
 *    - strspn()
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
TEST_GROUP(string_spn);


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
			   whites[] = " \v\t\r\n\f ";
	char *asciiSet;
	int len;

	TEST_ASSERT_EQUAL_INT(0, strlen(""));
	asciiSet = testdata_createCharStr(BUFF_SIZE + 1);

	TEST_ASSERT_NOT_NULL(asciiSet);

	/* Pangram with a whole alphabet set */
	len = sizeof(pangram) - 1;
	TEST_ASSERT_EQUAL_INT(len, strlen(pangram));
	TEST_ASSERT_EQUAL_INT(len - 1, strnlen(pangram, len - 1));
	TEST_ASSERT_EQUAL_INT(len, strnlen(pangram, len));
	TEST_ASSERT_EQUAL_INT(len, strnlen(pangram, len + 1));

	/* Text with null character in the middle */
	len = sizeof(torn) / 2 - 1;
	TEST_ASSERT_EQUAL_INT(len, strlen(torn));
	TEST_ASSERT_EQUAL_INT(len - 1, strnlen(torn, 3 - 1));
	TEST_ASSERT_EQUAL_INT(len, strnlen(torn, 3));
	TEST_ASSERT_EQUAL_INT(len, strnlen(torn, 3 + 1));

	/* End of string */
	len = 0;
	TEST_ASSERT_EQUAL_INT(len, strlen(doubleNul));
	TEST_ASSERT_EQUAL_INT(len, strnlen(doubleNul, 0));
	TEST_ASSERT_EQUAL_INT(len, strnlen(doubleNul, 1));
	TEST_ASSERT_EQUAL_INT(len, strlen(doubleNul));
	TEST_ASSERT_EQUAL_INT(len, strnlen(empty, 0));
	TEST_ASSERT_EQUAL_INT(len, strnlen(empty, 1));

	/* Special characters */
	len = sizeof(specials) - 1;
	TEST_ASSERT_EQUAL_INT(len, strlen(specials));
	TEST_ASSERT_EQUAL_INT(len - 1, strnlen(specials, len - 1));
	TEST_ASSERT_EQUAL_INT(len, strnlen(specials, len));
	TEST_ASSERT_EQUAL_INT(len, strnlen(specials, len + 1));

	/* White spaces */
	len = sizeof(whites) - 1;
	TEST_ASSERT_EQUAL_INT(len, strlen(whites));
	TEST_ASSERT_EQUAL_INT(len - 1, strnlen(whites, len - 1));
	TEST_ASSERT_EQUAL_INT(len, strnlen(whites, len));
	TEST_ASSERT_EQUAL_INT(len, strnlen(whites, len + 1));

	/* Checking ascii charset */
	len = BUFF_SIZE;
	TEST_ASSERT_EQUAL_INT(len, strlen(asciiSet));
	TEST_ASSERT_EQUAL_INT(len - 1, strnlen(asciiSet, len - 1));
	TEST_ASSERT_EQUAL_INT(len, strnlen(asciiSet, len));
	TEST_ASSERT_EQUAL_INT(len, strnlen(asciiSet, len + 1));

	free(asciiSet);
}


TEST(string_len, not_ascii)
{
	unsigned char charSet[BUFF_SIZE] = { 0 };
	int len, i;

	/* Checking out of ASCII bytes */
	for (i = sizeof(charSet); i < sizeof(charSet) * 2 - 1; i++) {
		charSet[i - sizeof(charSet)] = i;
	}

	len = sizeof(charSet) - 1;
	TEST_ASSERT_EQUAL_INT(len, strlen((const char *)charSet));
	TEST_ASSERT_EQUAL_INT(len - 1, strnlen((const char *)charSet, len - 1));
	TEST_ASSERT_EQUAL_INT(len, strnlen((const char *)charSet, len));
	TEST_ASSERT_EQUAL_INT(len, strnlen((const char *)charSet, len + 1));
}


TEST(string_len, big)
{
	char bigStr[PATH_MAX] = { 0 };
	int len;

	/* The length of the string is not restricted, so we test one of bigger value, which may be used  */
	memset(bigStr, 1, sizeof(bigStr) - 1);

	len = sizeof(bigStr) - 1;
	TEST_ASSERT_EQUAL_INT(len, strlen(bigStr));
	TEST_ASSERT_EQUAL_INT(len - 1, strnlen(bigStr, len - 1));
	TEST_ASSERT_EQUAL_INT(len, strnlen(bigStr, len));
	TEST_ASSERT_EQUAL_INT(len, strnlen(bigStr, len + 1));
}

/*
////////////////////////////////////////////////////////////////////////////////////
*/


TEST_SETUP(string_spn)
{
}


TEST_TEAR_DOWN(string_spn)
{
}

TEST(string_spn, basic)
{
	const char pangram[] = " The quick brown fox jumps over the lazy dog";
	char holder[45] = { 0 };
	int len;

	/* Checking if both functions recognize the holder as a different set of elements */
	len = sizeof(pangram) - 1;

	memcpy(holder, pangram, len + 1);

	/* Checking if both functions recognize the holder as the same set of elements */
	TEST_ASSERT_EQUAL_INT(0, strcspn(pangram, holder));
	TEST_ASSERT_EQUAL_INT(len, strspn(pangram, holder));

	TEST_ASSERT_EQUAL_INT(0, strcspn(pangram, &holder[len / 2]));
	/* One because strspn found space as*/
	TEST_ASSERT_EQUAL_INT(1, strspn(pangram, &holder[len / 2]));

	TEST_ASSERT_EQUAL_INT(len, strcspn(pangram, &holder[len]));
}


TEST(string_spn, ascii)
{
	char supportCharSet[BUFF_SIZE] = { 0 },
		 reversCharSet[BUFF_SIZE] = { 0 },
		 *asciiStr;
	int i;

	asciiStr = testdata_createCharStr(BUFF_SIZE + 1);

	TEST_ASSERT_NOT_NULL(asciiStr);

	for (i = 1; i < BUFF_SIZE; i++) {
		supportCharSet[i - 1] = i;
		reversCharSet[i - 1] = BUFF_SIZE - i;

		/*
		 * In this case we need to use fully filled set for strcspn
		 * because it counts size based on elements that are not
		 * present in himself
		 */
		TEST_ASSERT_EQUAL_INT(i - 1, strcspn(&asciiStr[1], &asciiStr[i]));
		TEST_ASSERT_EQUAL_INT(i, strspn(&asciiStr[1], supportCharSet));

		/* Checking if we reverse order strcspn and strspn will find elements that are correct */
		TEST_ASSERT_EQUAL_INT(BUFF_SIZE - 1, strcspn(&asciiStr[1], &reversCharSet[i]));

		if (i == 127) {
			TEST_ASSERT_EQUAL_INT(BUFF_SIZE - 1, strspn(&asciiStr[1], reversCharSet));
		}
		else {
			TEST_ASSERT_EQUAL_INT(0, strspn(&asciiStr[1], reversCharSet));
		}
	}

	free(asciiStr);
}


TEST(string_spn, not_ascii)
{
	unsigned char charSet[BUFF_SIZE] = { 0 };
	int len, i;

	/* Checking  out of ASCII bytes */
	for (i = sizeof(charSet); i < sizeof(charSet) * 2 - 1; i++) {
		charSet[i - sizeof(charSet)] = i;
	}

	len = sizeof(charSet) - 1;
	TEST_ASSERT_EQUAL_INT(len, strcspn((const char *)charSet, ""));
	TEST_ASSERT_EQUAL_INT(len, strspn((const char *)charSet, (const char *)charSet));
}


TEST(string_spn, big)
{
	char bigstr[PATH_MAX] = { 0 };
	int len;

	/*
	 * The length of the string is not restricted,
	 * so we test one of the bigger values, which may be used
	 * with remembering the last element must be a null term
	 * zero to collect the data as the string from
	 * declared space, that's why we memset one place less than its size
	 */

	len = sizeof(bigstr) - 1;
	memset(bigstr, 'a', len);

	TEST_ASSERT_EQUAL_INT(len, strcspn(bigstr, ""));
	TEST_ASSERT_EQUAL_INT(0, strcspn(bigstr, "a"));
	TEST_ASSERT_EQUAL_INT(len, strcspn(bigstr, "b"));
	TEST_ASSERT_EQUAL_INT(0, strcspn(bigstr, "ab"));

	TEST_ASSERT_EQUAL_INT(len, strspn(bigstr, "a"));
	TEST_ASSERT_EQUAL_INT(0, strspn(bigstr, ""));
	TEST_ASSERT_EQUAL_INT(0, strspn(bigstr, "b"));
	TEST_ASSERT_EQUAL_INT(len, strspn(bigstr, "ab"));

	bigstr[len - 3] = 'b';

	TEST_ASSERT_EQUAL_INT(len, strcspn(bigstr, ""));
	TEST_ASSERT_EQUAL_INT(0, strcspn(bigstr, "a"));
	TEST_ASSERT_EQUAL_INT(len - 3, strcspn(bigstr, "b"));
	TEST_ASSERT_EQUAL_INT(0, strcspn(bigstr, "ab"));

	TEST_ASSERT_EQUAL_INT(len - 3, strspn(bigstr, "a"));
	TEST_ASSERT_EQUAL_INT(0, strspn(bigstr, ""));
	TEST_ASSERT_EQUAL_INT(0, strspn(bigstr, "b"));
	TEST_ASSERT_EQUAL_INT(len, strspn(bigstr, "ab"));
}


TEST(string_spn, empty_args)
{
	TEST_ASSERT_EQUAL_INT(0, strcspn("", "abc"));
	TEST_ASSERT_EQUAL_INT(0, strspn("", "abc"));

	TEST_ASSERT_EQUAL_INT(0, strcspn("", ""));
	TEST_ASSERT_EQUAL_INT(0, strspn("", ""));

	TEST_ASSERT_EQUAL_INT(3, strcspn("abc", ""));
	TEST_ASSERT_EQUAL_INT(0, strspn("abc", ""));
}


/* This case is intended to check the ability to stop at the first byte */
TEST(string_spn, first_byte)
{
	TEST_ASSERT_EQUAL_INT(0, strspn("abc", "bc"));
	TEST_ASSERT_EQUAL_INT(0, strspn("abc", "c"));
	TEST_ASSERT_EQUAL_INT(0, strspn("aaaabc", "bc"));
	TEST_ASSERT_EQUAL_INT(0, strspn("aaaabc", "c"));


	TEST_ASSERT_EQUAL_INT(0, strcspn("abc", "abc"));
	TEST_ASSERT_EQUAL_INT(0, strcspn("abc", "abc"));
	TEST_ASSERT_EQUAL_INT(0, strcspn("abc", "ab"));
	TEST_ASSERT_EQUAL_INT(0, strcspn("abc", "a"));
	TEST_ASSERT_EQUAL_INT(0, strcspn("aaaabc", "ab"));
	TEST_ASSERT_EQUAL_INT(0, strcspn("aaaabc", "a"));
}


TEST(string_spn, mixed_order)
{
	int i;
	char reversStr[BUFF_SIZE],
		*testStr;

	testStr = testdata_createCharStr(BUFF_SIZE);

	TEST_ASSERT_NOT_NULL(testStr);

	TEST_ASSERT_NOT_NULL(reversStr);

	for (i = 0; i < BUFF_SIZE - 1; i++) {
		reversStr[i] = testStr[BUFF_SIZE - i - 2];
	}
	reversStr[BUFF_SIZE - 1] = 0;

	TEST_ASSERT_EQUAL_INT(0, strcspn("abc", "cba"));
	TEST_ASSERT_EQUAL_INT(3, strspn("abc", "cba"));

	TEST_ASSERT_EQUAL_INT(0, strcspn("abc", "bac"));
	TEST_ASSERT_EQUAL_INT(3, strspn("abc", "bac"));

	TEST_ASSERT_EQUAL_INT(0, strcspn("abc", "bca"));
	TEST_ASSERT_EQUAL_INT(3, strspn("abc", "bca"));

	TEST_ASSERT_EQUAL_INT(0, strcspn(testStr, reversStr));
	TEST_ASSERT_EQUAL_INT(BUFF_SIZE - 1, strspn(testStr, reversStr));

	TEST_ASSERT_EQUAL_INT(0, strcspn("abc", "aa"));
	TEST_ASSERT_EQUAL_INT(1, strcspn("abc", "bb"));
	TEST_ASSERT_EQUAL_INT(2, strcspn("abc", "cc"));
	TEST_ASSERT_EQUAL_INT(0, strcspn("abc", "aabbcc"));
	TEST_ASSERT_EQUAL_INT(0, strcspn("abc", "aaaaaa"));
	TEST_ASSERT_EQUAL_INT(1, strcspn("abc", "bbbbbb"));
	TEST_ASSERT_EQUAL_INT(2, strcspn("abc", "cccccc"));
	TEST_ASSERT_EQUAL_INT(0, strcspn("abc", "ccaabb"));
	TEST_ASSERT_EQUAL_INT(0, strcspn("abc", "bbaacc"));


	TEST_ASSERT_EQUAL_INT(1, strspn("abc", "aa"));
	TEST_ASSERT_EQUAL_INT(0, strspn("abc", "bb"));
	TEST_ASSERT_EQUAL_INT(0, strspn("abc", "cc"));
	TEST_ASSERT_EQUAL_INT(3, strspn("abc", "aabbcc"));
	TEST_ASSERT_EQUAL_INT(1, strspn("abc", "aaaaaa"));
	TEST_ASSERT_EQUAL_INT(0, strspn("abc", "bbbbbb"));
	TEST_ASSERT_EQUAL_INT(0, strspn("abc", "cccccc"));
	TEST_ASSERT_EQUAL_INT(3, strspn("abc", "ccaabb"));
	TEST_ASSERT_EQUAL_INT(3, strspn("abc", "bbaacc"));

	free(testStr);
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
	int len;

	len = strlen(lorem);

	/* Check of proper working base */
	TEST_ASSERT_EQUAL_PTR(lorem, strchr(lorem, 'L'));
	TEST_ASSERT_EQUAL_PTR(lorem, strrchr(lorem, 'L'));
	TEST_ASSERT_EQUAL_PTR(lorem, memchr(lorem, 'L', len));

	TEST_ASSERT_EQUAL_PTR(&lorem[14], strchr(lorem, 'l'));
	TEST_ASSERT_EQUAL_PTR(&lorem[52], strrchr(lorem, 'l'));
	TEST_ASSERT_EQUAL_PTR(&lorem[14], memchr(lorem, 'l', len));

	TEST_ASSERT_EQUAL_PTR(NULL, strchr(lorem, 'x'));
	TEST_ASSERT_EQUAL_PTR(NULL, strrchr(lorem, 'x'));
	TEST_ASSERT_EQUAL_PTR(NULL, memchr(lorem, 'x', len));

	TEST_ASSERT_EQUAL_PTR(&lorem[len], strchr(lorem, lorem[len]));
	TEST_ASSERT_EQUAL_PTR(&lorem[len], strrchr(lorem, lorem[len]));
	TEST_ASSERT_EQUAL_PTR(&lorem[len], memchr(lorem, lorem[len], len + 1));
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
	int len;

	/*
	 * Testing did strchr don't stop if found special signs
	 * Getting the first element from the string contains only special characters
	 */

	len = strlen(specials);

	TEST_ASSERT_EQUAL_PTR(specials, strchr(specials, specials[0]));
	TEST_ASSERT_EQUAL_PTR(specials, strrchr(specials, specials[0]));
	TEST_ASSERT_EQUAL_PTR(specials, memchr(specials, specials[0], strlen(specials)));

	/* Getting the last element from string contains only special characters */
	TEST_ASSERT_EQUAL_PTR(&specials[len], strchr(specials, specials[len]));
	TEST_ASSERT_EQUAL_PTR(&specials[len], strrchr(specials, specials[len]));
	TEST_ASSERT_EQUAL_PTR(&specials[len], memchr(specials, specials[len], len + 1));

	/* Getting the middle element from string contains only special characters */
	TEST_ASSERT_EQUAL_PTR(&specials[len / 2], strchr(specials, specials[len / 2]));
	TEST_ASSERT_EQUAL_PTR(&specials[len / 2], strrchr(specials, specials[len / 2]));
	TEST_ASSERT_EQUAL_PTR(&specials[len / 2], memchr(specials, specials[len / 2], len + 1));

	/* Getting element which is not into string */
	TEST_ASSERT_EQUAL_PTR(NULL, strchr(specials, 'I'));
	TEST_ASSERT_EQUAL_PTR(NULL, strrchr(specials, 'I'));
	TEST_ASSERT_EQUAL_PTR(NULL, memchr(specials, 'I', len));
}


TEST(string_chr, ascii)
{
	int len;
	char *asciiStr;

	asciiStr = testdata_createCharStr(INT8_MAX + 1);

	TEST_ASSERT_NOT_NULL(asciiStr);

	len = INT8_MAX;


	TEST_ASSERT_EQUAL_PTR(asciiStr, strchr(asciiStr, asciiStr[0]));
	/* In this scenario we need to use second place of array because testdata create string with 0 and replace it with 1 */
	TEST_ASSERT_EQUAL_PTR(&asciiStr[1], strrchr(asciiStr, asciiStr[0]));
	TEST_ASSERT_EQUAL_PTR(asciiStr, memchr(asciiStr, asciiStr[0], len + 1));

	TEST_ASSERT_EQUAL_PTR(&asciiStr[len / 2], strchr(asciiStr, asciiStr[len / 2]));
	TEST_ASSERT_EQUAL_PTR(&asciiStr[len / 2], strrchr(asciiStr, asciiStr[len / 2]));
	TEST_ASSERT_EQUAL_PTR(&asciiStr[len / 2], memchr(asciiStr, asciiStr[len / 2], len + 1));

	TEST_ASSERT_EQUAL_PTR(&asciiStr[len], strchr(asciiStr, asciiStr[len]));
	TEST_ASSERT_EQUAL_PTR(&asciiStr[len], strrchr(asciiStr, asciiStr[len]));
	TEST_ASSERT_EQUAL_PTR(&asciiStr[len], memchr(asciiStr, asciiStr[len], len + 1));

	free(asciiStr);
}


TEST(string_chr, not_ascii_chars)
{
	char notAsciiStr[129] = { 0 };
	int sz, i;

	/*
	 * Depending on architecture we will get
	 * output {128;255} if chars are unsigned or {-128;-1} when chars are signed
	 */

	notAsciiStr[128] = 0;
	sz = sizeof(notAsciiStr);
	for (i = 0; i <= 127; i++) {
		notAsciiStr[i] = i + 128;
		/* Testing capability of functions to hold and read not ascii set */
		TEST_ASSERT_EQUAL_STRING(&notAsciiStr[i], strchr(notAsciiStr, notAsciiStr[i]));
		TEST_ASSERT_EQUAL_STRING(&notAsciiStr[i], strrchr(notAsciiStr, notAsciiStr[i]));
		TEST_ASSERT_EQUAL_STRING(&notAsciiStr[i], memchr(notAsciiStr, notAsciiStr[i], sz));
	}
}

TEST(string_chr, int_to_char_cast)
{
	char str[2];
	int intVal[] = {
		INT_MIN,
		INT_MIN / 3,
		-514,
		-256,
		-129,
		129,
		256,
		514,
		INT_MAX / 3,
		INT_MAX
	},
		i;

	for (i = 0; i < sizeof(intVal) / sizeof(int); i++) {
		/* Copy value into first place in array as char*/
		str[0] = intVal[i];
		/*Setting up 0 on second place to recognize array as string*/
		str[1] = 0;

		TEST_ASSERT_EQUAL_PTR(&str[0], strrchr(str, intVal[i]));
	}
}


TEST(string_chr, torn)
{
	char *torn = "foo\0bar";
	int len;

	/* Checking correct working of null terminating point */
	len = strlen(torn) + 1;
	TEST_ASSERT_EQUAL_STRING("", strchr(torn, '\0'));
	TEST_ASSERT_EQUAL_STRING("", strrchr(torn, '\0'));
	TEST_ASSERT_EQUAL_STRING("", memchr(torn, '\0', len));

	TEST_ASSERT_EQUAL_STRING("foo", strchr(torn, 'f'));
	TEST_ASSERT_EQUAL_STRING("foo", strrchr(torn, 'f'));
	TEST_ASSERT_EQUAL_STRING("foo", memchr(torn, 'f', len));

	TEST_ASSERT_EQUAL_PTR(NULL, strchr(torn, 'b'));
	TEST_ASSERT_EQUAL_PTR(NULL, strrchr(torn, 'b'));
	TEST_ASSERT_EQUAL_PTR(NULL, memchr(torn, 'b', len));
}


TEST(string_chr, whitespaces)
{
	const char *exp = "Ipsum",
			   *strWhites = "Lorem \n\t\e\r\b\v\f\\ Ipsum",
			   whites[] = " \n\t\e\r\b\v\f\\";
	int len, i;

	/* Checking if white signs don't make any interference on functions output */
	len = sizeof(whites) - 1;

	for (i = 0; i < len; i++) {
		TEST_ASSERT_EQUAL_PTR(&whites[i], strchr(whites, whites[i]));
		TEST_ASSERT_EQUAL_PTR(&whites[i], strrchr(whites, whites[i]));
		TEST_ASSERT_EQUAL_PTR(&whites[i], memchr(whites, whites[i], i + 1));
	}

	TEST_ASSERT_EQUAL_PTR(NULL, strchr(whites, 'x'));
	TEST_ASSERT_EQUAL_PTR(NULL, strrchr(whites, 'x'));
	TEST_ASSERT_EQUAL_PTR(NULL, memchr(whites, 'x', len));

	len = strlen(strWhites);
	/* Testing if whitespaces interference output of functions */
	TEST_ASSERT_EQUAL_STRING(exp, strchr(strWhites, 'I'));
	TEST_ASSERT_EQUAL_STRING(exp, strrchr(strWhites, 'I'));
	TEST_ASSERT_EQUAL_STRING(exp, memchr(strWhites, 'I', len));
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
		 * where is the search char placed and memchr never meets
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


TEST_GROUP_RUNNER(string_spn)
{
	RUN_TEST_CASE(string_spn, basic);
	RUN_TEST_CASE(string_spn, ascii);
	RUN_TEST_CASE(string_spn, not_ascii);
	RUN_TEST_CASE(string_spn, empty_args);
	RUN_TEST_CASE(string_spn, first_byte);
	RUN_TEST_CASE(string_spn, mixed_order);
	RUN_TEST_CASE(string_spn, big);
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
	RUN_TEST_CASE(string_chr, not_ascii_chars);
	RUN_TEST_CASE(string_chr, int_to_char_cast);
	RUN_TEST_CASE(string_chr, torn);
}
