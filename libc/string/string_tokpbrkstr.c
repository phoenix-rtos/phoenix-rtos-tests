/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 * 	- string.h
 * TESTED:
 * 	- strpbrk()
 * 	- strstr()
 * 	- strtok()
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

#define ASCII_LENGTH          128
#define EXTENDED_ASCII_LENGTH 256
#define BUFFSIZE              24
#define LOREM_IPSUM           "Lorem Ipsum Dolor"

TEST_GROUP(string_tok);
TEST_GROUP(string_tok_r);
TEST_GROUP(string_str);
TEST_GROUP(string_pbrk);

static char *create_extAscii_set(void)
{
	int i;
	char *hold = malloc(EXTENDED_ASCII_LENGTH);

	TEST_ASSERT_NOT_NULL(hold);

	for (i = 1; i < EXTENDED_ASCII_LENGTH; i++) {
		hold[i - 1] = i;
	}
	hold[i - 1] = 0;

	return hold;
}


TEST_SETUP(string_tok)
{
}


TEST_TEAR_DOWN(string_tok)
{
}


TEST(string_tok, basic)
{
	char str[] = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Maecenas eleifend elementum tellum.";
	const char *separators = ", .";
	char *token;

	TEST_ASSERT_EQUAL_PTR(str, (token = strtok(str, separators)));
	TEST_ASSERT_EQUAL_STRING("Lorem", token);

	TEST_ASSERT_EQUAL_PTR(&str[6], (token = strtok(NULL, separators)));
	TEST_ASSERT_EQUAL_STRING("ipsum", token);

	TEST_ASSERT_EQUAL_PTR(&str[12], (token = strtok(NULL, separators)));
	TEST_ASSERT_EQUAL_STRING("dolor", token);

	TEST_ASSERT_EQUAL_PTR(&str[18], (token = strtok(NULL, separators)));
	TEST_ASSERT_EQUAL_STRING("sit", token);

	TEST_ASSERT_EQUAL_PTR(&str[22], (token = strtok(NULL, separators)));
	TEST_ASSERT_EQUAL_STRING("amet", token);

	TEST_ASSERT_EQUAL_PTR(&str[28], (token = strtok(NULL, separators)));
	TEST_ASSERT_EQUAL_STRING("consectetur", token);

	TEST_ASSERT_EQUAL_PTR(&str[40], (token = strtok(NULL, separators)));
	TEST_ASSERT_EQUAL_STRING("adipiscing", token);

	TEST_ASSERT_EQUAL_PTR(&str[51], (token = strtok(NULL, separators)));
	TEST_ASSERT_EQUAL_STRING("elit", token);

	TEST_ASSERT_EQUAL_PTR(&str[57], (token = strtok(NULL, separators)));
	TEST_ASSERT_EQUAL_STRING("Maecenas", token);

	TEST_ASSERT_EQUAL_PTR(&str[66], (token = strtok(NULL, separators)));
	TEST_ASSERT_EQUAL_STRING("eleifend", token);

	TEST_ASSERT_EQUAL_PTR(&str[75], (token = strtok(NULL, separators)));
	TEST_ASSERT_EQUAL_STRING("elementum", token);

	TEST_ASSERT_EQUAL_PTR(&str[85], (token = strtok(NULL, separators)));
	TEST_ASSERT_EQUAL_STRING("tellum", token);

	TEST_ASSERT_EQUAL_PTR(NULL, strtok(NULL, separators));
}


/* assert that looking for tokens stops after encountering even one NUL term */
TEST(string_tok, torn)
{
	char str[] = "Lor\0em ipsum";
	const char *separators = " ";
	char *tok;
	TEST_ASSERT_EQUAL_PTR(str, (tok = strtok(str, separators)));
	TEST_ASSERT_EQUAL_STRING("Lor", tok);
	TEST_ASSERT_EQUAL_PTR(NULL, (tok = strtok(NULL, separators)));
}


TEST(string_tok, empty_args)
{
	/* Different scenarios of acquiring null pointer as output*/
	TEST_ASSERT_EQUAL_PTR(NULL, strtok("", "d"));
	TEST_ASSERT_EQUAL_PTR(NULL, strtok(NULL, ""));

	TEST_ASSERT_EQUAL_PTR(NULL, strtok("\0", ""));
	TEST_ASSERT_EQUAL_PTR(NULL, strtok(NULL, ""));

	TEST_ASSERT_EQUAL_STRING("abc", strtok("abc", ""));
}


TEST(string_tok, multi_call)
{
	char loremStr[20] = LOREM_IPSUM;
	char sep[5] = { 0 };
	char multiCallStr[ASCII_LENGTH] = { 0 };
	char *asciiStr = testdata_createCharStr(ASCII_LENGTH);
	int i;

	TEST_ASSERT_NOT_NULL(asciiStr);

	/*
	 * In this case we are checking if it is possible to
	 * pass different stop points in each call to
	 * get desired tokens
	 */

	sprintf(sep, "%s", "ImDr");

	TEST_ASSERT_EQUAL_PTR(loremStr, strtok(loremStr, "I"));
	TEST_ASSERT_EQUAL_PTR(&loremStr[7], strtok(NULL, "m"));
	TEST_ASSERT_EQUAL_PTR(&loremStr[11], strtok(NULL, "D"));
	TEST_ASSERT_EQUAL_PTR(&loremStr[13], strtok(NULL, "r"));
	TEST_ASSERT_EQUAL_PTR(NULL, strtok(NULL, sep));

	/*
	 * strtok() changes the content of a string,
	 * that's why we have to set it once again
	 */
	memcpy(loremStr, LOREM_IPSUM, sizeof(LOREM_IPSUM));

	/*
	 * After passing all delimiters we cannot avoid misplaced tokens because
	 * strtok looks for all elements in "ImDr" and breaks a string
	 * in points where the first occurrence found
	 */
	TEST_ASSERT_EQUAL_PTR(loremStr, strtok(loremStr, sep));
	TEST_ASSERT_EQUAL_PTR(&loremStr[3], strtok(NULL, sep));
	TEST_ASSERT_EQUAL_PTR(&loremStr[5], strtok(NULL, sep));
	TEST_ASSERT_EQUAL_PTR(&loremStr[7], strtok(NULL, sep));
	TEST_ASSERT_EQUAL_PTR(&loremStr[11], strtok(NULL, sep));
	TEST_ASSERT_EQUAL_PTR(&loremStr[13], strtok(NULL, sep));
	TEST_ASSERT_EQUAL_PTR(NULL, strtok(NULL, sep));

	/*
	 * In this case, we tokenize the whole ASCII set using jump-over by
	 * 2 elements. We are forced to jump over 2 places because strtok set
	 *  divider placed as a null term and in this case, we are unable to
	 * cover the whole set. Jumping over 2 places guarantees us output in the form
	 * of one element form ascii set to do an assert with the place where
	 * strtok has saved the stopping place
	 */

	memset(sep, 0, sizeof(sep));

	TEST_ASSERT_EQUAL_PTR(&asciiStr[1], strtok(&asciiStr[1], "\2"));
	for (i = 4; i < ASCII_LENGTH; i += 2) {
		sep[0] = i;
		TEST_ASSERT_EQUAL_PTR(&asciiStr[i - 1], strtok(NULL, &sep[0]));
	}

	/* Checking if set is empty */
	TEST_ASSERT_EQUAL_PTR(NULL, strtok(NULL, &sep[0]));

	/* Creating string with similar pattern */
	for (i = 0; i < sizeof(multiCallStr) - 1; i++) {
		multiCallStr[i] = i % 3 + 1;
	}

	/* Preparing separator with a bunch of same elements*/
	memset(sep, 1, sizeof(sep) - 1);

	/* Initial tokenization */
	TEST_ASSERT_EQUAL_PTR(&multiCallStr[1], strtok(multiCallStr, sep));


	/* Checking if tokenization is in the correct places*/
	for (i = 4; i < sizeof(multiCallStr) - 3; i = i + 3) {
		TEST_ASSERT_EQUAL_PTR(&multiCallStr[i], strtok(NULL, sep));
	}

	memset(multiCallStr, 2, sizeof(multiCallStr));
	memset(&multiCallStr[sizeof(multiCallStr) / 2], 1, 5);
	multiCallStr[sizeof(multiCallStr) - 1] = 0;

	/* Checking ability to tokenize element with multiple same characters treated as a chain of the same element from sep*/
	TEST_ASSERT_EQUAL_PTR(multiCallStr, strtok(multiCallStr, sep));
	TEST_ASSERT_EQUAL_PTR(&multiCallStr[sizeof(multiCallStr) / 2 + 5], strtok(NULL, sep));
	TEST_ASSERT_EQUAL_PTR(NULL, strtok(NULL, sep));


	free(asciiStr);
}


TEST(string_tok, out_of_ascii)
{
	char divider[6] = { 0 };
	char separator[ASCII_LENGTH];
	char *extAsciiStr = create_extAscii_set();
	int i;

	divider[0] = extAsciiStr[0];
	divider[1] = extAsciiStr[64];
	divider[2] = extAsciiStr[128];
	divider[3] = extAsciiStr[192];
	divider[4] = extAsciiStr[254];


	/* Checking ability for tokenize element above standard charset */
	TEST_ASSERT_EQUAL_PTR(&extAsciiStr[1], strtok(extAsciiStr, divider));
	TEST_ASSERT_EQUAL_PTR(&extAsciiStr[65], strtok(NULL, divider));
	TEST_ASSERT_EQUAL_PTR(&extAsciiStr[129], strtok(NULL, divider));
	TEST_ASSERT_EQUAL_PTR(&extAsciiStr[193], strtok(NULL, divider));

	/* 256 is not taken as a token because on element after it is a null term and it is recognized as the end of set */
	TEST_ASSERT_EQUAL_PTR(NULL, strtok(NULL, divider));

	free(extAsciiStr);
	extAsciiStr = create_extAscii_set();
	/*
	 * Checking available tokenization on all elements
	 * from outside of ASCII charset one by one, with
	 * the first separator set as the first element of the set
	 */
	TEST_ASSERT_EQUAL_PTR(extAsciiStr, strtok(extAsciiStr, "\128"));
	for (i = ASCII_LENGTH + 1; i < sizeof(separator); i++) {
		separator[i - ASCII_LENGTH] = i;
		TEST_ASSERT_EQUAL_PTR(&extAsciiStr[i - ASCII_LENGTH], strtok(NULL, &separator[i - ASCII_LENGTH]));
	}

	free(extAsciiStr);
}


TEST(string_tok, big_string)
{
	char str[PATH_MAX];
	char supp[PATH_MAX];

	memset(str, 1, sizeof(str));

	str[sizeof(str) - 2] = 2;
	str[sizeof(str) - 1] = 0;

	/* We must use supp in this place to suppress tokenization on this element*/
	memcpy(supp, str, sizeof(str));

	TEST_ASSERT_EQUAL_PTR(str, strtok(str, "\2"));
	TEST_ASSERT_EQUAL_STRING("\2", strtok("\2", str));
	TEST_ASSERT_EQUAL_PTR(NULL, strtok(str, supp));
}


/*
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/


TEST_SETUP(string_tok_r)
{
}


TEST_TEAR_DOWN(string_tok_r)
{
}


TEST(string_tok_r, basic)
{
	/* Ifdef used because lack of function 'strtok_r' */
#ifdef __phoenix__
	TEST_IGNORE();
#else

	/*
	 * There needed to be two copies of the same element as strtok_r works
	 * on the same space where the variable is settled when even using restrict
	 */
	char str1[] = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Maecenas eleifend elementum tellum.";
	char *restState1 = NULL;
	char *restState2 = NULL;
	char *token;
	char str2[sizeof(str1)];

	const char *separators = ", .";

	memcpy(str2, str1, sizeof(str1));

	TEST_ASSERT_EQUAL_PTR(str1, (token = strtok_r(str1, separators, &restState1)));
	TEST_ASSERT_EQUAL_STRING("Lorem", token);
	TEST_ASSERT_EQUAL_PTR(str2, (token = strtok_r(str2, separators, &restState2)));
	TEST_ASSERT_EQUAL_STRING("Lorem", token);
	TEST_ASSERT_NOT_NULL(restState1);
	TEST_ASSERT_NOT_NULL(restState2);

	TEST_ASSERT_EQUAL_PTR(&str1[6], (token = strtok_r(NULL, separators, &restState1)));
	TEST_ASSERT_EQUAL_STRING("ipsum", token);
	TEST_ASSERT_EQUAL_PTR(&str2[6], (token = strtok_r(NULL, separators, &restState2)));
	TEST_ASSERT_EQUAL_STRING("ipsum", token);
	TEST_ASSERT_NOT_NULL(restState1);
	TEST_ASSERT_NOT_NULL(restState2);

	TEST_ASSERT_EQUAL_PTR(&str1[12], (token = strtok_r(NULL, separators, &restState1)));
	TEST_ASSERT_EQUAL_STRING("dolor", token);
	TEST_ASSERT_EQUAL_PTR(&str2[12], (token = strtok_r(NULL, separators, &restState2)));
	TEST_ASSERT_EQUAL_STRING("dolor", token);
	TEST_ASSERT_NOT_NULL(restState1);
	TEST_ASSERT_NOT_NULL(restState2);

	TEST_ASSERT_EQUAL_PTR(&str1[18], (token = strtok_r(NULL, separators, &restState1)));
	TEST_ASSERT_EQUAL_STRING("sit", token);
	TEST_ASSERT_EQUAL_PTR(&str2[18], (token = strtok_r(NULL, separators, &restState2)));
	TEST_ASSERT_EQUAL_STRING("sit", token);
	TEST_ASSERT_NOT_NULL(restState1);
	TEST_ASSERT_NOT_NULL(restState2);

	TEST_ASSERT_EQUAL_PTR(&str1[22], (token = strtok_r(NULL, separators, &restState1)));
	TEST_ASSERT_EQUAL_STRING("amet", token);
	TEST_ASSERT_EQUAL_PTR(&str2[22], (token = strtok_r(NULL, separators, &restState2)));
	TEST_ASSERT_EQUAL_STRING("amet", token);
	TEST_ASSERT_NOT_NULL(restState1);
	TEST_ASSERT_NOT_NULL(restState2);

	TEST_ASSERT_EQUAL_PTR(&str1[28], (token = strtok_r(NULL, separators, &restState1)));
	TEST_ASSERT_EQUAL_STRING("consectetur", token);
	TEST_ASSERT_EQUAL_PTR(&str2[28], (token = strtok_r(NULL, separators, &restState2)));
	TEST_ASSERT_EQUAL_STRING("consectetur", token);
	TEST_ASSERT_NOT_NULL(restState1);
	TEST_ASSERT_NOT_NULL(restState2);

	TEST_ASSERT_EQUAL_PTR(&str1[40], (token = strtok_r(NULL, separators, &restState1)));
	TEST_ASSERT_EQUAL_STRING("adipiscing", token);
	TEST_ASSERT_EQUAL_PTR(&str2[40], (token = strtok_r(NULL, separators, &restState2)));
	TEST_ASSERT_EQUAL_STRING("adipiscing", token);
	TEST_ASSERT_NOT_NULL(restState1);
	TEST_ASSERT_NOT_NULL(restState2);

	TEST_ASSERT_EQUAL_PTR(&str1[51], (token = strtok_r(NULL, separators, &restState1)));
	TEST_ASSERT_EQUAL_STRING("elit", token);
	TEST_ASSERT_EQUAL_PTR(&str2[51], (token = strtok_r(NULL, separators, &restState2)));
	TEST_ASSERT_EQUAL_STRING("elit", token);
	TEST_ASSERT_NOT_NULL(restState1);
	TEST_ASSERT_NOT_NULL(restState2);

	TEST_ASSERT_EQUAL_PTR(&str1[57], (token = strtok_r(NULL, separators, &restState1)));
	TEST_ASSERT_EQUAL_STRING("Maecenas", token);
	TEST_ASSERT_EQUAL_PTR(&str2[57], (token = strtok_r(NULL, separators, &restState2)));
	TEST_ASSERT_EQUAL_STRING("Maecenas", token);
	TEST_ASSERT_NOT_NULL(restState1);
	TEST_ASSERT_NOT_NULL(restState2);

	TEST_ASSERT_EQUAL_PTR(&str1[66], (token = strtok_r(NULL, separators, &restState1)));
	TEST_ASSERT_EQUAL_STRING("eleifend", token);
	TEST_ASSERT_EQUAL_PTR(&str2[66], (token = strtok_r(NULL, separators, &restState2)));
	TEST_ASSERT_EQUAL_STRING("eleifend", token);
	TEST_ASSERT_NOT_NULL(restState1);
	TEST_ASSERT_NOT_NULL(restState2);

	TEST_ASSERT_EQUAL_PTR(&str1[75], (token = strtok_r(NULL, separators, &restState1)));
	TEST_ASSERT_EQUAL_STRING("elementum", token);
	TEST_ASSERT_EQUAL_PTR(&str2[75], (token = strtok_r(NULL, separators, &restState2)));
	TEST_ASSERT_EQUAL_STRING("elementum", token);
	TEST_ASSERT_NOT_NULL(restState1);
	TEST_ASSERT_NOT_NULL(restState2);

	TEST_ASSERT_EQUAL_PTR(&str1[85], (token = strtok_r(NULL, separators, &restState1)));
	TEST_ASSERT_EQUAL_STRING("tellum", token);
	TEST_ASSERT_EQUAL_PTR(&str2[85], (token = strtok_r(NULL, separators, &restState2)));
	TEST_ASSERT_EQUAL_STRING("tellum", token);
	TEST_ASSERT_NOT_NULL(restState1);
	TEST_ASSERT_NOT_NULL(restState2);

	TEST_ASSERT_EQUAL_PTR(NULL, strtok_r(NULL, separators, &restState1));
	TEST_ASSERT_EQUAL_PTR(NULL, strtok_r(NULL, separators, &restState2));
	TEST_ASSERT_NOT_NULL(restState1);
	TEST_ASSERT_NOT_NULL(restState2);
	TEST_ASSERT_EMPTY(restState1);
	TEST_ASSERT_EMPTY(restState2);

#endif
}


TEST(string_tok_r, torn)
{
	/* Ifdef used because lack of function 'strtok_r' */
#ifdef __phoenix__
	TEST_IGNORE();
#else
	char *rest;
	char *tok;
	char str[] = "Lor\0em ipsum";

	const char *separators = " ";

	TEST_ASSERT_EQUAL_PTR(str, (tok = strtok_r(str, separators, &rest)));
	TEST_ASSERT_EQUAL_STRING("Lor", tok);
	TEST_ASSERT_NOT_NULL(rest);
	TEST_ASSERT_EQUAL_PTR(NULL, (tok = strtok_r(NULL, separators, &rest)));
	TEST_ASSERT_NOT_NULL(rest);

#endif
}


TEST(string_tok_r, empty_args)
{
	/* Ifdef used because lack of function 'strtok_r' */
#ifdef __phoenix__
	TEST_IGNORE();
#else
	char empty[] = { 0 };
	char *rest;

	/* Different scenarios of acquiring null pointer as output*/
	TEST_ASSERT_EQUAL_PTR(NULL, strtok_r("", "d", &rest));
	TEST_ASSERT_NOT_NULL(rest);
	TEST_ASSERT_EQUAL_PTR(NULL, strtok_r(NULL, "", &rest));
	TEST_ASSERT_NOT_NULL(rest);

	TEST_ASSERT_EQUAL_PTR(NULL, strtok_r(empty, "", &rest));
	TEST_ASSERT_NOT_NULL(rest);
	TEST_ASSERT_EQUAL_PTR(NULL, strtok_r(NULL, "", &rest));
	TEST_ASSERT_NOT_NULL(rest);

#endif
}


TEST(string_tok_r, multi_call)
{
	/* Ifdef used because lack of function 'strtok_r' */
#ifdef __phoenix__
	TEST_IGNORE();
#else


	char loremStr[20] = LOREM_IPSUM;
	char sep[5] = { 0 };
	char multiCallStr[ASCII_LENGTH] = { 0 };
	char *asciiStr = testdata_createCharStr(ASCII_LENGTH);
	char *rest;
	int i;

	TEST_ASSERT_NOT_NULL(asciiStr);

	/*
	 * In this case we are checking if it is possible to
	 * pass different stop points in each call to
	 * get desired tokens
	 */

	sprintf(sep, "%s", "ImDr");

	TEST_ASSERT_EQUAL_PTR(loremStr, strtok_r(loremStr, "I", &rest));
	TEST_ASSERT_NOT_NULL(rest);
	TEST_ASSERT_EQUAL_PTR(&loremStr[7], strtok_r(NULL, "m", &rest));
	TEST_ASSERT_NOT_NULL(rest);
	TEST_ASSERT_EQUAL_PTR(&loremStr[11], strtok_r(NULL, "D", &rest));
	TEST_ASSERT_NOT_NULL(rest);
	TEST_ASSERT_EQUAL_PTR(&loremStr[13], strtok_r(NULL, "r", &rest));
	TEST_ASSERT_NOT_NULL(rest);
	TEST_ASSERT_EQUAL_PTR(NULL, strtok_r(NULL, sep, &rest));
	TEST_ASSERT_NOT_NULL(rest);

	/*
	 * strtok() changes the content of a string,
	 * that's why we have to set it once again
	 */
	memcpy(loremStr, LOREM_IPSUM, sizeof(LOREM_IPSUM));

	/*
	 * After passing all delimiters we cannot avoid misplaced tokens because
	 * strtok looks for all elements in "ImDr" and breaks a string
	 * in points where the first occurrence of it is found
	 */
	TEST_ASSERT_EQUAL_PTR(loremStr, strtok_r(loremStr, sep, &rest));
	TEST_ASSERT_NOT_NULL(rest);
	TEST_ASSERT_EQUAL_PTR(&loremStr[3], strtok_r(NULL, sep, &rest));
	TEST_ASSERT_NOT_NULL(rest);
	TEST_ASSERT_EQUAL_PTR(&loremStr[5], strtok_r(NULL, sep, &rest));
	TEST_ASSERT_NOT_NULL(rest);
	TEST_ASSERT_EQUAL_PTR(&loremStr[7], strtok_r(NULL, sep, &rest));
	TEST_ASSERT_NOT_NULL(rest);
	TEST_ASSERT_EQUAL_PTR(&loremStr[11], strtok_r(NULL, sep, &rest));
	TEST_ASSERT_NOT_NULL(rest);
	TEST_ASSERT_EQUAL_PTR(&loremStr[13], strtok_r(NULL, sep, &rest));
	TEST_ASSERT_NOT_NULL(rest);
	TEST_ASSERT_EQUAL_PTR(NULL, strtok_r(NULL, sep, &rest));
	TEST_ASSERT_NOT_NULL(rest);


	/*
	 * In this case, we tokenize the whole ASCII set using jump-over by
	 * 2 elements. We are forced to jump over 2 places because strtok set
	 *  divider placed as a null term and in this case, we are unable to
	 * cover the whole set. Jumping over 2 places guarantees us output in the form
	 * of one element form ascii set to do an assert with the place where
	 * strtok has saved the stopping place
	 */

	memset(sep, 0, sizeof(sep));

	TEST_ASSERT_EQUAL_PTR(&asciiStr[1], strtok_r(&asciiStr[1], "\2", &rest));
	for (i = 4; i < ASCII_LENGTH; i += 2) {
		sep[0] = i;
		TEST_ASSERT_EQUAL_PTR(&asciiStr[i - 1], strtok_r(NULL, &sep[0], &rest));
		if (i >= ASCII_LENGTH - 2) {
			TEST_ASSERT_NOT_NULL(rest);
			TEST_ASSERT_EMPTY(rest);
		}
		else {
			TEST_ASSERT_NOT_NULL(rest);
			TEST_ASSERT_NOT_EMPTY(rest);
		}
	}

	/* Checking if set is empty */
	TEST_ASSERT_EQUAL_PTR(NULL, strtok_r(NULL, &sep[0], &rest));

	/* Creating string with similar pattern */
	for (i = 0; i < sizeof(multiCallStr) - 1; i++) {
		multiCallStr[i] = i % 3 + 1;
	}

	// /* Preparing separator with a bunch of similar elements*/
	memset(sep, 1, sizeof(sep) - 1);

	/* Initial tokenization */
	TEST_ASSERT_EQUAL_PTR(&multiCallStr[1], strtok_r(multiCallStr, sep, &rest));


	/* Checking if tokenization is in the correct places*/
	for (i = 4; i < sizeof(multiCallStr) - 3; i = i + 3) {
		TEST_ASSERT_EQUAL_PTR(&multiCallStr[i], strtok_r(NULL, sep, &rest));
	}

	memset(multiCallStr, 2, sizeof(multiCallStr));
	memset(&multiCallStr[sizeof(multiCallStr) / 2], 1, 5);
	multiCallStr[sizeof(multiCallStr) - 1] = 0;

	/* Checking ability to tokenize element with multiple same signs treated as a chain of the same element from sep*/
	TEST_ASSERT_EQUAL_PTR(multiCallStr, strtok_r(multiCallStr, sep, &rest));
	TEST_ASSERT_EQUAL_PTR(&multiCallStr[sizeof(multiCallStr) / 2 + 5], strtok_r(NULL, sep, &rest));
	TEST_ASSERT_EQUAL_PTR(NULL, strtok_r(NULL, sep, &rest));


	free(asciiStr);
#endif
}


TEST(string_tok_r, out_of_ascii)
{
	/* Ifdef used because lack of function 'strtok_r' */
#ifdef __phoenix__
	TEST_IGNORE();
#else

	char divider[6] = { 0 };
	char separator[ASCII_LENGTH];
	char *rest;
	char *extAsciiStr = create_extAscii_set();

	int i;

	divider[0] = extAsciiStr[0];
	divider[1] = extAsciiStr[64];
	divider[2] = extAsciiStr[128];
	divider[3] = extAsciiStr[192];
	divider[4] = extAsciiStr[254];

	/* Checking ability for tokenize element above standard charset */
	TEST_ASSERT_EQUAL_PTR(&extAsciiStr[1], strtok_r(extAsciiStr, divider, &rest));
	TEST_ASSERT_NOT_NULL(rest);
	TEST_ASSERT_EQUAL_PTR(&extAsciiStr[65], strtok_r(NULL, divider, &rest));
	TEST_ASSERT_NOT_NULL(rest);
	TEST_ASSERT_EQUAL_PTR(&extAsciiStr[129], strtok_r(NULL, divider, &rest));
	TEST_ASSERT_NOT_NULL(rest);
	TEST_ASSERT_EQUAL_PTR(&extAsciiStr[193], strtok_r(NULL, divider, &rest));
	TEST_ASSERT_NOT_NULL(rest);

	/* 256 is not taken as a token because on element after it is a null term and it is recognized as the end of set */
	TEST_ASSERT_EQUAL_PTR(NULL, strtok_r(NULL, divider, &rest));
	TEST_ASSERT_NOT_NULL(rest);
	TEST_ASSERT_EMPTY(rest);

	free(extAsciiStr);
	extAsciiStr = create_extAscii_set();
	/*
	 * Checking available tokenization on all elements
	 * from outside of ASCII charset one by one, with
	 * the first separator set as the first element of the set
	 */
	TEST_ASSERT_EQUAL_PTR(extAsciiStr, strtok_r(extAsciiStr, "\128", &rest));
	for (i = ASCII_LENGTH + 1; i < sizeof(separator); i++) {
		separator[i - ASCII_LENGTH] = i;
		TEST_ASSERT_EQUAL_PTR(&extAsciiStr[i - ASCII_LENGTH], strtok_r(NULL, &separator[i - ASCII_LENGTH], &rest));
		TEST_ASSERT_NOT_NULL(rest);
	}

	free(extAsciiStr);
#endif
}


TEST(string_tok_r, same_state)
{
	/* Ifdef used because lack of function 'strtok_r' */
#ifdef __phoenix__
	TEST_IGNORE();
#else

	char *rest[ASCII_LENGTH] = { 0 },
		 *asciiStr = testdata_createCharStr(ASCII_LENGTH);
	int i;

	TEST_ASSERT_NOT_NULL(asciiStr);

	for (i = 0; i < ASCII_LENGTH - 1; i++) {
		TEST_ASSERT_EQUAL_PTR(&asciiStr[1], strtok_r(&asciiStr[1], &asciiStr[i + 2], &rest[i]));

		TEST_ASSERT_NOT_NULL(rest[i]);
		/*We must avoid the first element to fill up the element behind for assertions*/
		if (i > 1) {
			TEST_ASSERT_EQUAL_HEX64(rest[i], rest[1]);
		}
	}

	free(asciiStr);
#endif
}

TEST(string_tok_r, big_string)
{
	/* Ifdef used because lack of function 'strtok_r' */
#ifdef __phoenix__
	TEST_IGNORE();
#else

	char str[PATH_MAX];
	char str1[PATH_MAX];
	char *rest[2] = { 0 };
	{
		memset(str, 1, sizeof(str));
		memset(str1, 1, sizeof(str1));

		str[sizeof(str) - 2] = 2;
		str[sizeof(str) - 1] = 0;
		str1[sizeof(str) - 2] = 2;
		str1[sizeof(str) - 1] = 0;

		TEST_ASSERT_EQUAL_PTR(str, strtok_r(str, "\2", &rest[0]));
		TEST_ASSERT_EQUAL_STRING("\2", strtok_r("\2", str, &rest[0]));
		TEST_ASSERT_EQUAL_PTR(NULL, strtok_r(str, str1, &rest[0]));

		TEST_ASSERT_EQUAL_PTR(str1, strtok_r(str1, "\2", &rest[1]));
		TEST_ASSERT_EQUAL_STRING("\2", strtok_r("\2", str, &rest[1]));
		TEST_ASSERT_EQUAL_PTR(NULL, strtok_r(str1, str, &rest[1]));
	}
#endif
}


/*
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/


TEST_SETUP(string_str)
{
}


TEST_TEAR_DOWN(string_str)
{
}


TEST(string_str, basic)
{
	const char loremIpsum[BUFFSIZE] = LOREM_IPSUM;
	char *asciiStr = testdata_createCharStr(ASCII_LENGTH);

	TEST_ASSERT_NOT_NULL(asciiStr);

	/* Standard use of strstr on arrays */
	TEST_ASSERT_EQUAL_PTR(loremIpsum, strstr(loremIpsum, "Lorem"));
	TEST_ASSERT_EQUAL_PTR(&loremIpsum[6], strstr(loremIpsum, "Ipsum"));
	TEST_ASSERT_EQUAL_PTR(&loremIpsum[12], strstr(loremIpsum, "Dolor"));

	/* Simple usage of strstr with ascii set */
	TEST_ASSERT_EQUAL_PTR(asciiStr, strstr(asciiStr, asciiStr));
	TEST_ASSERT_EQUAL_PTR(&asciiStr[ASCII_LENGTH / 4], strstr(asciiStr, &asciiStr[ASCII_LENGTH / 4]));
	TEST_ASSERT_EQUAL_PTR(&asciiStr[ASCII_LENGTH / 3], strstr(asciiStr, &asciiStr[ASCII_LENGTH / 3]));
	TEST_ASSERT_EQUAL_PTR(&asciiStr[ASCII_LENGTH / 2], strstr(asciiStr, &asciiStr[ASCII_LENGTH / 2]));


	free(asciiStr);
}


TEST(string_str, empty_args)
{
	const char str[] = "Lorem";

	char *asciiStr = testdata_createCharStr(ASCII_LENGTH);

	TEST_ASSERT_NOT_NULL(asciiStr);

	/* Different scenarios of using empty input or output*/
	TEST_ASSERT_EQUAL_PTR(NULL, strstr("", str));
	TEST_ASSERT_EQUAL_STRING("", strstr("", ""));
	TEST_ASSERT_EQUAL_PTR(str, strstr(str, ""));

	/* Passing an empty string as the second parameter to get first */
	TEST_ASSERT_EQUAL_PTR(asciiStr, strstr(asciiStr, ""));


	free(asciiStr);
}


TEST(string_str, out_of_ascii)
{
	int i;

	char *extAsciiStr = create_extAscii_set();

	for (i = 1; i < sizeof(extAsciiStr); i++) {
		TEST_ASSERT_EQUAL_PTR(&extAsciiStr[i], strstr(extAsciiStr, &extAsciiStr[i]));
	}

	free(extAsciiStr);
}


TEST(string_str, strstr_order)
{
	int i;
	char reversStr[ASCII_LENGTH];
	char *testStr;

	testStr = testdata_createCharStr(ASCII_LENGTH);

	TEST_ASSERT_NOT_NULL(reversStr);
	TEST_ASSERT_NOT_NULL(testStr);

	for (i = 0; i < ASCII_LENGTH - 1; i++) {
		reversStr[i] = testStr[ASCII_LENGTH - i - 2];
	}
	reversStr[ASCII_LENGTH - 1] = 0;

	TEST_ASSERT_EQUAL_STRING(NULL, strstr("abc", "cba"));
	TEST_ASSERT_EQUAL_STRING(NULL, strstr("abc", "bac"));
	TEST_ASSERT_EQUAL_STRING(NULL, strstr("abc", "bca"));

	/* Asserting reversed string */
	TEST_ASSERT_EQUAL_PTR(NULL, strstr(testStr, reversStr));

	free(testStr);
}


TEST(string_str, part_of_str)
{
	TEST_ASSERT_EQUAL_STRING(NULL, strstr("abc", "aa"));
	TEST_ASSERT_EQUAL_STRING("abc", strstr("abc", "ab"));
	TEST_ASSERT_EQUAL_STRING("bc", strstr("abc", "bc"));
	TEST_ASSERT_EQUAL_STRING(NULL, strstr("abc", "ac"));
	TEST_ASSERT_EQUAL_STRING(NULL, strstr("abc", "xbc"));
	TEST_ASSERT_EQUAL_STRING(NULL, strstr("abc", "bcx"));
	TEST_ASSERT_EQUAL_STRING(NULL, strstr("abc", "xab"));
}


TEST(string_str, big_string)
{
	char str[PATH_MAX];
	memset(str, 1, sizeof(str));

	str[sizeof(str) - 2] = 2;
	str[sizeof(str) - 1] = 0;
	TEST_ASSERT_EQUAL_PTR(&str[sizeof(str) - 2], strstr(str, "\2"));
	TEST_ASSERT_EQUAL_STRING(NULL, strstr("\2", str));
	TEST_ASSERT_EQUAL_PTR(str, strstr(str, str));
}


/*
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/


TEST_SETUP(string_pbrk)
{
}


TEST_TEAR_DOWN(string_pbrk)
{
}


TEST(string_pbrk, basic)
{
	const char loremIpsum[BUFFSIZE] = LOREM_IPSUM;
	char *asciiStr;

	asciiStr = testdata_createCharStr(ASCII_LENGTH);

	TEST_ASSERT_NOT_NULL(asciiStr);

	TEST_ASSERT_EQUAL_PTR(loremIpsum, strpbrk(loremIpsum, "Lorem"));

	/*
	 * strpbrk is sensitive to all elements in the array where
	 * is looking up which means 'm' is the first element of array 'loremIpsum' that strpbrk found
	 */
	TEST_ASSERT_EQUAL_PTR(&loremIpsum[4], strpbrk(loremIpsum, "Ipsum"));

	/* Like in the case before strpbrk found 'o' at second place in the array and his output point at second place in the array*/
	TEST_ASSERT_EQUAL_PTR(&loremIpsum[1], strpbrk(loremIpsum, "Dolor"));

	/* Simple usage of strpbrk with ascii set */
	TEST_ASSERT_EQUAL_PTR(asciiStr, strpbrk(asciiStr, asciiStr));
	TEST_ASSERT_EQUAL_PTR(&asciiStr[ASCII_LENGTH / 4], strpbrk(asciiStr, &asciiStr[ASCII_LENGTH / 4]));
	TEST_ASSERT_EQUAL_PTR(&asciiStr[ASCII_LENGTH / 3], strpbrk(asciiStr, &asciiStr[ASCII_LENGTH / 3]));
	TEST_ASSERT_EQUAL_PTR(&asciiStr[ASCII_LENGTH / 2], strpbrk(asciiStr, &asciiStr[ASCII_LENGTH / 2]));

	free(asciiStr);
}


TEST(string_pbrk, empty_args)
{
	/* Different scenarios to acquire null pointer as output*/
	TEST_ASSERT_NULL(strpbrk("", "abc"));
	TEST_ASSERT_NULL(strpbrk("", ""));
	TEST_ASSERT_NULL(strpbrk("abc", ""));
}


TEST(string_pbrk, out_of_ascii)
{
	int i;

	char *extAsciiStr = create_extAscii_set();

	/* Checking the ability of strpbrk to use chars from 0 to 255 as elements of arrays in search*/
	for (i = 0; i < sizeof(extAsciiStr) - 1; i++) {
		TEST_ASSERT_EQUAL_PTR(&extAsciiStr[i], strpbrk(extAsciiStr, &extAsciiStr[i]));
	}

	free(extAsciiStr);
}


TEST(string_pbrk, strpbrk_order)
{
	int i;
	char reversStr[ASCII_LENGTH];
	char *testStr;
	char str[] = "abc";

	testStr = testdata_createCharStr(ASCII_LENGTH);

	TEST_ASSERT_NOT_NULL(testStr);

	for (i = 0; i < ASCII_LENGTH - 1; i++) {
		reversStr[i] = testStr[ASCII_LENGTH - i - 2];
	}
	reversStr[ASCII_LENGTH - 1] = 0;

	TEST_ASSERT_EQUAL_STRING(str, strpbrk("abc", "cba"));
	TEST_ASSERT_EQUAL_STRING(str, strpbrk("abc", "bac"));
	TEST_ASSERT_EQUAL_STRING(str, strpbrk("abc", "bca"));

	/* Asserting reversed string */
	for (i = 0; i < ASCII_LENGTH - 1; i++) {
		TEST_ASSERT_EQUAL_PTR(testStr, strpbrk(testStr, &reversStr[i]));
	}

	free(testStr);
}


TEST(string_pbrk, part_of_str)
{
	char str[] = "abc";

	TEST_ASSERT_EQUAL_STRING(str, strpbrk("abc", "aa"));
	TEST_ASSERT_EQUAL_STRING(&str[1], strpbrk("abc", "bb"));
	TEST_ASSERT_EQUAL_STRING(&str[2], strpbrk("abc", "cc"));
	TEST_ASSERT_EQUAL_STRING(str, strpbrk("abc", "ab"));
	TEST_ASSERT_EQUAL_STRING(&str[1], strpbrk("abc", "bc"));
	TEST_ASSERT_EQUAL_STRING(str, strpbrk("abc", "ac"));
}


TEST(string_pbrk, multiple)
{
	char str[] = "abc";

	TEST_ASSERT_EQUAL_STRING(str, strpbrk("abc", "aabbcc"));
	TEST_ASSERT_EQUAL_STRING(str, strpbrk("abc", "ccaabb"));
	TEST_ASSERT_EQUAL_STRING(str, strpbrk("abc", "bbaacc"));
	TEST_ASSERT_EQUAL_STRING(str, strpbrk("abc", "aaaaaa"));
	TEST_ASSERT_EQUAL_STRING(&str[1], strpbrk("abc", "bbbbbb"));
	TEST_ASSERT_EQUAL_STRING(&str[2], strpbrk("abc", "cccccc"));
}


TEST(string_pbrk, additional_bytes)
{
	char str[] = "abc";

	TEST_ASSERT_EQUAL_STRING(str, strpbrk("abc", "xa"));
	TEST_ASSERT_EQUAL_STRING(&str[1], strpbrk("abc", "xb"));
	TEST_ASSERT_EQUAL_STRING(&str[2], strpbrk("abc", "xc"));
	TEST_ASSERT_EQUAL_STRING(str, strpbrk("abc", "xxxca"));
	TEST_ASSERT_EQUAL_STRING(&str[1], strpbrk("abc", "xxxcb"));
	TEST_ASSERT_EQUAL_STRING(str, strpbrk("abc", "xxxac"));
	TEST_ASSERT_EQUAL_STRING(NULL, strpbrk("abcdefg", "hij"));
	TEST_ASSERT_EQUAL_STRING("cdefg", strpbrk("cdefg", "abc"));
	TEST_ASSERT_EQUAL_STRING("efg", strpbrk("abefg", "cde"));
}


TEST(string_pbrk, not_present)
{
	int i;
	char *asciiStr = testdata_createCharStr(ASCII_LENGTH + 1);
	char holder[2] = { 0 };

	TEST_ASSERT_NOT_NULL(asciiStr);

	for (i = 1; i < ASCII_LENGTH; i++) {
		holder[0] = i;

		if (i == 1)
			asciiStr[i] = asciiStr[i + 1];
		else
			asciiStr[i] = asciiStr[i - 1];

		/* asciiStr start with double 1 that's why we jump over one place*/
		TEST_ASSERT_EQUAL_PTR(NULL, strpbrk(holder, &asciiStr[1]));
		asciiStr[i] = i;
	}

	free(asciiStr);
}


TEST(string_pbrk, big_string)
{
	char str[PATH_MAX];
	memset(str, 1, sizeof(str));

	str[sizeof(str) - 2] = 2;
	str[sizeof(str) - 1] = 0;
	TEST_ASSERT_EQUAL_PTR(&str[PATH_MAX - 2], strpbrk(str, "\2"));
	TEST_ASSERT_EQUAL_STRING("\2", strpbrk("\2", str));
	TEST_ASSERT_EQUAL_PTR(str, strpbrk(str, str));
}


/*
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/


TEST_GROUP_RUNNER(string_tok)
{
	RUN_TEST_CASE(string_tok, basic);
	RUN_TEST_CASE(string_tok, torn);
	RUN_TEST_CASE(string_tok, empty_args);
	RUN_TEST_CASE(string_tok, multi_call);
	RUN_TEST_CASE(string_tok, out_of_ascii);
	RUN_TEST_CASE(string_tok, big_string);
}


TEST_GROUP_RUNNER(string_tok_r)
{
	RUN_TEST_CASE(string_tok_r, basic);
	RUN_TEST_CASE(string_tok_r, torn);
	RUN_TEST_CASE(string_tok_r, empty_args);
	RUN_TEST_CASE(string_tok_r, multi_call);
	RUN_TEST_CASE(string_tok_r, out_of_ascii);
	RUN_TEST_CASE(string_tok_r, same_state);
	RUN_TEST_CASE(string_tok_r, big_string);
}


TEST_GROUP_RUNNER(string_str)
{
	RUN_TEST_CASE(string_str, basic);
	RUN_TEST_CASE(string_str, empty_args);
	RUN_TEST_CASE(string_str, out_of_ascii);
	RUN_TEST_CASE(string_str, strstr_order);
	RUN_TEST_CASE(string_str, part_of_str);
	RUN_TEST_CASE(string_str, big_string);
}


TEST_GROUP_RUNNER(string_pbrk)
{
	RUN_TEST_CASE(string_pbrk, basic);
	RUN_TEST_CASE(string_pbrk, empty_args);
	RUN_TEST_CASE(string_pbrk, out_of_ascii);
	RUN_TEST_CASE(string_pbrk, strpbrk_order);
	RUN_TEST_CASE(string_pbrk, part_of_str);
	RUN_TEST_CASE(string_pbrk, additional_bytes);
	RUN_TEST_CASE(string_pbrk, not_present);
	RUN_TEST_CASE(string_pbrk, multiple);
	RUN_TEST_CASE(string_pbrk, big_string);
}
