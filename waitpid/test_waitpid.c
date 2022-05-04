/*
 * Phoenix-RTOS
 *
 * phoenix-rtos-tests
 *
 * simple waitpid test
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
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <libgen.h>
#include <signal.h>
#include <sys/wait.h>

#include "unity_fixture.h"

TEST_GROUP(test_waitpid);

TEST_SETUP(test_waitpid)
{
}

TEST_TEAR_DOWN(test_waitpid)
{
}

/* Need to add more test cases */
TEST(test_waitpid, waitpid_wnohang)
{
	int pid[2] = { 0, 0 };
	int res, i, j;
	char *const arg[][2] = { { "exec_while_process", NULL }, { "exec_sum_process", NULL } };

	/* 1 iteration for easier problem reproduction, in the future there will be more irerations */
	for (i = 1; i > 0; i--) {
		for (j = 0; j < 2; j++) {
			if ((pid[j] = vfork()) < 0) {
				fprintf(stderr, "vfork function failed: %s\n", strerror(errno));
				return;
			}
			/* child process j */
			else if (!pid[j]) {
				execv("/bin/test-waitpid", arg[j]);
				fprintf(stderr, "exec function failed: %s\n", strerror(errno));
				_exit(EXIT_FAILURE);
			}
		}
		/* wait for process 1 to become a zombie process */
		usleep(200000);
		res = waitpid(pid[0], NULL, WNOHANG);
		/* instead of returning 0 at once waitpid function waits for a process state change */
		TEST_ASSERT_EQUAL_INT(0, res);
		res = waitpid(pid[1], NULL, WNOHANG);
		TEST_ASSERT_GREATER_OR_EQUAL_INT(1, res);

		kill(pid[1], SIGKILL);
		kill(pid[0], SIGKILL);
	}
}

TEST_GROUP_RUNNER(test_waitpid)
{
	RUN_TEST_CASE(test_waitpid, waitpid_wnohang);
}

void runner(void)
{
	RUN_TEST_GROUP(test_waitpid);
}

int main(int argc, char *argv[])
{
	int sum;
	if (!strcmp(basename(argv[0]), "exec_while_process")) {
		while (1)
			;
	}
	else if (!strcmp(basename(argv[0]), "exec_sum_process")) {
		if ((sum = 1 + 2) == 3)
			usleep(100000);
	}
	else {

		UnityMain(argc, (const char **)argv, runner);

		return 0;
	}
}
