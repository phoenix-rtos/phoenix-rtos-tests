/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - string.h
 *
 * TESTED:
 *    - strerror()
 *    - strerror_r()
 *    - strsignal()
 *
 * Copyright 2023-2024 Phoenix Systems
 * Author: Mateusz Bloch, Arkadiusz Kozlowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <unity_fixture.h>

#include "testdata.h"

/* Typical error message does not exceed ~60 characters, that's why we expect a maximum value a little bit bigger */
#define MAX_LEN_STRING 100

/* ECANCELED, EDQUOT, EIDRM, EMULTIHOP, ENOLINK, ENOSR, ENOSTR, EOWNERDEAD, ESTALE missing in Phoenix-RTOS
 *  #689 issue: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/689
 */
static const int error_codes[] = { E2BIG, EACCES, EADDRINUSE, EADDRNOTAVAIL, EAFNOSUPPORT, EAGAIN, EALREADY, EBADF,
	EBADMSG, EBUSY, ECHILD, ECONNABORTED, ECONNREFUSED, ECONNRESET, EDEADLK, EDESTADDRREQ, EDOM, EEXIST, EFAULT,
	EFBIG, EHOSTUNREACH, EILSEQ, EINPROGRESS, EINTR, EINVAL, EIO, EISCONN, EISDIR, ELOOP, EMFILE, EMLINK, EMSGSIZE,
	ENAMETOOLONG, ENETDOWN, ENETRESET, ENETUNREACH, ENFILE, ENOBUFS, ENODATA, ENODEV, ENOENT, ENOEXEC, ENOLCK, ENOMEM,
	ENOMSG, ENOPROTOOPT, ENOSPC, ENOSYS, ENOTCONN, ENOTDIR, ENOTEMPTY, ENOTRECOVERABLE, ENOTSOCK, ENOTSUP, ENOTTY,
	ENXIO, EOPNOTSUPP, EOVERFLOW, EPERM, EPIPE, EPROTO, EPROTONOSUPPORT, EPROTOTYPE, ERANGE, EROFS, ESPIPE, ESRCH,
	ETIME, ETIMEDOUT, ETXTBSY, EWOULDBLOCK, EXDEV, ENOTBLK };

const int signal_codes[] = { SIGABRT, SIGALRM, SIGBUS, SIGCHLD, SIGCONT, SIGFPE, SIGHUP, SIGILL, SIGINT,
	SIGKILL, SIGPIPE, SIGQUIT, SIGSEGV, SIGSTOP, SIGTERM, SIGTSTP, SIGTTIN, SIGTTOU, SIGUSR1, SIGUSR2, SIGPROF,
	SIGSYS, SIGTRAP, SIGURG, SIGVTALRM, SIGXCPU, SIGXFSZ };

static const unsigned int error_codes_len = sizeof(error_codes) / sizeof(error_codes[0]);

const unsigned int signal_codes_len = sizeof(signal_codes) / sizeof(signal_codes[0]);

const char *filename = "error.txt";

#define SECURE_CHECK(expr, ...) \
	do { \
		void *to_free[] = { __VA_ARGS__ }; \
		int num_to_free = sizeof(to_free) / sizeof(to_free[0]); \
		int expr_res = (expr); \
		for (int idx = 0; idx < num_to_free; ++idx) { \
			if (to_free[idx] != NULL) { \
				free(to_free[idx]); \
			} \
		} \
		if (!(expr_res)) { \
			TEST_FAIL_MESSAGE(#expr " has evaluated to False"); \
		} \
	} while (0);


char *perrorToFile(const char *msg)
{
	char failedFuncs[64] = "";
	int errnoBefore = errno;

	FILE *file = fopen(filename, "w+");

	TEST_ASSERT_NOT_NULL(file);

	if (dup2(fileno(file), STDERR_FILENO) == -1) {
		strcat(failedFuncs, "dup2 ");
	}

	errno = errnoBefore;
	perror(msg);

	if (fseek(file, 0, SEEK_END) != 0) {
		strcat(failedFuncs, "fseek ");
	}

	long fileSize = ftell(file);
	rewind(file);

	char *buffer = (char *)malloc(fileSize + 1);

	if (buffer == NULL) {
		strcat(failedFuncs, "malloc ");
		fclose(file);
		char *msg = strcat(failedFuncs, "has failed");
		TEST_FAIL_MESSAGE(msg);
	}

	if (fread(buffer, 1, fileSize, file) != fileSize) {
		strcat(failedFuncs, "fread");
	}
	buffer[fileSize] = '\0';

	fclose(file);
	if (strcmp("", failedFuncs)) {
		free(buffer);
		char *msg = strcat(failedFuncs, "has failed");
		TEST_FAIL_MESSAGE(msg);
	}

	return buffer;
}


TEST_GROUP(string_errsign);


TEST_SETUP(string_errsign)
{
}


TEST_TEAR_DOWN(string_errsign)
{
}


TEST(string_errsign, strerror_basic)
{
	for (int i = 0; i < error_codes_len; i++) {
		errno = 0;
		TEST_ASSERT_NOT_NULL(strerror(error_codes[i]));

		if (i != 0) {
			TEST_ASSERT_NOT_EQUAL_INT(0, strcmp(strerror(error_codes[i - 1]), strerror(error_codes[i])));
		}

		TEST_ASSERT_EQUAL_INT(0, errno);
	}
}


TEST(string_errsign, strerror_zero)
{
	errno = 0;
	TEST_ASSERT_NOT_NULL(strerror(0));
	TEST_ASSERT_EQUAL_INT(0, errno);
}


TEST(string_errsign, strerror_r_basic)
{
	char prev_buf[MAX_LEN_STRING] = { 0 };
	char buf[MAX_LEN_STRING] = { 0 };
	for (int i = 0; i < error_codes_len; i++) {

		errno = 0;
		TEST_ASSERT_EQUAL_INT(0, strerror_r(error_codes[i], buf, sizeof(buf)));
		TEST_ASSERT_NOT_EQUAL_INT(0, strcmp(buf, prev_buf));
		TEST_ASSERT_EQUAL_INT(0, errno);

		TEST_ASSERT_NOT_NULL(strcpy(prev_buf, buf));
	}
}


TEST(string_errsign, strerror_r_zero)
{
	char buf[MAX_LEN_STRING];
	errno = 0;
	TEST_ASSERT_EQUAL_INT(0, strerror_r(0, buf, sizeof(buf)));
	TEST_ASSERT_EQUAL_INT(0, errno);
}


TEST(string_errsign, strerror_r_invaild)
{
	const int values[] = { INT_MIN, INT_MIN / 2, INT_MIN / 4, -1024, -256, 256, 1024, INT_MAX / 4, INT_MAX / 2, INT_MAX };
	for (int i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {
		char buf[MAX_LEN_STRING] = { 0 };
		errno = 0;
		TEST_ASSERT_EQUAL_INT(EINVAL, strerror_r(values[i], buf, sizeof(buf)));
		TEST_ASSERT_EQUAL_INT(0, errno);
	}
}


TEST(string_errsign, strerror_r_overflow)
{
	char buf[1];
	char buf2[MAX_LEN_STRING];
	for (int i = 0; i < error_codes_len; i++) {
		errno = 0;
		TEST_ASSERT_EQUAL_INT(ERANGE, strerror_r(error_codes[i], buf, sizeof(buf)));
		TEST_ASSERT_EQUAL_INT(ERANGE, strerror_r(error_codes[i], buf2, 2));
		TEST_ASSERT_EQUAL_INT(0, errno);
	}
}


TEST(string_errsign, strsignal_basic)
{
	for (int i = 0; i < signal_codes_len; i++) {
		errno = 0;
		TEST_ASSERT_NOT_NULL(strsignal(signal_codes[i]));

		if (i != 0) {
			TEST_ASSERT_NOT_EQUAL_INT(0, strcmp(strsignal(signal_codes[i - 1]), strsignal(signal_codes[i])));
		}

		TEST_ASSERT_EQUAL_INT(0, errno);
	}
}


TEST(string_errsign, strsignal_real_time)
{
/* Disabled because of #687 issue: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/687 */
#if defined(SIGRTMIN) && defined(SIGRTMAX)
	for (int i = SIGRTMIN; i <= SIGRTMAX; i++) {
		errno = 0;
		TEST_ASSERT_NOT_NULL(strsignal(i));
		TEST_ASSERT_EQUAL_INT(0, errno);
	}
#endif
}


TEST_GROUP_RUNNER(string_errsign)
{
	RUN_TEST_CASE(string_errsign, strerror_basic);
	RUN_TEST_CASE(string_errsign, strerror_zero);

	RUN_TEST_CASE(string_errsign, strerror_r_basic);
	RUN_TEST_CASE(string_errsign, strerror_r_zero);
	RUN_TEST_CASE(string_errsign, strerror_r_invaild);
	RUN_TEST_CASE(string_errsign, strerror_r_overflow);

	RUN_TEST_CASE(string_errsign, strsignal_basic);
	RUN_TEST_CASE(string_errsign, strsignal_real_time);
}


TEST_GROUP(string_perror);


TEST_SETUP(string_perror)
{
}


TEST_TEAR_DOWN(string_perror)
{
	remove(filename);
}

TEST(string_perror, perror_basic)
{
	char msg[] = "Some error message";
	char *res;
	char exp[100];

	sprintf(exp, "Some error message: %s\n", strerror(0));
	res = perrorToFile(msg);
	SECURE_CHECK((strcmp(exp, res) == 0), res);
}


TEST(string_perror, perror_empty_message)
{
	TEST_IGNORE_MESSAGE("#929 issue");
	char *res;
	char exp[50];

	errno = 31;
	sprintf(exp, "%s\n", strerror(errno));
	SECURE_CHECK(strcmp(exp, res) == 0, res);
}


TEST(string_perror, perror_every_ascii)
{
	char *msg = testdata_createCharStr(257);
	char exp[356];
	char *res;

	errno = 8;
	res = perrorToFile(msg);
	sprintf(exp, "%s: %s\n", msg, strerror(errno));
	SECURE_CHECK(strcmp(exp, res) == 0, res, msg);
}


TEST(string_perror, perror_huge_argument)
{
	const char *msg = testdata_hugeStr;
	char *res;
	char exp[testdata_hugeSize + 100];

	errno = 42;
	res = perrorToFile(msg);
	sprintf(exp, "%s: %s\n", msg, strerror(errno));
	SECURE_CHECK(strcmp(exp, res) == 0, res);
}


TEST(string_perror, perror_every_errno)
{
	char *old_msg;
	char *new_msg;

	for (int i = 0; i < 150; i++) {
		errno = i;
		old_msg = perrorToFile("Some msg");
		errno++;
		new_msg = perrorToFile("Some msg");
		SECURE_CHECK(strcmp(old_msg, new_msg) != 0, old_msg, new_msg);
	}
}


TEST(string_perror, perror_null_argument)
{
	TEST_IGNORE_MESSAGE("#929 issue");
	char *res;
	char exp[50];
	res = perrorToFile(NULL);

	strcpy(exp, strerror(errno));
	strcat(exp, "\n");
	SECURE_CHECK(strcmp(exp, res) == 0, res);
}


TEST_GROUP_RUNNER(string_perror)
{
	RUN_TEST_CASE(string_perror, perror_basic);
	RUN_TEST_CASE(string_perror, perror_empty_message);
	RUN_TEST_CASE(string_perror, perror_huge_argument);
	RUN_TEST_CASE(string_perror, perror_null_argument);
	RUN_TEST_CASE(string_perror, perror_every_ascii);
	RUN_TEST_CASE(string_perror, perror_every_errno);
}
