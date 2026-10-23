/*
 * Phoenix-RTOS
 *
 * libc-tests
 *
 * Helpers shared by the scanf tests. Every test checks the same input and format
 * against all scanf functions by iterating over test_scanfFunc_t:
 *
 *	test_scanfSetInput("12 34");
 *	for (f = 0; f < scanf_funcCount; f++) {
 *		test_scanfPrepare(f, 0);
 *		a = b = 0;
 *		TEST_ASSERT_EQUAL_INT(2, TEST_SCANF(f, "%d %d", &a, &b));
 *		...
 *	}
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


#define TEST_SCANF_FILE "stdio_scanf_test"


typedef enum {
	scanf_fscanf,
	scanf_vfscanf,
	scanf_scanf,
	scanf_vscanf,
	scanf_sscanf,
	scanf_vsscanf,
	scanf_funcCount
} test_scanfFunc_t;


static const char *const test_scanfFuncName[scanf_funcCount] = {
	[scanf_fscanf] = "fscanf",
	[scanf_vfscanf] = "vfscanf",
	[scanf_scanf] = "scanf",
	[scanf_vscanf] = "vscanf",
	[scanf_sscanf] = "sscanf",
	[scanf_vsscanf] = "vsscanf",
};


static struct {
	FILE *file;            /* input of fscanf() and vfscanf() */
	const char *str;       /* input of sscanf() and vsscanf() */
	const char *pos;       /* position of the next sscanf()/vsscanf() call in str */
	test_scanfFunc_t func; /* function being checked, reported on failure */
	int savedStdin;
} test_scanfCommon = { .func = scanf_funcCount, .savedStdin = -1 };


/* Calls one of the va_list taking functions with the input set by test_scanfSetInput() */
static int test_vscanfFunc(test_scanfFunc_t f, const char *format, ...)
{
	int rc;
	va_list arg_ptr;

	va_start(arg_ptr, format);
	switch (f) {
		case scanf_vfscanf:
			rc = vfscanf(test_scanfCommon.file, format, arg_ptr);
			break;
		case scanf_vscanf:
			rc = vscanf(format, arg_ptr);
			break;
		default:
			rc = vsscanf(test_scanfCommon.pos, format, arg_ptr);
			break;
	}
	va_end(arg_ptr);

	return rc;
}


/* Calls `f` with the input set by test_scanfSetInput() at the position set by test_scanfPrepare() */
/* clang-format off */
#define TEST_SCANF(f, format, ...) \
	(((f) == scanf_fscanf) ? fscanf(test_scanfCommon.file, format, __VA_ARGS__) : \
		(((f) == scanf_scanf) ? scanf(format, __VA_ARGS__) : \
			(((f) == scanf_sscanf) ? sscanf(test_scanfCommon.pos, format, __VA_ARGS__) : \
				test_vscanfFunc(f, format, __VA_ARGS__))))
/* clang-format on */


/* Makes `str` the input of all scanf functions. `str` must stay valid until the next call. */
static void test_scanfSetInput(const char *str)
{
	/* Drop data stdin has read ahead, so that it isn't reused after the file is rewritten */
	fflush(stdin);

	TEST_ASSERT_EQUAL_INT(0, fclose(test_scanfCommon.file));
	test_scanfCommon.file = fopen(TEST_SCANF_FILE, "w+");
	TEST_ASSERT_NOT_NULL(test_scanfCommon.file);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fprintf(test_scanfCommon.file, "%s", str));
	TEST_ASSERT_EQUAL_INT(0, fflush(test_scanfCommon.file));

	test_scanfCommon.str = str;
	test_scanfCommon.pos = str;
}


/* Positions the input of `f` at `offset` and makes test_scanfTeardown() report `f` if the test fails */
static void test_scanfPrepare(test_scanfFunc_t f, size_t offset)
{
	test_scanfCommon.func = f;

	TEST_ASSERT_EQUAL_INT(0, fseek(test_scanfCommon.file, offset, SEEK_SET));
	TEST_ASSERT_EQUAL_INT(0, fseek(stdin, offset, SEEK_SET));
	test_scanfCommon.pos = test_scanfCommon.str + offset;
}


/* To be called in TEST_SETUP() - creates the empty input file and makes stdin read from it */
static void test_scanfSetup(void)
{
	int fd;

	test_scanfCommon.func = scanf_funcCount;
	test_scanfCommon.str = "";
	test_scanfCommon.pos = test_scanfCommon.str;
	test_scanfCommon.file = fopen(TEST_SCANF_FILE, "w+");
	TEST_ASSERT_NOT_NULL(test_scanfCommon.file);

	test_scanfCommon.savedStdin = dup(STDIN_FILENO);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, test_scanfCommon.savedStdin);

	fd = open(TEST_SCANF_FILE, O_RDONLY);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fd);
	TEST_ASSERT_EQUAL_INT(STDIN_FILENO, dup2(fd, STDIN_FILENO));
	TEST_ASSERT_EQUAL_INT(0, close(fd));
}


/* To be called in TEST_TEAR_DOWN() */
static void test_scanfTeardown(void)
{
	if ((Unity.CurrentTestFailed != 0) && (test_scanfCommon.func != scanf_funcCount)) {
		UnityPrint("Failed for ");
		UnityPrint(test_scanfFuncName[test_scanfCommon.func]);
		UnityPrint("()");
		UNITY_PRINT_EOL();
	}

	if (test_scanfCommon.file != NULL) {
		fclose(test_scanfCommon.file);
		test_scanfCommon.file = NULL;
	}

	if (test_scanfCommon.savedStdin >= 0) {
		/* Drop data buffered from the file before switching back */
		fflush(stdin);
		clearerr(stdin);

		TEST_ASSERT_EQUAL_INT(STDIN_FILENO, dup2(test_scanfCommon.savedStdin, STDIN_FILENO));
		TEST_ASSERT_EQUAL_INT(0, close(test_scanfCommon.savedStdin));
		test_scanfCommon.savedStdin = -1;
	}

	remove(TEST_SCANF_FILE);
}

#endif
