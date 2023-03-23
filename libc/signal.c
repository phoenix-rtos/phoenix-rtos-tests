/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - signal.h
 *
 * TESTED:
 *    - psignal()
 *
 * Copyright 2023 Phoenix Systems
 * Author: Mateusz Bloch
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <unity_fixture.h>

/* Typical error message does not exceed ~60 characters, that's why we expect a maximum value a little bit bigger */
#define MAX_LEN_STRING 100
#define STDERR_PATH    "stderr.log"
#define SIMPLE_MSG     "Simple Message"

static FILE *err_file;
static int stderr_fd;

extern const int signal_codes[];

extern const unsigned int signal_codes_len;

TEST_GROUP(signal_psignal);

TEST_SETUP(signal_psignal)
{
	/* stderr duplication in pre-test state */
	err_file = fopen(STDERR_PATH, "a+");
	stderr_fd = dup(STDERR_FILENO);
}


TEST_TEAR_DOWN(signal_psignal)
{
	fflush(err_file);
	/* redirect back stderr */
	dup2(stderr_fd, STDERR_FILENO);
	close(stderr_fd);
	fclose(err_file);
	remove(STDERR_PATH);
}


TEST(signal_psignal, basic)
{
/* Disabled because of #695 issue: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/695 */
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#695 issue");
#else
	/* Since these function shall not return a value, we check only errno */
	char buff[MAX_LEN_STRING];

	for (int i = 0; i < signal_codes_len; i++) {
		rewind(err_file);
		memset(buff, 0, sizeof(buff));

		/* we redirect stderr to another file to read it */
		int fd = open(STDERR_PATH, O_WRONLY, 0666);
		dup2(fd, STDERR_FILENO);

		errno = 0;
		psignal(signal_codes[i], "Lorem Ipsum");
		TEST_ASSERT_EQUAL_INT(0, errno);

		fflush(stderr);

		TEST_ASSERT_NOT_NULL(fgets(buff, MAX_LEN_STRING, err_file));
		TEST_ASSERT_NOT_NULL(strstr(buff, "Lorem Ipsum"));
		fflush(err_file);
		close(fd);
	}
#endif
}


TEST(signal_psignal, ascii_string)
{
/* Disabled because of #695 issue: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/695 */
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#695 issue");
#else
	char buff[MAX_LEN_STRING];

	char ascii_str[MAX_LEN_STRING] = { 0 };

	/*
	 * In ASCII table from number 33 starts printable characters
	 * in this case we want to dodge terminating ASCII signs
	 */
	const char first_char = '!';
	for (char i = first_char; i < 127; i++) {
		ascii_str[i - first_char] = i;
	}

	for (int i = 0; i < signal_codes_len; i++) {
		rewind(err_file);
		memset(buff, 0, sizeof(buff));

		int fd = open(STDERR_PATH, O_WRONLY, 0666);
		dup2(fd, STDERR_FILENO);

		errno = 0;
		psignal(signal_codes[i], ascii_str);
		TEST_ASSERT_EQUAL_INT(0, errno);

		fflush(stderr);

		TEST_ASSERT_NOT_NULL(fgets(buff, MAX_LEN_STRING, err_file));
		TEST_ASSERT_NOT_NULL(strstr(buff, ascii_str));
		fflush(err_file);
		close(fd);
	}
#endif
}


TEST(signal_psignal, psignal_strsignal)
{
/* Disabled because of #695 issue: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/695 */
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#695 issue");
#else
	char buff[MAX_LEN_STRING];

	for (int i = 0; i < signal_codes_len; i++) {
		char *signal_output = strsignal(signal_codes[i]);

		rewind(err_file);
		memset(buff, 0, sizeof(buff));

		int fd = open(STDERR_PATH, O_WRONLY, 0666);
		dup2(fd, STDERR_FILENO);

		errno = 0;
		psignal(signal_codes[i], SIMPLE_MSG);
		TEST_ASSERT_EQUAL_INT(0, errno);

		fflush(stderr);

		TEST_ASSERT_NOT_NULL(fgets(buff, MAX_LEN_STRING, err_file));
		TEST_ASSERT_NOT_NULL(strstr(buff, SIMPLE_MSG));
		TEST_ASSERT_NOT_NULL(strstr(buff, signal_output));
		fflush(err_file);
		close(fd);
	}
#endif
}


TEST(signal_psignal, psignal_strsignal_null)
{
/* Disabled because of #695 issue: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/695 */
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#695 issue");
#else
	char buff[MAX_LEN_STRING];

	for (int i = 0; i < signal_codes_len; i++) {
		char *signal_output = strsignal(signal_codes[i]);

		rewind(err_file);
		memset(buff, 0, sizeof(buff));

		int fd = open(STDERR_PATH, O_WRONLY, 0666);
		dup2(fd, STDERR_FILENO);

		errno = 0;
		psignal(signal_codes[i], NULL);
		TEST_ASSERT_EQUAL_INT(0, errno);

		fflush(stderr);

		TEST_ASSERT_NOT_NULL(fgets(buff, MAX_LEN_STRING, err_file));
		TEST_ASSERT_NOT_NULL(strstr(buff, signal_output));
		fflush(err_file);
		close(fd);
	}
#endif
}


TEST_GROUP_RUNNER(signal_psignal)
{
	RUN_TEST_CASE(signal_psignal, basic);
	RUN_TEST_CASE(signal_psignal, ascii_string);
	RUN_TEST_CASE(signal_psignal, psignal_strsignal);
	RUN_TEST_CASE(signal_psignal, psignal_strsignal_null);
}
