/*
 * Phoenix-RTOS
 *
 * ld.elf_so tests
 *
 * Exec assert lib
 *
 * Copyright 2024 by Phoenix Systems
 * Author: Hubert Badocha
 *
 * This file is a part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <unity_fixture.h>
#include <stdlib.h>
#include <string.h>

#include "execassert.h"


static int execAssert_start(const char *path, char *const argv[], char *const envp[], FILE **fout, FILE **ferr)
{
	int fdout[2];
	int fderr[2];
	int pid;

	TEST_ASSERT_EQUAL(0, pipe(fdout));
	TEST_ASSERT_EQUAL(0, pipe(fderr));
	pid = vfork();
	TEST_ASSERT_GREATER_OR_EQUAL(0, pid);

	if (pid == 0) {
		/* Can't use TEST_ASSERT in vforked program. */
		if (dup2(fdout[1], STDOUT_FILENO) == 0) {
			fputs("FAILED: dup2(fdout[1], STDOUT_FILENO)\n", stderr);
			exit(1);
		}
		if (dup2(fderr[1], STDERR_FILENO) == 0) {
			fputs("FAILED: dup2(fderr[1], STDERR_FILENO)\n", stderr);
			exit(1);
		}

		if (close(fdout[0]) != 0) {
			fputs("FAILED: close(fdout[0])\n", stderr);
			exit(1);
		}
		if (close(fdout[1]) != 0) {
			fputs("FAILED: close(fdout[1])\n", stderr);
			exit(1);
		}
		if (close(fderr[0]) != 0) {
			fputs("FAILED: close(fderr[0])\n", stderr);
			exit(1);
		}
		if (close(fderr[1]) != 0) {
			fputs("FAILED: close(fderr[1])\n", stderr);
			exit(1);
		}

		execve(path, argv, envp);
		fputs("FAILED: execl(command, NULL)\n", stderr);
		exit(1);
	}

	TEST_ASSERT_EQUAL(0, close(fdout[1]));
	TEST_ASSERT_EQUAL(0, close(fderr[1]));

	*fout = fdopen(fdout[0], "r");
	*ferr = fdopen(fderr[0], "r");

	return pid;
}


static int execAssert_end(int pid, FILE *fout, FILE *ferr)
{
	int stat;

	TEST_ASSERT_EQUAL(0, fclose(fout));
	TEST_ASSERT_EQUAL(0, fclose(ferr));

	while (waitpid(pid, &stat, 0) == -1) {
		TEST_ASSERT_EQUAL(EINTR, errno);
	}

	return stat;
}


static void execAssert_testStream(FILE *f, const char **exp)
{
	while ((exp != NULL) && ((*exp) != NULL)) {
		size_t size = strlen(*exp) + 1;
		char *buf = malloc(size);
		TEST_ASSERT_NOT_NULL(buf);
		TEST_ASSERT_NOT_NULL_MESSAGE(fgets(buf, size, f), buf);

		TEST_ASSERT_EQUAL_STRING(*exp, buf);
		exp++;
	}
	/* Check only EOF is left in the stream. */
	TEST_ASSERT_EQUAL(EOF, fgetc(f));
}


void execAssert_execve(const char *path, char *const argv[], char *const envp[], const int *code, const char **out, const char **err)
{
	FILE *fout, *ferr;
	int pid = execAssert_start(path, argv, envp, &fout, &ferr);

	execAssert_testStream(fout, out);

	execAssert_testStream(ferr, err);

	if (code != NULL) {
		TEST_ASSERT_EQUAL(*code, execAssert_end(pid, fout, ferr));
	}
}
