/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - string.h
 * TESTED:
 *    - memcpy()
 *    - memccpy()
 *    - strncpy()
 *    - stpncpy()
 *    - strcpy()
 *    - stpcpy()
 *    - strlcpy()
 *    - strlcat()
 *
 * Copyright 2023 Phoenix Systems
 * Author: Mateusz Niewiadomski, Damian Modzelewski
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

/* defines lengths of buffers holding strings*/
#define MAX_STR_LEN 24
/* {0..255} -> 256 elements */
#define CHARS_SET_SIZE (UCHAR_MAX + 1)
#define BIG_NUMB       1024

/* defines for string and mem copy */
#define TEST_STR1 "Lorem ipsum dolor"
#define TEST_STR2 "Maecenas id commodo"

/* defines for string_strlcpy */
#define STR_SRC  "abcd"
#define STR_DEST "xxxx"

/* defines for string_strlcat */
#define STR_SRC1        "abc"
#define STR_SRC2        "defgh"
#define STR_PLACEHOLDER "klmnopqrstu"


/*
//////////////////////////////////////////////////////////////////////////////////////////////
 */


TEST_GROUP(string_memcpy);
TEST_GROUP(string_memccpy);
TEST_GROUP(string_strncpy);
TEST_GROUP(string_stpncpy);
TEST_GROUP(string_strcpy_stpcpy);
TEST_GROUP(string_strlcpy);
TEST_GROUP(string_strlcat);

/*
//////////////////////////////////////////////////////////////////////////////////////////////
 */


TEST_SETUP(string_memcpy)
{
}


TEST_TEAR_DOWN(string_memcpy)
{
}


TEST(string_memcpy, basic)
{
	char strDest[MAX_STR_LEN] = { 0 },
		 strPlaceholder[MAX_STR_LEN] = TEST_STR2;
	int i;

	TEST_ASSERT_EQUAL_PTR(strDest, memcpy(strDest, TEST_STR1, sizeof(TEST_STR1)));
	TEST_ASSERT_EQUAL_STRING(TEST_STR1, strDest);

	/* Checking if we can overwrite array and doesn't overwrite elements after end of input*/
	for (i = sizeof(TEST_STR1) - 1; i < sizeof(strDest); i++) {
		TEST_ASSERT_EQUAL_CHAR('\0', strDest[i]);
	}

	/* Checking if we can overwrite data using memcpy*/
	TEST_ASSERT_EQUAL_PTR(strPlaceholder, memcpy(strPlaceholder, TEST_STR1, sizeof(TEST_STR1)));
	TEST_ASSERT_EQUAL_STRING(TEST_STR1, strDest);
	TEST_ASSERT_EQUAL_STRING(strPlaceholder, TEST_STR1);
}


TEST(string_memcpy, data_types)
{
	int num = 12345678,
		numDest = 0;
	float flt = 2.32252,
		  fltDest = 0;
	ptrdiff_t ptr = 0x2345,
			  ptrDest = 0;
	size_t size = SIZE_MAX,
		   sizDest = 0;

	/* Checking ability to copy numbers between places */
	TEST_ASSERT_EQUAL_PTR(&numDest, memcpy(&numDest, &num, sizeof(num)));
	TEST_ASSERT_NOT_EMPTY(&numDest);
	TEST_ASSERT_EQUAL_INT(num, numDest);

	TEST_ASSERT_EQUAL_PTR(&fltDest, memcpy(&fltDest, &flt, sizeof(flt)));
	TEST_ASSERT_NOT_EMPTY(&fltDest);
	TEST_ASSERT_FLOAT_IS_DETERMINATE(fltDest);
	TEST_ASSERT_EQUAL_FLOAT(flt, fltDest);

	TEST_ASSERT_EQUAL_PTR(&ptrDest, memcpy(&ptrDest, &ptr, sizeof(ptr)));
	TEST_ASSERT_NOT_EMPTY(&ptrDest);
	TEST_ASSERT_EQUAL_INT(ptr, ptrDest);

	TEST_ASSERT_EQUAL_PTR(&sizDest, memcpy(&sizDest, &size, sizeof(size)));
	TEST_ASSERT_NOT_EMPTY(&sizDest);
	TEST_ASSERT_EQUAL_DOUBLE(size, sizDest);
}


TEST(string_memcpy, adjacent)
{
	char testStr[] = "TEST",
		 memStr[MAX_STR_LEN] = "\0\0\0\0\0\0\0\0\0\0TEST",
		 expVal[MAX_STR_LEN] = "\0\0\0\0\0\0TESTTESTTEST",
		 zeroStr[MAX_STR_LEN] = { 0 },
		 testStrLen = sizeof(testStr) - 1;

	/*
	 * Copy the string in his memory space right before his original place to get the effect
	 * like using strcat but in the opposite direction
	 */
	TEST_ASSERT_EQUAL_PTR(&memStr[10 - testStrLen], memcpy(&memStr[10 - testStrLen], &memStr[10], testStrLen));
	TEST_ASSERT_EQUAL_CHAR_ARRAY(&memStr[10 - testStrLen], &expVal[10 - testStrLen], testStrLen * 2);
	/* Checking if zeros before and after text are intact */
	TEST_ASSERT_EQUAL_CHAR_ARRAY(memStr, zeroStr, 6);
	TEST_ASSERT_EQUAL_CHAR_ARRAY(&memStr[10 + testStrLen], zeroStr, 10);

	/* Copy text in a similar way how do it strcat*/
	TEST_ASSERT_EQUAL_PTR(&memStr[10 + testStrLen], memcpy(&memStr[10 + testStrLen], &memStr[10], testStrLen));
	/* Checking if zeros before and after text are intact and text was copied correctly */
	TEST_ASSERT_EQUAL_CHAR_ARRAY(memStr, expVal, sizeof(memStr));
}


TEST(string_memcpy, one_byte)
{
	char testArray[CHARS_SET_SIZE] = { 0 },
		 input[CHARS_SET_SIZE] = { 0 };
	int i;

	/* This loop will copy only one byte in a place where the loop iterator pointing to this place */
	for (i = 0; i < CHARS_SET_SIZE; i++) {
		input[i] = i;
		TEST_ASSERT_EQUAL_PTR(&testArray[i], memcpy(&testArray[i], &input[i], 1));
	}

	/* Checking if all elements were correctly copied */
	TEST_ASSERT_EQUAL_CHAR_ARRAY(input, testArray, CHARS_SET_SIZE);
}


TEST(string_memcpy, clearing_array)
{
	char testArray[MAX_STR_LEN] = TEST_STR1,
		 input[MAX_STR_LEN] = { 0 };

	/* Clearing array with support array filled with 0 using memcpy to do this*/
	TEST_ASSERT_EQUAL_PTR(testArray, memcpy(testArray, input, MAX_STR_LEN));
	TEST_ASSERT_EQUAL_CHAR_ARRAY(input, testArray, MAX_STR_LEN);
}


TEST(string_memcpy, various_sizes)
{

	char testArray[MAX_STR_LEN] = TEST_STR1,
		 input[MAX_STR_LEN] = TEST_STR2;

	/* Trying to copy zero bytes */
	TEST_ASSERT_EQUAL_PTR(testArray, memcpy(testArray, input, 0));
	TEST_ASSERT_EQUAL_STRING(testArray, TEST_STR1);

	/* Using sizes to copy only part of the array to another */
	TEST_ASSERT_EQUAL_PTR(testArray, memcpy(testArray, input, sizeof(input) / 2));
	/* Checking if a copy was executed only on half of the array size */
	TEST_ASSERT_EQUAL_CHAR_ARRAY(testArray, input, sizeof(testArray) / 2);
	TEST_ASSERT_EQUAL_CHAR_ARRAY(&testArray[sizeof(testArray) / 2], &TEST_STR1[sizeof(testArray) / 2], strlen(&TEST_STR1[sizeof(testArray) / 2]));
}


TEST(string_memcpy, big)
{
	char buff[BIG_NUMB] = { 0 };
	char *longStr;

	size_t longStrSize = sizeof(buff);

	/* Checking capability of handling big blocks of data */
	longStr = testdata_createCharStr(longStrSize);
	TEST_ASSERT_NOT_NULL(longStr);

	TEST_ASSERT_EQUAL_PTR(buff, memcpy(buff, longStr, longStrSize));
	TEST_ASSERT_EQUAL_CHAR_ARRAY(buff, longStr, longStrSize);

	free((void *)longStr);
}


/*
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
 */


TEST_SETUP(string_memccpy)
{
}


TEST_TEAR_DOWN(string_memccpy)
{
}


TEST(string_memccpy, basic)
{
	/* Ifdef used because lack of function 'memccpy' */
#ifdef __phoenix__
	TEST_IGNORE();
#endif
#ifndef __phoenix__


	char strDest[MAX_STR_LEN] = { 0 };
	int i;

	TEST_ASSERT_EQUAL_PTR(NULL, memccpy(strDest, TEST_STR1, 'x', sizeof(TEST_STR1)));
	TEST_ASSERT_EQUAL_STRING(TEST_STR1, strDest);

	/* Checking if we don't overwrite elements after the end of input*/
	for (i = sizeof(TEST_STR1); i < sizeof(strDest); i++) {
		TEST_ASSERT_EQUAL_CHAR('\0', strDest[i]);
	}

	memset(strDest, 0, sizeof(strDest));

	/* Copy only half of string str2 and in addition we search for the letter 's' on place 8 where the string contains 20 elements*/
	TEST_ASSERT_EQUAL_PTR(&strDest[8], memccpy(strDest, TEST_STR2, 's', sizeof(TEST_STR2)));
	TEST_ASSERT_EQUAL_CHAR_ARRAY(strDest, TEST_STR2, 8);

	/* Checking if we don't overwrite elements after the end of input*/
	for (i = strlen(strDest); i < sizeof(strDest); i++) {
		TEST_ASSERT_EQUAL_CHAR('\0', strDest[i]);
	}

#endif
}


TEST(string_memccpy, stop_char_found)
{
#ifdef __phoenix__
	TEST_IGNORE();
#endif
#ifndef __phoenix__

	char bigStrDest[ALL_CHARS_STRING_SIZE] = { 0 };
	const char *testStr;
	int i;

	testStr = testdata_createCharStr(ALL_CHARS_STRING_SIZE);

	TEST_ASSERT_NOT_NULL(testStr);

	/* 1 skipped, because of double one at the beginning of testStr */
	for (i = 2; i < ALL_CHARS_STRING_SIZE; i++) {
		TEST_ASSERT_EQUAL_PTR(&bigStrDest[i + 1], memccpy(bigStrDest, testStr, testStr[i], ALL_CHARS_STRING_SIZE));
		TEST_ASSERT_EQUAL_CHAR_ARRAY(bigStrDest, testStr, i);
	}

	free((void *)testStr);

#endif
}


TEST(string_memccpy, stop_int_found)
{
#ifdef __phoenix__
	TEST_IGNORE();
#endif
#ifndef __phoenix__

	const char *testStr;
	char bigStrDest[ALL_CHARS_STRING_SIZE] = { 0 };
	/* Memccpy accepts int as an element of stop and converts it to uint8 */
	int bigIntDest[] = { -UCHAR_MAX - 1, UCHAR_MAX + 1, INT_MAX / 5, INT_MAX / 3, INT_MAX, INT_MIN / 5, INT_MIN / 3, INT_MIN };
	int i, pos;
	unsigned long intsNr = sizeof(bigIntDest) / sizeof(int);

	/* testing all possible chars + 1 byte for NUL term */
	testStr = testdata_createCharStr(ALL_CHARS_STRING_SIZE);

	TEST_ASSERT_NOT_NULL(testStr);

	for (i = 0; i < intsNr; i++) {
		if ((unsigned char)bigIntDest[i] != 1) {
			pos = (unsigned char)bigIntDest[i] + 1;
			if ((unsigned char)bigIntDest[i] == 0) {
				pos = ALL_CHARS_STRING_SIZE;
			}
		}
		else {
			pos = (unsigned char)bigIntDest[i];
		}
		TEST_ASSERT_EQUAL_PTR(&bigStrDest[pos], memccpy(bigStrDest, testStr, bigIntDest[i], ALL_CHARS_STRING_SIZE));
		TEST_ASSERT_EQUAL_CHAR_ARRAY(bigStrDest, testStr, pos);
	}

	free((void *)testStr);
#endif
}


TEST(string_memccpy, data_types)
{
#ifdef __phoenix__
	TEST_IGNORE();
#endif
#ifndef __phoenix__

	int num = 12345678,
		numDest = 0;
	float flt = 2.32252,
		  fltDest = 0;
	ptrdiff_t ptr = 0x2345,
			  ptrDest = 0;
	size_t size = SIZE_MAX,
		   sizDest = 0;

	/* Checking ability to copy numbers between places */
	TEST_ASSERT_EQUAL_PTR(NULL, memccpy(&numDest, &num, 'x', sizeof(num)));
	TEST_ASSERT_NOT_EMPTY(&numDest);
	TEST_ASSERT_EQUAL_INT(num, numDest);

	TEST_ASSERT_EQUAL_PTR(NULL, memccpy(&fltDest, &flt, 'x', sizeof(flt)));
	TEST_ASSERT_NOT_EMPTY(&fltDest);
	TEST_ASSERT_FLOAT_IS_DETERMINATE(fltDest);
	TEST_ASSERT_EQUAL_FLOAT(flt, fltDest);

	TEST_ASSERT_EQUAL_PTR(NULL, memccpy(&ptrDest, &ptr, 'x', sizeof(ptr)));
	TEST_ASSERT_NOT_EMPTY(&ptrDest);
	TEST_ASSERT_EQUAL_INT(ptr, ptrDest);

	TEST_ASSERT_EQUAL_PTR(NULL, memccpy(&sizDest, &size, 'x', sizeof(size)));
	TEST_ASSERT_NOT_EMPTY(&sizDest);
	TEST_ASSERT_EQUAL_DOUBLE(size, sizDest);

#endif
}


TEST(string_memccpy, adjacent)
{
#ifdef __phoenix__
	TEST_IGNORE();
#endif
#ifndef __phoenix__

	char testStr[] = "TEST",
		 memStr[MAX_STR_LEN] = "\0\0\0\0\0\0\0\0\0\0TEST",
		 expVal[MAX_STR_LEN] = "\0\0\0\0\0\0TESTTESTTEST",
		 zeroStr[MAX_STR_LEN] = { 0 },
		 testStrLen = sizeof(testStr) - 1;

	/*
	 * Copy the string in his memory space right before his original place to get the effect
	 * like using strcat but in the opposite direction
	 */
	TEST_ASSERT_EQUAL_PTR(NULL, memccpy(&memStr[10 - testStrLen], &memStr[10], 'x', testStrLen));
	TEST_ASSERT_EQUAL_CHAR_ARRAY(&memStr[10 - testStrLen], &expVal[10 - testStrLen], testStrLen * 2);
	/* Checking if zeros before and after text are intact */
	TEST_ASSERT_EQUAL_CHAR_ARRAY(memStr, zeroStr, 6);
	TEST_ASSERT_EQUAL_CHAR_ARRAY(&memStr[10 + testStrLen], zeroStr, 10);

	/* Copy text in a similar way how do it strcat*/
	TEST_ASSERT_EQUAL_PTR(NULL, memccpy(&memStr[10 + testStrLen], &memStr[10], 'x', testStrLen));
	/* Checking if zeros before and after text are intact and text was copied correctly */
	TEST_ASSERT_EQUAL_CHAR_ARRAY(memStr, expVal, sizeof(memStr));

#endif
}


TEST(string_memccpy, one_byte)
{
#ifdef __phoenix__
	TEST_IGNORE();
#endif
#ifndef __phoenix__

	char testArray[CHARS_SET_SIZE] = { 0 },
		 input[CHARS_SET_SIZE] = { 0 };
	int i;

	for (i = 0; i < CHARS_SET_SIZE; i++) {
		input[i] = i;
		/* This loop will copy only one byte in a place where the loop iterator pointing to this place */
		TEST_ASSERT_EQUAL_PTR(&testArray[i] + 1, memccpy(&testArray[i], &input[i], input[i], 1));
	}

	/* Checking if all elements were correctly copied */
	TEST_ASSERT_EQUAL_CHAR_ARRAY(input, testArray, CHARS_SET_SIZE);

#endif
}


TEST(string_memccpy, clearing_array)
{
#ifdef __phoenix__
	TEST_IGNORE();
#endif
#ifndef __phoenix__

	char testArray[MAX_STR_LEN] = TEST_STR1,
		 input[MAX_STR_LEN] = { 0 };

	/* Clearing array with support array filled with 0 using memcpy to do this*/
	TEST_ASSERT_EQUAL_PTR(testArray, memcpy(testArray, input, MAX_STR_LEN));
	TEST_ASSERT_EQUAL_CHAR_ARRAY(input, testArray, MAX_STR_LEN);

#endif
}


TEST(string_memccpy, various_sizes)
{
#ifdef __phoenix__
	TEST_IGNORE();
#endif
#ifndef __phoenix__


	char testArray[MAX_STR_LEN] = TEST_STR1,
		 input[MAX_STR_LEN] = "0123456789";

	/* Trying to copy zero bytes */
	TEST_ASSERT_EQUAL_PTR(NULL, memccpy(testArray, input, 'x', 0));
	TEST_ASSERT_EQUAL_STRING(testArray, TEST_STR1);

	/* this particular check is not needed as we check size bigger than the destination using BIG_NUMB */
	/* Using sizes to copy only part of the array to another */
	TEST_ASSERT_EQUAL_PTR(NULL, memccpy(testArray, input, 'x', sizeof(testArray) / 2));
	/* Checking if a copy was executed only on half of the array size */
	TEST_ASSERT_EQUAL_CHAR_ARRAY(testArray, input, sizeof(testArray) / 2);
	TEST_ASSERT_EQUAL_CHAR_ARRAY(&testArray[sizeof(testArray) / 2], &TEST_STR1[sizeof(testArray) / 2], strlen(&TEST_STR1[sizeof(testArray) / 2]));

	memset(testArray, 0, sizeof(testArray));

	/* Testing size lower than stop character position */
	TEST_ASSERT_EQUAL_PTR(NULL, memccpy(testArray, input, '9', 5));
	TEST_ASSERT_EQUAL_CHAR_ARRAY(testArray, input, 5);

	memset(testArray, 0, sizeof(testArray));

	/* Testing size bigger than stop character position */
	TEST_ASSERT_EQUAL_PTR(&testArray[6], memccpy(testArray, input, '5', 9));
	TEST_ASSERT_EQUAL_CHAR_ARRAY(testArray, input, 6);

	memset(testArray, 0, sizeof(testArray));

	/* Testing passing length longer than the input string */
	TEST_ASSERT_EQUAL_PTR(NULL, memccpy(testArray, input, 'x', sizeof(input) - 1));
	TEST_ASSERT_EQUAL_CHAR_ARRAY(testArray, input, sizeof(input));

#endif
}


TEST(string_memccpy, big)
{
#ifdef __phoenix__
	TEST_IGNORE();
#endif
#ifndef __phoenix__

	char buff[BIG_NUMB] = { 0 };
	char *longStr;

	/* Checking capability of handling big blocks of data */
	longStr = testdata_createCharStr(BIG_NUMB);

	TEST_ASSERT_NOT_NULL(longStr);

	TEST_ASSERT_EQUAL_PTR(&buff[sizeof(buff)], memccpy(buff, longStr, 0, BIG_NUMB));
	TEST_ASSERT_EQUAL_CHAR_ARRAY(buff, longStr, BIG_NUMB);

	free((void *)longStr);

#endif
}


/*
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
 */


TEST_SETUP(string_strncpy)
{
}


TEST_TEAR_DOWN(string_strncpy)
{
}


TEST(string_strncpy, basic)
{
	char buff[MAX_STR_LEN] = { 0 };
	int i;

	TEST_ASSERT_EQUAL_STRING(TEST_STR1, strncpy(buff, TEST_STR1, sizeof(buff)));
	TEST_ASSERT_EQUAL_STRING(TEST_STR1, buff);

	/* Checking if we don't overwrite elements after the end of input*/
	for (i = sizeof(TEST_STR1) - 1; i < sizeof(buff); i++) {
		TEST_ASSERT_EQUAL_CHAR('\0', buff[i]);
	}

	memset(buff, 0, sizeof(buff));

	TEST_ASSERT_EQUAL_STRING(TEST_STR2, strncpy(buff, TEST_STR2, sizeof(buff)));
	TEST_ASSERT_EQUAL_STRING(TEST_STR2, buff);

	for (i = sizeof(TEST_STR2) - 1; i < sizeof(buff); i++) {
		TEST_ASSERT_EQUAL_CHAR('\0', buff[i]);
	}

	/* Buffer not cleared intentionally to check copy capability*/
	TEST_ASSERT_EQUAL_STRING(buff, strncpy(buff, TEST_STR1, sizeof(buff)));
	TEST_ASSERT_EQUAL_STRING(TEST_STR1, buff);

	for (i = sizeof(TEST_STR1); i < sizeof(buff); i++) {
		TEST_ASSERT_EQUAL_CHAR('\0', buff[i]);
	}
}


TEST(string_strncpy, ascii)
{
	char buff[CHARS_SET_SIZE] = { 0 },
		 ascii[CHARS_SET_SIZE] = { 0 };
	int i;

	for (i = 1; i < CHARS_SET_SIZE - 1; i++) {
		ascii[i - 1] = i;
		TEST_ASSERT_EQUAL_PTR(buff, strncpy(buff, ascii, sizeof(ascii)));
	}

	TEST_ASSERT_EQUAL_CHAR_ARRAY(buff, ascii, sizeof(buff));
}


TEST(string_strncpy, null_sens)
{
	char buff[MAX_STR_LEN] = { 0 };
	char testStr[] = "Lorem \0 ipsum";

	/* Checking if strncpy correctly read the string and if it is sensitivity on Null terminating zero */
	TEST_ASSERT_EQUAL_STRING(testStr, strncpy(buff, testStr, sizeof(buff)));
	TEST_ASSERT_NOT_EQUAL_CHAR(testStr[sizeof(testStr) / 2], buff[sizeof(testStr) / 2]);

	memset(buff, 0, sizeof(buff));
	memset(testStr, 0, sizeof(testStr));

	/* Checking if we can copy place where NULL element is present */
	TEST_ASSERT_EQUAL_PTR(buff, strncpy(buff, "", sizeof(buff)));
	TEST_ASSERT_EQUAL_CHAR_ARRAY(buff, testStr, sizeof(testStr));
}


TEST(string_strncpy, adjacent)
{
	/* Pragma truncation disable for the different lengths of copy string tests than source */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"

	char testStr[] = "TEST",
		 memStr[MAX_STR_LEN] = "\0\0\0\0\0\0\0\0\0\0TEST",
		 expVal[MAX_STR_LEN] = "\0\0\0\0\0\0TESTTESTTEST",
		 zeroStr[MAX_STR_LEN] = { 0 },
		 testStrLen = sizeof(testStr) - 1;

	/* Copy data in same space but with offset to left */
	TEST_ASSERT_EQUAL_PTR(&memStr[10 - testStrLen], strncpy(&memStr[10 - testStrLen], &memStr[10], testStrLen));
	TEST_ASSERT_EQUAL_CHAR_ARRAY(&memStr[10 - testStrLen], &expVal[10 - testStrLen], testStrLen * 2);
	/* Checking if zeros before and after text are intact */
	TEST_ASSERT_EQUAL_CHAR_ARRAY(memStr, zeroStr, 6);
	TEST_ASSERT_EQUAL_CHAR_ARRAY(&memStr[10 + testStrLen], zeroStr, 10);

	/* Copy data in same space but with offset to right */
	TEST_ASSERT_EQUAL_PTR(&memStr[10 + testStrLen], strncpy(&memStr[10 + testStrLen], &memStr[10], testStrLen));
	/* Checking if zeros before and after text are intact and text was copied correctly */
	TEST_ASSERT_EQUAL_CHAR_ARRAY(memStr, expVal, sizeof(memStr));

#pragma GCC diagnostic pop
}


TEST(string_strncpy, one_byte)
{
	char buff[CHARS_SET_SIZE] = { 0 },
		 ascii[CHARS_SET_SIZE] = { 0 };
	int i;

	/* Copy one by one from created ascii table */
	for (i = 1; i < CHARS_SET_SIZE; i++) {
		ascii[i - 1] = i;

		TEST_ASSERT_EQUAL_CHAR_ARRAY(buff, strncpy(buff, &ascii[i], 1), sizeof(buff));
	}
}


TEST(string_strncpy, various_sizes)
{
	/* Pragma truncation disable for the different lengths of copy string tests than source */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"

	char input[MAX_STR_LEN] = TEST_STR2,
		 testArray[MAX_STR_LEN] = { 0 },
		 smallInput[MAX_STR_LEN / 2] = { 0 };

	/* Trying to copy zero bytes */
	TEST_ASSERT_EQUAL_PTR(testArray, strncpy(testArray, input, 0));
	TEST_ASSERT_EQUAL_STRING(testArray, "");

	memset(testArray, 0, sizeof(testArray));

	/* Using sizes to copy only part of the array to another */
	TEST_ASSERT_EQUAL_PTR(testArray, strncpy(testArray, input, sizeof(TEST_STR2) / 2));
	/* Checking if a copy was executed only on half of the array size */
	TEST_ASSERT_EQUAL_CHAR_ARRAY(testArray, TEST_STR2, sizeof(TEST_STR2) / 2);
	TEST_ASSERT_EQUAL_CHAR_ARRAY(&testArray[sizeof(TEST_STR2) / 2], smallInput, sizeof(TEST_STR2) / 2);

	memset(testArray, 0, sizeof(testArray));
	memset(smallInput, 1, sizeof(smallInput) - 1);

	/* Checking ability to stop copy while the size is bigger than a null term that's why sizeof -1 to keep null term*/
	TEST_ASSERT_EQUAL_PTR(testArray, strncpy(testArray, smallInput, sizeof(smallInput) * 2));
	TEST_ASSERT_EQUAL_STRING(testArray, smallInput);

#pragma GCC diagnostic pop
}


TEST(string_strncpy, big)
{
	char bigBuff[BIG_NUMB] = { 0 };
	char *longStr;

	longStr = testdata_createCharStr(BIG_NUMB);

	TEST_ASSERT_NOT_NULL(longStr);

	/* Ability to copy long string */
	TEST_ASSERT_EQUAL_PTR(bigBuff, strncpy(bigBuff, longStr, sizeof(bigBuff) - 1));
	TEST_ASSERT_EQUAL_CHAR_ARRAY(bigBuff, longStr, sizeof(bigBuff));

	free((void *)longStr);
}


TEST(string_strncpy, append_null)
{
	char buff[BIG_NUMB] = { 0 },
		 input[BIG_NUMB / 2] = { 0 };
	const char *testStr;
	int i;

	testStr = testdata_createCharStr(BIG_NUMB);

	TEST_ASSERT_NOT_NULL(testStr);

	/* Creating string without null terminators */
	for (i = 0; i < BIG_NUMB / 2 - 1; i++) {
		if (testStr[i] == 0) {
			input[i] = testStr[i + 1];
		}
		else {
			input[i] = testStr[i];
		}
	}

	/* To check append ability we need to set up values in the buff to other than 0 */
	memset(buff, 1, sizeof(buff));

	TEST_ASSERT_EQUAL_PTR(buff, strncpy(buff, input, sizeof(buff)));
	TEST_ASSERT_EQUAL_CHAR_ARRAY(buff, input, sizeof(input) - 1);

	for (i = BIG_NUMB / 2 - 1; i < BIG_NUMB; i++) {
		TEST_ASSERT_EQUAL_CHAR('\0', buff[i]);
	}

	free((void *)testStr);
}


/*
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
 */


TEST_SETUP(string_stpncpy)
{
}


TEST_TEAR_DOWN(string_stpncpy)
{
}


TEST(string_stpncpy, basic)
{
	/* Ifdef used because lack of function 'stpncpy' */
#ifdef __phoenix__
	TEST_IGNORE();
#endif
#ifndef __phoenix__

	char buff[MAX_STR_LEN] = { 0 };
	int i;

	TEST_ASSERT_EQUAL_STRING(&buff[strlen(buff)], stpncpy(buff, TEST_STR1, sizeof(buff)));
	TEST_ASSERT_EQUAL_STRING(TEST_STR1, buff);

	/* Checking if we don't overwrite elements after the end of input*/
	for (i = sizeof(TEST_STR1) - 1; i < sizeof(buff); i++) {
		TEST_ASSERT_EQUAL_CHAR('\0', buff[i]);
	}

	memset(buff, 0, sizeof(buff));

	TEST_ASSERT_EQUAL_STRING(&buff[strlen(buff)], stpncpy(buff, TEST_STR2, sizeof(buff)));
	TEST_ASSERT_EQUAL_STRING(TEST_STR2, buff);

	for (i = sizeof(TEST_STR2) - 1; i < sizeof(buff); i++) {
		TEST_ASSERT_EQUAL_CHAR('\0', buff[i]);
	}

	/* Buffer not cleared intentionally to check copy capability*/
	TEST_ASSERT_EQUAL_STRING(&buff[strlen(buff)], stpncpy(buff, TEST_STR1, sizeof(buff)));
	TEST_ASSERT_EQUAL_STRING(TEST_STR1, buff);

	for (i = sizeof(TEST_STR1) - 1; i < sizeof(buff); i++) {
		TEST_ASSERT_EQUAL_CHAR('\0', buff[i]);
	}
#endif
}


TEST(string_stpncpy, ascii)
{
#ifdef __phoenix__
	TEST_IGNORE();
#endif
#ifndef __phoenix__


	char buff[CHARS_SET_SIZE] = { 0 },
		 ascii[CHARS_SET_SIZE] = { 0 };
	int i;

	for (i = 1; i < CHARS_SET_SIZE; i++) {
		ascii[i - 1] = i;

		TEST_ASSERT_EQUAL_PTR(&buff[i], stpncpy(buff, ascii, sizeof(ascii)));
	}

	TEST_ASSERT_EQUAL_CHAR_ARRAY(buff, ascii, sizeof(buff));
#endif
}


TEST(string_stpncpy, null_sens)
{
#ifdef __phoenix__
	TEST_IGNORE();
#endif
#ifndef __phoenix__

	char buff[MAX_STR_LEN] = { 0 };
	char testStr[] = "Lorem \0 ipsum";

	/* Checking if strncpy correctly read the string and if it is sensitivity on Null terminating zero */
	TEST_ASSERT_EQUAL_PTR(&buff[sizeof(testStr) / 2 - 1], stpncpy(buff, testStr, sizeof(buff)));
	TEST_ASSERT_NOT_EQUAL_CHAR(testStr[sizeof(testStr) / 2], buff[sizeof(testStr) / 2]);

	memset(buff, 0, sizeof(buff));
	memset(testStr, 0, sizeof(testStr));

	/* Checking if we can copy place where NULL element is present */
	TEST_ASSERT_EQUAL_PTR(buff, stpncpy(buff, "", sizeof(buff)));
	TEST_ASSERT_EQUAL_CHAR_ARRAY(buff, testStr, sizeof(testStr));

#endif
}


TEST(string_stpncpy, adjacent)
{
#ifdef __phoenix__
	TEST_IGNORE();
#endif
#ifndef __phoenix__
	/* Pragma truncation disable for the different lengths of copy string tests than source */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"

	char testStr[] = "TEST",
		 memStr[MAX_STR_LEN] = "\0\0\0\0\0\0\0\0\0\0TEST",
		 expVal[MAX_STR_LEN] = "\0\0\0\0\0\0TESTTESTTEST",
		 zeroStr[MAX_STR_LEN] = { 0 },
		 testStrLen = sizeof(testStr) - 1;

	/* Copy data in same space but with offset to left */
	TEST_ASSERT_EQUAL_STRING(&memStr[10], stpncpy(&memStr[10 - testStrLen], &memStr[10], testStrLen));
	TEST_ASSERT_EQUAL_CHAR_ARRAY(&memStr[10 - testStrLen], &expVal[10 - testStrLen], testStrLen * 2);
	/* Checking if zeros before and after text are intact */
	TEST_ASSERT_EQUAL_CHAR_ARRAY(memStr, zeroStr, 6);
	TEST_ASSERT_EQUAL_CHAR_ARRAY(&memStr[10 + testStrLen], zeroStr, 10);

	/* Copy data in same space but with offset to right */
	TEST_ASSERT_EQUAL_PTR(&memStr[10 + testStrLen * 2], stpncpy(&memStr[10 + testStrLen], &memStr[10], testStrLen));
	/* Checking if zeros before and after text are intact and text was copied correctly */
	TEST_ASSERT_EQUAL_CHAR_ARRAY(memStr, expVal, sizeof(memStr));

#pragma GCC diagnostic pop
#endif
}


TEST(string_stpncpy, one_byte)
{
#ifdef __phoenix__
	TEST_IGNORE();
#endif
#ifndef __phoenix__
	/* Pragma truncation disable for the different lengths of copy string tests than source */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"

	char buff[CHARS_SET_SIZE] = { 0 },
		 ascii[CHARS_SET_SIZE] = { 0 };
	int i;

	/* Copy one by one from created ascii table constantly into the same place */
	for (i = 1; i < CHARS_SET_SIZE; i++) {
		ascii[i - 1] = i;

		/* Stpncpy always returns the place where the NULL term was found */
		TEST_ASSERT_EQUAL_PTR(&buff[1], stpncpy(buff, &ascii[i - 1], 1));
		TEST_ASSERT_EQUAL_CHAR(buff[0], ascii[i - 1]);
		/* Checking if there is nothing more */
		TEST_ASSERT_EQUAL_CHAR_ARRAY(buff, &ascii[i - 1], 1);
	}

#pragma GCC diagnostic pop
#endif
}


TEST(string_stpncpy, various_sizes)
{
#ifdef __phoenix__
	TEST_IGNORE();
#endif
#ifndef __phoenix__
	/* Pragma truncation disable for the different lengths of copy string tests than source */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"

	char testArray[MAX_STR_LEN] = { 0 },
		 input[MAX_STR_LEN] = TEST_STR2,
		 smallInput[MAX_STR_LEN / 2] = { 0 };

	/* Trying to copy zero bytes */
	TEST_ASSERT_EQUAL_PTR(testArray, stpncpy(testArray, input, 0));
	TEST_ASSERT_EQUAL_STRING(testArray, "");

	memset(testArray, 0, sizeof(testArray));

	/* Using sizes to copy only part of the array to another */
	TEST_ASSERT_EQUAL_PTR(&testArray[sizeof(TEST_STR2) / 2], stpncpy(testArray, input, sizeof(TEST_STR2) / 2));
	/* Checking if a copy was executed only on half of the array size */
	TEST_ASSERT_EQUAL_CHAR_ARRAY(testArray, TEST_STR2, sizeof(TEST_STR2) / 2);
	TEST_ASSERT_EQUAL_CHAR_ARRAY(&testArray[sizeof(TEST_STR2) / 2], smallInput, sizeof(TEST_STR2) / 2);

	memset(testArray, 0, sizeof(testArray));

	/* Checking ability to stop copy while the size is bigger than a null term that's why sizeof -1 to keep null term*/
	memset(smallInput, 1, sizeof(smallInput) - 1);
	TEST_ASSERT_EQUAL_PTR(&testArray[sizeof(smallInput) - 1], stpncpy(testArray, smallInput, sizeof(smallInput) * 2));
	TEST_ASSERT_EQUAL_STRING(testArray, smallInput);

#pragma GCC diagnostic pop
#endif
}


TEST(string_stpncpy, big)
{
#ifdef __phoenix__
	TEST_IGNORE();
#endif
#ifndef __phoenix__

	char bigBuff[BIG_NUMB] = { 0 };
	char *longStr;

	longStr = testdata_createCharStr(BIG_NUMB);

	TEST_ASSERT_NOT_NULL(longStr);

	/* Ability to copy long string */
	TEST_ASSERT_EQUAL_PTR(&bigBuff[strlen(longStr)], stpncpy(bigBuff, longStr, sizeof(bigBuff) - 1));
	TEST_ASSERT_EQUAL_CHAR_ARRAY(bigBuff, longStr, sizeof(bigBuff));

	free((void *)longStr);

#endif
}


TEST(string_stpncpy, append_null)
{
#ifdef __phoenix__
	TEST_IGNORE();
#endif
#ifndef __phoenix__

	char buff[BIG_NUMB] = { 0 },
		 input[BIG_NUMB / 2] = { 0 };
	const char *testStr;
	int i;

	testStr = testdata_createCharStr(BIG_NUMB);

	TEST_ASSERT_NOT_NULL(testStr);

	/* Creating string without null terminators */
	for (i = 0; i < BIG_NUMB / 2 - 1; i++) {
		if (testStr[i] == 0) {
			input[i] = testStr[i + 1];
		}
		else {
			input[i] = testStr[i];
		}
	}

	/* To check append ability we need to set up values in the buff to other than 0 */
	memset(buff, 1, sizeof(buff));

	TEST_ASSERT_EQUAL_PTR(&buff[strlen(buff)], stpncpy(buff, input, sizeof(buff)));
	TEST_ASSERT_EQUAL_CHAR_ARRAY(buff, input, sizeof(input) - 1);

	for (i = BIG_NUMB / 2 - 1; i < BIG_NUMB; i++) {
		TEST_ASSERT_EQUAL_CHAR('\0', buff[i]);
	}

	free((void *)testStr);
#endif
}


/*
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
 */


TEST_SETUP(string_strcpy_stpcpy)
{
}


TEST_TEAR_DOWN(string_strcpy_stpcpy)
{
}


TEST(string_strcpy_stpcpy, basic)
{
	char buff[MAX_STR_LEN] = { 0 };
	int i;

	TEST_ASSERT_EQUAL_PTR(buff, strcpy(buff, TEST_STR1));
	TEST_ASSERT_EQUAL_STRING(TEST_STR1, buff);

	/* Checking if we don't overwrite elements after the end of input*/
	for (i = sizeof(TEST_STR1) - 1; i < sizeof(buff); i++) {
		TEST_ASSERT_EQUAL_CHAR('\0', buff[i]);
	}

	memset(buff, 0, sizeof(buff));

	TEST_ASSERT_EQUAL_PTR(&buff[sizeof(TEST_STR1) - 1], stpcpy(buff, TEST_STR1));
	TEST_ASSERT_EQUAL_STRING(TEST_STR1, buff);

	for (i = sizeof(TEST_STR1) - 1; i < sizeof(buff); i++) {
		TEST_ASSERT_EQUAL_CHAR('\0', buff[i]);
	}

	memset(buff, 0, sizeof(buff));

	TEST_ASSERT_EQUAL_PTR(buff, strcpy(buff, TEST_STR2));
	TEST_ASSERT_EQUAL_STRING(TEST_STR2, buff);

	for (i = sizeof(TEST_STR2) - 1; i < sizeof(buff); i++) {
		TEST_ASSERT_EQUAL_CHAR('\0', buff[i]);
	}

	memset(buff, 0, sizeof(buff));

	TEST_ASSERT_EQUAL_PTR(&buff[sizeof(TEST_STR2) - 1], stpcpy(buff, TEST_STR2));
	TEST_ASSERT_EQUAL_STRING(TEST_STR2, buff);

	for (i = sizeof(TEST_STR2) - 1; i < sizeof(buff); i++) {
		TEST_ASSERT_EQUAL_CHAR('\0', buff[i]);
	}
}


TEST(string_strcpy_stpcpy, ascii)
{
	char buff[CHARS_SET_SIZE] = { 0 },
		 ascii[CHARS_SET_SIZE] = { 0 };
	int i;

	/* Copying ASCII bytes one by one to buff and check if the values that functions give are right */
	for (i = 1; i < CHARS_SET_SIZE; i++) {

		ascii[i - 1] = i;

		TEST_ASSERT_EQUAL_PTR(buff, strcpy(buff, &ascii[i - 1]));
		TEST_ASSERT_EQUAL_STRING(buff, &ascii[i - 1]);

		memset(buff, 0, sizeof(buff));

		TEST_ASSERT_EQUAL_PTR(&buff[1], stpcpy(buff, &ascii[i - 1]));
		TEST_ASSERT_EQUAL_STRING(buff, &ascii[i - 1]);
	}
}


TEST(string_strcpy_stpcpy, null_sens)
{
	char buff[MAX_STR_LEN] = { 0 };
	char testStr[] = "Lorem \0 ipsum";

	/* Checking if strncpy correctly read the string and if it is sensitivity on Null terminating zero */
	TEST_ASSERT_EQUAL_PTR(buff, strcpy(buff, testStr));
	TEST_ASSERT_NOT_EQUAL_CHAR(testStr[sizeof(testStr) / 2], buff[sizeof(testStr) / 2]);

	/* Checking if strncpy correctly read the string and if it is sensitivity on Null terminating zero */
	memset(buff, 0, sizeof(buff));
	TEST_ASSERT_EQUAL_PTR(&buff[sizeof(testStr) / 2 - 1], stpcpy(buff, testStr));
	TEST_ASSERT_NOT_EQUAL_CHAR(testStr[sizeof(testStr) / 2], buff[sizeof(testStr) / 2]);

	memset(buff, 0, sizeof(buff));
	memset(testStr, 0, sizeof(testStr));

	/* Checking if we can copy place where NULL element is present */
	TEST_ASSERT_EQUAL_PTR(buff, strcpy(buff, ""));
	TEST_ASSERT_EQUAL_CHAR_ARRAY(buff, testStr, sizeof(testStr));

	memset(buff, 0, sizeof(buff));
	/* Checking if we can copy place where NULL element is present */
	TEST_ASSERT_EQUAL_PTR(buff, stpcpy(buff, ""));
	TEST_ASSERT_EQUAL_CHAR_ARRAY(buff, testStr, sizeof(testStr));
}


TEST(string_strcpy_stpcpy, one_byte)
{
	char buff[CHARS_SET_SIZE] = { 0 },
		 ascii[CHARS_SET_SIZE] = { 0 };
	int i;

	/* Copy one by one from created table constantly into the same place */
	for (i = 1; i < CHARS_SET_SIZE - 1; i++) {
		ascii[i - 1] = i;

		TEST_ASSERT_EQUAL_PTR(buff, strcpy(buff, &ascii[i - 1]));
		TEST_ASSERT_EQUAL_CHAR(buff[0], ascii[i - 1]);

		memset(buff, 0, sizeof(buff));
		TEST_ASSERT_EQUAL_PTR(&buff[1], stpcpy(buff, &ascii[i - 1]));
		TEST_ASSERT_EQUAL_CHAR(buff[i], ascii[i]);
	}
}


TEST(string_strcpy_stpcpy, strcpy_adjacent)
{
	char testStr[] = "TEST",
		 memStr[MAX_STR_LEN] = "\0\0\0\0\0\0\0\0\0\0TEST",
		 expVal[MAX_STR_LEN] = "\0\0\0\0\0TEST\0TEST\0TEST",
		 zeroStr[MAX_STR_LEN] = { 0 },
		 testStrLen = sizeof(testStr) - 1;

	/*
	 * Copy data in the same space but with offset to the left
	 * (9 is used because of the specific copy and adding /0 on the end)
	 */
	TEST_ASSERT_EQUAL_STRING(&memStr[9 - testStrLen], strcpy(&memStr[9 - testStrLen], &memStr[10]));
	TEST_ASSERT_EQUAL_CHAR_ARRAY(&memStr[10 - testStrLen], &expVal[10 - testStrLen], testStrLen * 2 + 1);
	/* Checking if zeros before and after text are intact */
	TEST_ASSERT_EQUAL_CHAR_ARRAY(memStr, zeroStr, 5);
	TEST_ASSERT_EQUAL_CHAR_ARRAY(&memStr[10 + testStrLen], zeroStr, 10);

	/* Copy data in the same space but with offset to right remembering we must jump over one place to avoid overlap */
	TEST_ASSERT_EQUAL_PTR(&memStr[11 + testStrLen], strcpy(&memStr[11 + testStrLen], &memStr[10]));
	/* Checking if zeros before and after text are intact and text was copied correctly */
	TEST_ASSERT_EQUAL_CHAR_ARRAY(memStr, expVal, sizeof(memStr));
}


TEST(string_strcpy_stpcpy, stpcpy_adjacent)
{
	char testStr[] = "TEST",
		 memStr[MAX_STR_LEN] = "\0\0\0\0\0\0\0\0\0\0TEST",
		 expVal[MAX_STR_LEN] = "\0\0\0\0\0TEST\0TEST\0TEST",
		 zeroStr[MAX_STR_LEN] = { 0 },
		 testStrLen = sizeof(testStr) - 1;

	/* Copy data in same space but with offset to left */
	TEST_ASSERT_EQUAL_STRING(&memStr[9], stpcpy(&memStr[9 - testStrLen], &memStr[10]));
	TEST_ASSERT_EQUAL_CHAR_ARRAY(&memStr[10 - testStrLen], &expVal[10 - testStrLen], testStrLen * 2 + 1);
	/* Checking if zeros before and after text are intact */
	TEST_ASSERT_EQUAL_CHAR_ARRAY(memStr, zeroStr, 5);
	TEST_ASSERT_EQUAL_CHAR_ARRAY(&memStr[10 + testStrLen], zeroStr, 10);

	/* Copy data in the same space but with offset to right remembering we must jump over one place to avoid overlap */
	TEST_ASSERT_EQUAL_PTR(&memStr[11 + testStrLen * 2], stpcpy(&memStr[11 + testStrLen], &memStr[10]));
	/* Checking if zeros before and after text are intact and text was copied correctly */
	TEST_ASSERT_EQUAL_CHAR_ARRAY(memStr, expVal, sizeof(memStr));
}


TEST(string_strcpy_stpcpy, big)
{
	char bigBuff[BIG_NUMB] = { 0 };
	char *longStr;

	longStr = testdata_createCharStr(BIG_NUMB);

	TEST_ASSERT_NOT_NULL(longStr);

	/* Ability to copy long string */
	TEST_ASSERT_EQUAL_PTR(bigBuff, strcpy(bigBuff, longStr));
	TEST_ASSERT_EQUAL_CHAR_ARRAY(bigBuff, longStr, sizeof(bigBuff));


	memset(bigBuff, 0, sizeof(bigBuff));

	TEST_ASSERT_EQUAL_PTR(&bigBuff[strlen(longStr)], stpcpy(bigBuff, longStr));
	TEST_ASSERT_EQUAL_CHAR_ARRAY(bigBuff, longStr, sizeof(bigBuff));


	free((void *)longStr);
}


/*
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/


TEST_SETUP(string_strlcpy)
{
}


TEST_TEAR_DOWN(string_strlcpy)
{
}


TEST(string_strlcpy, strlcpy_fullcopy)
{
#ifdef __phoenix__
	const char source[] = STR_SRC;
	char dest[] = STR_DEST;


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-function-declaration"
	/* Test full copy */
	int retval = strlcpy(dest, source, sizeof(source));
	TEST_ASSERT_EQUAL_INT(sizeof(source) - 1, retval);
	TEST_ASSERT_EQUAL_STRING(source, dest);
#pragma GCC diagnostic pop
#else
	TEST_IGNORE();
#endif
}


TEST(string_strlcpy, strlcpy_shorter)
{
#ifdef __phoenix__
	const char source[] = STR_SRC;
	char dest[] = STR_DEST;

	/* Test shorter than source copy */
	int retval = strlcpy(dest, source, sizeof(source) - 2);
	TEST_ASSERT_EQUAL_STRING("ab", dest);
	TEST_ASSERT_EQUAL_INT(sizeof(source) - 1, retval);
#else
	TEST_IGNORE();
#endif
}


TEST(string_strlcpy, strlcpy_longer)
{
#ifdef __phoenix__
	char source[] = STR_SRC;
	char dest[] = STR_DEST;

	/* Test longer than source copy */
	source[3] = '\0'; /* source is now "abc" null terminated; */
	int retval = strlcpy(dest, source, sizeof(source));
	TEST_ASSERT_EQUAL_STRING("abc", dest);
	TEST_ASSERT_EQUAL_INT(sizeof(source) - 2, retval);
	source[3] = 'd';
#else
	TEST_IGNORE();
#endif
}


TEST(string_strlcpy, strlcpy_onelength)
{
#ifdef __phoenix__
	const char source[] = STR_SRC;
	char dest[] = STR_DEST;

	/* Test 1 length copy */
	int retval = strlcpy(dest, source, 1);
	TEST_ASSERT_EQUAL_STRING("", dest);
	TEST_ASSERT_EQUAL_INT(sizeof(source) - 1, retval);
#else
	TEST_IGNORE();
#endif
}


TEST(string_strlcpy, strlcpy_zerolength)
{
#ifdef __phoenix__
	const char source[] = STR_SRC;
	char dest[] = STR_DEST;

	/* Test 0 length copy */
	int retval = strlcpy(dest, source, 0);
	TEST_ASSERT_EQUAL_STRING(STR_DEST, dest);
	TEST_ASSERT_EQUAL_INT(sizeof(source) - 1, retval);
#else
	TEST_IGNORE();
#endif
}


TEST_SETUP(string_strlcat)
{
}


TEST_TEAR_DOWN(string_strlcat)
{
}


TEST(string_strlcat, strlcat_fullconcat_empty)
{
#ifdef __phoenix__
	const char source[] = STR_SRC1;
	char buffer[] = STR_PLACEHOLDER;

	memset(buffer, '\0', sizeof(buffer));

	/* Normal, full concat to empty string */
	int retval = strlcat(buffer, source, sizeof(buffer));
	TEST_ASSERT_EQUAL_INT(3, retval);
	TEST_ASSERT_EQUAL_STRING(source, buffer);
#else
	TEST_IGNORE();
#endif
}


TEST(string_strlcat, strlcat_fullconcat_part)
{
#ifdef __phoenix__
	const char source[] = STR_SRC2;
	char buffer[] = STR_PLACEHOLDER;

	buffer[3] = '\0';

	/* Normal full concat to partially filled string */
	int retval = strlcat(buffer, source, sizeof(buffer));
	TEST_ASSERT_EQUAL_INT(sizeof(source) + 2, retval);
	TEST_ASSERT_EQUAL_STRING("klmdefgh", buffer);
#else
	TEST_IGNORE();
#endif
}


TEST(string_strlcat, strlcat_partconcat_overflow)
{
#ifdef __phoenix__
	const char source[] = STR_SRC2;
	char buffer[] = STR_PLACEHOLDER;

	buffer[8] = '\0';

	/* Partial concat to the partially filled string that should overflow the buffer */
	int retval = strlcat(buffer, source, sizeof(buffer));
	TEST_ASSERT_EQUAL_INT(sizeof(buffer) + 1, retval);
	TEST_ASSERT_EQUAL_STRING("klmnopqrdef", buffer);
#else
	TEST_IGNORE();
#endif
}


TEST(string_strlcat, strlcat_onelength)
{
#ifdef __phoenix__
	const char source[] = STR_SRC2;
	char buffer[] = STR_PLACEHOLDER;

	/* 1 length concat */
	buffer[6] = '\0';
	int retval = strlcat(buffer, source, 1);
	TEST_ASSERT_EQUAL_INT(sizeof(source), retval);
	TEST_ASSERT_EQUAL_STRING("klmnop", buffer);
#else
	TEST_IGNORE();
#endif
}


TEST(string_strlcat, strlcat_zerolength)
{
#ifdef __phoenix__
	const char source[] = STR_SRC2;
	char buffer[] = STR_PLACEHOLDER;

	/* 0 length concat */
	buffer[6] = '\0';
	int retval = strlcat(buffer, source, 0);
	TEST_ASSERT_EQUAL_INT(sizeof(source) - 1, retval);
	TEST_ASSERT_EQUAL_STRING("klmnop", buffer);
#else
	TEST_IGNORE();
#endif
}


/*
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/


TEST_GROUP_RUNNER(string_memcpy)
{
	RUN_TEST_CASE(string_memcpy, basic);
	RUN_TEST_CASE(string_memcpy, data_types);
	RUN_TEST_CASE(string_memcpy, adjacent);
	RUN_TEST_CASE(string_memcpy, one_byte);
	RUN_TEST_CASE(string_memcpy, clearing_array);
	RUN_TEST_CASE(string_memcpy, various_sizes);
	RUN_TEST_CASE(string_memcpy, big);
}


TEST_GROUP_RUNNER(string_memccpy)
{
	RUN_TEST_CASE(string_memccpy, basic);
	RUN_TEST_CASE(string_memccpy, stop_char_found);
	RUN_TEST_CASE(string_memccpy, stop_int_found);
	RUN_TEST_CASE(string_memccpy, data_types);
	RUN_TEST_CASE(string_memccpy, adjacent);
	RUN_TEST_CASE(string_memccpy, one_byte);
	RUN_TEST_CASE(string_memccpy, clearing_array);
	RUN_TEST_CASE(string_memccpy, various_sizes);
	RUN_TEST_CASE(string_memccpy, big);
}


TEST_GROUP_RUNNER(string_strncpy)
{
	RUN_TEST_CASE(string_strncpy, basic);
	RUN_TEST_CASE(string_strncpy, ascii);
	RUN_TEST_CASE(string_strncpy, null_sens);
	RUN_TEST_CASE(string_strncpy, adjacent);
	RUN_TEST_CASE(string_strncpy, one_byte);
	RUN_TEST_CASE(string_strncpy, various_sizes);
	RUN_TEST_CASE(string_strncpy, big);
	RUN_TEST_CASE(string_strncpy, append_null);
}


TEST_GROUP_RUNNER(string_stpncpy)
{
	RUN_TEST_CASE(string_stpncpy, basic);
	RUN_TEST_CASE(string_stpncpy, ascii);
	RUN_TEST_CASE(string_stpncpy, null_sens);
	RUN_TEST_CASE(string_stpncpy, adjacent)
	RUN_TEST_CASE(string_stpncpy, one_byte);
	RUN_TEST_CASE(string_stpncpy, various_sizes);
	RUN_TEST_CASE(string_stpncpy, big);
	RUN_TEST_CASE(string_stpncpy, append_null);
}


TEST_GROUP_RUNNER(string_strcpy_stpcpy)
{
	RUN_TEST_CASE(string_strcpy_stpcpy, basic);
	RUN_TEST_CASE(string_strcpy_stpcpy, ascii);
	RUN_TEST_CASE(string_strcpy_stpcpy, null_sens);
	RUN_TEST_CASE(string_strcpy_stpcpy, strcpy_adjacent);
	RUN_TEST_CASE(string_strcpy_stpcpy, stpcpy_adjacent);
	RUN_TEST_CASE(string_strcpy_stpcpy, one_byte);
	RUN_TEST_CASE(string_strcpy_stpcpy, big);
}


TEST_GROUP_RUNNER(string_strlcpy)
{
	RUN_TEST_CASE(string_strlcpy, strlcpy_fullcopy);
	RUN_TEST_CASE(string_strlcpy, strlcpy_shorter);
	RUN_TEST_CASE(string_strlcpy, strlcpy_longer);
	RUN_TEST_CASE(string_strlcpy, strlcpy_onelength);
	RUN_TEST_CASE(string_strlcpy, strlcpy_zerolength);
}


TEST_GROUP_RUNNER(string_strlcat)
{

	RUN_TEST_CASE(string_strlcat, strlcat_fullconcat_empty);
	RUN_TEST_CASE(string_strlcat, strlcat_fullconcat_part);
	RUN_TEST_CASE(string_strlcat, strlcat_partconcat_overflow);
	RUN_TEST_CASE(string_strlcat, strlcat_onelength);
	RUN_TEST_CASE(string_strlcat, strlcat_zerolength);
}
