/*
 * Phoenix-RTOS
 *
 * POSIX.1-2001 (SUSv3, XSI) standard library functions tests
 * HEADER:
 *    - <unistd.h>
 * TESTED:
 *    - usleep()
 *
 * NOTE: usleep() was removed in POSIX.1-2008, the requirements come from IEEE Std 1003.1-2001.
 *
 * Copyright 2026 Phoenix Systems
 * Author: Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* glibc declares usleep() only with _DEFAULT_SOURCE or X/Open feature test macros */
#define _DEFAULT_SOURCE

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "unity_fixture.h"

#define TEST_US_PER_S     1000000LL
#define TEST_NS_PER_US    1000L
#define TEST_SLEEP_US     100000U
#define TEST_SLEEP_MAX_US 999999U
#define TEST_SIGNAL_US    100000U
#define TEST_NO_EFFECT_US 50000LL


static struct {
	pthread_t thread;
	pthread_t sleeper;
	int threadRet;
	struct sigaction oldAct;
	volatile sig_atomic_t sigCount;
} test_common;


static int64_t test_nowUs(void)
{
	struct timespec ts;

	TEST_ASSERT_EQUAL_INT(0, clock_gettime(CLOCK_MONOTONIC, &ts));

	return (int64_t)ts.tv_sec * TEST_US_PER_S + ts.tv_nsec / TEST_NS_PER_US;
}


/* calls usleep(us), asserts the return value and returns the measured suspension time in microseconds */
static int64_t test_usleep(useconds_t us)
{
	int64_t start = test_nowUs();

	TEST_ASSERT_EQUAL_INT(0, usleep(us));

	return test_nowUs() - start;
}


static void test_sigHandler(int sig)
{
	(void)sig;
	test_common.sigCount++;
}


static void *test_signalThread(void *arg)
{
	(void)arg;

	/* the sleeper is already suspended by the time this signal is sent */
	test_common.threadRet = usleep(TEST_SIGNAL_US);
	if (test_common.threadRet == 0) {
		test_common.threadRet = pthread_kill(test_common.sleeper, SIGUSR1);
	}

	return NULL;
}


TEST_GROUP(unistd_usleep);

TEST_SETUP(unistd_usleep)
{
	test_common.threadRet = -1;
	test_common.sigCount = 0;
	TEST_ASSERT_EQUAL_INT(0, sigaction(SIGUSR1, NULL, &test_common.oldAct));
}


TEST_TEAR_DOWN(unistd_usleep)
{
	sigaction(SIGUSR1, &test_common.oldAct, NULL);
}


TEST(unistd_usleep, suspends_for_requested_time)
{
	/* the suspension may be longer due to scheduling, but never shorter */
	TEST_ASSERT_GREATER_OR_EQUAL_INT64(TEST_SLEEP_US, test_usleep(TEST_SLEEP_US));
}


TEST(unistd_usleep, suspends_for_minimal_time)
{
	TEST_ASSERT_GREATER_OR_EQUAL_INT64(1, test_usleep(1U));
}


TEST(unistd_usleep, suspends_for_maximal_time)
{
	/* the interval shall be less than one million microseconds */
	TEST_ASSERT_GREATER_OR_EQUAL_INT64(TEST_SLEEP_MAX_US, test_usleep(TEST_SLEEP_MAX_US));
}


TEST(unistd_usleep, zero_has_no_effect)
{
	/* the call has no effect - it returns successfully without suspending the thread */
	TEST_ASSERT_LESS_THAN_INT64(TEST_NO_EFFECT_US, test_usleep(0U));
}


TEST(unistd_usleep, caught_signal_ends_suspension)
{
	struct sigaction act;
	int64_t start, elapsed;
	int ret;

	memset(&act, 0, sizeof(act));
	act.sa_handler = test_sigHandler;
	TEST_ASSERT_EQUAL_INT(0, sigemptyset(&act.sa_mask));
	act.sa_flags = 0;
	TEST_ASSERT_EQUAL_INT(0, sigaction(SIGUSR1, &act, NULL));

	test_common.sleeper = pthread_self();
	TEST_ASSERT_EQUAL_INT(0, pthread_create(&test_common.thread, NULL, test_signalThread, NULL));

	start = test_nowUs();
	ret = usleep(TEST_SLEEP_MAX_US);
	elapsed = test_nowUs() - start;

	TEST_ASSERT_EQUAL_INT(0, pthread_join(test_common.thread, NULL));
	TEST_ASSERT_EQUAL_INT(0, test_common.threadRet);
	TEST_ASSERT_EQUAL_INT(1, test_common.sigCount);
	TEST_ASSERT_LESS_THAN_INT64(TEST_SLEEP_MAX_US, elapsed);
	/* the only return values defined are 0 and -1 */
	if (ret != 0) {
		TEST_ASSERT_EQUAL_INT(-1, ret);
	}
}


TEST(unistd_usleep, terminating_signal_ends_suspension)
{
	int64_t start, elapsed;
	pid_t pid;
	int status;

	pid = fork();
	if ((pid < 0) && (errno == ENOSYS)) {
		TEST_IGNORE_MESSAGE("fork() not supported");
	}
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, pid);
	if (pid == 0) {
		usleep(TEST_SLEEP_MAX_US);
		_exit(0);
	}

	start = test_nowUs();
	TEST_ASSERT_EQUAL_INT(0, usleep(TEST_SIGNAL_US));
	TEST_ASSERT_EQUAL_INT(0, kill(pid, SIGTERM));
	TEST_ASSERT_EQUAL_INT(pid, waitpid(pid, &status, 0));
	elapsed = test_nowUs() - start;

	TEST_ASSERT_TRUE(WIFSIGNALED(status));
	TEST_ASSERT_EQUAL_INT(SIGTERM, WTERMSIG(status));
	TEST_ASSERT_LESS_THAN_INT64(TEST_SLEEP_MAX_US, elapsed);
}


TEST_GROUP_RUNNER(unistd_usleep)
{
	RUN_TEST_CASE(unistd_usleep, suspends_for_requested_time);
	RUN_TEST_CASE(unistd_usleep, suspends_for_minimal_time);
	RUN_TEST_CASE(unistd_usleep, suspends_for_maximal_time);
	RUN_TEST_CASE(unistd_usleep, zero_has_no_effect);
	RUN_TEST_CASE(unistd_usleep, caught_signal_ends_suspension);
	RUN_TEST_CASE(unistd_usleep, terminating_signal_ends_suspension);
}
