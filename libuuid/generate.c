/*
 * Phoenix-RTOS
 *
 * libuuid-tests
 *
 * Testing generate related functions
 *
 * Copyright 2022 Phoenix Systems
 * Author: Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <stdio.h>
#include <string.h>
#include <uuid/uuid.h>

#include <unity_fixture.h>


#define TESTED_UUIDS_NUMBER 128


uuid_t uuids[TESTED_UUIDS_NUMBER];


TEST_GROUP(generate);


TEST_SETUP(generate)
{
}


TEST_TEAR_DOWN(generate)
{
}


static void generate_genUuids(uuid_t in[])
{
	int i;

	for (i = 0; i < TESTED_UUIDS_NUMBER; i++) {
		uuid_generate(in[i]);
	}
}


static void generate_assertNotEqual(uuid_t uuid1, uuid_t uuid2, const char *msg)
{
	int uuid_size = sizeof(uuid_t);
	int i;

	for (i = 0; i < uuid_size; i++) {
		if (uuid1[i] != uuid2[i]) {
			break;
		}
	}
	TEST_ASSERT_NOT_EQUAL_INT_MESSAGE(i, uuid_size, msg);  //todo: add when uuid parse will be added
}


TEST(generate, unique)
{
	int i, j;

	for (i = 0; i < TESTED_UUIDS_NUMBER; i++) {
		for (j = 0; j < TESTED_UUIDS_NUMBER; j++) {
			if (i != j)
				generate_assertNotEqual(uuids[i], uuids[j], "Two generated uuids are equal");
		}
	}
}


TEST(generate, version)
{
	unsigned char version, variant;
	int i;

	for (i = 0; i < TESTED_UUIDS_NUMBER; i++) {
		variant = (uuids[i][8] & 0xC0);
		if (variant == 0x40) {
			version = (unsigned char)(uuids[i][6] >> 4);

			TEST_ASSERT_NOT_EQUAL_UINT8_MESSAGE(0, version, "version is 0");
			TEST_ASSERT_LESS_THAN_UINT8_MESSAGE(6, version, "version is 6 or bigger");
		}
	}
}


TEST(generate, change)
{
	uuid_t prev;

	memcpy(prev, uuids[0], 16);

	uuid_generate(uuids[0]);

	generate_assertNotEqual(prev, uuids[0], "The uuid hasn't been generated!");
}


TEST_GROUP_RUNNER(generate)
{
	generate_genUuids(uuids);

	RUN_TEST_CASE(generate, change);
	RUN_TEST_CASE(generate, unique);
	RUN_TEST_CASE(generate, version);
}


TEST_GROUP(clear);


TEST_SETUP(clear)
{
}


TEST_TEAR_DOWN(clear)
{
}


TEST(clear, clear)
{
	int i;
	uuid_t cleared = {
		0x0,
	};

	for (i = 0; i < TESTED_UUIDS_NUMBER; i++) {
		uuid_clear(uuids[i]);
		TEST_ASSERT_EQUAL_UINT8_ARRAY(cleared, uuids[i], 16);
	}
}


TEST_GROUP_RUNNER(clear)
{
	RUN_TEST_CASE(clear, clear);
}
