/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <stropts.h>
 * TESTED:
 *    - fattach()
 *    - fdetach()
 *    - isastream()
 *
 * NOTE: the functions belong to the obsolescent XSI STREAMS option group. Implementations
 * not supporting the option (e.g. Phoenix-RTOS, glibc >= 2.30) don't provide <stropts.h>,
 * then all the test cases are ignored.
 *
 * Copyright 2026 Phoenix Systems
 * Author: Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#if defined(__has_include)
#if __has_include(<stropts.h>)
#include <stropts.h>
#define TEST_HAS_STROPTS 1
#endif
#endif

#include "unity_fixture.h"

#define STROPTS_FILE      "test_stropts_file"
#define STROPTS_FILE_MODE 0640
#define STROPTS_SLINK     "test_stropts_loop"
#define STROPTS_NOENT     "test_stropts_noent"
#define STROPTS_NOTDIR    STROPTS_FILE "/file"
#define STROPTS_TRAILING  STROPTS_FILE "/"


#ifdef TEST_HAS_STROPTS
static struct {
	int streamsFds[2];
	int fd;
	char longName[NAME_MAX + 2];
} test_common;


/* skips the test case if the XSI STREAMS option is not supported at runtime */
static void test_requireStreams(void)
{
#if defined(_XOPEN_STREAMS) && (_XOPEN_STREAMS > 0)
	const long supported = 1;
#elif defined(_SC_XOPEN_STREAMS)
	const long supported = sysconf(_SC_XOPEN_STREAMS);
#else
	const long supported = -1;
#endif

	if (supported <= 0) {
		TEST_IGNORE_MESSAGE("XSI STREAMS option not supported");
	}
}


/* opens a STREAMS-based file to test_common.streamsFds, there is no portable way to create one other than pipe() */
static int test_streamsFd(void)
{
	TEST_ASSERT_EQUAL_INT(0, pipe(test_common.streamsFds));
	if (isastream(test_common.streamsFds[0]) != 1) {
		TEST_IGNORE_MESSAGE("pipes are not STREAMS-based - no STREAMS file available");
	}

	return test_common.streamsFds[0];
}


static void test_setUp(void)
{
	int fd;

	test_common.streamsFds[0] = -1;
	test_common.streamsFds[1] = -1;
	test_common.fd = -1;
	memset(test_common.longName, 'a', sizeof(test_common.longName) - 1);
	test_common.longName[sizeof(test_common.longName) - 1] = '\0';

	fdetach(STROPTS_FILE);
	unlink(STROPTS_FILE);
	unlink(STROPTS_SLINK);
	fd = open(STROPTS_FILE, O_WRONLY | O_CREAT | O_TRUNC, STROPTS_FILE_MODE);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);
	TEST_ASSERT_EQUAL_INT(0, close(fd));
	TEST_ASSERT_EQUAL_INT(0, symlink(STROPTS_SLINK, STROPTS_SLINK));
}


static void test_tearDown(void)
{
	if (test_common.fd >= 0) {
		close(test_common.fd);
	}
	if (test_common.streamsFds[0] >= 0) {
		fdetach(STROPTS_FILE);
		close(test_common.streamsFds[0]);
		close(test_common.streamsFds[1]);
	}
	unlink(STROPTS_FILE);
	unlink(STROPTS_SLINK);
}
#else
static void test_requireStreams(void)
{
	TEST_IGNORE_MESSAGE("XSI STREAMS option not supported - no <stropts.h>");
}


static void test_setUp(void)
{
}


static void test_tearDown(void)
{
}
#endif


TEST_GROUP(stropts_isastream);

TEST_SETUP(stropts_isastream)
{
	test_requireStreams();
	test_setUp();
}


TEST_TEAR_DOWN(stropts_isastream)
{
	test_tearDown();
}


TEST(stropts_isastream, regular_file_returns_zero)
{
#ifdef TEST_HAS_STROPTS
	test_common.fd = open(STROPTS_FILE, O_RDONLY);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_common.fd);

	TEST_ASSERT_EQUAL_INT(0, isastream(test_common.fd));
#endif
}


TEST(stropts_isastream, directory_returns_zero)
{
#ifdef TEST_HAS_STROPTS
	test_common.fd = open(".", O_RDONLY);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_common.fd);

	TEST_ASSERT_EQUAL_INT(0, isastream(test_common.fd));
#endif
}


TEST(stropts_isastream, pipe_returns_zero_or_one)
{
#ifdef TEST_HAS_STROPTS
	int ret;

	TEST_ASSERT_EQUAL_INT(0, pipe(test_common.streamsFds));

	/* pipes may or may not be STREAMS-based, but the result is the same for both ends */
	ret = isastream(test_common.streamsFds[0]);
	TEST_ASSERT_TRUE((ret == 0) || (ret == 1));
	TEST_ASSERT_EQUAL_INT(ret, isastream(test_common.streamsFds[1]));
#endif
}


TEST(stropts_isastream, ebadf_invalid_fd)
{
#ifdef TEST_HAS_STROPTS
	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, isastream(-1));
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
#endif
}


TEST(stropts_isastream, ebadf_closed_fd)
{
#ifdef TEST_HAS_STROPTS
	int fd = open(STROPTS_FILE, O_RDONLY);

	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);
	TEST_ASSERT_EQUAL_INT(0, close(fd));

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, isastream(fd));
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
#endif
}


TEST_GROUP_RUNNER(stropts_isastream)
{
	RUN_TEST_CASE(stropts_isastream, regular_file_returns_zero);
	RUN_TEST_CASE(stropts_isastream, directory_returns_zero);
	RUN_TEST_CASE(stropts_isastream, pipe_returns_zero_or_one);
	RUN_TEST_CASE(stropts_isastream, ebadf_invalid_fd);
	RUN_TEST_CASE(stropts_isastream, ebadf_closed_fd);
}


TEST_GROUP(stropts_fattach);

TEST_SETUP(stropts_fattach)
{
	test_requireStreams();
	test_setUp();
}


TEST_TEAR_DOWN(stropts_fattach)
{
	test_tearDown();
}


TEST(stropts_fattach, path_names_streams_file)
{
#ifdef TEST_HAS_STROPTS
	struct stat fileSt, attachedSt, streamsSt;
	const int sfd = test_streamsFd();

	TEST_ASSERT_EQUAL_INT(0, stat(STROPTS_FILE, &fileSt));
	TEST_ASSERT_EQUAL_INT(0, fstat(sfd, &streamsSt));

	TEST_ASSERT_EQUAL_INT(0, fattach(sfd, STROPTS_FILE));

	test_common.fd = open(STROPTS_FILE, O_RDONLY);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_common.fd);
	TEST_ASSERT_EQUAL_INT(1, isastream(test_common.fd));

	/* permissions, owner and times come from the underlying file, size and device from the STREAMS file */
	TEST_ASSERT_EQUAL_INT(0, stat(STROPTS_FILE, &attachedSt));
	TEST_ASSERT_EQUAL_INT(fileSt.st_mode & (mode_t)07777, attachedSt.st_mode & (mode_t)07777);
	TEST_ASSERT_EQUAL_INT(fileSt.st_uid, attachedSt.st_uid);
	TEST_ASSERT_EQUAL_INT(fileSt.st_gid, attachedSt.st_gid);
	TEST_ASSERT_EQUAL_INT64((int64_t)fileSt.st_mtime, (int64_t)attachedSt.st_mtime);
	TEST_ASSERT_EQUAL_INT(1, attachedSt.st_nlink);
	TEST_ASSERT_EQUAL_INT64((int64_t)streamsSt.st_size, (int64_t)attachedSt.st_size);
	TEST_ASSERT_EQUAL_INT64((int64_t)streamsSt.st_dev, (int64_t)attachedSt.st_dev);
#endif
}


TEST(stropts_fattach, attribute_change_does_not_affect_underlying_file)
{
#ifdef TEST_HAS_STROPTS
	struct stat st;
	const int sfd = test_streamsFd();

	TEST_ASSERT_EQUAL_INT(0, fattach(sfd, STROPTS_FILE));
	TEST_ASSERT_EQUAL_INT(0, chmod(STROPTS_FILE, 0600));
	TEST_ASSERT_EQUAL_INT(0, fdetach(STROPTS_FILE));

	TEST_ASSERT_EQUAL_INT(0, stat(STROPTS_FILE, &st));
	TEST_ASSERT_EQUAL_INT(STROPTS_FILE_MODE, st.st_mode & (mode_t)07777);
#endif
}


TEST(stropts_fattach, fd_opened_before_refers_to_underlying_file)
{
#ifdef TEST_HAS_STROPTS
	const int sfd = test_streamsFd();

	test_common.fd = open(STROPTS_FILE, O_RDONLY);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_common.fd);

	TEST_ASSERT_EQUAL_INT(0, fattach(sfd, STROPTS_FILE));
	TEST_ASSERT_EQUAL_INT(0, isastream(test_common.fd));
#endif
}


TEST(stropts_fattach, ebadf_invalid_fd)
{
#ifdef TEST_HAS_STROPTS
	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, fattach(-1, STROPTS_FILE));
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
#endif
}


TEST(stropts_fattach, ebusy_already_attached)
{
#ifdef TEST_HAS_STROPTS
	const int sfd = test_streamsFd();

	TEST_ASSERT_EQUAL_INT(0, fattach(sfd, STROPTS_FILE));

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, fattach(test_common.streamsFds[1], STROPTS_FILE));
	TEST_ASSERT_EQUAL_INT(EBUSY, errno);
#endif
}


TEST(stropts_fattach, enoent_missing_or_empty_path)
{
#ifdef TEST_HAS_STROPTS
	const int sfd = test_streamsFd();

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, fattach(sfd, STROPTS_NOENT));
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, fattach(sfd, ""));
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);
#endif
}


TEST(stropts_fattach, enotdir_path_prefix_or_trailing_slash)
{
#ifdef TEST_HAS_STROPTS
	const int sfd = test_streamsFd();

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, fattach(sfd, STROPTS_NOTDIR));
	TEST_ASSERT_EQUAL_INT(ENOTDIR, errno);

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, fattach(sfd, STROPTS_TRAILING));
	TEST_ASSERT_EQUAL_INT(ENOTDIR, errno);
#endif
}


TEST(stropts_fattach, enametoolong_component)
{
#ifdef TEST_HAS_STROPTS
	const int sfd = test_streamsFd();

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, fattach(sfd, test_common.longName));
	TEST_ASSERT_EQUAL_INT(ENAMETOOLONG, errno);
#endif
}


TEST(stropts_fattach, eloop_symlink_loop)
{
#ifdef TEST_HAS_STROPTS
	const int sfd = test_streamsFd();

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, fattach(sfd, STROPTS_SLINK));
	TEST_ASSERT_EQUAL_INT(ELOOP, errno);
#endif
}


TEST_GROUP_RUNNER(stropts_fattach)
{
	RUN_TEST_CASE(stropts_fattach, path_names_streams_file);
	RUN_TEST_CASE(stropts_fattach, attribute_change_does_not_affect_underlying_file);
	RUN_TEST_CASE(stropts_fattach, fd_opened_before_refers_to_underlying_file);
	RUN_TEST_CASE(stropts_fattach, ebadf_invalid_fd);
	RUN_TEST_CASE(stropts_fattach, ebusy_already_attached);
	RUN_TEST_CASE(stropts_fattach, enoent_missing_or_empty_path);
	RUN_TEST_CASE(stropts_fattach, enotdir_path_prefix_or_trailing_slash);
	RUN_TEST_CASE(stropts_fattach, enametoolong_component);
	RUN_TEST_CASE(stropts_fattach, eloop_symlink_loop);
}


TEST_GROUP(stropts_fdetach);

TEST_SETUP(stropts_fdetach)
{
	test_requireStreams();
	test_setUp();
}


TEST_TEAR_DOWN(stropts_fdetach)
{
	test_tearDown();
}


TEST(stropts_fdetach, path_names_underlying_file_again)
{
#ifdef TEST_HAS_STROPTS
	const int sfd = test_streamsFd();

	TEST_ASSERT_EQUAL_INT(0, fattach(sfd, STROPTS_FILE));
	TEST_ASSERT_EQUAL_INT(0, fdetach(STROPTS_FILE));

	test_common.fd = open(STROPTS_FILE, O_RDONLY);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_common.fd);
	TEST_ASSERT_EQUAL_INT(0, isastream(test_common.fd));
#endif
}


TEST(stropts_fdetach, fd_opened_while_attached_refers_to_streams_file)
{
#ifdef TEST_HAS_STROPTS
	const int sfd = test_streamsFd();

	TEST_ASSERT_EQUAL_INT(0, fattach(sfd, STROPTS_FILE));
	test_common.fd = open(STROPTS_FILE, O_RDONLY);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_common.fd);

	TEST_ASSERT_EQUAL_INT(0, fdetach(STROPTS_FILE));
	TEST_ASSERT_EQUAL_INT(1, isastream(test_common.fd));
#endif
}


TEST(stropts_fdetach, einval_not_attached)
{
#ifdef TEST_HAS_STROPTS
	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, fdetach(STROPTS_FILE));
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
#endif
}


TEST(stropts_fdetach, enoent_missing_or_empty_path)
{
#ifdef TEST_HAS_STROPTS
	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, fdetach(STROPTS_NOENT));
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, fdetach(""));
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);
#endif
}


TEST(stropts_fdetach, enotdir_path_prefix_or_trailing_slash)
{
#ifdef TEST_HAS_STROPTS
	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, fdetach(STROPTS_NOTDIR));
	TEST_ASSERT_EQUAL_INT(ENOTDIR, errno);

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, fdetach(STROPTS_TRAILING));
	TEST_ASSERT_EQUAL_INT(ENOTDIR, errno);
#endif
}


TEST(stropts_fdetach, enametoolong_component)
{
#ifdef TEST_HAS_STROPTS
	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, fdetach(test_common.longName));
	TEST_ASSERT_EQUAL_INT(ENAMETOOLONG, errno);
#endif
}


TEST(stropts_fdetach, eloop_symlink_loop)
{
#ifdef TEST_HAS_STROPTS
	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, fdetach(STROPTS_SLINK));
	TEST_ASSERT_EQUAL_INT(ELOOP, errno);
#endif
}


TEST_GROUP_RUNNER(stropts_fdetach)
{
	RUN_TEST_CASE(stropts_fdetach, path_names_underlying_file_again);
	RUN_TEST_CASE(stropts_fdetach, fd_opened_while_attached_refers_to_streams_file);
	RUN_TEST_CASE(stropts_fdetach, einval_not_attached);
	RUN_TEST_CASE(stropts_fdetach, enoent_missing_or_empty_path);
	RUN_TEST_CASE(stropts_fdetach, enotdir_path_prefix_or_trailing_slash);
	RUN_TEST_CASE(stropts_fdetach, enametoolong_component);
	RUN_TEST_CASE(stropts_fdetach, eloop_symlink_loop);
}
