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

/* defines for string_strlcpy */
#define STR_SRC  "abcd"
#define STR_DEST "xxxx"

/* defines for string_strlcat */
#define STR_SRC1        "abc"
#define STR_SRC2        "defgh"
#define STR_PLACEHOLDER "klmnopqrstu"


TEST_GROUP(string_strlcpy);


TEST_SETUP(string_strlcpy)
{
}


TEST_TEAR_DOWN(string_strlcpy)
{
}


/* NOTE: strlcpy not available on linux, enable tests only on phoenix */
#ifdef __phoenix__
TEST(string_strlcpy, strlcpy_fullcopy)
{
	const char source[] = STR_SRC;
	char dest[] = STR_DEST;

	/* Test full copy */
	int retval = strlcpy(dest, source, sizeof(source));
	TEST_ASSERT_EQUAL_INT(sizeof(source) - 1, retval);
	TEST_ASSERT_EQUAL_STRING(source, dest);
}


TEST(string_strlcpy, strlcpy_shorter)
{
	const char source[] = STR_SRC;
	char dest[] = STR_DEST;

	/* Test shorter than source copy */
	int retval = strlcpy(dest, source, sizeof(source) - 2);
	TEST_ASSERT_EQUAL_STRING("ab", dest);
	TEST_ASSERT_EQUAL_INT(sizeof(source) - 1, retval);
}


TEST(string_strlcpy, strlcpy_longer)
{
	char source[] = STR_SRC;
	char dest[] = STR_DEST;

	/* Test longer than source copy */
	source[3] = '\0'; /* source is now "abc" null terminated; */
	int retval = strlcpy(dest, source, sizeof(source));
	TEST_ASSERT_EQUAL_STRING("abc", dest);
	TEST_ASSERT_EQUAL_INT(sizeof(source) - 2, retval);
	source[3] = 'd';
}


TEST(string_strlcpy, strlcpy_onelength)
{
	const char source[] = STR_SRC;
	char dest[] = STR_DEST;

	/* Test 1 length copy */
	int retval = strlcpy(dest, source, 1);
	TEST_ASSERT_EQUAL_STRING("", dest);
	TEST_ASSERT_EQUAL_INT(sizeof(source) - 1, retval);
}


TEST(string_strlcpy, strlcpy_zerolength)
{
	const char source[] = STR_SRC;
	char dest[] = STR_DEST;

	/* Test 0 length copy */
	int retval = strlcpy(dest, source, 0);
	TEST_ASSERT_EQUAL_STRING(STR_DEST, dest);
	TEST_ASSERT_EQUAL_INT(sizeof(source) - 1, retval);
}

#endif /* __phoenix__ */


TEST_GROUP_RUNNER(string_strlcpy)
{
#ifdef __phoenix__
	RUN_TEST_CASE(string_strlcpy, strlcpy_fullcopy);
	RUN_TEST_CASE(string_strlcpy, strlcpy_shorter);
	RUN_TEST_CASE(string_strlcpy, strlcpy_longer);
	RUN_TEST_CASE(string_strlcpy, strlcpy_onelength);
	RUN_TEST_CASE(string_strlcpy, strlcpy_zerolength);
#endif
}


TEST_GROUP(string_strlcat);


TEST_SETUP(string_strlcat)
{
}


TEST_TEAR_DOWN(string_strlcat)
{
}


#ifdef __phoenix__
TEST(string_strlcat, strlcat_fullconcat_empty)
{
	const char source[] = STR_SRC1;
	char buffer[] = STR_PLACEHOLDER;


	memset(buffer, '\0', sizeof(buffer));

	/* Normal, full concat to empty string */
	int retval = strlcat(buffer, source, sizeof(buffer));
	TEST_ASSERT_EQUAL_INT(3, retval);
	TEST_ASSERT_EQUAL_STRING(source, buffer);
}


TEST(string_strlcat, strlcat_fullconcat_part)
{
	const char source[] = STR_SRC2;
	char buffer[] = STR_PLACEHOLDER;

	buffer[3] = '\0';

	/* Normal full concat to partially filled string */
	int retval = strlcat(buffer, source, sizeof(buffer));
	TEST_ASSERT_EQUAL_INT(sizeof(source) + 2, retval);
	TEST_ASSERT_EQUAL_STRING("klmdefgh", buffer);
}


TEST(string_strlcat, strlcat_partconcat_overflow)
{
	const char source[] = STR_SRC2;
	char buffer[] = STR_PLACEHOLDER;

	buffer[8] = '\0';

	/* Partial concat to partially filled string that should overflow the buffer */
	int retval = strlcat(buffer, source, sizeof(buffer));
	TEST_ASSERT_EQUAL_INT(sizeof(buffer) + 1, retval);
	TEST_ASSERT_EQUAL_STRING("klmnopqrdef", buffer);
}


TEST(string_strlcat, strlcat_onelength)
{
	const char source[] = STR_SRC2;
	char buffer[] = STR_PLACEHOLDER;

	/* 1 length concat */
	buffer[6] = '\0';
	int retval = strlcat(buffer, source, 1);
	TEST_ASSERT_EQUAL_INT(sizeof(source), retval);
	TEST_ASSERT_EQUAL_STRING("klmnop", buffer);
}


TEST(string_strlcat, strlcat_zerolength)
{
	const char source[] = STR_SRC2;
	char buffer[] = STR_PLACEHOLDER;

	/* 0 length concat */
	buffer[6] = '\0';
	int retval = strlcat(buffer, source, 0);
	TEST_ASSERT_EQUAL_INT(sizeof(source) - 1, retval);
	TEST_ASSERT_EQUAL_STRING("klmnop", buffer);
}

#endif /* __phoenix__ */

TEST_GROUP_RUNNER(string_strlcat)
{
#ifdef __phoenix__
	RUN_TEST_CASE(string_strlcat, strlcat_fullconcat_empty);
	RUN_TEST_CASE(string_strlcat, strlcat_fullconcat_part);
	RUN_TEST_CASE(string_strlcat, strlcat_partconcat_overflow);
	RUN_TEST_CASE(string_strlcat, strlcat_onelength);
	RUN_TEST_CASE(string_strlcat, strlcat_zerolength);
#endif
}
