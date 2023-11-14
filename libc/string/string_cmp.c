/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - string.h
 * TESTED:
 *    - memcmp()
 *    - strcmp()
 *    - strncmp()
 *    - strcoll()
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
#include <stdlib.h>
#include <unistd.h>
#include <unity_fixture.h>

#include "testdata.h"

#define BUFF_SIZE 129
#define BIG_SIZE  1024

static char empty[BUFF_SIZE];

TEST_GROUP(string_memcmp);
TEST_GROUP(string_strncmp);
TEST_GROUP(string_strcmp);
TEST_GROUP(string_strcoll);


/*
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/


TEST_SETUP(string_memcmp)
{
}


TEST_TEAR_DOWN(string_memcmp)
{
}


TEST(string_memcmp, basic)
{
	const char *basicStr = "Test";

	TEST_ASSERT_EQUAL_INT(0, memcmp(basicStr, basicStr, 5));
	TEST_ASSERT_LESS_THAN_INT(0, memcmp(basicStr, "Tests", 5));
	TEST_ASSERT_GREATER_THAN_INT(0, memcmp("Tests", basicStr, 5));
}


TEST(string_memcmp, unsigned_char_cast)
{
	const signed char charEdgeValue = SCHAR_MIN;

	TEST_ASSERT_GREATER_THAN_INT(0, memcmp(&charEdgeValue, "\0", 1));
	TEST_ASSERT_LESS_THAN_INT(0, memcmp("\0", &charEdgeValue, 1));
}


TEST(string_memcmp, emptyInput)
{
	char *asciiSet = testdata_createCharStr(BUFF_SIZE),
		 separated[] = "\0\0\0\0\0TEST\0\0";

	TEST_ASSERT_NOT_NULL(asciiSet);

	asciiSet[0] = 0;

	TEST_ASSERT_LESS_THAN_INT(0, memcmp(empty, asciiSet, sizeof(empty)));
	TEST_ASSERT_GREATER_THAN_INT(0, memcmp(asciiSet, empty, sizeof(empty)));
	TEST_ASSERT_EQUAL_INT(0, memcmp(empty, empty, sizeof(empty)));

	/* Memory cmp is not sensitive for NUL characters */
	TEST_ASSERT_NOT_EQUAL_INT(0, memcmp(empty, separated, sizeof(separated)));

	free(asciiSet);
}


TEST(string_memcmp, big)
{
	char *hugeStr = testdata_createCharStr(BIG_SIZE),
		 hugeStr2[BIG_SIZE];

	TEST_ASSERT_NOT_NULL(hugeStr);

	memcpy(hugeStr2, hugeStr, BIG_SIZE);

	TEST_ASSERT_EQUAL_INT(0, memcmp(hugeStr, hugeStr, BIG_SIZE));
	/* Comparing the same strings, that are placed in different location */
	TEST_ASSERT_EQUAL_INT(0, memcmp(hugeStr, hugeStr2, BIG_SIZE));

	hugeStr[BIG_SIZE - 2] = 1;
	hugeStr2[sizeof(hugeStr2) - 2] = 2;

	TEST_ASSERT_LESS_THAN_INT(0, memcmp(hugeStr, hugeStr2, BIG_SIZE));
	TEST_ASSERT_GREATER_THAN_INT(0, memcmp(hugeStr2, hugeStr, BIG_SIZE));

	free(hugeStr);
}


TEST(string_memcmp, various_sizes)
{
	int i;

	char *asciiStr = testdata_createCharStr(BUFF_SIZE),
		 asciiStr2[BUFF_SIZE];

	TEST_ASSERT_NOT_NULL(asciiStr);

	memcpy(asciiStr2, asciiStr, sizeof(asciiStr2));
	for (i = 1; i < BUFF_SIZE - 1; i++) {
		asciiStr2[i] = asciiStr[i] - 1;
		TEST_ASSERT_EQUAL_INT(0, memcmp(asciiStr, asciiStr2, i));
		TEST_ASSERT_GREATER_THAN_INT(0, memcmp(asciiStr, asciiStr2, i + 1));
		asciiStr2[i] = asciiStr[i];
	}

	free(asciiStr);
}


TEST(string_memcmp, offsets)
{

	char dataSet[4000] = { 0 },
		 supportSet[4000] = { 0 };
	int s1Offs, s2Offs, szOffset, sz = sizeof(dataSet);

	for (s1Offs = 0; s1Offs < sz; s1Offs++) {
		dataSet[s1Offs] = s1Offs;
		supportSet[s1Offs] = s1Offs;
	}

	/*Testing different offset of data blocks with same space or different*/
	for (s1Offs = 0; s1Offs < 8; s1Offs++) {
		for (s2Offs = 0; s2Offs < 8; s2Offs++) {
			for (szOffset = 0; szOffset < 8; szOffset++) {

				if (s2Offs < s1Offs) {
					TEST_ASSERT_GREATER_THAN_INT(0, memcmp(&dataSet[s1Offs], &dataSet[s2Offs], sz - (s1Offs + szOffset)));
					TEST_ASSERT_GREATER_THAN_INT(0, memcmp(&dataSet[s1Offs], &supportSet[s2Offs], sz - (s1Offs + szOffset)));
				}
				else if (s2Offs == s1Offs) {
					TEST_ASSERT_EQUAL_INT(0, memcmp(&dataSet[s1Offs], &dataSet[s2Offs], sz - (s1Offs + s2Offs + szOffset)));
					TEST_ASSERT_EQUAL_INT(0, memcmp(&dataSet[s1Offs], &supportSet[s2Offs], sz - (s1Offs + s2Offs + szOffset)));
				}
				else {
					TEST_ASSERT_LESS_THAN_INT(0, memcmp(&dataSet[s1Offs], &dataSet[s2Offs], sz - (s2Offs + szOffset)));
					TEST_ASSERT_LESS_THAN_INT(0, memcmp(&dataSet[s1Offs], &supportSet[s2Offs], sz - (s2Offs + szOffset)));
				}
			}
		}
	}
}


/*
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/


TEST_SETUP(string_strncmp)
{
}


TEST_TEAR_DOWN(string_strncmp)
{
}


TEST(string_strncmp, basic)
{
	const char *basicStr = "Test";

	TEST_ASSERT_EQUAL_INT(0, strncmp(basicStr, basicStr, 6));
	TEST_ASSERT_LESS_THAN_INT(0, strncmp(basicStr, "Tests", 6));
	TEST_ASSERT_GREATER_THAN_INT(0, strncmp("Tests", basicStr, 6));
}


TEST(string_strncmp, unsigned_char_cast)
{
	const char charEdgeValue[1] = { SCHAR_MIN };

	TEST_ASSERT_GREATER_THAN_INT(0, strncmp(charEdgeValue, "\0", 1));
	TEST_ASSERT_LESS_THAN_INT(0, strncmp("\0", charEdgeValue, 1));
}


TEST(string_strncmp, emptyInput)
{
	char *asciiStr = testdata_createCharStr(BUFF_SIZE),
		 separated[] = "\0\0\0\0\0TEST\0\0";

	TEST_ASSERT_NOT_NULL(asciiStr);

	TEST_ASSERT_EQUAL_INT(0, strncmp(empty, empty, BUFF_SIZE));
	TEST_ASSERT_LESS_THAN_INT(0, strncmp(empty, asciiStr, BUFF_SIZE));
	TEST_ASSERT_GREATER_THAN_INT(0, strncmp(asciiStr, empty, BUFF_SIZE));

	/* Otherwise than in memcmp, strncmp is NUL character sensitive and treats 0 as the end of array */
	TEST_ASSERT_EQUAL_INT(0, strncmp(empty, separated, BUFF_SIZE));

	free(asciiStr);
}


TEST(string_strncmp, big)
{
	char *hugeStr = testdata_createCharStr(BIG_SIZE),
		 hugeStr2[BIG_SIZE];

	TEST_ASSERT_NOT_NULL(hugeStr);

	memcpy(hugeStr2, hugeStr, BIG_SIZE);

	TEST_ASSERT_EQUAL_INT(0, strncmp(hugeStr, hugeStr, BIG_SIZE));
	/* Comparing the same strings, that are placed in different location */
	TEST_ASSERT_EQUAL_INT(0, strncmp(hugeStr, hugeStr2, BIG_SIZE));

	hugeStr[BIG_SIZE - 2] = 1;
	hugeStr2[sizeof(hugeStr2) - 2] = 2;

	TEST_ASSERT_LESS_THAN_INT(0, strncmp(hugeStr, hugeStr2, BIG_SIZE));
	TEST_ASSERT_GREATER_THAN_INT(0, strncmp(hugeStr2, hugeStr, BIG_SIZE));

	free(hugeStr);
}


TEST(string_strncmp, various_sizes)
{
	int i;

	char *asciiStr = testdata_createCharStr(BUFF_SIZE),
		 asciiStr2[BUFF_SIZE];

	TEST_ASSERT_NOT_NULL(asciiStr);

	memcpy(asciiStr2, asciiStr, sizeof(asciiStr2));
	for (i = 1; i < BUFF_SIZE - 1; i++) {
		asciiStr2[i] = asciiStr[i] - 1;
		TEST_ASSERT_EQUAL_INT(0, strncmp(asciiStr, asciiStr2, i));
		TEST_ASSERT_GREATER_THAN_INT(0, strncmp(asciiStr, asciiStr2, i + 1));
		asciiStr2[i] = asciiStr[i];
	}

	free(asciiStr);
}


TEST(string_strncmp, offsets)
{
	char dataSet[4000] = { 0 },
		 supportSet[4000] = { 0 };
	int s1Offs, s2Offs, szOffset, sz = sizeof(dataSet);

	memset(dataSet, 1, sizeof(dataSet));
	memset(supportSet, 1, sizeof(supportSet));

	for (s1Offs = 0; s1Offs < sz; s1Offs++) {
		dataSet[s1Offs] = s1Offs;
		supportSet[s1Offs] = s1Offs;
	}

	/*Testing different offset of data blocks with same space or different*/
	for (s1Offs = 0; s1Offs < 8; s1Offs++) {
		for (s2Offs = 0; s2Offs < 8; s2Offs++) {
			for (szOffset = 0; szOffset < 8; szOffset++) {

				if (s2Offs < s1Offs) {
					TEST_ASSERT_GREATER_THAN_INT(0, strncmp(&dataSet[s1Offs], &dataSet[s2Offs], sz - (s1Offs + szOffset)));
					TEST_ASSERT_GREATER_THAN_INT(0, strncmp(&dataSet[s1Offs], &supportSet[s2Offs], sz - (s1Offs + szOffset)));
				}
				else if (s2Offs == s1Offs) {
					TEST_ASSERT_EQUAL_INT(0, strncmp(&dataSet[s1Offs], &dataSet[s2Offs], sz - (s1Offs + s2Offs + szOffset)));
					TEST_ASSERT_EQUAL_INT(0, strncmp(&dataSet[s1Offs], &supportSet[s2Offs], sz - (s1Offs + s2Offs + szOffset)));
				}
				else {
					TEST_ASSERT_LESS_THAN_INT(0, strncmp(&dataSet[s1Offs], &dataSet[s2Offs], sz - (s2Offs + szOffset)));
					TEST_ASSERT_LESS_THAN_INT(0, strncmp(&dataSet[s1Offs], &supportSet[s2Offs], sz - (s2Offs + szOffset)));
				}
			}
		}
	}
}


/*
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/


TEST_SETUP(string_strcmp)
{
}


TEST_TEAR_DOWN(string_strcmp)
{
}


TEST(string_strcmp, basic)
{
	const char *basicStr = "Test";

	TEST_ASSERT_EQUAL_INT(0, strcmp(basicStr, basicStr));
	TEST_ASSERT_LESS_THAN_INT(0, strcmp(basicStr, "Tests"));
	TEST_ASSERT_GREATER_THAN_INT(0, strcmp("Tests", basicStr));
}


TEST(string_strcmp, unsigned_char_cast)
{
	const char charEdgeValue[1] = { SCHAR_MIN };

	TEST_ASSERT_GREATER_THAN_INT(0, strcmp(charEdgeValue, "\0"));
	TEST_ASSERT_LESS_THAN_INT(0, strcmp("\0", charEdgeValue));
}


TEST(string_strcmp, emptyInput)
{
	char *asciiStr = testdata_createCharStr(BUFF_SIZE),
		 separated[] = "\0\0\0\0\0TEST\0\0";

	TEST_ASSERT_NOT_NULL(asciiStr);

	TEST_ASSERT_LESS_THAN_INT(0, strcmp(empty, asciiStr));
	TEST_ASSERT_GREATER_THAN_INT(0, strcmp(asciiStr, empty));
	TEST_ASSERT_EQUAL_INT(0, strcmp(empty, empty));

	/* Otherwise than in memcmp, strcmp is NUL character sensitive and treats 0 as the end of array */
	TEST_ASSERT_EQUAL_INT(0, strcmp(empty, separated));

	free(asciiStr);
}


TEST(string_strcmp, big)
{
	char *hugeStr = testdata_createCharStr(BIG_SIZE),
		 hugeStr2[BIG_SIZE];

	TEST_ASSERT_NOT_NULL(hugeStr);

	memcpy(hugeStr2, hugeStr, BIG_SIZE);

	TEST_ASSERT_EQUAL_INT(0, strcmp(hugeStr, hugeStr));
	/* Comparing the same strings, that are placed in different location */
	TEST_ASSERT_EQUAL_INT(0, strcmp(hugeStr, hugeStr2));

	hugeStr[BIG_SIZE - 2] = 1;
	hugeStr2[sizeof(hugeStr2) - 2] = 2;

	TEST_ASSERT_LESS_THAN_INT(0, strcmp(hugeStr, hugeStr2));
	TEST_ASSERT_GREATER_THAN_INT(0, strcmp(hugeStr2, hugeStr));

	free(hugeStr);
}


TEST(string_strcmp, offsets)
{
	char dataSet[4000] = { 0 },
		 supportSet[4000] = { 0 };
	int s1Offs, s2Offs, szOffset, sz = sizeof(dataSet);

	memset(dataSet, 1, sizeof(dataSet));
	memset(supportSet, 1, sizeof(supportSet));

	for (s1Offs = 0; s1Offs < sz; s1Offs++) {
		dataSet[s1Offs] = s1Offs;
		supportSet[s1Offs] = s1Offs;
	}

	/*Testing different offset of data blocks with same space or different*/
	for (s1Offs = 0; s1Offs < 8; s1Offs++) {
		for (s2Offs = 0; s2Offs < 8; s2Offs++) {
			for (szOffset = 0; szOffset < 8; szOffset++) {

				if (s2Offs < s1Offs) {
					TEST_ASSERT_GREATER_THAN_INT(0, strcmp(&dataSet[s1Offs], &dataSet[s2Offs]));
					TEST_ASSERT_GREATER_THAN_INT(0, strcmp(&dataSet[s1Offs], &supportSet[s2Offs]));
				}
				else if (s2Offs == s1Offs) {
					TEST_ASSERT_EQUAL_INT(0, strcmp(&dataSet[s1Offs], &dataSet[s2Offs]));
					TEST_ASSERT_EQUAL_INT(0, strcmp(&dataSet[s1Offs], &supportSet[s2Offs]));
				}
				else {
					TEST_ASSERT_LESS_THAN_INT(0, strcmp(&dataSet[s1Offs], &dataSet[s2Offs]));
					TEST_ASSERT_LESS_THAN_INT(0, strcmp(&dataSet[s1Offs], &supportSet[s2Offs]));
				}
			}
		}
	}
}


/*
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/


TEST_SETUP(string_strcoll)
{
}


TEST_TEAR_DOWN(string_strcoll)
{
}


TEST(string_strcoll, basic)
{
	const char *basicStr = "Test";

	TEST_ASSERT_EQUAL_INT(0, strcmp(basicStr, basicStr));
	TEST_ASSERT_LESS_THAN_INT(0, strcmp(basicStr, "Tests"));
	TEST_ASSERT_GREATER_THAN_INT(0, strcmp("Tests", basicStr));
}


TEST(string_strcoll, emptyInput)
{
	char *asciiStr = testdata_createCharStr(BUFF_SIZE),
		 separated[] = "\0\0\0\0\0TEST\0\0";

	TEST_ASSERT_NOT_NULL(asciiStr);

	TEST_ASSERT_EQUAL_INT(0, strcoll(empty, empty));
	TEST_ASSERT_LESS_THAN_INT(0, strcoll(empty, asciiStr));
	TEST_ASSERT_GREATER_THAN_INT(0, strcoll(asciiStr, empty));

	/* Otherwise than in memcmp, strcoll is NUL character sensitive and treats 0 as the end of array */
	TEST_ASSERT_EQUAL_INT(0, strcoll(empty, separated));

	free(asciiStr);
}


TEST(string_strcoll, big)
{
	char *hugeStr = testdata_createCharStr(BIG_SIZE),
		 hugeStr2[BIG_SIZE];

	TEST_ASSERT_NOT_NULL(hugeStr);

	memcpy(hugeStr2, hugeStr, BIG_SIZE);

	TEST_ASSERT_EQUAL_INT(0, strcoll(hugeStr, hugeStr));
	/* Comparing the same strings, that are placed in different location */
	TEST_ASSERT_EQUAL_INT(0, strcoll(hugeStr, hugeStr2));
	hugeStr[BIG_SIZE - 2] = 1;
	hugeStr2[sizeof(hugeStr2) - 2] = 2;
	TEST_ASSERT_LESS_THAN_INT(0, strcoll(hugeStr, hugeStr2));
	TEST_ASSERT_GREATER_THAN_INT(0, strcoll(hugeStr2, hugeStr));

	free(hugeStr);
}


TEST(string_strcoll, offsets)
{
	char dataSet[4000] = { 0 },
		 supportSet[4000] = { 0 };
	int s1Offs, s2Offs, szOffset, sz = sizeof(dataSet);

	memset(dataSet, 1, sizeof(dataSet));
	memset(supportSet, 1, sizeof(supportSet));

	for (s1Offs = 0; s1Offs < sz; s1Offs++) {
		dataSet[s1Offs] = s1Offs;
		supportSet[s1Offs] = s1Offs;
	}

	/*Testing different offset of data blocks with same space or different*/
	for (s1Offs = 0; s1Offs < 8; s1Offs++) {
		for (s2Offs = 0; s2Offs < 8; s2Offs++) {
			for (szOffset = 0; szOffset < 8; szOffset++) {

				if (s2Offs < s1Offs) {
					TEST_ASSERT_GREATER_THAN_INT(0, strcoll(&dataSet[s1Offs], &dataSet[s2Offs]));
					TEST_ASSERT_GREATER_THAN_INT(0, strcoll(&dataSet[s1Offs], &supportSet[s2Offs]));
				}
				else if (s2Offs == s1Offs) {
					TEST_ASSERT_EQUAL_INT(0, strcoll(&dataSet[s1Offs], &dataSet[s2Offs]));
					TEST_ASSERT_EQUAL_INT(0, strcoll(&dataSet[s1Offs], &supportSet[s2Offs]));
				}
				else {
					TEST_ASSERT_LESS_THAN_INT(0, strcoll(&dataSet[s1Offs], &dataSet[s2Offs]));
					TEST_ASSERT_LESS_THAN_INT(0, strcoll(&dataSet[s1Offs], &supportSet[s2Offs]));
				}
			}
		}
	}
}


/*
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/


TEST_GROUP_RUNNER(string_memcmp)
{
	RUN_TEST_CASE(string_memcmp, basic);
	RUN_TEST_CASE(string_memcmp, unsigned_char_cast);
	RUN_TEST_CASE(string_memcmp, emptyInput);
	RUN_TEST_CASE(string_memcmp, big);
	RUN_TEST_CASE(string_memcmp, various_sizes);
	RUN_TEST_CASE(string_memcmp, offsets);
}


TEST_GROUP_RUNNER(string_strncmp)
{
	RUN_TEST_CASE(string_strncmp, basic);
	RUN_TEST_CASE(string_strncmp, unsigned_char_cast);
	RUN_TEST_CASE(string_strncmp, emptyInput);
	RUN_TEST_CASE(string_strncmp, big);
	RUN_TEST_CASE(string_strncmp, various_sizes);
	RUN_TEST_CASE(string_strncmp, offsets);
}


TEST_GROUP_RUNNER(string_strcmp)
{
	RUN_TEST_CASE(string_strcmp, basic);
	RUN_TEST_CASE(string_strcmp, unsigned_char_cast);
	RUN_TEST_CASE(string_strcmp, emptyInput);
	RUN_TEST_CASE(string_strcmp, big);
	RUN_TEST_CASE(string_strcmp, offsets);
}


TEST_GROUP_RUNNER(string_strcoll)
{
	RUN_TEST_CASE(string_strcoll, basic);
	RUN_TEST_CASE(string_strcoll, emptyInput);
	RUN_TEST_CASE(string_strcoll, big);
	RUN_TEST_CASE(string_strcoll, offsets);
}
