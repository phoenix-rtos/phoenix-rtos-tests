/*
 * Phoenix-RTOS
 *
 * libc-tests
 *
 * Testing POSIX file operations.
 *
 * Copyright 2023 Phoenix Systems
 * Author: Arkadiusz Kozlowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

/* make compilable against glibc (all these tests were run on host against libc/linux kernel) */
#define _GNU_SOURCE

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

#include <unity_fixture.h>

#include "common.h"
#include <testdata.h>

#define EQ(a, b)  !strcmp(a, b)
#define FNAME_LEN 32

static char *modes[] = { "r", "r+", "w", "w+", "a", "a+", "rb", "rb+", "wb", "wb+", "ab", "ab+" };


TEST_GROUP(stdio_feof);

TEST_SETUP(stdio_feof)
{
}


TEST_TEAR_DOWN(stdio_feof)
{
}


TEST(stdio_feof, not_empty_all_modes)
{

	for (int i = 0; i < sizeof(modes) / sizeof(char *); ++i) {
		char filename[FNAME_LEN];
		TEST_ASSERT_GREATER_OR_EQUAL(0, sprintf(filename, "file_FEOF_filled_%s", modes[i]));

		FILE *f;
		f = fopen(filename, "w+");
		char *data = testdata_createCharStr(255);

		TEST_ASSERT_NOT_NULL(f);
		TEST_ASSERT_NOT_NULL(data);

		TEST_ASSERT_NOT_EQUAL_INT(EOF, fputs(data, f));
		free(data);
		TEST_ASSERT_EQUAL_INT(0, fclose(f));

		f = fopen(filename, modes[i]);

		TEST_ASSERT_NOT_NULL(f);
		TEST_ASSERT_EQUAL_INT(0, feof(f));

		TEST_ASSERT_EQUAL_INT(0, fseek(f, 0, SEEK_SET));
		TEST_ASSERT_EQUAL_INT(0, feof(f));

		TEST_ASSERT_EQUAL_INT(0, fseek(f, 1, SEEK_END));
		fgetc(f);

		if (EQ("w", modes[i]) ||
			EQ("a", modes[i]) ||
			EQ("wb", modes[i]) ||
			EQ("ab", modes[i])) {
			TEST_ASSERT_EQUAL_MESSAGE(0, feof(f), modes[i]);
		}
		else {
			TEST_ASSERT_NOT_EQUAL_MESSAGE(0, feof(f), modes[i]);
		}

		TEST_ASSERT_EQUAL_INT(0, fseek(f, 0, SEEK_SET));

		TEST_ASSERT_EQUAL_INT(0, feof(f));

		while (fgetc(f) != EOF) {
			continue;
		}

		if (EQ("w", modes[i]) ||
			EQ("a", modes[i]) ||
			EQ("wb", modes[i]) ||
			EQ("ab", modes[i])) {
			TEST_ASSERT_EQUAL_INT_MESSAGE(0, feof(f), modes[i]);
		}
		else {
			TEST_ASSERT_NOT_EQUAL_MESSAGE(0, feof(f), modes[i]);
		}

		TEST_ASSERT_EQUAL_INT(0, fclose(f));
		TEST_ASSERT_EQUAL_INT(0, remove(filename));
	}
}

TEST(stdio_feof, empty_all_modes)
{
	for (int i = 0; i < sizeof(modes) / sizeof(char *); ++i) {
		FILE *f;
		char filename[FNAME_LEN];

		TEST_ASSERT_GREATER_OR_EQUAL(0, sprintf(filename, "file_FEOF_empty_%s", modes[i]));

		f = fopen(filename, "w+");
		TEST_ASSERT_EQUAL_INT(0, fclose(f));
		TEST_ASSERT_NOT_NULL(f);

		f = fopen(filename, modes[i]);
		TEST_ASSERT_NOT_NULL(f);

		TEST_ASSERT_EQUAL_INT(0, feof(f));

		fgetc(f);

		if (EQ("a", modes[i]) ||
			EQ("w", modes[i]) ||
			EQ("ab", modes[i]) ||
			EQ("wb", modes[i])) {
			TEST_ASSERT_EQUAL_INT_MESSAGE(0, feof(f), modes[i]);
		}
		else {
			TEST_ASSERT_NOT_EQUAL_INT_MESSAGE(0, feof(f), modes[i]);
		}

		TEST_ASSERT_EQUAL_INT(0, fclose(f));
		TEST_ASSERT_EQUAL_INT(0, feof(f));

		TEST_ASSERT_EQUAL(0, remove(filename));
		TEST_ASSERT_EQUAL_INT(0, feof(f));
	}
}


TEST(stdio_feof, preserve_errno_huge_size)
{
	FILE *f = fopen("errno_preserve", "w+");
	TEST_ASSERT_NOT_NULL(f);

	for (int i = 0; i < 10; i++) {
		TEST_ASSERT_GREATER_OR_EQUAL_INT(0, fputs(testdata_hugeStr, f));
	}

	{
		long pos = 0;
		errno = 0;
		while (pos < 10 * testdata_hugeSize) {
			TEST_ASSERT_EQUAL_INT(0, fseek(f, pos, SEEK_SET));
			int errnoBefore = errno;
			TEST_ASSERT_EQUAL_INT(0, feof(f));
			TEST_ASSERT_EQUAL(errnoBefore, errno);
			pos += 97;
			errno++;
		}
	}

	remove("errno_preserve");
	fclose(f);
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

	for (int i = 0; i < sizeof(modes) / sizeof(char *); ++i) {

		char filename[FNAME_LEN];
		TEST_ASSERT_GREATER_OR_EQUAL(0, sprintf(filename, "file_FTELL_filled_%s", modes[i]));

		f = fopen(filename, "w+");
		char *data = testdata_createCharStr(255);
		TEST_ASSERT_NOT_EQUAL_INT(EOF, fputs(data, f));
		free(data);
		TEST_ASSERT_EQUAL_INT(0, fclose(f));

		errno = 0;
		TEST_ASSERT_EQUAL(-1, ftell(f));
		TEST_ASSERT_EQUAL(ftell(f), ftello(f));
		TEST_ASSERT_EQUAL(EBADF, errno);

		f = fopen(filename, modes[i]);

		TEST_ASSERT_NOT_NULL(f);
		TEST_ASSERT_EQUAL(ftell(f), ftello(f));

		if (EQ("w", modes[i]) || EQ("wb", modes[i])) { /* can't read in "w" mode */
			TEST_ASSERT_EQUAL_INT(0, fclose(f));
			TEST_ASSERT_EQUAL(0, remove(filename));
			continue;
		}

		while (fgetc(f) != EOF)
			continue;

		if (EQ("w+", modes[i]) || EQ("wb+", modes[i]))
			TEST_ASSERT_EQUAL(0, ftell(f));
		else
			TEST_ASSERT_EQUAL_INT_MESSAGE(254, ftell(f), modes[i]);

		TEST_ASSERT_EQUAL(ftell(f), ftello(f));


		if (EQ("w+", modes[i]) || EQ("wb+", modes[i])) {
			TEST_ASSERT_EQUAL(0, ftell(f));
		}
		else {
			TEST_ASSERT_EQUAL_INT_MESSAGE(254, ftell(f), modes[i]);
			TEST_ASSERT_EQUAL_INT(0, fseek(f, 2, SEEK_END));
			TEST_ASSERT_EQUAL(256, ftell(f));
		}
		TEST_ASSERT_EQUAL(ftell(f), ftello(f));


		TEST_ASSERT_EQUAL_INT(0, fclose(f));
		TEST_ASSERT_EQUAL(0, remove(filename));
	}
}


TEST(stdio_ftell, correct_position_empty)
{
	FILE *f;

	for (int i = 0; i < sizeof(modes) / sizeof(char *); ++i) {

		char filename[FNAME_LEN];
		TEST_ASSERT_GREATER_OR_EQUAL(0, sprintf(filename, "file_FTELL_empty_%s", modes[i]));

		f = fopen(filename, "w+");
		TEST_ASSERT_EQUAL_INT(0, fclose(f));

		f = fopen(filename, modes[i]);

		TEST_ASSERT_EQUAL(0, ftell(f));

		TEST_ASSERT_EQUAL(0, remove(filename));
	}
}


TEST(stdio_ftell, bad_file_descriptor)
{
	errno = 0;
	char filename[] = "test_bfd.txt";
	FILE *f = fopen(filename, "w+");
	TEST_ASSERT_EQUAL_INT(0, fclose(f));
	TEST_ASSERT_EQUAL(-1, ftell(f));
	TEST_ASSERT_EQUAL(EBADF, errno);
	TEST_ASSERT_EQUAL(0, remove(filename));
	errno = 0;
}


TEST(stdio_ftell, wrong_stream_type)
{
	TEST_IGNORE_MESSAGE("Issue 923");
	errno = 0;

	int socketfd = socket(AF_UNIX, SOCK_STREAM, 0);

	TEST_ASSERT_NOT_EQUAL(-1, socketfd);

	errno = 0;
	FILE *socket_stream = fdopen(socketfd, "w+");

	TEST_ASSERT_NOT_NULL_MESSAGE(socket_stream, strerror(errno));

	errno = 0;
	TEST_ASSERT_EQUAL(-1, ftell(socket_stream));
	TEST_ASSERT_EQUAL(ESPIPE, errno);

	errno = 0;
	TEST_ASSERT_EQUAL(-1, ftello(socket_stream));
	TEST_ASSERT_EQUAL(ESPIPE, errno);

	TEST_ASSERT_EQUAL_INT(0, fclose(socket_stream));
	TEST_ASSERT_NOT_EQUAL(EOF, close(socketfd));


	errno = 0;
	int pipefd[2];
	int pipe_res = pipe(pipefd);

	TEST_ASSERT_NOT_EQUAL(-1, pipe_res);

	FILE *pipe_stream = fdopen(pipefd[0], "r");
	TEST_ASSERT_NOT_NULL(pipe_stream);

	errno = 0;
	TEST_ASSERT_EQUAL(-1, ftell(pipe_stream));
	TEST_ASSERT_EQUAL(ESPIPE, errno);

	errno = 0;
	TEST_ASSERT_EQUAL(-1, ftello(pipe_stream));
	TEST_ASSERT_EQUAL(ESPIPE, errno);

	TEST_ASSERT_EQUAL_INT(0, fclose(pipe_stream));
	TEST_ASSERT_NOT_EQUAL(EOF, close(pipefd[0]));
	TEST_ASSERT_NOT_EQUAL(EOF, close(pipefd[1]));


	const char *fifo_path = "myfifo";
	TEST_ASSERT_NOT_EQUAL(-1, mkfifo(fifo_path, 0666));

	pid_t pid = fork();
	TEST_ASSERT_NOT_EQUAL(-1, pid);

	if (!pid) {
		int fifofd = open(fifo_path, O_WRONLY);

		if (fifofd == -1) {
			exit(EXIT_FAILURE);
		}

		if (write(fifofd, "1234", 4) == -1) {
			close(fifofd);
			exit(EXIT_FAILURE);
		}

		if (close(fifofd) == EOF) {
			exit(EXIT_FAILURE);
		}

		exit(EXIT_SUCCESS);
	}
	else {
		int status;
		wait(&status);
		TEST_ASSERT_EQUAL_INT(EXIT_SUCCESS, status);

		int fifofd = open(fifo_path, O_RDWR);

		TEST_ASSERT_NOT_EQUAL(-1, fifofd);

		FILE *fifo_stream = fdopen(fifofd, "w+");

		TEST_ASSERT_NOT_NULL(fifo_stream);

		TEST_ASSERT_NOT_EQUAL(EOF, fflush(fifo_stream));

		errno = 0;
		TEST_ASSERT_EQUAL(-1, ftell(fifo_stream));
		TEST_ASSERT_EQUAL(ESPIPE, errno);

		errno = 0;
		TEST_ASSERT_EQUAL(-1, ftello(fifo_stream));
		TEST_ASSERT_EQUAL(ESPIPE, errno);


		TEST_ASSERT_EQUAL_INT(0, fclose(fifo_stream));
		TEST_ASSERT_NOT_EQUAL(EOF, close(fifofd));
	}


	TEST_ASSERT_EQUAL(0, remove(fifo_path));
}


TEST_GROUP_RUNNER(stdio_ftell)
{
	RUN_TEST_CASE(stdio_ftell, correct_position_not_empty);
	RUN_TEST_CASE(stdio_ftell, correct_position_empty);
	RUN_TEST_CASE(stdio_ftell, bad_file_descriptor);
	RUN_TEST_CASE(stdio_ftell, wrong_stream_type);
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
	FILE *f = fopen(filename, "w+");

	char *lineptr = NULL;
	size_t len = 0;

	TEST_ASSERT_EQUAL_INT64(-1, getdelim(&lineptr, &len, 'Q', f));
	TEST_ASSERT_NOT_EQUAL(0, len);
	TEST_ASSERT_NOT_NULL(lineptr);

	TEST_ASSERT_NOT_EQUAL_INT(EOF, fputs("IeIIeIII", f));
	char *expected_first[3] = { "Ie", "IIe", "III" };


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
	FILE *f = fopen(filename, "w+");

	TEST_ASSERT_NOT_NULL(f);
	TEST_ASSERT_NOT_EQUAL_INT(EOF, fputs(testdata_hugeStr, f));

	char *lineptr = NULL;

	TEST_ASSERT_EQUAL_INT(0, fseek(f, 0, SEEK_SET));
	size_t len = 0;

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
	size_t n;
	size_t *counter = &n;
	*counter = 2;

	FILE *f = fopen("decently_long_test", "w+");
	TEST_ASSERT_NOT_EQUAL_INT(EOF, fputs("RelativelyLongTextThatWillBeBiggerThanNInGetdelim", f));
	rewind(f);

	TEST_ASSERT_EQUAL_INT64(-1, getdelim(buffer, counter, '\n', f));
	TEST_ASSERT_NULL(buffer);
	TEST_ASSERT_EQUAL_INT(0, fclose(f));
	TEST_ASSERT_EQUAL_INT(0, remove("decently_long_test"));
}


TEST(stdio_getdelim, realloc_lineptr_if_n_too_small)
{
	char *buffer = NULL;
	size_t n = 10;
	size_t *counter = &n;
	FILE *f = fopen("other_decently_long_test", "w+");

	TEST_ASSERT_NOT_NULL(f);
	TEST_ASSERT_NOT_EQUAL_INT(EOF, fputs("ABC", f));
	rewind(f);

	TEST_ASSERT_NOT_EQUAL_INT64(-1, getdelim(&buffer, counter, 'B', f));
	TEST_ASSERT_GREATER_OR_EQUAL_INT64(3, n);

	TEST_ASSERT_NOT_EQUAL_INT64(-1, getdelim(&buffer, counter, 'B', f));

	TEST_ASSERT_NOT_EQUAL_INT(EOF, fputs(testdata_hugeStr, f));
	rewind(f);

	char *bufferBefore = buffer;
	errno = 0;

	size_t n_before = n;

	TEST_ASSERT_NOT_EQUAL_INT64(-1, getdelim(&buffer, counter, '\n', f));

	TEST_ASSERT_GREATER_OR_EQUAL_INT64(n_before, n);
	TEST_ASSERT_NOT_EQUAL(bufferBefore, buffer);

	TEST_ASSERT_EQUAL_INT(0, fclose(f));
	TEST_ASSERT_EQUAL_INT(0, remove("other_decently_long_test"));
	free(buffer);
}


TEST(stdio_getdelim, every_delimiter)
{
	const char *filename = "every_delimiter.txt";
	char *lineptr = NULL, *getlinePtr = NULL;
	char *data = testdata_createCharStr(256);
	size_t len = 0, getlineLen = 0;
	FILE *f = fopen(filename, "w+");

	TEST_ASSERT_NOT_NULL(data);
	TEST_ASSERT_NOT_NULL(f);
	TEST_ASSERT_NOT_EQUAL_INT(EOF, fputs(data, f));

	for (int i = 3; i < 254; ++i) {

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
	FILE *file = fopen("test_einval.txt", "w+");
	fprintf(file, "%s", "lorem ipsum");

	TEST_ASSERT_NOT_NULL(file);


	char *line = NULL;
	size_t *len = NULL;
	ssize_t read;

	read = getdelim(&line, len, 'u', file);

	TEST_ASSERT_EQUAL(-1, read);
	TEST_ASSERT_EQUAL(EINVAL, errno);

	TEST_ASSERT_EQUAL_INT(0, fclose(file));
	free(line);
	TEST_ASSERT_EQUAL_INT(0, remove("test_einval.txt"));
}


TEST(stdio_getdelim, getdelim_wronly)
{
	char *line = NULL;
	size_t len = 0;

	/* read using getdelim from write-only file */
	FILE *f = fopen("test_wronly", "a");
	TEST_ASSERT_NOT_NULL(f);

	{
		rewind(f);
		TEST_ASSERT_EQUAL_INT(-1, getdelim(&line, &len, 'x', f));
		TEST_ASSERT_EQUAL_INT(EBADF, errno);
		/* even if line is a NULL pointer and there is nothing to read, it shall allocate even a byte for NUL termination char */
		TEST_ASSERT_NOT_NULL(line);
		free(line);
	}

	TEST_ASSERT_EQUAL_INT(0, remove("test_wronly"));
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
}
