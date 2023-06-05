/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - string.h
 * TESTED:
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

TEST_GROUP(string_strlcpy);
TEST_GROUP(string_strlcat);

/*
//////////////////////////////////////////////////////////////////////////////////////////////
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
