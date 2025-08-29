/*
 * Phoenix-RTOS
 *
 * test_register
 *
 * tests for `portRegister` and `portUnregister` syscalls
 *
 * Copyright 2026 Phoenix Systems
 * Author: Julian Uziembło
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <unistd.h>

#include "unity_fixture.h"

#define LETTERS_IN_ALPHABET ('z' - 'a' + 1)
#define NDEVS               (LETTERS_IN_ALPHABET * LETTERS_IN_ALPHABET)
#define DEVNAMSIZ           2

#define SIMPLE_PORT_NAME "/simple_test"

#define TEST_PORT_REGISTER(name, oid) \
	do { \
		uint32_t _port = (oid).port; \
		id_t _id = (oid).id; \
		TEST_ASSERT_EQUAL_MESSAGE(0, portRegister(_port, (name), &(oid)), (name)); \
		TEST_ASSERT_EQUAL_MESSAGE(_port, (oid).port, (name)); \
		TEST_ASSERT_EQUAL_MESSAGE(_id, (oid).id, (name)); \
	} while (0)


static uint32_t port;


TEST_GROUP(test_register);

TEST_SETUP(test_register)
{
}

TEST_TEAR_DOWN(test_register)
{
}

TEST(test_register, simple)
{
	const id_t id = 0;
	oid_t oid = { .port = port, .id = id }, ofil, odev;
	TEST_PORT_REGISTER(SIMPLE_PORT_NAME, oid);

	TEST_ASSERT_GREATER_OR_EQUAL(0, lookup(SIMPLE_PORT_NAME, &ofil, &odev));

	TEST_ASSERT_EQUAL(0, portUnregister(SIMPLE_PORT_NAME));

	TEST_ASSERT_LESS_THAN(0, lookup(SIMPLE_PORT_NAME, &ofil, &odev));

	TEST_ASSERT_EQUAL(-ENOENT, portUnregister(SIMPLE_PORT_NAME));
}

TEST(test_register, register_existing)
{
	const id_t id = 0;
	oid_t oid = { .port = port, .id = id };
	TEST_PORT_REGISTER(SIMPLE_PORT_NAME, oid);

	TEST_ASSERT_EQUAL(-EEXIST, portRegister(port, SIMPLE_PORT_NAME, &oid));

	TEST_ASSERT_EQUAL(0, portUnregister(SIMPLE_PORT_NAME));

	TEST_ASSERT_EQUAL(-ENOENT, portUnregister(SIMPLE_PORT_NAME));
}

TEST(test_register, register_many)
{
	static struct {
		char name[DEVNAMSIZ + 1];
		oid_t oid;
	} devs[NDEVS];

	char firstLetter = 'a';
	oid_t ofil, odev;

	for (int i = 0; i < NDEVS; i++) {
		/* names will follow pattern: aa, ab, ..., ba, bb, ... */

		if (i != 0 && i % LETTERS_IN_ALPHABET == 0) {
			firstLetter++;
		}

		char currName[3];
		currName[0] = firstLetter;
		currName[1] = (i % LETTERS_IN_ALPHABET) + 'a';
		currName[2] = '\0';
		memcpy(devs[i].name, currName, sizeof(currName));

		devs[i].oid.port = port;
		devs[i].oid.id = currName[0] + (LETTERS_IN_ALPHABET * currName[1]);

		TEST_PORT_REGISTER(devs[i].name, devs[i].oid);
	}

	for (int i = 0; i < NDEVS; i++) {
		TEST_ASSERT_GREATER_OR_EQUAL_MESSAGE(0, lookup(devs[i].name, &ofil, &odev), devs[i].name);
	}

	for (int i = 0; i < NDEVS; i++) {
		TEST_ASSERT_EQUAL_MESSAGE(0, portUnregister(devs[i].name), devs[i].name);
		TEST_ASSERT_LESS_THAN_MESSAGE(0, lookup(devs[i].name, &ofil, &odev), devs[i].name);
	}
}

TEST(test_register, bad_port)
{
	const id_t id = 0;
	oid_t oid = { .port = port, .id = id }, odev;

	/* make sure this port doesn't exist by destroying it */
	portDestroy(port);

	/* port should still be registered */
	TEST_ASSERT_EQUAL(0, portRegister(port, SIMPLE_PORT_NAME, &oid));
	TEST_ASSERT_GREATER_OR_EQUAL(0, lookup(SIMPLE_PORT_NAME, NULL, &odev));

	/* re-create the port */
	TEST_ASSERT_GREATER_OR_EQUAL(0, portCreate(&port));
}

TEST_GROUP_RUNNER(test_register)
{
	RUN_TEST_CASE(test_register, simple);
	RUN_TEST_CASE(test_register, register_existing);
	RUN_TEST_CASE(test_register, register_many);
	RUN_TEST_CASE(test_register, bad_port);
}

static void runner(void)
{
	RUN_TEST_GROUP(test_register);
}

int main(int argc, char *argv[])
{
	int err = portCreate(&port);
	if (err < 0) {
		fprintf(stderr, "Could not create port: %s (%d)\n", strerror(-err), -err);
		return EXIT_FAILURE;
	}
	err = (UnityMain(argc, (const char **)argv, runner) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
	portDestroy(port);
	return err;
}
