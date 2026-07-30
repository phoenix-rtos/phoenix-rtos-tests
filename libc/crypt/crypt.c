/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <unistd.h>
 * TESTED:
 *    - crypt()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Lukasz Kruszynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define _GNU_SOURCE

#include <unistd.h>
#include <string.h>

#include <unity_fixture.h>

#define CRYPT_KEY  "phoenix-rtos"
#define CRYPT_KEY2 "different-key"

/* Two-byte salts drawn from the portable set [a-zA-Z0-9./]. */
#define CRYPT_SALT1 "ab"
#define CRYPT_SALT2 "Z9"

/* Large enough to copy any implementation's encoded string before the next call. */
#define CRYPT_COPY_SZ 256


TEST_GROUP(crypt);


TEST_SETUP(crypt)
{
}


TEST_TEAR_DOWN(crypt)
{
}


/*
 * crypt() shall return a pointer to the encoded string whose first two bytes
 * are those of the salt argument.
 */
TEST(crypt, returns_encoded_with_salt_prefix)
{
	const char *result = crypt(CRYPT_KEY, CRYPT_SALT1);

	if (result == NULL) {
		TEST_IGNORE_MESSAGE("crypt() algorithm for this salt is unavailable");
	}

	TEST_ASSERT_TRUE(strlen(result) >= 2);
	TEST_ASSERT_EQUAL_CHAR(CRYPT_SALT1[0], result[0]);
	TEST_ASSERT_EQUAL_CHAR(CRYPT_SALT1[1], result[1]);
}


/* Encoding the same key and salt shall always yield the same string. */
TEST(crypt, deterministic_for_same_inputs)
{
	static char first[CRYPT_COPY_SZ];
	const char *result = crypt(CRYPT_KEY, CRYPT_SALT1);

	if (result == NULL) {
		TEST_IGNORE_MESSAGE("crypt() algorithm for this salt is unavailable");
	}
	strncpy(first, result, sizeof(first) - 1);
	first[sizeof(first) - 1] = '\0';

	/* The returned static storage is overwritten by each call, hence the copy. */
	result = crypt(CRYPT_KEY, CRYPT_SALT1);
	TEST_ASSERT_NOT_NULL(result);
	TEST_ASSERT_EQUAL_STRING(first, result);
}


/* The salt shall perturb the encoding: its bytes lead the result and change it. */
TEST(crypt, salt_prefix_varies_with_salt)
{
	static char first[CRYPT_COPY_SZ];
	const char *result = crypt(CRYPT_KEY, CRYPT_SALT1);

	if (result == NULL) {
		TEST_IGNORE_MESSAGE("crypt() algorithm for this salt is unavailable");
	}
	TEST_ASSERT_EQUAL_CHAR(CRYPT_SALT1[0], result[0]);
	TEST_ASSERT_EQUAL_CHAR(CRYPT_SALT1[1], result[1]);
	strncpy(first, result, sizeof(first) - 1);
	first[sizeof(first) - 1] = '\0';

	result = crypt(CRYPT_KEY, CRYPT_SALT2);
	if (result == NULL) {
		TEST_IGNORE_MESSAGE("crypt() algorithm for this salt is unavailable");
	}
	TEST_ASSERT_EQUAL_CHAR(CRYPT_SALT2[0], result[0]);
	TEST_ASSERT_EQUAL_CHAR(CRYPT_SALT2[1], result[1]);
	TEST_ASSERT_TRUE(strcmp(first, result) != 0);
}


/* Different keys with the same salt shall produce different encoded strings. */
TEST(crypt, different_keys_differ)
{
	static char first[CRYPT_COPY_SZ];
	const char *result = crypt(CRYPT_KEY, CRYPT_SALT1);

	if (result == NULL) {
		TEST_IGNORE_MESSAGE("crypt() algorithm for this salt is unavailable");
	}
	strncpy(first, result, sizeof(first) - 1);
	first[sizeof(first) - 1] = '\0';

	result = crypt(CRYPT_KEY2, CRYPT_SALT1);
	TEST_ASSERT_NOT_NULL(result);
	TEST_ASSERT_EQUAL_CHAR(CRYPT_SALT1[0], result[0]);
	TEST_ASSERT_EQUAL_CHAR(CRYPT_SALT1[1], result[1]);
	TEST_ASSERT_TRUE(strcmp(first, result) != 0);
}


TEST_GROUP_RUNNER(crypt)
{
	RUN_TEST_CASE(crypt, returns_encoded_with_salt_prefix);
	RUN_TEST_CASE(crypt, deterministic_for_same_inputs);
	RUN_TEST_CASE(crypt, salt_prefix_varies_with_salt);
	RUN_TEST_CASE(crypt, different_keys_differ);
}
