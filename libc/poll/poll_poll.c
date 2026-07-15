/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - poll.h
 * TESTED:
 *    - poll()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <time.h>
#include <signal.h>
#include <sys/stat.h>

#include "common.h"
#include "unity_fixture.h"

#define POLL_TEST_FILE "/tmp/test_poll_file"
#define POLL_TEST_FIFO "/tmp/test_poll_fifo"
#define POLL_TEST_DATA "poll data"

#define MS_TOLERANCE 50


static struct {
	int fd;
	int pipeFd[2];
} test_common;


static long test_elapsedMs(const struct timespec *t0, const struct timespec *t1)
{
	return (t1->tv_sec - t0->tv_sec) * 1000L + (t1->tv_nsec - t0->tv_nsec) / 1000000L;
}


TEST_GROUP(poll_poll);


TEST_SETUP(poll_poll)
{
	unlink(POLL_TEST_FILE);
	unlink(POLL_TEST_FIFO);

	test_common.fd = -1;
	test_common.pipeFd[0] = -1;
	test_common.pipeFd[1] = -1;
}


TEST_TEAR_DOWN(poll_poll)
{
	if (test_common.fd >= 0) {
		close(test_common.fd);
	}
	if (test_common.pipeFd[0] >= 0) {
		close(test_common.pipeFd[0]);
	}
	if (test_common.pipeFd[1] >= 0) {
		close(test_common.pipeFd[1]);
	}
	unlink(POLL_TEST_FILE);
	unlink(POLL_TEST_FIFO);
}


TEST(poll_poll, poll_regular_file_pollin_pollout)
{
	TEST_IGNORE_MESSAGE("no fs, todo: make it conditional");
	struct pollfd pfd;
	int ret;

	ret = _create_file(POLL_TEST_FILE, POLL_TEST_DATA);
	TEST_ASSERT_EQUAL_INT(0, ret);

	test_common.fd = open(POLL_TEST_FILE, O_RDWR);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_common.fd);

	/* Regular files shall always poll TRUE for reading and writing */
	pfd.fd = test_common.fd;
	pfd.events = POLLIN | POLLOUT;
	pfd.revents = 0;

	ret = poll(&pfd, 1, 0);
	TEST_ASSERT_EQUAL_INT(1, ret);
	TEST_ASSERT_NOT_EQUAL_INT(0, pfd.revents & POLLIN);
	TEST_ASSERT_NOT_EQUAL_INT(0, pfd.revents & POLLOUT);
}


TEST(poll_poll, poll_timeout_zero_returns_immediately)
{
	TEST_IGNORE_MESSAGE("no posixsrv, todo: make it conditional");
	struct pollfd pfd;
	struct timespec t0, t1;
	long elapsed;
	int ret;

	ret = pipe(test_common.pipeFd);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* pipe read end with no data, timeout=0 should return immediately with 0 */
	pfd.fd = test_common.pipeFd[0];
	pfd.events = POLLIN;
	pfd.revents = 0;

	ret = clock_gettime(CLOCK_MONOTONIC, &t0);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = poll(&pfd, 1, 0);

	ret = clock_gettime(CLOCK_MONOTONIC, &t1);
	TEST_ASSERT_EQUAL_INT(0, ret);

	elapsed = test_elapsedMs(&t0, &t1);
	TEST_ASSERT_LESS_THAN_INT(MS_TOLERANCE, (int)elapsed);
}


TEST(poll_poll, poll_timeout_expires)
{
	TEST_IGNORE_MESSAGE("no posixsrv, todo: make it conditional");
	struct pollfd pfd;
	struct timespec t0, t1;
	long elapsed;
	int ret;
	const int timeoutMs = 100;

	ret = pipe(test_common.pipeFd);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* pipe read end with no data, should wait at least timeoutMs */
	pfd.fd = test_common.pipeFd[0];
	pfd.events = POLLIN;
	pfd.revents = 0;

	ret = clock_gettime(CLOCK_MONOTONIC, &t0);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = poll(&pfd, 1, timeoutMs);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = clock_gettime(CLOCK_MONOTONIC, &t1);
	TEST_ASSERT_EQUAL_INT(0, ret);

	elapsed = test_elapsedMs(&t0, &t1);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(timeoutMs, (int)elapsed);
}


TEST(poll_poll, poll_returns_count_of_ready_fds)
{
	TEST_IGNORE_MESSAGE("no posixsrv, todo: make it conditional");
	struct pollfd pfds[2];
	int ret;

	ret = pipe(test_common.pipeFd);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* write end is always writable, read end has no data */
	pfds[0].fd = test_common.pipeFd[0];
	pfds[0].events = POLLIN;
	pfds[0].revents = 0;

	pfds[1].fd = test_common.pipeFd[1];
	pfds[1].events = POLLOUT;
	pfds[1].revents = 0;

	ret = poll(pfds, 2, 0);
	/* at least the write end should be ready */
	TEST_ASSERT_GREATER_OR_EQUAL_INT(1, ret);
	TEST_ASSERT_NOT_EQUAL_INT(0, pfds[1].revents & POLLOUT);
}


TEST(poll_poll, poll_pipe_readable_after_write)
{
	TEST_IGNORE_MESSAGE("no posixsrv, todo: make it conditional");
	struct pollfd pfd;
	const char data = 'x';
	ssize_t n;
	int ret;

	ret = pipe(test_common.pipeFd);
	TEST_ASSERT_EQUAL_INT(0, ret);

	n = write(test_common.pipeFd[1], &data, 1);
	TEST_ASSERT_EQUAL_INT(1, (int)n);

	pfd.fd = test_common.pipeFd[0];
	pfd.events = POLLIN;
	pfd.revents = 0;

	ret = poll(&pfd, 1, 0);
	TEST_ASSERT_EQUAL_INT(1, ret);
	TEST_ASSERT_NOT_EQUAL_INT(0, pfd.revents & POLLIN);
}


TEST(poll_poll, poll_pipe_hangup_on_writer_close)
{
	TEST_IGNORE_MESSAGE("no posixsrv, todo: make it conditional");
	struct pollfd pfd;
	int ret;

	ret = pipe(test_common.pipeFd);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* close write end */
	close(test_common.pipeFd[1]);
	test_common.pipeFd[1] = -1;

	pfd.fd = test_common.pipeFd[0];
	pfd.events = POLLIN;
	pfd.revents = 0;

	ret = poll(&pfd, 1, 0);
	TEST_ASSERT_EQUAL_INT(1, ret);
	TEST_ASSERT_NOT_EQUAL_INT(0, pfd.revents & POLLHUP);
}


TEST(poll_poll, poll_negative_fd_ignored)
{
	TEST_IGNORE_MESSAGE("no posixsrv, todo: make it conditional");
	struct pollfd pfd;
	int ret;

	/* If fd < 0, events shall be ignored and revents set to 0 */
	pfd.fd = -1;
	pfd.events = POLLIN | POLLOUT;
	pfd.revents = 0xff;

	ret = poll(&pfd, 1, 0);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, (int)pfd.revents);
}


TEST(poll_poll, poll_invalid_fd_pollnval)
{
	struct pollfd pfd;
	int ret;

	/* closed fd should get POLLNVAL in revents */
	pfd.fd = 999;
	pfd.events = POLLIN;
	pfd.revents = 0;

	ret = poll(&pfd, 1, 0);
	TEST_ASSERT_EQUAL_INT(1, ret);
	TEST_ASSERT_NOT_EQUAL_INT(0, pfd.revents & POLLNVAL);
}


TEST(poll_poll, poll_pollerr_pollhup_not_in_events)
{
	TEST_IGNORE_MESSAGE("no posixsrv, todo: make it conditional");
	struct pollfd pfd;
	int ret;

	ret = pipe(test_common.pipeFd);
	TEST_ASSERT_EQUAL_INT(0, ret);

	close(test_common.pipeFd[1]);
	test_common.pipeFd[1] = -1;

	/* POLLHUP only valid in revents, shall be ignored in events.
	 * Even with events=0, poll should still report POLLHUP */
	pfd.fd = test_common.pipeFd[0];
	pfd.events = 0;
	pfd.revents = 0;

	ret = poll(&pfd, 1, 0);
	TEST_ASSERT_EQUAL_INT(1, ret);
	TEST_ASSERT_NOT_EQUAL_INT(0, pfd.revents & POLLHUP);
}


TEST(poll_poll, poll_multiple_fds_mixed)
{
	TEST_IGNORE_MESSAGE("no fs, todo: make it conditional");
	struct pollfd pfds[3];
	int ret;

	ret = _create_file(POLL_TEST_FILE, POLL_TEST_DATA);
	TEST_ASSERT_EQUAL_INT(0, ret);

	test_common.fd = open(POLL_TEST_FILE, O_RDONLY);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_common.fd);

	/* fd 0: valid readable file */
	pfds[0].fd = test_common.fd;
	pfds[0].events = POLLIN;
	pfds[0].revents = 0;

	/* fd 1: negative fd, should be ignored */
	pfds[1].fd = -1;
	pfds[1].events = POLLIN;
	pfds[1].revents = 0xff;

	/* fd 2: invalid fd */
	pfds[2].fd = 998;
	pfds[2].events = POLLIN;
	pfds[2].revents = 0;

	ret = poll(pfds, 3, 0);
	/* regular file readable + invalid fd = 2 with non-zero revents */
	TEST_ASSERT_EQUAL_INT(2, ret);
	TEST_ASSERT_NOT_EQUAL_INT(0, pfds[0].revents & POLLIN);
	TEST_ASSERT_EQUAL_INT(0, (int)pfds[1].revents);
	TEST_ASSERT_NOT_EQUAL_INT(0, pfds[2].revents & POLLNVAL);
}


TEST(poll_poll, poll_nfds_zero)
{
	struct timespec t0, t1;
	long elapsed;
	int ret;
	const int timeoutMs = 50;

	/* nfds=0 with timeout should just wait and return 0 */
	ret = clock_gettime(CLOCK_MONOTONIC, &t0);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = poll(NULL, 0, timeoutMs);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = clock_gettime(CLOCK_MONOTONIC, &t1);
	TEST_ASSERT_EQUAL_INT(0, ret);

	elapsed = test_elapsedMs(&t0, &t1);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(timeoutMs, (int)elapsed);
}


TEST(poll_poll, poll_return_zero_on_timeout)
{
	TEST_IGNORE_MESSAGE("no posixsrv, todo: make it conditional");
	struct pollfd pfd;
	int ret;

	ret = pipe(test_common.pipeFd);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* no data on pipe, timeout=0 → returns 0 */
	pfd.fd = test_common.pipeFd[0];
	pfd.events = POLLIN;
	pfd.revents = 0;

	ret = poll(&pfd, 1, 0);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, (int)pfd.revents);
}


TEST(poll_poll, poll_revents_cleared_if_no_event)
{
	TEST_IGNORE_MESSAGE("no posixsrv, todo: make it conditional");
	struct pollfd pfd;
	int ret;

	ret = pipe(test_common.pipeFd);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* set revents to garbage, poll should clear it */
	pfd.fd = test_common.pipeFd[0];
	pfd.events = POLLIN;
	pfd.revents = 0xffff;

	ret = poll(&pfd, 1, 0);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, (int)pfd.revents);
}


TEST(poll_poll, poll_pollout_pipe_writable)
{
	TEST_IGNORE_MESSAGE("no posixsrv, todo: make it conditional");
	struct pollfd pfd;
	int ret;

	ret = pipe(test_common.pipeFd);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* write end of empty pipe should be writable */
	pfd.fd = test_common.pipeFd[1];
	pfd.events = POLLOUT;
	pfd.revents = 0;

	ret = poll(&pfd, 1, 0);
	TEST_ASSERT_EQUAL_INT(1, ret);
	TEST_ASSERT_NOT_EQUAL_INT(0, pfd.revents & POLLOUT);
}


TEST_GROUP_RUNNER(poll_poll)
{
	RUN_TEST_CASE(poll_poll, poll_regular_file_pollin_pollout);
	RUN_TEST_CASE(poll_poll, poll_timeout_zero_returns_immediately);
	RUN_TEST_CASE(poll_poll, poll_timeout_expires);
	RUN_TEST_CASE(poll_poll, poll_returns_count_of_ready_fds);
	RUN_TEST_CASE(poll_poll, poll_pipe_readable_after_write);
	RUN_TEST_CASE(poll_poll, poll_pipe_hangup_on_writer_close);
	RUN_TEST_CASE(poll_poll, poll_negative_fd_ignored);
	RUN_TEST_CASE(poll_poll, poll_invalid_fd_pollnval);
	RUN_TEST_CASE(poll_poll, poll_pollerr_pollhup_not_in_events);
	RUN_TEST_CASE(poll_poll, poll_multiple_fds_mixed);
	RUN_TEST_CASE(poll_poll, poll_nfds_zero);
	RUN_TEST_CASE(poll_poll, poll_return_zero_on_timeout);
	RUN_TEST_CASE(poll_poll, poll_revents_cleared_if_no_event);
	RUN_TEST_CASE(poll_poll, poll_pollout_pipe_writable);
}
