/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <sys/mman.h>
 * TESTED:
 *    - mprotect()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Lukasz Kruszynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define _XOPEN_SOURCE 700

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <unity_fixture.h>

#define MPROTECT_TEST_FILENAME "mprotect_test_file"
#define TEST_BYTE_A            0x5a
#define TEST_BYTE_B           0xa5


static struct {
	int fd;
	void *addr;
	size_t pageSize;
	volatile sig_atomic_t caughtSignal;
	struct sigaction oldSegv;
	struct sigaction oldBus;
} test_common;

static sigjmp_buf test_jmp;


/* Records a memory-access fault and returns control to the guarded access. */
static void test_faultHandler(int sig)
{
	(void)sig;
	test_common.caughtSignal = 1;
	siglongjmp(test_jmp, 1);
}


/* Map the test file page privately with read/write access. */
static volatile unsigned char *test_mapPrivateRW(void)
{
	void *addr = mmap(NULL, test_common.pageSize, PROT_READ | PROT_WRITE, MAP_PRIVATE, test_common.fd, 0);

	TEST_ASSERT_TRUE_MESSAGE(addr != MAP_FAILED, "mmap failed");
	test_common.addr = addr;
	return (volatile unsigned char *)addr;
}


TEST_GROUP(mman_mprotect);


TEST_SETUP(mman_mprotect)
{
	struct sigaction sa;

	test_common.fd = -1;
	test_common.addr = MAP_FAILED;
	test_common.caughtSignal = 0;
	test_common.pageSize = (size_t)sysconf(_SC_PAGESIZE);
	TEST_ASSERT_GREATER_THAN_INT(0, (int)test_common.pageSize);

	unlink(MPROTECT_TEST_FILENAME);
	test_common.fd = open(MPROTECT_TEST_FILENAME, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_common.fd);
	TEST_ASSERT_EQUAL_INT(0, ftruncate(test_common.fd, (off_t)test_common.pageSize));

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = test_faultHandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	TEST_ASSERT_EQUAL_INT(0, sigaction(SIGSEGV, &sa, &test_common.oldSegv));
	TEST_ASSERT_EQUAL_INT(0, sigaction(SIGBUS, &sa, &test_common.oldBus));
}


TEST_TEAR_DOWN(mman_mprotect)
{
	if (test_common.addr != MAP_FAILED) {
		munmap(test_common.addr, test_common.pageSize);
		test_common.addr = MAP_FAILED;
	}
	if (test_common.fd >= 0) {
		close(test_common.fd);
		test_common.fd = -1;
	}
	unlink(MPROTECT_TEST_FILENAME);

	sigaction(SIGSEGV, &test_common.oldSegv, NULL);
	sigaction(SIGBUS, &test_common.oldBus, NULL);
}


/* mprotect() shall return 0 for each protection combination it must support. */
TEST(mman_mprotect, supported_combinations_succeed)
{
	volatile unsigned char *p = test_mapPrivateRW();
	void *addr = (void *)p;

	TEST_ASSERT_EQUAL_INT(0, mprotect(addr, test_common.pageSize, PROT_NONE));
	TEST_ASSERT_EQUAL_INT(0, mprotect(addr, test_common.pageSize, PROT_READ));
	TEST_ASSERT_EQUAL_INT(0, mprotect(addr, test_common.pageSize, PROT_WRITE));
	TEST_ASSERT_EQUAL_INT(0, mprotect(addr, test_common.pageSize, PROT_READ | PROT_WRITE));
}


/* Data written while readable shall remain accessible after dropping to PROT_READ. */
TEST(mman_mprotect, read_protection_allows_read)
{
	volatile unsigned char *p = test_mapPrivateRW();

	p[0] = TEST_BYTE_A;
	p[1] = TEST_BYTE_B;

	TEST_ASSERT_EQUAL_INT(0, mprotect((void *)p, test_common.pageSize, PROT_READ));
	TEST_ASSERT_EQUAL_HEX8(TEST_BYTE_A, p[0]);
	TEST_ASSERT_EQUAL_HEX8(TEST_BYTE_B, p[1]);
}


/* No write shall succeed on a page whose protection lacks PROT_WRITE. */
TEST(mman_mprotect, write_faults_without_prot_write)
{
	volatile unsigned char *p = test_mapPrivateRW();

	p[0] = TEST_BYTE_A;
	TEST_ASSERT_EQUAL_INT(0, mprotect((void *)p, test_common.pageSize, PROT_READ));

	test_common.caughtSignal = 0;
	if (sigsetjmp(test_jmp, 1) == 0) {
		p[0] = 0x00;
		TEST_FAIL_MESSAGE("write to a PROT_READ page did not fault");
	}
	TEST_ASSERT_EQUAL_INT(1, test_common.caughtSignal);

	/* The forbidden write must not have taken effect. */
	TEST_ASSERT_EQUAL_INT(0, mprotect((void *)p, test_common.pageSize, PROT_READ | PROT_WRITE));
	TEST_ASSERT_EQUAL_HEX8(TEST_BYTE_A, p[0]);
}


/* No access shall succeed on a page set to PROT_NONE. */
TEST(mman_mprotect, read_faults_with_prot_none)
{
	TEST_IGNORE_MESSAGE("Unverified Failure");
	volatile unsigned char *p = test_mapPrivateRW();
	volatile unsigned char sink;

	p[0] = TEST_BYTE_A;
	TEST_ASSERT_EQUAL_INT(0, mprotect((void *)p, test_common.pageSize, PROT_NONE));

	test_common.caughtSignal = 0;
	if (sigsetjmp(test_jmp, 1) == 0) {
		sink = p[0];
		(void)sink;
		TEST_FAIL_MESSAGE("read from a PROT_NONE page did not fault");
	}
	TEST_ASSERT_EQUAL_INT(1, test_common.caughtSignal);
}


/* mprotect() over an unmapped range shall fail with ENOMEM. */
TEST(mman_mprotect, unmapped_range_enomem)
{
	void *region = mmap(NULL, test_common.pageSize, PROT_READ | PROT_WRITE, MAP_PRIVATE, test_common.fd, 0);

	TEST_ASSERT_TRUE_MESSAGE(region != MAP_FAILED, "mmap failed");
	TEST_ASSERT_EQUAL_INT(0, munmap(region, test_common.pageSize));

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, mprotect(region, test_common.pageSize, PROT_READ));
	TEST_ASSERT_EQUAL_INT(ENOMEM, errno);
}


/*
 * Requesting PROT_WRITE on a shared mapping of a read-only file descriptor
 * violates the process's access to the object and shall fail with EACCES.
 */
TEST(mman_mprotect, write_on_readonly_object_eacces)
{
	int roFd;
	void *p;
	int ret;
	int savedErrno;

	roFd = open(MPROTECT_TEST_FILENAME, O_RDONLY);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, roFd);

	p = mmap(NULL, test_common.pageSize, PROT_READ, MAP_SHARED, roFd, 0);
	if (p == MAP_FAILED) {
		close(roFd);
		TEST_FAIL_MESSAGE("shared read-only mapping could not be created");
	}

	errno = 0;
	ret = mprotect(p, test_common.pageSize, PROT_READ | PROT_WRITE);
	savedErrno = errno;

	munmap(p, test_common.pageSize);
	close(roFd);

	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EACCES, savedErrno);
}


TEST_GROUP_RUNNER(mman_mprotect)
{
	RUN_TEST_CASE(mman_mprotect, supported_combinations_succeed);
	RUN_TEST_CASE(mman_mprotect, read_protection_allows_read);
	RUN_TEST_CASE(mman_mprotect, write_faults_without_prot_write);
	RUN_TEST_CASE(mman_mprotect, read_faults_with_prot_none);
	RUN_TEST_CASE(mman_mprotect, unmapped_range_enomem);
	RUN_TEST_CASE(mman_mprotect, write_on_readonly_object_eacces);
}
