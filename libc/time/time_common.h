/*
 * Phoenix-RTOS
 *
 * test-libc-time
 *
 * Common utility functions for time.h tests.
 *
 * Copyright 2023 Phoenix Systems
 * Author: Jacek Maksymowicz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _TEST_LIBC_TIME_COMMON_H
#define _TEST_LIBC_TIME_COMMON_H

#include <stdlib.h>
#include <time.h>

#include "unity_fixture.h"

static inline void tm_to_str(struct tm *t, char *buff)
{
	sprintf(
		buff,
		"tm_sec %d, tm_min %d, tm_hour %d, tm_mday %d\n"
		"tm_mon %d tm_year %d tm_wday %d tm_yday %d tm_isdst %d",
		t->tm_sec,
		t->tm_min,
		t->tm_hour,
		t->tm_mday,
		t->tm_mon,
		t->tm_year,
		t->tm_wday,
		t->tm_yday,
		t->tm_isdst);
}


static inline void init_tm(struct tm *t, const int *input)
{
	t->tm_sec = input[0];
	t->tm_min = input[1];
	t->tm_hour = input[2];
	t->tm_mday = input[3];
	t->tm_mon = input[4];
	t->tm_year = input[5];
	t->tm_wday = input[6];
	t->tm_yday = input[7];
	t->tm_isdst = input[8];
}


static inline void struct_tm_assert_equal(struct tm *expected, struct tm *got)
{
	TEST_ASSERT_EQUAL_MESSAGE(expected->tm_sec, got->tm_sec, "tm_sec");
	TEST_ASSERT_EQUAL_MESSAGE(expected->tm_min, got->tm_min, "tm_min");
	TEST_ASSERT_EQUAL_MESSAGE(expected->tm_hour, got->tm_hour, "tm_hour");
	TEST_ASSERT_EQUAL_MESSAGE(expected->tm_mday, got->tm_mday, "tm_mday");
	TEST_ASSERT_EQUAL_MESSAGE(expected->tm_mon, got->tm_mon, "tm_mon");
	TEST_ASSERT_EQUAL_MESSAGE(expected->tm_year, got->tm_year, "tm_year");
	TEST_ASSERT_EQUAL_MESSAGE(expected->tm_wday, got->tm_wday, "tm_wday");
	TEST_ASSERT_EQUAL_MESSAGE(expected->tm_yday, got->tm_yday, "tm_yday");
	TEST_ASSERT_EQUAL_MESSAGE(expected->tm_isdst, got->tm_isdst, "tm_isdst");
}

#endif /* _TEST_LIBC_TIME_COMMON_H */
