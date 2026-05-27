/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <sys/mman.h>
 * TESTED:
 *    - mmap()
 *    - munmap()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <unity_fixture.h>

#define MMAP_TEST_FILENAME "mmap_test_file"
#define MMAP_FILE_SIZE     4096


static struct {
	int fd;
	void *addr;
	size_t pageSize;
} test_common;


/*
Test group for mmap.
*/
TEST_GROUP(mman_mmap);


TEST_SETUP(mman_mmap)
{
	char buf[MMAP_FILE_SIZE];

	test_common.addr = MAP_FAILED;
	test_common.pageSize = (size_t)sysconf(_SC_PAGESIZE);

	unlink(MMAP_TEST_FILENAME);

	/* create a test file of known size */
	test_common.fd = open(MMAP_TEST_FILENAME, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_common.fd);

	memset(buf, 'A', sizeof(buf));
	TEST_ASSERT_EQUAL_INT((ssize_t)sizeof(buf), write(test_common.fd, buf, sizeof(buf)));
}


TEST_TEAR_DOWN(mman_mmap)
{
	if (test_common.addr != MAP_FAILED) {
		munmap(test_common.addr, test_common.pageSize);
	}

	if (test_common.fd >= 0) {
		close(test_common.fd);
		test_common.fd = -1;
	}

	unlink(MMAP_TEST_FILENAME);
}


TEST(mman_mmap, mmap_basic_read)
{
	char *ptr;

	/* mmap with PROT_READ, MAP_PRIVATE */
	test_common.addr = mmap(NULL, test_common.pageSize, PROT_READ, MAP_PRIVATE, test_common.fd, 0);
	TEST_ASSERT_NOT_EQUAL(MAP_FAILED, test_common.addr);

	ptr = (char *)test_common.addr;
	/* verify data matches what was written */
	TEST_ASSERT_EQUAL_INT('A', ptr[0]);
	TEST_ASSERT_EQUAL_INT('A', ptr[test_common.pageSize - 1]);
}


TEST(mman_mmap, mmap_basic_write_shared)
{
	char *ptr;
#ifndef __phoenix__
	char readBuf[1];
	ssize_t n;
#endif

	/* mmap with PROT_READ|PROT_WRITE, MAP_SHARED: writes shall change underlying object */
	test_common.addr = mmap(NULL, test_common.pageSize, PROT_READ | PROT_WRITE, MAP_SHARED, test_common.fd, 0);
	TEST_ASSERT_NOT_EQUAL(MAP_FAILED, test_common.addr);

	ptr = (char *)test_common.addr;
	ptr[0] = 'B';

#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1655 issue");
#else
	/* verify write is reflected in the underlying file */
	TEST_ASSERT_EQUAL_INT(0, lseek(test_common.fd, 0, SEEK_SET));
	n = read(test_common.fd, readBuf, 1);
	TEST_ASSERT_EQUAL_INT(1, n);
	TEST_ASSERT_EQUAL_INT('B', readBuf[0]);
#endif
}


TEST(mman_mmap, mmap_basic_write_private)
{
	char *ptr;
	char readBuf[1];
	ssize_t n;

	/* mmap with MAP_PRIVATE: modifications shall not change underlying object */
	test_common.addr = mmap(NULL, test_common.pageSize, PROT_READ | PROT_WRITE, MAP_PRIVATE, test_common.fd, 0);
	TEST_ASSERT_NOT_EQUAL(MAP_FAILED, test_common.addr);

	ptr = (char *)test_common.addr;
	ptr[0] = 'C';

	/* verify original file is unchanged */
	TEST_ASSERT_EQUAL_INT(0, lseek(test_common.fd, 0, SEEK_SET));
	n = read(test_common.fd, readBuf, 1);
	TEST_ASSERT_EQUAL_INT(1, n);
	TEST_ASSERT_EQUAL_INT('A', readBuf[0]);
}


TEST(mman_mmap, mmap_offset)
{
	char *ptr;
	off_t off;

	/* map from an offset that is a multiple of page size */
	off = (off_t)test_common.pageSize;

	/* extend file to 2 pages */
	TEST_ASSERT_EQUAL_INT(0, ftruncate(test_common.fd, 2 * (off_t)test_common.pageSize));
	TEST_ASSERT_EQUAL_INT(off, lseek(test_common.fd, off, SEEK_SET));
	{
		char buf[1] = { 'Z' };
		TEST_ASSERT_EQUAL_INT(1, write(test_common.fd, buf, 1));
	}

	test_common.addr = mmap(NULL, test_common.pageSize, PROT_READ, MAP_PRIVATE, test_common.fd, off);
	TEST_ASSERT_NOT_EQUAL(MAP_FAILED, test_common.addr);

	ptr = (char *)test_common.addr;
	TEST_ASSERT_EQUAL_INT('Z', ptr[0]);
}


TEST(mman_mmap, mmap_einval_zero_len)
{
	/* len == 0 shall fail with EINVAL */
	errno = 0;
	test_common.addr = mmap(NULL, 0, PROT_READ, MAP_PRIVATE, test_common.fd, 0);
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1656 issue");
#else
	TEST_ASSERT_EQUAL_PTR(MAP_FAILED, test_common.addr);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
	test_common.addr = MAP_FAILED;
#endif
}


TEST(mman_mmap, mmap_einval_no_map_flag)
{
	/* neither MAP_SHARED nor MAP_PRIVATE shall fail with EINVAL */
	errno = 0;
	test_common.addr = mmap(NULL, test_common.pageSize, PROT_READ, 0, test_common.fd, 0);
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1656 issue");
#else
	TEST_ASSERT_EQUAL_PTR(MAP_FAILED, test_common.addr);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
	test_common.addr = MAP_FAILED;
#endif
}


TEST(mman_mmap, mmap_ebadf)
{
	/* invalid fd shall fail with EBADF */
	errno = 0;
	test_common.addr = mmap(NULL, test_common.pageSize, PROT_READ, MAP_PRIVATE, -1, 0);
	TEST_ASSERT_EQUAL_PTR(MAP_FAILED, test_common.addr);
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
	test_common.addr = MAP_FAILED;
}


TEST(mman_mmap, mmap_eacces_write_on_rdonly)
{
	int rdFd;

	/* PROT_WRITE with MAP_SHARED on a read-only fd shall fail with EACCES */
	rdFd = open(MMAP_TEST_FILENAME, O_RDONLY);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, rdFd);

	errno = 0;
	test_common.addr = mmap(NULL, test_common.pageSize, PROT_WRITE, MAP_SHARED, rdFd, 0);
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1657 issue");
#else
	TEST_ASSERT_EQUAL_PTR(MAP_FAILED, test_common.addr);
	TEST_ASSERT_EQUAL_INT(EACCES, errno);
	test_common.addr = MAP_FAILED;
#endif

	close(rdFd);
}


TEST(mman_mmap, mmap_enxio_invalid_offset)
{
	off_t bigOff;

	/* offset beyond object size shall fail with ENXIO */
	bigOff = (off_t)(1024 * 1024 * 1024);

	errno = 0;
	test_common.addr = mmap(NULL, test_common.pageSize, PROT_READ, MAP_PRIVATE, test_common.fd, bigOff);
	if (test_common.addr != MAP_FAILED) {
		/* some implementations may allow this; skip if so */
		munmap(test_common.addr, test_common.pageSize);
		test_common.addr = MAP_FAILED;
		TEST_IGNORE_MESSAGE("implementation allows mapping beyond file end");
	}
	else {
		TEST_ASSERT_EQUAL_PTR(MAP_FAILED, test_common.addr);
		/* POSIX says ENXIO; some systems return EINVAL or ENOMEM */
		TEST_ASSERT_TRUE(errno == ENXIO || errno == EINVAL || errno == ENOMEM);
		test_common.addr = MAP_FAILED;
	}
}


TEST(mman_mmap, mmap_not_null_return)
{
	/* successful mmap shall never return NULL */
	test_common.addr = mmap(NULL, test_common.pageSize, PROT_READ, MAP_PRIVATE, test_common.fd, 0);
	TEST_ASSERT_NOT_EQUAL(MAP_FAILED, test_common.addr);
	TEST_ASSERT_NOT_NULL(test_common.addr);
}


TEST(mman_mmap, mmap_fd_close_after_map)
{
	char *ptr;

	/* closing fd after mmap shall not invalidate the mapping */
	test_common.addr = mmap(NULL, test_common.pageSize, PROT_READ, MAP_PRIVATE, test_common.fd, 0);
	TEST_ASSERT_NOT_EQUAL(MAP_FAILED, test_common.addr);

	close(test_common.fd);
	test_common.fd = -1;

	ptr = (char *)test_common.addr;
	TEST_ASSERT_EQUAL_INT('A', ptr[0]);
}


TEST_GROUP_RUNNER(mman_mmap)
{
	RUN_TEST_CASE(mman_mmap, mmap_basic_read);
	RUN_TEST_CASE(mman_mmap, mmap_basic_write_shared);
	RUN_TEST_CASE(mman_mmap, mmap_basic_write_private);
	RUN_TEST_CASE(mman_mmap, mmap_offset);
	RUN_TEST_CASE(mman_mmap, mmap_einval_zero_len);
	RUN_TEST_CASE(mman_mmap, mmap_einval_no_map_flag);
	RUN_TEST_CASE(mman_mmap, mmap_ebadf);
	RUN_TEST_CASE(mman_mmap, mmap_eacces_write_on_rdonly);
	RUN_TEST_CASE(mman_mmap, mmap_enxio_invalid_offset);
	RUN_TEST_CASE(mman_mmap, mmap_not_null_return);
	RUN_TEST_CASE(mman_mmap, mmap_fd_close_after_map);
}


/*
Test group for munmap.
*/
TEST_GROUP(mman_munmap);


TEST_SETUP(mman_munmap)
{
	char buf[MMAP_FILE_SIZE];

	test_common.addr = MAP_FAILED;
	test_common.pageSize = (size_t)sysconf(_SC_PAGESIZE);

	unlink(MMAP_TEST_FILENAME);

	test_common.fd = open(MMAP_TEST_FILENAME, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_common.fd);

	memset(buf, 'M', sizeof(buf));
	TEST_ASSERT_EQUAL_INT((ssize_t)sizeof(buf), write(test_common.fd, buf, sizeof(buf)));
}


TEST_TEAR_DOWN(mman_munmap)
{
	if (test_common.addr != MAP_FAILED) {
		munmap(test_common.addr, test_common.pageSize);
	}

	if (test_common.fd >= 0) {
		close(test_common.fd);
		test_common.fd = -1;
	}

	unlink(MMAP_TEST_FILENAME);
}


TEST(mman_munmap, munmap_basic)
{
	int ret;

	test_common.addr = mmap(NULL, test_common.pageSize, PROT_READ, MAP_PRIVATE, test_common.fd, 0);
	TEST_ASSERT_NOT_EQUAL(MAP_FAILED, test_common.addr);

	ret = munmap(test_common.addr, test_common.pageSize);
	TEST_ASSERT_EQUAL_INT(0, ret);
	test_common.addr = MAP_FAILED;
}


TEST(mman_munmap, munmap_einval_zero_len)
{
	/* munmap with len == 0 shall fail with EINVAL */
	test_common.addr = mmap(NULL, test_common.pageSize, PROT_READ, MAP_PRIVATE, test_common.fd, 0);
	TEST_ASSERT_NOT_EQUAL(MAP_FAILED, test_common.addr);

#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1658 issue");
#else
	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, munmap(test_common.addr, 0));
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
#endif
}


TEST(mman_munmap, munmap_no_effect_unmapped)
{
	void *addr;
	int ret;

	/* munmap on a range with no mappings shall have no effect (return 0 or EINVAL) */
	addr = mmap(NULL, test_common.pageSize, PROT_READ, MAP_PRIVATE, test_common.fd, 0);
	TEST_ASSERT_NOT_EQUAL(MAP_FAILED, addr);

	ret = munmap(addr, test_common.pageSize);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* second munmap on same area - behavior is implementation-defined but should not crash */
	errno = 0;
	ret = munmap(addr, test_common.pageSize);
	/* POSIX says "no effect" if no mappings; some impls return EINVAL */
	TEST_ASSERT_TRUE(ret == 0 || (ret == -1 && errno == EINVAL));
}


TEST(mman_munmap, munmap_partial)
{
	char *ptr;
	int ret;
	size_t twoPages;

	/* map two pages, unmap the first, verify second still accessible */
	twoPages = 2 * test_common.pageSize;

	TEST_ASSERT_EQUAL_INT(0, ftruncate(test_common.fd, (off_t)twoPages));
	/* write a marker to second page */
	TEST_ASSERT_EQUAL_INT((off_t)test_common.pageSize, lseek(test_common.fd, (off_t)test_common.pageSize, SEEK_SET));
	{
		char marker = 'Z';
		TEST_ASSERT_EQUAL_INT(1, write(test_common.fd, &marker, 1));
	}

	test_common.addr = mmap(NULL, twoPages, PROT_READ, MAP_PRIVATE, test_common.fd, 0);
	TEST_ASSERT_NOT_EQUAL(MAP_FAILED, test_common.addr);

	/* unmap first page only */
	ret = munmap(test_common.addr, test_common.pageSize);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* second page should still be accessible */
	ptr = (char *)test_common.addr + test_common.pageSize;
	TEST_ASSERT_EQUAL_INT('Z', ptr[0]);

	/* clean up the remaining page */
	munmap(ptr, test_common.pageSize);
	test_common.addr = MAP_FAILED;
}


TEST_GROUP_RUNNER(mman_munmap)
{
	RUN_TEST_CASE(mman_munmap, munmap_basic);
	RUN_TEST_CASE(mman_munmap, munmap_einval_zero_len);
	RUN_TEST_CASE(mman_munmap, munmap_no_effect_unmapped);
	RUN_TEST_CASE(mman_munmap, munmap_partial);
}
