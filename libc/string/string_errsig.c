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
#include <unity_fixture.h>

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
