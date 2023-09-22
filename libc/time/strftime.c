/*
 * Phoenix-RTOS
 *
 * test-libc-time
 *
 * Tests of strftime function
 *
 * Copyright 2020 Phoenix Systems
 * Author: Marcin Brzykcy
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "time_common.h"


#define BUFF_LEN 35

struct test_data {
	const struct tm *t;
	const char format[BUFF_LEN];
	size_t n;
	const char output[BUFF_LEN];
	/* if ret = 0 expect 0 at output, otherwise expect strlen(output) */
	int ret;
};

#define BASIC_FORMATTING_LEN        9
#define ADDITIONAL_FORMAT_CHARS_LEN 19
#define FORMAT_WITH_PADDING_LEN     25

static const struct test_data basic_formatting[BASIC_FORMATTING_LEN];
static const struct test_data additional_format_chars[ADDITIONAL_FORMAT_CHARS_LEN];
static const struct test_data format_with_padding[FORMAT_WITH_PADDING_LEN];

#define SIZE_OF_TABLE(x) (sizeof(x) / sizeof(x[0]))


TEST_GROUP(time_strftime);


TEST_SETUP(time_strftime)
{
	tzset();
}


TEST_TEAR_DOWN(time_strftime)
{
}


static void strftime_assert(const struct test_data *data)
{
	char buff[BUFF_LEN];
	int ret = strftime(buff, data->n, data->format, data->t);
	if (ret == 0 && data->ret == 0) {
		return;
	}

	TEST_ASSERT_EQUAL_STRING(data->output, buff);
	TEST_ASSERT_EQUAL_INT_MESSAGE(strlen(data->output), ret, "Incorrect output length returned");
}


TEST(time_strftime, basic_formatting)
{
	for (int i = 0; i < SIZE_OF_TABLE(basic_formatting); i++) {
		strftime_assert(&basic_formatting[i]);
	}
}


TEST(time_strftime, additional_format_chars)
{
/* Disabled because of #351 issue: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/351 (point 1) */
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#351 issue");
#endif
	for (int i = 0; i < SIZE_OF_TABLE(additional_format_chars); i++) {
		strftime_assert(&additional_format_chars[i]);
	}
}

TEST(time_strftime, format_with_padding)
{
/* Disabled because of #351 issue: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/351 (point 2) */
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#351 issue");
#endif
	for (int i = 0; i < SIZE_OF_TABLE(format_with_padding); i++) {
		strftime_assert(&format_with_padding[i]);
	}
}


TEST_GROUP_RUNNER(time_strftime)
{
	RUN_TEST_CASE(time_strftime, basic_formatting);
	RUN_TEST_CASE(time_strftime, additional_format_chars);
	RUN_TEST_CASE(time_strftime, format_with_padding);
}

static const struct tm t1 = { .tm_sec = 1, .tm_min = 1, .tm_hour = 6, .tm_mday = 2, .tm_mon = 2, .tm_year = 111, .tm_wday = 0, .tm_yday = 2, .tm_isdst = 0 };
static const struct tm t2 = { .tm_sec = 11, .tm_min = 12, .tm_hour = 13, .tm_mday = 23, .tm_mon = 11, .tm_year = 95, .tm_wday = 6, .tm_yday = 235, .tm_isdst = 0 };
static const struct tm t3 = { .tm_sec = 11, .tm_min = 12, .tm_hour = 13, .tm_mday = 23, .tm_mon = 11, .tm_year = 105, .tm_wday = 1, .tm_yday = 235, .tm_isdst = 0 };
static const struct tm t4 = { .tm_sec = 0, .tm_min = 0, .tm_hour = 0, .tm_mday = 2, .tm_mon = 0, .tm_year = 99, .tm_wday = 6, .tm_yday = 2, .tm_isdst = 0 };

static const struct test_data basic_formatting[] = {
	{ .t = &t1, .format = "%A", .n = 5, .output = "", .ret = 0 },
	{ .t = &t1, .format = "%A", .n = 6, .output = "", .ret = 0 },
	{ .t = &t1, .format = "%A", .n = 7, .output = "Sunday", .ret = 1 },
	{ .t = &t1, .format = "%a %A %b %B", .n = BUFF_LEN, .output = "Sun Sunday Mar March", .ret = 1 },
	{ .t = &t1, .format = "lorem ipsum %a", .n = BUFF_LEN, .output = "lorem ipsum Sun", .ret = 1 },
	{ .t = &t1, .format = "%i %a", .n = BUFF_LEN, .output = "%i Sun", .ret = 1 },
	{ .t = &t1, .format = "lorem %i ips%aum", .n = BUFF_LEN, .output = "lorem %i ipsSunum", .ret = 1 },
	{ .t = &t1, .format = "%Y %y | %B %b %m | %d %e", .n = BUFF_LEN, .output = "2011 11 | March Mar 03 | 02  2", .ret = 1 },
	{ .t = &t1, .format = "%A %a %w | %j | %H:%M:%S", .n = BUFF_LEN, .output = "Sunday Sun 0 | 003 | 06:01:01", .ret = 1 },
};

static const struct test_data additional_format_chars[] = {
	{ .t = &t1, .format = "%c", .n = BUFF_LEN, .output = "Sun Mar  2 06:01:01 2011", .ret = 1 },
	{ .t = &t2, .format = "%C", .n = BUFF_LEN, .output = "19", .ret = 1 },
	{ .t = &t1, .format = "%h", .n = BUFF_LEN, .output = "Mar", .ret = 1 },
	{ .t = &t4, .format = "%D", .n = BUFF_LEN, .output = "01/02/99", .ret = 1 },
	{ .t = &t4, .format = "%F", .n = BUFF_LEN, .output = "1999-01-02", .ret = 1 },
	{ .t = &t4, .format = "%h", .n = BUFF_LEN, .output = "Jan", .ret = 1 },
	{ .t = &t4, .format = "%I", .n = BUFF_LEN, .output = "12", .ret = 1 },
	{ .t = &t4, .format = "%n", .n = BUFF_LEN, .output = "\n", .ret = 1 },
	{ .t = &t4, .format = "%p", .n = BUFF_LEN, .output = "AM", .ret = 1 },
	{ .t = &t4, .format = "%R", .n = BUFF_LEN, .output = "00:00", .ret = 1 },
	{ .t = &t4, .format = "%r", .n = BUFF_LEN, .output = "12:00:00 AM", .ret = 1 },
	{ .t = &t4, .format = "%T", .n = BUFF_LEN, .output = "00:00:00", .ret = 1 },
	{ .t = &t4, .format = "%t", .n = BUFF_LEN, .output = "\t", .ret = 1 },
	{ .t = &t4, .format = "%u", .n = BUFF_LEN, .output = "6", .ret = 1 },
	{ .t = &t4, .format = "%U", .n = BUFF_LEN, .output = "00", .ret = 1 },
	{ .t = &t4, .format = "%W", .n = BUFF_LEN, .output = "00", .ret = 1 },
	{ .t = &t4, .format = "%x", .n = BUFF_LEN, .output = "01/02/99", .ret = 1 },
	{ .t = &t4, .format = "%X", .n = BUFF_LEN, .output = "00:00:00", .ret = 1 },
	{ .t = &t4, .format = "%z", .n = BUFF_LEN, .output = "+0000", .ret = 1 },
};

static const struct test_data format_with_padding[] = {
	{ .t = &t2, .format = "%6b%12B", .n = BUFF_LEN, .output = "   Dec    December", .ret = 1 },
	{ .t = &t2, .format = "%6a%12A", .n = BUFF_LEN, .output = "   Sat    Saturday", .ret = 1 },
	{ .t = &t1, .format = "%C %6C %07C %1C", .n = BUFF_LEN, .output = "20 000020 0000020 20", .ret = 1 },
	{ .t = &t2, .format = "%C %6C %07C", .n = BUFF_LEN, .output = "19 000019 0000019", .ret = 1 },
	{ .t = &t3, .format = "%6d %3d %2d %d", .n = BUFF_LEN, .output = "000023 023 23 23", .ret = 1 },
	{ .t = &t1, .format = "%6d %3d %2d %d", .n = BUFF_LEN, .output = "000002 002 02 02", .ret = 1 },
	{ .t = &t1, .format = "%D %12D %012D", .n = BUFF_LEN, .output = "03/02/11     03/02/11 000003/02/11", .ret = 1 },
	{ .t = &t1, .format = "%e %6e %06e %1e", .n = BUFF_LEN, .output = " 2      2 000002  2", .ret = 1 },
	{ .t = &t2, .format = "%e %6e %06e %1e", .n = BUFF_LEN, .output = "23     23 000023 23", .ret = 1 },
	{ .t = &t2, .format = "%5F %15F", .n = BUFF_LEN, .output = "1995-12-23      1995-12-23", .ret = 1 },
	{ .t = &t2, .format = "%F %015F", .n = BUFF_LEN, .output = "1995-12-23 000001995-12-23", .ret = 1 },
	{ .t = &t1, .format = "%g %05g %G %07G", .n = BUFF_LEN, .output = "10 00010 2010 0002010", .ret = 1 },
	{ .t = &t2, .format = "%H %04H %I %04I", .n = BUFF_LEN, .output = "13 0013 01 0001", .ret = 1 },
	{ .t = &t2, .format = "%M %05M %p", .n = BUFF_LEN, .output = "12 00012 PM", .ret = 1 },
	{ .t = &t1, .format = "%M %05M %p", .n = BUFF_LEN, .output = "01 00001 AM", .ret = 1 },
	{ .t = &t1, .format = "%p%5p %r", .n = BUFF_LEN, .output = "AM   AM 06:01:01 AM", .ret = 1 },
	{ .t = &t2, .format = "%p%5p %r", .n = BUFF_LEN, .output = "PM   PM 01:12:11 PM", .ret = 1 },
	{ .t = &t2, .format = "%20r", .n = BUFF_LEN, .output = "         01:12:11 PM", .ret = 1 },
	{ .t = &t2, .format = "%g %05g %G %07G", .n = BUFF_LEN, .output = "95 00095 1995 0001995", .ret = 1 },
	{ .t = &t2, .format = "%R %R %S %05S", .n = BUFF_LEN, .output = "13:12 13:12 11 00011", .ret = 1 },
	{ .t = &t2, .format = "%T %15T", .n = BUFF_LEN, .output = "13:12:11        13:12:11", .ret = 1 },
	{ .t = &t2, .format = "%u %05u %w %05w", .n = BUFF_LEN, .output = "6 00006 6 00006", .ret = 1 },
	{ .t = &t2, .format = "%U %05U %W %05W", .n = BUFF_LEN, .output = "33 00033 33 00033", .ret = 1 },
	{ .t = &t3, .format = "%u %05u %U %5U %V %5V", .n = BUFF_LEN, .output = "1 00001 34 00034 35 00035", .ret = 1 },
	{ .t = &t2, .format = "%y %05y %Y %05Y", .n = BUFF_LEN, .output = "95 00095 1995 01995", .ret = 1 },
};
