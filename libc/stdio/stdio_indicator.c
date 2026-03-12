/*
 * Phoenix-RTOS
 *
 * libc-tests
 *
 * Testing POSIX file operations.
 *
 * Copyright 2023-2026 Phoenix Systems
 * Authors: Arkadiusz Kozlowski, Lukasz Kruszynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>
#include "../testdata.h"

#include <unity_fixture.h>


#define STR_EQUAL(a, b) !strcmp(a, b)
#define ALL_ASCII       256

#define CHECK_MATCH(_res, _first, ...) \
	do { \
		const char *_strs[] = { __VA_ARGS__ }; \
		int _match = 0; \
		for (size_t _idx = 0; _idx < sizeof(_strs) / sizeof(_strs[0]); ++_idx) { \
			if (strcmp(_first, _strs[_idx]) == 0) { \
				_match = 1; \
				break; \
			} \
		} \
		_res = _match; \
	} while (0);

const static char *modes[] = { "r", "r+", "w", "w+", "a", "a+", "rb", "rb+", "wb", "wb+", "ab", "ab+" };
const int num_modes = sizeof(modes) / sizeof(modes[0]);

const char *tellMode(const char *mode)
{
	static char msg[80];
	sprintf(msg, "Tested file mode: %s Error: %s", mode, strerror(errno));
	return msg;
}


/* scratch buffers for the every_delimiter test (no cleanup required) */
static char test_head[ALL_ASCII + 1];
static char test_tail[ALL_ASCII + 1];


/* all per-test resources live here so that TEST_TEAR_DOWN can release them */
static struct {
	FILE *f;
	FILE *fAux;
	char *data;
	char *lineptr;
	char *auxPtr;
	const char *filename;
	const char *fifoPath;
} test_common;


static void test_cleanup(void)
{
	if (test_common.f != NULL) {
		fclose(test_common.f);
		test_common.f = NULL;
	}
	if (test_common.fAux != NULL) {
		fclose(test_common.fAux);
		test_common.fAux = NULL;
	}
	free(test_common.data);
	test_common.data = NULL;
	free(test_common.lineptr);
	test_common.lineptr = NULL;
	free(test_common.auxPtr);
	test_common.auxPtr = NULL;
	if (test_common.filename != NULL) {
		remove(test_common.filename);
		test_common.filename = NULL;
	}
	if (test_common.fifoPath != NULL) {
		remove(test_common.fifoPath);
		test_common.fifoPath = NULL;
	}
}


TEST_GROUP(stdio_feof);

TEST_SETUP(stdio_feof)
{
	memset(&test_common, 0, sizeof(test_common));
}


TEST_TEAR_DOWN(stdio_feof)
{
	test_cleanup();
}


TEST(stdio_feof, not_empty_all_modes)
{
	int tmp;

	test_common.filename = "test_stdio_feof_filled";
	test_common.data = testdata_createCharStr(ALL_ASCII);
	TEST_ASSERT_NOT_NULL(test_common.data);

	for (int i = 0; i < num_modes; ++i) {
		test_common.f = fopen(test_common.filename, "w+");
		TEST_ASSERT_NOT_NULL_MESSAGE(test_common.f, tellMode(modes[i]));

		TEST_ASSERT_NOT_EQUAL_INT_MESSAGE(EOF, fputs(test_common.data, test_common.f), tellMode(modes[i]));
		TEST_ASSERT_EQUAL_INT_MESSAGE(0, fclose(test_common.f), tellMode(modes[i]));
		test_common.f = NULL;

		test_common.f = fopen(test_common.filename, modes[i]);
		TEST_ASSERT_NOT_NULL_MESSAGE(test_common.f, tellMode(modes[i]));
		TEST_ASSERT_EQUAL_INT_MESSAGE(0, feof(test_common.f), tellMode(modes[i]));
		TEST_ASSERT_EQUAL_INT_MESSAGE(0, fseek(test_common.f, 0, SEEK_END), tellMode(modes[i]));
		TEST_ASSERT_EQUAL_INT_MESSAGE(0, feof(test_common.f), tellMode(modes[i]));
		TEST_ASSERT_EQUAL_INT(-1, fgetc(test_common.f));
		CHECK_MATCH(tmp, modes[i], "w", "a", "wb", "ab");
		TEST_ASSERT_EQUAL_MESSAGE(tmp ? 0 : 1, feof(test_common.f), tellMode(modes[i]));

		TEST_ASSERT_EQUAL_INT_MESSAGE(0, fseek(test_common.f, 0, SEEK_SET), tellMode(modes[i]));
		TEST_ASSERT_EQUAL_INT_MESSAGE(0, feof(test_common.f), tellMode(modes[i]));
		TEST_ASSERT_EQUAL_INT_MESSAGE(0, fclose(test_common.f), tellMode(modes[i]));
		test_common.f = NULL;
	}
}

TEST(stdio_feof, empty_all_modes)
{
	int tmp;

	test_common.filename = "test_stdio_feof_empty";
	test_common.f = fopen(test_common.filename, "w+");
	TEST_ASSERT_NOT_NULL(test_common.f);
	TEST_ASSERT_EQUAL_INT(0, fclose(test_common.f));
	test_common.f = NULL;

	for (int i = 0; i < num_modes; ++i) {
		test_common.f = fopen(test_common.filename, modes[i]);
		TEST_ASSERT_NOT_NULL_MESSAGE(test_common.f, tellMode(modes[i]));

		TEST_ASSERT_EQUAL_INT_MESSAGE(0, feof(test_common.f), tellMode(modes[i]));

		TEST_ASSERT_EQUAL(-1, fgetc(test_common.f));
		CHECK_MATCH(tmp, modes[i], "w", "a", "wb", "ab");
		int expectedEof = tmp ? 0 : 1;
		TEST_ASSERT_EQUAL_MESSAGE(tmp ? 0 : 1, feof(test_common.f), tellMode(modes[i]));

		int fd = fileno(test_common.f);
		TEST_ASSERT_EQUAL_INT_MESSAGE(0, close(fd), tellMode(modes[i]));
		TEST_ASSERT_EQUAL_INT_MESSAGE(expectedEof, feof(test_common.f), tellMode(modes[i]));
		TEST_ASSERT_EQUAL_INT(EOF, fclose(test_common.f));
		test_common.f = NULL;
	}
}


TEST(stdio_feof, preserve_errno_huge_size)
{
	int errnoBefore;
	const int multiplier = 10;
	long pos = 0;

	test_common.filename = "test_stdio_feof_errno_preserve";
	test_common.f = fopen(test_common.filename, "w+");
	TEST_ASSERT_NOT_NULL(test_common.f);

	for (int i = 0; i < multiplier; i++) {
		TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fputs(testdata_hugeStr, test_common.f));
	}

	fputc('A', test_common.f);
	errno = 0;
	while (pos <= multiplier * testdata_hugeSize) {
		TEST_ASSERT_EQUAL_INT(0, fseek(test_common.f, pos, SEEK_SET));
		errnoBefore = errno;
		TEST_ASSERT_EQUAL_INT(0, feof(test_common.f));
		TEST_ASSERT_EQUAL(errnoBefore, errno);
		pos += (long)(testdata_hugeSize / 50);
		errno++;
	}
	TEST_ASSERT_EQUAL_INT(-1, fgetc(test_common.f));
	errnoBefore = errno;
	TEST_ASSERT_EQUAL_INT(1, feof(test_common.f));
	TEST_ASSERT_EQUAL_INT(errnoBefore, errno);
}


TEST(stdio_feof, cleared_by_functions)
{
	test_common.filename = "test_stdio_feof_clear";
	test_common.data = testdata_createCharStr(ALL_ASCII);
	TEST_ASSERT_NOT_NULL(test_common.data);

	test_common.f = fopen(test_common.filename, "w+");
	TEST_ASSERT_NOT_NULL(test_common.f);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fputs(test_common.data, test_common.f));
	fseek(test_common.f, 0, SEEK_END);
	fgetc(test_common.f);
	TEST_ASSERT_NOT_EQUAL_INT(0, feof(test_common.f));
	rewind(test_common.f);
	TEST_ASSERT_EQUAL_INT(0, feof(test_common.f));

	fseek(test_common.f, 0, SEEK_END);
	fgetc(test_common.f);
	TEST_ASSERT_NOT_EQUAL_INT(0, feof(test_common.f));
	fseek(test_common.f, 0, SEEK_SET);
	TEST_ASSERT_EQUAL_INT(0, feof(test_common.f));
}


TEST(stdio_feof, false_if_read_error)
{
	test_common.filename = "test_stdio_ignore_errors";
	test_common.f = fopen(test_common.filename, "w");

	TEST_ASSERT_NOT_NULL(test_common.f);
	TEST_ASSERT_EQUAL_INT(EOF, fgetc(test_common.f));
	TEST_ASSERT_NOT_EQUAL_INT(0, ferror(test_common.f));
	TEST_ASSERT_EQUAL_INT(0, feof(test_common.f));
}


TEST_GROUP_RUNNER(stdio_feof)
{
	RUN_TEST_CASE(stdio_feof, not_empty_all_modes);
	RUN_TEST_CASE(stdio_feof, empty_all_modes);
	RUN_TEST_CASE(stdio_feof, preserve_errno_huge_size);
	RUN_TEST_CASE(stdio_feof, cleared_by_functions);
	RUN_TEST_CASE(stdio_feof, false_if_read_error);
}


TEST_GROUP(stdio_ftell);

TEST_SETUP(stdio_ftell)
{
	memset(&test_common, 0, sizeof(test_common));
}


TEST_TEAR_DOWN(stdio_ftell)
{
	test_cleanup();
}


TEST(stdio_ftell, correct_position_not_empty)
{
	int tmp;

	test_common.filename = "test_stdio_ftell_not_empty";
	test_common.data = testdata_createCharStr(ALL_ASCII);
	TEST_ASSERT_NOT_NULL(test_common.data);

	for (int i = 0; i < num_modes; ++i) {
		test_common.f = fopen(test_common.filename, "w+");
		TEST_ASSERT_NOT_NULL(test_common.f);
		TEST_ASSERT_NOT_EQUAL_INT_MESSAGE(EOF, fputs(test_common.data, test_common.f), tellMode(modes[i]));
		TEST_ASSERT_EQUAL_INT_MESSAGE(0, fclose(test_common.f), tellMode(modes[i]));
		test_common.f = NULL;

		errno = 0;

		test_common.f = fopen(test_common.filename, modes[i]);

		TEST_ASSERT_NOT_NULL_MESSAGE(test_common.f, tellMode(modes[i]));
		TEST_ASSERT_EQUAL_INT32_MESSAGE((int32_t)ftell(test_common.f), (int32_t)ftello(test_common.f), tellMode(modes[i]));

		if (STR_EQUAL("w", modes[i]) || STR_EQUAL("wb", modes[i])) { /* can't read in "w" mode */
			TEST_ASSERT_EQUAL_INT_MESSAGE(0, fclose(test_common.f), tellMode(modes[i]));
			test_common.f = NULL;
			continue;
		}

		TEST_ASSERT_EQUAL_INT_MESSAGE(0, fseek(test_common.f, 0, SEEK_SET), tellMode(modes[i]));

		{
			int charCount = 1;
			int c;
			const int expectedErrno = 100;
			while ((c = fgetc(test_common.f)) != EOF) {
				errno = expectedErrno;

				TEST_ASSERT_EQUAL_INT32(charCount, (int32_t)ftell(test_common.f));
				TEST_ASSERT_EQUAL_INT32((int32_t)ftello(test_common.f), (int32_t)ftell(test_common.f));
				TEST_ASSERT_EQUAL_INT(expectedErrno, errno);
				charCount++;
			}
			errno = 0;
		}

		if (STR_EQUAL("w+", modes[i]) || STR_EQUAL("wb+", modes[i])) {
			TEST_ASSERT_EQUAL_INT32_MESSAGE(0, (int32_t)ftell(test_common.f), tellMode(modes[i]));
		}

		TEST_ASSERT_EQUAL_INT32_MESSAGE((int32_t)ftell(test_common.f), (int32_t)ftello(test_common.f), tellMode(modes[i]));
		TEST_ASSERT_EQUAL_INT_MESSAGE(0, fseek(test_common.f, 2, SEEK_END), tellMode(modes[i]));
		CHECK_MATCH(tmp, modes[i], "w+", "wb+");
		size_t dataLen = strlen(test_common.data);
		TEST_ASSERT_EQUAL_MESSAGE(tmp ? 2 : dataLen + 2, ftell(test_common.f), tellMode(modes[i]));
		TEST_ASSERT_EQUAL_INT32_MESSAGE((int32_t)ftell(test_common.f), (int32_t)ftello(test_common.f), tellMode(modes[i]));

		TEST_ASSERT_EQUAL_INT_MESSAGE(0, fclose(test_common.f), tellMode(modes[i]));
		test_common.f = NULL;
	}
}


TEST(stdio_ftell, correct_position_empty)
{
	test_common.filename = "test_stdio_ftell_empty";
	test_common.f = fopen(test_common.filename, "w+");
	TEST_ASSERT_NOT_NULL(test_common.f);
	TEST_ASSERT_EQUAL_INT(0, fclose(test_common.f));
	test_common.f = NULL;

	for (int i = 0; i < num_modes; ++i) {
		test_common.f = fopen(test_common.filename, modes[i]);
		TEST_ASSERT_NOT_NULL_MESSAGE(test_common.f, tellMode(modes[i]));
		TEST_ASSERT_EQUAL_INT32_MESSAGE(0, (int32_t)ftell(test_common.f), tellMode(modes[i]));
		TEST_ASSERT_EQUAL_INT_MESSAGE(0, fclose(test_common.f), tellMode(modes[i]));
		test_common.f = NULL;
	}
}


TEST(stdio_ftell, bad_file_descriptor)
{
	test_common.filename = "test_stdio_bfd.txt";
	test_common.f = fopen(test_common.filename, "w+");
	int fd = fileno(test_common.f);
	TEST_ASSERT_EQUAL_INT(0, close(fd));

	errno = 0;
	TEST_ASSERT_EQUAL_INT32(-1, (int32_t)ftell(test_common.f));
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


TEST(stdio_ftell, wrong_stream_type_socket)
{
	int socketfd = socket(AF_UNIX, SOCK_STREAM, 0);

	TEST_ASSERT_NOT_EQUAL_INT(-1, socketfd);

	errno = 0;
	test_common.f = fdopen(socketfd, "r");

	errno = 0;
	TEST_ASSERT_EQUAL_INT32(-1, (int32_t)ftell(test_common.f));
	TEST_ASSERT_EQUAL_INT(ESPIPE, errno);

	errno = 0;
	TEST_ASSERT_EQUAL_INT32(-1, (int32_t)ftello(test_common.f));
	TEST_ASSERT_EQUAL_INT(ESPIPE, errno);
}


TEST(stdio_ftell, wrong_stream_type_pipe)
{
	int pipefd[2];

	errno = 0;
	if (pipe(pipefd) == -1) {
		/* disabled because of issue #1338: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/1338 */
		if (errno == ENOSYS) {
			TEST_IGNORE_MESSAGE("#1338 issue");
		}
		else {
			TEST_FAIL_MESSAGE("pipe() returned -1");
		}
	}

	test_common.f = fdopen(pipefd[0], "r");
	if (test_common.f == NULL) {
		close(pipefd[0]);
	}

	test_common.fAux = fdopen(pipefd[1], "w");
	if (test_common.fAux == NULL) {
		close(pipefd[1]);
	}

	FILE *pipeStreams[2] = { test_common.f, test_common.fAux };

	for (int i = 0; i < 2; ++i) {
		TEST_ASSERT_NOT_NULL(pipeStreams[i]);

		errno = 0;
		TEST_ASSERT_EQUAL_INT32(-1, (int32_t)ftell(pipeStreams[i]));
		TEST_ASSERT_EQUAL_INT(ESPIPE, errno);

		errno = 0;
		TEST_ASSERT_EQUAL_INT32(-1, (int32_t)ftello(pipeStreams[i]));
		TEST_ASSERT_EQUAL_INT(ESPIPE, errno);
	}
}

TEST(stdio_ftell, wrong_stream_type_fifo)
{
	test_common.fifoPath = "test_stdio_ftell_fufu";
	remove(test_common.fifoPath);
	if (mkfifo(test_common.fifoPath, S_IRWXU) == -1) {
		/* disabled because of issue #1338: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/1338 */
		if (errno == ENOSYS) {
			TEST_IGNORE_MESSAGE("#1338 issue");
		}
		else {
			TEST_FAIL_MESSAGE("mkfifo() returned -1");
		}
	}

	int fifofd = open(test_common.fifoPath, O_RDONLY | O_NONBLOCK);

	TEST_ASSERT_NOT_EQUAL(-1, fifofd);

	test_common.f = fdopen(fifofd, "r");
	if (test_common.f == NULL) {
		close(fifofd);
	}

	TEST_ASSERT_NOT_NULL(test_common.f);

	errno = 0;
	TEST_ASSERT_EQUAL_INT32(-1, (int32_t)ftell(test_common.f));
	TEST_ASSERT_EQUAL(ESPIPE, errno);

	errno = 0;
	TEST_ASSERT_EQUAL_INT32(-1, (int32_t)ftello(test_common.f));
	TEST_ASSERT_EQUAL(ESPIPE, errno);
}


TEST(stdio_ftell, position_after_character_pushed_back)
{
	char first;

	test_common.filename = "test_stdio_ftell_ungetc";
	test_common.data = testdata_createCharStr(ALL_ASCII);
	TEST_ASSERT_NOT_NULL(test_common.data);

	test_common.f = fopen(test_common.filename, "w+");
	TEST_ASSERT_NOT_NULL(test_common.f);
	first = test_common.data[0];
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fputs(test_common.data, test_common.f));
	rewind(test_common.f);
	TEST_ASSERT_EQUAL_INT32(0, (int32_t)ftell(test_common.f));
	TEST_ASSERT_EQUAL_INT(first, fgetc(test_common.f));
	TEST_ASSERT_EQUAL_INT32(1, (int32_t)ftell(test_common.f));
	TEST_ASSERT_EQUAL_INT(first, ungetc(first, test_common.f));
	TEST_ASSERT_EQUAL_INT32(0, (int32_t)ftell(test_common.f));
}


TEST(stdio_ftell, position_after_append_after_rewind)
{

/* disabled because of issue #1403: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/1403 */
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#1403 issue");
#endif
	size_t len;

	test_common.filename = "test_stdio_ftell_append";
	test_common.data = testdata_createCharStr(ALL_ASCII);
	TEST_ASSERT_NOT_NULL(test_common.data);
	len = strlen(test_common.data);

	test_common.f = fopen(test_common.filename, "w");
	TEST_ASSERT_NOT_NULL(test_common.f);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fputs(test_common.data, test_common.f));
	TEST_ASSERT_EQUAL_INT(0, fclose(test_common.f));
	test_common.f = NULL;

	test_common.f = fopen(test_common.filename, "a+");
	TEST_ASSERT_NOT_NULL(test_common.f);
	rewind(test_common.f);
	TEST_ASSERT_EQUAL_INT32(0, (int32_t)ftell(test_common.f));
	TEST_ASSERT_NOT_EQUAL_INT(EOF, fputc('A', test_common.f));
	TEST_ASSERT_EQUAL_INT32(len + 1, (int32_t)ftell(test_common.f));
}


TEST(stdio_ftell, ftello_support_for_large_files)
{
	off_t largeOffset = (off_t)3 * 1024 * 1024 * 1024LL;

	test_common.filename = "test_stdio_ftello_giga_file";
	test_common.f = fopen(test_common.filename, "w+");
	TEST_ASSERT_NOT_NULL(test_common.f);
	TEST_ASSERT_EQUAL_INT(0, fseeko(test_common.f, largeOffset, SEEK_SET));
	TEST_ASSERT_EQUAL_INT64((int64_t)largeOffset, (int64_t)ftello(test_common.f));
}


TEST_GROUP_RUNNER(stdio_ftell)
{
	RUN_TEST_CASE(stdio_ftell, wrong_stream_type_fifo);
	RUN_TEST_CASE(stdio_ftell, correct_position_not_empty);
	RUN_TEST_CASE(stdio_ftell, correct_position_empty);
	RUN_TEST_CASE(stdio_ftell, bad_file_descriptor);
	RUN_TEST_CASE(stdio_ftell, wrong_stream_type_socket);
	RUN_TEST_CASE(stdio_ftell, wrong_stream_type_pipe);
	RUN_TEST_CASE(stdio_ftell, position_after_character_pushed_back);
	RUN_TEST_CASE(stdio_ftell, position_after_append_after_rewind);
	RUN_TEST_CASE(stdio_ftell, ftello_support_for_large_files);
}


TEST_GROUP(stdio_getdelim);

TEST_SETUP(stdio_getdelim)
{
	memset(&test_common, 0, sizeof(test_common));
}


TEST_TEAR_DOWN(stdio_getdelim)
{
	test_cleanup();
}


TEST(stdio_getdelim, existing_delim_empty_or_simple)
{
	const char *expectedFirst[3] = { "Ie", "IIe", "III" };
	size_t len = 0;
	int i;

	test_common.filename = "Simple_text";
	test_common.f = fopen(test_common.filename, "w+");
	TEST_ASSERT_NOT_NULL(test_common.f);
	TEST_ASSERT_EQUAL_INT64(-1, getdelim(&test_common.lineptr, &len, 'Q', test_common.f));
	TEST_ASSERT_NOT_EQUAL(0, len);
	TEST_ASSERT_NOT_NULL(test_common.lineptr);

	TEST_ASSERT_NOT_EQUAL_INT(EOF, fputs("IeIIeIII", test_common.f));

	rewind(test_common.f);

	i = 0;
	while (getdelim(&test_common.lineptr, &len, (int)'e', test_common.f) != -1) {
		TEST_ASSERT_EQUAL_STRING(expectedFirst[i++], test_common.lineptr);
	}
	TEST_ASSERT_EQUAL_INT(sizeof(expectedFirst) / sizeof(expectedFirst[0]), i);
}


TEST(stdio_getdelim, existing_delim_long_text)
{
	size_t len = 0;
	int i;

	test_common.filename = "Long_text";
	test_common.f = fopen(test_common.filename, "w+");
	TEST_ASSERT_NOT_NULL(test_common.f);
	TEST_ASSERT_NOT_EQUAL_INT(EOF, fputs(testdata_hugeStr, test_common.f));

	TEST_ASSERT_EQUAL_INT(0, fseek(test_common.f, 0, SEEK_SET));

	i = 0;
	while (getdelim(&test_common.lineptr, &len, (int)'e', test_common.f) != -1) {
		i++;
		TEST_ASSERT_NOT_NULL(test_common.lineptr);
		TEST_ASSERT_GREATER_OR_EQUAL_INT64(1, len);
		TEST_ASSERT_GREATER_OR_EQUAL_INT64(1, strlen(test_common.lineptr));
	}

	TEST_ASSERT_GREATER_THAN(10, i);
}


TEST(stdio_getdelim, invalid_arg_when_given_nullptr)
{
	char **buffer = NULL;
	size_t n = 2;

	test_common.filename = "test_getdelim_einval_lineptr";
	test_common.f = fopen(test_common.filename, "w+");
	TEST_ASSERT_NOT_NULL(test_common.f);
	TEST_ASSERT_NOT_EQUAL_INT(EOF, fputs("RelativelyLongTextThatWillBeBiggerThanNInGetdelim", test_common.f));
	rewind(test_common.f);
	errno = 0;
	TEST_ASSERT_EQUAL_INT64(-1, getdelim(buffer, &n, '\n', test_common.f));
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
	TEST_ASSERT_NULL(buffer);
}

TEST(stdio_getdelim, realloc_lineptr_if_n_too_small)
{
	size_t n = 10;
	size_t nBefore;

	test_common.filename = "test_reallocation_when_n_too_small";
	test_common.f = fopen(test_common.filename, "w+");
	TEST_ASSERT_NOT_NULL(test_common.f);
	TEST_ASSERT_NOT_EQUAL_INT(EOF, fputs("ABC", test_common.f));
	rewind(test_common.f);

	TEST_ASSERT_NOT_EQUAL_INT64(-1, getdelim(&test_common.lineptr, &n, 'B', test_common.f));
	TEST_ASSERT_GREATER_OR_EQUAL_INT64(3, n);
	nBefore = n;
	TEST_ASSERT_NOT_EQUAL_INT64(-1, getdelim(&test_common.lineptr, &n, 'B', test_common.f));

	TEST_ASSERT_NOT_EQUAL_INT(EOF, fputs(testdata_hugeStr, test_common.f));
	rewind(test_common.f);

	TEST_ASSERT_NOT_EQUAL_INT64(-1, getdelim(&test_common.lineptr, &n, '\n', test_common.f));

	TEST_ASSERT_GREATER_OR_EQUAL_INT64(nBefore, n);
}


TEST(stdio_getdelim, every_delimiter)
{
	size_t len = 0, getlineLen = 0;

	test_common.filename = "every_delimiter.txt";
	test_common.data = testdata_createCharStr(ALL_ASCII);
	TEST_ASSERT_NOT_NULL(test_common.data);

	test_common.f = fopen(test_common.filename, "w+");
	TEST_ASSERT_NOT_NULL(test_common.f);
	TEST_ASSERT_NOT_EQUAL_INT(EOF, fputs(test_common.data, test_common.f));

	for (int i = 2; i < 254; ++i) {
		size_t headSize = i + 1;
		size_t tailSize = 254 - i;

		memcpy(test_head, test_common.data, headSize);
		test_head[headSize] = '\0';
		memcpy(test_tail, test_common.data + i + 1, tailSize);
		test_tail[tailSize] = '\0';

		rewind(test_common.f);

		TEST_ASSERT_EQUAL(i + 1, getdelim(&test_common.lineptr, &len, i, test_common.f));
		TEST_ASSERT_EQUAL_STRING(test_head, test_common.lineptr);
		TEST_ASSERT_EQUAL(254 - i, getdelim(&test_common.lineptr, &len, i, test_common.f));
		TEST_ASSERT_EQUAL_STRING(test_tail, test_common.lineptr);
	}

	rewind(test_common.f);
	TEST_ASSERT_NOT_EQUAL(-1, getline(&test_common.auxPtr, &getlineLen, test_common.f));
	rewind(test_common.f);
	TEST_ASSERT_NOT_EQUAL(-1, getdelim(&test_common.lineptr, &len, '\n', test_common.f));

	TEST_ASSERT_EQUAL_STRING(test_common.auxPtr, test_common.lineptr);
}


TEST(stdio_getdelim, invalid_argument_null_length)
{
	size_t *len = NULL;
	ssize_t read;

	test_common.filename = "test_einval.txt";
	test_common.f = fopen(test_common.filename, "w+");
	TEST_ASSERT_NOT_NULL(test_common.f);
	fprintf(test_common.f, "%s", "lorem ipsum");

	errno = 0;
	read = getdelim(&test_common.lineptr, len, 'u', test_common.f);

	TEST_ASSERT_EQUAL(-1, read);
	TEST_ASSERT_EQUAL(EINVAL, errno);
}


TEST(stdio_getdelim, getdelim_wronly)
{
	size_t len = 0;

	test_common.filename = "test_wronly";
	/* read using getdelim from write-only file */
	test_common.f = fopen(test_common.filename, "a");
	TEST_ASSERT_NOT_NULL(test_common.f);

	rewind(test_common.f);
	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, getdelim(&test_common.lineptr, &len, 'x', test_common.f));
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
	/* even if line is a NULL pointer and there is nothing to read, it shall allocate even a byte for NUL termination char */
	TEST_ASSERT_NOT_NULL(test_common.lineptr);
}

TEST(stdio_getdelim, delim_boundary_values)
{
	const int delim = 'A';
	size_t n = 1;
	ssize_t bytesRead;

	test_common.filename = "delim_test.txt";
	test_common.lineptr = (char *)malloc(1);
	TEST_ASSERT_NOT_NULL(test_common.lineptr);
	test_common.lineptr[0] = 'X';

	test_common.f = fopen(test_common.filename, "w+");
	TEST_ASSERT_NOT_NULL(test_common.f);
	fputc(delim, test_common.f);
	fseek(test_common.f, 0, SEEK_SET);

	bytesRead = getdelim(&test_common.lineptr, &n, delim, test_common.f);

	TEST_ASSERT_GREATER_THAN(0, bytesRead);

	TEST_ASSERT_EQUAL_INT('A', test_common.lineptr[bytesRead - 1]);
}


TEST(stdio_getdelim, eof_sets_indicator)
{
	const char *content = "noDelimiterHere";
	size_t len = 0;

	test_common.filename = "test_getdelim_eof";
	test_common.f = fopen(test_common.filename, "w+");
	TEST_ASSERT_NOT_NULL(test_common.f);
	TEST_ASSERT_NOT_EQUAL_INT(EOF, fputs(content, test_common.f));
	rewind(test_common.f);

	/* whole content read without hitting the delimiter: returns bytes read */
	TEST_ASSERT_EQUAL_INT64((ssize_t)strlen(content), getdelim(&test_common.lineptr, &len, '\n', test_common.f));

	/* nothing left to read: -1 returned, EOF indicator set, error indicator clear */
	errno = 0;
	TEST_ASSERT_EQUAL_INT64(-1, getdelim(&test_common.lineptr, &len, '\n', test_common.f));
	TEST_ASSERT_NOT_EQUAL_INT(0, feof(test_common.f));
	TEST_ASSERT_EQUAL_INT(0, ferror(test_common.f));

	/* EOF indicator already set: function still returns -1 */
	TEST_ASSERT_EQUAL_INT64(-1, getdelim(&test_common.lineptr, &len, '\n', test_common.f));
}


TEST_GROUP_RUNNER(stdio_getdelim)
{
	RUN_TEST_CASE(stdio_getdelim, existing_delim_empty_or_simple);
	RUN_TEST_CASE(stdio_getdelim, existing_delim_long_text);
	RUN_TEST_CASE(stdio_getdelim, invalid_argument_null_length);
	RUN_TEST_CASE(stdio_getdelim, every_delimiter);
	RUN_TEST_CASE(stdio_getdelim, getdelim_wronly);
	RUN_TEST_CASE(stdio_getdelim, invalid_arg_when_given_nullptr);
	RUN_TEST_CASE(stdio_getdelim, realloc_lineptr_if_n_too_small);
	RUN_TEST_CASE(stdio_getdelim, delim_boundary_values);
	RUN_TEST_CASE(stdio_getdelim, eof_sets_indicator);
}
