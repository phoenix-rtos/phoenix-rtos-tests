/*
 * Phoenix-RTOS
 *
 * libc-tests
 *
 * executable program for various exec functions
 *
 * Copyright 2021 Phoenix Systems
 * Author: Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <libgen.h>
#include <errno.h>
#include <sys/threads.h>
#include <signal.h>

extern char **environ;
static unsigned char stack[_PAGE_SIZE] __attribute__((aligned(8)));


static void test_exec_execveEnv(bool changeEnv)
{
	char *const arg[] = { "/bin/to_exec", NULL };
	if (changeEnv) {
		char *const env[] = { "TEST1=exec_value", NULL };
		setenv("TEST1", "invalid_value", 1);
		setenv("TEST2", "should_dissapear", 1);

		if (execve("/bin/test-exec", arg, env) == -1) {
			fprintf(stderr, "execve function failed: %s\n", strerror(errno));

			exit(EXIT_FAILURE);
		}
	}
	else {
		setenv("TEST1", "unchanged_value", 1);

		if (execve("/bin/test-exec", arg, environ) == -1) {
			fprintf(stderr, "execve function failed: %s\n", strerror(errno));

			exit(EXIT_FAILURE);
		}
	}
}


static void test_exec_execvePath(void)
{
	char *const arg[] = { "to_exec", NULL };

	setenv("PATH", "/bin:/sbin:/usr/bin:/usr/sbin", 1);

	if (execve("test-exec", arg, environ) == -1) {
		fprintf(stderr, "execve function failed: %s\n", strerror(errno));

		exit(EXIT_FAILURE);
	}
}


static void test_exec_execvpeEnv(bool changeEnv)
{
	char *const arg[] = { "/bin/to_exec", NULL };
	if (changeEnv) {
		char *const env[] = { "TEST1=exec_value", NULL };
		setenv("TEST1", "invalid_value", 1);
		setenv("TEST2", "should_dissapear", 1);

		if (execvpe("/bin/test-exec", arg, env) == -1) {
			fprintf(stderr, "execvpe function failed: %s\n", strerror(errno));

			exit(EXIT_FAILURE);
		}
	}
	else {
		setenv("TEST1", "unchanged_value", 1);

		if (execvpe("/bin/test-exec", arg, environ) == -1) {
			fprintf(stderr, "execvpe function failed: %s\n", strerror(errno));

			exit(EXIT_FAILURE);
		}
	}
}


static void test_exec_execvpePath(void)
{
	char *const arg[] = { "to_exec", NULL };

	setenv("PATH", "/bin:/sbin:/usr/bin:/usr/sbin", 1);

	if (execvpe("test-exec", arg, environ) == -1) {
		fprintf(stderr, "execve function failed: %s\n", strerror(errno));

		exit(EXIT_FAILURE);
	}
}


static void test_exec_execvpEnv(void)
{
	char *const arg[] = { "/bin/to_exec", NULL };

	setenv("TEST1", "unchanged_value", 1);

	if (execvp("/bin/test-exec", arg) == -1) {
		fprintf(stderr, "execvp function failed: %s\n", strerror(errno));

		exit(EXIT_FAILURE);
	}
}


static void test_exec_execvpPath(void)
{
	char *const arg[] = { "to_exec", NULL };

	setenv("PATH", "/bin:/sbin:/usr/bin:/usr/sbin", 1);

	if (execvp("test-exec", arg) == -1) {
		fprintf(stderr, "execvp function failed: %s\n", strerror(errno));

		exit(EXIT_FAILURE);
	}
}


static void anotherThread(void *arg)
{
	for (;;) {
		sleep(1);
	}
}


static void test_exec_execvpPath_multithreaded(void)
{
	int tid;
	char tidstr[10];
	char *arg[] = { "to_exec_multithr", tidstr, NULL };

	setenv("PATH", "/bin:/sbin:/usr/bin:/usr/sbin", 1);
	if (beginthreadex(anotherThread, 4, stack, sizeof(stack), NULL, &tid) != 0) {
		fprintf(stderr, "beginthreadex failed: %s\n", strerror(errno));

		exit(EXIT_FAILURE);
	}

	snprintf(tidstr, sizeof(tidstr), "%d", tid);

	if (execvp("test-exec", arg) == -1) {
		fprintf(stderr, "execvp function failed: %s\n", strerror(errno));

		exit(EXIT_FAILURE);
	}
}


int main(int argc, char *argv[])
{
	bool changeEnv;

	if (!strcmp(basename(argv[0]), "to_exec")) {
		int i;
		printf("argc = %d\n", argc);

		for (i = 0; argv[i] != NULL; i++) {
			printf("argv[%d] = %s\n", i, argv[i]);
		}

		for (i = 0; environ[i] != NULL; i++) {
			printf("environ[%d] = %s\n", i, environ[i]);
		}
	}
	else if (!strcmp(basename(argv[0]), "to_exec_multithr")) {
		if (argc != 2) {
			fprintf(stderr, "Please specify thread ID\n");
			return 1;
		}

		if (signalPost(getpid(), atoi(argv[1]), SIGUSR1) != -EINVAL) {
			fprintf(stderr, "signalPost didn't return EINVAL, thread still running in exec'd process!\n");
			return 1;
		}
	}
	else {
		if ((argc == 1) || (atoi(argv[1]) > 10) || (atoi(argv[1]) < 0)) {
			fprintf(stderr, "Please specify test case number (1 to 10)\n");
			return 1;
		}

		clearenv();

		switch (atoi(argv[1])) {
			case 1:
				changeEnv = true;
				test_exec_execveEnv(changeEnv);
				break;

			case 2:
				changeEnv = false;
				test_exec_execveEnv(changeEnv);
				break;

			case 3:
				changeEnv = true;
				test_exec_execvePath();
				break;

			case 4:
				changeEnv = true;
				test_exec_execvpeEnv(changeEnv);
				break;

			case 5:
				changeEnv = false;
				test_exec_execvpeEnv(changeEnv);
				break;

			case 6:
				test_exec_execvpePath();
				break;

			case 7:
				test_exec_execvpEnv();
				break;

			case 8:
				test_exec_execvpPath();
				break;

			case 9:
				test_exec_execvpPath_multithreaded();
				break;

			default:
				break;
		}
	}

	return 0;
}
