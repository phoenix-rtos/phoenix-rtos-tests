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

static const struct test_data basic_formatting[BASIC_FORMATTING_LEN];

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


TEST_GROUP_RUNNER(time_strftime)
{
	RUN_TEST_CASE(time_strftime, basic_formatting);
}

static const struct tm t1 = { .tm_sec = 1, .tm_min = 1, .tm_hour = 6, .tm_mday = 2, .tm_mon = 2, .tm_year = 111, .tm_wday = 0, .tm_yday = 2, .tm_isdst = 0 };

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
