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

#include <sys/types.h>
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
	} while (0)


/*
 * POSIX requires the stream to be disassociated from the file whether or not fclose()
 * succeeds, so the tracked pointer must be cleared before the assertion - a failing
 * assertion longjmps straight to TEST_TEAR_DOWN, which would close the stream again.
 */
#define FCLOSE_AND_ASSERT(_stream, _expected, _msg) \
	do { \
		FILE *_toClose = (_stream); \
		(_stream) = NULL; \
		TEST_ASSERT_EQUAL_INT_MESSAGE((_expected), fclose(_toClose), (_msg)); \
	} while (0)

/* feof() and ferror() are only required to return non-zero when the indicator is set */
#define ASSERT_EOF_SET(_stream, _msg)     TEST_ASSERT_NOT_EQUAL_INT_MESSAGE(0, feof(_stream), (_msg))
#define ASSERT_EOF_CLEAR(_stream, _msg)   TEST_ASSERT_EQUAL_INT_MESSAGE(0, feof(_stream), (_msg))
#define ASSERT_ERROR_SET(_stream, _msg)   TEST_ASSERT_NOT_EQUAL_INT_MESSAGE(0, ferror(_stream), (_msg))
#define ASSERT_ERROR_CLEAR(_stream, _msg) TEST_ASSERT_EQUAL_INT_MESSAGE(0, ferror(_stream), (_msg))

#define ASSERT_EOF_STATE(_set, _stream, _msg) \
	do { \
		if ((_set) != 0) { \
			ASSERT_EOF_SET((_stream), (_msg)); \
		} \
		else { \
			ASSERT_EOF_CLEAR((_stream), (_msg)); \
		} \
	} while (0)

#define ASSERT_ERROR_STATE(_set, _stream, _msg) \
	do { \
		if ((_set) != 0) { \
			ASSERT_ERROR_SET((_stream), (_msg)); \
		} \
		else { \
			ASSERT_ERROR_CLEAR((_stream), (_msg)); \
		} \
	} while (0)


static const char *const modes[] = { "r", "r+", "w", "w+", "a", "a+", "rb", "rb+", "wb", "wb+", "ab", "ab+" };
static const int num_modes = sizeof(modes) / sizeof(modes[0]);


static const char *tellMode(const char *mode)
{
	static char msg[80];
	snprintf(msg, sizeof(msg), "Tested file mode: %s Error: %s", mode, strerror(errno));
	return msg;
}


/* scratch buffers for the every_delimiter test (no cleanup required) */
static char test_head[ALL_ASCII + 1];
static char test_tail[ALL_ASCII + 1];


static struct {
	FILE *f;
	FILE *fAux;
	int fd[2];
	char *data;
	char *lineptr;
	char *auxPtr;
	const char *filename;
	const char *fifoPath;
} test_common;


static void test_init(void)
{
	memset(&test_common, 0, sizeof(test_common));
	test_common.fd[0] = -1;
	test_common.fd[1] = -1;
}


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
	for (int i = 0; i < 2; ++i) {
		if (test_common.fd[i] >= 0) {
			close(test_common.fd[i]);
			test_common.fd[i] = -1;
		}
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


static void test_fdopen(FILE **streamSlot, int *fdSlot, const char *mode)
{
	*streamSlot = fdopen(*fdSlot, mode);
	if (*streamSlot != NULL) {
		*fdSlot = -1;
	}
}


/* leaves a readable stream at end of file with the EOF indicator set */
static void test_setEof(FILE *stream)
{
	TEST_ASSERT_EQUAL_INT(0, fseek(stream, 0, SEEK_END));
	TEST_ASSERT_EQUAL_INT(EOF, fgetc(stream));
	ASSERT_EOF_SET(stream, NULL);
}


/* both functions must report the same position; each is read once so that the
 * reported "expected/was" pair stays meaningful when they disagree */
static void test_assertTellEqual(FILE *stream, const char *msg)
{
	long tellPos = ftell(stream);
	off_t telloPos = ftello(stream);

	TEST_ASSERT_EQUAL_INT64_MESSAGE((int64_t)tellPos, (int64_t)telloPos, msg);
}


TEST_GROUP(stdio_feof);

TEST_SETUP(stdio_feof)
{
	test_init();
}


TEST_TEAR_DOWN(stdio_feof)
{
	test_cleanup();
}


TEST(stdio_feof, not_empty_all_modes)
{
	int writeOnly;

	test_common.filename = "test_stdio_feof_filled";
	test_common.data = testdata_createCharStr(ALL_ASCII);
	TEST_ASSERT_NOT_NULL(test_common.data);

	for (int i = 0; i < num_modes; ++i) {
		test_common.f = fopen(test_common.filename, "w+");
		TEST_ASSERT_NOT_NULL_MESSAGE(test_common.f, tellMode(modes[i]));

		TEST_ASSERT_NOT_EQUAL_INT_MESSAGE(EOF, fputs(test_common.data, test_common.f), tellMode(modes[i]));
		FCLOSE_AND_ASSERT(test_common.f, 0, tellMode(modes[i]));

		test_common.f = fopen(test_common.filename, modes[i]);
		TEST_ASSERT_NOT_NULL_MESSAGE(test_common.f, tellMode(modes[i]));
		ASSERT_EOF_CLEAR(test_common.f, tellMode(modes[i]));
		TEST_ASSERT_EQUAL_INT_MESSAGE(0, fseek(test_common.f, 0, SEEK_END), tellMode(modes[i]));
		ASSERT_EOF_CLEAR(test_common.f, tellMode(modes[i]));
		TEST_ASSERT_EQUAL_INT_MESSAGE(EOF, fgetc(test_common.f), tellMode(modes[i]));

		/* the failed read means end of file on a readable stream and an error on a write-only one */
		CHECK_MATCH(writeOnly, modes[i], "w", "a", "wb", "ab");
		ASSERT_EOF_STATE(!writeOnly, test_common.f, tellMode(modes[i]));
		ASSERT_ERROR_STATE(writeOnly, test_common.f, tellMode(modes[i]));

		TEST_ASSERT_EQUAL_INT_MESSAGE(0, fseek(test_common.f, 0, SEEK_SET), tellMode(modes[i]));
		ASSERT_EOF_CLEAR(test_common.f, tellMode(modes[i]));
		FCLOSE_AND_ASSERT(test_common.f, 0, tellMode(modes[i]));
	}
}


TEST(stdio_feof, empty_all_modes)
{
	int writeOnly;
	int fd;

	test_common.filename = "test_stdio_feof_empty";
	test_common.f = fopen(test_common.filename, "w+");
	TEST_ASSERT_NOT_NULL(test_common.f);
	FCLOSE_AND_ASSERT(test_common.f, 0, NULL);

	for (int i = 0; i < num_modes; ++i) {
		test_common.f = fopen(test_common.filename, modes[i]);
		TEST_ASSERT_NOT_NULL_MESSAGE(test_common.f, tellMode(modes[i]));

		ASSERT_EOF_CLEAR(test_common.f, tellMode(modes[i]));

		TEST_ASSERT_EQUAL_INT_MESSAGE(EOF, fgetc(test_common.f), tellMode(modes[i]));
		CHECK_MATCH(writeOnly, modes[i], "w", "a", "wb", "ab");
		ASSERT_EOF_STATE(!writeOnly, test_common.f, tellMode(modes[i]));

		/* closing the descriptor behind the stream must not change the indicator */
		fd = fileno(test_common.f);
		TEST_ASSERT_NOT_EQUAL_INT_MESSAGE(-1, fd, tellMode(modes[i]));
		TEST_ASSERT_EQUAL_INT_MESSAGE(0, close(fd), tellMode(modes[i]));
		ASSERT_EOF_STATE(!writeOnly, test_common.f, tellMode(modes[i]));
		FCLOSE_AND_ASSERT(test_common.f, EOF, tellMode(modes[i]));
	}
}


TEST(stdio_feof, preserve_errno_huge_size)
{
	int errnoBefore;
	const int multiplier = 10;
	const int steps = 50;
	long fileSize, step, pos;

	test_common.filename = "test_stdio_feof_errno_preserve";
	test_common.f = fopen(test_common.filename, "w+");
	TEST_ASSERT_NOT_NULL(test_common.f);

	for (int i = 0; i < multiplier; i++) {
		TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fputs(testdata_hugeStr, test_common.f));
	}
	TEST_ASSERT_NOT_EQUAL_INT(EOF, fputc('A', test_common.f));

	/* the walk has to follow the data actually written, not an estimate of it,
	 * otherwise the final read may land inside the file instead of past its end */
	TEST_ASSERT_EQUAL_INT(0, fseek(test_common.f, 0, SEEK_END));
	fileSize = ftell(test_common.f);
	TEST_ASSERT_EQUAL_INT64((int64_t)multiplier * (int64_t)(testdata_hugeSize - 1) + 1, (int64_t)fileSize);
	step = fileSize / steps;
	TEST_ASSERT_GREATER_THAN_INT32(0, (int32_t)step);

	/* seeking inside the file neither sets the indicator nor touches errno */
	errno = 0;
	for (pos = 0; pos < fileSize; pos += step) {
		TEST_ASSERT_EQUAL_INT(0, fseek(test_common.f, pos, SEEK_SET));
		errnoBefore = errno;
		ASSERT_EOF_CLEAR(test_common.f, NULL);
		TEST_ASSERT_EQUAL_INT(errnoBefore, errno);
		errno++;
	}

	/* only a read attempt past the last byte sets it */
	TEST_ASSERT_EQUAL_INT(0, fseek(test_common.f, 0, SEEK_END));
	TEST_ASSERT_EQUAL_INT(EOF, fgetc(test_common.f));
	errnoBefore = errno;
	ASSERT_EOF_SET(test_common.f, NULL);
	TEST_ASSERT_EQUAL_INT(errnoBefore, errno);
}


TEST(stdio_feof, cleared_by_functions)
{
	fpos_t pos;

	test_common.filename = "test_stdio_feof_clear";
	test_common.data = testdata_createCharStr(ALL_ASCII);
	TEST_ASSERT_NOT_NULL(test_common.data);

	test_common.f = fopen(test_common.filename, "w+");
	TEST_ASSERT_NOT_NULL(test_common.f);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fputs(test_common.data, test_common.f));

	test_setEof(test_common.f);
	rewind(test_common.f);
	ASSERT_EOF_CLEAR(test_common.f, "rewind()");

	test_setEof(test_common.f);
	TEST_ASSERT_EQUAL_INT(0, fseek(test_common.f, 0, SEEK_SET));
	ASSERT_EOF_CLEAR(test_common.f, "fseek()");

	test_setEof(test_common.f);
	TEST_ASSERT_EQUAL_INT(0, fseeko(test_common.f, 0, SEEK_SET));
	ASSERT_EOF_CLEAR(test_common.f, "fseeko()");

	TEST_ASSERT_EQUAL_INT(0, fgetpos(test_common.f, &pos));
	test_setEof(test_common.f);
	TEST_ASSERT_EQUAL_INT(0, fsetpos(test_common.f, &pos));
	ASSERT_EOF_CLEAR(test_common.f, "fsetpos()");

	/* ungetc() needs a byte already consumed from the buffer to push back into */
	TEST_ASSERT_EQUAL_INT(0, fseek(test_common.f, 0, SEEK_SET));
	TEST_ASSERT_NOT_EQUAL_INT(EOF, fgetc(test_common.f));
	test_setEof(test_common.f);
	TEST_ASSERT_EQUAL_INT('A', ungetc('A', test_common.f));
	ASSERT_EOF_CLEAR(test_common.f, "ungetc()");

	test_setEof(test_common.f);
	clearerr(test_common.f);
	ASSERT_EOF_CLEAR(test_common.f, "clearerr()");
}


TEST(stdio_feof, false_if_read_error)
{
	test_common.filename = "test_stdio_ignore_errors";
	test_common.f = fopen(test_common.filename, "w");

	TEST_ASSERT_NOT_NULL(test_common.f);
	TEST_ASSERT_EQUAL_INT(EOF, fgetc(test_common.f));
	ASSERT_ERROR_SET(test_common.f, NULL);
	ASSERT_EOF_CLEAR(test_common.f, NULL);
}


TEST(stdio_feof, error_indicator_kept_until_cleared)
{
	test_common.filename = "test_stdio_ferror_sticky";
	test_common.f = fopen(test_common.filename, "w");
	TEST_ASSERT_NOT_NULL(test_common.f);

	/* reading from a write-only stream fails and sets the error indicator */
	TEST_ASSERT_EQUAL_INT(EOF, fgetc(test_common.f));
	ASSERT_ERROR_SET(test_common.f, NULL);

	/* a successful operation must not clear it */
	TEST_ASSERT_NOT_EQUAL_INT(EOF, fputs("data", test_common.f));
	ASSERT_ERROR_SET(test_common.f, "after a successful write");

	/* neither does a successful seek, unlike the EOF indicator */
	TEST_ASSERT_EQUAL_INT(0, fseek(test_common.f, 0, SEEK_SET));
	ASSERT_ERROR_SET(test_common.f, "after fseek()");

	clearerr(test_common.f);
	ASSERT_ERROR_CLEAR(test_common.f, "after clearerr()");

	/* rewind() is defined as fseek() to the beginning plus clearerr() */
	TEST_ASSERT_EQUAL_INT(EOF, fgetc(test_common.f));
	ASSERT_ERROR_SET(test_common.f, NULL);
	rewind(test_common.f);
	ASSERT_ERROR_CLEAR(test_common.f, "after rewind()");
}


TEST(stdio_feof, clearerr_clears_both_indicators)
{
	test_common.filename = "test_stdio_clearerr";
	test_common.f = fopen(test_common.filename, "w+");
	TEST_ASSERT_NOT_NULL(test_common.f);
	TEST_ASSERT_NOT_EQUAL_INT(EOF, fputs("data", test_common.f));

	/* EOF indicator alone */
	test_setEof(test_common.f);
	ASSERT_ERROR_CLEAR(test_common.f, NULL);
	clearerr(test_common.f);
	ASSERT_EOF_CLEAR(test_common.f, NULL);
	ASSERT_ERROR_CLEAR(test_common.f, NULL);

	FCLOSE_AND_ASSERT(test_common.f, 0, NULL);

	/* both indicators at once: end of file reached by reading, then a write on a
	 * read-only stream - only that the write fails is portable, not when it does */
	test_common.f = fopen(test_common.filename, "r");
	TEST_ASSERT_NOT_NULL(test_common.f);
	test_setEof(test_common.f);
	fputs("data", test_common.f);
	fflush(test_common.f);
	ASSERT_ERROR_SET(test_common.f, NULL);
	ASSERT_EOF_SET(test_common.f, NULL);

	clearerr(test_common.f);
	ASSERT_EOF_CLEAR(test_common.f, NULL);
	ASSERT_ERROR_CLEAR(test_common.f, NULL);
}


TEST_GROUP_RUNNER(stdio_feof)
{
	RUN_TEST_CASE(stdio_feof, not_empty_all_modes);
	RUN_TEST_CASE(stdio_feof, empty_all_modes);
	RUN_TEST_CASE(stdio_feof, preserve_errno_huge_size);
	RUN_TEST_CASE(stdio_feof, cleared_by_functions);
	RUN_TEST_CASE(stdio_feof, false_if_read_error);
	RUN_TEST_CASE(stdio_feof, error_indicator_kept_until_cleared);
	RUN_TEST_CASE(stdio_feof, clearerr_clears_both_indicators);
}


TEST_GROUP(stdio_ftell);

TEST_SETUP(stdio_ftell)
{
	test_init();
}


TEST_TEAR_DOWN(stdio_ftell)
{
	test_cleanup();
}


TEST(stdio_ftell, correct_position_not_empty)
{
	int truncated;
	size_t dataLen;

	test_common.filename = "test_stdio_ftell_not_empty";
	test_common.data = testdata_createCharStr(ALL_ASCII);
	TEST_ASSERT_NOT_NULL(test_common.data);
	dataLen = strlen(test_common.data);

	for (int i = 0; i < num_modes; ++i) {
		test_common.f = fopen(test_common.filename, "w+");
		TEST_ASSERT_NOT_NULL_MESSAGE(test_common.f, tellMode(modes[i]));
		TEST_ASSERT_NOT_EQUAL_INT_MESSAGE(EOF, fputs(test_common.data, test_common.f), tellMode(modes[i]));
		FCLOSE_AND_ASSERT(test_common.f, 0, tellMode(modes[i]));

		test_common.f = fopen(test_common.filename, modes[i]);

		TEST_ASSERT_NOT_NULL_MESSAGE(test_common.f, tellMode(modes[i]));
		test_assertTellEqual(test_common.f, tellMode(modes[i]));

		if (STR_EQUAL("w", modes[i]) || STR_EQUAL("wb", modes[i])) { /* can't read in "w" mode */
			FCLOSE_AND_ASSERT(test_common.f, 0, tellMode(modes[i]));
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
				test_assertTellEqual(test_common.f, tellMode(modes[i]));
				TEST_ASSERT_EQUAL_INT(expectedErrno, errno);
				charCount++;
			}
			errno = 0;
		}

		/* "w+" truncated the file on open, so there was nothing to read */
		CHECK_MATCH(truncated, modes[i], "w+", "wb+");
		if (truncated != 0) {
			TEST_ASSERT_EQUAL_INT32_MESSAGE(0, (int32_t)ftell(test_common.f), tellMode(modes[i]));
		}

		test_assertTellEqual(test_common.f, tellMode(modes[i]));
		TEST_ASSERT_EQUAL_INT_MESSAGE(0, fseek(test_common.f, 2, SEEK_END), tellMode(modes[i]));
		TEST_ASSERT_EQUAL_INT64_MESSAGE(truncated ? 2 : (int64_t)dataLen + 2, (int64_t)ftell(test_common.f), tellMode(modes[i]));
		test_assertTellEqual(test_common.f, tellMode(modes[i]));

		FCLOSE_AND_ASSERT(test_common.f, 0, tellMode(modes[i]));
	}
}


TEST(stdio_ftell, correct_position_empty)
{
	test_common.filename = "test_stdio_ftell_empty";
	test_common.f = fopen(test_common.filename, "w+");
	TEST_ASSERT_NOT_NULL(test_common.f);
	FCLOSE_AND_ASSERT(test_common.f, 0, NULL);

	for (int i = 0; i < num_modes; ++i) {
		test_common.f = fopen(test_common.filename, modes[i]);
		TEST_ASSERT_NOT_NULL_MESSAGE(test_common.f, tellMode(modes[i]));
		TEST_ASSERT_EQUAL_INT32_MESSAGE(0, (int32_t)ftell(test_common.f), tellMode(modes[i]));
		test_assertTellEqual(test_common.f, tellMode(modes[i]));
		FCLOSE_AND_ASSERT(test_common.f, 0, tellMode(modes[i]));
	}
}


TEST(stdio_ftell, bad_file_descriptor)
{
	int fd;

	test_common.filename = "test_stdio_bfd.txt";
	test_common.f = fopen(test_common.filename, "w+");
	TEST_ASSERT_NOT_NULL(test_common.f);

	fd = fileno(test_common.f);
	TEST_ASSERT_NOT_EQUAL_INT(-1, fd);
	TEST_ASSERT_EQUAL_INT(0, close(fd));

	errno = 0;
	TEST_ASSERT_EQUAL_INT32(-1, (int32_t)ftell(test_common.f));
	TEST_ASSERT_EQUAL_INT(EBADF, errno);

	errno = 0;
	TEST_ASSERT_EQUAL_INT64(-1, (int64_t)ftello(test_common.f));
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


TEST(stdio_ftell, wrong_stream_type_socket)
{
	test_common.fd[0] = socket(AF_UNIX, SOCK_STREAM, 0);
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.fd[0]);

	test_fdopen(&test_common.f, &test_common.fd[0], "r");
	TEST_ASSERT_NOT_NULL(test_common.f);

	errno = 0;
	TEST_ASSERT_EQUAL_INT32(-1, (int32_t)ftell(test_common.f));
	TEST_ASSERT_EQUAL_INT(ESPIPE, errno);

	errno = 0;
	TEST_ASSERT_EQUAL_INT64(-1, (int64_t)ftello(test_common.f));
	TEST_ASSERT_EQUAL_INT(ESPIPE, errno);
}


TEST(stdio_ftell, wrong_stream_type_pipe)
{
	FILE *pipeStreams[2];

	errno = 0;
	if (pipe(test_common.fd) == -1) {
		/* a failing pipe() may still have written a descriptor number into the array
		 * without allocating it, and closing one the test does not own would be worse
		 * than leaking, so nothing stays tracked unless the call succeeded */
		test_common.fd[0] = -1;
		test_common.fd[1] = -1;

		/* disabled because of issue #1338: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/1338 */
		if (errno == ENOSYS) {
			TEST_IGNORE_MESSAGE("#1338 issue");
		}
		else {
			TEST_FAIL_MESSAGE("pipe() returned -1");
		}
	}

	test_fdopen(&test_common.f, &test_common.fd[0], "r");
	test_fdopen(&test_common.fAux, &test_common.fd[1], "w");

	pipeStreams[0] = test_common.f;
	pipeStreams[1] = test_common.fAux;

	for (int i = 0; i < 2; ++i) {
		TEST_ASSERT_NOT_NULL(pipeStreams[i]);

		errno = 0;
		TEST_ASSERT_EQUAL_INT32(-1, (int32_t)ftell(pipeStreams[i]));
		TEST_ASSERT_EQUAL_INT(ESPIPE, errno);

		errno = 0;
		TEST_ASSERT_EQUAL_INT64(-1, (int64_t)ftello(pipeStreams[i]));
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

	test_common.fd[0] = open(test_common.fifoPath, O_RDONLY | O_NONBLOCK);
	TEST_ASSERT_NOT_EQUAL_INT(-1, test_common.fd[0]);

	test_fdopen(&test_common.f, &test_common.fd[0], "r");
	TEST_ASSERT_NOT_NULL(test_common.f);

	errno = 0;
	TEST_ASSERT_EQUAL_INT32(-1, (int32_t)ftell(test_common.f));
	TEST_ASSERT_EQUAL_INT(ESPIPE, errno);

	errno = 0;
	TEST_ASSERT_EQUAL_INT64(-1, (int64_t)ftello(test_common.f));
	TEST_ASSERT_EQUAL_INT(ESPIPE, errno);
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
	test_assertTellEqual(test_common.f, NULL);
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
	FCLOSE_AND_ASSERT(test_common.f, 0, NULL);

	test_common.f = fopen(test_common.filename, "a+");
	TEST_ASSERT_NOT_NULL(test_common.f);
	rewind(test_common.f);
	TEST_ASSERT_EQUAL_INT32(0, (int32_t)ftell(test_common.f));
	TEST_ASSERT_NOT_EQUAL_INT(EOF, fputc('A', test_common.f));
	TEST_ASSERT_EQUAL_INT64((int64_t)len + 1, (int64_t)ftell(test_common.f));
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
	test_init();
}


TEST_TEAR_DOWN(stdio_getdelim)
{
	test_cleanup();
}


TEST(stdio_getdelim, existing_delim_empty_or_simple)
{
	const char *expectedFirst[3] = { "Ie", "IIe", "III" };
	const int expectedCount = sizeof(expectedFirst) / sizeof(expectedFirst[0]);
	size_t len = 0;
	int i;

	test_common.filename = "test_getdelim_simple_text";
	test_common.f = fopen(test_common.filename, "w+");
	TEST_ASSERT_NOT_NULL(test_common.f);
	TEST_ASSERT_EQUAL_INT64(-1, getdelim(&test_common.lineptr, &len, 'Q', test_common.f));
	ASSERT_EOF_SET(test_common.f, NULL);
	ASSERT_ERROR_CLEAR(test_common.f, NULL);
	/* not required by POSIX, but both libphoenix and glibc allocate the initial
	 * buffer before the first read attempt */
	TEST_ASSERT_NOT_EQUAL(0, len);
	TEST_ASSERT_NOT_NULL(test_common.lineptr);

	TEST_ASSERT_NOT_EQUAL_INT(EOF, fputs("IeIIeIII", test_common.f));

	rewind(test_common.f);

	i = 0;
	while (getdelim(&test_common.lineptr, &len, (int)'e', test_common.f) != -1) {
		/* bound the index before using it, a stray extra line must not read past the array */
		TEST_ASSERT_LESS_THAN_INT(expectedCount, i);
		TEST_ASSERT_EQUAL_STRING(expectedFirst[i], test_common.lineptr);
		i++;
	}
	TEST_ASSERT_EQUAL_INT(expectedCount, i);
}


TEST(stdio_getdelim, existing_delim_long_text)
{
	size_t len = 0, total = 0;
	ssize_t bytesRead;
	int i;

	test_common.filename = "test_getdelim_long_text";
	test_common.f = fopen(test_common.filename, "w+");
	TEST_ASSERT_NOT_NULL(test_common.f);
	TEST_ASSERT_NOT_EQUAL_INT(EOF, fputs(testdata_hugeStr, test_common.f));

	TEST_ASSERT_EQUAL_INT(0, fseek(test_common.f, 0, SEEK_SET));

	i = 0;
	while ((bytesRead = getdelim(&test_common.lineptr, &len, (int)'e', test_common.f)) != -1) {
		i++;
		TEST_ASSERT_NOT_NULL(test_common.lineptr);
		TEST_ASSERT_EQUAL_INT64((int64_t)bytesRead, (int64_t)strlen(test_common.lineptr));
		total += (size_t)bytesRead;
	}

	TEST_ASSERT_EQUAL_INT64((int64_t)strlen(testdata_hugeStr), (int64_t)total);
	TEST_ASSERT_GREATER_THAN_INT(10, i);
}


TEST(stdio_getdelim, invalid_arg_when_given_nullptr)
{
	size_t n = 2;

	test_common.filename = "test_getdelim_einval_lineptr";
	test_common.f = fopen(test_common.filename, "w+");
	TEST_ASSERT_NOT_NULL(test_common.f);
	TEST_ASSERT_NOT_EQUAL_INT(EOF, fputs("RelativelyLongTextThatWillBeBiggerThanNInGetdelim", test_common.f));
	rewind(test_common.f);
	errno = 0;
	TEST_ASSERT_EQUAL_INT64(-1, getdelim(NULL, &n, '\n', test_common.f));
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
	/* the stream must be left untouched */
	TEST_ASSERT_EQUAL_INT32(0, (int32_t)ftell(test_common.f));
	ASSERT_EOF_CLEAR(test_common.f, NULL);
}


TEST(stdio_getdelim, realloc_lineptr_if_n_too_small)
{
	size_t n = 10;
	size_t nBefore;

	test_common.filename = "test_reallocation_when_n_too_small";
	test_common.lineptr = (char *)malloc(n);
	TEST_ASSERT_NOT_NULL(test_common.lineptr);

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

	TEST_ASSERT_GREATER_THAN_INT64((int64_t)nBefore, (int64_t)n);
}


TEST(stdio_getdelim, every_delimiter)
{
	size_t len = 0, getlineLen = 0;
	size_t dataLen;

	test_common.filename = "test_getdelim_every_delimiter";
	test_common.data = testdata_createCharStr(ALL_ASCII);
	TEST_ASSERT_NOT_NULL(test_common.data);
	dataLen = strlen(test_common.data);

	test_common.f = fopen(test_common.filename, "w+");
	TEST_ASSERT_NOT_NULL(test_common.f);
	TEST_ASSERT_NOT_EQUAL_INT(EOF, fputs(test_common.data, test_common.f));

	for (int i = 2; i + 1 < (int)dataLen; ++i) {
		size_t headSize = i + 1;
		size_t tailSize = dataLen - headSize;

		memcpy(test_head, test_common.data, headSize);
		test_head[headSize] = '\0';
		memcpy(test_tail, test_common.data + headSize, tailSize);
		test_tail[tailSize] = '\0';

		rewind(test_common.f);

		TEST_ASSERT_EQUAL_INT64((int64_t)headSize, getdelim(&test_common.lineptr, &len, i, test_common.f));
		TEST_ASSERT_EQUAL_STRING(test_head, test_common.lineptr);
		TEST_ASSERT_EQUAL_INT64((int64_t)tailSize, getdelim(&test_common.lineptr, &len, i, test_common.f));
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
	ssize_t read;

	test_common.filename = "test_getdelim_einval_length";
	test_common.f = fopen(test_common.filename, "w+");
	TEST_ASSERT_NOT_NULL(test_common.f);
	TEST_ASSERT_GREATER_THAN_INT(0, fprintf(test_common.f, "%s", "lorem ipsum"));

	errno = 0;
	read = getdelim(&test_common.lineptr, NULL, 'u', test_common.f);

	TEST_ASSERT_EQUAL_INT64(-1, read);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST(stdio_getdelim, getdelim_wronly)
{
	size_t len = 0;

	test_common.filename = "test_getdelim_wronly";
	/* read using getdelim from write-only file */
	test_common.f = fopen(test_common.filename, "a");
	TEST_ASSERT_NOT_NULL(test_common.f);

	rewind(test_common.f);
	errno = 0;
	TEST_ASSERT_EQUAL_INT64(-1, getdelim(&test_common.lineptr, &len, 'x', test_common.f));
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
	/* a failed read is an error, not end of file */
	ASSERT_ERROR_SET(test_common.f, NULL);
	ASSERT_EOF_CLEAR(test_common.f, NULL);
	/* not required by POSIX: the buffer is allocated before the first read attempt,
	 * so it is not NULL - but nothing was stored in it, not even a NUL terminator */
	TEST_ASSERT_NOT_NULL(test_common.lineptr);
}


TEST(stdio_getdelim, delim_boundary_values)
{
	const int delim = 'A';
	size_t n = 1;
	ssize_t bytesRead;

	test_common.filename = "test_getdelim_delim_boundary";
	test_common.lineptr = (char *)malloc(1);
	TEST_ASSERT_NOT_NULL(test_common.lineptr);
	test_common.lineptr[0] = 'X';

	test_common.f = fopen(test_common.filename, "w+");
	TEST_ASSERT_NOT_NULL(test_common.f);
	TEST_ASSERT_NOT_EQUAL_INT(EOF, fputc(delim, test_common.f));
	TEST_ASSERT_EQUAL_INT(0, fseek(test_common.f, 0, SEEK_SET));

	bytesRead = getdelim(&test_common.lineptr, &n, delim, test_common.f);

	TEST_ASSERT_EQUAL_INT64(1, (int64_t)bytesRead);

	TEST_ASSERT_EQUAL_INT('A', test_common.lineptr[bytesRead - 1]);
}


TEST(stdio_getdelim, delim_boundary_bytes)
{
	const char content[] = { 'a', '\0', 'b', (char)0xff, 'c' };
	size_t n = 0;
	ssize_t bytesRead;

	test_common.filename = "test_getdelim_boundary_bytes";
	test_common.f = fopen(test_common.filename, "w+");
	TEST_ASSERT_NOT_NULL(test_common.f);
	TEST_ASSERT_EQUAL_INT((int)sizeof(content), (int)fwrite(content, 1, sizeof(content), test_common.f));
	rewind(test_common.f);

	bytesRead = getdelim(&test_common.lineptr, &n, '\0', test_common.f);
	TEST_ASSERT_EQUAL_INT64(2, (int64_t)bytesRead);
	TEST_ASSERT_EQUAL_INT('a', test_common.lineptr[0]);
	TEST_ASSERT_EQUAL_INT('\0', test_common.lineptr[bytesRead - 1]);

	bytesRead = getdelim(&test_common.lineptr, &n, 0xff, test_common.f);
	TEST_ASSERT_EQUAL_INT64(2, (int64_t)bytesRead);
	TEST_ASSERT_EQUAL_INT('b', test_common.lineptr[0]);
	TEST_ASSERT_EQUAL_INT((char)0xff, test_common.lineptr[bytesRead - 1]);

	/* the last byte is not followed by a delimiter */
	bytesRead = getdelim(&test_common.lineptr, &n, '\0', test_common.f);
	TEST_ASSERT_EQUAL_INT64(1, (int64_t)bytesRead);
	TEST_ASSERT_EQUAL_STRING("c", test_common.lineptr);
	ASSERT_EOF_SET(test_common.f, NULL);
	ASSERT_ERROR_CLEAR(test_common.f, NULL);
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
	TEST_ASSERT_EQUAL_INT64(-1, getdelim(&test_common.lineptr, &len, '\n', test_common.f));
	ASSERT_EOF_SET(test_common.f, NULL);
	ASSERT_ERROR_CLEAR(test_common.f, NULL);

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
	RUN_TEST_CASE(stdio_getdelim, delim_boundary_bytes);
	RUN_TEST_CASE(stdio_getdelim, eof_sets_indicator);
}
