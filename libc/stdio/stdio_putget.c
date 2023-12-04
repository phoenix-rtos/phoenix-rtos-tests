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

#include "testdata.h"


#define TMP_FILE "res.txt"

static FILE *output_stream;
static int stdout_copy;
static char *buf;

// static void gets_wrapped(const char *msg, char *buf)
// {
// 	FILE *input_stream = fopen(TMP_FILE, "w+");

// 	TEST_ASSERT_NOT_NULL(input_stream);

// 	fprintf(input_stream, "%s", msg);
// 	rewind(input_stream);
// 	fflush(input_stream);

// 	int stdin_copy = dup(fileno(stdin));  // Save a copy of stdin

// 	if (input_stream == NULL) {
// 		perror("Error opening file");
// 		return;
// 	}

// 	// Redirect stdin to the input file using dup2()
// 	if (dup2(fileno(input_stream), fileno(stdin)) == -1) {
// 		perror("Error redirecting stdin");
// 		return;
// 	}

// 	gets(buf);

// 	fclose(input_stream);  // Close the file stream

// 	// Restore original stdin using dup2()
// 	if (dup2(stdin_copy, fileno(stdin)) == -1) {
// 		perror("Error restoring stdin");
// 		return;
// 	}

// 	close(stdin_copy);  // Close the copied file descriptor
// }


static void gets_wrapped(const char *msg, char *buf)
{
	FILE *input_stream = freopen(TMP_FILE, "w+", stdin);  // Redirect stdin to the input file

	if (!input_stream) {
		perror("File opening");
		return;
	}

	fprintf(input_stream, "%s", msg);
	rewind(input_stream);

	TEST_ASSERT_EQUAL_CHAR(msg[0], getchar());

	fclose(input_stream);

	freopen("/dev/tty", "r", stdin);  // Restore stdin to default (console)
}


static char *puts_wrapped(const char *msg)
{
	long tmpEnd = ftell(output_stream);

	puts(msg);  // Write to the redirected stdout using puts()

	fseek(output_stream, 0, SEEK_END);
	long fileSize = ftell(output_stream);
	fseek(output_stream, tmpEnd, SEEK_SET);

	/* Allow for saving previous messages */
	buf = malloc(fileSize - tmpEnd + 1);
	buf[fileSize - tmpEnd] = '\0';
	fread(buf, fileSize - tmpEnd, 1, output_stream);
	fflush(stdout);  // Flush the redirected stdout to ensure the data is written

	return buf;
}


static void stdoutRedirect(const char *f)
{
	output_stream = fopen(f, "w+");
	stdout_copy = dup(fileno(stdout));

	if (output_stream == NULL || stdout_copy == -1)
		TEST_FAIL_MESSAGE("Error opening file");

	/* Redirect stdout to the output file using dup2() */
	if (dup2(fileno(output_stream), fileno(stdout)) == -1)
		TEST_FAIL_MESSAGE("Error redirecting stdout");

	buf = NULL;
}


static void stdoutRestore(const char *f)
{
	/* Restore original stdout using dup2() */
	if (dup2(stdout_copy, fileno(stdout)) == -1) {
		TEST_FAIL_MESSAGE("Error restoring stdout");
	}
	fflush(stdout);
	close(stdout_copy);

	free(buf);
	fclose(output_stream);
	remove(f);
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
	gets_wrapped("\a\b\e\f\r\t\v\\\"\?", res);
	TEST_ASSERT_EQUAL_STRING("\a\b\e\f\r\t\v\\\"\?", res);
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
	stdoutRedirect(TMP_FILE);
}


TEST_TEAR_DOWN(stdio_puts)
{
	stdoutRestore(TMP_FILE);
}


TEST(stdio_puts, puts_basic)
{
	buf = puts_wrapped("Some\nmessage");
	TEST_ASSERT_EQUAL_STRING("Some\nmessage\n", buf);
}

TEST(stdio_puts, puts_only_newlines)
{
	buf = puts_wrapped("\n\n\n");
	TEST_ASSERT_EQUAL_STRING("\n\n\n\n", buf);
}


TEST(stdio_puts, puts_null_terminator_in_argument)
{
	char *msg = "ABC\0DEF";
	buf = puts_wrapped(msg);
	TEST_ASSERT_EQUAL_STRING("ABC\n", buf);
	free(buf);
	buf = puts_wrapped(msg);
	TEST_ASSERT_EQUAL_STRING("ABC\n", buf);
}


TEST(stdio_puts, puts_long_text)
{

	char exp[testdata_hugeSize + 2];
	sprintf(exp, "%s\n", testdata_hugeStr);

	buf = puts_wrapped(testdata_hugeStr);
	TEST_ASSERT_EQUAL_STRING(exp, buf);
}


TEST(stdio_puts, puts_every_ascii)
{
	char exp[260];
	char *data = testdata_createCharStr(258);

	TEST_ASSERT_NOT_NULL(data);
	sprintf(exp, "%s\n", data);

	buf = puts_wrapped(data);

	TEST_ASSERT_EQUAL_STRING(exp, buf);

	free(data);
}


TEST(stdio_puts, puts_empty)
{
	buf = puts_wrapped("");
	TEST_ASSERT_EQUAL_STRING("\n", buf);
}


TEST(stdio_puts, puts_only_term_char)
{
	buf = puts_wrapped("\0");
	TEST_ASSERT_EQUAL_STRING("\n", buf);
}


TEST_GROUP_RUNNER(stdio_puts)
{
	RUN_TEST_CASE(stdio_puts, puts_basic);
	RUN_TEST_CASE(stdio_puts, puts_only_newlines);
	RUN_TEST_CASE(stdio_puts, puts_null_terminator_in_argument);
	RUN_TEST_CASE(stdio_puts, puts_long_text);
	RUN_TEST_CASE(stdio_puts, puts_every_ascii);
	RUN_TEST_CASE(stdio_puts, puts_empty);
	RUN_TEST_CASE(stdio_puts, puts_only_term_char);
}
