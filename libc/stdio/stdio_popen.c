/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - stdio.h
 * TESTED:
 *    - popen()
 *    - pclose()
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
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "common.h"
#include "unity_fixture.h"

/* the shell path is unspecified by POSIX, it's the one used by the tested implementations */
#define SH_PATH         "/bin/sh"
#define POPEN_FILE      "test_stdio_popen"
#define POPEN_TEXT      "popen text"
#define POPEN_VAR       "TEST_STDIO_POPEN_VAR"
#define POPEN_VAR_VALUE "popen variable value"
#define POPEN_EXIT_CODE 3
#define POPEN_EXIT_CMD  "exit 3" /* exits with POPEN_EXIT_CODE */
#define POPEN_TRUE_CMD  "exit 0"
#define CHILD_EXIT_CODE 5
#define TEST_LINE_MAX   64
#define TEST_CHUNK      512
/* more than a pipe holds, so the parent and the child have to run alternately */
#define TEST_BIG_SIZE   (256 * 1024)
#define TEST_TIMEOUT_S  2


static struct {
	pthread_mutex_t lock;
	pthread_cond_t cond;
	bool closed;
	int status;
	char line[TEST_LINE_MAX];
	uint8_t chunk[TEST_CHUNK];
} test_common;


static uint8_t test_patternByte(size_t offs)
{
	return (uint8_t)((offs * 7U) % 251U);
}


static void test_fillChunk(size_t offs, size_t len)
{
	size_t i;

	for (i = 0; i < len; i++) {
		test_common.chunk[i] = test_patternByte(offs + i);
	}
}


/* writes TEST_BIG_SIZE bytes of the pattern to the stream, returns 0 on success */
static int test_writePattern(FILE *fp)
{
	size_t offs;

	for (offs = 0; offs < TEST_BIG_SIZE; offs += sizeof(test_common.chunk)) {
		test_fillChunk(offs, sizeof(test_common.chunk));
		if (fwrite(test_common.chunk, 1, sizeof(test_common.chunk), fp) != sizeof(test_common.chunk)) {
			return -1;
		}
	}

	return 0;
}


/* reads the stream to the end, returns the number of bytes matching the pattern before the first mismatch */
static size_t test_readPattern(FILE *fp)
{
	size_t offs = 0, n, i;

	while ((n = fread(test_common.chunk, 1, sizeof(test_common.chunk), fp)) > 0) {
		for (i = 0; i < n; i++) {
			if (test_common.chunk[i] != test_patternByte(offs + i)) {
				return offs + i;
			}
		}
		offs += n;
	}

	return offs;
}


TEST_GROUP(stdio_popen);


TEST_SETUP(stdio_popen)
{
	unlink(POPEN_FILE);
}


TEST_TEAR_DOWN(stdio_popen)
{
	unlink(POPEN_FILE);
	unsetenv(POPEN_VAR);
}


TEST(stdio_popen, read_mode_child_stdout_to_stream)
{
	FILE *fp = popen("echo " POPEN_TEXT, "r");

	TEST_ASSERT_NOT_NULL(fp);
	/* fileno(stream) is the readable end of the pipe */
	TEST_ASSERT_EQUAL_INT(O_RDONLY, fcntl(fileno(fp), F_GETFL) & O_ACCMODE);

	TEST_ASSERT_NOT_NULL(fgets(test_common.line, sizeof(test_common.line), fp));
	TEST_ASSERT_EQUAL_STRING(POPEN_TEXT "\n", test_common.line);
	TEST_ASSERT_EQUAL_INT(EOF, fgetc(fp));
	TEST_ASSERT_TRUE(feof(fp));

	TEST_ASSERT_EQUAL_INT(0, pclose(fp));
}


TEST(stdio_popen, write_mode_stream_to_child_stdin)
{
	FILE *fp = popen("cat > " POPEN_FILE, "w");

	TEST_ASSERT_NOT_NULL(fp);
	/* fileno(stream) is the writable end of the pipe */
	TEST_ASSERT_EQUAL_INT(O_WRONLY, fcntl(fileno(fp), F_GETFL) & O_ACCMODE);

	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fputs(POPEN_TEXT, fp));
	TEST_ASSERT_EQUAL_INT(0, pclose(fp));

	check_file_contents(POPEN_TEXT, POPEN_FILE);
}


TEST(stdio_popen, command_run_by_sh)
{
	/* variable assignment and expansion are done by the shell */
	FILE *fp = popen("v='" POPEN_TEXT "'; echo \"$v\"", "r");

	TEST_ASSERT_NOT_NULL(fp);
	TEST_ASSERT_NOT_NULL(fgets(test_common.line, sizeof(test_common.line), fp));
	TEST_ASSERT_EQUAL_STRING(POPEN_TEXT "\n", test_common.line);
	TEST_ASSERT_EQUAL_INT(0, pclose(fp));
}


TEST(stdio_popen, empty_command)
{
	FILE *fp = popen("", "r");

	TEST_ASSERT_NOT_NULL(fp);
	TEST_ASSERT_EQUAL_INT(EOF, fgetc(fp));
	TEST_ASSERT_TRUE(feof(fp));
	TEST_ASSERT_EQUAL_INT(0, pclose(fp));
}


TEST(stdio_popen, child_inherits_environment)
{
	FILE *fp;

	TEST_ASSERT_EQUAL_INT(0, setenv(POPEN_VAR, POPEN_VAR_VALUE, 1));

	fp = popen("echo \"$" POPEN_VAR "\"", "r");
	TEST_ASSERT_NOT_NULL(fp);
	TEST_ASSERT_NOT_NULL(fgets(test_common.line, sizeof(test_common.line), fp));
	TEST_ASSERT_EQUAL_STRING(POPEN_VAR_VALUE "\n", test_common.line);
	TEST_ASSERT_EQUAL_INT(0, pclose(fp));
}


TEST(stdio_popen, read_mode_more_than_pipe_capacity)
{
	FILE *filep, *fp;

	filep = fopen(POPEN_FILE, "w");
	TEST_ASSERT_NOT_NULL(filep);
	TEST_ASSERT_EQUAL_INT(0, test_writePattern(filep));
	TEST_ASSERT_EQUAL_INT(0, fclose(filep));

	/* the child blocks on the full pipe until the parent reads */
	fp = popen("cat " POPEN_FILE, "r");
	TEST_ASSERT_NOT_NULL(fp);
	TEST_ASSERT_EQUAL_size_t(TEST_BIG_SIZE, test_readPattern(fp));
	TEST_ASSERT_EQUAL_INT(0, pclose(fp));
}


TEST(stdio_popen, write_mode_more_than_pipe_capacity)
{
	FILE *filep, *fp;

	/* the parent blocks on the full pipe until the child reads */
	fp = popen("cat > " POPEN_FILE, "w");
	TEST_ASSERT_NOT_NULL(fp);
	TEST_ASSERT_EQUAL_INT(0, test_writePattern(fp));
	TEST_ASSERT_EQUAL_INT(0, pclose(fp));

	filep = fopen(POPEN_FILE, "r");
	TEST_ASSERT_NOT_NULL(filep);
	TEST_ASSERT_EQUAL_size_t(TEST_BIG_SIZE, test_readPattern(filep));
	TEST_ASSERT_EQUAL_INT(0, fclose(filep));
}


static void *test_pcloseThread(void *arg)
{
	const int status = pclose((FILE *)arg);

	pthread_mutex_lock(&test_common.lock);
	test_common.status = status;
	test_common.closed = true;
	pthread_cond_signal(&test_common.cond);
	pthread_mutex_unlock(&test_common.lock);

	return NULL;
}


TEST(stdio_popen, previous_streams_closed_in_child)
{
	FILE *first, *second;
	pthread_t tid;
	struct timespec deadline;
	bool closedInTime;
	int ret = 0;

	test_common.closed = false;
	test_common.status = -1;
	TEST_ASSERT_EQUAL_INT(0, pthread_mutex_init(&test_common.lock, NULL));
	TEST_ASSERT_EQUAL_INT(0, pthread_cond_init(&test_common.cond, NULL));

	/* both children wait for the end of their input */
	first = popen("cat > /dev/null", "w");
	TEST_ASSERT_NOT_NULL(first);
	second = popen("cat > /dev/null", "w");
	TEST_ASSERT_NOT_NULL(second);

	/*
	 * Closing the first stream ends the input of its child, unless the second child keeps
	 * the writable end of the first pipe - then the first pclose() waits until the second
	 * child terminates, so it's done in a thread to avoid blocking the test.
	 */
	TEST_ASSERT_EQUAL_INT(0, clock_gettime(CLOCK_REALTIME, &deadline));
	deadline.tv_sec += TEST_TIMEOUT_S;
	TEST_ASSERT_EQUAL_INT(0, pthread_create(&tid, NULL, test_pcloseThread, first));

	pthread_mutex_lock(&test_common.lock);
	while (!test_common.closed && (ret == 0)) {
		ret = pthread_cond_timedwait(&test_common.cond, &test_common.lock, &deadline);
	}
	closedInTime = test_common.closed;
	pthread_mutex_unlock(&test_common.lock);

	/* unblocks the first pclose() also if the second child keeps the first pipe */
	TEST_ASSERT_EQUAL_INT(0, pclose(second));
	TEST_ASSERT_EQUAL_INT(0, pthread_join(tid, NULL));
	pthread_cond_destroy(&test_common.cond);
	pthread_mutex_destroy(&test_common.lock);

	TEST_ASSERT_EQUAL_INT(0, test_common.status);
#ifdef __phoenix__
	(void)closedInTime;
	TEST_IGNORE_MESSAGE("#1814 issue");
#else
	TEST_ASSERT_TRUE_MESSAGE(closedInTime, "stream of the previous popen() is open in the new child");
#endif
}


TEST_GROUP_RUNNER(stdio_popen)
{
	if (access(SH_PATH, X_OK) != 0) {
		/* commands can be run only if a shell is available */
		return;
	}
	RUN_TEST_CASE(stdio_popen, read_mode_child_stdout_to_stream);
	RUN_TEST_CASE(stdio_popen, write_mode_stream_to_child_stdin);
	RUN_TEST_CASE(stdio_popen, command_run_by_sh);
	RUN_TEST_CASE(stdio_popen, empty_command);
	RUN_TEST_CASE(stdio_popen, child_inherits_environment);
	RUN_TEST_CASE(stdio_popen, read_mode_more_than_pipe_capacity);
	RUN_TEST_CASE(stdio_popen, write_mode_more_than_pipe_capacity);
	RUN_TEST_CASE(stdio_popen, previous_streams_closed_in_child);
}


TEST_GROUP(stdio_pclose);


TEST_SETUP(stdio_pclose)
{
	unlink(POPEN_FILE);
}


TEST_TEAR_DOWN(stdio_pclose)
{
	unlink(POPEN_FILE);
}


TEST(stdio_pclose, returns_exit_status)
{
	FILE *fp = popen(POPEN_EXIT_CMD, "r");
	int status;

	TEST_ASSERT_NOT_NULL(fp);
	status = pclose(fp);
	TEST_ASSERT_TRUE(WIFEXITED(status));
	TEST_ASSERT_EQUAL_INT(POPEN_EXIT_CODE, WEXITSTATUS(status));
}


TEST(stdio_pclose, returns_signal_status)
{
	FILE *fp = popen("kill -KILL $$", "r");
	int status;

	TEST_ASSERT_NOT_NULL(fp);
	status = pclose(fp);
	TEST_ASSERT_TRUE(WIFSIGNALED(status));
	TEST_ASSERT_EQUAL_INT(SIGKILL, WTERMSIG(status));
}


TEST(stdio_pclose, waits_for_child_termination)
{
	/* the file is created by the child a second after the stream is closed */
	FILE *fp = popen("sleep 1; echo " POPEN_TEXT " > " POPEN_FILE, "r");

	TEST_ASSERT_NOT_NULL(fp);
	TEST_ASSERT_EQUAL_INT(0, pclose(fp));

	check_file_contents(POPEN_TEXT "\n", POPEN_FILE);
}


TEST(stdio_pclose, echild_after_wait)
{
	FILE *fp = popen(POPEN_TRUE_CMD, "r");

	TEST_ASSERT_NOT_NULL(fp);
	TEST_ASSERT_GREATER_THAN_INT(0, wait(NULL));

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, pclose(fp));
	TEST_ASSERT_EQUAL_INT(ECHILD, errno);
}


TEST(stdio_pclose, echild_after_waitpid_any)
{
	FILE *fp = popen(POPEN_TRUE_CMD, "r");

	TEST_ASSERT_NOT_NULL(fp);
	TEST_ASSERT_GREATER_THAN_INT(0, waitpid(-1, NULL, 0));

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, pclose(fp));
	TEST_ASSERT_EQUAL_INT(ECHILD, errno);
}


TEST(stdio_pclose, other_children_status_unaffected)
{
	FILE *fp;
	pid_t pid;
	int status;

	/* the parent resumes after the child has terminated */
	pid = vfork();
	if (pid == 0) {
		_exit(CHILD_EXIT_CODE);
	}
	TEST_ASSERT_GREATER_THAN_INT(0, pid);

	fp = popen(POPEN_TRUE_CMD, "r");
	TEST_ASSERT_NOT_NULL(fp);
	TEST_ASSERT_EQUAL_INT(0, pclose(fp));

	TEST_ASSERT_EQUAL_INT(pid, waitpid(pid, &status, 0));
	TEST_ASSERT_TRUE(WIFEXITED(status));
	TEST_ASSERT_EQUAL_INT(CHILD_EXIT_CODE, WEXITSTATUS(status));
}


TEST_GROUP_RUNNER(stdio_pclose)
{
	if (access(SH_PATH, X_OK) != 0) {
		/* commands can be run only if a shell is available */
		return;
	}
	RUN_TEST_CASE(stdio_pclose, returns_exit_status);
	RUN_TEST_CASE(stdio_pclose, returns_signal_status);
	RUN_TEST_CASE(stdio_pclose, waits_for_child_termination);
	RUN_TEST_CASE(stdio_pclose, echild_after_wait);
	RUN_TEST_CASE(stdio_pclose, echild_after_waitpid_any);
	RUN_TEST_CASE(stdio_pclose, other_children_status_unaffected);
}
