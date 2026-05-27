/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - signal.h
 * TESTED:
 *    - pthread_kill()
 *    - pthread_sigmask()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <signal.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include "unity_fixture.h"

#define INVALID_SIGNAL_NUM 9999


static struct {
	volatile sig_atomic_t handlerCalled;
	volatile pthread_t handlerThread;
	sigset_t origMask;
} test_common;


static void test_signalHandler(int sig)
{
	(void)sig;
	test_common.handlerCalled = 1;
	test_common.handlerThread = pthread_self();
}


/* ============================================================
 * TEST_GROUP: pthread_kill
 * ============================================================ */

TEST_GROUP(pthread_kill);


TEST_SETUP(pthread_kill)
{
	struct sigaction sa;

	test_common.handlerCalled = 0;
	test_common.handlerThread = 0;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = test_signalHandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	TEST_ASSERT_EQUAL_INT(0, sigaction(SIGUSR1, &sa, NULL));
	TEST_ASSERT_EQUAL_INT(0, sigaction(SIGUSR2, &sa, NULL));
}


TEST_TEAR_DOWN(pthread_kill)
{
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SIG_DFL;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	sigaction(SIGUSR1, &sa, NULL);
	sigaction(SIGUSR2, &sa, NULL);
}


TEST(pthread_kill, signal_delivery_to_self)
{
	int ret;

	test_common.handlerCalled = 0;

	ret = pthread_kill(pthread_self(), SIGUSR1);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(1, test_common.handlerCalled);
}


TEST(pthread_kill, handler_context_is_target_thread)
{
	int ret;
	pthread_t self;

	test_common.handlerCalled = 0;
	test_common.handlerThread = 0;
	self = pthread_self();

	ret = pthread_kill(self, SIGUSR1);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(1, test_common.handlerCalled);
	TEST_ASSERT_TRUE(pthread_equal(self, test_common.handlerThread));
}


static void *test_otherThreadFunc(void *arg)
{
	sigset_t set;

	(void)arg;

	/* Unblock SIGUSR1 in this thread so it can be delivered here */
	sigemptyset(&set);
	sigaddset(&set, SIGUSR1);
	pthread_sigmask(SIG_UNBLOCK, &set, NULL);

	/* Wait for signal */
	while (test_common.handlerCalled == 0) {
		usleep(1000);
	}

	return NULL;
}


TEST(pthread_kill, signal_delivery_to_other_thread)
{
	int ret;
	pthread_t thread;
	sigset_t blockSet;

	test_common.handlerCalled = 0;
	test_common.handlerThread = 0;

	/* Block SIGUSR1 in main thread to ensure it's delivered to the target thread */
	sigemptyset(&blockSet);
	sigaddset(&blockSet, SIGUSR1);
	TEST_ASSERT_EQUAL_INT(0, pthread_sigmask(SIG_BLOCK, &blockSet, NULL));

	ret = pthread_create(&thread, NULL, test_otherThreadFunc, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Give the other thread time to set up */
	usleep(10000);

	ret = pthread_kill(thread, SIGUSR1);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_join(thread, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	TEST_ASSERT_EQUAL_INT(1, test_common.handlerCalled);
	TEST_ASSERT_TRUE(pthread_equal(thread, test_common.handlerThread));

	/* Unblock SIGUSR1 in main thread */
	TEST_ASSERT_EQUAL_INT(0, pthread_sigmask(SIG_UNBLOCK, &blockSet, NULL));
}


TEST(pthread_kill, sig_zero_no_signal_sent)
{
	int ret;

	test_common.handlerCalled = 0;

	ret = pthread_kill(pthread_self(), 0);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, test_common.handlerCalled);
}


TEST(pthread_kill, einval_invalid_signal_number)
{
	int ret;

	ret = pthread_kill(pthread_self(), INVALID_SIGNAL_NUM);
	TEST_ASSERT_EQUAL_INT(EINVAL, ret);
}


TEST(pthread_kill, einval_negative_signal_number)
{
	int ret;

	ret = pthread_kill(pthread_self(), -1);
	TEST_ASSERT_EQUAL_INT(EINVAL, ret);
}


TEST(pthread_kill, multiple_signals)
{
	int ret;

	test_common.handlerCalled = 0;

	ret = pthread_kill(pthread_self(), SIGUSR1);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(1, test_common.handlerCalled);

	test_common.handlerCalled = 0;

	ret = pthread_kill(pthread_self(), SIGUSR2);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(1, test_common.handlerCalled);
}


TEST_GROUP_RUNNER(pthread_kill)
{
	RUN_TEST_CASE(pthread_kill, signal_delivery_to_self);
	RUN_TEST_CASE(pthread_kill, handler_context_is_target_thread);
	RUN_TEST_CASE(pthread_kill, signal_delivery_to_other_thread);
	RUN_TEST_CASE(pthread_kill, sig_zero_no_signal_sent);
	RUN_TEST_CASE(pthread_kill, einval_invalid_signal_number);
	RUN_TEST_CASE(pthread_kill, einval_negative_signal_number);
	RUN_TEST_CASE(pthread_kill, multiple_signals);
}


/* ============================================================
 * TEST_GROUP: pthread_sigmask
 * ============================================================ */

TEST_GROUP(pthread_sigmask);


TEST_SETUP(pthread_sigmask)
{
	struct sigaction sa;

	test_common.handlerCalled = 0;
	test_common.handlerThread = 0;

	/* Save current mask */
	TEST_ASSERT_EQUAL_INT(0, pthread_sigmask(SIG_SETMASK, NULL, &test_common.origMask));

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = test_signalHandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	TEST_ASSERT_EQUAL_INT(0, sigaction(SIGUSR1, &sa, NULL));
	TEST_ASSERT_EQUAL_INT(0, sigaction(SIGUSR2, &sa, NULL));
}


TEST_TEAR_DOWN(pthread_sigmask)
{
	struct sigaction sa;

	/* Restore original mask */
	pthread_sigmask(SIG_SETMASK, &test_common.origMask, NULL);

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SIG_DFL;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	sigaction(SIGUSR1, &sa, NULL);
	sigaction(SIGUSR2, &sa, NULL);
}


TEST(pthread_sigmask, block_adds_to_mask)
{
	int ret;
	sigset_t set, current;

	sigemptyset(&set);
	sigaddset(&set, SIGUSR1);

	ret = pthread_sigmask(SIG_BLOCK, &set, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_sigmask(SIG_SETMASK, NULL, &current);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(1, sigismember(&current, SIGUSR1));
}


TEST(pthread_sigmask, block_union_semantics)
{
	int ret;
	sigset_t set1, set2, current;

	/* Block SIGUSR1 */
	sigemptyset(&set1);
	sigaddset(&set1, SIGUSR1);
	ret = pthread_sigmask(SIG_BLOCK, &set1, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Block SIGUSR2 — SIGUSR1 should remain blocked */
	sigemptyset(&set2);
	sigaddset(&set2, SIGUSR2);
	ret = pthread_sigmask(SIG_BLOCK, &set2, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_sigmask(SIG_SETMASK, NULL, &current);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(1, sigismember(&current, SIGUSR1));
	TEST_ASSERT_EQUAL_INT(1, sigismember(&current, SIGUSR2));
}


TEST(pthread_sigmask, unblock_removes_from_mask)
{
	int ret;
	sigset_t set, current;

	/* First block SIGUSR1 */
	sigemptyset(&set);
	sigaddset(&set, SIGUSR1);
	ret = pthread_sigmask(SIG_BLOCK, &set, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Now unblock it */
	ret = pthread_sigmask(SIG_UNBLOCK, &set, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_sigmask(SIG_SETMASK, NULL, &current);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, sigismember(&current, SIGUSR1));
}


TEST(pthread_sigmask, setmask_replaces_mask)
{
	int ret;
	sigset_t set, current;

	/* Block SIGUSR1 first */
	sigemptyset(&set);
	sigaddset(&set, SIGUSR1);
	ret = pthread_sigmask(SIG_BLOCK, &set, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Now set mask to only SIGUSR2 */
	sigemptyset(&set);
	sigaddset(&set, SIGUSR2);
	ret = pthread_sigmask(SIG_SETMASK, &set, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_sigmask(SIG_SETMASK, NULL, &current);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, sigismember(&current, SIGUSR1));
	TEST_ASSERT_EQUAL_INT(1, sigismember(&current, SIGUSR2));
}


TEST(pthread_sigmask, oset_stores_previous_mask)
{
	int ret;
	sigset_t set, oldSet;

	/* Block SIGUSR1 */
	sigemptyset(&set);
	sigaddset(&set, SIGUSR1);
	ret = pthread_sigmask(SIG_BLOCK, &set, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Block SIGUSR2, capture old mask */
	sigemptyset(&set);
	sigaddset(&set, SIGUSR2);
	ret = pthread_sigmask(SIG_BLOCK, &set, &oldSet);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Old mask should have SIGUSR1 but not SIGUSR2 */
	TEST_ASSERT_EQUAL_INT(1, sigismember(&oldSet, SIGUSR1));
	TEST_ASSERT_EQUAL_INT(0, sigismember(&oldSet, SIGUSR2));
}


TEST(pthread_sigmask, set_null_queries_without_change)
{
	int ret;
	sigset_t set, current1, current2;

	/* Block SIGUSR1 to have a known state */
	sigemptyset(&set);
	sigaddset(&set, SIGUSR1);
	ret = pthread_sigmask(SIG_BLOCK, &set, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Query with set=NULL */
	ret = pthread_sigmask(SIG_BLOCK, NULL, &current1);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Query again — mask should be unchanged */
	ret = pthread_sigmask(SIG_SETMASK, NULL, &current2);
	TEST_ASSERT_EQUAL_INT(0, ret);

	TEST_ASSERT_EQUAL_INT(sigismember(&current1, SIGUSR1), sigismember(&current2, SIGUSR1));
	TEST_ASSERT_EQUAL_INT(sigismember(&current1, SIGUSR2), sigismember(&current2, SIGUSR2));
}


TEST(pthread_sigmask, einval_invalid_how)
{
	int ret;
	sigset_t set;

	sigemptyset(&set);
	sigaddset(&set, SIGUSR1);

	ret = pthread_sigmask(99, &set, NULL);
	TEST_ASSERT_EQUAL_INT(EINVAL, ret);
}


TEST(pthread_sigmask, pending_signal_delivered_on_unblock)
{
	int ret;
	sigset_t set;

	test_common.handlerCalled = 0;

	/* Block SIGUSR1 */
	sigemptyset(&set);
	sigaddset(&set, SIGUSR1);
	ret = pthread_sigmask(SIG_BLOCK, &set, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Send signal while blocked — should be pending */
	ret = pthread_kill(pthread_self(), SIGUSR1);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, test_common.handlerCalled);

	/* Unblock — signal must be delivered before pthread_sigmask returns */
	ret = pthread_sigmask(SIG_UNBLOCK, &set, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(1, test_common.handlerCalled);
}


TEST(pthread_sigmask, cannot_block_sigkill)
{
	int ret;
	sigset_t set, current;

	sigemptyset(&set);
	sigaddset(&set, SIGKILL);

	/* Attempt to block SIGKILL — no error, but shall not actually block it */
	ret = pthread_sigmask(SIG_BLOCK, &set, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_sigmask(SIG_SETMASK, NULL, &current);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, sigismember(&current, SIGKILL));
}


TEST(pthread_sigmask, cannot_block_sigstop)
{
	int ret;
	sigset_t set, current;

	sigemptyset(&set);
	sigaddset(&set, SIGSTOP);

	/* Attempt to block SIGSTOP — no error, but shall not actually block it */
	ret = pthread_sigmask(SIG_BLOCK, &set, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_sigmask(SIG_SETMASK, NULL, &current);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, sigismember(&current, SIGSTOP));
}


TEST_GROUP_RUNNER(pthread_sigmask)
{
	RUN_TEST_CASE(pthread_sigmask, block_adds_to_mask);
	RUN_TEST_CASE(pthread_sigmask, block_union_semantics);
	RUN_TEST_CASE(pthread_sigmask, unblock_removes_from_mask);
	RUN_TEST_CASE(pthread_sigmask, setmask_replaces_mask);
	RUN_TEST_CASE(pthread_sigmask, oset_stores_previous_mask);
	RUN_TEST_CASE(pthread_sigmask, set_null_queries_without_change);
	RUN_TEST_CASE(pthread_sigmask, einval_invalid_how);
	RUN_TEST_CASE(pthread_sigmask, pending_signal_delivered_on_unblock);
	RUN_TEST_CASE(pthread_sigmask, cannot_block_sigkill);
	RUN_TEST_CASE(pthread_sigmask, cannot_block_sigstop);
}
