/*
 * Phoenix-RTOS
 *
 * libc-tests
 *
 * Helpers for common steps during unit testing.
 *
 * Copyright 2021 Phoenix Systems
 * Author: Marek Bialowas
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _TEST_LIBC_COMMON_H
#define _TEST_LIBC_COMMON_H

#include <stdio.h>
#include <stddef.h>
#include <errno.h>
#include <sys/stat.h>

/*
 * Most of the helpers are macros (or at least surrounded by macros)
 * because we want real file:line when test fails.
*/

/* checks and frees char* returned by _fun() */
#define check_and_free_str(_expected_str, _fun) \
	do { \
		char *_res = _fun; \
		TEST_ASSERT_EQUAL_STRING(_expected_str, _res); \
		free(_res); \
	} while (0)


/* checks if _fun() == NULL and verifies resulting errno value */
#define check_null_and_errno(_expected_errno, _fun) \
	do { \
		char *_res = _fun; \
		TEST_ASSERT_NULL(_res); \
		TEST_ASSERT_EQUAL_INT(_expected_errno, errno); \
	} while (0)


/**** FS UTILS ****/


/* crete file with optional data (may be NULL) */
int _create_file(const char *path, const char *data);
#define create_file(_path, _char_data) \
	do { \
		TEST_ASSERT_EQUAL_INT(0, _create_file(_path, _char_data)); \
	} while (0)


/* verify that the file contents is as expected */
int _read_file(const char *path, char *buf, size_t bufsz);
#define check_file_contents(_expected_str, _path) \
	do { \
		char _tmp_buf[128]; \
		int ret = _read_file(_path, _tmp_buf, sizeof(_tmp_buf) - 1); \
		TEST_ASSERT_EQUAL_INT(strlen(_expected_str), ret); \
		_tmp_buf[ret] = 0; \
		TEST_ASSERT_EQUAL_STRING(_expected_str, _tmp_buf); \
	} while (0)


#define check_file_open_errno(_expected_errno, _path) \
	do { \
		int ret = _read_file(_path, NULL, 0); \
		TEST_ASSERT_EQUAL_INT(-1, ret); \
		TEST_ASSERT_EQUAL_INT(_expected_errno, errno); \
	} while (0)


#endif /* _TEST_LIBC_COMMON_H */
