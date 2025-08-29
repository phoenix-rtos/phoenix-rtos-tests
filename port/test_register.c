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
		oid_t _odev, _ofil; \
		TEST_ASSERT_EQUAL_MESSAGE(0, portRegister(_port, (name), &(oid)), (name)); \
		TEST_ASSERT_EQUAL_MESSAGE(_port, (oid).port, (name)); \
		TEST_ASSERT_EQUAL_MESSAGE(_id, (oid).id, (name)); \
		TEST_ASSERT_EQUAL_MESSAGE(0, lookup(name, &_ofil, &_odev), name); \
	} while (0)


static uint32_t port;


TEST_GROUP(test_register);

TEST_SETUP(test_register)
{
}

TEST_TEAR_DOWN(test_register)
{
	/* in any case, unregister the SIMPLE_PORT_NAME */
	portUnregister(SIMPLE_PORT_NAME);
}

TEST(test_register, simple)
{
	const id_t id = 0;
	oid_t oid = { .port = port, .id = id }, ofil, odev;
	TEST_PORT_REGISTER(SIMPLE_PORT_NAME, oid);

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

TEST(test_register, bad_port)
{
	/* check if we can register a port with an invalid oid. */
	/* TODO: revise if this behavior is the correct one */
	const id_t id = 0;
	oid_t oid = { .port = port, .id = id }, odev;

	/* make sure this port doesn't exist by destroying it */
	portDestroy(port);

	/* port should still be registered */
	TEST_ASSERT_EQUAL(0, portRegister(port, SIMPLE_PORT_NAME, &oid));
	TEST_ASSERT_EQUAL(0, lookup(SIMPLE_PORT_NAME, NULL, &odev));

	/* re-create the port */
	TEST_ASSERT_GREATER_OR_EQUAL(0, portCreate(&port));
}


static struct {
	char name[DEVNAMSIZ + 1];
	oid_t oid;
} devs[NDEVS];


TEST_GROUP(test_register_many);

TEST_SETUP(test_register_many)
{
	/* prepare names and oids */
	for (int i = 0; i < NDEVS; i++) {
		/* names will follow pattern: aa, ab, ..., ba, bb, ... */
		devs[i].name[0] = 'a' + (i / LETTERS_IN_ALPHABET);
		devs[i].name[1] = 'a' + (i % LETTERS_IN_ALPHABET);
		devs[i].name[2] = '\0';
		devs[i].oid.port = port;
		devs[i].oid.id = i;
	}
}

TEST_TEAR_DOWN(test_register_many)
{
	/* unregister if not already unregistered */
	for (int i = 0; i < NDEVS; i++) {
		portUnregister(devs[i].name);
	}
}


TEST(test_register_many, register_many)
{
	oid_t ofil, odev;

	for (int i = 0; i < NDEVS; i++) {
		TEST_PORT_REGISTER(devs[i].name, devs[i].oid);
		TEST_ASSERT_EQUAL_MESSAGE(0, portUnregister(devs[i].name), devs[i].name);
		TEST_ASSERT_LESS_THAN_MESSAGE(0, lookup(devs[i].name, &ofil, &odev), devs[i].name);
	}
}

TEST_GROUP_RUNNER(test_register)
{
	RUN_TEST_CASE(test_register, simple);
	RUN_TEST_CASE(test_register, register_existing);
	RUN_TEST_CASE(test_register, bad_port);
}

TEST_GROUP_RUNNER(test_register_many)
{
	RUN_TEST_CASE(test_register_many, register_many);
}

static void runner(void)
{
	RUN_TEST_GROUP(test_register);
	RUN_TEST_GROUP(test_register_many);
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
