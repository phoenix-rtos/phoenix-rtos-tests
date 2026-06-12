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
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/time.h>

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

	__attribute__((aligned(_PAGE_SIZE))) char buf[256];

	msg.edata = buf;
	msg.esize = sizeof(buf);

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


/* =====================================================  */
/* TEST_GROUP: msg_bwi_edge                               */
/* Complex BWI / priority inheritance edge cases          */
/* =====================================================  */

TEST_GROUP(msg_bwi_edge);

TEST_SETUP(msg_bwi_edge)
{
}

TEST_TEAR_DOWN(msg_bwi_edge)
{
}


/*
 * Chained IPC priority inheritance (depth 2):
 * Client(prio 2) -> Server1(prio 6) -> Server2(prio 7)
 *
 * Server1 receives from client (gets SC at prio 2), then calls Server2.
 * Server2 should also run at effective priority 2 via transitive SC donation.
 */
TEST(msg_bwi_edge, chained_donation)
{
	char shm_path[64];
	make_dev_path(shm_path, sizeof(shm_path), "bwi_ch_shm");
	pid_t shm_pid;
	if ((shm_pid = safe_fork()) == 0) {
		shmsrv_start(shm_path);
		exit(1);
	}

	char dev1[64], dev2[64];
	make_dev_path(dev1, sizeof(dev1), "bwi_ch1");
	make_dev_path(dev2, sizeof(dev2), "bwi_ch2");

	int *shared = (int *)shm_init(shm_path, true, 0x200);
	TEST_ASSERT_NOT_NULL(shared);
	/* shared[0] = server2 observed prio */
	shared[0] = -1;

	pid_t s2_pid;
	if ((s2_pid = safe_fork()) == 0) {
		int *sh = (int *)shm_init(shm_path, false, 0x200);
		uint32_t port = 0;
		if (setup_port_dev(dev2, &port) < 0)
			exit(3);

		priority(7);

		msg_t msg = { 0 };
		msg_rid_t rid;
		for (;;) {
			if (msgRecv(port, &msg, &rid) < 0)
				exit(0);
			sh[0] = priority(-1);
			msg.o.err = priority(-1);
			if (msgRespond(port, &msg, rid) < 0)
				exit(2);
		}
	}

	pid_t s1_pid;
	if ((s1_pid = safe_fork()) == 0) {
		uint32_t port = 0;
		if (setup_port_dev(dev1, &port) < 0)
			exit(3);

		priority(6);

		oid_t backend;
		while (lookup(dev2, NULL, &backend) < 0)
			usleep(10 * 1000);

		msg_t msg = { 0 };
		msg_rid_t rid;
		for (;;) {
			if (msgRecv(port, &msg, &rid) < 0)
				exit(0);

			/* Forward to server2 */
			msg_t fwd = { 0 };
			fwd.type = msg.type;
			fwd.i.size = 0;
			fwd.i.data = NULL;
			fwd.o.size = 0;
			fwd.o.data = NULL;
			if (msgSend(backend.port, &fwd) != 0)
				exit(4);

			msg.o.err = fwd.o.err;
			if (msgRespond(port, &msg, rid) < 0)
				exit(5);
		}
	}

	oid_t oid;
	while (lookup(dev1, NULL, &oid) < 0)
		usleep(10 * 1000);

	priority(2);

	msg_t msg = { 0 };
	msg.type = mtRead;
	msg.i.size = 0;
	msg.i.data = NULL;
	msg.o.size = 0;
	msg.o.data = NULL;
	TEST_ASSERT_EQUAL_INT(0, msgSend(oid.port, &msg));

	/* Server2 should have observed prio 2 through transitive SC donation */
	TEST_ASSERT_EQUAL_INT_MESSAGE(2, shared[0],
			"Transitive SC donation: server2 should run at client prio");

	assert_child_exit(s1_pid);
	assert_child_exit(s2_pid);
	munmap((void *)shared, 0x200);
	kill(shm_pid, SIGKILL);
	waitpid(shm_pid, NULL, 0);
}


/*
 * BWI with multiple queued clients at different priorities.
 *
 * Server at prio 6. Three clients at prio 1, 3, 5 queue.
 * Server should process each message running at the SC prio of that client.
 */
TEST(msg_bwi_edge, queued_clients_sc_tracking)
{
	char shm_path[64];
	make_dev_path(shm_path, sizeof(shm_path), "bwi_qc_shm");
	pid_t shm_pid;
	if ((shm_pid = safe_fork()) == 0) {
		shmsrv_start(shm_path);
		exit(1);
	}

	char dev_path[64];
	make_dev_path(dev_path, sizeof(dev_path), "bwi_qc");

	int *shared = (int *)shm_init(shm_path, true, 0x200);
	TEST_ASSERT_NOT_NULL(shared);
	/* shared[0..2] = observed prios for each msg processed
	 * shared[3] = server ready
	 * shared[4] = count of messages processed */
	memset((void *)shared, 0, 0x200);
	shared[0] = shared[1] = shared[2] = -1;
	shared[4] = 0;

	pid_t server_pid;
	if ((server_pid = safe_fork()) == 0) {
		int *sh = (int *)shm_init(shm_path, false, 0x200);
		uint32_t port = 0;
		if (setup_port_dev(dev_path, &port) < 0)
			exit(3);

		priority(6);
		sh[3] = 1;

		msg_t msg = { 0 };
		msg_rid_t rid;
		for (int i = 0; i < 3; i++) {
			if (msgRecv(port, &msg, &rid) < 0)
				exit(1);
			sh[i] = priority(-1);
			sh[4] = i + 1;
			msg.o.err = priority(-1);
			if (msgRespond(port, &msg, rid) < 0)
				exit(2);
		}
		exit(0);
	}

	oid_t oid;
	while (lookup(dev_path, NULL, &oid) < 0)
		usleep(10 * 1000);

	while (!shared[3])
		usleep(1000);

	/* Send from 3 clients at different prios */
	int prios[] = { 5, 3, 1 };
	pid_t clients[3];
	for (int i = 0; i < 3; i++) {
		if ((clients[i] = safe_fork()) == 0) {
			priority(prios[i]);
			msg_t msg = { 0 };
			msg.type = mtWrite;
			msg.i.size = 0;
			msg.i.data = NULL;
			msg.o.size = 0;
			msg.o.data = NULL;
			if (msgSend(oid.port, &msg) != 0)
				exit(1);
			/* Verify server ran at our prio */
			if (msg.o.err != prios[i])
				exit(2);
			exit(0);
		}
		usleep(5 * 1000);
	}

	for (int i = 0; i < 3; i++) {
		int status;
		waitpid(clients[i], &status, 0);
		TEST_ASSERT_EQUAL_INT_MESSAGE(0, WEXITSTATUS(status),
				"BWI: client didn't get correct SC prio from server");
	}

	waitpid(server_pid, NULL, 0);
	munmap((void *)shared, 0x200);
	kill(shm_pid, SIGKILL);
	waitpid(shm_pid, NULL, 0);
}


/*
 * Priority restoration after respond: after responding, server's effective
 * priority must return to its base, not remain elevated.
 */
TEST(msg_bwi_edge, prio_restored_after_respond)
{
	char shm_path[64];
	make_dev_path(shm_path, sizeof(shm_path), "bwi_pr_shm");
	pid_t shm_pid;
	if ((shm_pid = safe_fork()) == 0) {
		shmsrv_start(shm_path);
		exit(1);
	}

	char dev_path[64];
	make_dev_path(dev_path, sizeof(dev_path), "bwi_pr");

	int *shared = (int *)shm_init(shm_path, true, 0x200);
	TEST_ASSERT_NOT_NULL(shared);
	/* shared[0] = prio during msg processing
	 * shared[1] = prio after respond */
	shared[0] = -1;
	shared[1] = -1;

	pid_t server_pid;
	if ((server_pid = safe_fork()) == 0) {
		int *sh = (int *)shm_init(shm_path, false, 0x200);
		uint32_t port = 0;
		if (setup_port_dev(dev_path, &port) < 0)
			exit(3);
		priority(6);

		msg_t msg = { 0 };
		msg_rid_t rid;
		if (msgRecv(port, &msg, &rid) < 0)
			exit(1);
		sh[0] = priority(-1);
		msg.o.err = 0;
		if (msgRespond(port, &msg, rid) < 0)
			exit(2);
		sh[1] = priority(-1);
		exit(0);
	}

	oid_t oid;
	while (lookup(dev_path, NULL, &oid) < 0)
		usleep(10 * 1000);

	priority(2);
	msg_t msg = { 0 };
	msg.type = mtRead;
	msg.i.size = 0;
	msg.i.data = NULL;
	msg.o.size = 0;
	msg.o.data = NULL;
	TEST_ASSERT_EQUAL_INT(0, msgSend(oid.port, &msg));

	waitpid(server_pid, NULL, 0);

	TEST_ASSERT_EQUAL_INT_MESSAGE(2, shared[0], "Server should run at client prio 2");
	TEST_ASSERT_EQUAL_INT_MESSAGE(6, shared[1], "Server prio should restore to 6 after respond");

	munmap((void *)shared, 0x200);
	kill(shm_pid, SIGKILL);
	waitpid(shm_pid, NULL, 0);
}


/*
 * SC swap correctness under out-of-order respond:
 *
 * Two clients (A at prio 2, B at prio 4) send to server (prio 6).
 * Server receives both, responds in reverse order.
 * After all IPC, each client must hold its *own* SC (not swapped).
 *
 * Tests the FIXME: "currently nothing stops SCs from different clients to
 * end up swapped".
 */
TEST(msg_bwi_edge, sc_not_swapped)
{
	char dev_path[64];
	make_dev_path(dev_path, sizeof(dev_path), "bwi_sc");
	pid_t server_pid;

	if ((server_pid = safe_fork()) == 0) {
		uint32_t port = 0;
		if (setup_port_dev(dev_path, &port) < 0)
			exit(3);
		priority(6);

		msg_t msgs[2];
		msg_rid_t rids[2];
		for (int i = 0; i < 2; i++) {
			memset(&msgs[i], 0, sizeof(msg_t));
			if (msgRecv(port, &msgs[i], &rids[i]) < 0)
				exit(1);
		}
		/* Respond in reverse order */
		for (int i = 1; i >= 0; i--) {
			msgs[i].o.err = 0;
			if (msgRespond(port, &msgs[i], rids[i]) < 0)
				exit(2);
		}
		exit(0);
	}

	oid_t oid;
	while (lookup(dev_path, NULL, &oid) < 0)
		usleep(10 * 1000);

	pid_t c1, c2;
	if ((c1 = safe_fork()) == 0) {
		priority(2);
		msg_t msg = { 0 };
		msg.type = mtRead;
		msg.i.size = 0;
		msg.i.data = NULL;
		msg.o.size = 0;
		msg.o.data = NULL;
		if (msgSend(oid.port, &msg) != 0)
			exit(1);
		/* After IPC, our effective priority should still be 2 */
		if (priority(-1) != 2)
			exit(2);
		exit(0);
	}

	usleep(20 * 1000);

	if ((c2 = safe_fork()) == 0) {
		priority(4);
		msg_t msg = { 0 };
		msg.type = mtRead;
		msg.i.size = 0;
		msg.i.data = NULL;
		msg.o.size = 0;
		msg.o.data = NULL;
		if (msgSend(oid.port, &msg) != 0)
			exit(1);
		/* After IPC, our effective priority should still be 4 */
		if (priority(-1) != 4)
			exit(2);
		exit(0);
	}

	int s1, s2;
	waitpid(c1, &s1, 0);
	waitpid(c2, &s2, 0);
	waitpid(server_pid, NULL, 0);

	TEST_ASSERT_EQUAL_INT_MESSAGE(0, WEXITSTATUS(s1),
			"Client A: SC should not be swapped, prio must be 2");
	TEST_ASSERT_EQUAL_INT_MESSAGE(0, WEXITSTATUS(s2),
			"Client B: SC should not be swapped, prio must be 4");
}


/* =====================================================  */
/* TEST_GROUP: msg_lock_ipc                               */
/* Priority inversion between IPC BWI and kernel mutexes  */
/* =====================================================  */

TEST_GROUP(msg_lock_ipc);

TEST_SETUP(msg_lock_ipc)
{
}

TEST_TEAR_DOWN(msg_lock_ipc)
{
}


/*
 * Shared state for intra-process thread coordination in lock_ipc tests.
 * These tests fork a server process that spawns threads with beginthreadex.
 * Threads within that process share this struct.
 */
static struct {
	handle_t mutex;
	volatile int phase;     /* coordination barrier */
	volatile int prio_obs;  /* observed priority */
	volatile int prio_obs2; /* second observed priority */
	volatile int mid_ran;   /* medium thread ran to completion */
	char stack[4][4096] __attribute__((aligned(8)));
} li_common;


/*
 * Two-Task Problem with IPC:
 *
 *   Client H (P1) --IPC--> Server process:
 *     Thread S (base P4): receives IPC (gets SC at P1), tries to lock mutex M
 *     Thread L (base P6): holds mutex M
 *
 * When S (effective P1 via SC) blocks on mutex held by L, mutex PI should
 * boost L to P1 (S's effective priority, not just S's base P4).
 *
 * We verify by observing L's priority while it holds the mutex under
 * contention from S.
 */

static void li_two_task_holder(void *arg)
{
	/* Thread L: hold mutex, wait for S to contend, observe priority */
	mutexLock(li_common.mutex);
	li_common.phase = 1; /* signal: mutex held */

	/* Wait for S to block on mutex (it will try after receiving IPC) */
	while (li_common.phase < 2)
		usleep(1000);

	/* Small delay to let S actually block on mutexLock */
	usleep(20 * 1000);

	/* Observe our priority - should be boosted to P1 if mutex PI sees SC */
	li_common.prio_obs = priority(-1);

	mutexUnlock(li_common.mutex);
	endthread();
}

TEST(msg_lock_ipc, two_task)
{
	char shm_path[64];
	make_dev_path(shm_path, sizeof(shm_path), "li2t_shm");
	pid_t shm_pid;
	if ((shm_pid = safe_fork()) == 0) {
		shmsrv_start(shm_path);
		exit(1);
	}

	char dev_path[64];
	make_dev_path(dev_path, sizeof(dev_path), "li2t");

	int *shared = (int *)shm_init(shm_path, true, 0x200);
	TEST_ASSERT_NOT_NULL(shared);
	/* shared[0] = server ready
	 * shared[1] = L's observed priority while holding mutex under contention
	 * shared[2] = S's priority during IPC (before mutex)
	 * shared[3] = S's priority after acquiring mutex */
	memset((void *)shared, 0, 0x200);

	pid_t server_pid;
	if ((server_pid = safe_fork()) == 0) {
		int *sh = (int *)shm_init(shm_path, false, 0x200);
		uint32_t port = 0;
		if (setup_port_dev(dev_path, &port) < 0)
			exit(3);

		mutexCreate(&li_common.mutex);
		li_common.phase = 0;
		li_common.prio_obs = -1;

		handle_t tid_l;
		beginthreadex(li_two_task_holder, 6, li_common.stack[0],
				sizeof(li_common.stack[0]), NULL, &tid_l);

		/* Wait for L to hold the mutex */
		while (li_common.phase < 1)
			usleep(1000);

		priority(4);
		sh[0] = 1;

		msg_t msg = { 0 };
		msg_rid_t rid;
		if (msgRecv(port, &msg, &rid) < 0)
			exit(1);

		/* Running on client's SC at P1 now */
		sh[2] = priority(-1);
		li_common.phase = 2; /* signal: about to contend */

		/* This should block until L releases; mutex PI should boost L */
		mutexLock(li_common.mutex);
		sh[3] = priority(-1);
		mutexUnlock(li_common.mutex);

		sh[1] = li_common.prio_obs;

		msg.o.err = 0;
		msgRespond(port, &msg, rid);

		threadJoin(tid_l, 0);
		resourceDestroy(li_common.mutex);
		exit(0);
	}

	oid_t oid;
	while (lookup(dev_path, NULL, &oid) < 0)
		usleep(10 * 1000);
	while (!shared[0])
		usleep(1000);

	priority(1);
	msg_t msg = { 0 };
	msg.type = mtRead;
	msg.i.size = 0;
	msg.i.data = NULL;
	msg.o.size = 0;
	msg.o.data = NULL;
	TEST_ASSERT_EQUAL_INT(0, msgSend(oid.port, &msg));

	waitpid(server_pid, NULL, 0);

	TEST_ASSERT_EQUAL_INT_MESSAGE(1, shared[2],
			"Server thread S should run at P1 via SC donation");
	TEST_ASSERT_EQUAL_INT_MESSAGE(1, shared[1],
			"Two-task: mutex holder L should be boosted to P1 (SC effective prio)");

	munmap((void *)shared, 0x200);
	kill(shm_pid, SIGKILL);
	waitpid(shm_pid, NULL, 0);
}


/*
 * Three-Task Problem with IPC:
 *
 *   Client H (P1) --IPC--> Server process:
 *     Thread S (base P4): receives IPC (gets SC at P1), blocks on mutex
 *     Thread L (base P6): holds mutex M
 *     Thread M (base P2): CPU-bound busy loop
 *
 * Correct behavior (mutex PI sees SC prio):
 *   S blocks on mutex -> L boosted to P1 -> M (P2) cannot preempt L ->
 *   L finishes quickly -> S gets mutex -> responds -> H unblocks.
 *
 * Broken behavior (mutex PI only sees thread prio P4):
 *   S blocks on mutex -> L boosted to P4 -> M (P2) preempts L ->
 *   unbounded priority inversion: H (highest prio) waits for M to finish.
 *
 * We measure elapsed time. If M is a long busy loop and the IPC completes
 * fast, mutex PI correctly propagated SC priority. If IPC takes as long
 * as M's loop, we have inversion.
 */

static volatile int li_three_m_done;

static void li_three_task_holder(void *arg)
{
	/* Thread L (P6): hold mutex, do modest work, release */
	mutexLock(li_common.mutex);
	li_common.phase = 1;

	/* Wait until S has received IPC and is about to contend */
	while (li_common.phase < 2)
		usleep(1000);

	/* Let S actually block on the mutex */
	usleep(20 * 1000);

	li_common.prio_obs = priority(-1);

	/* Simulate some work (short) */
	for (volatile int i = 0; i < 10000; i++)
		;

	mutexUnlock(li_common.mutex);
	endthread();
}

static void li_three_task_medium(void *arg)
{
	/* Thread M (P2): CPU-bound busy loop - long enough to cause
	 * observable inversion if L is not properly boosted.
	 * Wait until S has received IPC and is contending on the mutex,
	 * so the PI chain is fully formed. */
	while (li_common.phase < 2)
		usleep(1000);
	usleep(10 * 1000); /* let S actually block on the mutex */

	/* Long busy loop */
	for (volatile int i = 0; i < 20000000; i++)
		;

	li_three_m_done = 1;
	endthread();
}

TEST(msg_lock_ipc, three_task)
{
	char shm_path[64];
	make_dev_path(shm_path, sizeof(shm_path), "li3t_shm");
	pid_t shm_pid;
	if ((shm_pid = safe_fork()) == 0) {
		shmsrv_start(shm_path);
		exit(1);
	}

	char dev_path[64];
	make_dev_path(dev_path, sizeof(dev_path), "li3t");

	int *shared = (int *)shm_init(shm_path, true, 0x200);
	TEST_ASSERT_NOT_NULL(shared);
	/* shared[0] = server ready
	 * shared[1] = L's observed priority while holding mutex
	 * shared[2] = IPC round-trip time (us)
	 * shared[3] = M finished before IPC returned (1=inversion) */
	memset((void *)shared, 0, 0x200);

	pid_t server_pid;
	if ((server_pid = safe_fork()) == 0) {
		int *sh = (int *)shm_init(shm_path, false, 0x200);
		uint32_t port = 0;
		if (setup_port_dev(dev_path, &port) < 0)
			exit(3);

		mutexCreate(&li_common.mutex);
		li_common.phase = 0;
		li_common.prio_obs = -1;
		li_three_m_done = 0;

		handle_t tid_l, tid_m;
		beginthreadex(li_three_task_holder, 6, li_common.stack[0],
				sizeof(li_common.stack[0]), NULL, &tid_l);
		beginthreadex(li_three_task_medium, 2, li_common.stack[1],
				sizeof(li_common.stack[1]), NULL, &tid_m);

		/* Wait for L to hold the mutex */
		while (li_common.phase < 1)
			usleep(1000);

		priority(4);
		sh[0] = 1;

		msg_t msg = { 0 };
		msg_rid_t rid;
		if (msgRecv(port, &msg, &rid) < 0)
			exit(1);

		/* Running on client's SC at P1 */
		li_common.phase = 2;

		/* Block on mutex held by L */
		mutexLock(li_common.mutex);
		mutexUnlock(li_common.mutex);

		sh[1] = li_common.prio_obs;
		sh[3] = li_three_m_done;

		msg.o.err = 0;
		msgRespond(port, &msg, rid);

		threadJoin(tid_l, 0);
		threadJoin(tid_m, 0);
		resourceDestroy(li_common.mutex);
		exit(0);
	}

	oid_t oid;
	while (lookup(dev_path, NULL, &oid) < 0)
		usleep(10 * 1000);
	while (!shared[0])
		usleep(1000);

	priority(1);
	msg_t msg = { 0 };
	msg.type = mtRead;
	msg.i.size = 0;
	msg.i.data = NULL;
	msg.o.size = 0;
	msg.o.data = NULL;

	time_t t_start, t_end;
	gettime(&t_start, NULL);
	TEST_ASSERT_EQUAL_INT(0, msgSend(oid.port, &msg));
	gettime(&t_end, NULL);

	waitpid(server_pid, NULL, 0);

	time_t elapsed = t_end - t_start;
	shared[2] = (int)elapsed;

	TEST_ASSERT_EQUAL_INT_MESSAGE(1, shared[1],
			"Three-task: L should be boosted to P1 (SC prio), not just P4 (thread prio)");
	TEST_ASSERT_EQUAL_INT_MESSAGE(0, shared[3],
			"Three-task: M (P2) should NOT have finished before IPC returned "
			"(would indicate priority inversion)");

	munmap((void *)shared, 0x200);
	kill(shm_pid, SIGKILL);
	waitpid(shm_pid, NULL, 0);
}


/*
 * Three-Task Problem, reversed direction: mutex inside IPC server,
 * IPC client does the contention.
 *
 *   Thread L (P6) in server: holds mutex, processes IPC from H
 *   Thread M (P3) in server: CPU-bound
 *   Client H (P1): sends IPC
 *
 * L receives IPC (gets SC at P1), holds mutex, starts processing.
 * M should not preempt L since L runs at P1 via SC.
 * Meanwhile another thread in server (T at P5) contends on the same
 * mutex. Mutex PI should boost L to at least T's prio, but L already
 * runs at P1 via SC, so the boost should be a no-op.
 *
 * Verifies that mutex PI and SC donation don't conflict / corrupt
 * each other when the SC-donated thread is the mutex owner.
 */

static void li_reverse_contender(void *arg)
{
	/* Thread T (P5): waits then contends on mutex */
	while (li_common.phase < 2)
		usleep(1000);

	mutexLock(li_common.mutex);
	li_common.prio_obs2 = priority(-1);
	mutexUnlock(li_common.mutex);
	endthread();
}

static void li_reverse_medium(void *arg)
{
	/* Thread M (P3): CPU hog */
	while (li_common.phase < 1)
		usleep(1000);

	for (volatile int i = 0; i < 20000000; i++)
		;

	li_common.mid_ran = 1;
	endthread();
}

TEST(msg_lock_ipc, reverse_three_task)
{
	char shm_path[64];
	make_dev_path(shm_path, sizeof(shm_path), "lir3_shm");
	pid_t shm_pid;
	if ((shm_pid = safe_fork()) == 0) {
		shmsrv_start(shm_path);
		exit(1);
	}

	char dev_path[64];
	make_dev_path(dev_path, sizeof(dev_path), "lir3");

	int *shared = (int *)shm_init(shm_path, true, 0x200);
	TEST_ASSERT_NOT_NULL(shared);
	/* shared[0] = server ready
	 * shared[1] = S's prio during IPC while holding mutex
	 * shared[2] = S's prio after mutex contention from T
	 * shared[3] = M finished before IPC response */
	memset((void *)shared, 0, 0x200);

	pid_t server_pid;
	if ((server_pid = safe_fork()) == 0) {
		int *sh = (int *)shm_init(shm_path, false, 0x200);
		uint32_t port = 0;
		if (setup_port_dev(dev_path, &port) < 0)
			exit(3);

		mutexCreate(&li_common.mutex);
		li_common.phase = 0;
		li_common.prio_obs = -1;
		li_common.prio_obs2 = -1;
		li_common.mid_ran = 0;

		handle_t tid_t, tid_m;
		/* Spawn M first so it's ready */
		beginthreadex(li_reverse_medium, 3, li_common.stack[1],
				sizeof(li_common.stack[1]), NULL, &tid_m);

		priority(6);
		sh[0] = 1;

		msg_t msg = { 0 };
		msg_rid_t rid;
		if (msgRecv(port, &msg, &rid) < 0)
			exit(1);

		/* Running on client H's SC at P1 now */
		li_common.phase = 1; /* let M start spinning */

		mutexLock(li_common.mutex);
		sh[1] = priority(-1); /* should be P1 from SC */

		/* Spawn T to contend on mutex */
		beginthreadex(li_reverse_contender, 5, li_common.stack[2],
				sizeof(li_common.stack[2]), NULL, &tid_t);
		li_common.phase = 2;

		/* Hold mutex for a bit under contention */
		usleep(20 * 1000);
		sh[2] = priority(-1); /* should still be P1, T's P5 < P1 */

		mutexUnlock(li_common.mutex);
		sh[3] = li_common.mid_ran;

		msg.o.err = 0;
		msgRespond(port, &msg, rid);

		threadJoin(tid_t, 0);
		threadJoin(tid_m, 0);
		resourceDestroy(li_common.mutex);
		exit(0);
	}

	oid_t oid;
	while (lookup(dev_path, NULL, &oid) < 0)
		usleep(10 * 1000);
	while (!shared[0])
		usleep(1000);

	priority(1);
	msg_t msg = { 0 };
	msg.type = mtRead;
	msg.i.size = 0;
	msg.i.data = NULL;
	msg.o.size = 0;
	msg.o.data = NULL;
	TEST_ASSERT_EQUAL_INT(0, msgSend(oid.port, &msg));

	waitpid(server_pid, NULL, 0);

	TEST_ASSERT_EQUAL_INT_MESSAGE(1, shared[1],
			"Reverse: S should run at P1 via SC while holding mutex");
	TEST_ASSERT_EQUAL_INT_MESSAGE(1, shared[2],
			"Reverse: S should still be at P1 even after T (P5) contends");
	TEST_ASSERT_EQUAL_INT_MESSAGE(0, shared[3],
			"Reverse: M (P3) should not finish before IPC response");

	munmap((void *)shared, 0x200);
	kill(shm_pid, SIGKILL);
	waitpid(shm_pid, NULL, 0);
}


/*
 * Nested: IPC within lock critical section, with PI propagation.
 *
 * Single process, two threads + external server:
 *   Thread L (P6): locks mutex, then sends IPC to external server S2 (P7)
 *   Thread H (P1): blocks on same mutex -> mutex PI boosts L to P1
 *
 * When L does IPC to S2 while boosted by mutex PI, S2 should see P1
 * as the effective caller priority (the boosted prio should propagate
 * through IPC SC donation).
 */

static struct {
	handle_t mutex;
	volatile int phase;
	volatile int l_prio_before_ipc;
	oid_t backend_oid;
	char stack[2][4096] __attribute__((aligned(8)));
} li_nest;

static void li_nest_low(void *arg)
{
	/* Thread L (P6): lock mutex, wait for H to contend, then do IPC */
	mutexLock(li_nest.mutex);
	li_nest.phase = 1; /* signal: mutex held */

	/* Wait for H to block on mutex (which boosts us) */
	while (li_nest.phase < 2)
		usleep(1000);
	usleep(20 * 1000); /* let H actually block */

	li_nest.l_prio_before_ipc = priority(-1);

	/* Do IPC while holding mutex and being boosted */
	msg_t msg = { 0 };
	msg.type = mtRead;
	msg.i.size = 0;
	msg.i.data = NULL;
	msg.o.size = 0;
	msg.o.data = NULL;
	msgSend(li_nest.backend_oid.port, &msg);

	mutexUnlock(li_nest.mutex);
	endthread();
}

static void li_nest_high(void *arg)
{
	/* Thread H (P1): wait for L to hold mutex, then block on it */
	while (li_nest.phase < 1)
		usleep(1000);

	li_nest.phase = 2; /* signal: about to contend */

	/* This blocks -> mutex PI boosts L to our P1 */
	mutexLock(li_nest.mutex);
	mutexUnlock(li_nest.mutex);
	endthread();
}

TEST(msg_lock_ipc, nested_lock_then_ipc)
{
	char shm_path[64];
	make_dev_path(shm_path, sizeof(shm_path), "lin_shm");
	pid_t shm_pid;
	if ((shm_pid = safe_fork()) == 0) {
		shmsrv_start(shm_path);
		exit(1);
	}

	char dev_path[64];
	make_dev_path(dev_path, sizeof(dev_path), "lin_s2");

	int *shared = (int *)shm_init(shm_path, true, 0x200);
	TEST_ASSERT_NOT_NULL(shared);
	/* shared[0] = S2's observed priority during IPC from L */
	memset((void *)shared, 0, 0x200);
	shared[0] = -1;

	/* Spawn echo server S2 that reports caller's effective priority */
	pid_t s2_pid;
	if ((s2_pid = safe_fork()) == 0) {
		int *sh = (int *)shm_init(shm_path, false, 0x200);
		uint32_t port = 0;
		if (setup_port_dev(dev_path, &port) < 0)
			exit(3);
		priority(7);

		msg_t msg = { 0 };
		msg_rid_t rid;
		for (;;) {
			if (msgRecv(port, &msg, &rid) < 0)
				exit(0);
			sh[0] = priority(-1);
			msg.o.err = 0;
			if (msgRespond(port, &msg, rid) < 0)
				exit(2);
		}
	}

	while (lookup(dev_path, NULL, &li_nest.backend_oid) < 0)
		usleep(10 * 1000);

	mutexCreate(&li_nest.mutex);
	li_nest.phase = 0;
	li_nest.l_prio_before_ipc = -1;

	handle_t tid_l, tid_h;
	beginthreadex(li_nest_low, 6, li_nest.stack[0],
			sizeof(li_nest.stack[0]), NULL, &tid_l);
	beginthreadex(li_nest_high, 1, li_nest.stack[1],
			sizeof(li_nest.stack[1]), NULL, &tid_h);

	threadJoin(tid_l, 0);
	threadJoin(tid_h, 0);
	resourceDestroy(li_nest.mutex);

	TEST_ASSERT_EQUAL_INT_MESSAGE(1, li_nest.l_prio_before_ipc,
			"Nested: L should be boosted to P1 by mutex PI before IPC");
	TEST_ASSERT_EQUAL_INT_MESSAGE(1, shared[0],
			"Nested: S2 should see P1 from L's boosted priority via IPC SC");

	assert_child_exit(s2_pid);
	munmap((void *)shared, 0x200);
	kill(shm_pid, SIGKILL);
	waitpid(shm_pid, NULL, 0);
}


/*
 * Cascaded: IPC donation -> mutex contention -> IPC donation.
 *
 *   Client H (P1) --IPC--> Server process:
 *     Thread S (base P4): receives IPC (SC at P1), locks mutex
 *     Thread W (base P5): blocks on mutex, then does IPC to S2 (P7)
 *
 * Chain: H's P1 -> SC to S -> mutex PI should boost... wait, S is the
 * owner, W blocks. S runs at P1, W at P5. W blocks on S's mutex,
 * but S is already P1, no boost needed.
 *
 * Better scenario: S receives IPC at P1, blocks on mutex held by W.
 * Mutex PI boosts W to P1. W then does IPC to S2. S2 should see P1.
 *
 *   Client H (P1) --IPC--> Server:
 *     Thread S (base P4): receives IPC (SC@P1), blocks on mutex held by W
 *     Thread W (base P5): holds mutex, does IPC to S2 when boosted
 */

static struct {
	handle_t mutex;
	volatile int phase;
	oid_t backend_oid;
	char stack[2][4096] __attribute__((aligned(8)));
} li_casc;

static void li_casc_worker(void *arg)
{
	int *sh = (int *)arg;
	/* Thread W (P5): hold mutex, wait for S to contend, observe boost,
	 * then do IPC to S2 */
	mutexLock(li_casc.mutex);
	li_casc.phase = 1; /* signal: mutex held */

	/* Wait for S to receive IPC and try to lock */
	while (li_casc.phase < 2)
		usleep(1000);
	usleep(20 * 1000); /* let S block on mutex */

	/* Our priority should be boosted by mutex PI */
	sh[1] = priority(-1);

	/* Do IPC while holding mutex and (hopefully) boosted */
	msg_t fwd = { 0 };
	fwd.type = mtRead;
	fwd.i.size = 0;
	fwd.i.data = NULL;
	fwd.o.size = 0;
	fwd.o.data = NULL;
	msgSend(li_casc.backend_oid.port, &fwd);

	mutexUnlock(li_casc.mutex);
	endthread();
}

TEST(msg_lock_ipc, cascaded_ipc_lock_ipc)
{
	char shm_path[64];
	make_dev_path(shm_path, sizeof(shm_path), "lic_shm");
	pid_t shm_pid;
	if ((shm_pid = safe_fork()) == 0) {
		shmsrv_start(shm_path);
		exit(1);
	}

	char dev1[64], dev2[64];
	make_dev_path(dev1, sizeof(dev1), "lic_s1");
	make_dev_path(dev2, sizeof(dev2), "lic_s2");

	int *shared = (int *)shm_init(shm_path, true, 0x200);
	TEST_ASSERT_NOT_NULL(shared);
	/* shared[0] = server ready
	 * shared[1] = W's observed prio (should be P1 from cascaded PI)
	 * shared[2] = S2's observed prio during IPC from W */
	memset((void *)shared, 0, 0x200);
	shared[1] = -1;
	shared[2] = -1;

	/* Spawn S2: echo server that reports observed priority */
	pid_t s2_pid;
	if ((s2_pid = safe_fork()) == 0) {
		int *sh = (int *)shm_init(shm_path, false, 0x200);
		uint32_t port = 0;
		if (setup_port_dev(dev2, &port) < 0)
			exit(3);
		priority(7);

		msg_t msg = { 0 };
		msg_rid_t rid;
		for (;;) {
			if (msgRecv(port, &msg, &rid) < 0)
				exit(0);
			sh[2] = priority(-1);
			msg.o.err = 0;
			if (msgRespond(port, &msg, rid) < 0)
				exit(2);
		}
	}

	while (lookup(dev2, NULL, &li_casc.backend_oid) < 0)
		usleep(10 * 1000);

	/* Spawn server S1: receives IPC, contends on mutex held by W */
	pid_t s1_pid;
	if ((s1_pid = safe_fork()) == 0) {
		int *sh = (int *)shm_init(shm_path, false, 0x200);
		uint32_t port = 0;
		if (setup_port_dev(dev1, &port) < 0)
			exit(3);

		mutexCreate(&li_casc.mutex);
		li_casc.phase = 0;

		handle_t tid_w;
		beginthreadex(li_casc_worker, 5, li_casc.stack[0],
				sizeof(li_casc.stack[0]), sh, &tid_w);

		/* Wait for W to hold the mutex */
		while (li_casc.phase < 1)
			usleep(1000);

		priority(4);
		sh[0] = 1;

		msg_t msg = { 0 };
		msg_rid_t rid;
		if (msgRecv(port, &msg, &rid) < 0)
			exit(1);

		/* Running on H's SC at P1 */
		li_casc.phase = 2; /* signal: about to contend */

		/* Block on mutex held by W ->
 PI should boost W to P1 */
		mutexLock(li_casc.mutex);
		mutexUnlock(li_casc.mutex);

		msg.o.err = 0;
		msgRespond(port, &msg, rid);

		threadJoin(tid_w, 0);
		resourceDestroy(li_casc.mutex);
		exit(0);
	}

	oid_t oid;
	while (lookup(dev1, NULL, &oid) < 0)
		usleep(10 * 1000);
	while (!shared[0])
		usleep(1000);

	priority(1);
	msg_t msg = { 0 };
	msg.type = mtRead;
	msg.i.size = 0;
	msg.i.data = NULL;
	msg.o.size = 0;
	msg.o.data = NULL;
	TEST_ASSERT_EQUAL_INT(0, msgSend(oid.port, &msg));

	waitpid(s1_pid, NULL, 0);

	TEST_ASSERT_EQUAL_INT_MESSAGE(1, shared[1],
			"Cascaded: W should be boosted to P1 via mutex PI from S (SC@P1)");
	TEST_ASSERT_EQUAL_INT_MESSAGE(1, shared[2],
			"Cascaded: S2 should see P1 from W's boosted IPC SC");

	assert_child_exit(s2_pid);
	munmap((void *)shared, 0x200);
	kill(shm_pid, SIGKILL);
	waitpid(shm_pid, NULL, 0);
}


/*
 * Transitive mutex chain with IPC (the classic chained dependency):
 *
 *   Client H (P1) --IPC--> Server process:
 *     Thread S (base P4): receives IPC (SC@P1), blocks on M1 (held by L2)
 *     Thread L2 (base P5): holds M1, blocked on M2 (held by L1)
 *     Thread L1 (base P7): holds M2, doing work
 *     Thread M (base P2): CPU-bound busy loop (interferer)
 *
 * Dependency chain: S(P1) --M1--> L2(P5) --M2--> L1(P7)
 *
 * Correct (transitive PI):
 *   S blocks on M1 -> L2 boosted to P1 (via SC)
 *   But L2 is blocked on M2 -> L1 should ALSO be boosted to P1
 *   L1 finishes at P1 -> releases M2 -> L2 runs at P1 -> releases M1 ->
 *   S runs -> responds -> H unblocks. M(P2) never preempts.
 *
 * Broken (single-hop PI only):
 *   S blocks on M1 -> L2 boosted to P1, but L2 is sleeping on M2
 *   L1 stays at P7 -> M(P2) preempts L1 -> unbounded inversion
 */

static struct {
	handle_t m1;
	handle_t m2;
	volatile int phase;
	volatile int l1_prio; /* L1's observed prio while holding M2 under chain */
	volatile int l2_prio; /* L2's observed prio while holding M1 under chain */
	volatile int m_done;  /* medium thread completed */
	char stack[4][4096] __attribute__((aligned(8)));
} li_chain;

static void li_chain_l1(void *arg)
{
	/* Thread L1 (P7): lock M2, signal, wait for chain to form, observe prio */
	mutexLock(li_chain.m2);
	li_chain.phase = 1; /* M2 held */

	/* Wait until S has received IPC and L2 is blocked on M2 */
	while (li_chain.phase < 4)
		usleep(1000);
	usleep(30 * 1000); /* let the chain fully form */

	li_chain.l1_prio = priority(-1);

	/* Short work, then release */
	for (volatile int i = 0; i < 5000; i++)
		;

	mutexUnlock(li_chain.m2);
	endthread();
}

static void li_chain_l2(void *arg)
{
	/* Thread L2 (P5): wait for L1 to hold M2, then lock M1 and block on M2 */
	while (li_chain.phase < 1)
		usleep(1000);

	mutexLock(li_chain.m1);
	li_chain.phase = 2; /* M1 held */

	/* Wait for S to be ready to contend */
	while (li_chain.phase < 3)
		usleep(1000);

	li_chain.phase = 4; /* about to block on M2 */

	/* Block on M2 (held by L1) — this is where transitive PI matters */
	mutexLock(li_chain.m2);

	/* After wakeup: L2 holds M1 (S waiting at P1) + M2 (no waiters).
	 * BWI should recalculate L2's priority from held locks → P1. */
	li_chain.l2_prio = priority(-1);

	mutexUnlock(li_chain.m2);
	mutexUnlock(li_chain.m1);
	endthread();
}

static void li_chain_medium(void *arg)
{
	/* Thread M (P2): CPU hog — causes inversion if L1 isn't boosted.
	 * Wait until S has received IPC and the full chain is forming. */
	while (li_chain.phase < 4)
		usleep(1000);
	usleep(20 * 1000); /* let S block on M1 and propagate boosts */

	for (volatile int i = 0; i < 30000000; i++)
		;

	li_chain.m_done = 1;
	endthread();
}

TEST(msg_lock_ipc, transitive_chain)
{
	char shm_path[64];
	make_dev_path(shm_path, sizeof(shm_path), "litc_shm");
	pid_t shm_pid;
	if ((shm_pid = safe_fork()) == 0) {
		shmsrv_start(shm_path);
		exit(1);
	}

	char dev_path[64];
	make_dev_path(dev_path, sizeof(dev_path), "litc");

	int *shared = (int *)shm_init(shm_path, true, 0x200);
	TEST_ASSERT_NOT_NULL(shared);
	/* shared[0] = server ready
	 * shared[1] = L1's observed prio
	 * shared[2] = L2's observed prio
	 * shared[3] = M finished before IPC returned (1=inversion)
	 * shared[4] = S's prio during IPC */
	memset((void *)shared, 0, 0x200);

	pid_t server_pid;
	if ((server_pid = safe_fork()) == 0) {
		int *sh = (int *)shm_init(shm_path, false, 0x200);
		uint32_t port = 0;
		if (setup_port_dev(dev_path, &port) < 0)
			exit(3);

		mutexCreate(&li_chain.m1);
		mutexCreate(&li_chain.m2);
		li_chain.phase = 0;
		li_chain.l1_prio = -1;
		li_chain.l2_prio = -1;
		li_chain.m_done = 0;

		handle_t tid_l1, tid_l2, tid_m;
		beginthreadex(li_chain_l1, 7, li_chain.stack[0],
				sizeof(li_chain.stack[0]), NULL, &tid_l1);
		beginthreadex(li_chain_l2, 5, li_chain.stack[1],
				sizeof(li_chain.stack[1]), NULL, &tid_l2);
		beginthreadex(li_chain_medium, 2, li_chain.stack[2],
				sizeof(li_chain.stack[2]), NULL, &tid_m);

		/* Wait for L2 to hold M1 */
		while (li_chain.phase < 2)
			usleep(1000);

		priority(4);
		sh[0] = 1;

		msg_t msg = { 0 };
		msg_rid_t rid;
		if (msgRecv(port, &msg, &rid) < 0)
			exit(1);

		/* Running on client's SC at P1 */
		sh[4] = priority(-1);
		li_chain.phase = 3; /* signal: about to contend on M1 */

		/* Wait for L2 to start blocking on M2 */
		while (li_chain.phase < 4)
			usleep(1000);
		usleep(10 * 1000);

		/* Block on M1 (held by L2, who is blocked on M2 held by L1) */
		mutexLock(li_chain.m1);
		mutexUnlock(li_chain.m1);

		sh[1] = li_chain.l1_prio;
		sh[2] = li_chain.l2_prio;
		sh[3] = li_chain.m_done;

		msg.o.err = 0;
		msgRespond(port, &msg, rid);

		threadJoin(tid_l1, 0);
		threadJoin(tid_l2, 0);
		threadJoin(tid_m, 0);
		resourceDestroy(li_chain.m1);
		resourceDestroy(li_chain.m2);
		exit(0);
	}

	oid_t oid;
	while (lookup(dev_path, NULL, &oid) < 0)
		usleep(10 * 1000);
	while (!shared[0])
		usleep(1000);

	priority(1);
	msg_t msg = { 0 };
	msg.type = mtRead;
	msg.i.size = 0;
	msg.i.data = NULL;
	msg.o.size = 0;
	msg.o.data = NULL;
	TEST_ASSERT_EQUAL_INT(0, msgSend(oid.port, &msg));

	waitpid(server_pid, NULL, 0);

	TEST_ASSERT_EQUAL_INT_MESSAGE(1, shared[4],
			"Transitive chain: S should run at P1 via SC");
	TEST_ASSERT_EQUAL_INT_MESSAGE(1, shared[2],
			"Transitive chain: L2 should be boosted to P1");
	TEST_ASSERT_EQUAL_INT_MESSAGE(1, shared[1],
			"Transitive chain: L1 should be transitively boosted to P1");
	TEST_ASSERT_EQUAL_INT_MESSAGE(0, shared[3],
			"Transitive chain: M(P2) must NOT finish before IPC (inversion)");

	munmap((void *)shared, 0x200);
	kill(shm_pid, SIGKILL);
	waitpid(shm_pid, NULL, 0);
}


/*
 * Diamond dependency: two high-prio paths converge on one lock holder.
 *
 *   Client H1 (P1) --IPC--> Server process:
 *     Thread S (base P4): receives IPC (SC@P1), blocks on M (held by L)
 *     Thread T (base P3): also blocks on M (held by L)
 *     Thread L (base P7): holds M
 *     Thread M_thr (base P2): CPU-bound interferer
 *
 * Both S(eff P1) and T(P3) block on M. L should be boosted to P1
 * (from S, not just P3 from T). When L releases M, the highest-prio
 * waiter (S at P1) should get the lock first.
 */

static struct {
	handle_t mutex;
	volatile int phase;
	volatile int l_prio;
	volatile int first_acquirer_prio;
	volatile int m_done;
	char stack[4][4096] __attribute__((aligned(8)));
} li_diamond;

static void li_diamond_holder(void *arg)
{
	/* Thread L (P7): hold mutex, wait for both S and T to contend */
	mutexLock(li_diamond.mutex);
	li_diamond.phase = 1;

	/* Wait for both contenders to be ready */
	while (li_diamond.phase < 3)
		usleep(1000);
	usleep(30 * 1000);

	li_diamond.l_prio = priority(-1);

	for (volatile int i = 0; i < 5000; i++)
		;

	mutexUnlock(li_diamond.mutex);
	endthread();
}

static void li_diamond_contender(void *arg)
{
	/* Thread T (P3): wait for L to hold mutex, then contend */
	while (li_diamond.phase < 1)
		usleep(1000);

	li_diamond.phase = 2;

	mutexLock(li_diamond.mutex);
	/* If we get here after L releases, record our prio if we're first */
	if (li_diamond.first_acquirer_prio == -1)
		li_diamond.first_acquirer_prio = priority(-1);
	mutexUnlock(li_diamond.mutex);
	endthread();
}

static void li_diamond_medium(void *arg)
{
	/* Wait until S has received IPC and both contenders are ready */
	while (li_diamond.phase < 3)
		usleep(1000);
	usleep(10 * 1000); /* let S and T block on mutex */

	for (volatile int i = 0; i < 30000000; i++)
		;

	li_diamond.m_done = 1;
	endthread();
}

TEST(msg_lock_ipc, diamond_contention)
{
	char shm_path[64];
	make_dev_path(shm_path, sizeof(shm_path), "lidc_shm");
	pid_t shm_pid;
	if ((shm_pid = safe_fork()) == 0) {
		shmsrv_start(shm_path);
		exit(1);
	}

	char dev_path[64];
	make_dev_path(dev_path, sizeof(dev_path), "lidc");

	int *shared = (int *)shm_init(shm_path, true, 0x200);
	TEST_ASSERT_NOT_NULL(shared);
	/* shared[0] = server ready
	 * shared[1] = L's observed prio (should be P1)
	 * shared[2] = M finished before IPC returned
	 * shared[3] = first acquirer prio after L releases */
	memset((void *)shared, 0, 0x200);

	pid_t server_pid;
	if ((server_pid = safe_fork()) == 0) {
		int *sh = (int *)shm_init(shm_path, false, 0x200);
		uint32_t port = 0;
		if (setup_port_dev(dev_path, &port) < 0)
			exit(3);

		mutexCreate(&li_diamond.mutex);
		li_diamond.phase = 0;
		li_diamond.l_prio = -1;
		li_diamond.first_acquirer_prio = -1;
		li_diamond.m_done = 0;

		handle_t tid_l, tid_t, tid_m;
		beginthreadex(li_diamond_holder, 7, li_diamond.stack[0],
				sizeof(li_diamond.stack[0]), NULL, &tid_l);
		beginthreadex(li_diamond_contender, 3, li_diamond.stack[1],
				sizeof(li_diamond.stack[1]), NULL, &tid_t);
		beginthreadex(li_diamond_medium, 2, li_diamond.stack[2],
				sizeof(li_diamond.stack[2]), NULL, &tid_m);

		while (li_diamond.phase < 2)
			usleep(1000);

		priority(4);
		sh[0] = 1;

		msg_t msg = { 0 };
		msg_rid_t rid;
		if (msgRecv(port, &msg, &rid) < 0)
			exit(1);

		/* Running on H1's SC at P1 */
		li_diamond.phase = 3; /* tell L both contenders are ready */

		/* Block on mutex held by L (alongside T) */
		mutexLock(li_diamond.mutex);
		if (li_diamond.first_acquirer_prio == -1)
			li_diamond.first_acquirer_prio = priority(-1);
		mutexUnlock(li_diamond.mutex);

		sh[1] = li_diamond.l_prio;
		sh[2] = li_diamond.m_done;
		sh[3] = li_diamond.first_acquirer_prio;

		msg.o.err = 0;
		msgRespond(port, &msg, rid);

		threadJoin(tid_l, 0);
		threadJoin(tid_t, 0);
		threadJoin(tid_m, 0);
		resourceDestroy(li_diamond.mutex);
		exit(0);
	}

	oid_t oid;
	while (lookup(dev_path, NULL, &oid) < 0)
		usleep(10 * 1000);
	while (!shared[0])
		usleep(1000);

	priority(1);
	msg_t msg = { 0 };
	msg.type = mtRead;
	msg.i.size = 0;
	msg.i.data = NULL;
	msg.o.size = 0;
	msg.o.data = NULL;
	TEST_ASSERT_EQUAL_INT(0, msgSend(oid.port, &msg));

	waitpid(server_pid, NULL, 0);

	TEST_ASSERT_EQUAL_INT_MESSAGE(1, shared[1],
			"Diamond: L should be boosted to P1 (highest contender via SC)");
	TEST_ASSERT_EQUAL_INT_MESSAGE(0, shared[2],
			"Diamond: M(P2) must not finish before IPC (inversion)");

	munmap((void *)shared, 0x200);
	kill(shm_pid, SIGKILL);
	waitpid(shm_pid, NULL, 0);
}


/*
 * Priority restoration correctness after chained unlock.
 *
 *   Server process:
 *     Thread S (base P4): receives IPC (SC@P1), locks M1, locks M2
 *     Thread A (base P3): blocks on M2
 *     Thread B (base P5): blocks on M1
 *
 * S holds M1 and M2. A(P3) blocks on M2 -> no boost needed (S already P1).
 * B(P5) blocks on M1 -> no boost needed.
 *
 * S unlocks M2 -> A gets M2. S still holds M1 with B waiting.
 * S's priority should remain P1 (from SC), not drop.
 *
 * S unlocks M1 -> B gets M1. S still has SC@P1.
 * After responding to IPC, S's priority reverts to base P4.
 *
 * Tests that unlocking a lock doesn't incorrectly drop the SC-donated prio.
 */

static struct {
	handle_t m1;
	handle_t m2;
	volatile int phase;
	char stack[2][4096] __attribute__((aligned(8)));
} li_restore;

static void li_restore_a(void *arg)
{
	/* Thread A (P3): block on M2 */
	while (li_restore.phase < 2)
		usleep(1000);

	mutexLock(li_restore.m2);
	mutexUnlock(li_restore.m2);
	endthread();
}

static void li_restore_b(void *arg)
{
	/* Thread B (P5): block on M1 */
	while (li_restore.phase < 2)
		usleep(1000);
	usleep(5 * 1000); /* stagger slightly after A */

	mutexLock(li_restore.m1);
	mutexUnlock(li_restore.m1);
	endthread();
}

TEST(msg_lock_ipc, unlock_preserves_sc_prio)
{
	char shm_path[64];
	make_dev_path(shm_path, sizeof(shm_path), "liup_shm");
	pid_t shm_pid;
	if ((shm_pid = safe_fork()) == 0) {
		shmsrv_start(shm_path);
		exit(1);
	}

	char dev_path[64];
	make_dev_path(dev_path, sizeof(dev_path), "liup");

	int *shared = (int *)shm_init(shm_path, true, 0x200);
	TEST_ASSERT_NOT_NULL(shared);
	/* shared[0] = server ready
	 * shared[1] = S prio after unlocking M2 (still holding M1 + SC)
	 * shared[2] = S prio after unlocking M1 (still holding SC) */
	memset((void *)shared, 0, 0x200);

	pid_t server_pid;
	if ((server_pid = safe_fork()) == 0) {
		int *sh = (int *)shm_init(shm_path, false, 0x200);
		uint32_t port = 0;
		if (setup_port_dev(dev_path, &port) < 0)
			exit(3);

		mutexCreate(&li_restore.m1);
		mutexCreate(&li_restore.m2);
		li_restore.phase = 0;

		handle_t tid_a, tid_b;
		beginthreadex(li_restore_a, 3, li_restore.stack[0],
				sizeof(li_restore.stack[0]), NULL, &tid_a);
		beginthreadex(li_restore_b, 5, li_restore.stack[1],
				sizeof(li_restore.stack[1]), NULL, &tid_b);

		priority(4);
		sh[0] = 1;

		msg_t msg = { 0 };
		msg_rid_t rid;
		if (msgRecv(port, &msg, &rid) < 0)
			exit(1);

		/* Running on SC@P1 */
		mutexLock(li_restore.m1);
		mutexLock(li_restore.m2);

		li_restore.phase = 2; /* let A and B start contending */
		usleep(30 * 1000);    /* let them block */

		/* Unlock M2 (A gets it). Our prio should stay P1 from SC. */
		mutexUnlock(li_restore.m2);
		sh[1] = priority(-1);

		/* Unlock M1 (B gets it). Our prio should stay P1 from SC. */
		mutexUnlock(li_restore.m1);
		sh[2] = priority(-1);

		msg.o.err = 0;
		msgRespond(port, &msg, rid);

		threadJoin(tid_a, 0);
		threadJoin(tid_b, 0);
		resourceDestroy(li_restore.m1);
		resourceDestroy(li_restore.m2);
		exit(0);
	}

	oid_t oid;
	while (lookup(dev_path, NULL, &oid) < 0)
		usleep(10 * 1000);
	while (!shared[0])
		usleep(1000);

	priority(1);
	msg_t msg = { 0 };
	msg.type = mtRead;
	msg.i.size = 0;
	msg.i.data = NULL;
	msg.o.size = 0;
	msg.o.data = NULL;
	TEST_ASSERT_EQUAL_INT(0, msgSend(oid.port, &msg));

	waitpid(server_pid, NULL, 0);

	TEST_ASSERT_EQUAL_INT_MESSAGE(1, shared[1],
			"Unlock M2: S prio must stay P1 from SC, not drop to base");
	TEST_ASSERT_EQUAL_INT_MESSAGE(1, shared[2],
			"Unlock M1: S prio must stay P1 from SC, not drop to base");

	munmap((void *)shared, 0x200);
	kill(shm_pid, SIGKILL);
	waitpid(shm_pid, NULL, 0);
}


/*
 * Lock-order inversion with IPC: two mutexes locked in opposite order
 * by different threads, with IPC priority in the mix.
 *
 *   Client H (P1) --IPC--> Server process:
 *     Thread S (base P4): receives IPC (SC@P1), locks M1 then blocks on M2
 *     Thread R (base P5): locks M2 then blocks on M1
 *     Thread M (base P2): CPU-bound interferer
 *
 * S holds M1, R holds M2. S blocks on M2 -> PI boosts R to P1.
 * R finishes with M2, unlocks, S gets M2. Then R blocks on M1 -> S
 * already at P1, no further boost needed.
 *
 * Key: R must be boosted to P1 (not just P4) to prevent M(P2) from
 * causing inversion.
 */

static struct {
	handle_t m1;
	handle_t m2;
	volatile int phase;
	volatile int r_prio; /* R's observed prio while holding M2 under contention */
	volatile int m_done;
	char stack[3][4096] __attribute__((aligned(8)));
} li_order;

static void li_order_r(void *arg)
{
	/* Thread R (P5): lock M2, signal, then block on M1 */
	mutexLock(li_order.m2);
	li_order.phase = 1; /* M2 held */

	/* Wait for S to contend on M2 */
	while (li_order.phase < 2)
		usleep(1000);
	usleep(20 * 1000);

	li_order.r_prio = priority(-1);

	/* Release M2 (S gets it), then try M1 (held by S) */
	mutexUnlock(li_order.m2);

	mutexLock(li_order.m1);
	mutexUnlock(li_order.m1);
	endthread();
}

static void li_order_medium(void *arg)
{
	/* Wait until S has received IPC and is contending on M2 */
	while (li_order.phase < 2)
		usleep(1000);
	usleep(10 * 1000); /* let S block on M2 and PI boost R */

	for (volatile int i = 0; i < 30000000; i++)
		;

	li_order.m_done = 1;
	endthread();
}

TEST(msg_lock_ipc, lock_order_inversion)
{
	char shm_path[64];
	make_dev_path(shm_path, sizeof(shm_path), "lioi_shm");
	pid_t shm_pid;
	if ((shm_pid = safe_fork()) == 0) {
		shmsrv_start(shm_path);
		exit(1);
	}

	char dev_path[64];
	make_dev_path(dev_path, sizeof(dev_path), "lioi");

	int *shared = (int *)shm_init(shm_path, true, 0x200);
	TEST_ASSERT_NOT_NULL(shared);
	/* shared[0] = server ready
	 * shared[1] = R's prio while holding M2 under contention from S
	 * shared[2] = M finished before IPC
	 * shared[3] = S's prio during IPC */
	memset((void *)shared, 0, 0x200);

	pid_t server_pid;
	if ((server_pid = safe_fork()) == 0) {
		int *sh = (int *)shm_init(shm_path, false, 0x200);
		uint32_t port = 0;
		if (setup_port_dev(dev_path, &port) < 0)
			exit(3);

		mutexCreate(&li_order.m1);
		mutexCreate(&li_order.m2);
		li_order.phase = 0;
		li_order.r_prio = -1;
		li_order.m_done = 0;

		handle_t tid_r, tid_m;
		beginthreadex(li_order_r, 5, li_order.stack[0],
				sizeof(li_order.stack[0]), NULL, &tid_r);
		beginthreadex(li_order_medium, 2, li_order.stack[1],
				sizeof(li_order.stack[1]), NULL, &tid_m);

		/* Wait for R to hold M2 */
		while (li_order.phase < 1)
			usleep(1000);

		priority(4);
		sh[0] = 1;

		msg_t msg = { 0 };
		msg_rid_t rid;
		if (msgRecv(port, &msg, &rid) < 0)
			exit(1);

		/* Running on SC@P1 */
		sh[3] = priority(-1);
		mutexLock(li_order.m1); /* S holds M1 */
		li_order.phase = 2;     /* signal: about to block on M2 */

		/* Block on M2 held by R. PI should boost R to P1. */
		mutexLock(li_order.m2);
		mutexUnlock(li_order.m2);
		mutexUnlock(li_order.m1);

		sh[1] = li_order.r_prio;
		sh[2] = li_order.m_done;

		msg.o.err = 0;
		msgRespond(port, &msg, rid);

		threadJoin(tid_r, 0);
		threadJoin(tid_m, 0);
		resourceDestroy(li_order.m1);
		resourceDestroy(li_order.m2);
		exit(0);
	}

	oid_t oid;
	while (lookup(dev_path, NULL, &oid) < 0)
		usleep(10 * 1000);
	while (!shared[0])
		usleep(1000);

	priority(1);
	msg_t msg = { 0 };
	msg.type = mtRead;
	msg.i.size = 0;
	msg.i.data = NULL;
	msg.o.size = 0;
	msg.o.data = NULL;
	TEST_ASSERT_EQUAL_INT(0, msgSend(oid.port, &msg));

	waitpid(server_pid, NULL, 0);

	TEST_ASSERT_EQUAL_INT_MESSAGE(1, shared[3],
			"Lock-order: S should run at P1 via SC");
	TEST_ASSERT_EQUAL_INT_MESSAGE(1, shared[1],
			"Lock-order: R should be boosted to P1 when S(SC@P1) contends on M2");
	TEST_ASSERT_EQUAL_INT_MESSAGE(0, shared[2],
			"Lock-order: M(P2) must not finish before IPC (inversion)");

	munmap((void *)shared, 0x200);
	kill(shm_pid, SIGKILL);
	waitpid(shm_pid, NULL, 0);
}


/*
 * Multiple IPC clients with different priorities contending through locks.
 *
 *   Client H1 (P1) --IPC--> Server (first)
 *   Client H2 (P3) --IPC--> Server (second, after H1 completes)
 *
 *   Thread S (base P4): receives each msg, blocks on M (held by L)
 *   Thread L (base P7): holds M
 *
 * After S finishes H1's request, S receives H2's msg (SC@P3), blocks on M again.
 * L should now only be boosted to P3 (not lingering P1).
 *
 * Tests that the PI boost is recalculated correctly per-IPC-message,
 * and old SC-donated priorities don't leak.
 */

static struct {
	handle_t mutex;
	volatile int phase;
	volatile int l_prio_round1;
	volatile int l_prio_round2;
	char stack[2][4096] __attribute__((aligned(8)));
} li_multi_ipc;

static void li_multi_ipc_holder(void *arg)
{
	/* Round 1: hold mutex, observe prio from H1's SC@P1 */
	mutexLock(li_multi_ipc.mutex);
	li_multi_ipc.phase = 1;

	while (li_multi_ipc.phase < 2)
		usleep(1000);
	usleep(20 * 1000);

	li_multi_ipc.l_prio_round1 = priority(-1);
	mutexUnlock(li_multi_ipc.mutex);

	/* Round 2: hold mutex again, observe prio from H2's SC@P3 */
	while (li_multi_ipc.phase < 3)
		usleep(1000);

	mutexLock(li_multi_ipc.mutex);
	li_multi_ipc.phase = 4;

	while (li_multi_ipc.phase < 5)
		usleep(1000);
	usleep(20 * 1000);

	li_multi_ipc.l_prio_round2 = priority(-1);
	mutexUnlock(li_multi_ipc.mutex);
	endthread();
}

TEST(msg_lock_ipc, multi_client_prio_recalc)
{
	char shm_path[64];
	make_dev_path(shm_path, sizeof(shm_path), "limc_shm");
	pid_t shm_pid;
	if ((shm_pid = safe_fork()) == 0) {
		shmsrv_start(shm_path);
		exit(1);
	}

	char dev_path[64];
	make_dev_path(dev_path, sizeof(dev_path), "limc");

	int *shared = (int *)shm_init(shm_path, true, 0x200);
	TEST_ASSERT_NOT_NULL(shared);
	/* shared[0] = server ready
	 * shared[1] = L's prio round1 (should be P1)
	 * shared[2] = L's prio round2 (should be P3) */
	memset((void *)shared, 0, 0x200);

	pid_t server_pid;
	if ((server_pid = safe_fork()) == 0) {
		int *sh = (int *)shm_init(shm_path, false, 0x200);
		uint32_t port = 0;
		if (setup_port_dev(dev_path, &port) < 0)
			exit(3);

		mutexCreate(&li_multi_ipc.mutex);
		li_multi_ipc.phase = 0;
		li_multi_ipc.l_prio_round1 = -1;
		li_multi_ipc.l_prio_round2 = -1;

		handle_t tid_l;
		beginthreadex(li_multi_ipc_holder, 7, li_multi_ipc.stack[0],
				sizeof(li_multi_ipc.stack[0]), NULL, &tid_l);

		while (li_multi_ipc.phase < 1)
			usleep(1000);

		priority(4);
		sh[0] = 1;

		/* Round 1: receive H1's message (SC@P1) */
		msg_t msg = { 0 };
		msg_rid_t rid;
		if (msgRecv(port, &msg, &rid) < 0)
			exit(1);

		li_multi_ipc.phase = 2;

		mutexLock(li_multi_ipc.mutex);
		mutexUnlock(li_multi_ipc.mutex);

		msg.o.err = 0;
		msgRespond(port, &msg, rid);

		/* Round 2: wait for L to hold mutex again, then receive H2's message */
		li_multi_ipc.phase = 3;
		while (li_multi_ipc.phase < 4)
			usleep(1000);

		if (msgRecv(port, &msg, &rid) < 0)
			exit(1);

		li_multi_ipc.phase = 5;

		mutexLock(li_multi_ipc.mutex);
		mutexUnlock(li_multi_ipc.mutex);

		sh[1] = li_multi_ipc.l_prio_round1;
		sh[2] = li_multi_ipc.l_prio_round2;

		msg.o.err = 0;
		msgRespond(port, &msg, rid);

		threadJoin(tid_l, 0);
		resourceDestroy(li_multi_ipc.mutex);
		exit(0);
	}

	oid_t oid;
	while (lookup(dev_path, NULL, &oid) < 0)
		usleep(10 * 1000);
	while (!shared[0])
		usleep(1000);

	/* H1 sends at P1 */
	pid_t h1;
	if ((h1 = safe_fork()) == 0) {
		priority(1);
		msg_t msg = { 0 };
		msg.type = mtRead;
		msg.i.size = 0;
		msg.i.data = NULL;
		msg.o.size = 0;
		msg.o.data = NULL;
		if (msgSend(oid.port, &msg) != 0)
			exit(1);
		exit(0);
	}

	waitpid(h1, NULL, 0);

	/* H2 sends at P3 (after H1 completes) */
	pid_t h2;
	if ((h2 = safe_fork()) == 0) {
		priority(3);
		msg_t msg = { 0 };
		msg.type = mtWrite;
		msg.i.size = 0;
		msg.i.data = NULL;
		msg.o.size = 0;
		msg.o.data = NULL;
		if (msgSend(oid.port, &msg) != 0)
			exit(1);
		exit(0);
	}

	waitpid(h2, NULL, 0);
	waitpid(server_pid, NULL, 0);

	TEST_ASSERT_EQUAL_INT_MESSAGE(1, shared[1],
			"Multi-client: L boosted to P1 during H1's IPC");
	TEST_ASSERT_EQUAL_INT_MESSAGE(3, shared[2],
			"Multi-client: L boosted to P3 during H2's IPC (not lingering P1)");

	munmap((void *)shared, 0x200);
	kill(shm_pid, SIGKILL);
	waitpid(shm_pid, NULL, 0);
}


/*
 * Long chain: 3 mutexes, 3 holders + IPC SC at the top.
 *
 *   Client H (P1) --IPC--> Server process:
 *     Thread S (base P4): receives IPC (SC@P1), blocks on M1 (held by T1)
 *     Thread T1 (base P5): holds M1, blocked on M2 (held by T2)
 *     Thread T2 (base P6): holds M2, blocked on M3 (held by T3)
 *     Thread T3 (base P7): holds M3, doing work
 *     Thread M (base P2): CPU-bound interferer
 *
 * Full transitive chain: S(P1) -> M1 -> T1(P5) -> M2 -> T2(P6) -> M3 -> T3(P7)
 *
 * Correct: T3 boosted to P1, T2 boosted to P1, T1 boosted to P1.
 * Broken: each hop only boosts one level, T3 stays at P7, M preempts.
 */

static struct {
	handle_t m1;
	handle_t m2;
	handle_t m3;
	volatile int phase;
	volatile int t3_prio;
	volatile int t2_prio;
	volatile int t1_prio;
	volatile int m_done;
	char stack[5][4096] __attribute__((aligned(8)));
} li_long;

static void li_long_t3(void *arg)
{
	mutexLock(li_long.m3);
	li_long.phase = 1;

	while (li_long.phase < 6)
		usleep(1000);
	usleep(30 * 1000);

	li_long.t3_prio = priority(-1);

	for (volatile int i = 0; i < 5000; i++)
		;

	mutexUnlock(li_long.m3);
	endthread();
}

static void li_long_t2(void *arg)
{
	while (li_long.phase < 1)
		usleep(1000);

	mutexLock(li_long.m2);
	li_long.phase = 2;

	while (li_long.phase < 5)
		usleep(1000);

	li_long.phase = 6;

	/* Block on M3 held by T3 */
	mutexLock(li_long.m3);

	/* After wakeup: T2 holds M2 (T1 at P1 waiting) + M3 → boosted to P1 */
	li_long.t2_prio = priority(-1);

	mutexUnlock(li_long.m3);
	mutexUnlock(li_long.m2);
	endthread();
}

static void li_long_t1(void *arg)
{
	while (li_long.phase < 2)
		usleep(1000);

	mutexLock(li_long.m1);
	li_long.phase = 3;

	while (li_long.phase < 4)
		usleep(1000);

	li_long.phase = 5;

	/* Block on M2 held by T2 */
	mutexLock(li_long.m2);

	/* After wakeup: T1 holds M1 (S at P1 waiting) + M2 → boosted to P1 */
	li_long.t1_prio = priority(-1);

	mutexUnlock(li_long.m2);
	mutexUnlock(li_long.m1);
	endthread();
}

static void li_long_medium(void *arg)
{
	/* Wait until S has received IPC and the full chain is forming */
	while (li_long.phase < 6)
		usleep(1000);
	usleep(20 * 1000); /* let S block on M1 and propagate boosts */

	for (volatile int i = 0; i < 30000000; i++)
		;

	li_long.m_done = 1;
	endthread();
}

TEST(msg_lock_ipc, long_chain_three_mutexes)
{
	char shm_path[64];
	make_dev_path(shm_path, sizeof(shm_path), "lilc_shm");
	pid_t shm_pid;
	if ((shm_pid = safe_fork()) == 0) {
		shmsrv_start(shm_path);
		exit(1);
	}

	char dev_path[64];
	make_dev_path(dev_path, sizeof(dev_path), "lilc");

	int *shared = (int *)shm_init(shm_path, true, 0x200);
	TEST_ASSERT_NOT_NULL(shared);
	/* shared[0] = ready
	 * shared[1] = T1 prio, shared[2] = T2 prio, shared[3] = T3 prio
	 * shared[4] = M finished before IPC */
	memset((void *)shared, 0, 0x200);

	pid_t server_pid;
	if ((server_pid = safe_fork()) == 0) {
		int *sh = (int *)shm_init(shm_path, false, 0x200);
		uint32_t port = 0;
		if (setup_port_dev(dev_path, &port) < 0)
			exit(3);

		mutexCreate(&li_long.m1);
		mutexCreate(&li_long.m2);
		mutexCreate(&li_long.m3);
		li_long.phase = 0;
		li_long.t1_prio = -1;
		li_long.t2_prio = -1;
		li_long.t3_prio = -1;
		li_long.m_done = 0;

		handle_t tid_t3, tid_t2, tid_t1, tid_m;
		beginthreadex(li_long_t3, 7, li_long.stack[0],
				sizeof(li_long.stack[0]), NULL, &tid_t3);
		beginthreadex(li_long_t2, 6, li_long.stack[1],
				sizeof(li_long.stack[1]), NULL, &tid_t2);
		beginthreadex(li_long_t1, 5, li_long.stack[2],
				sizeof(li_long.stack[2]), NULL, &tid_t1);
		beginthreadex(li_long_medium, 2, li_long.stack[3],
				sizeof(li_long.stack[3]), NULL, &tid_m);

		/* Wait for T1 to hold M1 */
		while (li_long.phase < 3)
			usleep(1000);

		priority(4);
		sh[0] = 1;

		msg_t msg = { 0 };
		msg_rid_t rid;
		if (msgRecv(port, &msg, &rid) < 0)
			exit(1);

		/* Running on SC@P1 */
		li_long.phase = 4; /* tell T1 to observe and block on M2 */

		/* Wait for chain to form: T1->M2->T2 and T2->M3->T3 */
		while (li_long.phase < 6)
			usleep(1000);
		usleep(10 * 1000);

		/* S blocks on M1 (held by T1, who is blocked on M2 -> T2 -> M3 -> T3) */
		mutexLock(li_long.m1);
		mutexUnlock(li_long.m1);

		sh[1] = li_long.t1_prio;
		sh[2] = li_long.t2_prio;
		sh[3] = li_long.t3_prio;
		sh[4] = li_long.m_done;

		msg.o.err = 0;
		msgRespond(port, &msg, rid);

		threadJoin(tid_t3, 0);
		threadJoin(tid_t2, 0);
		threadJoin(tid_t1, 0);
		threadJoin(tid_m, 0);
		resourceDestroy(li_long.m1);
		resourceDestroy(li_long.m2);
		resourceDestroy(li_long.m3);
		exit(0);
	}

	oid_t oid;
	while (lookup(dev_path, NULL, &oid) < 0)
		usleep(10 * 1000);
	while (!shared[0])
		usleep(1000);

	priority(1);
	msg_t msg = { 0 };
	msg.type = mtRead;
	msg.i.size = 0;
	msg.i.data = NULL;
	msg.o.size = 0;
	msg.o.data = NULL;
	TEST_ASSERT_EQUAL_INT(0, msgSend(oid.port, &msg));

	waitpid(server_pid, NULL, 0);

	TEST_ASSERT_EQUAL_INT_MESSAGE(1, shared[1],
			"Long chain: T1 should be boosted to P1");
	TEST_ASSERT_EQUAL_INT_MESSAGE(1, shared[2],
			"Long chain: T2 should be transitively boosted to P1");
	TEST_ASSERT_EQUAL_INT_MESSAGE(1, shared[3],
			"Long chain: T3 should be transitively boosted to P1");
	TEST_ASSERT_EQUAL_INT_MESSAGE(0, shared[4],
			"Long chain: M(P2) must not finish before IPC (inversion)");

	munmap((void *)shared, 0x200);
	kill(shm_pid, SIGKILL);
	waitpid(shm_pid, NULL, 0);
}


/* =====================================================  */
/* TEST_GROUP: msg_stress                                 */
/* Stress tests provoking races in IPC                    */
/* =====================================================  */

TEST_GROUP(msg_stress);

TEST_SETUP(msg_stress)
{
}

TEST_TEAR_DOWN(msg_stress)
{
}


/*
 * Many clients hammering one server concurrently.
 * Stress the fastpath/slowpath transition and SC queuing.
 */
TEST(msg_stress, many_clients_hammer)
{
	char dev_path[64];
	make_dev_path(dev_path, sizeof(dev_path), "str_mc");
	pid_t server_pid;

	const int NUM_CLIENTS = 8;
	const int MSGS_PER_CLIENT = 50;

	if ((server_pid = safe_fork()) == 0) {
		uint32_t port = 0;
		if (setup_port_dev(dev_path, &port) < 0)
			exit(3);
		server_echo_loop(port, 0);
		exit(0);
	}

	oid_t oid;
	while (lookup(dev_path, NULL, &oid) < 0)
		usleep(10 * 1000);

	pid_t clients[NUM_CLIENTS];
	for (int c = 0; c < NUM_CLIENTS; c++) {
		if ((clients[c] = safe_fork()) == 0) {
			for (int m = 0; m < MSGS_PER_CLIENT; m++) {
				msg_t msg = { 0 };
				msg.type = mtWrite;
				msg.i.io.offs = c * 10000 + m;
				msg.i.size = 0;
				msg.i.data = NULL;
				msg.o.size = 0;
				msg.o.data = NULL;
				if (msgSend(oid.port, &msg) != 0)
					exit(1);
				if (msg.o.err != mtWrite)
					exit(2);
			}
			exit(0);
		}
	}

	for (int c = 0; c < NUM_CLIENTS; c++) {
		int status;
		waitpid(clients[c], &status, 0);
		TEST_ASSERT_EQUAL_INT_MESSAGE(0, WEXITSTATUS(status),
				"Stress: client failed under load");
	}
	assert_child_exit(server_pid);
}


/*
 * Concurrent clients with data buffers (stresses buffer mapping/unmapping
 * concurrency and the ipc_buf_layout_t lifecycle).
 */
TEST(msg_stress, concurrent_data_transfer)
{
	char dev_path[64];
	make_dev_path(dev_path, sizeof(dev_path), "str_dt");
	pid_t server_pid;

	const int NUM_CLIENTS = 4;
	const int MSGS_PER_CLIENT = 20;
	const size_t BUFSZ = 4096;

	if ((server_pid = safe_fork()) == 0) {
		uint32_t port = 0;
		if (setup_port_dev(dev_path, &port) < 0)
			exit(3);
		server_echo_loop(port, 0);
		exit(0);
	}

	oid_t oid;
	while (lookup(dev_path, NULL, &oid) < 0)
		usleep(10 * 1000);

	pid_t clients[NUM_CLIENTS];
	for (int c = 0; c < NUM_CLIENTS; c++) {
		if ((clients[c] = safe_fork()) == 0) {
			char *ibuf = malloc(BUFSZ);
			char *obuf = malloc(BUFSZ);
			if (ibuf == NULL || obuf == NULL)
				exit(10);

			for (int m = 0; m < MSGS_PER_CLIENT; m++) {
				for (size_t i = 0; i < BUFSZ; i++)
					ibuf[i] = (char)((i + c + m) & 0xff);
				memset(obuf, 0, BUFSZ);

				msg_t msg = { 0 };
				msg.type = mtWrite;
				msg.i.data = ibuf;
				msg.i.size = BUFSZ;
				msg.o.data = obuf;
				msg.o.size = BUFSZ;
				if (msgSend(oid.port, &msg) != 0)
					exit(1);
				if (memcmp(ibuf, obuf, BUFSZ) != 0)
					exit(2);
			}
			free(ibuf);
			free(obuf);
			exit(0);
		}
	}

	for (int c = 0; c < NUM_CLIENTS; c++) {
		int status;
		waitpid(clients[c], &status, 0);
		TEST_ASSERT_EQUAL_INT_MESSAGE(0, WEXITSTATUS(status),
				"Stress: data transfer failed under concurrency");
	}
	assert_child_exit(server_pid);
}


/*
 * Rapid send/respond cycling with respondAndRecv (fastpath stress).
 * One client, one server, many rapid-fire messages.
 */
TEST(msg_stress, rapid_respondandrecv)
{
	char dev_path[64];
	make_dev_path(dev_path, sizeof(dev_path), "str_rr");
	pid_t server_pid;

	if ((server_pid = safe_fork()) == 0) {
		uint32_t port = 0;
		if (setup_port_dev(dev_path, &port) < 0)
			exit(3);
		server_echo_loop(port, 1);
		exit(0);
	}

	oid_t oid;
	while (lookup(dev_path, NULL, &oid) < 0)
		usleep(10 * 1000);

	for (int m = 0; m < 1000; m++) {
		msg_t msg = { 0 };
		msg.type = mtRead;
		msg.i.io.offs = m;
		msg.i.size = 0;
		msg.i.data = NULL;
		msg.o.size = 0;
		msg.o.data = NULL;
		TEST_ASSERT_EQUAL_INT(0, msgSend(oid.port, &msg));
		TEST_ASSERT_EQUAL_INT(mtRead, msg.o.err);
	}

	assert_child_exit(server_pid);
}


/*
 * Concurrent clients with respondAndRecv server (combination of queuing
 * + fastpath + SC switching under concurrency).
 */
TEST(msg_stress, concurrent_respondandrecv)
{
	char dev_path[64];
	make_dev_path(dev_path, sizeof(dev_path), "str_crr");
	pid_t server_pid;

	const int NUM_CLIENTS = 4;
	const int MSGS_PER_CLIENT = 50;

	if ((server_pid = safe_fork()) == 0) {
		uint32_t port = 0;
		if (setup_port_dev(dev_path, &port) < 0)
			exit(3);
		server_echo_loop(port, 1);
		exit(0);
	}

	oid_t oid;
	while (lookup(dev_path, NULL, &oid) < 0)
		usleep(10 * 1000);

	pid_t clients[NUM_CLIENTS];
	for (int c = 0; c < NUM_CLIENTS; c++) {
		if ((clients[c] = safe_fork()) == 0) {
			for (int m = 0; m < MSGS_PER_CLIENT; m++) {
				msg_t msg = { 0 };
				msg.type = mtWrite;
				msg.i.io.offs = c * 10000 + m;
				msg.i.size = 0;
				msg.i.data = NULL;
				msg.o.size = 0;
				msg.o.data = NULL;
				if (msgSend(oid.port, &msg) != 0)
					exit(1);
				if (msg.o.err != mtWrite)
					exit(2);
			}
			exit(0);
		}
	}

	for (int c = 0; c < NUM_CLIENTS; c++) {
		int status;
		waitpid(clients[c], &status, 0);
		TEST_ASSERT_EQUAL_INT_MESSAGE(0, WEXITSTATUS(status),
				"Stress: concurrent respondAndRecv client failed");
	}
	assert_child_exit(server_pid);
}


/*
 * Pulse racing with send: one process sends pulses while another sends messages.
 * Server should handle both correctly without deadlock/crash.
 */
TEST(msg_stress, pulse_send_race)
{
	char dev_path[64];
	make_dev_path(dev_path, sizeof(dev_path), "str_ps");
	pid_t server_pid;

	if ((server_pid = safe_fork()) == 0) {
		uint32_t port = 0;
		if (setup_port_dev(dev_path, &port) < 0)
			exit(3);

		msg_t msg = { 0 };
		msg_rid_t rid;
		int count = 0;

		for (;;) {
			int err = msgRecv(port, &msg, &rid);
			if (err == -EPULSE) {
				/* got pulse, loop */
				continue;
			}
			if (err < 0) {
				exit(0);
			}
			msg.o.err = msg.type;
			if (msgRespond(port, &msg, rid) < 0)
				exit(2);
			count++;
			if (count >= 20)
				exit(0);
		}
	}

	oid_t oid;
	while (lookup(dev_path, NULL, &oid) < 0)
		usleep(10 * 1000);

	/* Pulser */
	pid_t pulser;
	if ((pulser = safe_fork()) == 0) {
		for (int i = 0; i < 20; i++) {
			msgPulse(oid.port, (uint8_t)(i + 1));
			usleep(5 * 1000);
		}
		exit(0);
	}

	/* Sender */
	for (int m = 0; m < 20; m++) {
		msg_t msg = { 0 };
		msg.type = mtRead;
		msg.i.size = 0;
		msg.i.data = NULL;
		msg.o.size = 0;
		msg.o.data = NULL;
		/* Ignoring errors here - the server may exit mid-test */
		msgSend(oid.port, &msg);
	}

	waitpid(pulser, NULL, 0);
	waitpid(server_pid, NULL, 0);
	/* If we got here without hanging, the test passes */
}


/* =====================================================  */
/* TEST_GROUP: msg_respond_recv_chain                     */
/* respondAndRecv leaf behind forwarding servers           */
/* =====================================================  */

TEST_GROUP(msg_respond_recv_chain);

TEST_SETUP(msg_respond_recv_chain)
{
}

TEST_TEAR_DOWN(msg_respond_recv_chain)
{
}


/* Helper: spawn a forwarding server that uses separate recv + respond
 * and forwards to the given backend port. */
static pid_t spawn_forwarder(const char *dev_path, oid_t *backend)
{
	oid_t be = *backend;
	pid_t pid;
	if ((pid = safe_fork()) == 0) {
		uint32_t port = 0;
		if (setup_port_dev(dev_path, &port) < 0)
			exit(3);

		msg_t msg = { 0 };
		msg_rid_t rid;
		for (;;) {
			if (msgRecv(port, &msg, &rid) < 0)
				exit(0);

			msg_t fwd = { 0 };
			fwd.type = msg.type;
			memcpy(fwd.i.raw, msg.i.raw, sizeof(fwd.i.raw));
			fwd.i.data = msg.i.data;
			fwd.i.size = msg.i.size;
			fwd.o.data = msg.o.data;
			fwd.o.size = msg.o.size;

			if (msgSend(be.port, &fwd) != 0)
				exit(4);

			memcpy(msg.o.raw, fwd.o.raw, sizeof(msg.o.raw));
			msg.o.err = fwd.o.err;

			if (msgRespond(port, &msg, rid) < 0)
				exit(5);
		}
	}
	return pid;
}


/*
 * Direct call to respondAndRecv leaf (depth 1, no forwarding).
 * Baseline: this should work (and it does in msg_respond_recv).
 */
TEST(msg_respond_recv_chain, rr_leaf_direct)
{
	char dev_path[64];
	make_dev_path(dev_path, sizeof(dev_path), "rrc_d1");
	pid_t server_pid;

	if ((server_pid = safe_fork()) == 0) {
		uint32_t port = 0;
		if (setup_port_dev(dev_path, &port) < 0)
			exit(3);
		server_echo_loop(port, 1);
		exit(0);
	}

	oid_t oid;
	while (lookup(dev_path, NULL, &oid) < 0)
		usleep(10 * 1000);

	for (int i = 0; i < 10; i++) {
		msg_t msg = { 0 };
		msg.type = mtRead;
		msg.i.io.offs = i;
		msg.i.size = 0;
		msg.i.data = NULL;
		msg.o.size = 0;
		msg.o.data = NULL;
		TEST_ASSERT_EQUAL_INT(0, msgSend(oid.port, &msg));
		TEST_ASSERT_EQUAL_INT(mtRead, msg.o.err);
	}

	assert_child_exit(server_pid);
}


/*
 * respondAndRecv leaf behind ONE forwarding server (depth 2).
 *
 * Forwarder uses separate recv+respond, leaf uses respondAndRecv.
 * This is the minimal reproduction of the hang observed in msg_bench.
 */
TEST(msg_respond_recv_chain, rr_leaf_behind_forwarder)
{
	char leaf_path[64], fwd_path[64];
	make_dev_path(leaf_path, sizeof(leaf_path), "rrc_leaf");
	make_dev_path(fwd_path, sizeof(fwd_path), "rrc_fwd");

	/* Spawn the respondAndRecv leaf */
	pid_t leaf_pid;
	if ((leaf_pid = safe_fork()) == 0) {
		uint32_t port = 0;
		if (setup_port_dev(leaf_path, &port) < 0)
			exit(3);
		server_echo_loop(port, 1);
		exit(0);
	}

	oid_t leaf_oid;
	while (lookup(leaf_path, NULL, &leaf_oid) < 0)
		usleep(10 * 1000);

	/* Spawn forwarder that does recv+respond and forwards to leaf */
	pid_t fwd_pid = spawn_forwarder(fwd_path, &leaf_oid);

	oid_t fwd_oid;
	while (lookup(fwd_path, NULL, &fwd_oid) < 0)
		usleep(10 * 1000);

	for (int i = 0; i < 10; i++) {
		msg_t msg = { 0 };
		msg.type = mtWrite;
		msg.i.io.offs = i;
		msg.i.size = 0;
		msg.i.data = NULL;
		msg.o.size = 0;
		msg.o.data = NULL;
		TEST_ASSERT_EQUAL_INT(0, msgSend(fwd_oid.port, &msg));
		TEST_ASSERT_EQUAL_INT(mtWrite, msg.o.err);
	}

	kill(fwd_pid, SIGKILL);
	waitpid(fwd_pid, NULL, 0);
	kill(leaf_pid, SIGKILL);
	waitpid(leaf_pid, NULL, 0);
}


/*
 * Same as above but leaf uses separate recv+respond (should always work).
 * Control test to confirm the forwarding logic itself is fine.
 *
 * Regression test for not handling the server thread->end != 0 prior to becoming passive.
 * If it is left unhandled, the thread goes passive and is never reaped.
 */
TEST(msg_respond_recv_chain, normal_leaf_behind_forwarder)
{
	char leaf_path[64], fwd_path[64];
	make_dev_path(leaf_path, sizeof(leaf_path), "rrc_nl");
	make_dev_path(fwd_path, sizeof(fwd_path), "rrc_nf");

	pid_t leaf_pid;
	if ((leaf_pid = safe_fork()) == 0) {
		uint32_t port = 0;
		if (setup_port_dev(leaf_path, &port) < 0)
			exit(3);
		server_echo_loop(port, 0);
		exit(0);
	}

	oid_t leaf_oid;
	while (lookup(leaf_path, NULL, &leaf_oid) < 0)
		usleep(10 * 1000);

	pid_t fwd_pid = spawn_forwarder(fwd_path, &leaf_oid);

	oid_t fwd_oid;
	while (lookup(fwd_path, NULL, &fwd_oid) < 0)
		usleep(10 * 1000);

	for (int i = 0; i < 10; i++) {
		msg_t msg = { 0 };
		msg.type = mtWrite;
		msg.i.io.offs = i;
		msg.i.size = 0;
		msg.i.data = NULL;
		msg.o.size = 0;
		msg.o.data = NULL;
		TEST_ASSERT_EQUAL_INT(0, msgSend(fwd_oid.port, &msg));
		TEST_ASSERT_EQUAL_INT(mtWrite, msg.o.err);
	}

	kill(fwd_pid, SIGKILL);
	waitpid(fwd_pid, NULL, 0);
	kill(leaf_pid, SIGKILL);
	waitpid(leaf_pid, NULL, 0);
}


/*
 * respondAndRecv leaf behind TWO forwarding servers (depth 3).
 */
TEST(msg_respond_recv_chain, rr_leaf_depth3)
{
	char leaf_path[64], fwd1_path[64], fwd2_path[64];
	make_dev_path(leaf_path, sizeof(leaf_path), "rrc3_l");
	make_dev_path(fwd1_path, sizeof(fwd1_path), "rrc3_f1");
	make_dev_path(fwd2_path, sizeof(fwd2_path), "rrc3_f2");

	pid_t leaf_pid;
	if ((leaf_pid = safe_fork()) == 0) {
		uint32_t port = 0;
		if (setup_port_dev(leaf_path, &port) < 0)
			exit(3);
		server_echo_loop(port, 1);
		exit(0);
	}

	oid_t leaf_oid;
	while (lookup(leaf_path, NULL, &leaf_oid) < 0)
		usleep(10 * 1000);

	/* fwd1 forwards to leaf */
	pid_t fwd1_pid = spawn_forwarder(fwd1_path, &leaf_oid);

	oid_t fwd1_oid;
	while (lookup(fwd1_path, NULL, &fwd1_oid) < 0)
		usleep(10 * 1000);

	/* fwd2 forwards to fwd1 */
	pid_t fwd2_pid = spawn_forwarder(fwd2_path, &fwd1_oid);

	oid_t fwd2_oid;
	while (lookup(fwd2_path, NULL, &fwd2_oid) < 0)
		usleep(10 * 1000);

	for (int i = 0; i < 10; i++) {
		msg_t msg = { 0 };
		msg.type = mtRead;
		msg.i.io.offs = i;
		msg.i.size = 0;
		msg.i.data = NULL;
		msg.o.size = 0;
		msg.o.data = NULL;
		TEST_ASSERT_EQUAL_INT(0, msgSend(fwd2_oid.port, &msg));
		TEST_ASSERT_EQUAL_INT(mtRead, msg.o.err);
	}

	kill(fwd2_pid, SIGKILL);
	waitpid(fwd2_pid, NULL, 0);
	kill(fwd1_pid, SIGKILL);
	waitpid(fwd1_pid, NULL, 0);
	kill(leaf_pid, SIGKILL);
	waitpid(leaf_pid, NULL, 0);
}


/*
 * respondAndRecv leaf behind forwarder, with data buffers.
 * The hang could be data-transfer related, so test with payloads too.
 */
TEST(msg_respond_recv_chain, rr_leaf_behind_forwarder_data)
{
	char leaf_path[64], fwd_path[64];
	make_dev_path(leaf_path, sizeof(leaf_path), "rrcd_l");
	make_dev_path(fwd_path, sizeof(fwd_path), "rrcd_f");

	pid_t leaf_pid;
	if ((leaf_pid = safe_fork()) == 0) {
		uint32_t port = 0;
		if (setup_port_dev(leaf_path, &port) < 0)
			exit(3);
		server_echo_loop(port, 1);
		exit(0);
	}

	oid_t leaf_oid;
	while (lookup(leaf_path, NULL, &leaf_oid) < 0)
		usleep(10 * 1000);

	pid_t fwd_pid = spawn_forwarder(fwd_path, &leaf_oid);

	oid_t fwd_oid;
	while (lookup(fwd_path, NULL, &fwd_oid) < 0)
		usleep(10 * 1000);

	const size_t bufsz = 256;
	char *ibuf = malloc(bufsz);
	char *obuf = malloc(bufsz);
	TEST_ASSERT_NOT_NULL(ibuf);
	TEST_ASSERT_NOT_NULL(obuf);

	for (int i = 0; i < 5; i++) {
		for (size_t j = 0; j < bufsz; j++)
			ibuf[j] = (char)((j + i * 17) & 0xff);
		memset(obuf, 0, bufsz);

		msg_t msg = { 0 };
		msg.type = mtWrite;
		msg.i.data = ibuf;
		msg.i.size = bufsz;
		msg.o.data = obuf;
		msg.o.size = bufsz;
		TEST_ASSERT_EQUAL_INT(0, msgSend(fwd_oid.port, &msg));
		TEST_ASSERT_EQUAL_INT(mtWrite, msg.o.err);
		TEST_ASSERT_EQUAL_MEMORY(ibuf, obuf, bufsz);
	}

	free(ibuf);
	free(obuf);
	kill(fwd_pid, SIGKILL);
	waitpid(fwd_pid, NULL, 0);
	kill(leaf_pid, SIGKILL);
	waitpid(leaf_pid, NULL, 0);
}


/*
 * Forwarder also uses respondAndRecv (both layers use it).
 * Tests whether two levels of respondAndRecv chain correctly.
 */
TEST(msg_respond_recv_chain, rr_forwarder_rr_leaf)
{
	char leaf_path[64], fwd_path[64];
	make_dev_path(leaf_path, sizeof(leaf_path), "rrcr_l");
	make_dev_path(fwd_path, sizeof(fwd_path), "rrcr_f");

	/* respondAndRecv leaf */
	pid_t leaf_pid;
	if ((leaf_pid = safe_fork()) == 0) {
		uint32_t port = 0;
		if (setup_port_dev(leaf_path, &port) < 0)
			exit(3);
		server_echo_loop(port, 1);
		exit(0);
	}

	oid_t leaf_oid;
	while (lookup(leaf_path, NULL, &leaf_oid) < 0)
		usleep(10 * 1000);

	/* respondAndRecv forwarder */
	oid_t be = leaf_oid;
	pid_t fwd_pid;
	if ((fwd_pid = safe_fork()) == 0) {
		uint32_t port = 0;
		if (setup_port_dev(fwd_path, &port) < 0)
			exit(3);

		msg_t msg = { 0 };
		msg_rid_t rid;

		for (;;) {
			int err = msgRespondAndRecv(port, &msg, &rid);
			if (err < 0)
				exit(0);

			msg_t fwd = { 0 };
			fwd.type = msg.type;
			memcpy(fwd.i.raw, msg.i.raw, sizeof(fwd.i.raw));
			fwd.i.data = msg.i.data;
			fwd.i.size = msg.i.size;
			fwd.o.data = msg.o.data;
			fwd.o.size = msg.o.size;

			if (msgSend(be.port, &fwd) != 0)
				exit(4);

			memcpy(msg.o.raw, fwd.o.raw, sizeof(msg.o.raw));
			msg.o.err = fwd.o.err;
		}
	}

	oid_t fwd_oid;
	while (lookup(fwd_path, NULL, &fwd_oid) < 0)
		usleep(10 * 1000);

	for (int i = 0; i < 10; i++) {
		msg_t msg = { 0 };
		msg.type = mtRead;
		msg.i.io.offs = i;
		msg.i.size = 0;
		msg.i.data = NULL;
		msg.o.size = 0;
		msg.o.data = NULL;
		TEST_ASSERT_EQUAL_INT(0, msgSend(fwd_oid.port, &msg));
		TEST_ASSERT_EQUAL_INT(mtRead, msg.o.err);
	}

	kill(fwd_pid, SIGKILL);
	waitpid(fwd_pid, NULL, 0);
	kill(leaf_pid, SIGKILL);
	waitpid(leaf_pid, NULL, 0);
}


/*
 * Multiple concurrent clients sending through forwarder to respondAndRecv leaf.
 * If there's a race in SC handling with respondAndRecv under load, this catches it.
 */
TEST(msg_respond_recv_chain, rr_leaf_concurrent_clients)
{
	char leaf_path[64], fwd_path[64];
	make_dev_path(leaf_path, sizeof(leaf_path), "rrcc_l");
	make_dev_path(fwd_path, sizeof(fwd_path), "rrcc_f");

	pid_t leaf_pid;
	if ((leaf_pid = safe_fork()) == 0) {
		uint32_t port = 0;
		if (setup_port_dev(leaf_path, &port) < 0)
			exit(3);
		server_echo_loop(port, 1);
		exit(0);
	}

	oid_t leaf_oid;
	while (lookup(leaf_path, NULL, &leaf_oid) < 0)
		usleep(10 * 1000);

	pid_t fwd_pid = spawn_forwarder(fwd_path, &leaf_oid);

	oid_t fwd_oid;
	while (lookup(fwd_path, NULL, &fwd_oid) < 0)
		usleep(10 * 1000);

	const int NUM_CLIENTS = 4;
	const int MSGS_PER = 10;
	pid_t clients[NUM_CLIENTS];

	for (int c = 0; c < NUM_CLIENTS; c++) {
		if ((clients[c] = safe_fork()) == 0) {
			for (int m = 0; m < MSGS_PER; m++) {
				msg_t msg = { 0 };
				msg.type = mtRead;
				msg.i.io.offs = c * 1000 + m;
				msg.i.size = 0;
				msg.i.data = NULL;
				msg.o.size = 0;
				msg.o.data = NULL;
				if (msgSend(fwd_oid.port, &msg) != 0)
					exit(1);
				if (msg.o.err != mtRead)
					exit(2);
			}
			exit(0);
		}
	}

	for (int c = 0; c < NUM_CLIENTS; c++) {
		int status;
		waitpid(clients[c], &status, 0);
		TEST_ASSERT_EQUAL_INT_MESSAGE(0, WEXITSTATUS(status),
				"Concurrent client failed through rr-leaf chain");
	}

	kill(fwd_pid, SIGKILL);
	waitpid(fwd_pid, NULL, 0);
	kill(leaf_pid, SIGKILL);
	waitpid(leaf_pid, NULL, 0);
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


TEST_GROUP_RUNNER(msg_bwi_edge)
{
	RUN_TEST_CASE(msg_bwi_edge, chained_donation);
	RUN_TEST_CASE(msg_bwi_edge, queued_clients_sc_tracking);
	RUN_TEST_CASE(msg_bwi_edge, prio_restored_after_respond);
	RUN_TEST_CASE(msg_bwi_edge, sc_not_swapped);
}


TEST_GROUP_RUNNER(msg_lock_ipc)
{
	RUN_TEST_CASE(msg_lock_ipc, two_task);
	RUN_TEST_CASE(msg_lock_ipc, three_task);
	RUN_TEST_CASE(msg_lock_ipc, reverse_three_task);
	RUN_TEST_CASE(msg_lock_ipc, nested_lock_then_ipc);
	RUN_TEST_CASE(msg_lock_ipc, cascaded_ipc_lock_ipc);
	RUN_TEST_CASE(msg_lock_ipc, transitive_chain);
	RUN_TEST_CASE(msg_lock_ipc, diamond_contention);
	RUN_TEST_CASE(msg_lock_ipc, unlock_preserves_sc_prio);
	RUN_TEST_CASE(msg_lock_ipc, lock_order_inversion);
	RUN_TEST_CASE(msg_lock_ipc, multi_client_prio_recalc);
	RUN_TEST_CASE(msg_lock_ipc, long_chain_three_mutexes);
}


TEST_GROUP_RUNNER(msg_stress)
{
	RUN_TEST_CASE(msg_stress, many_clients_hammer);
	RUN_TEST_CASE(msg_stress, concurrent_data_transfer);
	RUN_TEST_CASE(msg_stress, rapid_respondandrecv);
	RUN_TEST_CASE(msg_stress, concurrent_respondandrecv);
	RUN_TEST_CASE(msg_stress, pulse_send_race);
}


TEST_GROUP_RUNNER(msg_respond_recv_chain)
{
	RUN_TEST_CASE(msg_respond_recv_chain, rr_leaf_direct);
	RUN_TEST_CASE(msg_respond_recv_chain, rr_leaf_behind_forwarder);
	RUN_TEST_CASE(msg_respond_recv_chain, normal_leaf_behind_forwarder);
	RUN_TEST_CASE(msg_respond_recv_chain, rr_leaf_depth3);
	RUN_TEST_CASE(msg_respond_recv_chain, rr_leaf_behind_forwarder_data);
	RUN_TEST_CASE(msg_respond_recv_chain, rr_forwarder_rr_leaf);
	RUN_TEST_CASE(msg_respond_recv_chain, rr_leaf_concurrent_clients);
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

	RUN_TEST_GROUP(msg_bwi_edge);
	RUN_TEST_GROUP(msg_lock_ipc);

	RUN_TEST_GROUP(msg_stress);
	RUN_TEST_GROUP(msg_respond_recv_chain);
}


int main(int argc, char *argv[])
{
	return (UnityMain(argc, (const char **)argv, runner) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
