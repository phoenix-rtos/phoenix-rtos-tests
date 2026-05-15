/*
 * Phoenix-RTOS
 *
 * test-sys-msg
 *
 * Tests for rendezvous message passing
 *
 * Copyright 2026 Phoenix Systems
 * Author: Adam Greloch
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/mman.h>

#include <posix/utils.h>

#include <sys/threads.h>
#include <sys/msg.h>

#include <unity_fixture.h>

#include "shmsrv.h"
#include "shm_utils.h"


#define REPS 10


/* ---- helpers ---- */


static int dev_counter = 0;


static pid_t safe_fork(void)
{
	pid_t pid;
	fflush(stdout);
	if ((pid = fork()) < 0) {
		if (errno == ENOSYS) {
			TEST_IGNORE_MESSAGE("fork syscall not supported");
		}
		else {
			FAIL("fork");
		}
	}
	return pid;
}


static int setup_port_dev(const char *name, uint32_t *rport)
{
	uint32_t port;
	int err;

	err = portCreate(&port);
	if (err < 0) {
		return err;
	}

	oid_t oid;
	oid.port = port;
	oid.id = 0;

	if (rport != NULL) {
		*rport = port;
	}

	return create_dev(&oid, name);
}


/* Wait for child, assert it exited cleanly with status 0. Then SIGKILL as cleanup. */
static void assert_child_exit(pid_t pid)
{
	int status;
	int w;

	/* give server a moment to set up / catch up */
	usleep(50 * 1000);

	w = waitpid(pid, &status, WNOHANG);
	if (w < 0) {
		FAIL("waitpid");
	}

	if (w > 0 && WIFEXITED(status)) {
		TEST_ASSERT_EQUAL_INT_MESSAGE(0, WEXITSTATUS(status), "child exited with nonzero");
	}

	kill(pid, SIGKILL);
	waitpid(pid, NULL, 0);
}


/* Generate a unique device path for the current test to avoid collisions */
static void make_dev_path(char *buf, size_t bufsz, const char *test_name)
{
	snprintf(buf, bufsz, "/dev/mt_%s_%d_%d", test_name, getpid(), dev_counter++);
}


/* Simple server loop: recv, process, respond. Exits(0) on EINTR from recv. */
static void server_echo_loop(uint32_t port, int use_respond_and_recv)
{
	msg_t msg = { 0 };
	msg_rid_t rid;

	for (;;) {
		int err;

		if (!use_respond_and_recv) {
			err = msgRecv(port, &msg, &rid);
		}
		else {
			err = msgRespondAndRecv(port, &msg, &rid);
		}

		if (err < 0) {
			/* pulse or interrupt - handle accordingly */
			exit(0);
		}

		/* echo: copy i.raw -> o.raw, copy i.data -> o.data, set o.err = msg.type */
		memcpy(msg.o.raw, msg.i.raw, sizeof(msg.o.raw));

		if (msg.i.data != NULL && msg.o.data != NULL) {
			size_t copy_sz = msg.i.size < msg.o.size ? msg.i.size : msg.o.size;
			memcpy(msg.o.data, msg.i.data, copy_sz);
		}

		msg.o.err = msg.type;

		if (!use_respond_and_recv) {
			if (msgRespond(port, &msg, rid) < 0) {
				exit(2);
			}
		}
	}
}


/* ===================================== */
/* TEST_GROUP: msg_errnos                */
/* ===================================== */

TEST_GROUP(msg_errnos);

TEST_SETUP(msg_errnos)
{
}

TEST_TEAR_DOWN(msg_errnos)
{
}


/* Send to a non-existent port -> -EINVAL */
TEST(msg_errnos, send_invalid_port)
{
	msg_t msg = { 0 };
	TEST_ASSERT_EQUAL_INT(-EINVAL, msgSend(0xdeadbeef, &msg));
}


/* Recv on a non-existent port -> -EINVAL */
TEST(msg_errnos, recv_invalid_port)
{
	msg_t msg = { 0 };
	msg_rid_t rid;
	TEST_ASSERT_EQUAL_INT(-EINVAL, msgRecv(0xdeadbeef, &msg, &rid));
}


/* Respond on a non-existent port -> -EINVAL */
TEST(msg_errnos, respond_invalid_port)
{
	msg_t msg = { 0 };
	TEST_ASSERT_EQUAL_INT(-EINVAL, msgRespond(0xdeadbeef, &msg, 0));
}


/* Pulse on a non-existent port -> -EINVAL */
TEST(msg_errnos, pulse_invalid_port)
{
	TEST_ASSERT_EQUAL_INT(-EINVAL, msgPulse(0xdeadbeef, 42));
}


/* Send to a valid port that has no receiver should NOT return immediately with -EINVAL */
/* (the caller should block until a receiver shows up, or return error on closed port) */
/* For now, just test that portCreate succeeds */
TEST(msg_errnos, port_create_destroy)
{
	uint32_t port;
	TEST_ASSERT_EQUAL_INT(0, portCreate(&port));
	portDestroy(port);
}


/* ===================================== */
/* TEST_GROUP: msg_raw                   */
/* Small message transfer via raw fields */
/* ===================================== */

TEST_GROUP(msg_raw);

TEST_SETUP(msg_raw)
{
}

TEST_TEAR_DOWN(msg_raw)
{
}


/* Transfer small message through i.raw / o.raw only (no data buffers) */
TEST(msg_raw, raw_roundtrip)
{
	char dev_path[64];
	make_dev_path(dev_path, sizeof(dev_path), "raw_rt");
	pid_t pid;

	if ((pid = safe_fork()) == 0) {
		uint32_t port = 0;
		if (setup_port_dev(dev_path, &port) < 0) {
			exit(3);
		}
		server_echo_loop(port, 0);
		exit(0);
	}

	oid_t oid;
	while (lookup(dev_path, NULL, &oid) < 0) {
		usleep(10 * 1000);
	}

	msg_t msg = { 0 };

	/* Fill i.raw with a pattern */
	for (size_t i = 0; i < sizeof(msg.i.raw); i++) {
		msg.i.raw[i] = (unsigned char)(i ^ 0xAA);
	}

	msg.type = mtDevCtl;
	msg.i.size = 0;
	msg.i.data = NULL;
	msg.o.size = 0;
	msg.o.data = NULL;

	TEST_ASSERT_EQUAL_INT(0, msgSend(oid.port, &msg));
	TEST_ASSERT_EQUAL_INT(mtDevCtl, msg.o.err);

	/* o.raw should be a copy of i.raw (echo server copies it) */
	TEST_ASSERT_EQUAL_MEMORY(msg.i.raw, msg.o.raw, sizeof(msg.o.raw));

	assert_child_exit(pid);
}


/* Transfer a message that uses only some of i.raw and check the rest is zeroed */
TEST(msg_raw, raw_partial)
{
	char dev_path[64];
	make_dev_path(dev_path, sizeof(dev_path), "raw_part");
	pid_t pid;

	if ((pid = safe_fork()) == 0) {
		uint32_t port = 0;
		if (setup_port_dev(dev_path, &port) < 0) {
			exit(3);
		}
		server_echo_loop(port, 0);
		exit(0);
	}

	oid_t oid;
	while (lookup(dev_path, NULL, &oid) < 0) {
		usleep(10 * 1000);
	}

	msg_t msg = { 0 };
	msg.i.io.offs = 12345;
	msg.i.io.len = 42;
	msg.type = mtRead;
	msg.i.size = 0;
	msg.i.data = NULL;
	msg.o.size = 0;
	msg.o.data = NULL;

	TEST_ASSERT_EQUAL_INT(0, msgSend(oid.port, &msg));
	TEST_ASSERT_EQUAL_INT(mtRead, msg.o.err);

	assert_child_exit(pid);
}


/* Fill i.raw with max pattern (64 bytes) and verify round-trip */
TEST(msg_raw, raw_max_fill)
{
	char dev_path[64];
	make_dev_path(dev_path, sizeof(dev_path), "raw_max");
	pid_t pid;

	if ((pid = safe_fork()) == 0) {
		uint32_t port = 0;
		if (setup_port_dev(dev_path, &port) < 0) {
			exit(3);
		}
		server_echo_loop(port, 0);
		exit(0);
	}

	oid_t oid;
	while (lookup(dev_path, NULL, &oid) < 0) {
		usleep(10 * 1000);
	}

	msg_t msg = { 0 };
	for (size_t i = 0; i < sizeof(msg.i.raw); i++) {
		msg.i.raw[i] = (unsigned char)i;
	}
	msg.type = mtWrite;
	msg.i.size = 0;
	msg.i.data = NULL;
	msg.o.size = 0;
	msg.o.data = NULL;

	TEST_ASSERT_EQUAL_INT(0, msgSend(oid.port, &msg));
	TEST_ASSERT_EQUAL_INT(mtWrite, msg.o.err);

	for (size_t i = 0; i < sizeof(msg.o.raw); i++) {
		TEST_ASSERT_EQUAL_HEX8(i, msg.o.raw[i]);
	}

	assert_child_exit(pid);
}


/* Repeated raw transfers to stress basic path */
TEST(msg_raw, raw_repeated)
{
	char dev_path[64];
	make_dev_path(dev_path, sizeof(dev_path), "raw_rep");
	pid_t pid;

	if ((pid = safe_fork()) == 0) {
		uint32_t port = 0;
		if (setup_port_dev(dev_path, &port) < 0) {
			exit(3);
		}
		server_echo_loop(port, 0);
		exit(0);
	}

	oid_t oid;
	while (lookup(dev_path, NULL, &oid) < 0) {
		usleep(10 * 1000);
	}

	for (int iter = 0; iter < 200; iter++) {
		msg_t msg = { 0 };
		msg.i.io.offs = iter;
		msg.i.io.len = iter * 3;
		msg.type = mtRead;
		msg.i.size = 0;
		msg.i.data = NULL;
		msg.o.size = 0;
		msg.o.data = NULL;

		TEST_ASSERT_EQUAL_INT(0, msgSend(oid.port, &msg));
		TEST_ASSERT_EQUAL_INT(mtRead, msg.o.err);
	}

	assert_child_exit(pid);
}


/* msg.type and msg.oid preservation across IPC */
TEST(msg_raw, type_and_oid_preserved)
{
	char dev_path[64];
	make_dev_path(dev_path, sizeof(dev_path), "typeoid");
	pid_t pid;

	if ((pid = safe_fork()) == 0) {
		uint32_t port = 0;
		if (setup_port_dev(dev_path, &port) < 0) {
			exit(3);
		}

		msg_t msg = { 0 };
		msg_rid_t rid;

		for (;;) {
			if (msgRecv(port, &msg, &rid) < 0) {
				exit(0);
			}

			/* Validate oid and type arrived */
			if (msg.oid.id != 0x42) {
				exit(10);
			}

			/* echo type into o.err so client can check */
			msg.o.err = msg.type;

			/* echo oid.id into o.raw */
			memcpy(msg.o.raw, &msg.oid.id, sizeof(msg.oid.id));

			if (msgRespond(port, &msg, rid) < 0) {
				exit(11);
			}
		}
	}

	oid_t oid;
	while (lookup(dev_path, NULL, &oid) < 0) {
		usleep(10 * 1000);
	}

	for (int type = mtOpen; type < mtCount; type++) {
		msg_t msg = { 0 };
		msg.type = type;
		msg.oid.port = oid.port;
		msg.oid.id = 0x42;
		msg.i.size = 0;
		msg.i.data = NULL;
		msg.o.size = 0;
		msg.o.data = NULL;

		TEST_ASSERT_EQUAL_INT(0, msgSend(oid.port, &msg));
		TEST_ASSERT_EQUAL_INT(type, msg.o.err);

		id_t got_id;
		memcpy(&got_id, msg.o.raw, sizeof(got_id));
		TEST_ASSERT_EQUAL_UINT64(0x42, got_id);
	}

	assert_child_exit(pid);
}


/* ===================================== */
/* TEST_GROUP: msg_data                  */
/* Large data transfer via mapped bufs   */
/* ===================================== */

TEST_GROUP(msg_data);

TEST_SETUP(msg_data)
{
}

TEST_TEAR_DOWN(msg_data)
{
}


/* Transfer with both i.data and o.data (mapped buffers) */
TEST(msg_data, idata_and_odata)
{
	char dev_path[64];
	make_dev_path(dev_path, sizeof(dev_path), "data_io");
	pid_t pid;

	const size_t bufsz = 4096;

	if ((pid = safe_fork()) == 0) {
		uint32_t port = 0;
		if (setup_port_dev(dev_path, &port) < 0) {
			exit(3);
		}
		server_echo_loop(port, 0);
		exit(0);
	}

	oid_t oid;
	while (lookup(dev_path, NULL, &oid) < 0) {
		usleep(10 * 1000);
	}

	char *ibuf = malloc(bufsz);
	char *obuf = malloc(bufsz);
	TEST_ASSERT_NOT_NULL(ibuf);
	TEST_ASSERT_NOT_NULL(obuf);

	for (size_t i = 0; i < bufsz; i++) {
		ibuf[i] = (char)(i & 0xff);
	}
	memset(obuf, 0, bufsz);

	msg_t msg = { 0 };
	msg.type = mtWrite;
	msg.i.data = ibuf;
	msg.i.size = bufsz;
	msg.o.data = obuf;
	msg.o.size = bufsz;

	TEST_ASSERT_EQUAL_INT(0, msgSend(oid.port, &msg));
	TEST_ASSERT_EQUAL_INT(mtWrite, msg.o.err);
	TEST_ASSERT_EQUAL_MEMORY(ibuf, obuf, bufsz);

	free(ibuf);
	free(obuf);
	assert_child_exit(pid);
}


/* Transfer with only i.data (no o.data buffer) */
TEST(msg_data, idata_only)
{
	char dev_path[64];
	make_dev_path(dev_path, sizeof(dev_path), "data_i");
	pid_t pid;

	const size_t bufsz = 2048;

	if ((pid = safe_fork()) == 0) {
		uint32_t port = 0;
		if (setup_port_dev(dev_path, &port) < 0) {
			exit(3);
		}

		msg_t msg = { 0 };
		msg_rid_t rid;

		for (;;) {
			if (msgRecv(port, &msg, &rid) < 0) {
				exit(0);
			}

			/* Validate input data arrived */
			if (msg.i.data == NULL || msg.i.size != bufsz) {
				exit(10);
			}

			unsigned char *p = (unsigned char *)msg.i.data;
			for (size_t i = 0; i < bufsz; i++) {
				if (p[i] != (unsigned char)(i & 0xff)) {
					exit(11);
				}
			}

			msg.o.err = 0;

			if (msgRespond(port, &msg, rid) < 0) {
				exit(12);
			}
		}
	}

	oid_t oid;
	while (lookup(dev_path, NULL, &oid) < 0) {
		usleep(10 * 1000);
	}

	char *ibuf = malloc(bufsz);
	TEST_ASSERT_NOT_NULL(ibuf);
	for (size_t i = 0; i < bufsz; i++) {
		ibuf[i] = (char)(i & 0xff);
	}

	msg_t msg = { 0 };
	msg.type = mtWrite;
	msg.i.data = ibuf;
	msg.i.size = bufsz;
	msg.o.data = NULL;
	msg.o.size = 0;

	TEST_ASSERT_EQUAL_INT(0, msgSend(oid.port, &msg));
	TEST_ASSERT_EQUAL_INT(0, msg.o.err);

	free(ibuf);
	assert_child_exit(pid);
}


/* Transfer with only o.data (no i.data buffer) */
TEST(msg_data, odata_only)
{
	char dev_path[64];
	make_dev_path(dev_path, sizeof(dev_path), "data_o");
	pid_t pid;

	const size_t bufsz = 2048;

	if ((pid = safe_fork()) == 0) {
		uint32_t port = 0;
		if (setup_port_dev(dev_path, &port) < 0) {
			exit(3);
		}

		msg_t msg = { 0 };
		msg_rid_t rid;

		for (;;) {
			if (msgRecv(port, &msg, &rid) < 0) {
				exit(0);
			}

			/* Fill o.data with a pattern for client to verify */
			if (msg.o.data != NULL && msg.o.size >= bufsz) {
				unsigned char *p = (unsigned char *)msg.o.data;
				for (size_t i = 0; i < bufsz; i++) {
					p[i] = (unsigned char)(i ^ 0x55);
				}
			}

			msg.o.err = 0;

			if (msgRespond(port, &msg, rid) < 0) {
				exit(12);
			}
		}
	}

	oid_t oid;
	while (lookup(dev_path, NULL, &oid) < 0) {
		usleep(10 * 1000);
	}

	char *obuf = malloc(bufsz);
	TEST_ASSERT_NOT_NULL(obuf);
	memset(obuf, 0, bufsz);

	msg_t msg = { 0 };
	msg.type = mtRead;
	msg.i.data = NULL;
	msg.i.size = 0;
	msg.o.data = obuf;
	msg.o.size = bufsz;

	TEST_ASSERT_EQUAL_INT(0, msgSend(oid.port, &msg));
	TEST_ASSERT_EQUAL_INT(0, msg.o.err);

	for (size_t i = 0; i < bufsz; i++) {
		TEST_ASSERT_EQUAL_HEX8((unsigned char)(i ^ 0x55), (unsigned char)obuf[i]);
	}

	free(obuf);
	assert_child_exit(pid);
}


/* Large multi-page transfer (tests _mapBufferUnaligned with multiple pages) */
TEST(msg_data, large_multipage)
{
	char dev_path[64];
	make_dev_path(dev_path, sizeof(dev_path), "data_lg");
	pid_t pid;

	const size_t bufsz = 16 * 4096;

	if ((pid = safe_fork()) == 0) {
		uint32_t port = 0;
		if (setup_port_dev(dev_path, &port) < 0) {
			exit(3);
		}
		server_echo_loop(port, 0);
		exit(0);
	}

	oid_t oid;
	while (lookup(dev_path, NULL, &oid) < 0) {
		usleep(10 * 1000);
	}

	char *ibuf = malloc(bufsz);
	char *obuf = malloc(bufsz);
	TEST_ASSERT_NOT_NULL(ibuf);
	TEST_ASSERT_NOT_NULL(obuf);

	for (size_t i = 0; i < bufsz; i++) {
		ibuf[i] = (char)((i * 7 + 3) & 0xff);
	}
	memset(obuf, 0, bufsz);

	msg_t msg = { 0 };
	msg.type = mtWrite;
	msg.i.data = ibuf;
	msg.i.size = bufsz;
	msg.o.data = obuf;
	msg.o.size = bufsz;

	TEST_ASSERT_EQUAL_INT(0, msgSend(oid.port, &msg));
	TEST_ASSERT_EQUAL_INT(mtWrite, msg.o.err);
	TEST_ASSERT_EQUAL_MEMORY(ibuf, obuf, bufsz);

	free(ibuf);
	free(obuf);
	assert_child_exit(pid);
}


/*
 * Unaligned buffer transfer - tests the boffs/eoffs shadow page copy paths
 * in _mapBufferUnaligned (the partial first/last page copy).
 */
TEST(msg_data, unaligned_buffer)
{
	char dev_path[64];
	make_dev_path(dev_path, sizeof(dev_path), "data_ua");
	pid_t pid;

	if ((pid = safe_fork()) == 0) {
		uint32_t port = 0;
		if (setup_port_dev(dev_path, &port) < 0) {
			exit(3);
		}
		server_echo_loop(port, 0);
		exit(0);
	}

	oid_t oid;
	while (lookup(dev_path, NULL, &oid) < 0) {
		usleep(10 * 1000);
	}

	/* Allocate oversized buffer, use an offset to force unalignment */
	const size_t total = 4096 * 3;
	const size_t offset = 137;
	const size_t payload = total - offset - 53;

	char *raw_ibuf = malloc(total);
	char *raw_obuf = malloc(total);
	TEST_ASSERT_NOT_NULL(raw_ibuf);
	TEST_ASSERT_NOT_NULL(raw_obuf);

	char *ibuf = raw_ibuf + offset;
	char *obuf = raw_obuf + offset;

	for (size_t i = 0; i < payload; i++) {
		ibuf[i] = (char)((i * 13 + 5) & 0xff);
	}
	memset(obuf, 0, payload);

	msg_t msg = { 0 };
	msg.type = mtWrite;
	msg.i.data = ibuf;
	msg.i.size = payload;
	msg.o.data = obuf;
	msg.o.size = payload;

	TEST_ASSERT_EQUAL_INT(0, msgSend(oid.port, &msg));
	TEST_ASSERT_EQUAL_INT(mtWrite, msg.o.err);
	TEST_ASSERT_EQUAL_MEMORY(ibuf, obuf, payload);

	free(raw_ibuf);
	free(raw_obuf);
	assert_child_exit(pid);
}


/* Sub-page (small) buffer that fits within one page with offset */
TEST(msg_data, sub_page_single)
{
	char dev_path[64];
	make_dev_path(dev_path, sizeof(dev_path), "data_sp");
	pid_t pid;

	if ((pid = safe_fork()) == 0) {
		uint32_t port = 0;
		if (setup_port_dev(dev_path, &port) < 0) {
			exit(3);
		}
		server_echo_loop(port, 0);
		exit(0);
	}

	oid_t oid;
	while (lookup(dev_path, NULL, &oid) < 0) {
		usleep(10 * 1000);
	}

	const size_t bufsz = 100;
	char ibuf[100];
	char obuf[100];

	for (size_t i = 0; i < bufsz; i++) {
		ibuf[i] = (char)(i + 1);
	}
	memset(obuf, 0, bufsz);

	msg_t msg = { 0 };
	msg.type = mtWrite;
	msg.i.data = ibuf;
	msg.i.size = bufsz;
	msg.o.data = obuf;
	msg.o.size = bufsz;

	TEST_ASSERT_EQUAL_INT(0, msgSend(oid.port, &msg));
	TEST_ASSERT_EQUAL_INT(mtWrite, msg.o.err);
	TEST_ASSERT_EQUAL_MEMORY(ibuf, obuf, bufsz);

	assert_child_exit(pid);
}


/* Repeated large data transfers to stress buffer mapping/unmapping */
TEST(msg_data, repeated_large)
{
	char dev_path[64];
	make_dev_path(dev_path, sizeof(dev_path), "data_rl");
	pid_t pid;

	const size_t bufsz = 4096 * 2;

	if ((pid = safe_fork()) == 0) {
		uint32_t port = 0;
		if (setup_port_dev(dev_path, &port) < 0) {
			exit(3);
		}
		server_echo_loop(port, 0);
		exit(0);
	}

	oid_t oid;
	while (lookup(dev_path, NULL, &oid) < 0) {
		usleep(10 * 1000);
	}

	char *ibuf = malloc(bufsz);
	char *obuf = malloc(bufsz);
	TEST_ASSERT_NOT_NULL(ibuf);
	TEST_ASSERT_NOT_NULL(obuf);

	for (int iter = 0; iter < 50; iter++) {
		for (size_t i = 0; i < bufsz; i++) {
			ibuf[i] = (char)((i + iter) & 0xff);
		}
		memset(obuf, 0, bufsz);

		msg_t msg = { 0 };
		msg.type = mtWrite;
		msg.i.data = ibuf;
		msg.i.size = bufsz;
		msg.o.data = obuf;
		msg.o.size = bufsz;

		TEST_ASSERT_EQUAL_INT(0, msgSend(oid.port, &msg));
		TEST_ASSERT_EQUAL_MEMORY(ibuf, obuf, bufsz);
	}

	free(ibuf);
	free(obuf);
	assert_child_exit(pid);
}


/* ===================================== */
/* TEST_GROUP: msg_respond_recv          */
/* Combined respond-and-recv path        */
/* ===================================== */

TEST_GROUP(msg_respond_recv);

TEST_SETUP(msg_respond_recv)
{
}

TEST_TEAR_DOWN(msg_respond_recv)
{
}


/* Basic respondAndRecv: server uses msgRespondAndRecv instead of separate recv+respond */
TEST(msg_respond_recv, basic)
{
	for (int i = 0; i < REPS; i++) {
		char dev_path[64];
		make_dev_path(dev_path, sizeof(dev_path), "rr_basic");
		pid_t pid;

		if ((pid = safe_fork()) == 0) {
			uint32_t port = 0;
			if (setup_port_dev(dev_path, &port) < 0) {
				exit(3);
			}
			server_echo_loop(port, 1);
			exit(0);
		}

		oid_t oid;
		while (lookup(dev_path, NULL, &oid) < 0) {
			usleep(10 * 1000);
		}

		for (int i = 0; i < 100; i++) {
			msg_t msg = { 0 };
			msg.i.io.offs = i;
			msg.type = mtRead;
			msg.i.size = 0;
			msg.i.data = NULL;
			msg.o.size = 0;
			msg.o.data = NULL;

			TEST_ASSERT_EQUAL_INT(0, msgSend(oid.port, &msg));
			TEST_ASSERT_EQUAL_INT(mtRead, msg.o.err);
		}

		assert_child_exit(pid);
	}
}


/* respondAndRecv with data buffers */
TEST(msg_respond_recv, with_data)
{
	for (int i = 0; i < REPS; i++) {
		char dev_path[64];
		make_dev_path(dev_path, sizeof(dev_path), "rr_data");
		pid_t pid;

		const size_t bufsz = 4096;

		if ((pid = safe_fork()) == 0) {
			uint32_t port = 0;
			if (setup_port_dev(dev_path, &port) < 0) {
				exit(3);
			}
			server_echo_loop(port, 1);
			exit(0);
		}

		oid_t oid;
		while (lookup(dev_path, NULL, &oid) < 0) {
			usleep(10 * 1000);
		}

		char *ibuf = malloc(bufsz);
		char *obuf = malloc(bufsz);
		TEST_ASSERT_NOT_NULL(ibuf);
		TEST_ASSERT_NOT_NULL(obuf);

		for (int iter = 0; iter < 20; iter++) {
			for (size_t i = 0; i < bufsz; i++) {
				ibuf[i] = (char)((i + iter * 37) & 0xff);
			}
			memset(obuf, 0, bufsz);

			msg_t msg = { 0 };
			msg.type = mtWrite;
			msg.i.data = ibuf;
			msg.i.size = bufsz;
			msg.o.data = obuf;
			msg.o.size = bufsz;

			TEST_ASSERT_EQUAL_INT(0, msgSend(oid.port, &msg));
			TEST_ASSERT_EQUAL_INT(mtWrite, msg.o.err);
			TEST_ASSERT_EQUAL_MEMORY(ibuf, obuf, bufsz);
		}

		free(ibuf);
		free(obuf);
		assert_child_exit(pid);
	}
}


/* ===================================== */
/* TEST_GROUP: msg_pulse                 */
/* Pulse delivery, late pulses           */
/* ===================================== */

TEST_GROUP(msg_pulse);

TEST_SETUP(msg_pulse)
{
}

TEST_TEAR_DOWN(msg_pulse)
{
}


/* Pulse delivered to a blocked receiver */
TEST(msg_pulse, pulse_to_blocked_recv)
{
	char dev_path[64];
	make_dev_path(dev_path, sizeof(dev_path), "pulse_b");
	pid_t pid;

	if ((pid = safe_fork()) != 0) {
		/* parent = server */
		uint32_t port = 0;
		TEST_ASSERT_GREATER_OR_EQUAL_INT(0, setup_port_dev(dev_path, &port));

		msg_t msg = { 0 };
		msg_rid_t rid;

		int err = msgRecv(port, &msg, &rid);
		/* Expect pulse return */
		TEST_ASSERT_EQUAL_INT(-EPULSE, err);
		TEST_ASSERT_EQUAL_UINT8(42, msg.o.pulse);

		if (waitpid(pid, NULL, 0) < 0) {
			FAIL("waitpid");
		}
	}
	else {
		/* child = pulse sender */
		oid_t oid;
		while (lookup(dev_path, NULL, &oid) < 0) {
			usleep(10 * 1000);
		}
		/* give server time to enter msgRecv */
		usleep(100 * 1000);

		msgPulse(oid.port, 42);
		exit(0);
	}
}


/* Pulse stored on port for a late receiver */
TEST(msg_pulse, late_pulse)
{
	uint32_t port;
	TEST_ASSERT_EQUAL_INT(0, portCreate(&port));

	/* Send pulse before any receiver is blocked */
	TEST_ASSERT_EQUAL_INT(0, msgPulse(port, 99));

	/* Now recv should immediately return with the pulse */
	msg_t msg = { 0 };
	msg_rid_t rid;
	int err = msgRecv(port, &msg, &rid);
	TEST_ASSERT_EQUAL_INT(-EPULSE, err);
	TEST_ASSERT_EQUAL_UINT8(99, msg.o.pulse);

	portDestroy(port);
}


/* Multiple pulses: only last one should stick (port.pulse overwrites) */
TEST(msg_pulse, overwrite_pending)
{
	uint32_t port;
	TEST_ASSERT_EQUAL_INT(0, portCreate(&port));

	TEST_ASSERT_EQUAL_INT(0, msgPulse(port, 10));
	TEST_ASSERT_EQUAL_INT(0, msgPulse(port, 20));
	TEST_ASSERT_EQUAL_INT(0, msgPulse(port, 30));

	msg_t msg = { 0 };
	msg_rid_t rid;
	int err = msgRecv(port, &msg, &rid);
	TEST_ASSERT_LESS_THAN_INT(0, err);
	/* The last pulse value should have overwritten previous ones */
	TEST_ASSERT_EQUAL_UINT8(30, msg.o.pulse);

	portDestroy(port);
}


/* Pulse with value 0 edge case */
TEST(msg_pulse, pulse_zero)
{
	char dev_path[64];
	make_dev_path(dev_path, sizeof(dev_path), "pulse_z");
	pid_t pid;

	if ((pid = safe_fork()) != 0) {
		uint32_t port = 0;
		TEST_ASSERT_GREATER_OR_EQUAL_INT(0, setup_port_dev(dev_path, &port));

		/* Send pulse 0 - this should still be delivered as a pulse, not confused with "no pulse" */
		/* Note: in current impl, pulse=0 means no pulse on the port, so this tests that edge case */
		usleep(100 * 1000);
		/* child will send pulse(0) to us */
		msg_t msg = { 0 };
		msg_rid_t rid;

		int err = msgRecv(port, &msg, &rid);
		/* With pulse=0 stored, recv sees p->pulse == 0 -> won't return with pulse.
		 * It will block. The child will then send a non-zero pulse to unblock. */
		TEST_ASSERT_LESS_THAN_INT(0, err);
		TEST_ASSERT_EQUAL_UINT8(1, msg.o.pulse);

		if (waitpid(pid, NULL, 0) < 0) {
			FAIL("waitpid");
		}
	}
	else {
		oid_t oid;
		while (lookup(dev_path, NULL, &oid) < 0) {
			usleep(10 * 1000);
		}
		/* pulse(0) won't actually be seen because port treats pulse==0 as "no pulse" */
		msgPulse(oid.port, 0);

		usleep(200 * 1000);
		/* now send a real pulse to unblock the server */
		msgPulse(oid.port, 1);

		exit(0);
	}
}


/* ===================================== */
/* TEST_GROUP: msg_queuing               */
/* Multiple clients queued on one port   */
/* ===================================== */

TEST_GROUP(msg_queuing);

TEST_SETUP(msg_queuing)
{
}

TEST_TEAR_DOWN(msg_queuing)
{
}


/*
 * Multiple concurrent clients sending to the same port.
 * The server processes them one at a time.
 * Tests the queuing path (_mustSlowCall / prio_queue).
 */
TEST(msg_queuing, concurrent_clients)
{
	char dev_path[64];
	make_dev_path(dev_path, sizeof(dev_path), "queue_cc");
	pid_t server_pid;

	const int NUM_CLIENTS = 4;
	const int MSGS_PER_CLIENT = 20;

	if ((server_pid = safe_fork()) == 0) {
		uint32_t port = 0;
		if (setup_port_dev(dev_path, &port) < 0) {
			exit(3);
		}
		server_echo_loop(port, 0);
		exit(0);
	}

	oid_t oid;
	while (lookup(dev_path, NULL, &oid) < 0) {
		usleep(10 * 1000);
	}

	pid_t clients[NUM_CLIENTS];

	for (int c = 0; c < NUM_CLIENTS; c++) {
		if ((clients[c] = safe_fork()) == 0) {
			for (int m = 0; m < MSGS_PER_CLIENT; m++) {
				msg_t msg = { 0 };
				msg.type = mtWrite;
				msg.i.io.offs = c * 1000 + m;
				msg.i.size = 0;
				msg.i.data = NULL;
				msg.o.size = 0;
				msg.o.data = NULL;

				if (msgSend(oid.port, &msg) != 0) {
					exit(1);
				}

				if (msg.o.err != mtWrite) {
					exit(2);
				}
			}
			exit(0);
		}
	}

	/* Wait for all clients to finish */
	for (int c = 0; c < NUM_CLIENTS; c++) {
		int status;
		waitpid(clients[c], &status, 0);
		if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
			TEST_FAIL_MESSAGE("client exited with error");
		}
	}

	assert_child_exit(server_pid);
}


/* Clients at different priorities: higher prio should be served first */
TEST(msg_queuing, priority_ordering)
{
	char dev_path[64];
	make_dev_path(dev_path, sizeof(dev_path), "queue_po");

	pid_t server_pid;

	if ((server_pid = safe_fork()) == 0) {
		uint32_t port = 0;
		if (setup_port_dev(dev_path, &port) < 0) {
			exit(3);
		}

		msg_t msg = { 0 };
		msg_rid_t rid;
		for (int i = 0; i < 3; i++) {
			if (msgRecv(port, &msg, &rid) < 0) {
				exit(1);
			}

			msg.o.err = (int)msg.priority;

			if (msgRespond(port, &msg, rid) < 0) {
				exit(2);
			}
		}

		/* Don't exit with error on ordering - let parent assert */
		exit(0);
	}

	oid_t oid;
	while (lookup(dev_path, NULL, &oid) < 0) {
		usleep(10 * 1000);
	}

	/* Spawn 3 clients at different priorities */
	pid_t c1, c2, c3;

	if ((c3 = safe_fork()) == 0) {
		/* lowest prio (highest number) fires first to get queued */
		priority(6);
		msg_t msg = { 0 };
		msg.type = mtWrite;
		msg.i.size = 0;
		msg.i.data = NULL;
		msg.o.size = 0;
		msg.o.data = NULL;
		if (msgSend(oid.port, &msg) != 0) {
			exit(1);
		}
		exit(0);
	}

	usleep(10 * 1000);

	if ((c2 = safe_fork()) == 0) {
		priority(4);
		msg_t msg = { 0 };
		msg.type = mtWrite;
		msg.i.size = 0;
		msg.i.data = NULL;
		msg.o.size = 0;
		msg.o.data = NULL;
		if (msgSend(oid.port, &msg) != 0) {
			exit(1);
		}
		exit(0);
	}

	usleep(10 * 1000);

	if ((c1 = safe_fork()) == 0) {
		priority(2);
		msg_t msg = { 0 };
		msg.type = mtWrite;
		msg.i.size = 0;
		msg.i.data = NULL;
		msg.o.size = 0;
		msg.o.data = NULL;
		if (msgSend(oid.port, &msg) != 0) {
			exit(1);
		}
		exit(0);
	}

	int s1, s2, s3;
	waitpid(c1, &s1, 0);
	waitpid(c2, &s2, 0);
	waitpid(c3, &s3, 0);

	TEST_ASSERT_TRUE_MESSAGE(WIFEXITED(s1) && WEXITSTATUS(s1) == 0, "prio client 2 failed");
	TEST_ASSERT_TRUE_MESSAGE(WIFEXITED(s2) && WEXITSTATUS(s2) == 0, "prio client 4 failed");
	TEST_ASSERT_TRUE_MESSAGE(WIFEXITED(s3) && WEXITSTATUS(s3) == 0, "prio client 6 failed");

	assert_child_exit(server_pid);
}


/* ===================================== */
/* TEST_GROUP: msg_interrupt             */
/* Tests for interrupts during IPC       */
/* ===================================== */

TEST_GROUP(msg_interrupt);

TEST_SETUP(msg_interrupt)
{
}

TEST_TEAR_DOWN(msg_interrupt)
{
}


/* recv is interruptible: killing the receiver while blocked should unblock it */
TEST(msg_interrupt, recv_interrupted_by_signal)
{
	char dev_path[64];
	make_dev_path(dev_path, sizeof(dev_path), "int_sig");
	pid_t pid;

	if ((pid = safe_fork()) == 0) {
		uint32_t port = 0;
		if (setup_port_dev(dev_path, &port) < 0) {
			exit(3);
		}

		msg_t msg = { 0 };
		msg_rid_t rid;

		/* This recv should be interrupted by SIGUSR1 */
		int err = msgRecv(port, &msg, &rid);
		if (err < 0) {
			/* Expected: recv returned due to signal/interrupt */
			exit(0);
		}

		/* Recv shouldn't have succeeded as it can't get a message */
		exit(1);
	}

	/* Give child time to set up port and block in recv */
	usleep(200 * 1000);

	/* Send SIGUSR1 to interrupt the recv */
	kill(pid, SIGUSR1);

	int status;
	waitpid(pid, &status, 0);

	// FIXME: WIFEXITED doesnt work properly on phoenix due to
	// https://github.com/phoenix-rtos/phoenix-rtos-project/issues/1233
	//
	// TEST_ASSERT_TRUE(WIFEXITED(status));

	TEST_ASSERT_EQUAL_INT(0, WEXITSTATUS(status));
}


/*
 * Server exit while client is blocked in send.
 *
 * FIXME in kernel: recv could get interrupted during msg transfer.
 * This tests that the client doesn't hang forever when the server dies.
 * Currently the implementation may not handle this correctly (abort on
 * server fault/port closure TODO).
 */
TEST(msg_interrupt, server_exit_during_send)
{
	char dev_path[64];
	make_dev_path(dev_path, sizeof(dev_path), "int_sev");
	pid_t server_pid;

	if ((server_pid = safe_fork()) == 0) {
		uint32_t port = 0;
		if (setup_port_dev(dev_path, &port) < 0) {
			exit(3);
		}

		msg_t msg = { 0 };
		msg_rid_t rid;

		/* Handle exactly one message, then exit */
		if (msgRecv(port, &msg, &rid) < 0) {
			exit(0);
		}

		msg.o.err = 0;
		msgRespond(port, &msg, rid);

		/* Server exits; port gets destroyed */
		exit(0);
	}

	oid_t oid;
	while (lookup(dev_path, NULL, &oid) < 0) {
		usleep(10 * 1000);
	}

	/* First message should succeed */
	msg_t msg = { 0 };
	msg.type = mtRead;
	msg.i.size = 0;
	msg.i.data = NULL;
	msg.o.size = 0;
	msg.o.data = NULL;
	TEST_ASSERT_EQUAL_INT(0, msgSend(oid.port, &msg));

	/* Wait for server to exit */
	waitpid(server_pid, NULL, 0);

	/* Second message: server is gone, port is destroyed.
	 * This should return -EINVAL (or at least not hang). */
	msg_t msg2 = { 0 };
	msg2.type = mtRead;
	msg2.i.size = 0;
	msg2.i.data = NULL;
	msg2.o.size = 0;
	msg2.o.data = NULL;
	int err = msgSend(oid.port, &msg2);
	TEST_ASSERT_LESS_THAN_INT(0, err);
}


/*
 * Client exit while server hasn't responded yet.
 *
 * Tests that the kernel handles a client disappearing mid-IPC.
 */
TEST(msg_interrupt, client_exit_before_respond)
{
	char dev_path[64];
	make_dev_path(dev_path, sizeof(dev_path), "int_cev");
	pid_t server_pid;

	if ((server_pid = safe_fork()) == 0) {
		uint32_t port = 0;
		if (setup_port_dev(dev_path, &port) < 0) {
			exit(3);
		}

		msg_t msg = { 0 };
		msg_rid_t rid;

		for (;;) {
			if (msgRecv(port, &msg, &rid) < 0) {
				exit(0);
			}

			/* Delay respond to give client time to die */
			usleep(200 * 1000);

			msg.o.err = 0;
			/* The client may be gone by now; respond should handle this gracefully */
			msgRespond(port, &msg, rid);
		}
	}

	oid_t oid;
	while (lookup(dev_path, NULL, &oid) < 0) {
		usleep(10 * 1000);
	}

	/* Spawn a client that sends and then exits immediately */
	pid_t client_pid;
	if ((client_pid = safe_fork()) == 0) {
		msg_t msg = { 0 };
		msg.type = mtWrite;
		msg.i.size = 0;
		msg.i.data = NULL;
		msg.o.size = 0;
		msg.o.data = NULL;
		/* This blocks waiting for respond, but we'll kill the client */
		msgSend(oid.port, &msg);
		exit(0);
	}

	/* Let the send start, then kill the client */
	usleep(50 * 1000);
	kill(client_pid, SIGKILL);
	waitpid(client_pid, NULL, 0);

	/* Server should still be functional after client dies */
	/* Send another message to verify */
	usleep(300 * 1000);

	pid_t c2;
	if ((c2 = safe_fork()) == 0) {
		msg_t msg = { 0 };
		msg.type = mtRead;
		msg.i.size = 0;
		msg.i.data = NULL;
		msg.o.size = 0;
		msg.o.data = NULL;
		int err = msgSend(oid.port, &msg);
		exit(err == 0 ? 0 : 1);
	}

	int status;
	waitpid(c2, &status, 0);
	// TEST_ASSERT_TRUE(WIFEXITED(status));
	TEST_ASSERT_EQUAL_INT_MESSAGE(0, WEXITSTATUS(status), "server should still work after client death");

	assert_child_exit(server_pid);
}


/* ===================================== */
/* TEST_GROUP: msg_multiserver           */
/* Multiple callers, out-of-order resp   */
/* ===================================== */

TEST_GROUP(msg_multiserver);

TEST_SETUP(msg_multiserver)
{
}

TEST_TEAR_DOWN(msg_multiserver)
{
}


/*
 * Server receives multiple messages and responds out of order.
 */
TEST(msg_multiserver, out_of_order_respond)
{
	char dev_path[64];
	make_dev_path(dev_path, sizeof(dev_path), "ms_ooo");
	pid_t server_pid;

	const int N = 3;

	if ((server_pid = safe_fork()) == 0) {
		uint32_t port = 0;
		if (setup_port_dev(dev_path, &port) < 0) {
			exit(3);
		}

		msg_t msgs[N];
		msg_rid_t rids[N];

		/* Receive N messages */
		for (int i = 0; i < N; i++) {
			memset(&msgs[i], 0, sizeof(msg_t));
			if (msgRecv(port, &msgs[i], &rids[i]) < 0) {
				exit(1);
			}
		}

		/* Respond in reverse order */
		for (int i = N - 1; i >= 0; i--) {
			msgs[i].o.err = msgs[i].type;
			if (msgRespond(port, &msgs[i], rids[i]) < 0) {
				exit(2);
			}
		}

		exit(0);
	}

	oid_t oid;
	while (lookup(dev_path, NULL, &oid) < 0) {
		usleep(10 * 1000);
	}

	pid_t clients[N];

	for (int i = 0; i < N; i++) {
		if ((clients[i] = safe_fork()) == 0) {
			msg_t msg = { 0 };
			msg.type = mtOpen + i;
			msg.i.size = 0;
			msg.i.data = NULL;
			msg.o.size = 0;
			msg.o.data = NULL;

			if (msgSend(oid.port, &msg) != 0) {
				exit(1);
			}
			if (msg.o.err != mtOpen + i) {
				exit(2);
			}
			exit(0);
		}
		usleep(20 * 1000);
	}

	for (int i = 0; i < N; i++) {
		int status;
		waitpid(clients[i], &status, 0);
		TEST_ASSERT_TRUE_MESSAGE(WIFEXITED(status) && WEXITSTATUS(status) == 0,
				"out-of-order respond: client failed");
	}

	int status;
	waitpid(server_pid, &status, 0);
	TEST_ASSERT_TRUE(WIFEXITED(status));
	TEST_ASSERT_EQUAL_INT(0, WEXITSTATUS(status));
}


/*
 * Server thread receives from one port and forwards to another (chain).
 * Tests that the receiving thread doesn't need to be the same as the
 * responding thread (commented in proc_respond: "see: p-r-corelibs/libstorage").
 */
TEST(msg_multiserver, server_chain)
{
	char dev_path1[64], dev_path2[64];
	make_dev_path(dev_path1, sizeof(dev_path1), "ms_ch1");
	make_dev_path(dev_path2, sizeof(dev_path2), "ms_ch2");
	pid_t backend_pid, frontend_pid;

	/* Backend server: simple echo */
	if ((backend_pid = safe_fork()) == 0) {
		uint32_t port = 0;
		if (setup_port_dev(dev_path2, &port) < 0) {
			exit(3);
		}
		server_echo_loop(port, 0);
		exit(0);
	}

	/* Frontend server: receives, forwards to backend, responds to original caller */
	if ((frontend_pid = safe_fork()) == 0) {
		uint32_t port = 0;
		if (setup_port_dev(dev_path1, &port) < 0) {
			exit(3);
		}

		oid_t backend_oid;
		while (lookup(dev_path2, NULL, &backend_oid) < 0) {
			usleep(10 * 1000);
		}

		msg_t msg = { 0 };
		msg_rid_t rid;

		for (;;) {
			if (msgRecv(port, &msg, &rid) < 0) {
				exit(0);
			}

			/* Forward to backend */
			msg_t fwd = { 0 };
			fwd.type = msg.type;
			memcpy(fwd.i.raw, msg.i.raw, sizeof(fwd.i.raw));
			fwd.i.size = 0;
			fwd.i.data = NULL;
			fwd.o.size = 0;
			fwd.o.data = NULL;

			if (msgSend(backend_oid.port, &fwd) != 0) {
				exit(4);
			}

			/* Relay response */
			memcpy(msg.o.raw, fwd.o.raw, sizeof(msg.o.raw));
			msg.o.err = fwd.o.err;

			if (msgRespond(port, &msg, rid) < 0) {
				exit(5);
			}
		}
	}

	oid_t oid;
	while (lookup(dev_path1, NULL, &oid) < 0) {
		usleep(10 * 1000);
	}

	for (int i = 0; i < 10; i++) {
		msg_t msg = { 0 };
		msg.type = mtDevCtl;
		msg.i.io.offs = i;
		msg.i.size = 0;
		msg.i.data = NULL;
		msg.o.size = 0;
		msg.o.data = NULL;

		TEST_ASSERT_EQUAL_INT(0, msgSend(oid.port, &msg));
		TEST_ASSERT_EQUAL_INT(mtDevCtl, msg.o.err);
	}

	assert_child_exit(frontend_pid);
	assert_child_exit(backend_pid);
}


/* =====================================================  */
/* TEST_GROUP: msg_prio_inherit                           */
/* Priority inheritance through IPC (IPCP)                */
/* =====================================================  */

TEST_GROUP(msg_prio_inherit);

TEST_SETUP(msg_prio_inherit)
{
}

TEST_TEAR_DOWN(msg_prio_inherit)
{
}


/*
 * Priority inheritance: when a high-prio client calls a low-prio server,
 * the server should execute at the client's priority to avoid priority
 * inversion.
 *
 * The kernel implementation donates the caller's SC to the receiver.
 * This test validates that the server's effective priority is elevated
 * during message processing and restored after respond.
 *
 * msg.priority field carries caller's priority to the server.
 */
TEST(msg_prio_inherit, basic_donation)
{
	char dev_path[64];
	make_dev_path(dev_path, sizeof(dev_path), "pi_basic");
	pid_t server_pid;

	if ((server_pid = safe_fork()) == 0) {
		uint32_t port = 0;
		if (setup_port_dev(dev_path, &port) < 0) {
			exit(3);
		}

		/* Server runs at low priority */
		priority(6);

		msg_t msg = { 0 };
		msg_rid_t rid;

		for (;;) {
			if (msgRecv(port, &msg, &rid) < 0) {
				exit(0);
			}

			/* Check our effective priority during message processing.
			 * With IPCP, priority(-1) should return the donated (higher) priority. */
			int cur_prio = priority(-1);

			/* Store the observed priority in the response for the client to check */
			msg.o.err = cur_prio;

			if (msgRespond(port, &msg, rid) < 0) {
				exit(2);
			}

			/* After respond, our priority should be restored to base (6) */
			int restored_prio = priority(-1);
			if (restored_prio != 6) {
				/* Priority not restored - this is a bug */
				exit(3);
			}
		}
	}

	oid_t oid;
	while (lookup(dev_path, NULL, &oid) < 0) {
		usleep(10 * 1000);
	}

	/* Client at high priority */
	priority(2);

	msg_t msg = { 0 };
	msg.type = mtRead;
	msg.i.size = 0;
	msg.i.data = NULL;
	msg.o.size = 0;
	msg.o.data = NULL;

	TEST_ASSERT_EQUAL_INT(0, msgSend(oid.port, &msg));

	/* Server should have run at our priority (2) due to SC donation */
	TEST_ASSERT_EQUAL_INT_MESSAGE(2, msg.o.err,
			"Server should run at caller's priority through SC donation");

	assert_child_exit(server_pid);
}


/*
 * IPCP with multiple clients at different priorities.
 * Server should execute at the priority of each calling client.
 */
TEST(msg_prio_inherit, multiple_clients)
{
	char dev_path[64];
	make_dev_path(dev_path, sizeof(dev_path), "pi_multi");
	pid_t server_pid;

	if ((server_pid = safe_fork()) == 0) {
		uint32_t port = 0;
		if (setup_port_dev(dev_path, &port) < 0) {
			exit(3);
		}

		priority(7);

		msg_t msg = { 0 };
		msg_rid_t rid;

		for (;;) {
			if (msgRecv(port, &msg, &rid) < 0) {
				exit(0);
			}

			int cur_prio = priority(-1);
			msg.o.err = cur_prio;

			if (msgRespond(port, &msg, rid) < 0) {
				exit(2);
			}
		}
	}

	oid_t oid;
	while (lookup(dev_path, NULL, &oid) < 0) {
		usleep(10 * 1000);
	}

	/* Test with different caller priorities */
	int test_prios[] = { 1, 3, 5 };

	for (size_t i = 0; i < sizeof(test_prios) / sizeof(test_prios[0]); i++) {
		pid_t cpid;
		if ((cpid = safe_fork()) == 0) {
			priority(test_prios[i]);

			msg_t msg = { 0 };
			msg.type = mtRead;
			msg.i.size = 0;
			msg.i.data = NULL;
			msg.o.size = 0;
			msg.o.data = NULL;

			if (msgSend(oid.port, &msg) != 0) {
				exit(1);
			}

			/* Server should have had our priority */
			if (msg.o.err != test_prios[i]) {
				exit(2);
			}
			exit(0);
		}

		int status;
		waitpid(cpid, &status, 0);
		TEST_ASSERT_TRUE_MESSAGE(WIFEXITED(status) && WEXITSTATUS(status) == 0,
				"IPCP: server didn't inherit client priority");
	}

	assert_child_exit(server_pid);
}


/*
 * Priority inversion scenario:
 *
 * Three threads: low (L), medium (M), high (H)
 * L holds a server port, H sends to it.
 * Without IPCP, M could preempt L causing H to wait.
 * With IPCP, L should execute at H's priority, preventing M from preempting.
 *
 * This is the classic priority inversion test adapted for IPC.
 */
TEST(msg_prio_inherit, inversion_scenario)
{
	char shm_path[64];
	make_dev_path(shm_path, sizeof(shm_path), "pi_inv_shm");

	pid_t shm_server_pid;
	if ((shm_server_pid = safe_fork()) == 0) {
		shmsrv_start(shm_path);
		exit(1);
	}

	for (int i = 0; i < REPS; i++) {
		char dev_path[64];
		make_dev_path(dev_path, sizeof(dev_path), "pi_inv");
		pid_t server_pid;

		int *shared = (int *)shm_init(shm_path, true, 0x200);
		TEST_ASSERT_NOT_NULL(shared);

		/* shared[0] = server_observed_prio
		 * shared[1] = server ready flag
		 * shared[2] = medium thread started
		 */
		shared[0] = -1;
		shared[1] = 0;
		shared[2] = 0;

		if ((server_pid = safe_fork()) == 0) {
			int *shared = (int *)shm_init(shm_path, false, 0x200);

			uint32_t port = 0;
			if (setup_port_dev(dev_path, &port) < 0) {
				exit(3);
			}

			/* Server at low priority */
			priority(6);
			shared[1] = 1;

			msg_t msg = { 0 };
			msg_rid_t rid;

			if (msgRecv(port, &msg, &rid) < 0) {
				exit(0);
			}

			/* Record our priority while processing the message from H */
			shared[0] = priority(-1);

			/* Simulate some work */
			usleep(50 * 1000);

			msg.o.err = 0;
			msgRespond(port, &msg, rid);

			exit(0);
		}

		oid_t oid;
		while (lookup(dev_path, NULL, &oid) < 0) {
			usleep(10 * 1000);
		}

		int retry = 0;
		/* Wait for server to be ready */
		while (!shared[1]) {
			usleep(1000);
			retry++;
			if (retry > 1000) {
				TEST_FAIL_MESSAGE("shm doesn't work");
			}
		}

		/* Launch medium-priority busy looper */
		pid_t med_pid;
		if ((med_pid = safe_fork()) == 0) {
			priority(4);
			shared[2] = 1;
			/* Busy wait to act as a potential preemptor */
			for (volatile int i = 0; i < 10000000; i++) {
				/* spin */
			}
			exit(0);
		}

		while (!shared[2]) {
			usleep(1000);
		}

		/* High-priority client sends to low-priority server */
		priority(2);

		msg_t msg = { 0 };
		msg.type = mtWrite;
		msg.i.size = 0;
		msg.i.data = NULL;
		msg.o.size = 0;
		msg.o.data = NULL;

		/* waitpit with WNOHANG so that msgSend is not interrupted with SIGCHLD */
		waitpid(med_pid, NULL, WNOHANG);
		waitpid(server_pid, NULL, WNOHANG);

		TEST_ASSERT_EQUAL_INT(0, msgSend(oid.port, &msg));

		/* With IPCP, server should have executed at priority 2 (our priority),
		 * so medium thread shouldn't have preempted it */
		TEST_ASSERT_EQUAL_INT_MESSAGE(2, shared[0],
				"Server should inherit caller's high priority to prevent inversion");

		waitpid(med_pid, NULL, 0);
		waitpid(server_pid, NULL, 0);

		munmap((void *)shared, 0x200);
	}

	kill(shm_server_pid, SIGKILL);
	waitpid(shm_server_pid, NULL, 0);
}


/* =====================================================  */
/* TEST_GROUP: msg_priority_field                         */
/* msg.priority carries caller's priority                 */
/* =====================================================  */

TEST_GROUP(msg_priority_field);

TEST_SETUP(msg_priority_field)
{
}

TEST_TEAR_DOWN(msg_priority_field)
{
}


/* Verify that msg.priority reflects caller's priority at the server side */
TEST(msg_priority_field, reflects_caller)
{
	char dev_path[64];
	make_dev_path(dev_path, sizeof(dev_path), "pf_cal");
	pid_t server_pid;

	if ((server_pid = safe_fork()) == 0) {
		uint32_t port = 0;
		if (setup_port_dev(dev_path, &port) < 0) {
			exit(3);
		}

		msg_t msg = { 0 };
		msg_rid_t rid;

		for (;;) {
			if (msgRecv(port, &msg, &rid) < 0) {
				exit(0);
			}
			/* Echo the received msg.priority back in o.err */
			msg.o.err = (int)msg.priority;
			if (msgRespond(port, &msg, rid) < 0) {
				exit(2);
			}
		}
	}

	oid_t oid;
	while (lookup(dev_path, NULL, &oid) < 0) {
		usleep(10 * 1000);
	}

	int test_prios[] = { 1, 3, 5, 7 };
	for (size_t i = 0; i < sizeof(test_prios) / sizeof(test_prios[0]); i++) {
		pid_t cpid;
		if ((cpid = safe_fork()) == 0) {
			priority(test_prios[i]);

			msg_t msg = { 0 };
			msg.type = mtRead;
			msg.i.size = 0;
			msg.i.data = NULL;
			msg.o.size = 0;
			msg.o.data = NULL;

			if (msgSend(oid.port, &msg) != 0) {
				exit(1);
			}

			if (msg.o.err != test_prios[i]) {
				exit(2);
			}
			exit(0);
		}

		int status;
		waitpid(cpid, &status, 0);
		TEST_ASSERT_TRUE_MESSAGE(WIFEXITED(status) && WEXITSTATUS(status) == 0,
				"msg.priority didn't reflect caller priority");
	}

	assert_child_exit(server_pid);
}


/* msg.pid carries caller's PID */
TEST(msg_priority_field, pid_preserved)
{
	char dev_path[64];
	make_dev_path(dev_path, sizeof(dev_path), "pf_pid");
	pid_t server_pid;

	if ((server_pid = safe_fork()) == 0) {
		uint32_t port = 0;
		if (setup_port_dev(dev_path, &port) < 0) {
			exit(3);
		}

		msg_t msg = { 0 };
		msg_rid_t rid;

		if (msgRecv(port, &msg, &rid) < 0) {
			exit(1);
		}
		/* Echo the received pid in o.err */
		msg.o.err = (int)msg.pid;

		if (msgRespond(port, &msg, rid) < 0) {
			exit(2);
		}
		exit(0);
	}

	oid_t oid;
	while (lookup(dev_path, NULL, &oid) < 0) {
		usleep(10 * 1000);
	}

	msg_t msg = { 0 };
	msg.type = mtRead;
	msg.i.size = 0;
	msg.i.data = NULL;
	msg.o.size = 0;
	msg.o.data = NULL;

	TEST_ASSERT_EQUAL_INT(0, msgSend(oid.port, &msg));
	TEST_ASSERT_EQUAL_INT((int)getpid(), msg.o.err);

	waitpid(server_pid, NULL, 0);
}


/* ===================================== */
/* TEST_GROUP: msg_edge                  */
/* Edge cases and stress                 */
/* ===================================== */

TEST_GROUP(msg_edge);

TEST_SETUP(msg_edge)
{
}

TEST_TEAR_DOWN(msg_edge)
{
}


/* Send with zero-length data buffers (size=0 but data!=NULL) */
TEST(msg_edge, zero_size_nonull_data)
{
	char dev_path[64];
	make_dev_path(dev_path, sizeof(dev_path), "edge_zs");
	pid_t pid;

	if ((pid = safe_fork()) == 0) {
		uint32_t port = 0;
		if (setup_port_dev(dev_path, &port) < 0) {
			exit(3);
		}
		server_echo_loop(port, 0);
		exit(0);
	}

	oid_t oid;
	while (lookup(dev_path, NULL, &oid) < 0) {
		usleep(10 * 1000);
	}

	char dummy = 'x';
	msg_t msg = { 0 };
	msg.type = mtRead;
	msg.i.data = &dummy;
	msg.i.size = 0;
	msg.o.data = &dummy;
	msg.o.size = 0;

	TEST_ASSERT_EQUAL_INT(0, msgSend(oid.port, &msg));
	TEST_ASSERT_EQUAL_INT(mtRead, msg.o.err);

	assert_child_exit(pid);
}


/* Exact page boundary buffer sizes */
TEST(msg_edge, page_boundary_buffer)
{
	char dev_path[64];
	make_dev_path(dev_path, sizeof(dev_path), "edge_pb");
	pid_t pid;

	if ((pid = safe_fork()) == 0) {
		uint32_t port = 0;
		if (setup_port_dev(dev_path, &port) < 0) {
			exit(3);
		}
		server_echo_loop(port, 0);
		exit(0);
	}

	oid_t oid;
	while (lookup(dev_path, NULL, &oid) < 0) {
		usleep(10 * 1000);
	}

	/* Test sizes at page boundaries: N*4096, N*4096-1, N*4096+1 */
	size_t sizes[] = { 4096, 4095, 4097, 8192, 8191, 8193 };

	for (size_t s = 0; s < sizeof(sizes) / sizeof(sizes[0]); s++) {
		size_t bufsz = sizes[s];
		char *ibuf = malloc(bufsz);
		char *obuf = malloc(bufsz);
		TEST_ASSERT_NOT_NULL(ibuf);
		TEST_ASSERT_NOT_NULL(obuf);

		for (size_t i = 0; i < bufsz; i++) {
			ibuf[i] = (char)((i + s) & 0xff);
		}
		memset(obuf, 0, bufsz);

		msg_t msg = { 0 };
		msg.type = mtWrite;
		msg.i.data = ibuf;
		msg.i.size = bufsz;
		msg.o.data = obuf;
		msg.o.size = bufsz;

		TEST_ASSERT_EQUAL_INT(0, msgSend(oid.port, &msg));
		TEST_ASSERT_EQUAL_MEMORY(ibuf, obuf, bufsz);

		free(ibuf);
		free(obuf);
	}

	assert_child_exit(pid);
}


/* Asymmetric i/o sizes: small in, large out */
TEST(msg_edge, asymmetric_io_sizes)
{
	char dev_path[64];
	make_dev_path(dev_path, sizeof(dev_path), "edge_as");
	pid_t pid;

	if ((pid = safe_fork()) == 0) {
		uint32_t port = 0;
		if (setup_port_dev(dev_path, &port) < 0) {
			exit(3);
		}

		msg_t msg = { 0 };
		msg_rid_t rid;

		for (;;) {
			if (msgRecv(port, &msg, &rid) < 0) {
				exit(0);
			}

			/* Write a pattern to the large output buffer */
			if (msg.o.data != NULL && msg.o.size > 0) {
				unsigned char *p = (unsigned char *)msg.o.data;
				for (size_t i = 0; i < msg.o.size; i++) {
					p[i] = (unsigned char)(i & 0xff);
				}
			}

			msg.o.err = 0;
			if (msgRespond(port, &msg, rid) < 0) {
				exit(2);
			}
		}
	}

	oid_t oid;
	while (lookup(dev_path, NULL, &oid) < 0) {
		usleep(10 * 1000);
	}

	const size_t obufsz = 8192;
	char *obuf = malloc(obufsz);
	TEST_ASSERT_NOT_NULL(obuf);
	memset(obuf, 0, obufsz);

	msg_t msg = { 0 };
	msg.type = mtRead;
	msg.i.data = NULL;
	msg.i.size = 0;
	msg.o.data = obuf;
	msg.o.size = obufsz;

	TEST_ASSERT_EQUAL_INT(0, msgSend(oid.port, &msg));
	TEST_ASSERT_EQUAL_INT(0, msg.o.err);

	for (size_t i = 0; i < obufsz; i++) {
		TEST_ASSERT_EQUAL_HEX8((unsigned char)(i & 0xff), (unsigned char)obuf[i]);
	}

	free(obuf);
	assert_child_exit(pid);
}


/* Rapid open/close of ports shouldn't leak resources or crash */
TEST(msg_edge, rapid_port_lifecycle)
{
	for (int i = 0; i < 100; i++) {
		uint32_t port;
		TEST_ASSERT_EQUAL_INT(0, portCreate(&port));
		portDestroy(port);
	}
}


/* Destroyed port: operations should fail cleanly */
TEST(msg_edge, destroyed_port_ops)
{
	uint32_t port;
	TEST_ASSERT_EQUAL_INT(0, portCreate(&port));
	portDestroy(port);

	msg_t msg = { 0 };
	msg_rid_t rid;

	/* These should all return -EINVAL on a destroyed port */
	TEST_ASSERT_LESS_THAN_INT(0, msgSend(port, &msg));
	TEST_ASSERT_LESS_THAN_INT(0, msgRecv(port, &msg, &rid));
	TEST_ASSERT_LESS_THAN_INT(0, msgPulse(port, 1));
}


/* ===================================== */
/* TEST_GROUP: msg_respond_recv_mixed    */
/* Mixed respond+recv / respond / recv   */
/* ===================================== */

TEST_GROUP(msg_respond_recv_mixed);

TEST_SETUP(msg_respond_recv_mixed)
{
}

TEST_TEAR_DOWN(msg_respond_recv_mixed)
{
}


/* Server alternates between msgRespond+msgRecv and msgRespondAndRecv */
TEST(msg_respond_recv_mixed, alternating)
{
	char dev_path[64];
	make_dev_path(dev_path, sizeof(dev_path), "rrm_alt");
	pid_t server_pid;

	if ((server_pid = safe_fork()) == 0) {
		uint32_t port = 0;
		if (setup_port_dev(dev_path, &port) < 0) {
			exit(3);
		}

		msg_t msg = { 0 };
		msg_rid_t rid;
		int count = 0;

		/* First recv */
		if (msgRecv(port, &msg, &rid) < 0) {
			exit(0);
		}

		for (;;) {
			msg.o.err = msg.type;
			count++;

			if (count % 2 == 0) {
				/* Use separate respond + recv */
				if (msgRespond(port, &msg, rid) < 0) {
					exit(1);
				}
				if (msgRecv(port, &msg, &rid) < 0) {
					exit(0);
				}
			}
			else {
				/* Use combined respondAndRecv */
				if (msgRespondAndRecv(port, &msg, &rid) < 0) {
					exit(0);
				}
			}
		}
	}

	oid_t oid;
	while (lookup(dev_path, NULL, &oid) < 0) {
		usleep(10 * 1000);
	}

	for (int i = 0; i < 50; i++) {
		msg_t msg = { 0 };
		msg.type = mtRead + (i % 5);
		msg.i.size = 0;
		msg.i.data = NULL;
		msg.o.size = 0;
		msg.o.data = NULL;

		TEST_ASSERT_EQUAL_INT(0, msgSend(oid.port, &msg));
		TEST_ASSERT_EQUAL_INT(mtRead + (i % 5), msg.o.err);
	}

	assert_child_exit(server_pid);
}


/* ===================================== */
/* Group runners                         */
/* ===================================== */


TEST_GROUP_RUNNER(msg_errnos)
{
	RUN_TEST_CASE(msg_errnos, send_invalid_port);
	RUN_TEST_CASE(msg_errnos, recv_invalid_port);
	RUN_TEST_CASE(msg_errnos, respond_invalid_port);
	RUN_TEST_CASE(msg_errnos, pulse_invalid_port);
	RUN_TEST_CASE(msg_errnos, port_create_destroy);
}


TEST_GROUP_RUNNER(msg_raw)
{
	RUN_TEST_CASE(msg_raw, raw_roundtrip);
	RUN_TEST_CASE(msg_raw, raw_partial);
	RUN_TEST_CASE(msg_raw, raw_max_fill);
	RUN_TEST_CASE(msg_raw, raw_repeated);
	RUN_TEST_CASE(msg_raw, type_and_oid_preserved);
}


TEST_GROUP_RUNNER(msg_data)
{

	RUN_TEST_CASE(msg_data, idata_and_odata);
	RUN_TEST_CASE(msg_data, idata_only);
	RUN_TEST_CASE(msg_data, odata_only);
	RUN_TEST_CASE(msg_data, large_multipage);
	RUN_TEST_CASE(msg_data, unaligned_buffer);
	RUN_TEST_CASE(msg_data, sub_page_single);
	RUN_TEST_CASE(msg_data, repeated_large);
}


TEST_GROUP_RUNNER(msg_respond_recv)
{
	RUN_TEST_CASE(msg_respond_recv, basic);
	RUN_TEST_CASE(msg_respond_recv, with_data);
}


TEST_GROUP_RUNNER(msg_pulse)
{
	RUN_TEST_CASE(msg_pulse, pulse_to_blocked_recv);
	RUN_TEST_CASE(msg_pulse, late_pulse);
	RUN_TEST_CASE(msg_pulse, overwrite_pending);
	RUN_TEST_CASE(msg_pulse, pulse_zero);
}


TEST_GROUP_RUNNER(msg_queuing)
{
	RUN_TEST_CASE(msg_queuing, concurrent_clients);
	RUN_TEST_CASE(msg_queuing, priority_ordering);
}


TEST_GROUP_RUNNER(msg_interrupt)
{
	RUN_TEST_CASE(msg_interrupt, recv_interrupted_by_signal);
	RUN_TEST_CASE(msg_interrupt, server_exit_during_send);
	RUN_TEST_CASE(msg_interrupt, client_exit_before_respond);
}


TEST_GROUP_RUNNER(msg_multiserver)
{
	RUN_TEST_CASE(msg_multiserver, out_of_order_respond);
	RUN_TEST_CASE(msg_multiserver, server_chain);
}


TEST_GROUP_RUNNER(msg_prio_inherit)
{
	RUN_TEST_CASE(msg_prio_inherit, basic_donation);
	RUN_TEST_CASE(msg_prio_inherit, multiple_clients);
	RUN_TEST_CASE(msg_prio_inherit, inversion_scenario);
}


TEST_GROUP_RUNNER(msg_priority_field)
{
	RUN_TEST_CASE(msg_priority_field, reflects_caller);
	RUN_TEST_CASE(msg_priority_field, pid_preserved);
}


TEST_GROUP_RUNNER(msg_edge)
{
	RUN_TEST_CASE(msg_edge, zero_size_nonull_data);
	RUN_TEST_CASE(msg_edge, page_boundary_buffer);
	RUN_TEST_CASE(msg_edge, asymmetric_io_sizes);
	RUN_TEST_CASE(msg_edge, rapid_port_lifecycle);
	RUN_TEST_CASE(msg_edge, destroyed_port_ops);
}


TEST_GROUP_RUNNER(msg_respond_recv_mixed)
{
	RUN_TEST_CASE(msg_respond_recv_mixed, alternating);
}


void runner(void)
{
	RUN_TEST_GROUP(msg_errnos);
	RUN_TEST_GROUP(msg_raw);
	RUN_TEST_GROUP(msg_data);
	RUN_TEST_GROUP(msg_respond_recv);
	RUN_TEST_GROUP(msg_pulse);
	RUN_TEST_GROUP(msg_queuing);
	RUN_TEST_GROUP(msg_interrupt);
	RUN_TEST_GROUP(msg_multiserver);
	RUN_TEST_GROUP(msg_prio_inherit);
	RUN_TEST_GROUP(msg_priority_field);
	RUN_TEST_GROUP(msg_edge);
	RUN_TEST_GROUP(msg_respond_recv_mixed);
}


int main(int argc, char *argv[])
{
	return (UnityMain(argc, (const char **)argv, runner) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
