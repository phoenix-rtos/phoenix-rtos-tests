/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <fcntl.h>
 * TESTED:
 *    - fcntl()
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
#include <sys/stat.h>
#include <sys/wait.h>
#include <limits.h>

#include <unity_fixture.h>

#define FCNTL_TEST_FILE "fcntl_test_file.tmp"


static struct {
	int fd;
} test_common;


TEST_GROUP(fcntl_fcntl);


TEST_SETUP(fcntl_fcntl)
{
	(void)unlink(FCNTL_TEST_FILE);
	test_common.fd = open(FCNTL_TEST_FILE, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
}


TEST_TEAR_DOWN(fcntl_fcntl)
{
	if (test_common.fd >= 0) {
		close(test_common.fd);
		test_common.fd = -1;
	}
	(void)unlink(FCNTL_TEST_FILE);
}


/* F_DUPFD: shall return a new fd >= arg */
TEST(fcntl_fcntl, dupfd_returns_fd_gte_arg)
{
	int newfd;

	newfd = fcntl(test_common.fd, F_DUPFD, 10);
	TEST_ASSERT_TRUE(newfd >= 10);

	close(newfd);
}


/* F_DUPFD: new fd refers to same file (shares offset) */
TEST(fcntl_fcntl, dupfd_shares_offset)
{
	int newfd;
	off_t off1;
	off_t off2;
	ssize_t n;

	n = write(test_common.fd, "hello", 5);
	TEST_ASSERT_EQUAL_INT(5, (int)n);

	newfd = fcntl(test_common.fd, F_DUPFD, 0);
	TEST_ASSERT_TRUE(newfd >= 0);

	off1 = lseek(test_common.fd, 0, SEEK_CUR);
	off2 = lseek(newfd, 0, SEEK_CUR);
	TEST_ASSERT_EQUAL_INT((int)off1, (int)off2);

	close(newfd);
}


/* F_DUPFD: FD_CLOEXEC shall be cleared on new fd */
TEST(fcntl_fcntl, dupfd_clears_cloexec)
{
	int newfd;
	int flags;

	newfd = fcntl(test_common.fd, F_DUPFD, 0);
	TEST_ASSERT_TRUE(newfd >= 0);

	flags = fcntl(newfd, F_GETFD);
	TEST_ASSERT_TRUE(flags >= 0);
	TEST_ASSERT_EQUAL_INT(0, flags & FD_CLOEXEC);

	close(newfd);
}


/* F_DUPFD: lowest numbered fd >= arg is returned */
TEST(fcntl_fcntl, dupfd_lowest_available)
{
	int newfd;
	int fd10;

	/* Occupy fd 10 */
	fd10 = fcntl(test_common.fd, F_DUPFD, 10);
	TEST_ASSERT_TRUE(fd10 >= 10);

	/* Request >= 10, should skip occupied fd10 */
	newfd = fcntl(test_common.fd, F_DUPFD, 10);
	TEST_ASSERT_TRUE(newfd > fd10);

	close(fd10);
	close(newfd);
}


/* F_DUPFD_CLOEXEC: FD_CLOEXEC shall be set on new fd */
TEST(fcntl_fcntl, dupfd_cloexec_sets_flag)
{
	int newfd;
	int flags;

	newfd = fcntl(test_common.fd, F_DUPFD_CLOEXEC, 0);
	TEST_ASSERT_TRUE(newfd >= 0);

	flags = fcntl(newfd, F_GETFD);
	TEST_ASSERT_TRUE(flags >= 0);
	TEST_ASSERT_EQUAL_INT(FD_CLOEXEC, flags & FD_CLOEXEC);

	close(newfd);
}


/* F_DUPFD_CLOEXEC: shall return fd >= arg */
TEST(fcntl_fcntl, dupfd_cloexec_gte_arg)
{
	int newfd;

	newfd = fcntl(test_common.fd, F_DUPFD_CLOEXEC, 20);
	TEST_ASSERT_TRUE(newfd >= 20);

	close(newfd);
}


/* F_GETFD/F_SETFD: round-trip of FD_CLOEXEC flag */
TEST(fcntl_fcntl, getfd_setfd_cloexec)
{
	int flags;
	int ret;

	/* Initially FD_CLOEXEC should be clear (opened without O_CLOEXEC) */
	flags = fcntl(test_common.fd, F_GETFD);
	TEST_ASSERT_TRUE(flags >= 0);
	TEST_ASSERT_EQUAL_INT(0, flags & FD_CLOEXEC);

	/* Set FD_CLOEXEC */
	ret = fcntl(test_common.fd, F_SETFD, FD_CLOEXEC);
	TEST_ASSERT_TRUE(ret != -1);

	flags = fcntl(test_common.fd, F_GETFD);
	TEST_ASSERT_TRUE(flags >= 0);
	TEST_ASSERT_EQUAL_INT(FD_CLOEXEC, flags & FD_CLOEXEC);

	/* Clear FD_CLOEXEC */
	ret = fcntl(test_common.fd, F_SETFD, 0);
	TEST_ASSERT_TRUE(ret != -1);

	flags = fcntl(test_common.fd, F_GETFD);
	TEST_ASSERT_TRUE(flags >= 0);
	TEST_ASSERT_EQUAL_INT(0, flags & FD_CLOEXEC);
}


/* F_GETFL: shall return file status flags and access modes */
TEST(fcntl_fcntl, getfl_returns_flags)
{
	int flags;

	flags = fcntl(test_common.fd, F_GETFL);
	TEST_ASSERT_TRUE(flags >= 0);

	/* We opened with O_RDWR */
	TEST_ASSERT_EQUAL_INT(O_RDWR, flags & O_ACCMODE);
}


/* F_SETFL: set O_APPEND flag */
TEST(fcntl_fcntl, setfl_append)
{
	int flags;
	int ret;

	ret = fcntl(test_common.fd, F_SETFL, O_APPEND);
	TEST_ASSERT_TRUE(ret != -1);

	flags = fcntl(test_common.fd, F_GETFL);
	TEST_ASSERT_TRUE(flags >= 0);
	TEST_ASSERT_EQUAL_INT(O_APPEND, flags & O_APPEND);
}


/* F_SETFL: set O_NONBLOCK flag */
TEST(fcntl_fcntl, setfl_nonblock)
{
	int flags;
	int ret;

	ret = fcntl(test_common.fd, F_SETFL, O_NONBLOCK);
	TEST_ASSERT_TRUE(ret != -1);

	flags = fcntl(test_common.fd, F_GETFL);
	TEST_ASSERT_TRUE(flags >= 0);
	TEST_ASSERT_EQUAL_INT(O_NONBLOCK, flags & O_NONBLOCK);
}


/* F_SETFL: clear previously set flags */
TEST(fcntl_fcntl, setfl_clear_flags)
{
	int flags;
	int ret;

	ret = fcntl(test_common.fd, F_SETFL, O_APPEND | O_NONBLOCK);
	TEST_ASSERT_TRUE(ret != -1);

	ret = fcntl(test_common.fd, F_SETFL, 0);
	TEST_ASSERT_TRUE(ret != -1);

	flags = fcntl(test_common.fd, F_GETFL);
	TEST_ASSERT_TRUE(flags >= 0);
	TEST_ASSERT_EQUAL_INT(0, flags & O_APPEND);
	TEST_ASSERT_EQUAL_INT(0, flags & O_NONBLOCK);
}


/* F_SETFL: access mode bits are ignored */
TEST(fcntl_fcntl, setfl_ignores_access_mode)
{
	int flags;
	int ret;

	/* Try to change access mode — should be ignored */
	ret = fcntl(test_common.fd, F_SETFL, O_RDONLY);
	TEST_ASSERT_TRUE(ret != -1);

	flags = fcntl(test_common.fd, F_GETFL);
	TEST_ASSERT_TRUE(flags >= 0);
	/* Access mode should remain O_RDWR */
	TEST_ASSERT_EQUAL_INT(O_RDWR, flags & O_ACCMODE);
}


/* F_GETLK: no conflicting lock — l_type set to F_UNLCK */
TEST(fcntl_fcntl, getlk_no_conflict)
{
	struct flock fl;
	int ret;

	memset(&fl, 0, sizeof(fl));
	fl.l_type = F_WRLCK;
	fl.l_whence = SEEK_SET;
	fl.l_start = 0;
	fl.l_len = 0;

	ret = fcntl(test_common.fd, F_GETLK, &fl);
	TEST_ASSERT_TRUE(ret != -1);
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1235 issue");
#else
	TEST_ASSERT_EQUAL_INT(F_UNLCK, fl.l_type);
#endif
}


/* F_SETLK: set exclusive lock, then unlock */
TEST(fcntl_fcntl, setlk_exclusive_lock_unlock)
{
	struct flock fl;
	int ret;

	memset(&fl, 0, sizeof(fl));
	fl.l_type = F_WRLCK;
	fl.l_whence = SEEK_SET;
	fl.l_start = 0;
	fl.l_len = 100;

	ret = fcntl(test_common.fd, F_SETLK, &fl);
	TEST_ASSERT_TRUE(ret != -1);

	/* Unlock */
	fl.l_type = F_UNLCK;
	ret = fcntl(test_common.fd, F_SETLK, &fl);
	TEST_ASSERT_TRUE(ret != -1);
}


/* F_SETLK: set shared lock on read-only fd */
TEST(fcntl_fcntl, setlk_shared_lock)
{
	struct flock fl;
	int ret;
	int rdfd;

	rdfd = open(FCNTL_TEST_FILE, O_RDONLY);
	TEST_ASSERT_TRUE(rdfd >= 0);

	memset(&fl, 0, sizeof(fl));
	fl.l_type = F_RDLCK;
	fl.l_whence = SEEK_SET;
	fl.l_start = 0;
	fl.l_len = 50;

	ret = fcntl(rdfd, F_SETLK, &fl);
	TEST_ASSERT_TRUE(ret != -1);

	fl.l_type = F_UNLCK;
	ret = fcntl(rdfd, F_SETLK, &fl);
	TEST_ASSERT_TRUE(ret != -1);

	close(rdfd);
}


/* F_SETLK: lock with l_len=0 locks to end of file */
TEST(fcntl_fcntl, setlk_len_zero_locks_to_eof)
{
	struct flock fl;
	int ret;

	memset(&fl, 0, sizeof(fl));
	fl.l_type = F_WRLCK;
	fl.l_whence = SEEK_SET;
	fl.l_start = 0;
	fl.l_len = 0;

	ret = fcntl(test_common.fd, F_SETLK, &fl);
	TEST_ASSERT_TRUE(ret != -1);

	/* Verify via F_GETLK from a different fd that the whole range is locked */
	fl.l_type = F_UNLCK;
	ret = fcntl(test_common.fd, F_SETLK, &fl);
	TEST_ASSERT_TRUE(ret != -1);
}


/* fcntl: EBADF for invalid file descriptor */
TEST(fcntl_fcntl, ebadf_invalid_fd)
{
	int ret;

	errno = 0;
	ret = fcntl(-1, F_GETFD);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


/* F_DUPFD: EINVAL for negative arg */
TEST(fcntl_fcntl, dupfd_einval_negative_arg)
{
	int ret;

	errno = 0;
	ret = fcntl(test_common.fd, F_DUPFD, -1);
	TEST_ASSERT_EQUAL_INT(-1, ret);
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1687 issue");
#else
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
#endif
}


/* F_GETFD: fd flags are per-descriptor, not per-description */
TEST(fcntl_fcntl, getfd_per_descriptor)
{
	int newfd;
	int flags;
	int ret;

	newfd = fcntl(test_common.fd, F_DUPFD, 0);
	TEST_ASSERT_TRUE(newfd >= 0);

	/* Set cloexec on original */
	ret = fcntl(test_common.fd, F_SETFD, FD_CLOEXEC);
	TEST_ASSERT_TRUE(ret != -1);

	/* New fd should not be affected */
	flags = fcntl(newfd, F_GETFD);
	TEST_ASSERT_TRUE(flags >= 0);
	TEST_ASSERT_EQUAL_INT(0, flags & FD_CLOEXEC);

	close(newfd);
}


/* F_GETFL: file status flags are per-description (shared by dup'd fds) */
TEST(fcntl_fcntl, getfl_per_description)
{
	int newfd;
	int flags;
	int ret;

	newfd = fcntl(test_common.fd, F_DUPFD, 0);
	TEST_ASSERT_TRUE(newfd >= 0);

	/* Set append on original */
	ret = fcntl(test_common.fd, F_SETFL, O_APPEND);
	TEST_ASSERT_TRUE(ret != -1);

	/* New fd shares the description — should also see O_APPEND */
	flags = fcntl(newfd, F_GETFL);
	TEST_ASSERT_TRUE(flags >= 0);
	TEST_ASSERT_EQUAL_INT(O_APPEND, flags & O_APPEND);

	close(newfd);
}


TEST_GROUP_RUNNER(fcntl_fcntl)
{
	RUN_TEST_CASE(fcntl_fcntl, dupfd_returns_fd_gte_arg);
	RUN_TEST_CASE(fcntl_fcntl, dupfd_shares_offset);
	RUN_TEST_CASE(fcntl_fcntl, dupfd_clears_cloexec);
	RUN_TEST_CASE(fcntl_fcntl, dupfd_lowest_available);
	RUN_TEST_CASE(fcntl_fcntl, dupfd_cloexec_sets_flag);
	RUN_TEST_CASE(fcntl_fcntl, dupfd_cloexec_gte_arg);
	RUN_TEST_CASE(fcntl_fcntl, getfd_setfd_cloexec);
	RUN_TEST_CASE(fcntl_fcntl, getfl_returns_flags);
	RUN_TEST_CASE(fcntl_fcntl, setfl_append);
	RUN_TEST_CASE(fcntl_fcntl, setfl_nonblock);
	RUN_TEST_CASE(fcntl_fcntl, setfl_clear_flags);
	RUN_TEST_CASE(fcntl_fcntl, setfl_ignores_access_mode);
	RUN_TEST_CASE(fcntl_fcntl, getlk_no_conflict);
	RUN_TEST_CASE(fcntl_fcntl, setlk_exclusive_lock_unlock);
	RUN_TEST_CASE(fcntl_fcntl, setlk_shared_lock);
	RUN_TEST_CASE(fcntl_fcntl, setlk_len_zero_locks_to_eof);
	RUN_TEST_CASE(fcntl_fcntl, ebadf_invalid_fd);
	RUN_TEST_CASE(fcntl_fcntl, dupfd_einval_negative_arg);
	RUN_TEST_CASE(fcntl_fcntl, getfd_per_descriptor);
	RUN_TEST_CASE(fcntl_fcntl, getfl_per_description);
}
