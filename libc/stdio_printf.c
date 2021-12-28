/*
 * Phoenix-RTOS
 *
 * libc-tests
 *
 * libphoenix fprintf, snprintf, sprintf formatting tests
 * 
 * Copyright 2021 Phoenix Systems
 * Author: Mateusz Niewiadomski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>

#include <unity_fixture.h>


static FILE *wr, *rd;
static char buf[50];
static char stdpath[] = "stdio_printf_test";
static int i;

/* comma delimiter macro ',' for multiple arguments parsing to *printf() function via macros */
#define COMMA ,

/* macro-assertion of fprintf formatting */
#define ftest(_format, _value, _expect) \
	{ \
		rewind(wr); \
		rewind(rd); \
		TEST_ASSERT_EQUAL_INT(strlen(_expect), fprintf(wr, _format, _value)); \
		fputc('\n', wr); \
		fflush(wr); \
		fgets(buf, strlen(_expect) + 1, rd); \
		TEST_ASSERT_EQUAL_STRING(_expect, buf); \
	}

/* macro-assertion of sprintf formatting */
#define stest(_format, _value, _expect) \
	{ \
		memset(buf, 0, sizeof(buf)); \
		TEST_ASSERT_EQUAL_INT(strlen(_expect), sprintf(buf, _format, _value)); \
		TEST_ASSERT_EQUAL_STRING(_expect, buf); \
	}

#define sntest(_snret, _snlen, _format, _value, _expect) \
	{ \
		memset(buf, 0, sizeof(buf)); \
		TEST_ASSERT_EQUAL_INT(_snret, snprintf(buf, _snlen, _format, _value)); \
		TEST_ASSERT_EQUAL_STRING(_expect, buf); \
	}


/* group asserting pure usage of type specifiers */
TEST_GROUP(fprintf_formatting);

TEST_SETUP(fprintf_formatting)
{
	wr = fopen(stdpath, "w");
	rd = fopen(stdpath, "r");
}


TEST_TEAR_DOWN(fprintf_formatting)
{
	fclose(wr);
	fclose(rd);
	remove(stdpath);
}


TEST(fprintf_formatting, format_int)
{
	/* integers */
	ftest("%d", 123, "123");
	ftest("%ld", (long)123, "123");
	ftest("%i", 123, "123");
	ftest("%o", 123, "173");
	ftest("%x", 123, "7b");
	ftest("%X", 123, "7B");
}


TEST(fprintf_formatting, format_float)
{
	/* floating point */
	ftest("%f", 392.69, "392.690002"); /* single precision float test */

	ftest("%g", 392.69, "392.69");

	TEST_IGNORE();
	/* <posix incompliant> scientific notation not supported */
	ftest("%e", 392.69, "3.9269e+2");
	ftest("%E", 392.69, "3.9269E+2");
	ftest("%g", 392.69444444444, "3.9269e+2");
	ftest("%G", 392.69444444444, "3.9269e+2");

	/* <posix incompliant> hex floating point not supported */
	ftest("%a", 392.69, "0x1.88b0a3d70a3d7p+8");
	ftest("%A", 392.69, "0X1.88B0A3D70A3D7P+8");

	/* <posix incompliant> can`t print floats bigger than 2^20 */
	ftest("%f", 1048577.F, "1048577.000000"); /* 1048577 = 2^20 + 1 */
	ftest("%Lf", (long double)1.79769313486231571e+308, "179769313486231570814527423731704356798070567525844996598917476803157260780028538760589558632766878171540458953514382464234321326889464182768467546703537516986049910576551282076245490090389328944075868508455133942304583236903222948165808559332123348274797826204144723168738177180919299881250404026184124858368.000000");
}


TEST(fprintf_formatting, format_char)
{
	/* character(s) and pointers */
	ftest("%c", 'a', "a");
	ftest("%s", stdpath, stdpath);
}


TEST(fprintf_formatting, format_specs)
{
	ftest("%p", (void *)(0x12ab34cd), "12ab34cd"); /* pointer */
	ftest("%% %c", 'a', "% a");                    /* %% specifier should print just '%' sign */
}

TEST(fprintf_formatting, format_n)
{
	/* <posix incompliant> %n format specifier does not work */
	TEST_IGNORE();

	ftest("abcd%nefgh", &i, "abcdefgh"); /* %n specifier */
}

TEST(fprintf_formatting, numbered_argument)
{
	/* <posix incompliant> numbered_argument formatting does not work */
	TEST_IGNORE();

	ftest("%1$d %2$d %3$d", 1 COMMA 2 COMMA 3, "3 2 1");
}


TEST_GROUP(sprintf_formatting);

TEST_SETUP(sprintf_formatting)
{ /* empty */
}


TEST_TEAR_DOWN(sprintf_formatting)
{ /* empty */
}


TEST(sprintf_formatting, mod_width)
{
	/* check for space/zero padding and trucation */
	stest("%5d", 123, "  123");
	stest("%05d", 123, "00123");
	stest("%5d", 1234567890, "1234567890");

	stest("%10f", 1.23, "  1.230000");
	stest("%010f", 1.23, "001.230000");

	stest("%5s", "ab", "   ab");

	/* width specified as additional argument */
	stest("%*d", 5 COMMA 123, "  123");

	TEST_IGNORE();
	/* <posix incompliance> justification does not work for chars */
	stest("%5c", 'a', "    a");

	/* <posix incompliance> truncates string */
	stest("%5s", "abcdefgh", "abcdefgh");
}


TEST(sprintf_formatting, mod_flags_ljust)
{
	stest("%-5s", "ab", "ab   ");
	stest("%-*s", 5 COMMA "ab", "ab   ");

	TEST_IGNORE();
	/* <posix incompliance> left justyfication works only for strings */
	stest("%-10f", 1.23, "1.230000  ");
	stest("%-5c", 'a', "a    ");
	stest("%-5d", 123, "123  ");

	/* <posix incompliance> justyfication does not work for chars */
	stest("%-5c", 'a', "a    ");
}

TEST(sprintf_formatting, mod_flags_sign)
{
	stest("%+d", -123, "-123");
	stest("%+d", 123, "+123");
	stest("%+f", -1.23, "-1.230000");

	TEST_IGNORE();
	/* <posix incompliance> sign modifier does not work for positive floats */
	stest("%+f", 1.23, "+1.230000");
}


TEST(sprintf_formatting, mod_flags_hash)
{
	stest("%#o", 123, "0173");
	stest("%#x", 123, "0x7b");
}


TEST(sprintf_formatting, mod_flag_precision)
{
	stest("%.5d", 12345678, "12345678");
	stest("%.5o", 12345678, "57060516");
	stest("%.5x", 12345678, "bc614e");

	stest("%.3f", 1.2345678, "1.235");
	stest("%.3f", 1.4, "1.400");

	stest("%.3s", "a", "a");
	stest("%.3s", "abcdefgh", "abc");

	/* precision specified as additional argument */
	stest("%.*f", 3 COMMA 1.2345678, "1.235");

	TEST_IGNORE();
	/* <posix incompliance> precision modifier does not work for shorter integers */
	stest("%.5d", 123, "00123");
	stest("%.5o", 123, "00173");
	stest("%.5x", 123, "0007b");
}


TEST(sprintf_formatting, specifiers)
{
	stest("%ld", (long int)0x80000000, "-2147483648");
	stest("%lu", (long unsigned int)0x80000000, "2147483648");

	stest("%lld", (long long int)0x8000000000000000, "-9223372036854775808");
	stest("%llu", (long long unsigned int)0x8000000000000000, "9223372036854775808");

	stest("%jd", (intmax_t)(0x8000000000000000), "-9223372036854775808");
	stest("%zd", (size_t)(0x80000000), "-2147483648");

	TEST_IGNORE();
	/* <posix incompliance>  prints format string */

	stest("%hhd", 2147483520, "-128");
	stest("%hhu", 2147483520, "128");

	stest("%hd", 2147450880, "-32768");
	stest("%hu", 2147450880, "32768");

	/* FIXME: make it use long double precision number, not dummy one */
	stest("%Lf", (long double)1.0, "1.0");

	stest("%td", (ptrdiff_t)(0x80000000), "2147483648");
}


TEST_GROUP(snprintf_formatting);

TEST_SETUP(snprintf_formatting)
{ /* empty */
}


TEST_TEAR_DOWN(snprintf_formatting)
{ /* empty */
}

TEST(snprintf_formatting, snprintf_basic)
{
/* 
		NOTE: we are disabling truncation warning for this test as we are doing it deliberately!	
	*/
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"

	/* snprintf() integer truncation */
	sntest(3, 4, "%d", 123, "123");
	sntest(6, 4, "%d", 123456, "123");

	/* snprintf() string truncation */
	sntest(3, 4, "%s", "abc", "abc");
	sntest(8, 4, "%s", "abcdefgh", "abc");

	/* snprintf() return value tests */
	sntest(5, 3, "%5s", "ab", "  ");
	sntest(5, 5, "%5s", "ab", "   a");
	sntest(5, 10, "%5s", "ab", "   ab");

#pragma GCC diagnostic pop
}


TEST_GROUP_RUNNER(stdio_format)
{
	RUN_TEST_CASE(fprintf_formatting, format_int);
	RUN_TEST_CASE(fprintf_formatting, format_float);
	RUN_TEST_CASE(fprintf_formatting, format_char);
	RUN_TEST_CASE(fprintf_formatting, format_specs);
	RUN_TEST_CASE(fprintf_formatting, format_n);
	RUN_TEST_CASE(fprintf_formatting, numbered_argument);

	RUN_TEST_CASE(sprintf_formatting, mod_width);
	RUN_TEST_CASE(sprintf_formatting, mod_flags_ljust);
	RUN_TEST_CASE(sprintf_formatting, mod_flags_sign);
	RUN_TEST_CASE(sprintf_formatting, mod_flags_hash);
	RUN_TEST_CASE(sprintf_formatting, mod_flag_precision);
	RUN_TEST_CASE(sprintf_formatting, specifiers);

	RUN_TEST_CASE(snprintf_formatting, snprintf_basic);
}
