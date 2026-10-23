/*
 * Phoenix-RTOS
 *
 * libc-tests
 *
 * Helpers shared by the scanf tests.
 *
 * Copyright 2023, 2024, 2026 Phoenix Systems
 * Author: Damian Modzelewski, Adam Greloch
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _TEST_LIBC_STDIO_SCANF_COMMON_H
#define _TEST_LIBC_STDIO_SCANF_COMMON_H

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <unity_fixture.h>


static int test_savedStdin = -1;


static int test_vsscanfWrapper(const char *str, const char *format, ...)
{
	int rc;
	va_list arg_ptr;

	va_start(arg_ptr, format);
	rc = vsscanf(str, format, arg_ptr);
	va_end(arg_ptr);

	return rc;
}


static int test_vfscanfWrapper(FILE *stream, const char *format, ...)
{
	int rc;
	va_list arg_ptr;

	va_start(arg_ptr, format);
	rc = vfscanf(stream, format, arg_ptr);
	va_end(arg_ptr);

	return rc;
}


static int test_vscanfWrapper(const char *format, ...)
{
	int rc;
	va_list arg_ptr;

	va_start(arg_ptr, format);
	rc = vscanf(format, arg_ptr);
	va_end(arg_ptr);

	/* Drop read-ahead data, so that it isn't reused after the file is rewritten */
	fflush(stdin);

	return rc;
}


/* Makes stdin read from the file at `path` */
static void test_stdinRedirect(const char *path)
{
	int fd;

	test_savedStdin = dup(STDIN_FILENO);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_savedStdin);

	fd = open(path, O_RDONLY);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);
	TEST_ASSERT_EQUAL_INT(STDIN_FILENO, dup2(fd, STDIN_FILENO));
	TEST_ASSERT_EQUAL_INT(0, close(fd));
}


static void test_stdinRestore(void)
{
	if (test_savedStdin < 0) {
		return;
	}

	/* Drop data buffered from the file before switching back */
	fflush(stdin);
	clearerr(stdin);

	TEST_ASSERT_EQUAL_INT(STDIN_FILENO, dup2(test_savedStdin, STDIN_FILENO));
	TEST_ASSERT_EQUAL_INT(0, close(test_savedStdin));
	test_savedStdin = -1;
}

#endif
