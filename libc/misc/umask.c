/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <sys/stat.h>
 * TESTED:
 *    - umask()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Lukasz Kruszynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "unity_fixture.h"

#define UMASK_TEST_FILE "/tmp/test_umask_file"
#define UMASK_TEST_DIR  "/tmp/test_umask_dir"
#define UMASK_TEST_FIFO "/tmp/test_umask_fifo"

TEST_GROUP(misc_umask);

TEST_SETUP(misc_umask)
{
	unlink(UMASK_TEST_FILE);
	unlink(UMASK_TEST_FIFO);
	rmdir(UMASK_TEST_DIR);
}

TEST_TEAR_DOWN(misc_umask)
{
	unlink(UMASK_TEST_FILE);
	unlink(UMASK_TEST_FIFO);
	rmdir(UMASK_TEST_DIR);
}


TEST(misc_umask, umask_returns_previous_value)
{
	mode_t prev;
	mode_t curr;

	/* Set a known mask */
	prev = umask(022);

	/* Set a different mask; should return the one we just set */
	curr = umask(077);
#ifdef __phoenix__
	(void)curr;
	TEST_IGNORE_MESSAGE("#1633 issue");
#else
	TEST_ASSERT_EQUAL_INT(022, (curr & 0777));
#endif

	/* Restore */
	umask(prev);
}


TEST(misc_umask, umask_set_zero)
{
	mode_t prev;
	mode_t ret;

	prev = umask(0);

	ret = umask(prev);
	TEST_ASSERT_EQUAL_INT(0, (ret & 0777));
}


TEST(misc_umask, umask_set_all_permission_bits)
{
	mode_t prev;
	mode_t ret;

	prev = umask(0777);

	ret = umask(prev);
#ifdef __phoenix__
	(void)ret;
	TEST_IGNORE_MESSAGE("#2 issue");
#else
	TEST_ASSERT_EQUAL_INT(0777, (ret & 0777));
#endif
}


TEST(misc_umask, umask_only_permission_bits_used)
{
	mode_t prev;
	mode_t ret;

	/*
	 * POSIX: "Only the file permission bits of cmask are used;
	 * the meaning of the other bits is implementation-defined."
	 * Setting bits beyond 0777 should not affect the permission-bit portion.
	 */
	prev = umask(07777);

	ret = umask(prev);
#ifdef __phoenix__
	(void)ret;
	TEST_IGNORE_MESSAGE("#2 issue");
#else
	/* The permission bits portion must be 0777 */
	TEST_ASSERT_EQUAL_INT(0777, (ret & 0777));
#endif
}


TEST(misc_umask, umask_roundtrip_preserves_mask)
{
	mode_t orig;
	mode_t restored;

	/*
	 * POSIX: "a subsequent call to umask() with the returned value as cmask
	 * shall leave the state of the mask the same as its state before the
	 * first call, including any unspecified use of those bits."
	 */
	orig = umask(0123);
	/* orig holds previous mask; now restore it */
	restored = umask(orig);
#ifdef __phoenix__
	(void)restored;
	TEST_IGNORE_MESSAGE("#2 issue");
#else
	TEST_ASSERT_EQUAL_INT(0123, (restored & 0777));

	/* Verify the mask is truly back to orig */
	restored = umask(orig);
	TEST_ASSERT_EQUAL_INT((orig & 0777), (restored & 0777));
#endif
}


TEST(misc_umask, umask_affects_open_creat)
{
	mode_t prev;
	int fd;
	struct stat st;
	int ret;

	prev = umask(077);

	fd = open(UMASK_TEST_FILE, O_CREAT | O_WRONLY, 0666);
	TEST_ASSERT_NOT_EQUAL_INT(-1, fd);
	close(fd);

	ret = stat(UMASK_TEST_FILE, &st);
	TEST_ASSERT_EQUAL_INT(0, ret);

#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1628 issue");
#else
	/* 0666 & ~077 = 0600 */
	TEST_ASSERT_EQUAL_INT(0600, (st.st_mode & 0777));
#endif

	umask(prev);
}


TEST(misc_umask, umask_affects_open_creat_zero_mask)
{
	mode_t prev;
	int fd;
	struct stat st;
	int ret;

	prev = umask(0);

	fd = open(UMASK_TEST_FILE, O_CREAT | O_WRONLY, 0777);
	TEST_ASSERT_NOT_EQUAL_INT(-1, fd);
	close(fd);

	ret = stat(UMASK_TEST_FILE, &st);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* 0777 & ~0 = 0777 */
	TEST_ASSERT_EQUAL_INT(0777, (st.st_mode & 0777));

	umask(prev);
}


TEST(misc_umask, umask_affects_mkdir)
{
	mode_t prev;
	struct stat st;
	int ret;

	prev = umask(027);

	ret = mkdir(UMASK_TEST_DIR, 0777);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = stat(UMASK_TEST_DIR, &st);
	TEST_ASSERT_EQUAL_INT(0, ret);

#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1628 issue");
#else
	/* 0777 & ~027 = 0750 */
	TEST_ASSERT_EQUAL_INT(0750, (st.st_mode & 0777));
#endif

	umask(prev);
}


TEST(misc_umask, umask_clears_bits_in_mode)
{
	mode_t prev;
	int fd;
	struct stat st;
	int ret;

	/* Set mask to clear group-write and other-all */
	prev = umask(027);

	fd = open(UMASK_TEST_FILE, O_CREAT | O_WRONLY, 0666);
	TEST_ASSERT_NOT_EQUAL_INT(-1, fd);
	close(fd);

	ret = stat(UMASK_TEST_FILE, &st);
	TEST_ASSERT_EQUAL_INT(0, ret);

#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1628 issue");
#else
	/* 0666 & ~027 = 0640 */
	TEST_ASSERT_EQUAL_INT(0640, (st.st_mode & 0777));
#endif

	umask(prev);
}


TEST(misc_umask, umask_no_errors_defined)
{
	mode_t prev;

	/*
	 * POSIX: "No errors are defined."
	 * umask always succeeds; verify errno is not modified.
	 */
	errno = 0;
	prev = umask(022);
	TEST_ASSERT_EQUAL_INT(0, errno);

	errno = 0;
	umask(prev);
	TEST_ASSERT_EQUAL_INT(0, errno);
}


TEST(misc_umask, umask_individual_bits)
{
	mode_t prev;
	mode_t ret;

	/* Test each individual permission bit */
	prev = umask(S_IRUSR);
	ret = umask(S_IWUSR);
#ifdef __phoenix__
	(void)prev;
	(void)ret;
	TEST_IGNORE_MESSAGE("#2 issue");
#else
	TEST_ASSERT_EQUAL_INT(S_IRUSR, (ret & 0777));

	ret = umask(S_IXUSR);
	TEST_ASSERT_EQUAL_INT(S_IWUSR, (ret & 0777));

	ret = umask(S_IRGRP);
	TEST_ASSERT_EQUAL_INT(S_IXUSR, (ret & 0777));

	ret = umask(S_IWGRP);
	TEST_ASSERT_EQUAL_INT(S_IRGRP, (ret & 0777));

	ret = umask(S_IXGRP);
	TEST_ASSERT_EQUAL_INT(S_IWGRP, (ret & 0777));

	ret = umask(S_IROTH);
	TEST_ASSERT_EQUAL_INT(S_IXGRP, (ret & 0777));

	ret = umask(S_IWOTH);
	TEST_ASSERT_EQUAL_INT(S_IROTH, (ret & 0777));

	ret = umask(S_IXOTH);
	TEST_ASSERT_EQUAL_INT(S_IWOTH, (ret & 0777));

	ret = umask(prev);
	TEST_ASSERT_EQUAL_INT(S_IXOTH, (ret & 0777));
#endif
}


TEST(misc_umask, umask_affects_mkfifo)
{
	mode_t prev;
	struct stat st;
	int ret;

	prev = umask(027);

	TEST_IGNORE_MESSAGE("no fs, todo: make it conditional");
	ret = mkfifo(UMASK_TEST_FIFO, 0666);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = stat(UMASK_TEST_FIFO, &st);
	TEST_ASSERT_EQUAL_INT(0, ret);

#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1628 issue");
#else
	/* 0666 & ~027 = 0640 */
	TEST_ASSERT_EQUAL_INT(0640, (st.st_mode & 0777));
	TEST_ASSERT_TRUE(S_ISFIFO(st.st_mode));
#endif

	umask(prev);
}


TEST(misc_umask, umask_inherited_by_fork)
{
	mode_t prev;
	pid_t pid;
	int status;

	prev = umask(0135);

	TEST_IGNORE_MESSAGE("no mmu, todo: make it conditional");
	pid = fork();
	TEST_ASSERT_NOT_EQUAL_INT(-1, pid);

	if (pid == 0) {
		/* Child: verify inherited mask and exit with result */
		mode_t childMask = umask(0);
		_exit(((childMask & 0777) == 0135) ? 0 : 1);
	}

	/* Parent: wait for child and check exit status */
	TEST_ASSERT_EQUAL_INT(pid, waitpid(pid, &status, 0));
	TEST_ASSERT_TRUE(WIFEXITED(status));
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#2 issue");
#else
	TEST_ASSERT_EQUAL_INT(0, WEXITSTATUS(status));
#endif

	umask(prev);
}


TEST_GROUP_RUNNER(misc_umask)
{
	RUN_TEST_CASE(misc_umask, umask_returns_previous_value);
	RUN_TEST_CASE(misc_umask, umask_set_zero);
	RUN_TEST_CASE(misc_umask, umask_set_all_permission_bits);
	RUN_TEST_CASE(misc_umask, umask_only_permission_bits_used);
	RUN_TEST_CASE(misc_umask, umask_roundtrip_preserves_mask);
	RUN_TEST_CASE(misc_umask, umask_affects_open_creat);
	RUN_TEST_CASE(misc_umask, umask_affects_open_creat_zero_mask);
	RUN_TEST_CASE(misc_umask, umask_affects_mkdir);
	RUN_TEST_CASE(misc_umask, umask_affects_mkfifo);
	RUN_TEST_CASE(misc_umask, umask_clears_bits_in_mode);
	RUN_TEST_CASE(misc_umask, umask_no_errors_defined);
	RUN_TEST_CASE(misc_umask, umask_individual_bits);
	RUN_TEST_CASE(misc_umask, umask_inherited_by_fork);
}
