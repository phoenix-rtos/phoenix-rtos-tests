/*
 * Phoenix-RTOS
 *
 * SPWRTR driver tests
 *
 * Copyright 2025 Phoenix Systems
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#include <board_config.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/platform.h>


#include <grspwrtr.h>
#include <unity_fixture.h>

#define TEST_SPWRTR_PATH0 "/dev/spwrtr0"


/* helper functions */


static oid_t test_getOid(const char *path)
{
	oid_t oid;
	while (lookup(path, NULL, &oid) < 0) {
		usleep(10000);
	}

	return oid;
}


TEST_GROUP(test_spwrtr);


TEST_SETUP(test_spwrtr)
{
	oid_t oid = test_getOid(TEST_SPWRTR_PATH0);
	msg_t msg = {
		.type = mtDevCtl,
		.i = { .data = NULL, .size = 0 },
		.o = { .data = NULL, .size = 0 },
		.oid.id = 0,
		.oid.port = oid.port,
	};
	spwrtr_t *ictl = (spwrtr_t *)msg.i.raw;

	ictl->type = spwrtr_reset;

	TEST_ASSERT_EQUAL_INT(0, msgSend(oid.port, &msg));
	TEST_ASSERT_EQUAL_INT(0, msg.o.err);
}


TEST_TEAR_DOWN(test_spwrtr)
{
}


TEST(test_spwrtr, spwrtrGetMapping)
{
	/* after reset, each PHY port has bits set only at its port number; all other are cleared */
	struct {
		uint8_t port;
		uint32_t expected;
	} tests[] = {
		{
			.port = 1u,
			.expected = 1u << 1,
		},
		{
			.port = 2u,
			.expected = 1u << 2,
		},
		{
			.port = 3u,
			.expected = 1u << 3,
		},
		{
			.port = 4u,
			.expected = 1u << 4,
		}
	};

	oid_t oid = test_getOid(TEST_SPWRTR_PATH0);
	msg_t msg = {
		.type = mtDevCtl,
		.i = { .data = NULL, .size = 0 },
		.o = { .data = NULL, .size = 0 },
		.oid.id = 0,
		.oid.port = oid.port,
	};
	spwrtr_t *ictl = (spwrtr_t *)msg.i.raw;

	for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
		char info[19];
		snprintf(info, 19, "Test nr %zu failed", i);

		ictl->type = spwrtr_pmap_get;
		ictl->task.mapping.port = tests[i].port;

		TEST_ASSERT_EQUAL_INT_MESSAGE(0, msgSend(oid.port, &msg), info);
		TEST_ASSERT_EQUAL_INT_MESSAGE(0, msg.o.err, info);

		spwrtr_o_t *octl = (spwrtr_o_t *)msg.o.raw;
		TEST_ASSERT_EQUAL_UINT_MESSAGE(tests[i].expected, octl->val, info);
	}
}


TEST(test_spwrtr, spwrtrSetMapping)
{
	uint8_t port = 33u;
	uint32_t enPorts = 1u << 3;

	oid_t oid = test_getOid(TEST_SPWRTR_PATH0);
	msg_t msg = {
		.type = mtDevCtl,
		.i = { .data = NULL, .size = 0 },
		.o = { .data = NULL, .size = 0 },
		.oid.id = 0,
		.oid.port = oid.port,
	};
	spwrtr_t *ictl = (spwrtr_t *)msg.i.raw;

	ictl->type = spwrtr_pmap_set;
	ictl->task.mapping.port = port;
	ictl->task.mapping.enPorts = enPorts;

	TEST_ASSERT_EQUAL_INT(0, msgSend(oid.port, &msg));
	TEST_ASSERT_EQUAL_INT(0, msg.o.err);

	ictl->type = spwrtr_pmap_get;
	ictl->task.mapping.port = port;

	TEST_ASSERT_EQUAL_INT(0, msgSend(oid.port, &msg));
	TEST_ASSERT_EQUAL_INT(0, msg.o.err);

	spwrtr_o_t *octl = (spwrtr_o_t *)msg.o.raw;
	TEST_ASSERT_EQUAL_UINT(enPorts, octl->val);
}


TEST_GROUP_RUNNER(test_spwrtr)
{
	RUN_TEST_CASE(test_spwrtr, spwrtrGetMapping);
	RUN_TEST_CASE(test_spwrtr, spwrtrSetMapping);
}


void runner(void)
{
	RUN_TEST_GROUP(test_spwrtr);
}


int main(int argc, char *argv[])
{
	return (UnityMain(argc, (const char **)argv, runner) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
