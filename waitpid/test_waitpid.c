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

static char *cmd_name;

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
	int pid[2];
	int res, i;
	char *const arg[2][2] = { { "exec_infinite_process", NULL }, { "exec_sum_process", NULL } };

	for (i = 0; i < 2; i++) {
		pid[i] = vfork();
		TEST_ASSERT_GREATER_OR_EQUAL(0, pid[i]);
		/* child process i */
		if (pid[i] == 0) {
			execv(cmd_name, arg[i]);
			_exit(EXIT_FAILURE);
		}
	}
	/* wait for process 1 to become a zombie process */
	sleep(2);
	res = waitpid(pid[0], NULL, WNOHANG);
	TEST_ASSERT_EQUAL_INT(0, res);
	res = waitpid(pid[1], NULL, WNOHANG);
	TEST_ASSERT_EQUAL_INT(pid[1], res);

	res = kill(pid[0], SIGKILL);
	TEST_ASSERT_EQUAL_INT(0, res);
}


TEST(test_waitpid, waitpid_other_zombie)
{
	int pid[2];
	int res, i, status;
	char *const arg[2][2] = { { "exec_sum_process", NULL }, { "exec_sleep", NULL } };

	for (i = 0; i < 2; i++) {
		pid[i] = vfork();
		TEST_ASSERT_GREATER_OR_EQUAL(0, pid[i]);
		/* child process i */
		if (pid[i] == 0) {
			execv(cmd_name, arg[i]);
			_exit(EXIT_FAILURE);
		}
	}
	res = waitpid(pid[1], &status, 0);
	TEST_ASSERT_EQUAL_INT(pid[1], res);
	TEST_ASSERT_TRUE(WIFEXITED(status));
	TEST_ASSERT_EQUAL_INT(EXIT_SUCCESS, WEXITSTATUS(status));

	res = waitpid(pid[0], &status, 0);
	TEST_ASSERT_EQUAL_INT(pid[0], res);
	TEST_ASSERT_TRUE(WIFEXITED(status));
	TEST_ASSERT_EQUAL_INT(EXIT_SUCCESS, WEXITSTATUS(status));
}


TEST(test_waitpid, waitpid_other_zombie_before)
{
	int pid[2];
	int res, i, status;
	char *const arg[2][2] = { { "exec_sleep", NULL }, { "exec_sum_process", NULL } };

	for (i = 0; i < 2; i++) {
		pid[i] = vfork();
		TEST_ASSERT_GREATER_OR_EQUAL(0, pid[i]);
		/* child process i */
		if (pid[i] == 0) {
			execv(cmd_name, arg[i]);
			_exit(EXIT_FAILURE);
		}
	}
	/* wait for process 1 to become a zombie process */
	sleep(2);
	res = waitpid(pid[0], &status, 0);
	TEST_ASSERT_EQUAL_INT(pid[0], res);
	TEST_ASSERT_TRUE(WIFEXITED(status));
	TEST_ASSERT_EQUAL_INT(EXIT_SUCCESS, WEXITSTATUS(status));

	res = waitpid(pid[1], &status, 0);
	TEST_ASSERT_EQUAL_INT(pid[1], res);
	TEST_ASSERT_TRUE(WIFEXITED(status));
	TEST_ASSERT_EQUAL_INT(EXIT_SUCCESS, WEXITSTATUS(status));
}


TEST_GROUP_RUNNER(test_waitpid)
{
	RUN_TEST_CASE(test_waitpid, waitpid_wnohang);
	RUN_TEST_CASE(test_waitpid, waitpid_other_zombie);
	RUN_TEST_CASE(test_waitpid, waitpid_other_zombie_before);
}


void runner(void)
{
	RUN_TEST_GROUP(test_waitpid);
}


int main(int argc, char *argv[])
{
	cmd_name = argv[0];
	int failures = 0;
	int sum;
	if (!strcmp(basename(argv[0]), "exec_infinite_process")) {
		for (;;) {
			usleep(10000);
		}
	}
	else if (!strcmp(basename(argv[0]), "exec_sum_process")) {
		sum = 1 + 2;
		if (sum == 3) {
			usleep(100000);
		}
	}
	else if (!strcmp(basename(argv[0]), "exec_sleep")) {
		sleep(1);
	}
	else {
		failures = UnityMain(argc, (const char **)argv, runner);
	}

	return (failures == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
