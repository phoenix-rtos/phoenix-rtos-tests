/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <stdio.h>
 * TESTED:
 *    - getchar_unlocked()
 *    - gets()
 *    - putchar_unlocked()
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
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>

#include "unity_fixture.h"

#define STDSTREAMS_FILE    "test_stdio_stdstreams"
#define TEST_FILE_MODE     0644
#define TEST_ALARM_S       1U
#define TEST_THREADS       2
#define TEST_CHUNK         64
#define TEST_CHUNKS        32
#define TEST_THREADED_SIZE (TEST_THREADS * TEST_CHUNKS * TEST_CHUNK)
#define TEST_WRITE_LIMIT   (1024 * 1024)
#define TEST_FSIZE_LIMIT   1024
#define TEST_LINE_MAX      1024
#define TEST_FILL          'X'


#ifndef __phoenix__
/*
 * gets() is still specified by POSIX.1-2017 (as obsolescent), but glibc declares it only
 * in pre-C11 compilation modes - it's still provided by the library.
 */
char *gets(char *s);
#endif


static struct {
	int savedStdin;
	int savedStdout;
	int pipeWr;
	bool sigSet;
	struct sigaction oldAct;
	volatile sig_atomic_t sigCount;
	uint8_t buf[TEST_THREADED_SIZE + 1];
	char line[TEST_LINE_MAX + 1];
} test_common;


static void test_sigHandler(int sig)
{
	(void)sig;
	test_common.sigCount++;
}


static int test_installHandler(int sig)
{
	struct sigaction act;

	memset(&act, 0, sizeof(act));
	act.sa_handler = test_sigHandler;
	sigemptyset(&act.sa_mask);
	/* no SA_RESTART - blocking I/O interrupted by the handler shall fail with EINTR */
	act.sa_flags = 0;
	test_common.sigCount = 0;

	return sigaction(sig, &act, &test_common.oldAct);
}


static void test_writeFile(const void *data, size_t len, int flags)
{
	int fd = open(STDSTREAMS_FILE, O_WRONLY | O_CREAT | flags, TEST_FILE_MODE);

	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);
	TEST_ASSERT_EQUAL_INT((int)len, (int)write(fd, data, len));
	TEST_ASSERT_EQUAL_INT(0, close(fd));
}


/* reads STDSTREAMS_FILE into test_common.buf, returns its length */
static size_t test_readFile(void)
{
	ssize_t len;
	int fd = open(STDSTREAMS_FILE, O_RDONLY);

	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);
	len = read(fd, test_common.buf, sizeof(test_common.buf) - 1);
	close(fd);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, (int)len);

	return (size_t)len;
}


/* sets the timestamps of the test file to the Epoch, returns the current time */
static time_t test_resetFileTimes(void)
{
	const struct utimbuf times = { .actime = 0, .modtime = 0 };
	time_t now;

	TEST_ASSERT_EQUAL_INT(0, utime(STDSTREAMS_FILE, &times));
	now = time(NULL);
	TEST_ASSERT_GREATER_THAN_INT64(0, (int64_t)now);

	return now;
}


/* makes fd the file descriptor underlying stdin, fd is consumed */
static void test_redirectStdin(int fd)
{
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);
	clearerr(stdin);
	test_common.savedStdin = dup(STDIN_FILENO);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_common.savedStdin);
	TEST_ASSERT_EQUAL_INT(STDIN_FILENO, dup2(fd, STDIN_FILENO));
	TEST_ASSERT_EQUAL_INT(0, close(fd));
}


static void test_stdinFromData(const void *data, size_t len)
{
	test_writeFile(data, len, O_TRUNC);
	test_redirectStdin(open(STDSTREAMS_FILE, O_RDONLY));
}


static void test_restoreStdin(void)
{
	if (test_common.pipeWr >= 0) {
		close(test_common.pipeWr);
		test_common.pipeWr = -1;
	}

	if (test_common.savedStdin >= 0) {
		/* drop data buffered from the redirected file, so that it's not seen by the next test */
		clearerr(stdin);
		while (getchar() != EOF) {
		}
		clearerr(stdin);
		dup2(test_common.savedStdin, STDIN_FILENO);
		close(test_common.savedStdin);
		test_common.savedStdin = -1;
	}
}


/* makes fd the file descriptor underlying stdout, fd is consumed */
static void test_redirectStdout(int fd)
{
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);
	TEST_ASSERT_EQUAL_INT(0, fflush(stdout));
	test_common.savedStdout = dup(STDOUT_FILENO);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_common.savedStdout);
	TEST_ASSERT_EQUAL_INT(STDOUT_FILENO, dup2(fd, STDOUT_FILENO));
	TEST_ASSERT_EQUAL_INT(0, close(fd));
}


/* NOTE: assertions output goes to stdout, so they shall be made only after restoring it */
static void test_restoreStdout(void)
{
	if (test_common.savedStdout >= 0) {
		fflush(stdout);
		clearerr(stdout);
		dup2(test_common.savedStdout, STDOUT_FILENO);
		close(test_common.savedStdout);
		test_common.savedStdout = -1;
	}
}


static void test_commonSetUp(void)
{
	test_common.savedStdin = -1;
	test_common.savedStdout = -1;
	test_common.pipeWr = -1;
	test_common.sigSet = false;
	test_common.sigCount = 0;
	unlink(STDSTREAMS_FILE);
}


static void test_commonTearDown(void)
{
	if (test_common.sigSet) {
		alarm(0);
		sigaction(SIGALRM, &test_common.oldAct, NULL);
		test_common.sigSet = false;
	}
	test_restoreStdout();
	test_restoreStdin();
	unlink(STDSTREAMS_FILE);
}


/*
 * Runs fn() in a child process, so that a stream left with unflushable data by a write error
 * does not affect the stdout of the test process. fn() returns 0 on success or a failed step number.
 */
static void test_runInChild(int (*fn)(void))
{
	pid_t pid;
	int status;

	TEST_ASSERT_EQUAL_INT(0, fflush(stdout));
	pid = fork();
	if ((pid < 0) && (errno == ENOSYS)) {
		TEST_IGNORE_MESSAGE("fork() not supported");
	}
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, pid);
	if (pid == 0) {
		_exit(fn());
	}

	TEST_ASSERT_EQUAL_INT(pid, waitpid(pid, &status, 0));
	TEST_ASSERT_TRUE(WIFEXITED(status));
	TEST_ASSERT_EQUAL_INT_MESSAGE(0, WEXITSTATUS(status), "failed step in child");
}


/* writes to stdout until putchar_unlocked() fails, returns 0 if it failed as expected */
static int test_childPutUntilError(int expectedErrno)
{
	int i;

	errno = 0;
	for (i = 0; i < TEST_WRITE_LIMIT; i++) {
		if (putchar_unlocked('x') == EOF) {
			break;
		}
	}

	if (i == TEST_WRITE_LIMIT) {
		return 10;
	}
	if (ferror(stdout) == 0) {
		return 11;
	}
	if (errno != expectedErrno) {
		return 12;
	}

	return 0;
}


TEST_GROUP(stdio_getchar_unlocked);

TEST_SETUP(stdio_getchar_unlocked)
{
	test_commonSetUp();
}


TEST_TEAR_DOWN(stdio_getchar_unlocked)
{
	test_commonTearDown();
}


TEST(stdio_getchar_unlocked, reads_bytes_in_order)
{
	const char data[] = "abc";

	test_stdinFromData(data, sizeof(data) - 1);

	TEST_ASSERT_EQUAL_INT('a', getchar_unlocked());
	TEST_ASSERT_EQUAL_INT('b', getchar_unlocked());
	TEST_ASSERT_EQUAL_INT('c', getchar_unlocked());
	TEST_ASSERT_EQUAL_INT(EOF, getchar_unlocked());
}


TEST(stdio_getchar_unlocked, returns_byte_as_unsigned_char)
{
	const uint8_t data[] = { 0x00U, 0x7fU, 0x80U, 0xffU };

	test_stdinFromData(data, sizeof(data));

	/* no byte value can be confused with EOF */
	TEST_ASSERT_EQUAL_INT(0x00, getchar_unlocked());
	TEST_ASSERT_EQUAL_INT(0x7f, getchar_unlocked());
	TEST_ASSERT_EQUAL_INT(0x80, getchar_unlocked());
	TEST_ASSERT_EQUAL_INT(0xff, getchar_unlocked());
	TEST_ASSERT_EQUAL_INT(0, feof(stdin));
	TEST_ASSERT_EQUAL_INT(EOF, getchar_unlocked());
}


TEST(stdio_getchar_unlocked, eof_sets_indicator)
{
	test_stdinFromData("", 0);

	TEST_ASSERT_EQUAL_INT(EOF, getchar_unlocked());
	TEST_ASSERT_NOT_EQUAL_INT(0, feof(stdin));
	TEST_ASSERT_EQUAL_INT(0, ferror(stdin));
}


TEST(stdio_getchar_unlocked, eof_indicator_set_returns_eof)
{
	const char first[] = "a";
	const char appended[] = "b";

	test_stdinFromData(first, sizeof(first) - 1);

	TEST_ASSERT_EQUAL_INT('a', getchar_unlocked());
	TEST_ASSERT_EQUAL_INT(EOF, getchar_unlocked());
	TEST_ASSERT_NOT_EQUAL_INT(0, feof(stdin));

	/* a next byte is now present, but the end-of-file indicator is still set */
	test_writeFile(appended, sizeof(appended) - 1, O_APPEND);
	TEST_ASSERT_EQUAL_INT(EOF, getchar_unlocked());
	TEST_ASSERT_NOT_EQUAL_INT(0, feof(stdin));

	clearerr(stdin);
	TEST_ASSERT_EQUAL_INT('b', getchar_unlocked());
}


TEST(stdio_getchar_unlocked, equivalent_to_getchar)
{
	const char data[] = "abcd";

	test_stdinFromData(data, sizeof(data) - 1);

	/* both functions operate on the same stream state */
	TEST_ASSERT_EQUAL_INT('a', getchar_unlocked());
	TEST_ASSERT_EQUAL_INT('b', getchar());
	TEST_ASSERT_EQUAL_INT('c', getchar_unlocked());
	TEST_ASSERT_EQUAL_INT('d', getc(stdin));
	TEST_ASSERT_EQUAL_INT(EOF, getchar_unlocked());
	TEST_ASSERT_NOT_EQUAL_INT(0, feof(stdin));
}


TEST(stdio_getchar_unlocked, ebadf_not_open_for_reading)
{
	test_writeFile("abc", 3, O_TRUNC);
	test_redirectStdin(open(STDSTREAMS_FILE, O_WRONLY));

	errno = 0;
	TEST_ASSERT_EQUAL_INT(EOF, getchar_unlocked());
	TEST_ASSERT_NOT_EQUAL_INT(0, ferror(stdin));
	TEST_ASSERT_EQUAL_INT(0, feof(stdin));
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


TEST(stdio_getchar_unlocked, eagain_nonblocking_no_data)
{
	int fds[2];

	TEST_ASSERT_EQUAL_INT(0, pipe(fds));
	test_common.pipeWr = fds[1];
	TEST_ASSERT_EQUAL_INT(0, fcntl(fds[0], F_SETFL, fcntl(fds[0], F_GETFL) | O_NONBLOCK));
	test_redirectStdin(fds[0]);

	errno = 0;
	TEST_ASSERT_EQUAL_INT(EOF, getchar_unlocked());
	TEST_ASSERT_NOT_EQUAL_INT(0, ferror(stdin));
	TEST_ASSERT_EQUAL_INT(0, feof(stdin));
	TEST_ASSERT_EQUAL_INT(EAGAIN, errno);
}


TEST(stdio_getchar_unlocked, eintr_read_interrupted)
{
	int fds[2];

#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1401 issue");
#endif

	TEST_ASSERT_EQUAL_INT(0, pipe(fds));
	test_common.pipeWr = fds[1];
	test_redirectStdin(fds[0]);

	TEST_ASSERT_EQUAL_INT(0, test_installHandler(SIGALRM));
	test_common.sigSet = true;
	alarm(TEST_ALARM_S);

	/* the pipe is empty and its write end is open - the read blocks until SIGALRM arrives */
	errno = 0;
	TEST_ASSERT_EQUAL_INT(EOF, getchar_unlocked());
	TEST_ASSERT_EQUAL_INT(1, test_common.sigCount);
	TEST_ASSERT_NOT_EQUAL_INT(0, ferror(stdin));
	TEST_ASSERT_EQUAL_INT(0, feof(stdin));
	TEST_ASSERT_EQUAL_INT(EINTR, errno);
}


TEST(stdio_getchar_unlocked, marks_atime_for_update)
{
	struct stat st;
	time_t now;

	test_stdinFromData("a", 1);
	now = test_resetFileTimes();

	TEST_ASSERT_EQUAL_INT('a', getchar_unlocked());
	TEST_ASSERT_EQUAL_INT(0, fstat(STDIN_FILENO, &st));
	TEST_ASSERT_GREATER_OR_EQUAL_INT64((int64_t)now, (int64_t)st.st_atime);
}


typedef struct {
	size_t count;
	size_t errors;
} test_readerResult_t;


static void *test_lockedReader(void *arg)
{
	test_readerResult_t *res = arg;
	int c = 0;
	int prev;
	int i;

	while (c != EOF) {
		flockfile(stdin);
		prev = -1;
		for (i = 0; i < TEST_CHUNK; i++) {
			c = getchar_unlocked();
			if (c == EOF) {
				break;
			}
			/* bytes read within one locked scope shall be consecutive bytes of the file */
			if ((prev >= 0) && (c != ((prev + 1) & 0xff))) {
				res->errors++;
			}
			prev = c;
			res->count++;
		}
		funlockfile(stdin);
		sched_yield();
	}

	return NULL;
}


TEST(stdio_getchar_unlocked, thread_safe_within_flockfile)
{
	pthread_t threads[TEST_THREADS];
	test_readerResult_t res[TEST_THREADS];
	size_t total = 0;
	size_t i;

	for (i = 0; i < sizeof(test_common.buf); i++) {
		test_common.buf[i] = (uint8_t)i;
	}
	test_stdinFromData(test_common.buf, TEST_THREADED_SIZE);

	memset(res, 0, sizeof(res));
	for (i = 0; i < TEST_THREADS; i++) {
		TEST_ASSERT_EQUAL_INT(0, pthread_create(&threads[i], NULL, test_lockedReader, &res[i]));
	}
	for (i = 0; i < TEST_THREADS; i++) {
		TEST_ASSERT_EQUAL_INT(0, pthread_join(threads[i], NULL));
		TEST_ASSERT_EQUAL_INT(0, res[i].errors);
		total += res[i].count;
	}

	/* every byte was read exactly once */
	TEST_ASSERT_EQUAL_INT(TEST_THREADED_SIZE, total);
}


TEST_GROUP_RUNNER(stdio_getchar_unlocked)
{
	RUN_TEST_CASE(stdio_getchar_unlocked, reads_bytes_in_order);
	RUN_TEST_CASE(stdio_getchar_unlocked, returns_byte_as_unsigned_char);
	RUN_TEST_CASE(stdio_getchar_unlocked, eof_sets_indicator);
	RUN_TEST_CASE(stdio_getchar_unlocked, eof_indicator_set_returns_eof);
	RUN_TEST_CASE(stdio_getchar_unlocked, equivalent_to_getchar);
	RUN_TEST_CASE(stdio_getchar_unlocked, marks_atime_for_update);
	RUN_TEST_CASE(stdio_getchar_unlocked, ebadf_not_open_for_reading);
	RUN_TEST_CASE(stdio_getchar_unlocked, eagain_nonblocking_no_data);
#ifndef __phoenix__
	/* blocking operations are not interruptible (POSIX deviation) */
	RUN_TEST_CASE(stdio_getchar_unlocked, eintr_read_interrupted);
#endif
	RUN_TEST_CASE(stdio_getchar_unlocked, thread_safe_within_flockfile);
}


TEST_GROUP(stdio_putchar_unlocked);

TEST_SETUP(stdio_putchar_unlocked)
{
	test_commonSetUp();
}


TEST_TEAR_DOWN(stdio_putchar_unlocked)
{
	test_commonTearDown();
}


static void test_stdoutToFile(int flags)
{
	test_redirectStdout(open(STDSTREAMS_FILE, O_WRONLY | O_CREAT | flags, TEST_FILE_MODE));
}


TEST(stdio_putchar_unlocked, writes_byte_returns_it)
{
	const char expected[] = "Ab\n";
	int ret[3];
	size_t len;

	test_stdoutToFile(O_TRUNC);
	ret[0] = putchar_unlocked('A');
	ret[1] = putchar_unlocked('b');
	ret[2] = putchar_unlocked('\n');
	test_restoreStdout();

	TEST_ASSERT_EQUAL_INT('A', ret[0]);
	TEST_ASSERT_EQUAL_INT('b', ret[1]);
	TEST_ASSERT_EQUAL_INT('\n', ret[2]);
	len = test_readFile();
	TEST_ASSERT_EQUAL_INT(sizeof(expected) - 1, len);
	TEST_ASSERT_EQUAL_MEMORY(expected, test_common.buf, len);
}


TEST(stdio_putchar_unlocked, converts_to_unsigned_char)
{
	const uint8_t expected[] = { 'A', 0xffU, 0x00U, 0x80U };
	int ret[4];
	size_t len;

	test_stdoutToFile(O_TRUNC);
	ret[0] = putchar_unlocked(0x100 + 'A');
	ret[1] = putchar_unlocked(-1);
	ret[2] = putchar_unlocked(0x300);
	ret[3] = putchar_unlocked(-0x80);
	test_restoreStdout();

	len = test_readFile();
	TEST_ASSERT_EQUAL_INT(sizeof(expected), len);
	TEST_ASSERT_EQUAL_MEMORY(expected, test_common.buf, len);

	/* the returned value is the byte written, not the argument */
	TEST_ASSERT_EQUAL_INT('A', ret[0]);
	TEST_ASSERT_EQUAL_INT(0xff, ret[1]);
	TEST_ASSERT_EQUAL_INT(0x00, ret[2]);
	TEST_ASSERT_EQUAL_INT(0x80, ret[3]);
}


TEST(stdio_putchar_unlocked, append_mode_appends)
{
	const char initial[] = "xyz";
	const char expected[] = "xyzA";
	off_t off;
	int ret;
	size_t len;

	test_writeFile(initial, sizeof(initial) - 1, O_TRUNC);
	test_stdoutToFile(O_APPEND);
	off = lseek(STDOUT_FILENO, 0, SEEK_SET);
	ret = putchar_unlocked('A');
	test_restoreStdout();

	TEST_ASSERT_EQUAL_INT(0, (int)off);
	TEST_ASSERT_EQUAL_INT('A', ret);
	len = test_readFile();
	TEST_ASSERT_EQUAL_INT(sizeof(expected) - 1, len);
	TEST_ASSERT_EQUAL_MEMORY(expected, test_common.buf, len);
}


TEST(stdio_putchar_unlocked, equivalent_to_putchar)
{
	const char expected[] = "abcd";
	size_t len;

	test_stdoutToFile(O_TRUNC);
	/* both functions operate on the same stream state */
	putchar_unlocked('a');
	putchar('b');
	putchar_unlocked('c');
	putc('d', stdout);
	test_restoreStdout();

	len = test_readFile();
	TEST_ASSERT_EQUAL_INT(sizeof(expected) - 1, len);
	TEST_ASSERT_EQUAL_MEMORY(expected, test_common.buf, len);
}


TEST(stdio_putchar_unlocked, marks_mtime_for_update)
{
	struct stat st;
	time_t now;
	int ret;

	test_writeFile("", 0, O_TRUNC);
	now = test_resetFileTimes();

	test_stdoutToFile(0);
	ret = putchar_unlocked('A');
	test_restoreStdout();

	/* the timestamp is updated at the latest by fflush() done when restoring stdout */
	TEST_ASSERT_EQUAL_INT('A', ret);
	TEST_ASSERT_EQUAL_INT(0, stat(STDSTREAMS_FILE, &st));
	TEST_ASSERT_GREATER_OR_EQUAL_INT64((int64_t)now, (int64_t)st.st_mtime);
}


static void *test_lockedWriter(void *arg)
{
	const int c = *(const int *)arg;
	int i, j;

	for (i = 0; i < TEST_CHUNKS; i++) {
		flockfile(stdout);
		for (j = 0; j < TEST_CHUNK; j++) {
			putchar_unlocked(c);
		}
		funlockfile(stdout);
		sched_yield();
	}

	return NULL;
}


TEST(stdio_putchar_unlocked, thread_safe_within_flockfile)
{
	const int chars[TEST_THREADS] = { 'A', 'B' };
	pthread_t threads[TEST_THREADS];
	int created[TEST_THREADS] = { -1, -1 };
	int joined[TEST_THREADS] = { -1, -1 };
	size_t len, i, j;

	test_stdoutToFile(O_TRUNC);
	for (i = 0; i < TEST_THREADS; i++) {
		created[i] = pthread_create(&threads[i], NULL, test_lockedWriter, (void *)&chars[i]);
	}
	for (i = 0; i < TEST_THREADS; i++) {
		if (created[i] == 0) {
			joined[i] = pthread_join(threads[i], NULL);
		}
	}
	test_restoreStdout();

	for (i = 0; i < TEST_THREADS; i++) {
		TEST_ASSERT_EQUAL_INT(0, created[i]);
		TEST_ASSERT_EQUAL_INT(0, joined[i]);
	}

	len = test_readFile();
	TEST_ASSERT_EQUAL_INT(TEST_THREADED_SIZE, len);
	/* bytes written within one locked scope are not interleaved with the other thread's ones */
	for (i = 0; i < len; i += TEST_CHUNK) {
		for (j = 1; j < TEST_CHUNK; j++) {
			TEST_ASSERT_EQUAL_HEX8(test_common.buf[i], test_common.buf[i + j]);
		}
	}
}


static int test_childEbadf(void)
{
	int fd = open(STDSTREAMS_FILE, O_RDONLY | O_CREAT, TEST_FILE_MODE);

	if ((fd < 0) || (dup2(fd, STDOUT_FILENO) != STDOUT_FILENO)) {
		return 1;
	}

	return test_childPutUntilError(EBADF);
}


TEST(stdio_putchar_unlocked, ebadf_not_open_for_writing)
{
	test_runInChild(test_childEbadf);
}


static int test_childEagain(void)
{
	int fds[2];

	if (pipe(fds) != 0) {
		return 1;
	}
	if (fcntl(fds[1], F_SETFL, fcntl(fds[1], F_GETFL) | O_NONBLOCK) != 0) {
		return 2;
	}
	if (dup2(fds[1], STDOUT_FILENO) != STDOUT_FILENO) {
		return 3;
	}

	/* nobody reads the pipe - it fills up */
	return test_childPutUntilError(EAGAIN);
}


TEST(stdio_putchar_unlocked, eagain_nonblocking_full)
{
	test_runInChild(test_childEagain);
}


static int test_childEintr(void)
{
	int fds[2];
	int flags;

	if (pipe(fds) != 0) {
		return 1;
	}

	/* fill the pipe so that the next write blocks */
	flags = fcntl(fds[1], F_GETFL);
	if (fcntl(fds[1], F_SETFL, flags | O_NONBLOCK) != 0) {
		return 2;
	}
	while (write(fds[1], "x", 1) == 1) {
	}
	if ((errno != EAGAIN) || (fcntl(fds[1], F_SETFL, flags) != 0)) {
		return 3;
	}
	if (dup2(fds[1], STDOUT_FILENO) != STDOUT_FILENO) {
		return 4;
	}

	if (test_installHandler(SIGALRM) != 0) {
		return 5;
	}
	alarm(TEST_ALARM_S);

	return test_childPutUntilError(EINTR);
}


TEST(stdio_putchar_unlocked, eintr_write_interrupted)
{
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1401 issue");
#endif
	test_runInChild(test_childEintr);
}


static int test_childEpipe(void)
{
	int fds[2];
	int ret;

	if (pipe(fds) != 0) {
		return 1;
	}
	if ((close(fds[0]) != 0) || (dup2(fds[1], STDOUT_FILENO) != STDOUT_FILENO)) {
		return 2;
	}
	if (test_installHandler(SIGPIPE) != 0) {
		return 3;
	}

	ret = test_childPutUntilError(EPIPE);
	if ((ret == 0) && (test_common.sigCount == 0)) {
		/* SIGPIPE shall also be sent */
		ret = 4;
	}

	return ret;
}


TEST(stdio_putchar_unlocked, epipe_no_reader)
{
	test_runInChild(test_childEpipe);
}


#ifdef RLIMIT_FSIZE
static int test_childEfbig(void)
{
	const struct rlimit rl = { .rlim_cur = TEST_FSIZE_LIMIT, .rlim_max = TEST_FSIZE_LIMIT };
	int fd = open(STDSTREAMS_FILE, O_WRONLY | O_CREAT | O_TRUNC, TEST_FILE_MODE);

	if ((fd < 0) || (dup2(fd, STDOUT_FILENO) != STDOUT_FILENO)) {
		return 1;
	}
	/* SIGXFSZ would terminate the process by default */
	if ((signal(SIGXFSZ, SIG_IGN) == SIG_ERR) || (setrlimit(RLIMIT_FSIZE, &rl) != 0)) {
		return 2;
	}

	return test_childPutUntilError(EFBIG);
}
#endif


TEST(stdio_putchar_unlocked, efbig_file_size_limit)
{
#ifdef RLIMIT_FSIZE
	test_runInChild(test_childEfbig);
#else
	TEST_IGNORE_MESSAGE("#1809 issue");
#endif
}


TEST_GROUP_RUNNER(stdio_putchar_unlocked)
{
	RUN_TEST_CASE(stdio_putchar_unlocked, writes_byte_returns_it);
	RUN_TEST_CASE(stdio_putchar_unlocked, converts_to_unsigned_char);
	RUN_TEST_CASE(stdio_putchar_unlocked, append_mode_appends);
	RUN_TEST_CASE(stdio_putchar_unlocked, equivalent_to_putchar);
	RUN_TEST_CASE(stdio_putchar_unlocked, marks_mtime_for_update);
	RUN_TEST_CASE(stdio_putchar_unlocked, thread_safe_within_flockfile);
	RUN_TEST_CASE(stdio_putchar_unlocked, ebadf_not_open_for_writing);
	RUN_TEST_CASE(stdio_putchar_unlocked, eagain_nonblocking_full);
#ifndef __phoenix__
	/* blocking operations are not interruptible (POSIX deviation) */
	RUN_TEST_CASE(stdio_putchar_unlocked, eintr_write_interrupted);
#endif
	RUN_TEST_CASE(stdio_putchar_unlocked, epipe_no_reader);
#ifndef __phoenix__
	/* the file size limit of a process isn't supported (POSIX deviation) */
	RUN_TEST_CASE(stdio_putchar_unlocked, efbig_file_size_limit);
#endif
}


TEST_GROUP(stdio_gets);

TEST_SETUP(stdio_gets)
{
	test_commonSetUp();
	memset(test_common.line, TEST_FILL, sizeof(test_common.line));
}


TEST_TEAR_DOWN(stdio_gets)
{
	test_commonTearDown();
}


TEST(stdio_gets, reads_line_discards_newline)
{
	const char data[] = "abc\nde\n";

	test_stdinFromData(data, sizeof(data) - 1);

	/* the null byte is placed right after the last byte read, the newline is not stored */
	TEST_ASSERT_EQUAL_PTR(test_common.line, gets(test_common.line));
	TEST_ASSERT_EQUAL_STRING("abc", test_common.line);
	TEST_ASSERT_EQUAL_PTR(test_common.line, gets(test_common.line));
	TEST_ASSERT_EQUAL_STRING("de", test_common.line);
	TEST_ASSERT_EQUAL_INT(0, feof(stdin));
}


TEST(stdio_gets, empty_line)
{
	const char data[] = "\nx\n";

	test_stdinFromData(data, sizeof(data) - 1);

	TEST_ASSERT_EQUAL_PTR(test_common.line, gets(test_common.line));
	TEST_ASSERT_EQUAL_STRING("", test_common.line);
	TEST_ASSERT_EQUAL_PTR(test_common.line, gets(test_common.line));
	TEST_ASSERT_EQUAL_STRING("x", test_common.line);
}


TEST(stdio_gets, reads_until_eof_without_newline)
{
	const char data[] = "abc\nxyz";

	test_stdinFromData(data, sizeof(data) - 1);

	TEST_ASSERT_EQUAL_PTR(test_common.line, gets(test_common.line));
	TEST_ASSERT_EQUAL_STRING("abc", test_common.line);

	/* the last line is terminated by end-of-file - bytes were read, so it's a success */
	TEST_ASSERT_EQUAL_PTR(test_common.line, gets(test_common.line));
	TEST_ASSERT_EQUAL_STRING("xyz", test_common.line);
	TEST_ASSERT_EQUAL_INT(0, ferror(stdin));

	TEST_ASSERT_NULL(gets(test_common.line));
	TEST_ASSERT_NOT_EQUAL_INT(0, feof(stdin));
}


TEST(stdio_gets, reads_any_byte_values)
{
	const char data[] = "\x01\x7f\x80\xff \t\r\n";
	const char expected[] = "\x01\x7f\x80\xff \t\r";

	test_stdinFromData(data, sizeof(data) - 1);

	TEST_ASSERT_EQUAL_PTR(test_common.line, gets(test_common.line));
	TEST_ASSERT_EQUAL_MEMORY(expected, test_common.line, sizeof(expected));
}


TEST(stdio_gets, reads_long_line)
{
	size_t i;

	/* longer than BUFSIZ on some implementations - the line spans multiple stream buffer refills */
	for (i = 0; i < TEST_LINE_MAX - 1; i++) {
		test_common.buf[i] = (uint8_t)('a' + (i % 26));
	}
	test_common.buf[TEST_LINE_MAX - 1] = '\n';
	test_stdinFromData(test_common.buf, TEST_LINE_MAX);

	TEST_ASSERT_EQUAL_PTR(test_common.line, gets(test_common.line));
	TEST_ASSERT_EQUAL_INT(TEST_LINE_MAX - 1, strlen(test_common.line));
	TEST_ASSERT_EQUAL_MEMORY(test_common.buf, test_common.line, TEST_LINE_MAX - 1);
}


TEST(stdio_gets, eof_returns_null_sets_indicator)
{
	test_stdinFromData("", 0);

	TEST_ASSERT_NULL(gets(test_common.line));
	TEST_ASSERT_NOT_EQUAL_INT(0, feof(stdin));
	TEST_ASSERT_EQUAL_INT(0, ferror(stdin));
}


TEST(stdio_gets, eof_indicator_set_returns_null)
{
	const char first[] = "a\n";
	const char appended[] = "b\n";

	test_stdinFromData(first, sizeof(first) - 1);

	TEST_ASSERT_EQUAL_PTR(test_common.line, gets(test_common.line));
	TEST_ASSERT_NULL(gets(test_common.line));
	TEST_ASSERT_NOT_EQUAL_INT(0, feof(stdin));

	/* a next line is now present, but the end-of-file indicator is still set */
	test_writeFile(appended, sizeof(appended) - 1, O_APPEND);
	TEST_ASSERT_NULL(gets(test_common.line));
	TEST_ASSERT_NOT_EQUAL_INT(0, feof(stdin));

	clearerr(stdin);
	TEST_ASSERT_EQUAL_PTR(test_common.line, gets(test_common.line));
	TEST_ASSERT_EQUAL_STRING("b", test_common.line);
}


TEST(stdio_gets, marks_atime_for_update)
{
	struct stat st;
	time_t now;

	test_stdinFromData("a\n", 2);
	now = test_resetFileTimes();

	TEST_ASSERT_EQUAL_PTR(test_common.line, gets(test_common.line));
	TEST_ASSERT_EQUAL_INT(0, fstat(STDIN_FILENO, &st));
	TEST_ASSERT_GREATER_OR_EQUAL_INT64((int64_t)now, (int64_t)st.st_atime);
}


TEST(stdio_gets, ebadf_not_open_for_reading)
{
	test_writeFile("abc\n", 4, O_TRUNC);
	test_redirectStdin(open(STDSTREAMS_FILE, O_WRONLY));

	errno = 0;
	TEST_ASSERT_NULL(gets(test_common.line));
	TEST_ASSERT_NOT_EQUAL_INT(0, ferror(stdin));
	TEST_ASSERT_EQUAL_INT(0, feof(stdin));
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


TEST(stdio_gets, eagain_nonblocking_no_data)
{
	int fds[2];

	TEST_ASSERT_EQUAL_INT(0, pipe(fds));
	test_common.pipeWr = fds[1];
	TEST_ASSERT_EQUAL_INT(0, fcntl(fds[0], F_SETFL, fcntl(fds[0], F_GETFL) | O_NONBLOCK));
	test_redirectStdin(fds[0]);

	errno = 0;
	TEST_ASSERT_NULL(gets(test_common.line));
	TEST_ASSERT_NOT_EQUAL_INT(0, ferror(stdin));
	TEST_ASSERT_EQUAL_INT(0, feof(stdin));
	TEST_ASSERT_EQUAL_INT(EAGAIN, errno);
}


TEST(stdio_gets, eintr_read_interrupted)
{
	int fds[2];

#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1401 issue");
#endif

	TEST_ASSERT_EQUAL_INT(0, pipe(fds));
	test_common.pipeWr = fds[1];
	test_redirectStdin(fds[0]);

	TEST_ASSERT_EQUAL_INT(0, test_installHandler(SIGALRM));
	test_common.sigSet = true;
	alarm(TEST_ALARM_S);

	/* the pipe is empty and its write end is open - the read blocks until SIGALRM arrives */
	errno = 0;
	TEST_ASSERT_NULL(gets(test_common.line));
	TEST_ASSERT_EQUAL_INT(1, test_common.sigCount);
	TEST_ASSERT_NOT_EQUAL_INT(0, ferror(stdin));
	TEST_ASSERT_EQUAL_INT(0, feof(stdin));
	TEST_ASSERT_EQUAL_INT(EINTR, errno);
}


TEST_GROUP_RUNNER(stdio_gets)
{
	RUN_TEST_CASE(stdio_gets, reads_line_discards_newline);
	RUN_TEST_CASE(stdio_gets, empty_line);
	RUN_TEST_CASE(stdio_gets, reads_until_eof_without_newline);
	RUN_TEST_CASE(stdio_gets, reads_any_byte_values);
	RUN_TEST_CASE(stdio_gets, reads_long_line);
	RUN_TEST_CASE(stdio_gets, eof_returns_null_sets_indicator);
	RUN_TEST_CASE(stdio_gets, eof_indicator_set_returns_null);
	RUN_TEST_CASE(stdio_gets, marks_atime_for_update);
	RUN_TEST_CASE(stdio_gets, ebadf_not_open_for_reading);
	RUN_TEST_CASE(stdio_gets, eagain_nonblocking_no_data);
#ifndef __phoenix__
	/* blocking operations are not interruptible (POSIX deviation) */
	RUN_TEST_CASE(stdio_gets, eintr_read_interrupted);
#endif
}
