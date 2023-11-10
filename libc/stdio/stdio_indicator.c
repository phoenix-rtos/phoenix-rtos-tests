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
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>

#include <unity_fixture.h>

#include "common.h"
#include <testdata.h>

#define EQ(a, b) !strcmp(a, b)


TEST_GROUP(stdio_feof);

TEST_SETUP(stdio_feof)
{
}


TEST_TEAR_DOWN(stdio_feof)
{
}


TEST(stdio_feof, open_and_closed_not_empty_file_every_mode)
{
	char *modes[] = { "r", "r+", "w", "w+", "a", "a+", "rb", "rb+", "wb", "wb+", "ab", "ab+" };

	for (int i = 0; i < 12; ++i) {
		char filename[32];
		sprintf(filename, "file_FEOF_filled_%s", modes[i]);

		FILE *f;
		f = fopen(filename, "w+");
		char *data = testdata_createCharStr(255);
		fputs(data, f);
		free(data);
		fclose(f);

		f = fopen(filename, modes[i]);

		TEST_ASSERT_EQUAL(0, feof(f));


		fseek(f, 0, SEEK_SET);
		TEST_ASSERT_EQUAL(0, feof(f));
		fseek(f, 1, SEEK_END);
		fgetc(f);

		if (EQ("w", modes[i]) ||
			EQ("wb", modes[i]) ||
			EQ("a", modes[i]) ||
			EQ("ab", modes[i]))
			TEST_ASSERT_EQUAL(0, feof(f));
		else
			TEST_ASSERT_NOT_EQUAL_MESSAGE(0, feof(f), modes[i]);

		fseek(f, 0, SEEK_SET);
		char buffer[260];

		while (fgetc(f) != EOF)
			continue;

		if (EQ("w", modes[i]) ||
			EQ("wb", modes[i]) ||
			EQ("a", modes[i]) ||
			EQ("ab", modes[i])) /* not w and not wb */
			TEST_ASSERT_EQUAL_MESSAGE(0, feof(f), modes[i]);
		else
			TEST_ASSERT_NOT_EQUAL_MESSAGE(0, feof(f), modes[i]);

		fclose(f);
		remove(filename);
	}
}

TEST(stdio_feof, open_and_closed_empty_file_every_mode)
{
	char *modes[] = { "r", "r+", "w", "w+", "a", "a+", "rb", "rb+", "wb", "wb+", "ab", "ab+" };

	for (int i = 0; i < 12; ++i) {

		char filename[32];
		sprintf(filename, "file_FEOF_empty_%s", modes[i]);

		FILE *f;
		f = fopen(filename, "w+");
		fclose(f);

		f = fopen(filename, modes[i]);
		if (!f)
			continue;

		TEST_ASSERT_EQUAL(0, feof(f));

		fgetc(f);

		if (EQ("a", modes[i]) ||
			EQ("ab", modes[i]) ||
			EQ("w", modes[i]) ||
			EQ("wb", modes[i]))
			TEST_ASSERT_EQUAL_MESSAGE(0, feof(f), modes[i]);
		else
			TEST_ASSERT_NOT_EQUAL_MESSAGE(0, feof(f), modes[i]);

		fclose(f);
		TEST_ASSERT_EQUAL(0, feof(f));

		remove(filename);
		TEST_ASSERT_EQUAL(0, feof(f));
	}
}


TEST(stdio_feof, standard_streams_read)
{
	FILE *streams[] = { stdin, stdout, stderr };

	for (int i = 0; i < 3; ++i) {
		fprintf(streams[i], "Feof stream test\n");
		int character;

		while ((character = fgetc(streams[i])) != EOF) {
			fputc(character, stdout);
		}

		TEST_ASSERT_EQUAL(0, feof(streams[i]));
		fflush(stdout);
	}
}


TEST_GROUP_RUNNER(stdio_feof)
{
	RUN_TEST_CASE(stdio_feof, open_and_closed_not_empty_file_every_mode);
	RUN_TEST_CASE(stdio_feof, open_and_closed_empty_file_every_mode);

	// RUN_TEST_CASE(feof, standard_streams_read);
}


TEST_GROUP(stdio_ftell);

TEST_SETUP(stdio_ftell)
{
}


TEST_TEAR_DOWN(stdio_ftell)
{
}


TEST(stdio_ftell, correct_position_not_empty_file)
{
	char *modes[] = { "r", "r+", "w", "w+", "a", "a+", "rb", "rb+", "wb", "wb+", "ab", "ab+" };
	FILE *f;

	for (int i = 0; i < 12; ++i) {

		char filename[32];
		sprintf(filename, "file_FTELL_filled_%s", modes[i]);

		f = fopen(filename, "w+");
		char *data = testdata_createCharStr(255);
		fputs(data, f);
		free(data);
		fclose(f);

		errno = 0;
		TEST_ASSERT_EQUAL(-1, ftell(f));
		TEST_ASSERT_EQUAL(ftell(f), ftello(f));
		TEST_ASSERT_EQUAL(EBADF, errno);
		errno = 0;

		f = fopen(filename, modes[i]);

		if (!f)
			TEST_MESSAGE("File couldn't be created");

		if (EQ(modes[i], "a") || EQ(modes[i], "ab"))
			TEST_ASSERT_EQUAL(254, ftell(f));
		else
			TEST_ASSERT_EQUAL(0, ftell(f));

		TEST_ASSERT_EQUAL(ftell(f), ftello(f));

		if (EQ("w", modes[i]) || EQ("wb", modes[i])) { /* can't read in "w" mode */
			fclose(f);
			remove(filename);
			continue;
		}

		while (fgetc(f) != EOF)
			continue;

		if (EQ("w+", modes[i]) || EQ("wb+", modes[i]))
			TEST_ASSERT_EQUAL(0, ftell(f));
		else
			TEST_ASSERT_EQUAL_MESSAGE(254, ftell(f), modes[i]);

		TEST_ASSERT_EQUAL(ftell(f), ftello(f));


		if (EQ("w+", modes[i]) || EQ("wb+", modes[i])) {
			TEST_ASSERT_EQUAL(0, ftell(f));
		}
		else {
			TEST_ASSERT_EQUAL_MESSAGE(254, ftell(f), modes[i]);
			fseek(f, 2, SEEK_END);
			TEST_ASSERT_EQUAL(256, ftell(f));
		}
		TEST_ASSERT_EQUAL(ftell(f), ftello(f));


		fclose(f);
		remove(filename);
	}
}


TEST(stdio_ftell, correct_position_empty_file)
{
	char *modes[] = { "r", "r+", "w", "w+", "a", "a+", "rb", "rb+", "wb", "wb+", "ab", "ab+" };
	FILE *f;

	for (int i = 0; i < 12; ++i) {

		char filename[32];
		sprintf(filename, "file_FTELL_empty_%s", modes[i]);

		f = fopen(filename, "w+");
		fclose(f);

		f = fopen(filename, modes[i]);

		TEST_ASSERT_EQUAL(0, ftell(f));

		remove(filename);
	}
}


TEST(stdio_ftell, standard_streams_error)
{
	/* Not called on purpose */
	FILE *streams[] = { stdin, stdout, stderr };

	for (int i = 0; i < 3; ++i) {
		fprintf(streams[i], "Ftell Std stream test\n");
		int character;

		while ((character = fgetc(streams[i])) != EOF) {
			fputc(character, stdout);
		}
		fflush(stdout);

		TEST_ASSERT_EQUAL(EBADF, errno);
		TEST_ASSERT_EQUAL(-1, ftell(streams[i]));
		TEST_ASSERT_EQUAL(ftello(streams[i]), ftell(streams[i]));
	}
}

TEST(stdio_ftell, bad_file_descriptor)
{
	errno = 0;
	char filename[] = "test_bfd.txt";
	FILE *f = fopen(filename, "w+");
	fclose(f);
	TEST_ASSERT_EQUAL(-1, ftell(f));
	TEST_ASSERT_EQUAL(EBADF, errno);
	remove(filename);
	errno = 0;
}


TEST(stdio_ftell, wrong_stream_type)
{
	errno = 0;

	int socketfd = socket(AF_INET, SOCK_STREAM, 0);
	TEST_ASSERT_NOT_EQUAL_MESSAGE(-1, socketfd, "Socket creation failed");

	FILE *socket_stream = fdopen(socketfd, "w+");

	errno = 0;
	TEST_ASSERT_EQUAL(-1, ftell(socket_stream));
	TEST_ASSERT_EQUAL(ESPIPE, errno);

	errno = 0;
	TEST_ASSERT_EQUAL(-1, ftello(socket_stream));
	TEST_ASSERT_EQUAL(ESPIPE, errno);

	fclose(socket_stream);
	close(socketfd);


	errno = 0;
	chmod(".", 0777);
	const char *fifo_path = "myfifo";
	int fifo_res = mkfifo(fifo_path, 0666);


	int fifofd = open(fifo_path, O_RDWR);

	TEST_ASSERT_NOT_EQUAL(-1, fifofd);

	FILE *fifo_stream = fdopen(fifofd, "r+");

	TEST_ASSERT_NOT_NULL(fifo_stream);

	errno = 0;
	TEST_ASSERT_EQUAL(-1, ftell(fifo_stream));
	TEST_ASSERT_EQUAL(ESPIPE, errno);

	errno = 0;
	TEST_ASSERT_EQUAL(-1, ftello(fifo_stream));
	TEST_ASSERT_EQUAL(ESPIPE, errno);


	fclose(fifo_stream);
	close(fifofd);


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


	fclose(pipe_stream);
	close(pipefd[0]);
	close(pipefd[1]);
	remove(fifo_path);
}


TEST_GROUP_RUNNER(stdio_ftell)
{
	RUN_TEST_CASE(stdio_ftell, correct_position_not_empty_file);
	RUN_TEST_CASE(stdio_ftell, correct_position_empty_file);
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


TEST(stdio_getdelim, existing_delimiter_read_from_file_empty_or_simple_text)
{
	const char *filename = "Simple_text";
	FILE *f = fopen(filename, "w+");


	fputs("IeIIeIII", f);
	char *expected_first[3] = { "Ie", "IIe", "III" };
	char temp_result[4][4];

	char *lineptr = NULL;
	size_t len = 0;

	TEST_ASSERT_EQUAL(-1, getdelim(lineptr, &len, 'Q', f));
	TEST_ASSERT_EQUAL(0, len);
	TEST_ASSERT_NULL(lineptr);

	rewind(f);

	/* Write to each cell of temp_result, compare it with expected_first */
	int i = 0;
	while (getdelim(&lineptr, &len, (int)'e', f) != -1) {
		TEST_ASSERT_EQUAL_STRING(expected_first[i++], lineptr);
	}

	fclose(f);
	free(lineptr);
	remove(filename);
}


TEST(stdio_getdelim, existing_delimiter_read_from_long_text)
{
	const char *filename = "Long_text";
	FILE *f = fopen(filename, "w+");

	fputs(testdata_hugeStr, f);

	char *lineptr = NULL;

	fseek(f, 0, SEEK_SET);
	size_t len = 0;

	int i = 0;
	while (getdelim(&lineptr, &len, (int)'e', f) != -1) {
		i++;
		TEST_ASSERT_NOT_NULL(lineptr);
		TEST_ASSERT_GREATER_OR_EQUAL(1, len);
		TEST_ASSERT_GREATER_OR_EQUAL(1, strlen(lineptr));
	}

	TEST_ASSERT_GREATER_THAN(10, i);

	fclose(f);
	free(lineptr);
	remove(filename);
}


TEST(stdio_getdelim, too_small_buffer)
{
	char **buffer;
	int n;
	int *counter = &n;
	*counter = 2;

	FILE *f = fopen("decently_long_test", "w+");
	fputs("RelativelyLongTextThatWillBeBiggerThanNInGetdelim", f);
	rewind(f);

	TEST_ASSERT_EQUAL(-1, getdelim(buffer, counter, '\n', f));
	TEST_ASSERT_NULL(buffer);
	fclose(f);
	remove("decently_long_test");
}


TEST(stdio_getdelim, realloc_lineptr_if_not_big_enough)
{
	char *buffer = NULL;

	size_t n = 10;
	size_t *counter = &n;

	FILE *f = fopen("other_decently_long_test", "w+");

	fputs("ABC", f);
	rewind(f);

	TEST_ASSERT_NOT_EQUAL_MESSAGE(-1, getdelim(&buffer, counter, 'B', f), strerror(errno));
	TEST_ASSERT_GREATER_OR_EQUAL(3, n);

	TEST_ASSERT_NOT_EQUAL(-1, getdelim(&buffer, counter, 'B', f));

	fputs(testdata_hugeStr, f);
	rewind(f);

	char *bufferBefore = buffer;
	errno = 0;

	size_t n_before = n;

	TEST_ASSERT_NOT_EQUAL_MESSAGE(-1, getdelim(&buffer, counter, '\n', f), strerror(errno));

	TEST_ASSERT_GREATER_OR_EQUAL(n_before, n);
	TEST_ASSERT_NOT_EQUAL(bufferBefore, buffer);

	fclose(f);
	remove("other_decently_long_test");
	free(buffer);
}


TEST(stdio_getdelim, every_delimiter)
{
	const char *filename = "every_delimiter.txt";
	char *lineptr = NULL, *getlinePtr = NULL;
	char *data = testdata_createCharStr(256);
	size_t len = 0, getlineLen = 0;

	FILE *f = fopen(filename, "w+");
	fprintf(f, data);

	for (int i = 3; i < 254; ++i) {

		size_t headSize = i + 1;
		size_t tailSize = 254 - i;

		char *head = (char *)malloc(headSize + 1);
		char *tail = (char *)malloc(tailSize + 1);

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
	getline(&getlinePtr, &getlineLen, f);
	rewind(f);
	getdelim(&lineptr, &len, '\n', f);

	TEST_ASSERT_EQUAL_STRING(getlinePtr, lineptr);

	free(data);
	free(lineptr);
	free(getlinePtr);
	fclose(f);
	remove(filename);
}


TEST(stdio_getdelim, invalid_argument_null_length)
{
	FILE *file = fopen("test_einval.txt", "w+");
	fprintf(file, "lorem ipsum");

	TEST_ASSERT_NOT_NULL(file);


	char *line = NULL;
	size_t *len = NULL;
	ssize_t read;

	read = getdelim(&line, len, 'u', file);

	TEST_ASSERT_EQUAL(-1, read);
	TEST_ASSERT_EQUAL(EINVAL, errno);

	fclose(file);
	free(line);
	remove("test_einval.txt");
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

	remove("test_wronly");
}


TEST_GROUP_RUNNER(stdio_getdelim)
{
	RUN_TEST_CASE(stdio_getdelim, existing_delimiter_read_from_file_empty_or_simple_text);
	RUN_TEST_CASE(stdio_getdelim, existing_delimiter_read_from_long_text);
	RUN_TEST_CASE(stdio_getdelim, invalid_argument_null_length);
	RUN_TEST_CASE(stdio_getdelim, every_delimiter);
	RUN_TEST_CASE(stdio_getdelim, getdelim_wronly);
	RUN_TEST_CASE(stdio_getdelim, too_small_buffer);
	RUN_TEST_CASE(stdio_getdelim, realloc_lineptr_if_not_big_enough);
}
