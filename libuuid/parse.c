/*
 * Phoenix-RTOS
 *
 * libuuid-tests
 *
 * Testing parse functions
 *
 * Copyright 2022 Phoenix Systems
 * Author: Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */
#include <stdio.h>
#include <uuid/uuid.h>

#include <unity_fixture.h>


#define TESTED_UUIDS_NUMBER 128
#define ARRAY_SIZE          4

const uuid_t raw[ARRAY_SIZE] = {
	{ 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
	{ 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf },
	{ 0x0, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90, 0xa0, 0xb0, 0xc0, 0xd0, 0xe0, 0xf0 },
	{ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }
};

const char *correctStr[ARRAY_SIZE] = {
	"00000000-0000-0000-0000-000000000000",
	"00010203-0405-0607-0809-0a0b0c0d0e0f",
	"00102030-4050-6070-8090-a0b0c0d0e0f0",
	"ffffffff-ffff-ffff-ffff-ffffffffffff"
};

const char *wrongStr[ARRAY_SIZE] = {
	"00000000-0000-0000-0000-0000000000ww",
	"000-10203-0405-0607-0809-0a0b0c0d0e0",
	"+102030-4050-6070-8090-a0b0c0d=e0f0",
	")(*&*(:|}\\$&*><;^@"
};

TEST_GROUP(parse);


TEST_SETUP(parse)
{
}


TEST_TEAR_DOWN(parse)
{
}


TEST(parse, parse_fail)
{
	int i, ret;
	const char *msg = "Wrong return value, when passing uuid string in wrong format!";
	uuid_t destUuid;

	ret = uuid_parse("This string has got too many characters inside", destUuid);

	TEST_ASSERT_EQUAL_INT_MESSAGE(-1, ret, "Wrong return value when passing too long input string!");

	ret = uuid_parse("Incomplete uuid", destUuid);

	TEST_ASSERT_EQUAL_INT_MESSAGE(-1, ret, "Wrong return value when passing incomplete input string!");

	for (i = 0; i < ARRAY_SIZE; i++) {
		uuid_parse(wrongStr[i], destUuid);
		TEST_ASSERT_EQUAL_INT_MESSAGE(-1, ret, msg);
	}
}


TEST(parse, parse_basic)
{
	int i;
	uuid_t destUuid[ARRAY_SIZE];

	for (i = 0; i < ARRAY_SIZE; i++) {
		uuid_parse(correctStr[i], destUuid[i]);
		TEST_ASSERT_EQUAL_UINT8_ARRAY(raw[i], destUuid[i], 16);
	}
}


TEST(parse, unparse)
{
	int i;
	char destStr[ARRAY_SIZE][37];

	for (i = 0; i < ARRAY_SIZE; i++) {
		uuid_unparse(raw[i], destStr[i]);
		TEST_ASSERT_EQUAL_STRING(correctStr[i], destStr[i]);
	}
}


TEST_GROUP_RUNNER(parse)
{
	RUN_TEST_CASE(parse, unparse);
	RUN_TEST_CASE(parse, parse_basic);
	RUN_TEST_CASE(parse, parse_fail);
}
