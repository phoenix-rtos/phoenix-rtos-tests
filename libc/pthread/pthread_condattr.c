/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <pthread.h>
 * TESTED:
 *    - pthread_condattr_init()
 *    - pthread_condattr_destroy()
 *    - pthread_cond_destroy()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#include "unity_fixture.h"

#define TEST_TIMEOUT_MS 100L
#define TEST_NS_PER_MS  1000000L
#define TEST_NS_PER_S   1000000000L
#define TEST_US_PER_MS  1000LL
#define TEST_US_PER_S   1000000LL
#define TEST_NS_PER_US  1000L


static struct {
	pthread_mutex_t mutex;
	pthread_cond_t *cond;
	pthread_t thread;
	bool threadCreated;
	bool mutexLocked;
	bool ready;
	bool go;
	int waitRet;
} test_common;


static int64_t test_nowUs(clockid_t clk)
{
	struct timespec ts;

	TEST_ASSERT_EQUAL_INT(0, clock_gettime(clk, &ts));

	return (int64_t)ts.tv_sec * TEST_US_PER_S + ts.tv_nsec / TEST_NS_PER_US;
}


/* waits on cond with an absolute timeout measured against clk, asserts that the wait timed out no earlier than requested */
static void test_assertTimedWaitTimesOut(pthread_cond_t *cond, clockid_t clk)
{
	struct timespec abstime;
	int64_t start;
	int ret;

	TEST_ASSERT_EQUAL_INT(0, clock_gettime(clk, &abstime));
	abstime.tv_nsec += TEST_TIMEOUT_MS * TEST_NS_PER_MS;
	if (abstime.tv_nsec >= TEST_NS_PER_S) {
		abstime.tv_sec += 1;
		abstime.tv_nsec -= TEST_NS_PER_S;
	}

	start = test_nowUs(clk);
	TEST_ASSERT_EQUAL_INT(0, pthread_mutex_lock(&test_common.mutex));
	do {
		ret = pthread_cond_timedwait(cond, &test_common.mutex, &abstime);
	} while (ret == 0);
	TEST_ASSERT_EQUAL_INT(0, pthread_mutex_unlock(&test_common.mutex));

	TEST_ASSERT_EQUAL_INT(ETIMEDOUT, ret);
	/* the timeout is measured against the clock the condition variable was initialized with */
	TEST_ASSERT_GREATER_OR_EQUAL_INT64(TEST_TIMEOUT_MS * TEST_US_PER_MS, test_nowUs(clk) - start);
}


static void *test_waiterThread(void *arg)
{
	(void)arg;

	pthread_mutex_lock(&test_common.mutex);
	test_common.ready = true;
	test_common.waitRet = 0;
	while ((test_common.go == false) && (test_common.waitRet == 0)) {
		test_common.waitRet = pthread_cond_wait(test_common.cond, &test_common.mutex);
	}
	pthread_mutex_unlock(&test_common.mutex);

	return NULL;
}


/* starts test_waiterThread and returns with test_common.mutex locked once the thread blocks on cond */
static void test_startWaiterAndLock(pthread_cond_t *cond)
{
	test_common.cond = cond;
	test_common.ready = false;
	test_common.go = false;
	TEST_ASSERT_EQUAL_INT(0, pthread_create(&test_common.thread, NULL, test_waiterThread, NULL));
	test_common.threadCreated = true;

	/* the waiter holds the mutex from setting `ready` until pthread_cond_wait() releases it */
	for (;;) {
		TEST_ASSERT_EQUAL_INT(0, pthread_mutex_lock(&test_common.mutex));
		if (test_common.ready) {
			test_common.mutexLocked = true;
			break;
		}
		TEST_ASSERT_EQUAL_INT(0, pthread_mutex_unlock(&test_common.mutex));
		sched_yield();
	}
}


static void test_unlock(void)
{
	test_common.mutexLocked = false;
	TEST_ASSERT_EQUAL_INT(0, pthread_mutex_unlock(&test_common.mutex));
}


static void test_joinWaiter(void)
{
	TEST_ASSERT_EQUAL_INT(0, pthread_join(test_common.thread, NULL));
	test_common.threadCreated = false;
	TEST_ASSERT_EQUAL_INT(0, test_common.waitRet);
}


/* asserts that cond wakes up a thread blocked on it when signalled */
static void test_assertSignalWakesWaiter(pthread_cond_t *cond)
{
	test_startWaiterAndLock(cond);
	test_common.go = true;
	TEST_ASSERT_EQUAL_INT(0, pthread_cond_signal(cond));
	test_unlock();
	test_joinWaiter();
}


static void test_commonSetUp(void)
{
	TEST_ASSERT_EQUAL_INT(0, pthread_mutex_init(&test_common.mutex, NULL));
	test_common.cond = NULL;
	test_common.threadCreated = false;
	test_common.mutexLocked = false;
	test_common.ready = false;
	test_common.go = false;
	test_common.waitRet = 0;
}


static void test_commonTearDown(void)
{
	/* release a waiter left blocked by a failed test so that it can be joined */
	if (test_common.threadCreated) {
		if (!test_common.mutexLocked) {
			pthread_mutex_lock(&test_common.mutex);
		}
		test_common.go = true;
		pthread_cond_broadcast(test_common.cond);
		pthread_mutex_unlock(&test_common.mutex);
		test_common.mutexLocked = false;
		pthread_join(test_common.thread, NULL);
		test_common.threadCreated = false;
	}
	else if (test_common.mutexLocked) {
		pthread_mutex_unlock(&test_common.mutex);
		test_common.mutexLocked = false;
	}
	pthread_mutex_destroy(&test_common.mutex);
}


TEST_GROUP(pthread_condattr_init);

TEST_SETUP(pthread_condattr_init)
{
	test_commonSetUp();
}


TEST_TEAR_DOWN(pthread_condattr_init)
{
	test_commonTearDown();
}


TEST(pthread_condattr_init, init_returns_zero)
{
	pthread_condattr_t attr;

	TEST_ASSERT_EQUAL_INT(0, pthread_condattr_init(&attr));
	TEST_ASSERT_EQUAL_INT(0, pthread_condattr_destroy(&attr));
}


TEST(pthread_condattr_init, init_default_pshared_private)
{
	pthread_condattr_t attr;
	int pshared = -1;

	TEST_ASSERT_EQUAL_INT(0, pthread_condattr_init(&attr));
	TEST_ASSERT_EQUAL_INT(0, pthread_condattr_getpshared(&attr, &pshared));
	TEST_ASSERT_EQUAL_INT(PTHREAD_PROCESS_PRIVATE, pshared);
	TEST_ASSERT_EQUAL_INT(0, pthread_condattr_destroy(&attr));
}


TEST(pthread_condattr_init, init_default_clock_system)
{
	pthread_condattr_t attr;
	clockid_t clk = (clockid_t)-1;

	TEST_ASSERT_EQUAL_INT(0, pthread_condattr_init(&attr));
	TEST_ASSERT_EQUAL_INT(0, pthread_condattr_getclock(&attr, &clk));
	TEST_ASSERT_EQUAL_INT(CLOCK_REALTIME, clk);
	TEST_ASSERT_EQUAL_INT(0, pthread_condattr_destroy(&attr));
}


TEST(pthread_condattr_init, init_attr_usable_for_cond_init)
{
	pthread_condattr_t attr;
	pthread_cond_t cond;

	TEST_ASSERT_EQUAL_INT(0, pthread_condattr_init(&attr));
	TEST_ASSERT_EQUAL_INT(0, pthread_cond_init(&cond, &attr));

	/* default clock attribute: the timeout is measured against the system clock */
	test_assertTimedWaitTimesOut(&cond, CLOCK_REALTIME);
	test_assertSignalWakesWaiter(&cond);

	TEST_ASSERT_EQUAL_INT(0, pthread_cond_destroy(&cond));
	TEST_ASSERT_EQUAL_INT(0, pthread_condattr_destroy(&attr));
}


TEST(pthread_condattr_init, attr_change_does_not_affect_initialized_cond)
{
	pthread_condattr_t attr;
	pthread_cond_t cond;
	clockid_t clk;

	TEST_ASSERT_EQUAL_INT(0, pthread_condattr_init(&attr));
	TEST_ASSERT_EQUAL_INT(0, pthread_condattr_setclock(&attr, CLOCK_MONOTONIC));
	TEST_ASSERT_EQUAL_INT(0, pthread_cond_init(&cond, &attr));

	TEST_ASSERT_EQUAL_INT(0, pthread_condattr_setclock(&attr, CLOCK_REALTIME));
	TEST_ASSERT_EQUAL_INT(0, pthread_condattr_getclock(&attr, &clk));
	TEST_ASSERT_EQUAL_INT(CLOCK_REALTIME, clk);

	/* cond still measures its timeout against CLOCK_MONOTONIC */
	test_assertTimedWaitTimesOut(&cond, CLOCK_MONOTONIC);

	TEST_ASSERT_EQUAL_INT(0, pthread_cond_destroy(&cond));
	TEST_ASSERT_EQUAL_INT(0, pthread_condattr_destroy(&attr));
}


TEST_GROUP_RUNNER(pthread_condattr_init)
{
	RUN_TEST_CASE(pthread_condattr_init, init_returns_zero);
	RUN_TEST_CASE(pthread_condattr_init, init_default_pshared_private);
	RUN_TEST_CASE(pthread_condattr_init, init_default_clock_system);
	RUN_TEST_CASE(pthread_condattr_init, init_attr_usable_for_cond_init);
	RUN_TEST_CASE(pthread_condattr_init, attr_change_does_not_affect_initialized_cond);
}


TEST_GROUP(pthread_condattr_destroy);

TEST_SETUP(pthread_condattr_destroy)
{
	test_commonSetUp();
}


TEST_TEAR_DOWN(pthread_condattr_destroy)
{
	test_commonTearDown();
}


TEST(pthread_condattr_destroy, destroy_returns_zero)
{
	pthread_condattr_t attr;

	TEST_ASSERT_EQUAL_INT(0, pthread_condattr_init(&attr));
	TEST_ASSERT_EQUAL_INT(0, pthread_condattr_destroy(&attr));
}


TEST(pthread_condattr_destroy, destroy_modified_attr_returns_zero)
{
	pthread_condattr_t attr;

	TEST_ASSERT_EQUAL_INT(0, pthread_condattr_init(&attr));
	TEST_ASSERT_EQUAL_INT(0, pthread_condattr_setclock(&attr, CLOCK_MONOTONIC));
	TEST_ASSERT_EQUAL_INT(0, pthread_condattr_destroy(&attr));
}


TEST(pthread_condattr_destroy, reinit_after_destroy_restores_defaults)
{
	pthread_condattr_t attr;
	clockid_t clk = (clockid_t)-1;
	int pshared = -1;

	TEST_ASSERT_EQUAL_INT(0, pthread_condattr_init(&attr));
	TEST_ASSERT_EQUAL_INT(0, pthread_condattr_setclock(&attr, CLOCK_MONOTONIC));
	TEST_ASSERT_EQUAL_INT(0, pthread_condattr_destroy(&attr));

	TEST_ASSERT_EQUAL_INT(0, pthread_condattr_init(&attr));
	TEST_ASSERT_EQUAL_INT(0, pthread_condattr_getclock(&attr, &clk));
	TEST_ASSERT_EQUAL_INT(CLOCK_REALTIME, clk);
	TEST_ASSERT_EQUAL_INT(0, pthread_condattr_getpshared(&attr, &pshared));
	TEST_ASSERT_EQUAL_INT(PTHREAD_PROCESS_PRIVATE, pshared);
	TEST_ASSERT_EQUAL_INT(0, pthread_condattr_destroy(&attr));
}


TEST(pthread_condattr_destroy, destroy_does_not_affect_initialized_conds)
{
	pthread_condattr_t attr;
	pthread_cond_t cond1, cond2;

	TEST_ASSERT_EQUAL_INT(0, pthread_condattr_init(&attr));
	TEST_ASSERT_EQUAL_INT(0, pthread_condattr_setclock(&attr, CLOCK_MONOTONIC));
	TEST_ASSERT_EQUAL_INT(0, pthread_cond_init(&cond1, &attr));
	TEST_ASSERT_EQUAL_INT(0, pthread_cond_init(&cond2, &attr));
	TEST_ASSERT_EQUAL_INT(0, pthread_condattr_destroy(&attr));

	/* both condition variables keep the clock attribute and stay usable */
	test_assertTimedWaitTimesOut(&cond1, CLOCK_MONOTONIC);
	test_assertTimedWaitTimesOut(&cond2, CLOCK_MONOTONIC);
	test_assertSignalWakesWaiter(&cond1);
	test_assertSignalWakesWaiter(&cond2);

	TEST_ASSERT_EQUAL_INT(0, pthread_cond_destroy(&cond1));
	TEST_ASSERT_EQUAL_INT(0, pthread_cond_destroy(&cond2));
}


TEST_GROUP_RUNNER(pthread_condattr_destroy)
{
	RUN_TEST_CASE(pthread_condattr_destroy, destroy_returns_zero);
	RUN_TEST_CASE(pthread_condattr_destroy, destroy_modified_attr_returns_zero);
	RUN_TEST_CASE(pthread_condattr_destroy, reinit_after_destroy_restores_defaults);
	RUN_TEST_CASE(pthread_condattr_destroy, destroy_does_not_affect_initialized_conds);
}


TEST_GROUP(pthread_cond_destroy);

TEST_SETUP(pthread_cond_destroy)
{
	test_commonSetUp();
}


TEST_TEAR_DOWN(pthread_cond_destroy)
{
	test_commonTearDown();
}


TEST(pthread_cond_destroy, destroy_initialized_returns_zero)
{
	pthread_cond_t cond;

	TEST_ASSERT_EQUAL_INT(0, pthread_cond_init(&cond, NULL));
	TEST_ASSERT_EQUAL_INT(0, pthread_cond_destroy(&cond));
}


TEST(pthread_cond_destroy, destroy_static_initializer_returns_zero)
{
	pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

	TEST_ASSERT_EQUAL_INT(0, pthread_cond_destroy(&cond));
}


TEST(pthread_cond_destroy, destroy_used_static_initializer_returns_zero)
{
	pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

	test_assertSignalWakesWaiter(&cond);
	TEST_ASSERT_EQUAL_INT(0, pthread_cond_destroy(&cond));
}


TEST(pthread_cond_destroy, destroy_with_attr_returns_zero)
{
	pthread_condattr_t attr;
	pthread_cond_t cond;

	TEST_ASSERT_EQUAL_INT(0, pthread_condattr_init(&attr));
	TEST_ASSERT_EQUAL_INT(0, pthread_condattr_setclock(&attr, CLOCK_MONOTONIC));
	TEST_ASSERT_EQUAL_INT(0, pthread_cond_init(&cond, &attr));
	TEST_ASSERT_EQUAL_INT(0, pthread_condattr_destroy(&attr));

	TEST_ASSERT_EQUAL_INT(0, pthread_cond_destroy(&cond));
}


TEST(pthread_cond_destroy, destroy_after_waiter_returned)
{
	pthread_cond_t cond;

	TEST_ASSERT_EQUAL_INT(0, pthread_cond_init(&cond, NULL));
	test_assertSignalWakesWaiter(&cond);
	test_assertTimedWaitTimesOut(&cond, CLOCK_REALTIME);

	TEST_ASSERT_EQUAL_INT(0, pthread_cond_destroy(&cond));
}


TEST(pthread_cond_destroy, destroy_immediately_after_broadcast)
{
	pthread_cond_t cond;

	TEST_ASSERT_EQUAL_INT(0, pthread_cond_init(&cond, NULL));
	test_startWaiterAndLock(&cond);

	/* once broadcast, no thread is blocked on cond - the woken waiter can't reacquire the mutex yet */
	test_common.go = true;
	TEST_ASSERT_EQUAL_INT(0, pthread_cond_broadcast(&cond));
	TEST_ASSERT_EQUAL_INT(0, pthread_cond_destroy(&cond));
	test_unlock();

	test_joinWaiter();
}


TEST(pthread_cond_destroy, reinit_after_destroy)
{
	pthread_condattr_t attr;
	pthread_cond_t cond;

	TEST_ASSERT_EQUAL_INT(0, pthread_cond_init(&cond, NULL));
	TEST_ASSERT_EQUAL_INT(0, pthread_cond_destroy(&cond));

	TEST_ASSERT_EQUAL_INT(0, pthread_condattr_init(&attr));
	TEST_ASSERT_EQUAL_INT(0, pthread_condattr_setclock(&attr, CLOCK_MONOTONIC));
	TEST_ASSERT_EQUAL_INT(0, pthread_cond_init(&cond, &attr));
	TEST_ASSERT_EQUAL_INT(0, pthread_condattr_destroy(&attr));

	/* the reinitialized object takes the new attributes */
	test_assertTimedWaitTimesOut(&cond, CLOCK_MONOTONIC);
	test_assertSignalWakesWaiter(&cond);

	TEST_ASSERT_EQUAL_INT(0, pthread_cond_destroy(&cond));
}


TEST_GROUP_RUNNER(pthread_cond_destroy)
{
	RUN_TEST_CASE(pthread_cond_destroy, destroy_initialized_returns_zero);
	RUN_TEST_CASE(pthread_cond_destroy, destroy_static_initializer_returns_zero);
	RUN_TEST_CASE(pthread_cond_destroy, destroy_used_static_initializer_returns_zero);
	RUN_TEST_CASE(pthread_cond_destroy, destroy_with_attr_returns_zero);
	RUN_TEST_CASE(pthread_cond_destroy, destroy_after_waiter_returned);
	RUN_TEST_CASE(pthread_cond_destroy, destroy_immediately_after_broadcast);
	RUN_TEST_CASE(pthread_cond_destroy, reinit_after_destroy);
}
