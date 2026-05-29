/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <unistd.h>
 * TESTED:
 *    - fexecve()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>

#include "unity_fixture.h"

#define TRUE_PATH "/usr/bin/true"
#define FALSE_PATH "/usr/bin/false"


TEST_GROUP(proc_fexecve);

TEST_SETUP(proc_fexecve) {}

TEST_TEAR_DOWN(proc_fexecve) {}


TEST(proc_fexecve, fexecve_executes_program)
{
	pid_t pid;
	int status;
	int fd;

	fd = open(TRUE_PATH, O_RDONLY);
	TEST_ASSERT_TRUE(fd >= 0);

	pid = fork();
	TEST_ASSERT_TRUE(pid >= 0);

	if (pid == 0) {
		char *argv[] = { "true", NULL };
		char *envp[] = { NULL };

		fexecve(fd, argv, envp);
		/* If fexecve returns, it failed */
		_exit(99);
	}

	close(fd);

	pid = waitpid(pid, &status, 0);
	TEST_ASSERT_TRUE(pid > 0);
	TEST_ASSERT_TRUE(WIFEXITED(status));
	TEST_ASSERT_EQUAL_INT(0, WEXITSTATUS(status));
}


TEST(proc_fexecve, fexecve_exit_code_nonzero)
{
	pid_t pid;
	int status;
	int fd;

	fd = open(FALSE_PATH, O_RDONLY);
	TEST_ASSERT_TRUE(fd >= 0);

	pid = fork();
	TEST_ASSERT_TRUE(pid >= 0);

	if (pid == 0) {
		char *argv[] = { "false", NULL };
		char *envp[] = { NULL };

		fexecve(fd, argv, envp);
		_exit(99);
	}

	close(fd);

	pid = waitpid(pid, &status, 0);
	TEST_ASSERT_TRUE(pid > 0);
	TEST_ASSERT_TRUE(WIFEXITED(status));
	TEST_ASSERT_EQUAL_INT(1, WEXITSTATUS(status));
}


TEST(proc_fexecve, fexecve_passes_environment)
{
	pid_t pid;
	int status;
	int fd;
	int pipeFds[2];
	int ret;

	/* Use /usr/bin/env or /bin/sh to check env passing.
	 * We'll use /bin/sh -c 'echo $TEST_VAR' and read from a pipe. */
	fd = open("/bin/sh", O_RDONLY);
	TEST_ASSERT_TRUE(fd >= 0);

	ret = pipe(pipeFds);
	TEST_ASSERT_EQUAL_INT(0, ret);

	pid = fork();
	TEST_ASSERT_TRUE(pid >= 0);

	if (pid == 0) {
		char *argv[] = { "sh", "-c", "printf '%s' \"$TEST_FEXECVE_VAR\"", NULL };
		char *envp[] = { "TEST_FEXECVE_VAR=hello123", NULL };

		close(pipeFds[0]);
		/* Redirect stdout to pipe */
		if (dup2(pipeFds[1], STDOUT_FILENO) < 0) {
			_exit(98);
		}
		close(pipeFds[1]);

		fexecve(fd, argv, envp);
		_exit(99);
	}

	close(fd);
	close(pipeFds[1]);

	{
		char buf[64];
		ssize_t n = read(pipeFds[0], buf, sizeof(buf) - 1);
		TEST_ASSERT_TRUE(n > 0);
		buf[n] = '\0';
		TEST_ASSERT_EQUAL_STRING("hello123", buf);
	}
	close(pipeFds[0]);

	pid = waitpid(pid, &status, 0);
	TEST_ASSERT_TRUE(pid > 0);
	TEST_ASSERT_TRUE(WIFEXITED(status));
	TEST_ASSERT_EQUAL_INT(0, WEXITSTATUS(status));
}


TEST(proc_fexecve, fexecve_ebadf)
{
	pid_t pid;
	int status;

	pid = fork();
	TEST_ASSERT_TRUE(pid >= 0);

	if (pid == 0) {
		char *argv[] = { "true", NULL };
		char *envp[] = { NULL };

		errno = 0;
		fexecve(-1, argv, envp);
		/* fexecve should have failed with EBADF or EINVAL */
		if (errno == EBADF || errno == EINVAL) {
			_exit(0);
		}
		_exit(1);
	}

	pid = waitpid(pid, &status, 0);
	TEST_ASSERT_TRUE(pid > 0);
	TEST_ASSERT_TRUE(WIFEXITED(status));
	TEST_ASSERT_EQUAL_INT(0, WEXITSTATUS(status));
}


TEST(proc_fexecve, fexecve_eacces_not_executable)
{
	pid_t pid;
	int status;
	int fd;
	const char *tmpFile = "/tmp/test_fexecve_noexec";
	int tmpFd;

	/* Create a non-executable file */
	unlink(tmpFile);
	tmpFd = open(tmpFile, O_RDWR | O_CREAT | O_TRUNC, 0644);
	TEST_ASSERT_TRUE(tmpFd >= 0);
	if (write(tmpFd, "#!/bin/sh\ntrue\n", 15) < 0) {
		TEST_FAIL_MESSAGE("write failed");
	}
	close(tmpFd);

	fd = open(tmpFile, O_RDONLY);
	TEST_ASSERT_TRUE(fd >= 0);

	pid = fork();
	TEST_ASSERT_TRUE(pid >= 0);

	if (pid == 0) {
		char *argv[] = { "noexec", NULL };
		char *envp[] = { NULL };

		errno = 0;
		fexecve(fd, argv, envp);
		if (errno == EACCES) {
			_exit(0);
		}
		_exit(1);
	}

	close(fd);

	pid = waitpid(pid, &status, 0);
	TEST_ASSERT_TRUE(pid > 0);
	TEST_ASSERT_TRUE(WIFEXITED(status));
	TEST_ASSERT_EQUAL_INT(0, WEXITSTATUS(status));

	unlink(tmpFile);
}


TEST(proc_fexecve, fexecve_cloexec_fd_closed)
{
	pid_t pid;
	int status;
	int fd;
	int pipeFds[2];
	int ret;

	/* Open the executable with O_CLOEXEC.
	 * The fd itself will be closed after exec but we need to exec before that
	 * matters. fexecve itself must work even with O_CLOEXEC since it uses
	 * the fd before exec replaces the image. */
	fd = open(TRUE_PATH, O_RDONLY | O_CLOEXEC);
	TEST_ASSERT_TRUE(fd >= 0);

	ret = pipe(pipeFds);
	TEST_ASSERT_EQUAL_INT(0, ret);

	pid = fork();
	TEST_ASSERT_TRUE(pid >= 0);

	if (pid == 0) {
		char *argv[] = { "true", NULL };
		char *envp[] = { NULL };

		close(pipeFds[0]);
		close(pipeFds[1]);

		fexecve(fd, argv, envp);
		_exit(99);
	}

	close(fd);
	close(pipeFds[1]);
	close(pipeFds[0]);

	pid = waitpid(pid, &status, 0);
	TEST_ASSERT_TRUE(pid > 0);
	TEST_ASSERT_TRUE(WIFEXITED(status));
	TEST_ASSERT_EQUAL_INT(0, WEXITSTATUS(status));
}


TEST_GROUP_RUNNER(proc_fexecve)
{
	RUN_TEST_CASE(proc_fexecve, fexecve_executes_program);
	RUN_TEST_CASE(proc_fexecve, fexecve_exit_code_nonzero);
	RUN_TEST_CASE(proc_fexecve, fexecve_passes_environment);
	RUN_TEST_CASE(proc_fexecve, fexecve_ebadf);
	RUN_TEST_CASE(proc_fexecve, fexecve_eacces_not_executable);
	RUN_TEST_CASE(proc_fexecve, fexecve_cloexec_fd_closed);
}
