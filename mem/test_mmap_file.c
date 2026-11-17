/*
 * Phoenix-RTOS
 *
 * phoenix-rtos-test
 *
 * file-backed and contiguous mmap tests
 *
 * Copyright 2026 Phoenix Systems
 * Author: Marek Bialowas
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <posix/utils.h>

#include "unity_fixture.h"

#define FILE_NAME "mmap_file_testfile"
#define DEV_PATH  "/dev/mmap_file_testdev"

/*
 * Pages the kernel will load with a single read once file objects are loaded in bulk (an upcoming change),
 * file sizes around it are the interesting ones. Until then the kernel reads one page at a time.
 */
#define CHUNK_PAGES 64U
/* Two full chunks and a partial one */
#define BIG_PAGES (2U * CHUNK_PAGES + 9U)


static size_t page_size;

/* /tmp is a RAM filesystem on most targets: the kernel side does not depend on it and flash writes are slow */
static const char *file_path = FILE_NAME;

/* File served by the test itself, to control and observe what the kernel reads */
static struct {
	uint32_t port;
	volatile size_t size;
	volatile bool fail;
	volatile size_t failOffs; /* reads reaching this offset fail */
} srv;


static uint8_t pattern(size_t offs)
{
	return (uint8_t)(1U + offs * 31U + (offs >> 8) * 7U + (offs >> 16) * 3U);
}


static size_t round_up(size_t size)
{
	return (size + page_size - 1U) & ~(page_size - 1U);
}


/* Returns offset of the first byte different from the pattern or -1 */
static long check_pattern(const uint8_t *map, size_t offs, size_t len)
{
	for (size_t i = 0; i < len; i++) {
		if (map[i] != pattern(offs + i)) {
			return (long)(offs + i);
		}
	}

	return -1;
}


static int create_file(size_t size)
{
	static uint8_t buf[4096];
	size_t offs = 0;

	int fd = open(file_path, O_RDWR | O_CREAT | O_TRUNC, 0600);
	if (fd < 0) {
		return -1;
	}

	while (offs < size) {
		size_t chunk = ((size - offs) < sizeof(buf)) ? (size - offs) : sizeof(buf);
		for (size_t i = 0; i < chunk; i++) {
			buf[i] = pattern(offs + i);
		}
		if (write(fd, buf, chunk) != (ssize_t)chunk) {
			close(fd);
			return -1;
		}
		offs += chunk;
	}

	return fd;
}


/* Maps the whole file of the given size and checks its content */
static void check_whole_file(int fd, size_t size)
{
	size_t len = round_up(size);

	uint8_t *map = mmap(NULL, len, PROT_READ, MAP_PRIVATE, fd, 0);
	TEST_ASSERT_NOT_EQUAL(MAP_FAILED, map);

	long bad = check_pattern(map, 0, size);

	TEST_ASSERT_EQUAL(0, munmap(map, len));
	TEST_ASSERT_EQUAL_MESSAGE(-1, bad, "wrong file content at this offset");
}


static void check_file_size(size_t size)
{
	int fd = create_file(size);
	TEST_ASSERT_GREATER_OR_EQUAL(0, fd);

	check_whole_file(fd, size);

	TEST_ASSERT_EQUAL(0, close(fd));
}


static void check_file_range(int fd, size_t offs, size_t size)
{
	size_t len = round_up(size);

	uint8_t *map = mmap(NULL, len, PROT_READ, MAP_PRIVATE, fd, (off_t)offs);
	TEST_ASSERT_NOT_EQUAL(MAP_FAILED, map);

	long bad = check_pattern(map, offs, size);

	TEST_ASSERT_EQUAL(0, munmap(map, len));
	TEST_ASSERT_EQUAL_MESSAGE(-1, bad, "wrong file content at this offset");
}


TEST_GROUP(mmap_file);


TEST_SETUP(mmap_file)
{
}


TEST_TEAR_DOWN(mmap_file)
{
	unlink(file_path);
}


TEST(mmap_file, size_one_byte)
{
	check_file_size(1);
}


TEST(mmap_file, size_below_page)
{
	check_file_size(page_size - 1U);
}


TEST(mmap_file, size_page)
{
	check_file_size(page_size);
}


TEST(mmap_file, size_above_page)
{
	check_file_size(page_size + 1U);
}


TEST(mmap_file, size_below_chunk)
{
	check_file_size(CHUNK_PAGES * page_size - 1U);
}


TEST(mmap_file, size_chunk)
{
	check_file_size(CHUNK_PAGES * page_size);
}


TEST(mmap_file, size_above_chunk)
{
	check_file_size(CHUNK_PAGES * page_size + 1U);
}


TEST(mmap_file, size_big)
{
	check_file_size((BIG_PAGES - 1U) * page_size + 100U);
}


TEST(mmap_file, size_zero)
{
	int fd = create_file(0);
	TEST_ASSERT_GREATER_OR_EQUAL(0, fd);

	/* There is no page to map */
	TEST_ASSERT_EQUAL_PTR(MAP_FAILED, mmap(NULL, page_size, PROT_READ, MAP_PRIVATE, fd, 0));

	TEST_ASSERT_EQUAL(0, close(fd));
}


TEST(mmap_file, longer_than_file)
{
	int fd = create_file(page_size + 1U);
	TEST_ASSERT_GREATER_OR_EQUAL(0, fd);

	/* Pages are not loaded lazily, the ones past the end of file can't be provided */
	TEST_ASSERT_EQUAL_PTR(MAP_FAILED, mmap(NULL, 3U * page_size, PROT_READ, MAP_PRIVATE, fd, 0));

	/* The failed attempt must not break the valid range */
	check_whole_file(fd, page_size + 1U);

	TEST_ASSERT_EQUAL(0, close(fd));
}


TEST(mmap_file, offset)
{
	const size_t size = (BIG_PAGES - 1U) * page_size + 100U;
	const size_t pages = round_up(size) / page_size;

	int fd = create_file(size);
	TEST_ASSERT_GREATER_OR_EQUAL(0, fd);

	check_file_range(fd, page_size, page_size);
	check_file_range(fd, 3U * page_size, 5U * page_size);
	/* Across the chunk boundary */
	check_file_range(fd, (CHUNK_PAGES - 1U) * page_size, 3U * page_size);
	check_file_range(fd, CHUNK_PAGES * page_size, (pages - CHUNK_PAGES - 1U) * page_size);
	check_file_range(fd, (2U * CHUNK_PAGES - 1U) * page_size, 2U * page_size);
	/* More than a chunk, not aligned to it */
	check_file_range(fd, 7U * page_size, (CHUNK_PAGES + 9U) * page_size);

	/* Last, partial page */
	check_file_range(fd, (pages - 1U) * page_size, size - (pages - 1U) * page_size);

	TEST_ASSERT_EQUAL(0, close(fd));
}


/* Pages already held by other mappings of the file are interleaved with the ones still to be loaded */
TEST(mmap_file, partially_loaded)
{
	const size_t size = (BIG_PAGES - 1U) * page_size + 100U;
	const size_t held[] = { 0U, 10U, CHUNK_PAGES - 1U, CHUNK_PAGES + 6U };
	uint8_t *maps[sizeof(held) / sizeof(held[0])];

	int fd = create_file(size);
	TEST_ASSERT_GREATER_OR_EQUAL(0, fd);

	for (size_t i = 0; i < sizeof(held) / sizeof(held[0]); i++) {
		maps[i] = mmap(NULL, 2U * page_size, PROT_READ, MAP_PRIVATE, fd, (off_t)(held[i] * page_size));
		TEST_ASSERT_NOT_EQUAL(MAP_FAILED, maps[i]);
	}

	check_whole_file(fd, size);

	for (size_t i = 0; i < sizeof(held) / sizeof(held[0]); i++) {
		long bad = check_pattern(maps[i], held[i] * page_size, 2U * page_size);
		TEST_ASSERT_EQUAL(0, munmap(maps[i], 2U * page_size));
		TEST_ASSERT_EQUAL_MESSAGE(-1, bad, "wrong file content at this offset");
	}

	TEST_ASSERT_EQUAL(0, close(fd));
}


/* Two processes load the same, not yet loaded file at the same time */
TEST(mmap_file, concurrent)
{
	const size_t size = (BIG_PAGES - 1U) * page_size + 100U;
	const size_t len = round_up(size);

	int fd = create_file(size);
	TEST_ASSERT_GREATER_OR_EQUAL(0, fd);

	/* The kernel drops the pages when the last child exits, each round loads the file again */
	for (int iter = 0; iter < 5; iter++) {
		int start[2];
		pid_t pids[2];

		TEST_ASSERT_EQUAL(0, pipe(start));

		for (int i = 0; i < 2; i++) {
			pids[i] = fork();
			TEST_ASSERT_GREATER_OR_EQUAL(0, pids[i]);
			if (pids[i] == 0) {
				char c;
				close(start[1]);
				/* Wait for the parent to close the pipe */
				(void)read(start[0], &c, 1);

				uint8_t *map = mmap(NULL, len, PROT_READ, MAP_PRIVATE, fd, 0);
				if (map == MAP_FAILED) {
					_exit(2);
				}
				_exit((check_pattern(map, 0, size) < 0) ? 0 : 1);
			}
		}

		close(start[0]);
		close(start[1]);

		for (int i = 0; i < 2; i++) {
			int status = -1;
			TEST_ASSERT_EQUAL(pids[i], waitpid(pids[i], &status, 0));
			TEST_ASSERT_TRUE(WIFEXITED(status));
			TEST_ASSERT_EQUAL_MESSAGE(0, WEXITSTATUS(status), "1 - wrong content, 2 - mmap failed");
		}
	}

	TEST_ASSERT_EQUAL(0, close(fd));
}


TEST_GROUP_RUNNER(mmap_file)
{
	RUN_TEST_CASE(mmap_file, size_one_byte);
	RUN_TEST_CASE(mmap_file, size_below_page);
	RUN_TEST_CASE(mmap_file, size_page);
	RUN_TEST_CASE(mmap_file, size_above_page);
	RUN_TEST_CASE(mmap_file, size_below_chunk);
	RUN_TEST_CASE(mmap_file, size_chunk);
	RUN_TEST_CASE(mmap_file, size_above_chunk);
	RUN_TEST_CASE(mmap_file, size_big);
	RUN_TEST_CASE(mmap_file, size_zero);
	RUN_TEST_CASE(mmap_file, longer_than_file);
	RUN_TEST_CASE(mmap_file, offset);
	RUN_TEST_CASE(mmap_file, partially_loaded);
	RUN_TEST_CASE(mmap_file, concurrent);
}


static void *srv_thread(void *arg)
{
	msg_t msg;
	msg_rid_t rid;

	for (;;) {
		if (msgRecv(srv.port, &msg, &rid) < 0) {
			continue;
		}

		switch (msg.type) {
			case mtOpen:
			case mtClose:
				msg.o.err = 0;
				break;

			case mtGetAttr:
				if (msg.i.attr.type == atSize) {
					msg.o.attr.val = (long long)srv.size;
					msg.o.err = 0;
				}
				else {
					msg.o.err = -ENOSYS;
				}
				break;

			case mtRead: {
				size_t offs = (size_t)msg.i.io.offs;
				size_t len = (offs < srv.size) ? (srv.size - offs) : 0U;
				if (len > msg.o.size) {
					len = msg.o.size;
				}

				if (srv.fail && ((offs + len) > srv.failOffs)) {
					msg.o.err = -EIO;
					break;
				}

				for (size_t i = 0; i < len; i++) {
					((uint8_t *)msg.o.data)[i] = pattern(offs + i);
				}
				msg.o.err = (int)len;
				break;
			}

			default:
				msg.o.err = -ENOSYS;
				break;
		}

		msgRespond(srv.port, &msg, rid);
	}

	return NULL;
}


static int srv_open(size_t size)
{
	srv.size = size;
	srv.fail = false;

	return open(DEV_PATH, O_RDONLY);
}


TEST_GROUP(mmap_file_srv);


TEST_SETUP(mmap_file_srv)
{
}


TEST_TEAR_DOWN(mmap_file_srv)
{
}


TEST(mmap_file_srv, content)
{
	const size_t size = (BIG_PAGES - 1U) * page_size + 100U;

	int fd = srv_open(size);
	TEST_ASSERT_GREATER_OR_EQUAL(0, fd);

	check_whole_file(fd, size);
	check_file_range(fd, (CHUNK_PAGES - 1U) * page_size, 3U * page_size);

	TEST_ASSERT_EQUAL(0, close(fd));
}


TEST(mmap_file_srv, read_error)
{
	const size_t size = (BIG_PAGES - 1U) * page_size + 100U;
	const size_t len = round_up(size);
	const size_t failPages[] = { 0U, 1U, CHUNK_PAGES - 1U, CHUNK_PAGES, CHUNK_PAGES + 6U, len / page_size - 1U };

	for (size_t i = 0; i < sizeof(failPages) / sizeof(failPages[0]); i++) {
		int fd = srv_open(size);
		TEST_ASSERT_GREATER_OR_EQUAL(0, fd);
		srv.failOffs = failPages[i] * page_size;
		srv.fail = true;

		TEST_ASSERT_EQUAL_PTR(MAP_FAILED, mmap(NULL, len, PROT_READ, MAP_PRIVATE, fd, 0));

		/* Pages before the failing one are available */
		if (failPages[i] != 0U) {
			check_file_range(fd, 0, failPages[i] * page_size);
		}

		/* No leftovers of the failure once the file can be read */
		srv.fail = false;
		check_whole_file(fd, size);

		TEST_ASSERT_EQUAL(0, close(fd));
	}
}


TEST_GROUP_RUNNER(mmap_file_srv)
{
	RUN_TEST_CASE(mmap_file_srv, content);
	RUN_TEST_CASE(mmap_file_srv, read_error);
}


static void runner(void)
{
	RUN_TEST_GROUP(mmap_file_srv);
	RUN_TEST_GROUP(mmap_file);
}


int main(int argc, char *argv[])
{
	pthread_t tid;
	oid_t oid;

	page_size = (size_t)sysconf(_SC_PAGESIZE);
	if (access("/tmp", W_OK) == 0) {
		file_path = "/tmp/" FILE_NAME;
	}

	if (portCreate(&srv.port) < 0) {
		fprintf(stderr, "Could not create port\n");
		return EXIT_FAILURE;
	}
	oid.port = srv.port;
	oid.id = 0;
	if (create_dev(&oid, DEV_PATH) < 0) {
		fprintf(stderr, "Could not create %s\n", DEV_PATH);
		return EXIT_FAILURE;
	}
	if (pthread_create(&tid, NULL, srv_thread, NULL) != 0) {
		fprintf(stderr, "Could not start the server thread\n");
		destroy_dev(DEV_PATH);
		return EXIT_FAILURE;
	}

	int ret = (UnityMain(argc, (const char **)argv, runner) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;

	destroy_dev(DEV_PATH);

	return ret;
}
