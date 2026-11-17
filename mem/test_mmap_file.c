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
#include <string.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <posix/utils.h>

#include "unity_fixture.h"

#define FILE_PATH "mmap_file_testfile"
#define DEV_PATH  "/dev/mmap_file_testdev"

/* Size of a single file object load done by the kernel, file sizes around it are the interesting ones */
#define CHUNK_PAGES 64U
/* Two full chunks and a partial one */
#define BIG_PAGES (2U * CHUNK_PAGES + 9U)

/* Amount of free memory filled with non-zero bytes */
#define DIRTY_SIZE (16U * 1024U * 1024U)


static size_t page_size;

/* File served by the test itself, to control and observe what the kernel reads */
static struct {
	uint32_t port;
	volatile size_t size;
	volatile bool fail;
	volatile size_t failOffs; /* reads reaching this offset fail */
	volatile size_t maxRead;  /* 0 - unlimited */
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


/* Returns offset of the first non-zero byte or -1 */
static long check_zero(const uint8_t *map, size_t offs, size_t len)
{
	for (size_t i = 0; i < len; i++) {
		if (map[i] != 0U) {
			return (long)(offs + i);
		}
	}

	return -1;
}


static int create_file(size_t size)
{
	uint8_t buf[512];
	size_t offs = 0;

	int fd = open(FILE_PATH, O_RDWR | O_CREAT | O_TRUNC, 0600);
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


/* Leaves non-zero content in the free memory */
static void dirty_free_memory(void)
{
	for (size_t size = DIRTY_SIZE; size >= page_size; size /= 2U) {
		void *area = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
		if (area != MAP_FAILED) {
			memset(area, 0xa5, size);
			munmap(area, size);
			break;
		}
	}
}


/* Maps files with a partial last page and checks that the rest of the page is zeroed */
static void check_tails(int (*open_file)(size_t size))
{
	const size_t sizes[] = { 1U, 100U, page_size - 1U, page_size + 1U, CHUNK_PAGES * page_size - 1U, CHUNK_PAGES * page_size + 1U };

	/* Repeated as the outcome depends on which free pages the kernel takes */
	for (int iter = 0; iter < 3; iter++) {
		for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); i++) {
			size_t len = round_up(sizes[i]);

			dirty_free_memory();

			int fd = open_file(sizes[i]);
			TEST_ASSERT_GREATER_OR_EQUAL(0, fd);
			uint8_t *map = mmap(NULL, len, PROT_READ, MAP_PRIVATE, fd, 0);
			TEST_ASSERT_NOT_EQUAL(MAP_FAILED, map);

			long bad = check_zero(map + sizes[i], sizes[i], len - sizes[i]);

			TEST_ASSERT_EQUAL(0, munmap(map, len));
			TEST_ASSERT_EQUAL(0, close(fd));
			TEST_ASSERT_EQUAL_MESSAGE(-1, bad, "non-zero byte after the end of file at this offset");
		}
	}
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
	unlink(FILE_PATH);
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

	for (int iter = 0; iter < 5; iter++) {
		int start[2];
		pid_t pids[2];

		int fd = create_file(size);
		TEST_ASSERT_GREATER_OR_EQUAL(0, fd);
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

		TEST_ASSERT_EQUAL(0, close(fd));
		TEST_ASSERT_EQUAL(0, unlink(FILE_PATH));
	}
}


/* A short read at the end of file must not expose what the page contained before */
TEST(mmap_file, tail_zeroed)
{
	check_tails(create_file);
}


/*
 * Regression: munmap() of a contiguous mapping made the kernel forget all file objects,
 * the next mapping of an already mapped file got separate pages.
 */
TEST(mmap_file, contiguous_keeps_file_objects)
{
	int fd = create_file(2U * page_size);
	TEST_ASSERT_GREATER_OR_EQUAL(0, fd);

	uint8_t *first = mmap(NULL, 2U * page_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	TEST_ASSERT_NOT_EQUAL(MAP_FAILED, first);

	uint8_t *area = mmap(NULL, 2U * page_size, PROT_READ | PROT_WRITE, MAP_CONTIGUOUS | MAP_ANONYMOUS, -1, 0);
	TEST_ASSERT_NOT_EQUAL(MAP_FAILED, area);
	memset(area, 0xff, 2U * page_size);
	TEST_ASSERT_EQUAL(0, munmap(area, 2U * page_size));

	uint8_t *second = mmap(NULL, 2U * page_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	TEST_ASSERT_NOT_EQUAL(MAP_FAILED, second);

	/* Both mappings share the pages of one file object */
	first[0] = (uint8_t)~pattern(0);
	first[page_size] = (uint8_t)~pattern(page_size);
	uint8_t got0 = second[0];
	uint8_t got1 = second[page_size];

	TEST_ASSERT_EQUAL(0, munmap(second, 2U * page_size));
	TEST_ASSERT_EQUAL(0, munmap(first, 2U * page_size));
	TEST_ASSERT_EQUAL(0, close(fd));

	TEST_ASSERT_EQUAL_HEX8((uint8_t)~pattern(0), got0);
	TEST_ASSERT_EQUAL_HEX8((uint8_t)~pattern(page_size), got1);
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
	RUN_TEST_CASE(mmap_file, tail_zeroed);
	/* Keep it last, without the fix the kernel state is damaged afterwards */
	RUN_TEST_CASE(mmap_file, contiguous_keeps_file_objects);
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
				if ((srv.maxRead != 0U) && (len > srv.maxRead)) {
					len = srv.maxRead;
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
	srv.maxRead = 0;

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


/* The server returns less than requested, but never less than a page before the end of file */
TEST(mmap_file_srv, short_reads)
{
	const size_t size = (BIG_PAGES - 1U) * page_size + 100U;

	int fd = srv_open(size);
	TEST_ASSERT_GREATER_OR_EQUAL(0, fd);
	srv.maxRead = 3U * page_size + 100U;

	check_whole_file(fd, size);

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


/* As mmap_file.tail_zeroed, but independent of what the filesystem does with the rest of the read buffer */
TEST(mmap_file_srv, tail_zeroed)
{
	check_tails(srv_open);
}


TEST_GROUP_RUNNER(mmap_file_srv)
{
	RUN_TEST_CASE(mmap_file_srv, content);
	RUN_TEST_CASE(mmap_file_srv, short_reads);
	RUN_TEST_CASE(mmap_file_srv, read_error);
	RUN_TEST_CASE(mmap_file_srv, tail_zeroed);
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
