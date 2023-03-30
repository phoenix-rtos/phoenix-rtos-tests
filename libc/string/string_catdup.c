/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - string.h
 *
 * TESTED:
 *    - strcat()
 *    - strncat()
 *    - strdup()
 *    - strmdup()
 *
 * Copyright 2023 Phoenix Systems
 * Author: Mateusz Bloch
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>
#include <unity_fixture.h>
#include "testdata.h"


#define WORD_1          "Lorem "
#define WORD_2          "ipsum"
#define NON_ASCII_1     "\xe3\x83\x9e\xe3\x83\xaa\xe3\x82\xa2\xe3\x83\xbb"
#define NON_ASCII_2     "\xe3\x82\xb9\xe3\x82\xaf\xe3\x82\xa6\xe3\x82\xa9\xe3\x83\x89\xe3\x83\x95\xe3\x82\xb9\xe3\x82\xab\xef\xbc\x9d\xe3\x82\xad\xe3\x83\xa5\xe3\x83\xaa\xe3\x83\xbc"
#define SPECIALS_STRING "\n\t !\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~"
#define SIZE_BUF        32
#define SIZE_BIGGER_BUF 150
#define SIZE_ASCII_BUF  128


TEST_GROUP(string_cat);
TEST_GROUP(string_dup);


TEST_SETUP(string_cat)
{
}


TEST_TEAR_DOWN(string_cat)
{
}


TEST(string_cat, strcat_basic)
{
	int i;
	char buf[SIZE_BUF] = WORD_1;
	char buf1[] = WORD_2;

	TEST_ASSERT_EQUAL_PTR(buf, strcat(buf, buf1));
	TEST_ASSERT_EQUAL_STRING(WORD_1 WORD_2, buf);

	for (i = sizeof(WORD_1 WORD_2); i < SIZE_BUF; i++) {
		TEST_ASSERT_EQUAL_CHAR('\0', buf[i]);
	}
}


TEST(string_cat, strcat_empty_dest)
{
	int i;
	char buf[SIZE_BUF] = "";
	char buf1[] = WORD_2;

	TEST_ASSERT_EQUAL_PTR(buf, strcat(buf, buf1));
	TEST_ASSERT_EQUAL_STRING(WORD_2, buf);

	for (i = sizeof(WORD_2); i < SIZE_BUF; i++) {
		TEST_ASSERT_EQUAL_CHAR('\0', buf[i]);
	}
}


TEST(string_cat, strcat_empty_src)
{
	int i;
	char buf[SIZE_BUF] = WORD_1;
	char buf1[] = "";

	TEST_ASSERT_EQUAL_PTR(buf, strcat(buf, buf1));
	TEST_ASSERT_EQUAL_STRING(WORD_1, buf);

	for (i = sizeof(WORD_1); i < SIZE_BUF; i++) {
		TEST_ASSERT_EQUAL_CHAR('\0', buf[i]);
	}
}


TEST(string_cat, strcat_empty)
{
	int i;
	char buf[SIZE_BUF / 2] = "";
	char buf1[] = "";

	TEST_ASSERT_EQUAL_PTR(buf, strcat(buf, buf1));
	TEST_ASSERT_EQUAL_STRING("", buf);

	for (i = 0; i < SIZE_BUF / 2; i++) {
		TEST_ASSERT_EQUAL_CHAR('\0', buf[i]);
	}
}


TEST(string_cat, strcat_huge_string_dest)
{
	char buf[PATH_MAX] = "";
	char buf1[SIZE_BUF] = "";

	memcpy(buf, testdata_hugeStr, PATH_MAX - 1);

	TEST_ASSERT_EQUAL_PTR(buf, strcat(buf, buf1));
	TEST_ASSERT_EQUAL_CHAR_ARRAY(testdata_hugeStr, buf, PATH_MAX - 1);
	TEST_ASSERT_EQUAL_CHAR('\0', buf[PATH_MAX - 1]);
}


TEST(string_cat, strcat_huge_string_src)
{
	char buf[PATH_MAX] = "";
	char buf1[PATH_MAX] = "";

	memcpy(buf1, testdata_hugeStr, PATH_MAX - 1);

	TEST_ASSERT_EQUAL_PTR(buf, strcat(buf, buf1));
	TEST_ASSERT_EQUAL_CHAR_ARRAY(testdata_hugeStr, buf, PATH_MAX - 1);
	TEST_ASSERT_EQUAL_CHAR('\0', buf[PATH_MAX - 1]);
}


TEST(string_cat, strcat_huge_string_both)
{
	char buf[PATH_MAX] = "";
	char buf1[PATH_MAX / 2] = "";

	memcpy(buf, testdata_hugeStr, PATH_MAX / 2);
	memcpy(buf1, &testdata_hugeStr[PATH_MAX / 2], PATH_MAX / 2 - 1);

	TEST_ASSERT_EQUAL_PTR(buf, strcat(buf, buf1));
	TEST_ASSERT_EQUAL_CHAR_ARRAY(testdata_hugeStr, buf, PATH_MAX - 1);
	TEST_ASSERT_EQUAL_CHAR('\0', buf[PATH_MAX - 1]);
}


TEST(string_cat, strcat_specials_string)
{
	int i;
	const char *expectedStr = SPECIALS_STRING SPECIALS_STRING;
	char buf[SIZE_BUF * 4] = SPECIALS_STRING;
	char buf1[] = SPECIALS_STRING;

	TEST_ASSERT_EQUAL_PTR(buf, strcat(buf, buf1));
	TEST_ASSERT_EQUAL_STRING(expectedStr, buf);

	for (i = sizeof(SPECIALS_STRING SPECIALS_STRING) - 1; i < SIZE_BUF * 4; i++) {
		TEST_ASSERT_EQUAL_CHAR('\0', buf[i]);
	}
}


TEST(string_cat, strcat_overwrite)
{
	int i;
	char buf[SIZE_BUF];
	char buf1[] = "orem ipsum";

	memset(buf, 'X', sizeof(buf));
	buf[0] = 'L';
	buf[1] = '\0';

	TEST_ASSERT_EQUAL_PTR(buf, strcat(buf, buf1));
	TEST_ASSERT_EQUAL_STRING(WORD_1 WORD_2, buf);

	TEST_ASSERT_EQUAL_CHAR('\0', buf[sizeof(WORD_1 WORD_2) - 1]);
	for (i = sizeof(WORD_1 WORD_2); i < SIZE_BUF; i++) {
		TEST_ASSERT_EQUAL_CHAR('X', buf[i]);
	}
}


TEST(string_cat, strcat_ascii)
{
	int i;
	char buf[SIZE_BIGGER_BUF] = { 0 };
	char ascii[SIZE_ASCII_BUF] = { 0 };

	for (i = 1; i < sizeof(ascii); i++) {
		ascii[i - 1] = i;
	}

	TEST_ASSERT_EQUAL_PTR(buf, strcat(buf, ascii));
	TEST_ASSERT_EQUAL_STRING(ascii, buf);

	for (i = sizeof(ascii) + 1; i < SIZE_BIGGER_BUF; i++) {
		TEST_ASSERT_EQUAL_CHAR('\0', buf[i]);
	}
}


TEST(string_cat, strcat_extended_ascii)
{
	int i;
	char buf[SIZE_BIGGER_BUF] = { 0 };
	char ascii[SIZE_ASCII_BUF] = { 0 };

	for (i = 0; i < sizeof(ascii) - 1; ++i) {
		ascii[i] = i + 128;
	}

	TEST_ASSERT_EQUAL_PTR(buf, strcat(buf, ascii));
	TEST_ASSERT_EQUAL_STRING(ascii, buf);

	for (i = sizeof(ascii) + 1; i < SIZE_BIGGER_BUF; i++) {
		TEST_ASSERT_EQUAL_CHAR('\0', buf[i]);
	}
}


TEST(string_cat, strcat_non_ascii)
{
	int i;
	char buf[SIZE_BUF * 2] = NON_ASCII_1;
	char buf1[] = NON_ASCII_2;

	TEST_ASSERT_EQUAL_PTR(buf, strcat(buf, buf1));
	TEST_ASSERT_EQUAL_STRING(NON_ASCII_1 NON_ASCII_2, buf);

	for (i = sizeof(NON_ASCII_1 NON_ASCII_2) + 1; i < SIZE_BUF * 2; i++) {
		TEST_ASSERT_EQUAL_CHAR('\0', buf[i]);
	}
}

///////////////////////////////////////////////////////////////////////////////////

TEST(string_cat, strncat_basic)
{
	int i;
	char buf[SIZE_BUF] = WORD_1;
	char buf1[] = WORD_2;

	TEST_ASSERT_EQUAL_PTR(buf, strncat(buf, buf1, sizeof(WORD_2) + 1));
	TEST_ASSERT_EQUAL_STRING(WORD_1 WORD_2, buf);

	for (i = sizeof(WORD_1 WORD_2); i < SIZE_BUF; i++) {
		TEST_ASSERT_EQUAL_CHAR('\0', buf[i]);
	}
}


TEST(string_cat, strncat_zero_n)
{
	int i;
	char buf[SIZE_BUF] = WORD_1;
	char buf1[] = WORD_2;

	TEST_ASSERT_EQUAL_PTR(buf, strncat(buf, buf1, 0));
	TEST_ASSERT_EQUAL_STRING(WORD_1, buf);

	for (i = sizeof(WORD_1); i < SIZE_BUF; i++) {
		TEST_ASSERT_EQUAL_CHAR('\0', buf[i]);
	}
}


TEST(string_cat, strncat_exceed)
{
	int i;
	char buf[SIZE_BUF] = WORD_1;
	char buf1[] = WORD_2;

	TEST_ASSERT_EQUAL_PTR(buf, strncat(buf, buf1, sizeof(WORD_2)));
	TEST_ASSERT_EQUAL_STRING(WORD_1 WORD_2, buf);

	for (i = strlen(WORD_1 WORD_2); i < SIZE_BUF; i++) {
		TEST_ASSERT_EQUAL_CHAR('\0', buf[i]);
	}
}


TEST(string_cat, strncat_trimming)
{
/* Warning redundant, we turn it off, because we check if for n smaller than the size of the string, n characters will be copied */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"

	char buf[SIZE_BUF] = WORD_1;
	char buf1[] = WORD_2;
	const char *str = WORD_1 WORD_2;
	const size_t n = sizeof(buf1) / 2;
	const int num_elements = sizeof(WORD_1) + n;

	TEST_ASSERT_EQUAL_PTR(buf, strncat(buf, buf1, n));
	TEST_ASSERT_EQUAL_CHAR_ARRAY(str, buf, num_elements - 1);
	TEST_ASSERT_EQUAL_CHAR('\0', buf[num_elements]);

#pragma GCC diagnostic pop
}


TEST(string_cat, strncat_empty_dest)
{
	int i;
	char buf[SIZE_BUF] = "";
	char buf1[] = WORD_2;

	TEST_ASSERT_EQUAL_PTR(buf, strncat(buf, buf1, sizeof(WORD_2)));
	TEST_ASSERT_EQUAL_STRING(WORD_2, buf);

	for (i = sizeof(WORD_2); i < SIZE_BUF; i++) {
		TEST_ASSERT_EQUAL_CHAR('\0', buf[i]);
	}
}


TEST(string_cat, strncat_empty_src)
{
	int i;
	char buf[SIZE_BUF] = WORD_1;
	char buf1[] = "";

	TEST_ASSERT_EQUAL_PTR(buf, strncat(buf, buf1, 0));
	TEST_ASSERT_EQUAL_STRING(WORD_1, buf);

	for (i = sizeof(WORD_1); i < SIZE_BUF; i++) {
		TEST_ASSERT_EQUAL_CHAR('\0', buf[i]);
	}
}


TEST(string_cat, strncat_huge_string_dest)
{
	char buf[PATH_MAX] = "";
	char buf1[] = "";

	memcpy(buf, testdata_hugeStr, PATH_MAX - 1);

	TEST_ASSERT_EQUAL_PTR(buf, strncat(buf, buf1, 1));
	TEST_ASSERT_EQUAL_CHAR_ARRAY(testdata_hugeStr, buf, PATH_MAX - 1);
	TEST_ASSERT_EQUAL_CHAR('\0', buf[PATH_MAX - 1]);
}


TEST(string_cat, strncat_huge_string_src)
{
	char buf[PATH_MAX + 1] = "";
	char buf1[PATH_MAX] = "";

	memcpy(buf1, testdata_hugeStr, PATH_MAX);

	TEST_ASSERT_EQUAL_PTR(buf, strncat(buf, buf1, PATH_MAX));
	TEST_ASSERT_EQUAL_CHAR_ARRAY(testdata_hugeStr, buf, PATH_MAX);
	TEST_ASSERT_EQUAL_CHAR('\0', buf[PATH_MAX]);
}


TEST(string_cat, strncat_huge_string_both)
{
	char buf[PATH_MAX] = "";
	char buf1[PATH_MAX / 2] = "";

	memcpy(buf, testdata_hugeStr, PATH_MAX / 2);
	memcpy(buf1, &testdata_hugeStr[PATH_MAX / 2], PATH_MAX / 2 - 1);

	TEST_ASSERT_EQUAL_PTR(buf, strncat(buf, buf1, PATH_MAX / 2));
	TEST_ASSERT_EQUAL_CHAR_ARRAY(testdata_hugeStr, buf, PATH_MAX - 1);
	TEST_ASSERT_EQUAL_CHAR('\0', buf[PATH_MAX - 1]);
}


TEST(string_cat, strncat_huge_string_trimming)
{
/* Warning redundant, we turn it off, because we check if for n smaller than the size of the string, n characters will be copied */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"

	const size_t n = PATH_MAX / 4;
	const int num_elements = PATH_MAX / 2 + n;
	char buf[PATH_MAX] = "";
	char buf1[PATH_MAX / 2] = "";

	memcpy(buf, testdata_hugeStr, PATH_MAX / 2);
	memcpy(buf1, &testdata_hugeStr[PATH_MAX / 2], PATH_MAX / 2 - 1);

	TEST_ASSERT_EQUAL_PTR(buf, strncat(buf, buf1, n));
	TEST_ASSERT_EQUAL_CHAR_ARRAY(testdata_hugeStr, buf, num_elements - 1);
	TEST_ASSERT_EQUAL_CHAR('\0', buf[num_elements]);

#pragma GCC diagnostic pop
}


TEST(string_cat, strncat_specials_string)
{
	int i;
	const char *expectedStr = SPECIALS_STRING SPECIALS_STRING;
	const int expectedStrLen = sizeof(SPECIALS_STRING SPECIALS_STRING) - 1;

	char buf[SIZE_BUF * 4] = SPECIALS_STRING;
	char buf1[] = SPECIALS_STRING;

	TEST_ASSERT_EQUAL_PTR(buf, strncat(buf, buf1, sizeof(SPECIALS_STRING)));
	TEST_ASSERT_EQUAL_STRING(expectedStr, buf);

	for (i = expectedStrLen; i < SIZE_BUF * 4; i++) {
		TEST_ASSERT_EQUAL_CHAR('\0', buf[i]);
	}
}

TEST(string_cat, strncat_ascii)
{
	int i;
	char buf[SIZE_BIGGER_BUF] = { 0 };
	char ascii[SIZE_ASCII_BUF] = { 0 };

	for (i = 1; i < sizeof(ascii); i++) {
		ascii[i - 1] = i;
	}

	TEST_ASSERT_EQUAL_PTR(buf, strncat(buf, ascii, SIZE_ASCII_BUF));
	TEST_ASSERT_EQUAL_STRING(ascii, buf);
	for (i = sizeof(ascii); i < SIZE_BIGGER_BUF; i++) {
		TEST_ASSERT_EQUAL_CHAR('\0', buf[i]);
	}
}


TEST(string_cat, strncat_extended_ascii)
{
	int i;
	char buf[SIZE_BIGGER_BUF] = { 0 };
	char ascii[SIZE_ASCII_BUF] = { 0 };

	for (i = 0; i < sizeof(ascii) - 1; ++i) {
		ascii[i] = i + 128;
	}

	TEST_ASSERT_EQUAL_PTR(buf, strncat(buf, ascii, SIZE_ASCII_BUF));
	TEST_ASSERT_EQUAL_STRING(ascii, buf);
	for (i = sizeof(ascii); i < SIZE_BIGGER_BUF; i++) {
		TEST_ASSERT_EQUAL_CHAR('\0', buf[i]);
	}
}


TEST(string_cat, strncat_non_ascii)
{
	int i;
	const char *expectedStr = NON_ASCII_1 NON_ASCII_2;
	const size_t expectedStrLen = sizeof(NON_ASCII_1 NON_ASCII_2) - 1;

	char buf[SIZE_BUF * 2] = NON_ASCII_1;
	char buf1[] = NON_ASCII_2;

	TEST_ASSERT_EQUAL_PTR(buf, strncat(buf, buf1, expectedStrLen));
	TEST_ASSERT_EQUAL_STRING(expectedStr, buf);

	for (i = expectedStrLen; i < SIZE_BUF * 2; i++) {
		TEST_ASSERT_EQUAL_CHAR('\0', buf[i]);
	}
}

/////////////////////////////////////////////////////////////////////////

TEST_SETUP(string_dup)
{
}


TEST_TEAR_DOWN(string_dup)
{
}


TEST(string_dup, strdup_basic)
{
	char buf[SIZE_BUF] = WORD_1 WORD_2;
	char *buf1 = strdup(buf);

	TEST_ASSERT_NOT_NULL(buf1);

	TEST_ASSERT_EQUAL_STRING(buf, buf1);

	free(buf1);
}


TEST(string_dup, strdup_empty)
{
	char buf[2] = "";
	char *buf1 = strdup(buf);

	TEST_ASSERT_NOT_NULL(buf1);

	TEST_ASSERT_EQUAL_STRING(buf, buf1);

	free(buf1);
}


TEST(string_dup, strdup_huge_string)
{
	char *buf;
	char buf1[PATH_MAX] = "";

	memcpy(buf1, testdata_hugeStr, PATH_MAX - 1);

	buf = strdup(buf1);

	TEST_ASSERT_NOT_NULL(buf);

	TEST_ASSERT_EQUAL_CHAR_ARRAY(testdata_hugeStr, buf, PATH_MAX - 1);

	free(buf);
}


TEST(string_dup, strdup_specials_string)
{
	char buf[SIZE_BUF * 2] = SPECIALS_STRING;
	char *buf1 = strdup(buf);

	TEST_ASSERT_NOT_NULL(buf1);

	TEST_ASSERT_EQUAL_STRING(buf, buf1);

	free(buf1);
}


TEST(string_dup, strdup_ascii)
{
	int i;
	char *buf;
	char ascii[128] = { 0 };

	for (i = 1; i < 128; i++) {
		ascii[i - 1] = i;
	}

	buf = strdup(ascii);

	TEST_ASSERT_EQUAL_STRING(ascii, buf);

	free(buf);
}


TEST(string_dup, strdup_extended_ascii)
{
	int i;
	char *buf;
	char ascii[128] = { 0 };

	for (i = 128; i < 255; i++) {
		ascii[i - 128] = i;
	}

	buf = strdup(ascii);

	TEST_ASSERT_EQUAL_STRING(ascii, buf);

	free(buf);
}


TEST(string_dup, strdup_non_ascii)
{
	char buf[SIZE_BUF * 2] = NON_ASCII_1 NON_ASCII_2;
	char *buf1 = strdup(buf);

	TEST_ASSERT_NOT_NULL(buf1);

	TEST_ASSERT_EQUAL_STRING(buf, buf1);

	free(buf1);
}

///////////////////////////////////////////////////////////////////////////////////

TEST(string_dup, strndup_part)
{
	char buf[SIZE_BUF] = WORD_1 WORD_2;
	char *buf1 = strndup(buf, sizeof(WORD_1) - 1);

	TEST_ASSERT_NOT_NULL(buf1);

	TEST_ASSERT_EQUAL_STRING(WORD_1 WORD_2, buf);
	TEST_ASSERT_EQUAL_STRING(WORD_1, buf1);

	free(buf1);
}


TEST(string_dup, strndup_full_string)
{
	char buf[SIZE_BUF] = WORD_1 WORD_2;
	char *buf1 = strndup(buf, sizeof(WORD_1 WORD_2));

	TEST_ASSERT_NOT_NULL(buf1);

	TEST_ASSERT_EQUAL_STRING(buf, buf1);

	free(buf1);
}


TEST(string_dup, strndup_zero_size)
{
	char buf[SIZE_BUF] = WORD_1 WORD_2;
	char *buf1 = strndup(buf, 0);

	TEST_ASSERT_NOT_NULL(buf1);

	TEST_ASSERT_EQUAL_STRING(WORD_1 WORD_2, buf);
	TEST_ASSERT_EQUAL_STRING("", buf1);

	free(buf1);
}


TEST(string_dup, strndup_exceed)
{
	errno = 0;
	char buf[SIZE_BUF] = WORD_1 WORD_2;
	char *buf1 = strndup(buf, sizeof(buf));

	TEST_ASSERT_NOT_NULL(buf1);
	TEST_ASSERT_EQUAL_INT(0, errno);

	TEST_ASSERT_EQUAL_STRING(buf, buf1);

	free(buf1);
}


TEST(string_dup, strndup_specials_string)
{
	char buf[SIZE_BUF * 2] = SPECIALS_STRING;
	char *buf1 = strndup(buf, sizeof(SPECIALS_STRING));

	TEST_ASSERT_NOT_NULL(buf1);

	TEST_ASSERT_EQUAL_STRING(buf, buf1);

	free(buf1);
}


TEST(string_dup, strndup_huge_string_part)
{
	const int n = PATH_MAX / 2;
	char *buf;
	char buf1[PATH_MAX] = "";

	memcpy(buf1, testdata_hugeStr, PATH_MAX - 1);

	buf = strndup(buf1, n);

	TEST_ASSERT_NOT_NULL(buf);

	TEST_ASSERT_EQUAL_CHAR_ARRAY(testdata_hugeStr, buf, n);
	TEST_ASSERT_EQUAL_CHAR('\0', buf[n]);

	free(buf);
}


TEST(string_dup, strndup_huge_string_full)
{
	char *buf;
	char buf1[PATH_MAX] = "";

	memcpy(buf1, testdata_hugeStr, PATH_MAX - 1);

	buf = strndup(buf1, sizeof(buf1));

	TEST_ASSERT_NOT_NULL(buf);

	TEST_ASSERT_EQUAL_CHAR_ARRAY(testdata_hugeStr, buf, PATH_MAX - 1);
	TEST_ASSERT_EQUAL_CHAR('\0', buf[PATH_MAX - 1]);

	free(buf);
}


TEST(string_dup, strndup_ascii)
{
	int i;
	char *buf;
	char ascii[128] = { 0 };

	for (i = 1; i < 128; i++) {
		ascii[i - 1] = i;
	}

	buf = strndup(ascii, sizeof(ascii));

	TEST_ASSERT_EQUAL_STRING(ascii, buf);

	free(buf);
}


TEST(string_dup, strndup_extended_ascii)
{
	int i;
	char *buf;
	char ascii[128] = { 0 };

	for (i = 128; i < 255; i++) {
		ascii[i - 128] = i;
	}

	buf = strndup(ascii, sizeof(ascii));

	TEST_ASSERT_EQUAL_STRING(ascii, buf);

	free(buf);
}


TEST(string_dup, strndup_non_ascii)
{
	char buf[SIZE_BUF * 2] = NON_ASCII_1 NON_ASCII_2;
	char *buf1 = strndup(buf, sizeof(buf));

	TEST_ASSERT_NOT_NULL(buf1);

	TEST_ASSERT_EQUAL_STRING(buf, buf1);

	free(buf1);
}


TEST_GROUP_RUNNER(string_cat)
{
	RUN_TEST_CASE(string_cat, strcat_basic);
	RUN_TEST_CASE(string_cat, strcat_empty_dest);
	RUN_TEST_CASE(string_cat, strcat_empty_src);
	RUN_TEST_CASE(string_cat, strcat_empty);

	RUN_TEST_CASE(string_cat, strcat_huge_string_dest);
	RUN_TEST_CASE(string_cat, strcat_huge_string_src);
	RUN_TEST_CASE(string_cat, strcat_huge_string_both);

	RUN_TEST_CASE(string_cat, strcat_specials_string);
	RUN_TEST_CASE(string_cat, strcat_ascii);
	RUN_TEST_CASE(string_cat, strcat_extended_ascii);
	RUN_TEST_CASE(string_cat, strcat_non_ascii);
	RUN_TEST_CASE(string_cat, strcat_overwrite);


	RUN_TEST_CASE(string_cat, strncat_basic);
	RUN_TEST_CASE(string_cat, strncat_zero_n);
	RUN_TEST_CASE(string_cat, strncat_exceed);
	RUN_TEST_CASE(string_cat, strncat_trimming);
	RUN_TEST_CASE(string_cat, strncat_empty_dest);
	RUN_TEST_CASE(string_cat, strncat_empty_src);

	RUN_TEST_CASE(string_cat, strncat_huge_string_dest);
	RUN_TEST_CASE(string_cat, strncat_huge_string_src);
	RUN_TEST_CASE(string_cat, strncat_huge_string_both);
	RUN_TEST_CASE(string_cat, strncat_huge_string_trimming);

	RUN_TEST_CASE(string_cat, strncat_specials_string);
	RUN_TEST_CASE(string_cat, strncat_ascii);
	RUN_TEST_CASE(string_cat, strncat_extended_ascii);
	RUN_TEST_CASE(string_cat, strncat_non_ascii);
}


TEST_GROUP_RUNNER(string_dup)
{
	RUN_TEST_CASE(string_dup, strdup_basic);
	RUN_TEST_CASE(string_dup, strdup_empty);
	RUN_TEST_CASE(string_dup, strdup_huge_string);
	RUN_TEST_CASE(string_dup, strdup_specials_string);
	RUN_TEST_CASE(string_dup, strdup_ascii);
	RUN_TEST_CASE(string_dup, strdup_extended_ascii);
	RUN_TEST_CASE(string_dup, strdup_non_ascii);

	RUN_TEST_CASE(string_dup, strndup_part);
	RUN_TEST_CASE(string_dup, strndup_full_string);
	RUN_TEST_CASE(string_dup, strndup_zero_size);
	RUN_TEST_CASE(string_dup, strndup_exceed);
	RUN_TEST_CASE(string_dup, strndup_specials_string);
	RUN_TEST_CASE(string_dup, strndup_huge_string_part);
	RUN_TEST_CASE(string_dup, strndup_huge_string_full);
	RUN_TEST_CASE(string_dup, strndup_ascii);
	RUN_TEST_CASE(string_dup, strndup_extended_ascii);
	RUN_TEST_CASE(string_dup, strndup_non_ascii);
}
