/*
 * Phoenix-RTOS
 *
 * libc/posixsrv
 *
 * tests for tmpfile()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Julian Uziembło
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "unity_fixture.h"

#define ARRAYLEN(arr) (sizeof(arr) / sizeof(arr[0]))


static FILE *filep, *fileps[12];
static char buf[1 << ARRAYLEN(fileps)];


static void assert_fclosed(FILE **fp)
{
	int ret = fclose(*fp);
	*fp = NULL;
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST_GROUP(tmpfile);


TEST_SETUP(tmpfile)
{
}


TEST_TEAR_DOWN(tmpfile)
{
	if (filep != NULL) {
		fclose(filep);
		filep = NULL;
	}

	for (int i = 0; i < ARRAYLEN(fileps); i++) {
		if (fileps[i] != NULL) {
			fclose(fileps[i]);
			fileps[i] = NULL;
		}
	}
}


TEST(tmpfile, basic)
{
	char buf[16] = { 0 };

	filep = tmpfile();
	TEST_ASSERT_NOT_NULL(filep);

	TEST_ASSERT_GREATER_OR_EQUAL(0, fputs("Hello, world!", filep));
	TEST_ASSERT_EQUAL(0, fflush(filep));
	rewind(filep);

	TEST_ASSERT_EQUAL_PTR(buf, fgets(buf, sizeof(buf), filep));
	TEST_ASSERT_EQUAL_STRING("Hello, world!", buf);

	assert_fclosed(&filep);
}


/* tmptile() should be open in "wb+" mode */
TEST(tmpfile, binary)
{
	unsigned char out[256];
	unsigned char in[256];
	size_t i;

	filep = tmpfile();
	TEST_ASSERT_NOT_NULL(filep);

	for (i = 0; i < ARRAYLEN(out); ++i) {
		out[i] = (unsigned char)i;
	}

	TEST_ASSERT_EQUAL(sizeof(out), fwrite(out, 1, sizeof(out), filep));
	TEST_ASSERT_EQUAL(0, fflush(filep));
	rewind(filep);

	TEST_ASSERT_EQUAL(sizeof(in), fread(in, 1, sizeof(in), filep));
	TEST_ASSERT_EQUAL_MEMORY(out, in, sizeof(in));

	assert_fclosed(&filep);
}


TEST(tmpfile, multiple)
{
	char text[32];
	char buf[32];
	int i;

	for (i = 0; i < ARRAYLEN(fileps); ++i) {
		fileps[i] = tmpfile();
		TEST_ASSERT_NOT_NULL(fileps[i]);

		snprintf(text, sizeof(text), "tmpfile-%d", i);

		TEST_ASSERT_GREATER_THAN(0, fprintf(fileps[i], "%s", text));
		TEST_ASSERT_EQUAL(0, fflush(fileps[i]));
	}

	for (i = 0; i < ARRAYLEN(fileps); ++i) {
		memset(buf, 0, sizeof(buf));

		rewind(fileps[i]);
		TEST_ASSERT_EQUAL_PTR(buf, fgets(buf, sizeof(buf), fileps[i]));

		snprintf(text, sizeof(text), "tmpfile-%d", i);

		TEST_ASSERT_EQUAL_STRING(text, buf);

		assert_fclosed(&fileps[i]);
	}
}


static void makeText(char *buf, size_t len)
{
	for (int i = 0; i < len; i++) {
		buf[i] = i & 0xff;
	}
}


TEST(tmpfile, fstat)
{
	int fd, i;
	struct stat stbuf;
	time_t now = time(NULL);
	char rbuf[sizeof(buf)];
	size_t buflen;

	srand(now);

	for (i = 0; i < ARRAYLEN(fileps); i++) {
		fileps[i] = tmpfile();
		TEST_ASSERT_NOT_NULL(fileps[i]);
		fd = fileno(fileps[i]);

		buflen = sizeof(buf);
		makeText(buf, 1 << i);

		TEST_ASSERT_EQUAL(buflen, fwrite(buf, 1, buflen, fileps[i]));
		TEST_ASSERT_EQUAL(0, fflush(fileps[i]));
		rewind(fileps[i]);

		TEST_ASSERT_EQUAL(0, fstat(fd, &stbuf));
		TEST_ASSERT_GREATER_OR_EQUAL(now, stbuf.st_atime);
		TEST_ASSERT_GREATER_OR_EQUAL(now, stbuf.st_mtime);
		TEST_ASSERT_GREATER_OR_EQUAL(now, stbuf.st_ctime);
		TEST_ASSERT_EQUAL(buflen, stbuf.st_size);
		TEST_ASSERT(S_ISREG(stbuf.st_mode));
		TEST_ASSERT_EQUAL(0600, stbuf.st_mode & 0600); /* owner has to have at least RW permissions */

		TEST_ASSERT_EQUAL(buflen, fread(rbuf, 1, sizeof(rbuf), fileps[i]));

		assert_fclosed(&fileps[i]);
	}
}


TEST_GROUP_RUNNER(tmpfile)
{
	RUN_TEST_CASE(tmpfile, basic);
	RUN_TEST_CASE(tmpfile, binary);
	RUN_TEST_CASE(tmpfile, multiple);
	RUN_TEST_CASE(tmpfile, fstat);
}
