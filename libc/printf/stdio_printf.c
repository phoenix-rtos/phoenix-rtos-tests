/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - stdio.h
 *
 * TESTED:
 *    - dprintf()
 *    - fprintf()
 *    - printf()
 *    - snprintf()
 *    - sprintf()
 *    - vdprintf()
 *    - vfprintf()
 *    - vprintf()
 *    - vsnprintf()
 *    - vsprintf()
 *
 * Copyright 2023 Phoenix Systems
 * Author: Mateusz Bloch
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <float.h>
#include <limits.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <unity_fixture.h>
#include <signal.h>

static FILE *test_outFile;
static char test_buff[700];

enum test_outType { DTEST,
	FTEST,
	TEST,
	SNTEST,
	STEST };

#define NUM_OUTPUT_TYPES 5

#define PATH "stdio_printf_test"

/* comma delimiter macro ',' for multiple arguments parsing to *printf() function via macros */
#define COMMA ,

/* macro assertion to test printf functions*/
#define test_assertPrintfs(expect, format, ...) \
	{ \
		for (enum test_outType type = DTEST; type < NUM_OUTPUT_TYPES; type++) { \
			memset(test_buff, 0, sizeof(test_buff)); \
			rewind(test_outFile); \
			switch (type) { \
				case DTEST: \
					dprintf(fileno(test_outFile), format, __VA_ARGS__); \
					break; \
				case FTEST: \
					fprintf(test_outFile, format, __VA_ARGS__); \
					break; \
				case TEST: { \
					int stdout_fd = dup(STDOUT_FILENO); \
					int fd = open(PATH, O_WRONLY | O_CREAT, 0666); \
					dup2(fd, STDOUT_FILENO); \
					printf(format, __VA_ARGS__); \
					fflush(stdout); \
					dup2(stdout_fd, STDOUT_FILENO); \
					close(fd); \
					close(stdout_fd); \
					break; \
				} \
				case SNTEST: \
					snprintf(test_buff, strlen(expect) + 1, format, __VA_ARGS__); \
					break; \
				case STEST: \
					sprintf(test_buff, format, __VA_ARGS__); \
					break; \
			} \
			fseek(test_outFile, 0, SEEK_SET); \
			fflush(test_outFile); \
			TEST_ASSERT_EQUAL_STRING(expect, fgets(test_buff, strlen(expect) + 1, test_outFile)); \
			TEST_ASSERT_EQUAL_STRING(expect, test_buff); \
		} \
	}

/* wrappers for vprintf tests*/
static void test_assertVprintfs(char *expect, const char *format, ...)
{
	va_list arg_ptr;
	va_start(arg_ptr, format);

	for (enum test_outType type = DTEST; type < NUM_OUTPUT_TYPES; type++) {
		memset(test_buff, 0, sizeof(test_buff));
		rewind(test_outFile);

		switch (type) {
			case DTEST:
				vdprintf(fileno(test_outFile), format, arg_ptr);
				break;
			case FTEST:
				vfprintf(test_outFile, format, arg_ptr);
				break;
			case TEST: {
				int stdout_fd = dup(STDOUT_FILENO);
				int fd = open(PATH, O_WRONLY | O_CREAT, 0666);
				dup2(fd, STDOUT_FILENO);
				vprintf(format, arg_ptr);
				fflush(stdout);
				dup2(stdout_fd, STDOUT_FILENO);
				close(fd);
				close(stdout_fd);
				break;
			}
			case SNTEST:
				vsnprintf(test_buff, strlen(expect) + 1, format, arg_ptr);
				break;
			case STEST:
				vsprintf(test_buff, format, arg_ptr);
				break;
		}

		fseek(test_outFile, 0, SEEK_SET);
		fflush(test_outFile);
		TEST_ASSERT_EQUAL_STRING(expect, fgets(test_buff, strlen(expect) + 1, test_outFile));
		TEST_ASSERT_EQUAL_STRING(expect, test_buff);

		va_end(arg_ptr);
		va_start(arg_ptr, format);  // re-initialize arg_ptr for next loop iteration
	}
	va_end(arg_ptr);
}


/* other functions */
static int test_signedToStr(long long value, int base, char *out)
{
	int len = 1;
	long long n = value;
	char *const out_orig = out;

	if (value < 0) {
		*out++ = '-';
	}

	while (n /= base) {
		len++;
	}

	for (int i = len - 1; i >= 0; i--) {
		int rem = value % base;
		if (rem < 0) {
			rem = -rem; /* support negative values by negating the remainder only*/
		}
		out[i] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
		value /= base;
	}
	out += len;
	*out = '\0';

	return out - out_orig;
}


static int test_unsignedToStr(unsigned long long int value, int base, bool bigLetters, char *out)
{
	int len = 1;
	unsigned long long n = value;

	while (n /= base) {
		len++;
	}

	char *const out_orig = out;

	for (int i = len - 1; i >= 0; i--) {
		int rem = value % base;
		if (bigLetters)
			out[i] = (rem > 9) ? (rem - 10) + 'A' : rem + '0';
		else
			out[i] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
		value /= base;
	}

	out += len;
	*out = '\0';

	return out - out_orig;
}

///////////////////////////////////////////////////////////////////////////////

TEST_GROUP(stdio_printf_d);
TEST_GROUP(stdio_printf_i);
TEST_GROUP(stdio_printf_o);
TEST_GROUP(stdio_printf_u);
TEST_GROUP(stdio_printf_x);
TEST_GROUP(stdio_printf_fega);
TEST_GROUP(stdio_printf_cspn);
TEST_GROUP(stdio_printf_rest);


TEST_SETUP(stdio_printf_d)
{
	test_outFile = fopen(PATH, "w+");
}


TEST_TEAR_DOWN(stdio_printf_d)
{
	fclose(test_outFile);
	remove(PATH);
}


TEST(stdio_printf_d, d)
{
	char expect[256] = { 0 };
	char *buf = expect;
	const char *format = "%d %d %d %d %d";
	const int values[] = { INT_MAX, INT_MAX / 2, 0, INT_MIN / 2, INT_MIN };

	for (int i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {
		if (i > 0) {
			*buf++ = ' ';
		}
		buf += test_signedToStr(values[i], 10, buf);
	}

	test_assertPrintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
}


TEST(stdio_printf_d, hhd)
{
	char expect[256] = { 0 };
	char *buf = expect;
	const char *format = "%hhd %hhd %hhd %hhd %hhd";
	const signed char values[] = { SCHAR_MAX, SCHAR_MAX / 2, 0, SCHAR_MIN / 2, SCHAR_MIN };

	for (int i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {
		if (i > 0) {
			*buf++ = ' ';
		}
		buf += test_signedToStr(values[i], 10, buf);
	}

	test_assertPrintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
}


TEST(stdio_printf_d, hd)
{
	char expect[256] = { 0 };
	char *buf = expect;
	const char *format = "%hd %hd %hd %hd %hd";
	const short values[] = { SHRT_MAX, SHRT_MAX / 2, 0, SHRT_MIN / 2, SHRT_MIN };

	for (int i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {
		if (i > 0) {
			*buf++ = ' ';
		}
		buf += test_signedToStr(values[i], 10, buf);
	}

	test_assertPrintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
}


TEST(stdio_printf_d, ld)
{
	char expect[256] = { 0 };
	char *buf = expect;
	const char *format = "%ld %ld %ld %ld %ld";
	const long int values[] = { LONG_MAX, LONG_MAX / 2, 0, LONG_MIN / 2, LONG_MIN };

	for (int i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {
		if (i > 0) {
			*buf++ = ' ';
		}
		buf += test_signedToStr(values[i], 10, buf);
	}

	test_assertPrintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
}


TEST(stdio_printf_d, lld)
{
	char expect[256] = { 0 };
	char *buf = expect;
	const char *format = "%lld %lld %lld %lld %lld";
	const long long int values[] = { LLONG_MAX, LLONG_MAX / 2, 0, LLONG_MIN / 2, LLONG_MIN };

	for (int i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {
		if (i > 0) {
			*buf++ = ' ';
		}
		buf += test_signedToStr(values[i], 10, buf);
	}

	test_assertPrintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
}


TEST(stdio_printf_d, jd)
{
	char expect[256] = { 0 };
	char *buf = expect;
	const char *format = "%jd %jd %jd %jd %jd";
	const intmax_t values[] = { INTMAX_MAX, INTMAX_MAX / 2, 0, INTMAX_MIN / 2, INTMAX_MIN };

	for (int i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {
		if (i > 0) {
			*buf++ = ' ';
		}
		buf += test_signedToStr(values[i], 10, buf);
	}

	test_assertPrintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
}


TEST(stdio_printf_d, zd)
{
	char expect[256] = { 0 };
	char *buf = expect;
	const char *format = "%zd %zd %zd";
	const size_t values[] = { SSIZE_MAX, SSIZE_MAX / 2, 0 };

	for (int i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {
		if (i > 0) {
			*buf++ = ' ';
		}
		buf += test_signedToStr(values[i], 10, buf);
	}

	test_assertPrintfs(expect, format, values[0], values[1], values[2]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2]);
}


TEST(stdio_printf_d, td)
{
	char expect[256] = { 0 };
	char *buf = expect;
	const char *format = "%td %td %td %td %td";
	const ptrdiff_t values[] = { PTRDIFF_MAX, PTRDIFF_MAX / 2, 0, PTRDIFF_MIN / 2, PTRDIFF_MIN };

	for (int i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {
		if (i > 0) {
			*buf++ = ' ';
		}
		buf += test_signedToStr(values[i], 10, buf);
	}

	test_assertPrintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
}


TEST(stdio_printf_d, out_of_bonds)
{
	char expect[256] = { 0 };
	char *buf = expect;
	const char *format = "%hhd %hd";

	/* Creating string with expecting output */
	buf += test_signedToStr((signed char)(INT_MAX), 10, buf);

	*buf++ = ' ';
	buf += test_signedToStr((short)(INT_MAX), 10, buf);

	test_assertPrintfs(expect, format, INT_MAX, INT_MAX);
	test_assertVprintfs(expect, format, INT_MAX, INT_MAX);
}

///////////////////////////////////////////////////////////////////////////////

TEST_SETUP(stdio_printf_i)
{
	test_outFile = fopen(PATH, "w+");
}


TEST_TEAR_DOWN(stdio_printf_i)
{
	fclose(test_outFile);
	remove(PATH);
}


TEST(stdio_printf_i, i)
{
	char expect[256] = { 0 };
	char *buf = expect;
	const char *format = "%i %i %i %i %i";
	const int values[] = { INT_MAX, INT_MAX / 2, 0, INT_MIN / 2, INT_MIN };

	for (int i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {
		if (i > 0) {
			*buf++ = ' ';
		}
		buf += test_signedToStr(values[i], 10, buf);
	}

	test_assertPrintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
}


TEST(stdio_printf_i, hhi)
{
	char expect[256] = { 0 };
	char *buf = expect;
	const char *format = "%hhi %hhi %hhi %hhi %hhi";
	const signed char values[] = { SCHAR_MAX, SCHAR_MAX / 2, 0, SCHAR_MIN / 2, SCHAR_MIN };

	for (int i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {
		if (i > 0) {
			*buf++ = ' ';
		}
		buf += test_signedToStr(values[i], 10, buf);
	}

	test_assertPrintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
}


TEST(stdio_printf_i, hi)
{
	char expect[256] = { 0 };
	char *buf = expect;
	const char *format = "%hi %hi %hi %hi %hi";
	const short values[] = { SHRT_MAX, SHRT_MAX / 2, 0, SHRT_MIN / 2, SHRT_MIN };

	for (int i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {
		if (i > 0) {
			*buf++ = ' ';
		}
		buf += test_signedToStr(values[i], 10, buf);
	}

	test_assertPrintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
}


TEST(stdio_printf_i, li)
{
	char expect[256] = { 0 };
	char *buf = expect;
	const char *format = "%li %li %li %li %li";
	const long int values[] = { LONG_MAX, LONG_MAX / 2, 0, LONG_MIN / 2, LONG_MIN };

	for (int i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {
		if (i > 0) {
			*buf++ = ' ';
		}
		buf += test_signedToStr(values[i], 10, buf);
	}

	test_assertPrintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
}


TEST(stdio_printf_i, lli)
{
	char expect[256] = { 0 };
	char *buf = expect;
	const char *format = "%lli %lli %lli %lli %lli";
	const long long int values[] = { LLONG_MAX, LLONG_MAX / 2, 0, LLONG_MIN / 2, LLONG_MIN };

	for (int i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {
		if (i > 0) {
			*buf++ = ' ';
		}
		buf += test_signedToStr(values[i], 10, buf);
	}

	test_assertPrintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
}


TEST(stdio_printf_i, ji)
{
	char expect[256] = { 0 };
	char *buf = expect;
	const char *format = "%ji %ji %ji %ji %ji";
	const intmax_t values[] = { INTMAX_MAX, INTMAX_MAX / 2, 0, INTMAX_MIN / 2, INTMAX_MIN };

	for (int i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {
		if (i > 0) {
			*buf++ = ' ';
		}
		buf += test_signedToStr(values[i], 10, buf);
	}

	test_assertPrintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
}


TEST(stdio_printf_i, zi)
{
	char expect[256] = { 0 };
	char *buf = expect;
	const char *format = "%zi %zi %zi";
	const size_t values[] = { SSIZE_MAX, SSIZE_MAX / 2, 0 };

	for (int i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {
		if (i > 0) {
			*buf++ = ' ';
		}
		buf += test_signedToStr(values[i], 10, buf);
	}

	test_assertPrintfs(expect, format, values[0], values[1], values[2]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2]);
}


TEST(stdio_printf_i, ti)
{
	char expect[256] = { 0 };
	char *buf = expect;
	const char *format = "%ti %ti %ti %ti %ti";
	const ptrdiff_t values[] = { PTRDIFF_MAX, PTRDIFF_MAX / 2, 0, PTRDIFF_MIN / 2, PTRDIFF_MIN };

	for (int i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {
		if (i > 0) {
			*buf++ = ' ';
		}
		buf += test_signedToStr(values[i], 10, buf);
	}

	test_assertPrintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
}


TEST(stdio_printf_i, out_of_bonds)
{
	char expect[256] = { 0 };
	char *buf = expect;
	const char *format = "%hhi %hi";

	/* Creating string with expecting output */
	buf += test_signedToStr((signed char)(INT_MAX), 10, buf);
	*buf++ = ' ';

	buf += test_signedToStr((short)(INT_MAX), 10, buf);

	test_assertPrintfs(expect, format, INT_MAX, INT_MAX);

	test_assertVprintfs(expect, format, INT_MAX, INT_MAX);
}

///////////////////////////////////////////////////////////////////////////////

TEST_SETUP(stdio_printf_o)
{
	test_outFile = fopen(PATH, "w+");
}


TEST_TEAR_DOWN(stdio_printf_o)
{
	fclose(test_outFile);
	remove(PATH);
}


TEST(stdio_printf_o, o)
{
	char expect[256] = { 0 };
	char *buf = expect;
	const char *format = "%o %o %o";
	const unsigned int values[] = { UINT_MAX, UINT_MAX / 2, 0 };

	for (int i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {
		if (i > 0) {
			*buf++ = ' ';
		}
		buf += test_unsignedToStr(values[i], 8, false, buf);
	}

	test_assertPrintfs(expect, format, values[0], values[1], values[2]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2]);
}


TEST(stdio_printf_o, hho)
{
	char expect[256] = { 0 };
	char *buf = expect;
	const char *format = "%hho %hho %hho";
	const unsigned char values[] = { UCHAR_MAX, UCHAR_MAX / 2, 0 };

	for (int i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {
		if (i > 0) {
			*buf++ = ' ';
		}
		buf += test_unsignedToStr(values[i], 8, false, buf);
	}

	test_assertPrintfs(expect, format, values[0], values[1], values[2]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2]);
}


TEST(stdio_printf_o, ho)
{
	char expect[256] = { 0 };
	char *buf = expect;
	const char *format = "%ho %ho %ho";
	const unsigned short values[] = { USHRT_MAX, USHRT_MAX / 2, 0 };

	for (int i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {
		if (i > 0) {
			*buf++ = ' ';
		}
		buf += test_unsignedToStr(values[i], 8, false, buf);
	}

	test_assertPrintfs(expect, format, values[0], values[1], values[2]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2]);
}


TEST(stdio_printf_o, lo)
{
	char expect[256] = { 0 };
	char *buf = expect;
	const char *format = "%lo %lo %lo";
	const unsigned long int values[] = { ULONG_MAX, ULONG_MAX / 2, 0 };

	for (int i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {
		if (i > 0) {
			*buf++ = ' ';
		}
		buf += test_unsignedToStr(values[i], 8, false, buf);
	}

	test_assertPrintfs(expect, format, values[0], values[1], values[2]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2]);
}


TEST(stdio_printf_o, llo)
{
	char expect[256] = { 0 };
	char *buf = expect;
	const char *format = "%llo %llo %llo";
	const unsigned long long int values[] = { ULLONG_MAX, ULLONG_MAX / 2, 0 };

	for (int i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {
		if (i > 0) {
			*buf++ = ' ';
		}
		buf += test_unsignedToStr(values[i], 8, false, buf);
	}

	test_assertPrintfs(expect, format, values[0], values[1], values[2]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2]);
}


TEST(stdio_printf_o, jo)
{
	char expect[256] = { 0 };
	char *buf = expect;
	const char *format = "%jo %jo %jo";
	const uintmax_t values[] = { UINTMAX_MAX, UINTMAX_MAX / 2, 0 };

	for (int i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {
		if (i > 0) {
			*buf++ = ' ';
		}
		buf += test_unsignedToStr(values[i], 8, false, buf);
	}

	test_assertPrintfs(expect, format, values[0], values[1], values[2]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2]);
}


TEST(stdio_printf_o, zo)
{
	char expect[256] = { 0 };
	char *buf = expect;
	const char *format = "%zo %zo %zo";
	const size_t values[] = { SIZE_MAX, SIZE_MAX / 2, 0 };

	for (int i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {
		if (i > 0) {
			*buf++ = ' ';
		}
		buf += test_unsignedToStr(values[i], 8, false, buf);
	}

	test_assertPrintfs(expect, format, values[0], values[1], values[2]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2]);
}


TEST(stdio_printf_o, to)
{
	char expect[256] = { 0 };
	char *buf = expect;
	const char *format = "%to %to %to";
	/* only max tested because %to, applies to unsigned type argument; */
	const ptrdiff_t values[] = { PTRDIFF_MAX, PTRDIFF_MAX / 2, 0 };

	for (int i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {
		if (i > 0) {
			*buf++ = ' ';
		}
		buf += test_unsignedToStr(values[i], 8, false, buf);
	}

	test_assertPrintfs(expect, format, values[0], values[1], values[2]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2]);
}


TEST(stdio_printf_o, out_of_bonds)
{
	char expect[256] = { 0 };
	char *buf = expect;
	const char *format = "%hho %ho";

	/* Creating string with expecting output */
	buf += test_unsignedToStr((unsigned char)(UINT_MAX), 8, false, buf);
	*buf++ = ' ';

	buf += test_unsignedToStr((unsigned short)(UINT_MAX), 8, false, buf);

	test_assertPrintfs(expect, format, UINT_MAX, UINT_MAX);
	test_assertVprintfs(expect, format, UINT_MAX, UINT_MAX);
}

///////////////////////////////////////////////////////////////////////////////

TEST_SETUP(stdio_printf_u)
{
	test_outFile = fopen(PATH, "w+");
}


TEST_TEAR_DOWN(stdio_printf_u)
{
	fclose(test_outFile);
	remove(PATH);
}


TEST(stdio_printf_u, u)
{
	char expect[256] = { 0 };
	char *buf = expect;
	const char *format = "%u %u %u";
	const unsigned int values[] = { UINT_MAX, UINT_MAX / 2, 0 };

	for (int i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {
		if (i > 0) {
			*buf++ = ' ';
		}
		buf += test_unsignedToStr(values[i], 10, false, buf);
	}

	test_assertPrintfs(expect, format, values[0], values[1], values[2]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2]);
}


TEST(stdio_printf_u, hhu)
{
	char expect[256] = { 0 };
	char *buf = expect;
	const char *format = "%hhu %hhu %hhu";
	const unsigned char values[] = { UCHAR_MAX, UCHAR_MAX / 2, 0 };

	for (int i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {
		if (i > 0) {
			*buf++ = ' ';
		}
		buf += test_unsignedToStr(values[i], 10, false, buf);
	}

	test_assertPrintfs(expect, format, values[0], values[1], values[2]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2]);
}


TEST(stdio_printf_u, hu)
{
	char expect[256] = { 0 };
	char *buf = expect;
	const char *format = "%hu %hu %hu";
	const unsigned short values[] = { USHRT_MAX, USHRT_MAX / 2, 0 };

	for (int i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {
		if (i > 0) {
			*buf++ = ' ';
		}
		buf += test_unsignedToStr(values[i], 10, false, buf);
	}

	test_assertPrintfs(expect, format, values[0], values[1], values[2]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2]);
}


TEST(stdio_printf_u, lu)
{
	char expect[256] = { 0 };
	char *buf = expect;
	const char *format = "%lu %lu %lu";
	const unsigned long int values[] = { ULONG_MAX, ULONG_MAX / 2, 0 };

	for (int i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {
		if (i > 0) {
			*buf++ = ' ';
		}
		buf += test_unsignedToStr(values[i], 10, false, buf);
	}

	test_assertPrintfs(expect, format, values[0], values[1], values[2]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2]);
}


TEST(stdio_printf_u, llu)
{
	char expect[256] = { 0 };
	char *buf = expect;
	const char *format = "%llu %llu %llu";
	const unsigned long long int values[] = { ULLONG_MAX, ULLONG_MAX / 2, 0 };

	for (int i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {
		if (i > 0) {
			*buf++ = ' ';
		}
		buf += test_unsignedToStr(values[i], 10, false, buf);
	}

	test_assertPrintfs(expect, format, values[0], values[1], values[2]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2]);
}


TEST(stdio_printf_u, ju)
{
	char expect[256] = { 0 };
	char *buf = expect;
	const char *format = "%ju %ju %ju";
	const uintmax_t values[] = { UINTMAX_MAX, UINTMAX_MAX / 2, 0 };

	for (int i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {
		if (i > 0) {
			*buf++ = ' ';
		}
		buf += test_unsignedToStr(values[i], 10, false, buf);
	}

	test_assertPrintfs(expect, format, values[0], values[1], values[2]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2]);
}


TEST(stdio_printf_u, zu)
{
	char expect[256] = { 0 };
	char *buf = expect;
	const char *format = "%zu %zu %zu";
	const size_t values[] = { SIZE_MAX, SIZE_MAX / 2, 0 };

	for (int i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {
		if (i > 0) {
			*buf++ = ' ';
		}
		buf += test_unsignedToStr(values[i], 10, false, buf);
	}

	test_assertPrintfs(expect, format, values[0], values[1], values[2]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2]);
}


TEST(stdio_printf_u, tu)
{
	char expect[256] = { 0 };
	char *buf = expect;
	const char *format = "%tu %tu %tu";
	/* only max tested because %tu, applies to unsigned type argument; */
	const ptrdiff_t values[] = { PTRDIFF_MAX, PTRDIFF_MAX / 2, 0 };

	for (int i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {
		if (i > 0) {
			*buf++ = ' ';
		}
		buf += test_unsignedToStr(values[i], 10, false, buf);
	}

	test_assertPrintfs(expect, format, values[0], values[1], values[2]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2]);
}


TEST(stdio_printf_u, out_of_bonds)
{
	char expect[256] = { 0 };
	char *buf = expect;
	const char *format = "%hhu %hu";

	buf += test_unsignedToStr((unsigned char)(UINT_MAX), 10, false, buf);
	*buf++ = ' ';
	buf += test_unsignedToStr((unsigned short)(UINT_MAX), 10, false, buf);

	test_assertPrintfs(expect, format, UINT_MAX, UINT_MAX);
	test_assertVprintfs(expect, format, UINT_MAX, UINT_MAX);
}

///////////////////////////////////////////////////////////////////////////////

TEST_SETUP(stdio_printf_x)
{
	test_outFile = fopen(PATH, "w+");
}


TEST_TEAR_DOWN(stdio_printf_x)
{
	fclose(test_outFile);
	remove(PATH);
}


TEST(stdio_printf_x, x)
{
	char expect[256] = { 0 };
	char *buf = expect;
	const char *format = "%x %x %x";
	const unsigned int values[] = { UINT_MAX, UINT_MAX / 2, 0 };

	for (int i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {
		if (i > 0) {
			*buf++ = ' ';
		}
		buf += test_unsignedToStr(values[i], 16, false, buf);
	}

	test_assertPrintfs(expect, format, values[0], values[1], values[2]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2]);
}

TEST(stdio_printf_x, hhx)
{
	char expect[256] = { 0 };
	char *buf = expect;
	const char *format = "%hhx %hhx %hhx";
	const unsigned char values[] = { UCHAR_MAX, UCHAR_MAX / 2, 0 };

	for (int i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {
		if (i > 0) {
			*buf++ = ' ';
		}
		buf += test_unsignedToStr(values[i], 16, false, buf);
	}

	test_assertPrintfs(expect, format, values[0], values[1], values[2]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2]);
}


TEST(stdio_printf_x, hx)
{
	char expect[256] = { 0 };
	char *buf = expect;
	const char *format = "%hx %hx %hx";
	const unsigned short values[] = { USHRT_MAX, USHRT_MAX / 2, 0 };

	for (int i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {
		if (i > 0) {
			*buf++ = ' ';
		}
		buf += test_unsignedToStr(values[i], 16, false, buf);
	}

	test_assertPrintfs(expect, format, values[0], values[1], values[2]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2]);
}


TEST(stdio_printf_x, lx)
{
	char expect[256] = { 0 };
	char *buf = expect;
	const char *format = "%lx %lx %lx";
	const unsigned long int values[] = { ULONG_MAX, ULONG_MAX / 2, 0 };

	for (int i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {
		if (i > 0) {
			*buf++ = ' ';
		}
		buf += test_unsignedToStr(values[i], 16, false, buf);
	}

	test_assertPrintfs(expect, format, values[0], values[1], values[2]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2]);
}


TEST(stdio_printf_x, llx)
{
	char expect[256] = { 0 };
	char *buf = expect;
	const char *format = "%llx %llx %llx";
	const unsigned long long int values[] = { ULLONG_MAX, ULLONG_MAX / 2, 0 };

	for (int i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {
		if (i > 0) {
			*buf++ = ' ';
		}
		buf += test_unsignedToStr(values[i], 16, false, buf);
	}

	test_assertPrintfs(expect, format, values[0], values[1], values[2]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2]);
}


TEST(stdio_printf_x, jx)
{
	char expect[256] = { 0 };
	char *buf = expect;
	const char *format = "%jx %jx %jx";
	const uintmax_t values[] = { UINTMAX_MAX, UINTMAX_MAX / 2, 0 };

	for (int i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {
		if (i > 0) {
			*buf++ = ' ';
		}
		buf += test_unsignedToStr(values[i], 16, false, buf);
	}

	test_assertPrintfs(expect, format, values[0], values[1], values[2]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2]);
}


TEST(stdio_printf_x, zx)
{
	char expect[256] = { 0 };
	char *buf = expect;
	const char *format = "%zx %zx %zx";
	const size_t values[] = { SIZE_MAX, SIZE_MAX / 2, 0 };

	for (int i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {
		if (i > 0) {
			*buf++ = ' ';
		}
		buf += test_unsignedToStr(values[i], 16, false, buf);
	}

	test_assertPrintfs(expect, format, values[0], values[1], values[2]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2]);
}


TEST(stdio_printf_x, tx)
{
	char expect[256] = { 0 };
	char *buf = expect;
	const char *format = "%tx %tx %tx";
	/* only max tested because %tx, applies to unsigned type argument; */
	const ptrdiff_t values[] = { PTRDIFF_MAX, PTRDIFF_MAX / 2, 0 };

	for (int i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {
		if (i > 0) {
			*buf++ = ' ';
		}
		buf += test_unsignedToStr(values[i], 16, false, buf);
	}

	test_assertPrintfs(expect, format, values[0], values[1], values[2]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2]);
}


TEST(stdio_printf_x, x_out_of_bonds)
{
	char expect[256] = { 0 };
	char *buf = expect;
	const char *format = "%hhx %hx";

	buf += test_unsignedToStr((unsigned char)(UINT_MAX), 16, false, buf);
	*buf++ = ' ';
	buf += test_unsignedToStr((unsigned short)(UINT_MAX), 16, false, buf);

	test_assertPrintfs(expect, format, UINT_MAX, UINT_MAX);
	test_assertVprintfs(expect, format, UINT_MAX, UINT_MAX);
}

///////////////////////////////////////////////////////////////////////////////

TEST(stdio_printf_x, X)
{
	char expect[256] = { 0 };
	char *buf = expect;
	const char *format = "%X %X %X";
	const unsigned int values[] = { UINT_MAX, UINT_MAX / 2, 0 };

	for (int i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {
		if (i > 0) {
			*buf++ = ' ';
		}
		buf += test_unsignedToStr(values[i], 16, true, buf);
	}

	test_assertPrintfs(expect, format, values[0], values[1], values[2]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2]);
}


TEST(stdio_printf_x, hhX)
{
	char expect[256] = { 0 };
	char *buf = expect;
	const char *format = "%hhX %hhX %hhX";
	const unsigned char values[] = { UCHAR_MAX, UCHAR_MAX / 2, 0 };

	for (int i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {
		if (i > 0) {
			*buf++ = ' ';
		}
		buf += test_unsignedToStr(values[i], 16, true, buf);
	}

	test_assertPrintfs(expect, format, values[0], values[1], values[2]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2]);
}


TEST(stdio_printf_x, hX)
{
	char expect[256] = { 0 };
	char *buf = expect;
	const char *format = "%hX %hX %hX";
	const unsigned short values[] = { USHRT_MAX, USHRT_MAX / 2, 0 };

	for (int i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {
		if (i > 0) {
			*buf++ = ' ';
		}
		buf += test_unsignedToStr(values[i], 16, true, buf);
	}

	test_assertPrintfs(expect, format, values[0], values[1], values[2]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2]);
}


TEST(stdio_printf_x, lX)
{
	char expect[256] = { 0 };
	char *buf = expect;
	const char *format = "%lX %lX %lX";
	const unsigned long int values[] = { ULONG_MAX, ULONG_MAX / 2, 0 };

	for (int i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {
		if (i > 0) {
			*buf++ = ' ';
		}
		buf += test_unsignedToStr(values[i], 16, true, buf);
	}

	test_assertPrintfs(expect, format, values[0], values[1], values[2]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2]);
}


TEST(stdio_printf_x, llX)
{
	char expect[256] = { 0 };
	char *buf = expect;
	const char *format = "%llX %llX %llX";
	const unsigned long long int values[] = { ULLONG_MAX, ULLONG_MAX / 2, 0 };

	for (int i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {
		if (i > 0) {
			*buf++ = ' ';
		}
		buf += test_unsignedToStr(values[i], 16, true, buf);
	}

	test_assertPrintfs(expect, format, values[0], values[1], values[2]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2]);
}


TEST(stdio_printf_x, jX)
{
	char expect[256] = { 0 };
	char *buf = expect;
	const char *format = "%jX %jX %jX";
	const uintmax_t values[] = { UINTMAX_MAX, UINTMAX_MAX / 2, 0 };

	for (int i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {
		if (i > 0) {
			*buf++ = ' ';
		}
		buf += test_unsignedToStr(values[i], 16, true, buf);
	}

	test_assertPrintfs(expect, format, values[0], values[1], values[2]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2]);
}


TEST(stdio_printf_x, zX)
{
	char expect[256] = { 0 };
	char *buf = expect;
	const char *format = "%zX %zX %zX";
	const size_t values[] = { SIZE_MAX, SIZE_MAX / 2, 0 };

	for (int i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {
		if (i > 0) {
			*buf++ = ' ';
		}
		buf += test_unsignedToStr(values[i], 16, true, buf);
	}

	test_assertPrintfs(expect, format, values[0], values[1], values[2]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2]);
}


TEST(stdio_printf_x, tX)
{
	char expect[256] = { 0 };
	char *buf = expect;
	const char *format = "%tX %tX %tX";
	/* only max tested because %tX, applies to unsigned type argument; */
	const ptrdiff_t values[] = { PTRDIFF_MAX, PTRDIFF_MAX / 2, 0 };

	for (int i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {
		if (i > 0) {
			*buf++ = ' ';
		}
		buf += test_unsignedToStr(values[i], 16, true, buf);
	}

	test_assertPrintfs(expect, format, values[0], values[1], values[2]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2]);
}


TEST(stdio_printf_x, X_out_of_bonds)
{
#ifdef __phoenix__
	TEST_IGNORE();
#endif
	char expect[256] = { 0 };
	char *buf = expect;
	const char *format = "%hhX %hX";

	buf += test_unsignedToStr((unsigned char)(UINT_MAX), 16, true, buf);
	*buf++ = ' ';
	buf += test_unsignedToStr((unsigned short)(UINT_MAX), 16, true, buf);

	test_assertPrintfs(expect, format, UINT_MAX, UINT_MAX);
	test_assertVprintfs(expect, format, UINT_MAX, UINT_MAX);
}

//////////////////////////////////////////////////////////////////////////

TEST_SETUP(stdio_printf_fega)
{
	test_outFile = fopen(PATH, "w+");
}


TEST_TEAR_DOWN(stdio_printf_fega)
{
	fclose(test_outFile);
	remove(PATH);
}


TEST(stdio_printf_fega, f)
{
	const char *format = "%f %f %f %f %f";
	const float values[] = {
		FLT_MIN,
		FLT_MIN / 2,
		0,
		FLT_MAX / 2,
		FLT_MAX,
	};
	char *expect = "0.000000 0.000000 0.000000 170141173319264429905852091742258462720.000000 340282346638528859811704183484516925440.000000";

	test_assertPrintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
}


TEST(stdio_printf_fega, lf)
{
	const char *format = "%lf %lf %lf %lf %lf";
	const double values[] = {
		DBL_MIN,
		DBL_MIN / 2,
		0,
		DBL_MAX / 2,
		DBL_MAX,
	};
	char *expect = "0.000000 0.000000 0.000000 89884656743115785407263711865852178399035283762922498299458738401578630390014269380294779316383439085770229476757191232117160663444732091384233773351768758493024955288275641038122745045194664472037934254227566971152291618451611474082904279666061674137398913102072361584369088590459649940625202013092062429184.000000 179769313486231570814527423731704356798070567525844996598917476803157260780028538760589558632766878171540458953514382464234321326889464182768467546703537516986049910576551282076245490090389328944075868508455133942304583236903222948165808559332123348274797826204144723168738177180919299881250404026184124858368.000000";

	test_assertPrintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
}


TEST(stdio_printf_fega, Lf)
{
	const char *format = "%Lf %Lf %Lf %Lf %Lf";
	const long double values[] = {
		DBL_MIN,
		DBL_MIN / 2,
		0,
		DBL_MAX / 2,
		DBL_MAX,
	};
	char *expect = "0.000000 0.000000 0.000000 89884656743115785407263711865852178399035283762922498299458738401578630390014269380294779316383439085770229476757191232117160663444732091384233773351768758493024955288275641038122745045194664472037934254227566971152291618451611474082904279666061674137398913102072361584369088590459649940625202013092062429184.000000 179769313486231570814527423731704356798070567525844996598917476803157260780028538760589558632766878171540458953514382464234321326889464182768467546703537516986049910576551282076245490090389328944075868508455133942304583236903222948165808559332123348274797826204144723168738177180919299881250404026184124858368.000000";

	test_assertPrintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
}


TEST(stdio_printf_fega, F)
{
	const char *format = "%F %F %F %F %F";
	const float values[] = {
		FLT_MIN,
		FLT_MIN / 2,
		0,
		FLT_MAX / 2,
		FLT_MAX,
	};
	char *expect = "0.000000 0.000000 0.000000 170141173319264429905852091742258462720.000000 340282346638528859811704183484516925440.000000";

	test_assertPrintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
}


TEST(stdio_printf_fega, lF)
{
	const char *format = "%lF %lF %lF %lF %lF";
	const double values[] = {
		DBL_MIN,
		DBL_MIN / 2,
		0,
		DBL_MAX / 2,
		DBL_MAX,
	};
	char *expect = "0.000000 0.000000 0.000000 89884656743115785407263711865852178399035283762922498299458738401578630390014269380294779316383439085770229476757191232117160663444732091384233773351768758493024955288275641038122745045194664472037934254227566971152291618451611474082904279666061674137398913102072361584369088590459649940625202013092062429184.000000 179769313486231570814527423731704356798070567525844996598917476803157260780028538760589558632766878171540458953514382464234321326889464182768467546703537516986049910576551282076245490090389328944075868508455133942304583236903222948165808559332123348274797826204144723168738177180919299881250404026184124858368.000000";

	test_assertPrintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
}


TEST(stdio_printf_fega, LF)
{
	const char *format = "%LF %LF %LF %LF %LF";
	const long double values[] = {
		DBL_MIN,
		DBL_MIN / 2,
		0,
		DBL_MAX / 2,
		DBL_MAX,
	};
	char *expect = "0.000000 0.000000 0.000000 89884656743115785407263711865852178399035283762922498299458738401578630390014269380294779316383439085770229476757191232117160663444732091384233773351768758493024955288275641038122745045194664472037934254227566971152291618451611474082904279666061674137398913102072361584369088590459649940625202013092062429184.000000 179769313486231570814527423731704356798070567525844996598917476803157260780028538760589558632766878171540458953514382464234321326889464182768467546703537516986049910576551282076245490090389328944075868508455133942304583236903222948165808559332123348274797826204144723168738177180919299881250404026184124858368.000000";

	test_assertPrintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
}


TEST(stdio_printf_fega, e)
{
	const char *format = "%e %e %e %e %e";
	const float values[] = {
		FLT_MIN,
		FLT_MIN / 2,
		0,
		FLT_MAX / 2,
		FLT_MAX,
	};
	char *expect = "1.175494e-38 5.877472e-39 0.000000e+00 1.701412e+38 3.402823e+38";

	test_assertPrintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
}


TEST(stdio_printf_fega, le)
{
	const char *format = "%le %le %le %le %le";
	const double values[] = {
		DBL_MIN,
		DBL_MIN / 2,
		0,
		DBL_MAX / 2,
		DBL_MAX,
	};
	char *expect = "2.225074e-308 1.112537e-308 0.000000e+00 8.988466e+307 1.797693e+308";

	test_assertPrintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
}


TEST(stdio_printf_fega, Le)
{
	const char *format = "%Le %Le %Le %Le %Le";
	const long double values[] = {
		DBL_MIN,
		DBL_MIN / 2,
		0,
		DBL_MAX / 2,
		DBL_MAX,
	};
	char *expect = "2.225074e-308 1.112537e-308 0.000000e+00 8.988466e+307 1.797693e+308";

	test_assertPrintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
}


TEST(stdio_printf_fega, E)
{
	const char *format = "%E %E %E %E %E";
	const float values[] = {
		FLT_MIN,
		FLT_MIN / 2,
		0,
		FLT_MAX / 2,
		FLT_MAX,
	};
	char *expect = "1.175494E-38 5.877472E-39 0.000000E+00 1.701412E+38 3.402823E+38";

	test_assertPrintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
}


TEST(stdio_printf_fega, lE)
{
	const char *format = "%lE %lE %lE %lE %lE";
	const double values[] = {
		DBL_MIN,
		DBL_MIN / 2,
		0,
		DBL_MAX / 2,
		DBL_MAX,
	};
	char *expect = "2.225074E-308 1.112537E-308 0.000000E+00 8.988466E+307 1.797693E+308";

	test_assertPrintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
}


TEST(stdio_printf_fega, LE)
{
	const char *format = "%LE %LE %LE %LE %LE";
	const long double values[] = {
		DBL_MIN,
		DBL_MIN / 2,
		0,
		DBL_MAX / 2,
		DBL_MAX,
	};
	char *expect = "2.225074E-308 1.112537E-308 0.000000E+00 8.988466E+307 1.797693E+308";

	test_assertPrintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
}


TEST(stdio_printf_fega, g)
{
	const char *format = "%g %g %g %g %g";
	const float values[] = {
		FLT_MIN,
		FLT_MIN / 2,
		0,
		FLT_MAX / 2,
		FLT_MAX,
	};
	char *expect = "1.17549e-38 5.87747e-39 0 1.70141e+38 3.40282e+38";

	test_assertPrintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
}


TEST(stdio_printf_fega, lg)
{
	const char *format = "%lg %lg %lg %lg %lg";
	const double values[] = {
		DBL_MIN,
		DBL_MIN / 2,
		0,
		DBL_MAX / 2,
		DBL_MAX,
	};
	char *expect = "2.22507e-308 1.11254e-308 0 8.98847e+307 1.79769e+308";

	test_assertPrintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
}


TEST(stdio_printf_fega, Lg)
{
	const char *format = "%Lg %Lg %Lg %Lg %Lg";
	const long double values[] = {
		DBL_MIN,
		DBL_MIN / 2,
		0,
		DBL_MAX / 2,
		DBL_MAX,
	};
	char *expect = "2.22507e-308 1.11254e-308 0 8.98847e+307 1.79769e+308";

	test_assertPrintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
}


TEST(stdio_printf_fega, G)
{
	const char *format = "%G %G %G %G %G";
	const float values[] = {
		FLT_MIN,
		FLT_MIN / 2,
		0,
		FLT_MAX / 2,
		FLT_MAX,
	};
	char *expect = "1.17549E-38 5.87747E-39 0 1.70141E+38 3.40282E+38";

	test_assertPrintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
}


TEST(stdio_printf_fega, lG)
{
	const char *format = "%lG %lG %lG %lG %lG";
	const double values[] = {
		DBL_MIN,
		DBL_MIN / 2,
		0,
		DBL_MAX / 2,
		DBL_MAX,
	};
	char *expect = "2.22507E-308 1.11254E-308 0 8.98847E+307 1.79769E+308";

	test_assertPrintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
}


TEST(stdio_printf_fega, LG)
{
	const char *format = "%LG %LG %LG %LG %LG";
	const long double values[] = {
		DBL_MIN,
		DBL_MIN / 2,
		0,
		DBL_MAX / 2,
		DBL_MAX,
	};
	char *expect = "2.22507E-308 1.11254E-308 0 8.98847E+307 1.79769E+308";

	test_assertPrintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
}


TEST(stdio_printf_fega, a)
{
	/* testing is with %.6a due to the cross-platform nature of the test. On phoenix at %a, there are trailing zeros, which is not a bug */
	const char *format = "%.6a %.6a %.6a %.6a %.6a";
	const float values[] = {
		FLT_MIN,
		FLT_MIN / 2,
		0,
		FLT_MAX / 2,
		FLT_MAX,
	};
	char *expect = "0x1.000000p-126 0x1.000000p-127 0x0.000000p+0 0x1.fffffep+126 0x1.fffffep+127";

	test_assertPrintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
}


TEST(stdio_printf_fega, la)
{
	/* testing is with %.6la due to the cross-platform nature of the test. On phoenix at %la, there are trailing zeros, which is not a bug */
	const char *format = "%.6la %.6la %.6la %.6la %.6la";
	const double values[] = {
		DBL_MIN,
		DBL_MIN / 2,
		0,
		DBL_MAX / 2,
		DBL_MAX,
	};
	char *expect = "0x1.000000p-1022 0x0.800000p-1022 0x0.000000p+0 0x2.000000p+1022 0x2.000000p+1023";

	test_assertPrintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
}


TEST(stdio_printf_fega, La)
{
/* Disabled because of #739 issue: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/739 */
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#739 issue");
#endif
	const char *format = "%.6La %.6La %.6La %.6La %.6La";
	const long double values[] = {
		DBL_MIN,
		DBL_MIN / 2,
		0,
		DBL_MAX / 2,
		DBL_MAX,
	};
	char *expect = "0x8.000000p-1025 0x8.000000p-1026 0x0.000000p+0 0x1.000000p+1023 0x1.000000p+1024";

	test_assertPrintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
}


TEST(stdio_printf_fega, A)
{
	/* testing is with %.6A due to the cross-platform nature of the test. On phoenix at %A, there are trailing zeros, which is not a bug */
	const char *format = "%.6A %.6A %.6A %.6A %.6A";
	const float values[] = {
		FLT_MIN,
		FLT_MIN / 2,
		0,
		FLT_MAX / 2,
		FLT_MAX,
	};
	char *expect = "0X1.000000P-126 0X1.000000P-127 0X0.000000P+0 0X1.FFFFFEP+126 0X1.FFFFFEP+127";

	test_assertPrintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
}


TEST(stdio_printf_fega, lA)
{
	/* testing is with %.6lA due to the cross-platform nature of the test. On phoenix at %lA, there are trailing zeros, which is not a bug */
	const char *format = "%.6lA %.6lA %.6lA %.6lA %.6lA";
	const double values[] = {
		DBL_MIN,
		DBL_MIN / 2,
		0,
		DBL_MAX / 2,
		DBL_MAX,
	};
	char *expect = "0X1.000000P-1022 0X0.800000P-1022 0X0.000000P+0 0X2.000000P+1022 0X2.000000P+1023";

	test_assertPrintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
}


TEST(stdio_printf_fega, LA)
{
/* Disabled because of #739 issue: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/739 */
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#739 issue");
#endif

	const char *format = "%.6LA %.6LA %.6LA %.6LA %.6LA";
	const long double values[] = {
		DBL_MIN,
		DBL_MIN / 2,
		0,
		DBL_MAX / 2,
		DBL_MAX,
	};
	char *expect = "0X8.000000P-1025 0X8.000000P-1026 0X0.000000P+0 0X1.000000P+1023 0X1.000000P+1024";

	test_assertPrintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2], values[3], values[4]);
}


TEST(stdio_printf_fega, fega_inf_nan)
{
	int i;
	const char *format[] = { "%f %f %f %f", "%e %e %e %e", "%g %g %g %g", "%a %a %a %a" };
	const float values[] = { INFINITY, -INFINITY, NAN, -NAN };
	char *expect = "inf -inf nan -nan";

	for (i = 0; i < (sizeof(format) / sizeof(format[0])); i++) {

		test_assertPrintfs(expect, format[i], values[0], values[1], values[2], values[3]);
		test_assertVprintfs(expect, format[i], values[0], values[1], values[2], values[3]);
	}
}


TEST(stdio_printf_fega, lfega_inf_nan)
{
	int i;
	const char *format[] = { "%lf %lf %lf %lf", "%le %le %le %le", "%lg %lg %lg %lg", "%la %la %la %la" };
	const double values[] = { INFINITY, -INFINITY, NAN, -NAN };
	char *expect = "inf -inf nan -nan";

	for (i = 0; i < (sizeof(format) / sizeof(format[0])); i++) {

		test_assertPrintfs(expect, format[i], values[0], values[1], values[2], values[3]);
		test_assertVprintfs(expect, format[i], values[0], values[1], values[2], values[3]);
	}
}


TEST(stdio_printf_fega, Lfega_inf_nan)
{
	int i;
	const char *format[] = { "%Lf %Lf %Lf %Lf", "%Le %Le %Le %Le", "%Lg %Lg %Lg %Lg", "%La %La %La %La" };
	const long double values[] = { INFINITY, -INFINITY, NAN, -NAN };
	char *expect = "inf -inf nan -nan";

	for (i = 0; i < (sizeof(format) / sizeof(format[0])); i++) {

		test_assertPrintfs(expect, format[i], values[0], values[1], values[2], values[3]);
		test_assertVprintfs(expect, format[i], values[0], values[1], values[2], values[3]);
	}
}


TEST(stdio_printf_fega, FEGA_inf_nan)
{
	int i;
	const char *format[] = { "%F %F %F %F", "%E %E %E %E", "%G %G %G %G", "%A %A %A %A" };
	const float values[] = { INFINITY, -INFINITY, NAN, -NAN };
	char *expect = "INF -INF NAN -NAN";

	for (i = 0; i < (sizeof(format) / sizeof(format[0])); i++) {

		test_assertPrintfs(expect, format[i], values[0], values[1], values[2], values[3]);
		test_assertVprintfs(expect, format[i], values[0], values[1], values[2], values[3]);
	}
}


TEST(stdio_printf_fega, lFEGA_inf_nan)
{
	int i;
	const char *format[] = { "%lF %lF %lF %lF", "%lE %lE %lE %lE", "%lG %lG %lG %lG", "%lA %lA %lA %lA" };
	const double values[] = { INFINITY, -INFINITY, NAN, -NAN };
	char *expect = "INF -INF NAN -NAN";

	for (i = 0; i < (sizeof(format) / sizeof(format[0])); i++) {

		test_assertPrintfs(expect, format[i], values[0], values[1], values[2], values[3]);
		test_assertVprintfs(expect, format[i], values[0], values[1], values[2], values[3]);
	}
}


TEST(stdio_printf_fega, LFEGA_inf_nan)
{
	int i;
	const char *format[] = { "%LF %LF %LF %LF", "%LE %LE %LE %LE", "%LG %LG %LG %LG", "%LA %LA %LA %LA" };
	const long double values[] = { INFINITY, -INFINITY, NAN, -NAN };
	char *expect = "INF -INF NAN -NAN";

	for (i = 0; i < (sizeof(format) / sizeof(format[0])); i++) {

		test_assertPrintfs(expect, format[i], values[0], values[1], values[2], values[3]);
		test_assertVprintfs(expect, format[i], values[0], values[1], values[2], values[3]);
	}
}

///////////////////////////////////////////////////////////////////////////

TEST_SETUP(stdio_printf_cspn)
{
	test_outFile = fopen(PATH, "w+");
}


TEST_TEAR_DOWN(stdio_printf_cspn)
{
	fclose(test_outFile);
	remove(PATH);
}


TEST(stdio_printf_cspn, c)
{
	int i;
	const char *format = "Lorem-ips%cm-dolor";
	const char values[3] = { 'u', 'x', '\0' };
	char *expect[] = { "Lorem-ipsum-dolor", "Lorem-ipsxm-dolor", "Lorem-ips" };

	for (i = 1; i < (sizeof(values) / sizeof(values[0])); i++) {

		test_assertPrintfs(expect[i], format, values[i]);
		test_assertVprintfs(expect[i], format, values[i]);
	}
}


TEST(stdio_printf_cspn, c_ascii_printf)
{
	const char *format = "%c";
	char expect[2] = { 0 };

	for (int i = 1; i < 128; i++) {
		expect[0] = i;

		test_assertPrintfs(expect, format, i);
	}
}


TEST(stdio_printf_cspn, c_ascii_vprintf)
{
	const char *format = "%c";
	char expect[2] = { 0 };

	for (int i = 1; i < 128; i++) {
		expect[0] = i;

		test_assertVprintfs(expect, format, i);
	}
}


TEST(stdio_printf_cspn, c_non_ascii_printf)
{
	const char *format = "%c";
	char expect[2] = { 0 };

	for (int i = 128; i < 256; i++) {
		expect[0] = i;

		test_assertPrintfs(expect, format, i);
	}
}


TEST(stdio_printf_cspn, c_non_ascii_vprintf)
{
	const char *format = "%c";
	char expect[2] = { 0 };

	for (int i = 128; i < 256; i++) {
		expect[0] = i;

		test_assertVprintfs(expect, format, i);
	}
}


TEST(stdio_printf_cspn, lc)
{
	const char *format = "%lc %lc %lc %lc %lc %lc";
	const wchar_t values[] = { L'a', L'A', L'0', L'9', L'!', L';' };
	char *expect = "a A 0 9 ! ;";

	test_assertPrintfs(expect, format, values[0], values[1], values[2], values[3], values[4], values[5]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2], values[3], values[4], values[5]);
}


TEST(stdio_printf_cspn, C)
{
/* Disabled because of #709 issue: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/709 */
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#709 issue");
#endif
	const char *format = "%C %C %C %C %C %C";
	const wchar_t values[] = { L'a', L'A', L'0', L'9', L'!', L';' };
	char *expect = "a A 0 9 ! ;";

	test_assertPrintfs(expect, format, values[0], values[1], values[2], values[3], values[4], values[5]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2], values[3], values[4], values[5]);
}


TEST(stdio_printf_cspn, s)
{
	const char *format = "%s %s %s %s %s %s";
	char *values[] = { "Lorem", "ipsum", "dolor", "sir", "amet", "Ut hendrerit iaculis tempus. Ut eu dapibus ante." };
	char *expect = "Lorem ipsum dolor sir amet Ut hendrerit iaculis tempus. Ut eu dapibus ante.";

	test_assertPrintfs(expect, format, values[0], values[1], values[2], values[3], values[4], values[5]);
	test_assertVprintfs(expect, format, values[0], values[1], values[2], values[3], values[4], values[5]);
}


TEST(stdio_printf_cspn, s_specific)
{
	int i;
	const char *format = "%s";
	const char *values[] = { " ", "hello\0\0world", "hello\0world", "#99\0ns" };
	char *expect[] = { " ", "hello", "hello", "#99" };

	for (i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {

		test_assertPrintfs(expect[i], format, values[i]);
		test_assertVprintfs(expect[i], format, values[i]);
	}
}


TEST(stdio_printf_cspn, s_ascii_printf)
{
	const char *format = "%s";
	char expect[2] = { 0 };

	/*
	 * In ASCII table from number 33 starts printable characters
	 * in this case we want to dodge terminating ASCII signs
	 */
	for (int i = 33; i < 128; i++) {
		expect[0] = i;

		test_assertPrintfs(expect, format, expect);
	}
}


TEST(stdio_printf_cspn, s_ascii_vprintf)
{
	const char *format = "%s";
	char expect[2] = { 0 };

	/*
	 * In ASCII table from number 33 starts printable characters
	 * in this case we want to dodge terminating ASCII signs
	 */
	for (int i = 33; i < 128; i++) {
		expect[0] = i;

		test_assertVprintfs(expect, format, expect);
	}
}


TEST(stdio_printf_cspn, s_huge_string)
{
	const char *format = "%s";
	char *values = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Phasellus hendrerit various orci, ac sollicitudin nunc imperdiet ac. Morbi laoreet, enim eu mollis consequat, leo risus pellentesque arcu, a pulvinar augue magna nec erat. Morbi gravida dui ut lacus mattis, et maximus dolor facilisis cras ame";

	test_assertPrintfs(values, format, values);
	test_assertVprintfs(values, format, values);
}


TEST(stdio_printf_cspn, ls)
{
/* Disabled because of #698 issue: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/698 */
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#698 issue");
#endif
	int i;
	const char *format = "%ls";
	const wchar_t *values[] = { L"Lorem", L"hello\0\0world", L"\4399\0ns", L"Ut hendrerit iaculis tempus. Ut eu dapibus ante." };
	char *expect[] = { "Lorem", "hello", "#99", "Ut hendrerit iaculis tempus. Ut eu dapibus ante." };

	for (i = 1; i < (sizeof(values) / sizeof(values[0])); i++) {

		test_assertPrintfs(expect[i], format, values[i]);
		test_assertVprintfs(expect[i], format, values[i]);
	}
}


TEST(stdio_printf_cspn, S)
{
/* Disabled because of #709 issue: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/709 */
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#709 issue");
#endif
	int i;
	const char *format = "%S";
	const wchar_t *values[] = { L"Lorem", L"hello\0\0world", L"\4399\0ns", L"Ut hendrerit iaculis tempus. Ut eu dapibus ante." };
	char *expect[] = { "Lorem", "hello", "#99", "Ut hendrerit iaculis tempus. Ut eu dapibus ante." };

	for (i = 1; i < (sizeof(values) / sizeof(values[0])); i++) {

		test_assertPrintfs(expect[i], format, values[i]);
		test_assertVprintfs(expect[i], format, values[i]);
	}
}


TEST(stdio_printf_cspn, p)
{
#ifdef __phoenix__
	char *expect = "deadbeef    (nil) 7fffffff 80000000";
#else
	char *expect = "0xdeadbeef (nil) 0x7fffffffffffffff 0x8000000000000000";
#endif

	const char *format = "%p %p %p %p";

	test_assertPrintfs(expect, format, (void *)0xDEADBEEF, (void *)0x00000000, (void *)INTPTR_MAX, (void *)INTPTR_MIN);
	test_assertVprintfs(expect, format, (void *)0xDEADBEEF, (void *)0x00000000, (void *)INTPTR_MAX, (void *)INTPTR_MIN);
}


TEST(stdio_printf_cspn, n)
{
/* Disabled because of #277 issue: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/277 */
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#277 issue");
#endif
	int count = 0;
	const char *format = "Lorem ipsum%n";
	char *expect = "Lorem ipsum";

	test_assertPrintfs(expect, format, &count);
	TEST_ASSERT_EQUAL_INT(strlen(expect), count);
	count = 0;

	test_assertVprintfs(expect, format, &count);
	TEST_ASSERT_EQUAL_INT(strlen(expect), count);
	count = 0;
}


TEST(stdio_printf_cspn, percent)
{
	const char *format = "%% yes %%Lorem%%Ipsum %% Hello";
	char *expect = "% yes %Lorem%Ipsum % Hello";

	test_assertPrintfs(expect, format, 0);
	test_assertVprintfs(expect, format, 0);
}

///////////////////////////////////////////////////////////////////////////

TEST_SETUP(stdio_printf_rest)
{
	test_outFile = fopen(PATH, "w+");
}


TEST_TEAR_DOWN(stdio_printf_rest)
{
	fclose(test_outFile);
	remove(PATH);
}


TEST(stdio_printf_rest, mods_int)
{
	int i;
	const char *format = "%2d %5d %05d %+d %.5d %-10d";
	const int values[] = { INT_MAX, 0, INT_MIN, -123, 123, 123456789 };
	char *expect[] = { "2147483647 2147483647 2147483647 +2147483647 2147483647 2147483647", " 0     0 00000 +0 00000 0         ", "-2147483648 -2147483648 -2147483648 -2147483648 -2147483648 -2147483648", "-123  -123 -0123 -123 -00123 -123      ", "123   123 00123 +123 00123 123       ", "123456789 123456789 123456789 +123456789 123456789 123456789 " };

	for (i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {

		test_assertPrintfs(expect[i], format, values[i], values[i], values[i], values[i], values[i], values[i]);
		test_assertVprintfs(expect[i], format, values[i], values[i], values[i], values[i], values[i], values[i]);
	}
}


TEST(stdio_printf_rest, mods_float)
{
	int i;
	const char *format = "%2f %10f %010f %+f %.5f %-10f";
	const float values[] = { FLT_MAX, 0, FLT_MIN, -1.23, 1.23, 0.23456789 };
	char *expect[] = { "340282346638528859811704183484516925440.000000 340282346638528859811704183484516925440.000000 340282346638528859811704183484516925440.000000 +340282346638528859811704183484516925440.000000 340282346638528859811704183484516925440.00000 340282346638528859811704183484516925440.000000", "0.000000   0.000000 000.000000 +0.000000 0.00000 0.000000  ", "0.000000   0.000000 000.000000 +0.000000 0.00000 0.000000  ", "-1.230000  -1.230000 -01.230000 -1.230000 -1.23000 -1.230000 ", "1.230000   1.230000 001.230000 +1.230000 1.23000 1.230000  ", "0.234568   0.234568 000.234568 +0.234568 0.23457 0.234568  " };

	for (i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {

		test_assertPrintfs(expect[i], format, values[i], values[i], values[i], values[i], values[i], values[i]);
		test_assertVprintfs(expect[i], format, values[i], values[i], values[i], values[i], values[i], values[i]);
	}
}


TEST(stdio_printf_rest, mods_double)
{
	int i;
	const char *format = "%2le %10le %010le %+le %.5le %-10le";
	const double values[] = { DBL_MIN, 0, DBL_MAX };
	char *expect[] = { "2.225074e-308 2.225074e-308 2.225074e-308 +2.225074e-308 2.22507e-308 2.225074e-308", "0.000000e+00 0.000000e+00 0.000000e+00 +0.000000e+00 0.00000e+00 0.000000e+00", "1.797693e+308 1.797693e+308 1.797693e+308 +1.797693e+308 1.79769e+308 1.797693e+308" };

	for (i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {

		test_assertPrintfs(expect[i], format, values[i], values[i], values[i], values[i], values[i], values[i]);
		test_assertVprintfs(expect[i], format, values[i], values[i], values[i], values[i], values[i], values[i]);
	}
}


TEST(stdio_printf_rest, mods_string)
{
	int i;
	const char *format = "%2s %5s %05s %.3s %-10s";
	const char *values[] = { "a", "abcdefghij", "xcb/./32154@#$%$#%^sa" };
	char *expect[] = { " a     a     a a a         ", "abcdefghij abcdefghij abcdefghij abc abcdefghij", "xcb/./32154@#$%$#%^sa xcb/./32154@#$%$#%^sa xcb/./32154@#$%$#%^sa xcb xcb/./32154@#$%$#%^sa" };

	for (i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {

		test_assertPrintfs(expect[i], format, values[i], values[i], values[i], values[i], values[i]);
		test_assertVprintfs(expect[i], format, values[i], values[i], values[i], values[i], values[i]);
	}
}


TEST(stdio_printf_rest, mods_o_x)
{
	int i;
	const char *format = "%#o %.3o %-10o %#x %.3x %-10x";
	const int values[] = { 0, -123, 123, 123456789 };
	char *expect[] = { "0 000 0          0 000 0         ", "037777777605 37777777605 37777777605 0xffffff85 ffffff85 ffffff85  ", "0173 173 173        0x7b 07b 7b        ", "0726746425 726746425 726746425  0x75bcd15 75bcd15 75bcd15   " };

	for (i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {

		test_assertPrintfs(expect[i], format, values[i], values[i], values[i], values[i], values[i], values[i]);
		test_assertVprintfs(expect[i], format, values[i], values[i], values[i], values[i], values[i], values[i]);
	}
}


TEST(stdio_printf_rest, mods_sharp_fega)
{
	/* testing is with %.6A due to the cross-platform nature of the test. On phoenix at %a, there are trailing zeros, which is not a bug */
	int i;
	const char *format = "%#f %#e %#g %#.6a";
	const float values[] = { 0, -123.456, 123.456, FLT_MIN, FLT_MAX };
	char *expect[] = { "0.000000 0.000000e+00 0.00000 0x0.000000p+0", "-123.456001 -1.234560e+02 -123.456 -0x1.edd2f2p+6", "123.456001 1.234560e+02 123.456 0x1.edd2f2p+6", "0.000000 1.175494e-38 1.17549e-38 0x1.000000p-126", "340282346638528859811704183484516925440.000000 3.402823e+38 3.40282e+38 0x1.fffffep+127" };

	for (i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {

		test_assertPrintfs(expect[i], format, values[i], values[i], values[i], values[i]);
		test_assertVprintfs(expect[i], format, values[i], values[i], values[i], values[i]);
	}
}


TEST(stdio_printf_rest, mods_sharp_FEGA)
{
	/* testing is with %.6a due to the cross-platform nature of the test. On phoenix at %a, there are trailing zeros, which is not a bug */
	int i;
	const char *format = "%#F %#E %#G %#.6A";
	const float values[] = { 0, -123.456, 123.456, FLT_MIN, FLT_MAX };
	char *expect[] = { "0.000000 0.000000E+00 0.00000 0X0.000000P+0", "-123.456001 -1.234560E+02 -123.456 -0X1.EDD2F2P+6", "123.456001 1.234560E+02 123.456 0X1.EDD2F2P+6", "0.000000 1.175494E-38 1.17549E-38 0X1.000000P-126", "340282346638528859811704183484516925440.000000 3.402823E+38 3.40282E+38 0X1.FFFFFEP+127" };

	for (i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {

		test_assertPrintfs(expect[i], format, values[i], values[i], values[i], values[i]);
		test_assertVprintfs(expect[i], format, values[i], values[i], values[i], values[i]);
	}
}


TEST(stdio_printf_rest, lmods_zero_int)
{
	int i;
	const char *format = "%0d %04d";
	const int values[] = { 0, -1, 1, -64, 64, 8192 };
	char *expect[] = { "0 0000", "-1 -001", "1 0001", "-64 -064", "64 0064", "8192 8192" };

	for (i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {

		test_assertPrintfs(expect[i], format, values[i], values[i]);
		test_assertVprintfs(expect[i], format, values[i], values[i]);
	}
}


TEST(stdio_printf_rest, lmods_zero_float)
{
	int i;
	const char *format = "%0.2f %04.2f %08.2f";
	const float values[] = { 0, -1, 1, -64.321, 64.321, 98765.43120 };
	char *expect[] = { "0.00 0.00 00000.00", "-1.00 -1.00 -0001.00", "1.00 1.00 00001.00", "-64.32 -64.32 -0064.32", "64.32 64.32 00064.32", "98765.43 98765.43 98765.43" };

	for (i = 0; i < (sizeof(values) / sizeof(values[0])); i++) {

		test_assertPrintfs(expect[i], format, values[i], values[i], values[i]);
		test_assertVprintfs(expect[i], format, values[i], values[i], values[i]);
	}
}


TEST(stdio_printf_rest, numbered_argument)
{
/* Disabled because of #719 issue: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/719 */
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#719 issue");
#endif
	const char *format = "%3$d %2$d %1$d";

	test_assertPrintfs("3 2 1", format, 1 COMMA 2 COMMA 3);
	test_assertVprintfs("3 2 1", format, 1 COMMA 2 COMMA 3);
}


TEST(stdio_printf_rest, snprintf_truncation)
{
/* Intentionally testing string truncation */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"

	memset(test_buff, 0, sizeof(test_buff));
	snprintf(test_buff, 0, "%d", 1234567890);
	TEST_ASSERT_EQUAL_STRING("", test_buff);

	memset(test_buff, 0, sizeof(test_buff));
	snprintf(test_buff, 6, "%d", 1234567890);
	TEST_ASSERT_EQUAL_STRING("12345", test_buff);

	memset(test_buff, 0, sizeof(test_buff));
	snprintf(test_buff, 6, "%f", 1.23456789);
	TEST_ASSERT_EQUAL_STRING("1.234", test_buff);

	memset(test_buff, 0, sizeof(test_buff));
	snprintf(test_buff, 0, "%s", "abcdefighjklmnop");
	TEST_ASSERT_EQUAL_STRING("", test_buff);

	memset(test_buff, 0, sizeof(test_buff));
	snprintf(test_buff, 6, "%s", "abcdefighjklmnop");
	TEST_ASSERT_EQUAL_STRING("abcde", test_buff);

	memset(test_buff, 0, sizeof(test_buff));
	snprintf(test_buff, 3, "%6s", "abc");
	TEST_ASSERT_EQUAL_STRING("  ", test_buff);

	memset(test_buff, 0, sizeof(test_buff));
	snprintf(test_buff, 6, "%6s", "abc");
	TEST_ASSERT_EQUAL_STRING("   ab", test_buff);

	memset(test_buff, 0, sizeof(test_buff));
	snprintf(test_buff, 7, "%6s", "abc");
	TEST_ASSERT_EQUAL_STRING("   abc", test_buff);

#pragma GCC diagnostic pop
}


TEST(stdio_printf_rest, errnos)
{
	/*
	 * EILSEQ not tested, because locales not supported on PHOENIX
	 * EBADF not tested, because then memory leaks occurs
	 */

/* Intentionally testing overflow situations */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-overflow"

	errno = 0;
	fprintf(test_outFile, "%.1000000000000000000000000000000lf %.100000000000000000000lf", DBL_MAX, DBL_MAX);
	TEST_ASSERT_TRUE(errno == ENOMEM || errno == EOVERFLOW);

#pragma GCC diagnostic pop
}

//////////////////////////////////////////////////////////////////////////

TEST_GROUP_RUNNER(stdio_printf_d)
{
	RUN_TEST_CASE(stdio_printf_d, d);
	RUN_TEST_CASE(stdio_printf_d, hhd);
	RUN_TEST_CASE(stdio_printf_d, hd);
	RUN_TEST_CASE(stdio_printf_d, ld);
	RUN_TEST_CASE(stdio_printf_d, lld);
	RUN_TEST_CASE(stdio_printf_d, jd);
	RUN_TEST_CASE(stdio_printf_d, zd);
	RUN_TEST_CASE(stdio_printf_d, td);
	RUN_TEST_CASE(stdio_printf_d, out_of_bonds);
}


TEST_GROUP_RUNNER(stdio_printf_i)
{
	RUN_TEST_CASE(stdio_printf_i, i);
	RUN_TEST_CASE(stdio_printf_i, hhi);
	RUN_TEST_CASE(stdio_printf_i, hi);
	RUN_TEST_CASE(stdio_printf_i, li);
	RUN_TEST_CASE(stdio_printf_i, lli);
	RUN_TEST_CASE(stdio_printf_i, ji);
	RUN_TEST_CASE(stdio_printf_i, zi);
	RUN_TEST_CASE(stdio_printf_i, ti);
	RUN_TEST_CASE(stdio_printf_i, out_of_bonds);
}


TEST_GROUP_RUNNER(stdio_printf_o)
{
	RUN_TEST_CASE(stdio_printf_o, o);
	RUN_TEST_CASE(stdio_printf_o, hho);
	RUN_TEST_CASE(stdio_printf_o, ho);
	RUN_TEST_CASE(stdio_printf_o, lo);
	RUN_TEST_CASE(stdio_printf_o, llo);
	RUN_TEST_CASE(stdio_printf_o, jo);
	RUN_TEST_CASE(stdio_printf_o, zo);
	RUN_TEST_CASE(stdio_printf_o, to);
	RUN_TEST_CASE(stdio_printf_o, out_of_bonds);
}


TEST_GROUP_RUNNER(stdio_printf_u)
{
	RUN_TEST_CASE(stdio_printf_u, u);
	RUN_TEST_CASE(stdio_printf_u, hhu);
	RUN_TEST_CASE(stdio_printf_u, hu);
	RUN_TEST_CASE(stdio_printf_u, lu);
	RUN_TEST_CASE(stdio_printf_u, llu);
	RUN_TEST_CASE(stdio_printf_u, ju);
	RUN_TEST_CASE(stdio_printf_u, zu);
	RUN_TEST_CASE(stdio_printf_u, tu);
	RUN_TEST_CASE(stdio_printf_u, out_of_bonds);
}


TEST_GROUP_RUNNER(stdio_printf_x)
{
	RUN_TEST_CASE(stdio_printf_x, x);
	RUN_TEST_CASE(stdio_printf_x, hhx);
	RUN_TEST_CASE(stdio_printf_x, hx);
	RUN_TEST_CASE(stdio_printf_x, lx);
	RUN_TEST_CASE(stdio_printf_x, llx);
	RUN_TEST_CASE(stdio_printf_x, jx);
	RUN_TEST_CASE(stdio_printf_x, zx);
	RUN_TEST_CASE(stdio_printf_x, tx);
	RUN_TEST_CASE(stdio_printf_x, x_out_of_bonds);

	RUN_TEST_CASE(stdio_printf_x, X);
	RUN_TEST_CASE(stdio_printf_x, hhX);
	RUN_TEST_CASE(stdio_printf_x, hX);
	RUN_TEST_CASE(stdio_printf_x, lX);
	RUN_TEST_CASE(stdio_printf_x, llX);
	RUN_TEST_CASE(stdio_printf_x, jX);
	RUN_TEST_CASE(stdio_printf_x, zX);
	RUN_TEST_CASE(stdio_printf_x, tX);
	RUN_TEST_CASE(stdio_printf_x, X_out_of_bonds);
}


TEST_GROUP_RUNNER(stdio_printf_fega)
{
	RUN_TEST_CASE(stdio_printf_fega, f);
	RUN_TEST_CASE(stdio_printf_fega, lf);
	RUN_TEST_CASE(stdio_printf_fega, Lf);
	RUN_TEST_CASE(stdio_printf_fega, F);
	RUN_TEST_CASE(stdio_printf_fega, lF);
	RUN_TEST_CASE(stdio_printf_fega, LF);

	RUN_TEST_CASE(stdio_printf_fega, e);
	RUN_TEST_CASE(stdio_printf_fega, le);
	RUN_TEST_CASE(stdio_printf_fega, Le);
	RUN_TEST_CASE(stdio_printf_fega, E);
	RUN_TEST_CASE(stdio_printf_fega, lE);
	RUN_TEST_CASE(stdio_printf_fega, LE);

	RUN_TEST_CASE(stdio_printf_fega, g);
	RUN_TEST_CASE(stdio_printf_fega, lg);
	RUN_TEST_CASE(stdio_printf_fega, Lg);
	RUN_TEST_CASE(stdio_printf_fega, G);
	RUN_TEST_CASE(stdio_printf_fega, lG);
	RUN_TEST_CASE(stdio_printf_fega, LG);

	RUN_TEST_CASE(stdio_printf_fega, a);
	RUN_TEST_CASE(stdio_printf_fega, la);
	RUN_TEST_CASE(stdio_printf_fega, La);
	RUN_TEST_CASE(stdio_printf_fega, A);
	RUN_TEST_CASE(stdio_printf_fega, lA);
	RUN_TEST_CASE(stdio_printf_fega, LA);

	RUN_TEST_CASE(stdio_printf_fega, fega_inf_nan);
	RUN_TEST_CASE(stdio_printf_fega, lfega_inf_nan);
	RUN_TEST_CASE(stdio_printf_fega, Lfega_inf_nan);

	RUN_TEST_CASE(stdio_printf_fega, FEGA_inf_nan);
	RUN_TEST_CASE(stdio_printf_fega, lFEGA_inf_nan);
	RUN_TEST_CASE(stdio_printf_fega, LFEGA_inf_nan);
}


TEST_GROUP_RUNNER(stdio_printf_cspn)
{
	RUN_TEST_CASE(stdio_printf_cspn, c);
	RUN_TEST_CASE(stdio_printf_cspn, c_ascii_printf);
	RUN_TEST_CASE(stdio_printf_cspn, c_ascii_vprintf);
	RUN_TEST_CASE(stdio_printf_cspn, c_non_ascii_printf);
	RUN_TEST_CASE(stdio_printf_cspn, c_non_ascii_vprintf);
	RUN_TEST_CASE(stdio_printf_cspn, lc);
	RUN_TEST_CASE(stdio_printf_cspn, C);
	RUN_TEST_CASE(stdio_printf_cspn, s);

	RUN_TEST_CASE(stdio_printf_cspn, s_specific);
	RUN_TEST_CASE(stdio_printf_cspn, s_ascii_printf);
	RUN_TEST_CASE(stdio_printf_cspn, s_ascii_vprintf);
	RUN_TEST_CASE(stdio_printf_cspn, s_huge_string);
	RUN_TEST_CASE(stdio_printf_cspn, ls);
	RUN_TEST_CASE(stdio_printf_cspn, S);

	RUN_TEST_CASE(stdio_printf_cspn, p);
	RUN_TEST_CASE(stdio_printf_cspn, n);
	RUN_TEST_CASE(stdio_printf_cspn, percent);
}


TEST_GROUP_RUNNER(stdio_printf_rest)
{
	RUN_TEST_CASE(stdio_printf_rest, mods_int);
	RUN_TEST_CASE(stdio_printf_rest, mods_float);
	RUN_TEST_CASE(stdio_printf_rest, mods_double);
	RUN_TEST_CASE(stdio_printf_rest, mods_string);
	RUN_TEST_CASE(stdio_printf_rest, mods_o_x);
	RUN_TEST_CASE(stdio_printf_rest, mods_sharp_fega);
	RUN_TEST_CASE(stdio_printf_rest, mods_sharp_FEGA);

	RUN_TEST_CASE(stdio_printf_rest, lmods_zero_int);
	RUN_TEST_CASE(stdio_printf_rest, lmods_zero_float);
	RUN_TEST_CASE(stdio_printf_rest, numbered_argument);

	RUN_TEST_CASE(stdio_printf_rest, snprintf_truncation);
	RUN_TEST_CASE(stdio_printf_rest, errnos);
}


void runner(void)
{
	RUN_TEST_GROUP(stdio_printf_d);
	RUN_TEST_GROUP(stdio_printf_i);
	RUN_TEST_GROUP(stdio_printf_u);
	RUN_TEST_GROUP(stdio_printf_o);
	RUN_TEST_GROUP(stdio_printf_x);
	RUN_TEST_GROUP(stdio_printf_fega);
	RUN_TEST_GROUP(stdio_printf_cspn);
	RUN_TEST_GROUP(stdio_printf_rest);
}


int main(int argc, char *argv[])
{
	UnityMain(argc, (const char **)argv, runner);

	return 0;
}
