/*
 * Phoenix-RTOS
 *
 * libc-tests
 *
 * Testing string.h functions
 *
 * Copyright 2021 Phoenix Systems
 * Author: Mateusz Niewiadomski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <string.h>

#include <unity_fixture.h>

/* fixtures for strlcpy */
char source[5];
char dest[5];
char tmplt[5];

/* fixtures for strlcat */
char source1[4];
char source2[6];
char buffer[12];

/* common fixtures */
int i;


TEST_GROUP(string_strlcpy);


TEST_SETUP(string_strlcpy)
{
	memcpy(source, "abcd", 5);
	memcpy(dest, "xxxx", 5);
	i = -1;
}


TEST_TEAR_DOWN(string_strlcpy)
{
}


TEST(string_strlcpy, strlcpy_fullcopy)
{
	/* Test full copy */
	i = strlcpy(dest, source, 5);
	TEST_ASSERT_EQUAL_INT(4, i);
	TEST_ASSERT_EQUAL_STRING(source, dest);
}


TEST(string_strlcpy, strlcpy_shorter)
{
	/* Test shorter than source copy */
	i = strlcpy(dest, source, 3);
	TEST_ASSERT_EQUAL_STRING("ab", dest);
	TEST_ASSERT_EQUAL_INT(4, i);
}


TEST(string_strlcpy, strlcpy_longer)
{
	/* Test longer than source copy */
	source[3] = '\0'; /* source is now "abc" null terminated; */
	i = strlcpy(dest, source, 5);
	TEST_ASSERT_EQUAL_STRING("abc", dest);
	TEST_ASSERT_EQUAL_INT(3, i);
	source[3] = 'd';
}


TEST(string_strlcpy, strlcpy_onelength)
{
	/* Test 1 length copy */
	i = strlcpy(dest, source, 1);
	TEST_ASSERT_EQUAL_STRING("", dest);
	TEST_ASSERT_EQUAL_INT(4, i);
}


TEST(string_strlcpy, strlcpy_zerolength)
{
	/* Test 1 length copy */
	i = strlcpy(dest, source, 0);
	TEST_ASSERT_EQUAL_STRING("xxxx", dest);
	TEST_ASSERT_EQUAL_INT(4, i);
}


TEST_GROUP_RUNNER(string_strlcpy)
{
	RUN_TEST_CASE(string_strlcpy, strlcpy_fullcopy);
	RUN_TEST_CASE(string_strlcpy, strlcpy_shorter);
	RUN_TEST_CASE(string_strlcpy, strlcpy_longer);
	RUN_TEST_CASE(string_strlcpy, strlcpy_onelength);
	RUN_TEST_CASE(string_strlcpy, strlcpy_zerolength);
}


TEST_GROUP(string_strlcat);


TEST_SETUP(string_strlcat)
{
	memcpy(source1, "abc", 4);
	memcpy(source2, "defgh", 6);
	memcpy(buffer, "klmnopqrstu", 12);
	i = -1;
}


TEST_TEAR_DOWN(string_strlcat)
{
}


TEST(string_strlcat, strlcat_fullconcat_empty)
{
	memset(buffer, '\0', 12);

	/* Normal, full concat to empty string */
	i = strlcat(buffer, source1, 12);
	TEST_ASSERT_EQUAL_INT(3, i);
	TEST_ASSERT_EQUAL_STRING(source1, buffer);
}


TEST(string_strlcat, strlcat_fullconcat_part)
{
	buffer[3] = '\0';

	/* Normal full concat to partially filled string */
	i = strlcat(buffer, source2, 12);
	TEST_ASSERT_EQUAL_INT(8, i);
	TEST_ASSERT_EQUAL_STRING("klmdefgh", buffer);
}


TEST(string_strlcat, strlcat_partconcat_overflow)
{
	buffer[8] = '\0';

	/* Partial concat to partially filled string that should overflow the buffer */
	i = strlcat(buffer, source2, 12);
	TEST_ASSERT_EQUAL_INT(13, i);
	TEST_ASSERT_EQUAL_STRING("klmnopqrdef", buffer);
}


TEST(string_strlcat, strlcat_onelength)
{
	/* 1 length concat */
	buffer[6] = '\0';
	i = strlcat(buffer, source2, 1);
	TEST_ASSERT_EQUAL_INT(6, i);
	TEST_ASSERT_EQUAL_STRING("klmnop", buffer);
}


TEST(string_strlcat, strlcat_zerolength)
{
	/* 0 length concat */
	buffer[6] = '\0';
	i = strlcat(buffer, source2, 0);
	TEST_ASSERT_EQUAL_INT(5, i);
	//TEST_ASSERT_EQUAL_STRING("klmnop", buffer);
}


TEST_GROUP_RUNNER(string_strlcat)
{
	RUN_TEST_CASE(string_strlcat, strlcat_fullconcat_empty);
	RUN_TEST_CASE(string_strlcat, strlcat_fullconcat_part);
	RUN_TEST_CASE(string_strlcat, strlcat_partconcat_overflow);
	RUN_TEST_CASE(string_strlcat, strlcat_onelength);
	RUN_TEST_CASE(string_strlcat, strlcat_zerolength);
}
