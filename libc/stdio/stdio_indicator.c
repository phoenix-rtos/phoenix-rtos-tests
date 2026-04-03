/*
 * Phoenix-RTOS
 *
 * libc-tests
 *
 * Testing POSIX file operations.
 *
 * Copyright 2023-2024 Phoenix Systems
 * Author: Arkadiusz Kozlowski
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


TEST_GROUP(stdio_feof);

TEST_SETUP(stdio_feof)
{
}


TEST_TEAR_DOWN(stdio_feof)
{
}


TEST(stdio_feof, not_empty_all_modes)
{
	const char *filename = "test_stdio_feof_filled";
	char *data = testdata_createCharStr(ALL_ASCII);
	int tmp;
	for (int i = 0; i < num_modes; ++i) {
		FILE *f;
		f = fopen(filename, "w+");

		TEST_ASSERT_NOT_NULL_MESSAGE(f, tellMode(modes[i]));
		TEST_ASSERT_NOT_NULL_MESSAGE(data, tellMode(modes[i]));

		TEST_ASSERT_NOT_EQUAL_INT_MESSAGE(EOF, fputs(data, f), tellMode(modes[i]));
		TEST_ASSERT_EQUAL_INT_MESSAGE(0, fclose(f), tellMode(modes[i]));

		f = fopen(filename, modes[i]);

		TEST_ASSERT_NOT_NULL_MESSAGE(f, tellMode(modes[i]));
		TEST_ASSERT_EQUAL_INT_MESSAGE(0, feof(f), tellMode(modes[i]));
		TEST_ASSERT_EQUAL_INT_MESSAGE(0, fseek(f, 0, SEEK_END), tellMode(modes[i]));
		TEST_ASSERT_EQUAL_INT_MESSAGE(0, feof(f), tellMode(modes[i]));
		TEST_ASSERT_EQUAL_INT(-1, fgetc(f));
		CHECK_MATCH(tmp, modes[i], "w", "a", "wb", "ab");
		TEST_ASSERT_EQUAL_MESSAGE(tmp ? 0 : 1, feof(f), tellMode(modes[i]));

		TEST_ASSERT_EQUAL_INT_MESSAGE(0, fseek(f, 0, SEEK_SET), tellMode(modes[i]));
		TEST_ASSERT_EQUAL_INT_MESSAGE(0, feof(f), tellMode(modes[i]));
		TEST_ASSERT_EQUAL_INT_MESSAGE(0, fclose(f), tellMode(modes[i]));
	}
	free(data);
	TEST_ASSERT_EQUAL_INT(0, remove(filename));
}

TEST(stdio_feof, empty_all_modes)
{
	FILE *f;
	const char *filename = "test_stdio_feof_empty";
	f = fopen(filename, "w+");
	int tmp;

	TEST_ASSERT_NOT_NULL(f);
	TEST_ASSERT_EQUAL_INT(0, fclose(f));

	for (int i = 0; i < num_modes; ++i) {
		f = fopen(filename, modes[i]);
		TEST_ASSERT_NOT_NULL_MESSAGE(f, tellMode(modes[i]));

		TEST_ASSERT_EQUAL_INT_MESSAGE(0, feof(f), tellMode(modes[i]));

		TEST_ASSERT_EQUAL(-1, fgetc(f));
		CHECK_MATCH(tmp, modes[i], "w", "a", "wb", "ab");
		TEST_ASSERT_EQUAL_MESSAGE(tmp ? 0 : 1, feof(f), tellMode(modes[i]));

		TEST_ASSERT_EQUAL_INT_MESSAGE(0, fclose(f), tellMode(modes[i]));
		TEST_ASSERT_EQUAL_INT_MESSAGE(0, feof(f), tellMode(modes[i]));

		TEST_ASSERT_EQUAL_INT_MESSAGE(0, feof(f), tellMode(modes[i]));
	}
	TEST_ASSERT_EQUAL(0, remove(filename));
}


TEST(stdio_feof, preserve_errno_huge_size)
{
	int errnoBefore;
	int multiplier = 10;
	long pos = 0;
	const char *filename = "test_stdio_feof_errno_preserve";
	FILE *f = fopen(filename, "w+");
	TEST_ASSERT_NOT_NULL(f);

	for (int i = 0; i < multiplier; i++) {
		TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fputs(testdata_hugeStr, f));
	}

	fputc('A', f);
	errno = 0;
	while (pos <= multiplier * testdata_hugeSize) {
		TEST_ASSERT_EQUAL_INT(0, fseek(f, pos, SEEK_SET));
		errnoBefore = errno;
		TEST_ASSERT_EQUAL_INT(0, feof(f));
		TEST_ASSERT_EQUAL(errnoBefore, errno);
		pos += testdata_hugeSize / 50;
		errno++;
	}
	TEST_ASSERT_EQUAL_INT(-1, fgetc(f));
	errnoBefore = errno;
	TEST_ASSERT_EQUAL_INT(1, feof(f));
	TEST_ASSERT_EQUAL_INT(errnoBefore, errno);
	fclose(f);
	remove(filename);
}


TEST_GROUP_RUNNER(stdio_feof)
{
	RUN_TEST_CASE(stdio_feof, not_empty_all_modes);
	RUN_TEST_CASE(stdio_feof, empty_all_modes);
	RUN_TEST_CASE(stdio_feof, preserve_errno_huge_size)
}


TEST_GROUP(stdio_ftell);

TEST_SETUP(stdio_ftell)
{
}


TEST_TEAR_DOWN(stdio_ftell)
{
}


TEST(stdio_ftell, correct_position_not_empty)
{
	FILE *f;
	const char *filename = "test_stdio_ftell_not_empty";
	char *data = testdata_createCharStr(ALL_ASCII);
	int tmp;

	for (int i = 0; i < num_modes; ++i) {
		f = fopen(filename, "w+");
		TEST_ASSERT_NOT_NULL(f);
		TEST_ASSERT_NOT_NULL(data);
		TEST_ASSERT_NOT_EQUAL_INT_MESSAGE(EOF, fputs(data, f), tellMode(modes[i]));
		TEST_ASSERT_EQUAL_INT_MESSAGE(0, fclose(f), tellMode(modes[i]));

		errno = 0;
		TEST_ASSERT_EQUAL_INT32_MESSAGE(-1, (int32_t)ftell(f), tellMode(modes[i]));
		TEST_ASSERT_EQUAL_INT32_MESSAGE((int32_t)ftell(f), (int32_t)ftello(f), tellMode(modes[i]));

		f = fopen(filename, modes[i]);

		TEST_ASSERT_NOT_NULL_MESSAGE(f, tellMode(modes[i]));
		TEST_ASSERT_EQUAL_INT32_MESSAGE((int32_t)ftell(f), (int32_t)ftello(f), tellMode(modes[i]));

		if (STR_EQUAL("w", modes[i]) || STR_EQUAL("wb", modes[i])) { /* can't read in "w" mode */
			TEST_ASSERT_EQUAL_INT_MESSAGE(0, fclose(f), tellMode(modes[i]));
			TEST_ASSERT_EQUAL_MESSAGE(0, remove(filename), tellMode(modes[i]));
			continue;
		}

		TEST_ASSERT_EQUAL_INT_MESSAGE(0, fseek(f, 0, SEEK_SET), tellMode(modes[i]));

		{
			int i = 1;
			int errnoBefore;
			char c;
			while ((c = fgetc(f)) != -1) {
				errnoBefore = ++errno;
				TEST_ASSERT_EQUAL_INT32(i, (int32_t)ftell(f));
				TEST_ASSERT_EQUAL_INT32((int32_t)ftello(f), (int32_t)ftell(f));
				TEST_ASSERT_EQUAL_INT(errnoBefore, errno);
				i++;
			}
		}

		if (STR_EQUAL("w+", modes[i]) || STR_EQUAL("wb+", modes[i])) {
			TEST_ASSERT_EQUAL_INT32_MESSAGE(0, (int32_t)ftell(f), tellMode(modes[i]));
		}

		TEST_ASSERT_EQUAL_INT32_MESSAGE((int32_t)ftell(f), (int32_t)ftello(f), tellMode(modes[i]));
		TEST_ASSERT_EQUAL_INT_MESSAGE(0, fseek(f, 2, SEEK_END), tellMode(modes[i]));
		CHECK_MATCH(tmp, modes[i], "w+", "wb+");
		TEST_ASSERT_EQUAL_MESSAGE(tmp ? 2 : ALL_ASCII + 1, ftell(f), tellMode(modes[i]));
		TEST_ASSERT_EQUAL_INT32_MESSAGE((int32_t)ftell(f), (int32_t)ftello(f), tellMode(modes[i]));

		TEST_ASSERT_EQUAL_INT_MESSAGE(0, fclose(f), tellMode(modes[i]));
	}
	free(data);
	TEST_ASSERT_EQUAL_INT(0, remove(filename));
}


TEST(stdio_ftell, correct_position_empty)
{
	FILE *f;
	const char *filename = "test_stdio_ftell_empty";
	f = fopen(filename, "w+");
	TEST_ASSERT_NOT_NULL(f);
	TEST_ASSERT_EQUAL_INT(0, fclose(f));

	for (int i = 0; i < num_modes; ++i) {
		f = fopen(filename, modes[i]);
		TEST_ASSERT_NOT_NULL_MESSAGE(f, tellMode(modes[i]));
		TEST_ASSERT_EQUAL_INT32_MESSAGE(0, (int32_t)ftell(f), tellMode(modes[i]));
	}
	TEST_ASSERT_EQUAL_INT(0, remove(filename));
}


TEST(stdio_ftell, bad_file_descriptor)
{
	const char *filename = "test_stdio_bfd.txt";
	FILE *f = fopen(filename, "w+");
	TEST_ASSERT_EQUAL_INT(0, fclose(f));

	errno = 0;
	TEST_ASSERT_EQUAL_INT32(-1, (int32_t)ftell(f));
	TEST_ASSERT_EQUAL_INT(EBADF, errno);

	TEST_ASSERT_EQUAL_INT(0, remove(filename));
}


TEST(stdio_ftell, wrong_stream_type_socket)
{
	FILE *socket_stream;
	int socketfd = socket(AF_UNIX, SOCK_STREAM, 0);

	TEST_ASSERT_NOT_EQUAL_INT(-1, socketfd);

	errno = 0;
	socket_stream = fdopen(socketfd, "r");

	if (!socket_stream) {
		TEST_IGNORE_MESSAGE("#923 issue: unix sockets not supported");
	}

	errno = 0;
	TEST_ASSERT_EQUAL_INT32(-1, (int32_t)ftell(socket_stream));
	TEST_ASSERT_EQUAL_INT(ESPIPE, errno);

	errno = 0;
	TEST_ASSERT_EQUAL_INT32(-1, (int32_t)ftello(socket_stream));
	TEST_ASSERT_EQUAL_INT(ESPIPE, errno);

	TEST_ASSERT_EQUAL_INT(0, close(socketfd));
}

TEST(stdio_ftell, wrong_stream_type_pipe)
{
	errno = 0;
	int pipefd[2];
	int pipe_res = pipe(pipefd);

	TEST_ASSERT_NOT_EQUAL_INT(-1, pipe_res);

	FILE *pipe_stream_read = fdopen(pipefd[0], "r");
	FILE *pipe_stream_write = fdopen(pipefd[1], "w");

	FILE *pipe_streams[2] = { pipe_stream_read, pipe_stream_write };

	for (int i = 0; i < 2; ++i) {
		TEST_ASSERT_NOT_NULL(pipe_streams[i]);

		errno = 0;
		TEST_ASSERT_EQUAL_INT32(-1, (int32_t)ftell(pipe_streams[i]));
		TEST_ASSERT_EQUAL_INT(ESPIPE, errno);

		errno = 0;
		TEST_ASSERT_EQUAL_INT32(-1, (int32_t)ftello(pipe_streams[i]));
		TEST_ASSERT_EQUAL_INT(ESPIPE, errno);

		TEST_ASSERT_EQUAL_INT(0, fclose(pipe_streams[i]));
	}
}

TEST(stdio_ftell, wrong_stream_type_fifo)
{
	const char *fifo_path = "test_stdio_ftell_fufu";
	remove(fifo_path);
	TEST_ASSERT_NOT_EQUAL_INT(-1, mkfifo(fifo_path, S_IRWXU));

	int fifofd = open(fifo_path, O_RDONLY | O_NONBLOCK);

	TEST_ASSERT_NOT_EQUAL(-1, fifofd);

	FILE *fifo_stream = fdopen(fifofd, "r");

	TEST_ASSERT_NOT_NULL(fifo_stream);

	errno = 0;
	TEST_ASSERT_EQUAL_INT32(-1, (int32_t)ftell(fifo_stream));
	TEST_ASSERT_EQUAL(ESPIPE, errno);

	errno = 0;
	TEST_ASSERT_EQUAL_INT32(-1, (int32_t)ftello(fifo_stream));
	TEST_ASSERT_EQUAL(ESPIPE, errno);


	TEST_ASSERT_EQUAL_INT(0, fclose(fifo_stream));


	TEST_ASSERT_EQUAL(0, remove(fifo_path));
}


TEST_GROUP_RUNNER(stdio_ftell)
{
	RUN_TEST_CASE(stdio_ftell, wrong_stream_type_fifo);
	RUN_TEST_CASE(stdio_ftell, correct_position_not_empty);
	RUN_TEST_CASE(stdio_ftell, correct_position_empty);
	RUN_TEST_CASE(stdio_ftell, bad_file_descriptor);
	RUN_TEST_CASE(stdio_ftell, wrong_stream_type_socket);
	RUN_TEST_CASE(stdio_ftell, wrong_stream_type_pipe);
}


TEST_GROUP(stdio_getdelim);

TEST_SETUP(stdio_getdelim)
{
}


TEST_TEAR_DOWN(stdio_getdelim)
{
}


TEST(stdio_getdelim, existing_delim_empty_or_simple)
{
	const char *filename = "Simple_text";
	const char *expected_first[3] = { "Ie", "IIe", "III" };
	char *lineptr = NULL;
	FILE *f = fopen(filename, "w+");
	size_t len = 0;

	TEST_ASSERT_NOT_NULL(f);
	TEST_ASSERT_EQUAL_INT64(-1, getdelim(&lineptr, &len, 'Q', f));
	TEST_ASSERT_NOT_EQUAL(0, len);
	TEST_ASSERT_NOT_NULL(lineptr);

	TEST_ASSERT_NOT_EQUAL_INT(EOF, fputs("IeIIeIII", f));

	rewind(f);

	/* Write to each cell of temp_result, compare it with expected_first */
	{
		int i = 0;
		while (getdelim(&lineptr, &len, (int)'e', f) != -1) {
			TEST_ASSERT_EQUAL_STRING(expected_first[i++], lineptr);
		}
	}

	TEST_ASSERT_EQUAL_INT(0, fclose(f));
	free(lineptr);
	TEST_ASSERT_EQUAL_INT(0, remove(filename));
}


TEST(stdio_getdelim, existing_delim_long_text)
{
	const char *filename = "Long_text";
	char *lineptr = NULL;
	FILE *f = fopen(filename, "w+");
	size_t len = 0;

	TEST_ASSERT_NOT_NULL(f);
	TEST_ASSERT_NOT_EQUAL_INT(EOF, fputs(testdata_hugeStr, f));


	TEST_ASSERT_EQUAL_INT(0, fseek(f, 0, SEEK_SET));

	int i = 0;
	while (getdelim(&lineptr, &len, (int)'e', f) != -1) {
		i++;
		TEST_ASSERT_NOT_NULL(lineptr);
		TEST_ASSERT_GREATER_OR_EQUAL_INT64(1, len);
		TEST_ASSERT_GREATER_OR_EQUAL_INT64(1, strlen(lineptr));
	}

	TEST_ASSERT_GREATER_THAN(10, i);

	TEST_ASSERT_EQUAL_INT(0, fclose(f));
	free(lineptr);
	TEST_ASSERT_EQUAL_INT(0, remove(filename));
}


TEST(stdio_getdelim, too_small_buffer)
{
	char **buffer = NULL;
	const char *filename = "decently_long_test";
	size_t n = 2;
	FILE *f = fopen(filename, "w+");

	TEST_ASSERT_NOT_NULL(f);
	TEST_ASSERT_NOT_EQUAL_INT(EOF, fputs("RelativelyLongTextThatWillBeBiggerThanNInGetdelim", f));
	rewind(f);

	TEST_ASSERT_EQUAL_INT64(-1, getdelim(buffer, &n, '\n', f));
	TEST_ASSERT_NULL(buffer);
	TEST_ASSERT_EQUAL_INT(0, fclose(f));
	TEST_ASSERT_EQUAL_INT(0, remove(filename));
}


TEST(stdio_getdelim, realloc_lineptr_if_n_too_small)
{
	char *buffer = NULL;
	char *bufferBefore;
	const char *filename = "other_decently_long_test";
	size_t n = 10;
	FILE *f = fopen(filename, "w+");
	size_t n_before;

	TEST_ASSERT_NOT_NULL(f);
	TEST_ASSERT_NOT_EQUAL_INT(EOF, fputs("ABC", f));
	rewind(f);

	TEST_ASSERT_NOT_EQUAL_INT64(-1, getdelim(&buffer, &n, 'B', f));
	TEST_ASSERT_GREATER_OR_EQUAL_INT64(3, n);
	n_before = n;
	TEST_ASSERT_NOT_EQUAL_INT64(-1, getdelim(&buffer, &n, 'B', f));

	TEST_ASSERT_NOT_EQUAL_INT(EOF, fputs(testdata_hugeStr, f));
	rewind(f);

	bufferBefore = buffer;

	TEST_ASSERT_NOT_EQUAL_INT64(-1, getdelim(&buffer, &n, '\n', f));

	TEST_ASSERT_GREATER_OR_EQUAL_INT64(n_before, n);
	TEST_ASSERT_NOT_EQUAL(bufferBefore, buffer);

	TEST_ASSERT_EQUAL_INT(0, fclose(f));
	TEST_ASSERT_EQUAL_INT(0, remove(filename));
	free(buffer);
}


TEST(stdio_getdelim, every_delimiter)
{
	const char *filename = "every_delimiter.txt";
	char *lineptr = NULL, *getlinePtr = NULL;
	char *data = testdata_createCharStr(ALL_ASCII);
	size_t len = 0, getlineLen = 0;
	FILE *f = fopen(filename, "w+");

	TEST_ASSERT_NOT_NULL(data);
	TEST_ASSERT_NOT_NULL(f);
	TEST_ASSERT_NOT_EQUAL_INT(EOF, fputs(data, f));

	for (int i = 2; i < 254; ++i) {
		size_t headSize = i + 1;
		size_t tailSize = 254 - i;

		char *head = (char *)malloc(headSize + 1);
		char *tail = (char *)malloc(tailSize + 1);

		TEST_ASSERT_NOT_NULL(head);
		TEST_ASSERT_NOT_NULL(tail);

		// Copy data to head and tail
		memcpy(head, data, headSize);
		head[headSize] = '\0';
		memcpy(tail, data + i + 1, tailSize);
		tail[tailSize] = '\0';

		rewind(f);

		TEST_ASSERT_EQUAL(i + 1, getdelim(&lineptr, &len, i, f));
		TEST_ASSERT_EQUAL_STRING(head, lineptr);
		TEST_ASSERT_EQUAL(254 - i, getdelim(&lineptr, &len, i, f));
		TEST_ASSERT_EQUAL_STRING(tail, lineptr);

		free(head);
		free(tail);
	}

	rewind(f);
	TEST_ASSERT_NOT_EQUAL(-1, getline(&getlinePtr, &getlineLen, f));
	rewind(f);
	TEST_ASSERT_NOT_EQUAL(-1, getdelim(&lineptr, &len, '\n', f));

	TEST_ASSERT_EQUAL_STRING(getlinePtr, lineptr);

	free(data);
	free(lineptr);
	free(getlinePtr);
	TEST_ASSERT_EQUAL_INT(0, fclose(f));
	TEST_ASSERT_EQUAL_INT(0, remove(filename));
}


TEST(stdio_getdelim, invalid_argument_null_length)
{
	char *line = NULL;
	const char *filename = "test_einval.txt";
	size_t *len = NULL;
	ssize_t read;
	FILE *f = fopen(filename, "w+");

	TEST_ASSERT_NOT_NULL(f);
	fprintf(f, "%s", "lorem ipsum");

	read = getdelim(&line, len, 'u', f);

	TEST_ASSERT_EQUAL(-1, read);
	TEST_ASSERT_EQUAL(EINVAL, errno);

	TEST_ASSERT_EQUAL_INT(0, fclose(f));
	free(line);
	TEST_ASSERT_EQUAL_INT(0, remove(filename));
}


TEST(stdio_getdelim, getdelim_wronly)
{
	char *line = NULL;
	char *filename = "test_wronly";
	size_t len = 0;

	/* read using getdelim from write-only file */
	FILE *f = fopen(filename, "a");
	TEST_ASSERT_NOT_NULL(f);
	{
		rewind(f);
		errno = 0;
		TEST_ASSERT_EQUAL_INT(-1, getdelim(&line, &len, 'x', f));
		TEST_ASSERT_EQUAL_INT(EBADF, errno);
		/* even if line is a NULL pointer and there is nothing to read, it shall allocate even a byte for NUL termination char */
		TEST_ASSERT_NOT_NULL(line);
		free(line);
	}
	TEST_ASSERT_EQUAL(0, fclose(f));
	TEST_ASSERT_EQUAL_INT(0, remove(filename));
}

TEST(stdio_getdelim, delim_boundary_values)
{
	const char *filename = "delim_test.txt";
	char *buffer = (char *)malloc(1);
	*buffer = 'X';
	int delim = 65;
	size_t n = 1;

	FILE *stream = fopen(filename, "w+");
	TEST_ASSERT_NOT_NULL(stream);
	fputc(delim, stream);
	fseek(stream, 0, SEEK_SET);

	char *lineptr;
	lineptr = buffer;

	TEST_ASSERT_GREATER_THAN(0, getdelim(&lineptr, &n, delim, stream));

	TEST_ASSERT_EQUAL_INT(delim, 65);
	fclose(stream);
	free(lineptr);
	remove(filename);
}

TEST_GROUP_RUNNER(stdio_getdelim)
{
	RUN_TEST_CASE(stdio_getdelim, existing_delim_empty_or_simple);
	RUN_TEST_CASE(stdio_getdelim, existing_delim_long_text);
	RUN_TEST_CASE(stdio_getdelim, invalid_argument_null_length);
	RUN_TEST_CASE(stdio_getdelim, every_delimiter);
	RUN_TEST_CASE(stdio_getdelim, getdelim_wronly);
	RUN_TEST_CASE(stdio_getdelim, too_small_buffer);
	RUN_TEST_CASE(stdio_getdelim, realloc_lineptr_if_n_too_small);
	RUN_TEST_CASE(stdio_getdelim, delim_boundary_values)
}
