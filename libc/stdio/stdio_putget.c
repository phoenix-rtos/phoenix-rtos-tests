/*
 * Copyright 2023 Phoenix Systems
 * Author: Arkadiusz Kozlowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include <unity_fixture.h>


#define TMP_FILE "tmpstdin.txt"

void gets_wrapped2(char *msg, char *buf)
{
	FILE *input_stream = fopen(TMP_FILE, "w+");

	fprintf(input_stream, "%s", msg);
	rewind(input_stream);
	fflush(input_stream);

	int stdin_copy = dup(fileno(stdin));  // Save a copy of stdin

	if (input_stream == NULL) {
		perror("Error opening file");
		return;
	}

	// Redirect stdin to the input file using dup2()
	if (dup2(fileno(input_stream), fileno(stdin)) == -1) {
		perror("Error redirecting stdin");
		return;
	}

	gets(buf);

	fclose(input_stream);  // Close the file stream

	// Restore original stdin using dup2()
	if (dup2(stdin_copy, fileno(stdin)) == -1) {
		perror("Error restoring stdin");
		return;
	}

	close(stdin_copy);  // Close the copied file descriptor
}


void gets_wrapped(char *msg, char *buf)
{
	FILE *input_stream = fopen(TMP_FILE, "r+");

	if (input_stream == NULL) {
		perror("Error opening file");
		return;
	}

	fprintf(input_stream, "%s", msg);
	rewind(input_stream);

	FILE *stdin_backup = freopen(TMP_FILE, "r", stdin);  // Redirect stdin to the input file

	if (stdin_backup == NULL) {
		perror("Error redirecting stdin");
		fclose(input_stream);
		return;
	}

	gets(buf);


	fclose(input_stream);
	fclose(stdin_backup);

	freopen("/dev/tty", "r", stdin);  // Restore stdin to default (console)
}


void puts_wrapped(char *msg, char *buf)
{
	FILE *output_stream = fopen(TMP_FILE, "w+");


	rewind(output_stream);
	fflush(output_stream);

	int stdout_copy = dup(fileno(stdout));  // Save a copy of stdin

	if (output_stream == NULL) {
		perror("Error opening file");
		return;
	}

	// Redirect stdin to the input file using dup2()
	if (dup2(fileno(output_stream), fileno(stdout)) == -1) {
		perror("Error redirecting stdin");
		return;
	}

	puts(msg);

	fclose(output_stream);  // Close the file stream

	// Restore original stdin using dup2()
	if (dup2(stdout_copy, fileno(stdout)) == -1) {
		perror("Error restoring stdin");
		return;
	}


	close(stdout_copy);  // Close the copied file descriptor
}


TEST_GROUP(stdio_gets);


TEST_SETUP(stdio_gets)
{
}


TEST_TEAR_DOWN(stdio_gets)
{
}


TEST(stdio_gets, gets_basic)
{
	char res[20];
	gets_wrapped("testunio", res);
	TEST_ASSERT_EQUAL_STRING("testunio", res);
}


TEST(stdio_gets, gets_newline_in_argument)
{
	char res[20];
	gets_wrapped("1234\n5678", res);
	TEST_ASSERT_EQUAL_STRING("1234", res);
}


TEST(stdio_gets, gets_only_newlines)
{
	char res[20];
	gets_wrapped("\n\n\n\n\n\n", res);
	TEST_ASSERT_EQUAL_STRING("", res);
}


TEST(stdio_gets, gets_empty_stdin)
{
	char res[20];
	gets_wrapped("", res);
	TEST_ASSERT_EQUAL_STRING("", res);
}


TEST(stdio_gets, gets_other_escape_chars)
{
	char res[20];
	gets_wrapped("\a\b\e\f\r\t\v\\\`\"\?", res);
	TEST_ASSERT_EQUAL_STRING("\a\b\e\f\r\t\v\\\`\"\?", res);
}

TEST(stdio_gets, gets_multiple_calls)
{
	char res[20];
	gets_wrapped("ABC\nDEF\nFGH", res);
	TEST_ASSERT_EQUAL_STRING("ABC", res);

	gets(res);
	TEST_ASSERT_EQUAL_STRING("DEF", res);

	gets(res);
	TEST_ASSERT_EQUAL_STRING("FGH", res);
}


TEST_GROUP_RUNNER(stdio_gets)
{
	RUN_TEST_CASE(stdio_gets, gets_basic);
	RUN_TEST_CASE(stdio_gets, gets_newline_in_argument);
	RUN_TEST_CASE(stdio_gets, gets_only_newlines);
	RUN_TEST_CASE(stdio_gets, gets_empty_stdin);
	RUN_TEST_CASE(stdio_gets, gets_other_escape_chars);
	RUN_TEST_CASE(stdio_gets, gets_multiple_calls);
}


TEST_GROUP(stdio_puts);

TEST_SETUP(stdio_puts)
{
}


TEST_TEAR_DOWN(stdio_puts)
{
}
