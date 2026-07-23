/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - pthread.h
 * TESTED:
 *    - pthread_atfork()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <pthread.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "unity_fixture.h"


TEST_GROUP(pthread_atfork);


TEST_SETUP(pthread_atfork)
{
}


TEST_TEAR_DOWN(pthread_atfork)
{
}


/* pthread_atfork: shall return 0 on success */
static void test_atforkPrepare(void)
{
}

static void test_atforkParent(void)
{
}

static void test_atforkChild(void)
{
}


TEST(pthread_atfork, atfork_returns_zero)
{
	int ret;

	ret = pthread_atfork(test_atforkPrepare, test_atforkParent, test_atforkChild);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


/* pthread_atfork: NULL handlers are acceptable */
TEST(pthread_atfork, atfork_null_handlers)
{
	int ret;

	ret = pthread_atfork(NULL, NULL, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_atfork(test_atforkPrepare, NULL, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_atfork(NULL, test_atforkParent, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_atfork(NULL, NULL, test_atforkChild);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


/* pthread_atfork: prepare handler called before fork, parent handler after fork in parent */
static int test_atforkPrepareFlag;
static int test_atforkParentFlag;
static int test_atforkChildFlag;

static void test_atforkPrepare2(void)
{
	test_atforkPrepareFlag = 1;
}

static void test_atforkParent2(void)
{
	test_atforkParentFlag = 1;
}

static void test_atforkChild2(void)
{
	test_atforkChildFlag = 1;
}


TEST(pthread_atfork, atfork_handlers_called_on_fork)
{
	pid_t pid;
	int status;
	int ret;

	test_atforkPrepareFlag = 0;
	test_atforkParentFlag = 0;
	test_atforkChildFlag = 0;

	ret = pthread_atfork(test_atforkPrepare2, test_atforkParent2, test_atforkChild2);
	TEST_ASSERT_EQUAL_INT(0, ret);

	pid = fork();
	TEST_ASSERT_TRUE(pid >= 0);

	if (pid == 0) {
		/* Child process: child handler should have been called */
		if (test_atforkChildFlag != 1) {
			_exit(1);
		}
		_exit(0);
	}

	/* Parent: prepare and parent handlers should have been called */
	TEST_ASSERT_EQUAL_INT(1, test_atforkPrepareFlag);
	TEST_ASSERT_EQUAL_INT(1, test_atforkParentFlag);

	ret = waitpid(pid, &status, 0);
	TEST_ASSERT_TRUE(ret > 0);
	TEST_ASSERT_TRUE(WIFEXITED(status));
	TEST_ASSERT_EQUAL_INT(0, WEXITSTATUS(status));
}


/* pthread_atfork: prepare handlers called in reverse order of registration */
#define ATFORK_ORDER_MAX 16
static int test_atforkOrder[ATFORK_ORDER_MAX];
static int test_atforkOrderIdx;

static void test_atforkPrepareA(void)
{
	if (test_atforkOrderIdx < ATFORK_ORDER_MAX) {
		test_atforkOrder[test_atforkOrderIdx++] = 1;
	}
}

static void test_atforkPrepareB(void)
{
	if (test_atforkOrderIdx < ATFORK_ORDER_MAX) {
		test_atforkOrder[test_atforkOrderIdx++] = 2;
	}
}

static void test_atforkParentA(void)
{
	if (test_atforkOrderIdx < ATFORK_ORDER_MAX) {
		test_atforkOrder[test_atforkOrderIdx++] = 3;
	}
}

static void test_atforkParentB(void)
{
	if (test_atforkOrderIdx < ATFORK_ORDER_MAX) {
		test_atforkOrder[test_atforkOrderIdx++] = 4;
	}
}


TEST(pthread_atfork, atfork_prepare_reverse_order)
{
	pid_t pid;
	int status;
	int ret;
	int startIdx;

	test_atforkOrderIdx = 0;

	ret = pthread_atfork(test_atforkPrepareA, test_atforkParentA, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_atfork(test_atforkPrepareB, test_atforkParentB, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Record start — previously registered handlers may also fire */
	startIdx = test_atforkOrderIdx;

	pid = fork();
	TEST_ASSERT_TRUE(pid >= 0);

	if (pid == 0) {
		_exit(0);
	}

	ret = waitpid(pid, &status, 0);
	TEST_ASSERT_TRUE(ret > 0);
	TEST_ASSERT_TRUE(WIFEXITED(status));

	/*
	 * Among the entries recorded after startIdx, find our markers.
	 * Prepare: reverse order (B=2 before A=1), Parent: registration order (A=3 then B=4).
	 * Other handlers from prior tests may interleave, so find relative positions.
	 */
	{
		int posPreA = -1, posPreB = -1, posParA = -1, posParB = -1;
		int i;

		for (i = startIdx; i < test_atforkOrderIdx; i++) {
			if (test_atforkOrder[i] == 1 && posPreA == -1) {
				posPreA = i;
			}
			else if (test_atforkOrder[i] == 2 && posPreB == -1) {
				posPreB = i;
			}
			else if (test_atforkOrder[i] == 3 && posParA == -1) {
				posParA = i;
			}
			else if (test_atforkOrder[i] == 4 && posParB == -1) {
				posParB = i;
			}
		}

		TEST_ASSERT_TRUE(posPreA >= 0);
		TEST_ASSERT_TRUE(posPreB >= 0);
		TEST_ASSERT_TRUE(posParA >= 0);
		TEST_ASSERT_TRUE(posParB >= 0);

		/* Prepare: B before A (reverse order) */
		TEST_ASSERT_TRUE(posPreB < posPreA);
		/* Parent: A before B (registration order) */
		TEST_ASSERT_TRUE(posParA < posParB);
	}
}


/* pthread_atfork: child handlers called in registration order */
#define ATFORK_CHILD_ORDER_MAX 16
static int test_atforkChildOrder[ATFORK_CHILD_ORDER_MAX];
static int test_atforkChildOrderIdx;

static void test_atforkChildA(void)
{
	if (test_atforkChildOrderIdx < ATFORK_CHILD_ORDER_MAX) {
		test_atforkChildOrder[test_atforkChildOrderIdx++] = 1;
	}
}

static void test_atforkChildB(void)
{
	if (test_atforkChildOrderIdx < ATFORK_CHILD_ORDER_MAX) {
		test_atforkChildOrder[test_atforkChildOrderIdx++] = 2;
	}
}


TEST(pthread_atfork, atfork_child_registration_order)
{
	pid_t pid;
	int status;
	int ret;

	test_atforkChildOrderIdx = 0;

	ret = pthread_atfork(NULL, NULL, test_atforkChildA);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_atfork(NULL, NULL, test_atforkChildB);
	TEST_ASSERT_EQUAL_INT(0, ret);

	pid = fork();
	TEST_ASSERT_TRUE(pid >= 0);

	if (pid == 0) {
		/* Child: find relative positions of our handlers */
		int posA = -1, posB = -1;
		int i;

		for (i = 0; i < test_atforkChildOrderIdx; i++) {
			if (test_atforkChildOrder[i] == 1 && posA == -1) {
				posA = i;
			}
			else if (test_atforkChildOrder[i] == 2 && posB == -1) {
				posB = i;
			}
		}

		/* A registered first, should be called before B */
		if (posA < 0 || posB < 0 || posA >= posB) {
			_exit(1);
		}
		_exit(0);
	}

	ret = waitpid(pid, &status, 0);
	TEST_ASSERT_TRUE(ret > 0);
	TEST_ASSERT_TRUE(WIFEXITED(status));
	TEST_ASSERT_EQUAL_INT(0, WEXITSTATUS(status));
}


TEST_GROUP_RUNNER(pthread_atfork)
{
	RUN_TEST_CASE(pthread_atfork, atfork_returns_zero);
	RUN_TEST_CASE(pthread_atfork, atfork_null_handlers);
	RUN_TEST_CASE(pthread_atfork, atfork_handlers_called_on_fork);
	RUN_TEST_CASE(pthread_atfork, atfork_prepare_reverse_order);
	RUN_TEST_CASE(pthread_atfork, atfork_child_registration_order);
}
