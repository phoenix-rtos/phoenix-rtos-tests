/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <unistd.h>
 * TESTED:
 *    - fchown()
 *    - fdatasync()
 *    - lockf()
 *    - sync()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>

#include "unity_fixture.h"

#define MISC_TEST_FILE "/tmp/test_fileops_misc"

static struct {
	int fd;
} test_common;


/* ========================================================================= */
/* fchown */
/* ========================================================================= */

TEST_GROUP(fileops_fchown);

TEST_SETUP(fileops_fchown)
{
	test_common.fd = -1;
	unlink(MISC_TEST_FILE);
	test_common.fd = open(MISC_TEST_FILE, O_RDWR | O_CREAT | O_TRUNC, 0644);
	TEST_ASSERT_TRUE(test_common.fd >= 0);
}

TEST_TEAR_DOWN(fileops_fchown)
{
	if (test_common.fd >= 0) {
		close(test_common.fd);
		test_common.fd = -1;
	}
	unlink(MISC_TEST_FILE);
}


TEST(fileops_fchown, fchown_no_change)
{
	int ret;

	/* Passing -1 for both owner and group means no change */
	ret = fchown(test_common.fd, (uid_t)-1, (gid_t)-1);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST(fileops_fchown, fchown_set_own_uid_gid)
{
	int ret;
	struct stat st;
	uid_t myUid;
	gid_t myGid;

	myUid = getuid();
	myGid = getgid();

	/* Setting to our own uid/gid should always succeed */
	ret = fchown(test_common.fd, myUid, myGid);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = fstat(test_common.fd, &st);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(myUid, st.st_uid);
	TEST_ASSERT_EQUAL_INT(myGid, st.st_gid);
}


TEST(fileops_fchown, fchown_ebadf)
{
#ifdef __phoenix__
	/* #1693 issue unpublished */
	TEST_IGNORE_MESSAGE("fchown not implemented");
#else
	int ret;

	errno = 0;
	ret = fchown(-1, getuid(), getgid());
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
#endif
}


TEST_GROUP_RUNNER(fileops_fchown)
{
	RUN_TEST_CASE(fileops_fchown, fchown_no_change);
	RUN_TEST_CASE(fileops_fchown, fchown_set_own_uid_gid);
	RUN_TEST_CASE(fileops_fchown, fchown_ebadf);
}


/* ========================================================================= */
/* fdatasync */
/* ========================================================================= */

#ifndef __phoenix__

TEST_GROUP(fileops_fdatasync);

TEST_SETUP(fileops_fdatasync)
{
	test_common.fd = -1;
	unlink(MISC_TEST_FILE);
	test_common.fd = open(MISC_TEST_FILE, O_RDWR | O_CREAT | O_TRUNC, 0644);
	TEST_ASSERT_TRUE(test_common.fd >= 0);
}

TEST_TEAR_DOWN(fileops_fdatasync)
{
	if (test_common.fd >= 0) {
		close(test_common.fd);
		test_common.fd = -1;
	}
	unlink(MISC_TEST_FILE);
}


TEST(fileops_fdatasync, fdatasync_success)
{
	int ret;
	const char data[] = "sync test data";
	ssize_t n;

	n = write(test_common.fd, data, strlen(data));
	TEST_ASSERT_EQUAL_INT((ssize_t)strlen(data), n);

	ret = fdatasync(test_common.fd);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST(fileops_fdatasync, fdatasync_empty_file)
{
	int ret;

	/* fdatasync on empty file should succeed */
	ret = fdatasync(test_common.fd);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST(fileops_fdatasync, fdatasync_ebadf)
{
	int ret;

	errno = 0;
	ret = fdatasync(-1);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


TEST_GROUP_RUNNER(fileops_fdatasync)
{
	RUN_TEST_CASE(fileops_fdatasync, fdatasync_success);
	RUN_TEST_CASE(fileops_fdatasync, fdatasync_empty_file);
	RUN_TEST_CASE(fileops_fdatasync, fdatasync_ebadf);
}
#else
TEST_GROUP_UNIMPLEMENTED(fileops_fdatasync, "fdatasync not implemented")
#endif


/* ========================================================================= */
/* lockf */
/* ========================================================================= */

#ifndef __phoenix__

TEST_GROUP(fileops_lockf);

TEST_SETUP(fileops_lockf)
{
	test_common.fd = -1;
	unlink(MISC_TEST_FILE);
	test_common.fd = open(MISC_TEST_FILE, O_RDWR | O_CREAT | O_TRUNC, 0644);
	TEST_ASSERT_TRUE(test_common.fd >= 0);
}

TEST_TEAR_DOWN(fileops_lockf)
{
	if (test_common.fd >= 0) {
		if (lockf(test_common.fd, F_ULOCK, 0) < 0) {
			/* best effort */
		}
		close(test_common.fd);
		test_common.fd = -1;
	}
	unlink(MISC_TEST_FILE);
}


TEST(fileops_lockf, lockf_lock_and_unlock)
{
	int ret;

	ret = lockf(test_common.fd, F_LOCK, 100);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = lockf(test_common.fd, F_ULOCK, 100);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST(fileops_lockf, lockf_tlock_success)
{
	int ret;

	ret = lockf(test_common.fd, F_TLOCK, 50);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = lockf(test_common.fd, F_ULOCK, 50);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST(fileops_lockf, lockf_test_unlocked)
{
	int ret;

	/* F_TEST on an unlocked region should succeed */
	ret = lockf(test_common.fd, F_TEST, 100);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST(fileops_lockf, lockf_tlock_conflict_other_process)
{
	int ret;
	pid_t childPid;
	int status;
	int pipeFds[2];

	ret = pipe(pipeFds);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Lock in parent */
	ret = lockf(test_common.fd, F_LOCK, 100);
	TEST_ASSERT_EQUAL_INT(0, ret);

	childPid = fork();
	TEST_ASSERT_TRUE(childPid >= 0);

	if (childPid == 0) {
		int childFd;
		int lockRet;
		char c = 'x';

		close(pipeFds[0]);

		childFd = open(MISC_TEST_FILE, O_RDWR);
		if (childFd < 0) {
			_exit(99);
		}

		/* Try non-blocking lock — should fail since parent holds it */
		errno = 0;
		lockRet = lockf(childFd, F_TLOCK, 100);
		if (lockRet == -1 && (errno == EAGAIN || errno == EACCES)) {
			if (write(pipeFds[1], &c, 1) < 0) {
				_exit(99);
			}
			close(pipeFds[1]);
			close(childFd);
			_exit(0);
		}
		close(pipeFds[1]);
		close(childFd);
		_exit(1);
	}

	close(pipeFds[1]);
	{
		char c;
		ret = read(pipeFds[0], &c, 1);
		TEST_ASSERT_EQUAL_INT(1, ret);
	}
	close(pipeFds[0]);

	childPid = waitpid(childPid, &status, 0);
	TEST_ASSERT_TRUE(childPid > 0);
	TEST_ASSERT_TRUE(WIFEXITED(status));
	TEST_ASSERT_EQUAL_INT(0, WEXITSTATUS(status));

	ret = lockf(test_common.fd, F_ULOCK, 100);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST(fileops_lockf, lockf_lock_size_zero_to_eof)
{
	int ret;

	/* Size 0 locks from current offset to end of file */
	ret = lockf(test_common.fd, F_LOCK, 0);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = lockf(test_common.fd, F_ULOCK, 0);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST(fileops_lockf, lockf_ebadf)
{
	int ret;

	errno = 0;
	ret = lockf(-1, F_LOCK, 100);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


TEST(fileops_lockf, lockf_einval_bad_function)
{
	int ret;

	errno = 0;
	ret = lockf(test_common.fd, 9999, 100);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST_GROUP_RUNNER(fileops_lockf)
{
	RUN_TEST_CASE(fileops_lockf, lockf_lock_and_unlock);
	RUN_TEST_CASE(fileops_lockf, lockf_tlock_success);
	RUN_TEST_CASE(fileops_lockf, lockf_test_unlocked);
	RUN_TEST_CASE(fileops_lockf, lockf_tlock_conflict_other_process);
	RUN_TEST_CASE(fileops_lockf, lockf_lock_size_zero_to_eof);
	RUN_TEST_CASE(fileops_lockf, lockf_ebadf);
	RUN_TEST_CASE(fileops_lockf, lockf_einval_bad_function);
}
#else
TEST_GROUP_UNIMPLEMENTED(fileops_lockf, "lockf not implemented")
#endif


/* ========================================================================= */
/* sync */
/* ========================================================================= */

TEST_GROUP(fileops_sync);

TEST_SETUP(fileops_sync) { }

TEST_TEAR_DOWN(fileops_sync) { }


TEST(fileops_sync, sync_does_not_crash)
{
	/* sync() returns void and has no defined errors; just verify it doesn't crash */
	sync();
}


TEST_GROUP_RUNNER(fileops_sync)
{
	RUN_TEST_CASE(fileops_sync, sync_does_not_crash);
}
