/*
 * Phoenix-RTOS
 *
 * phoenix-rtos-tests
 *
 * test/libc/pthread
 *
 * pthread state in a child process created by fork()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Adam Greloch
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "unity_fixture.h"


#define CHILD_CHECK(cond, code) \
	do { \
		if (!(cond)) { \
			_exit(code); \
		} \
	} while (0)


#define CHURN_FORKS 20


static struct {
	pthread_key_t key;
	pthread_mutex_t lock;
	pthread_cond_t cond;
	int release;
	int stop;
} fork_common;


/* Non-stack values stored as thread-specific data */
static int spec_dummy[2];


static void *test_noopThread(void *arg)
{
	return arg;
}


/* Keeps another thread alive in the parent (and on the pthread list) across fork() */
static void *test_waiterThread(void *arg)
{
	pthread_mutex_lock(&fork_common.lock);
	while (fork_common.release == 0) {
		pthread_cond_wait(&fork_common.cond, &fork_common.lock);
	}
	pthread_mutex_unlock(&fork_common.lock);

	return NULL;
}


static void test_waiterStart(pthread_t *tid)
{
	fork_common.release = 0;
	TEST_ASSERT_EQUAL_INT(0, pthread_create(tid, NULL, test_waiterThread, NULL));
}


static void test_waiterStop(pthread_t tid)
{
	pthread_mutex_lock(&fork_common.lock);
	fork_common.release = 1;
	pthread_cond_broadcast(&fork_common.cond);
	pthread_mutex_unlock(&fork_common.lock);

	TEST_ASSERT_EQUAL_INT(0, pthread_join(tid, NULL));
}


/* Exercises thread creation and release (both joinable and detached) in the child */
static int test_childThreads(void)
{
	pthread_t tid;
	pthread_attr_t attr;
	void *ret = NULL;
	int i;

	if (pthread_attr_init(&attr) != 0 || pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0) {
		return -1;
	}

	for (i = 0; i < 4; i++) {
		if (pthread_create(&tid, &attr, test_noopThread, NULL) != 0) {
			pthread_attr_destroy(&attr);
			return -1;
		}
		if (pthread_create(&tid, NULL, test_noopThread, (void *)(uintptr_t)(i + 1)) != 0) {
			pthread_attr_destroy(&attr);
			return -1;
		}
		if (pthread_join(tid, &ret) != 0 || ret != (void *)(uintptr_t)(i + 1)) {
			pthread_attr_destroy(&attr);
			return -1;
		}
	}

	pthread_attr_destroy(&attr);

	return 0;
}


/*
 * createThreads: if other threads may be inside pthread_create()/malloc() at fork(), the child
 * must not create threads (only async-signal-safe calls are allowed in a child of a multithreaded
 * process, and e.g. sanitizer runtimes or malloc may deadlock on locks held by the other threads)
 */
static void test_child(pthread_t parentSelf, int createThreads)
{
	void *spec;

	/* The child's only thread is a copy of the thread which called fork() */
	CHILD_CHECK(pthread_equal(pthread_self(), parentSelf), 10);

	/* Thread-specific data of the forking thread is preserved */
	spec = pthread_getspecific(fork_common.key);
	CHILD_CHECK(spec == (void *)&fork_common, 11);
	CHILD_CHECK(pthread_setspecific(fork_common.key, &spec_dummy[0]) == 0, 12);
	CHILD_CHECK(pthread_getspecific(fork_common.key) == (void *)&spec_dummy[0], 13);

	errno = 0;
	CHILD_CHECK(errno == 0, 14);
	errno = EINVAL;
	CHILD_CHECK(errno == EINVAL, 15);

	if (createThreads != 0) {
		CHILD_CHECK(test_childThreads() == 0, 16);
		CHILD_CHECK(pthread_equal(pthread_self(), parentSelf), 17);
	}

	_exit(EXIT_SUCCESS);
}


/* Returns child's pid or -errno */
static pid_t test_forkAndCheck(int createThreads)
{
	pthread_t self = pthread_self();
	pid_t pid;

	if (pthread_setspecific(fork_common.key, &fork_common) != 0) {
		return -EINVAL;
	}

	pid = fork();
	if (pid == 0) {
		test_child(self, createThreads);
		/* no return */
	}

	return (pid < 0) ? -errno : pid;
}


typedef struct {
	int createThreads;
	pid_t pid;
} test_forkerArg_t;


static void *test_forkerThread(void *arg)
{
	test_forkerArg_t *forkerArg = arg;

	forkerArg->pid = test_forkAndCheck(forkerArg->createThreads);
	return NULL;
}


static void test_waitChild(pid_t pid)
{
	int status;

	if (pid == -ENOSYS) {
		TEST_IGNORE_MESSAGE("fork syscall not supported");
	}
	TEST_ASSERT_GREATER_THAN_INT(0, pid);

	TEST_ASSERT_EQUAL_INT(pid, waitpid(pid, &status, 0));
	TEST_ASSERT_TRUE_MESSAGE(WIFEXITED(status), "child terminated abnormally");
	TEST_ASSERT_EQUAL_INT_MESSAGE(EXIT_SUCCESS, WEXITSTATUS(status), "child check failed");
}


static pid_t test_forkFromThread(int createThreads)
{
	pthread_t tid;
	test_forkerArg_t arg = { .createThreads = createThreads, .pid = 0 };

	TEST_ASSERT_EQUAL_INT(0, pthread_create(&tid, NULL, test_forkerThread, &arg));
	TEST_ASSERT_EQUAL_INT(0, pthread_join(tid, NULL));

	return arg.pid;
}


static int test_churnStopped(void)
{
	int stop;

	pthread_mutex_lock(&fork_common.lock);
	stop = fork_common.stop;
	pthread_mutex_unlock(&fork_common.lock);

	return stop;
}


static void *test_churnThread(void *arg)
{
	pthread_t tid;
	pthread_attr_t attr;

	(void)arg;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	while (test_churnStopped() == 0) {
		if (pthread_create(&tid, NULL, test_noopThread, NULL) == 0) {
			pthread_join(tid, NULL);
		}
		(void)pthread_create(&tid, &attr, test_noopThread, NULL);
		(void)pthread_setspecific(fork_common.key, &spec_dummy[1]);
		usleep(100);
	}

	pthread_attr_destroy(&attr);

	return NULL;
}


TEST_GROUP(test_pthread_fork);


TEST_SETUP(test_pthread_fork)
{
	TEST_ASSERT_EQUAL_INT(0, pthread_key_create(&fork_common.key, NULL));
	TEST_ASSERT_EQUAL_INT(0, pthread_mutex_init(&fork_common.lock, NULL));
	TEST_ASSERT_EQUAL_INT(0, pthread_cond_init(&fork_common.cond, NULL));
	fork_common.release = 0;
	fork_common.stop = 0;
}


TEST_TEAR_DOWN(test_pthread_fork)
{
	pthread_cond_destroy(&fork_common.cond);
	pthread_mutex_destroy(&fork_common.lock);
	pthread_key_delete(fork_common.key);
}


TEST(test_pthread_fork, fork_main_thread)
{
	test_waitChild(test_forkAndCheck(1));
}


TEST(test_pthread_fork, fork_non_main_thread)
{
	test_waitChild(test_forkFromThread(1));
}


TEST(test_pthread_fork, fork_non_main_thread_with_other_threads)
{
	pthread_t waiter;
	pid_t pid;

	test_waiterStart(&waiter);
	pid = test_forkFromThread(1);
	test_waiterStop(waiter);

	test_waitChild(pid);
}


TEST(test_pthread_fork, fork_after_detached_thread_exit)
{
	pthread_t tid;
	pthread_attr_t attr;

	/* A detached thread which has exited leaves its stack pending for release */
	TEST_ASSERT_EQUAL_INT(0, pthread_attr_init(&attr));
	TEST_ASSERT_EQUAL_INT(0, pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED));
	TEST_ASSERT_EQUAL_INT(0, pthread_create(&tid, &attr, test_noopThread, NULL));
	pthread_attr_destroy(&attr);
	/* No way to wait for a detached thread; if it is not released yet, the test just covers less */
	usleep(100 * 1000);

	test_waitChild(test_forkFromThread(1));
}


TEST(test_pthread_fork, fork_during_thread_churn)
{
	pthread_t churn;
	pid_t pids[CHURN_FORKS];
	int i, n;

	TEST_ASSERT_EQUAL_INT(0, pthread_create(&churn, NULL, test_churnThread, NULL));

	for (n = 0; n < CHURN_FORKS; n++) {
		/* churn thread may hold locks at fork() - don't create threads in the child */
		pids[n] = (n % 2 == 0) ? test_forkAndCheck(0) : test_forkFromThread(0);
		if (pids[n] < 0) {
			break;
		}
	}

	pthread_mutex_lock(&fork_common.lock);
	fork_common.stop = 1;
	pthread_mutex_unlock(&fork_common.lock);
	TEST_ASSERT_EQUAL_INT(0, pthread_join(churn, NULL));

	for (i = 0; i < n; i++) {
		test_waitChild(pids[i]);
	}
	if (n < CHURN_FORKS) {
		test_waitChild(pids[n]);
	}
}


TEST_GROUP_RUNNER(test_pthread_fork)
{
	RUN_TEST_CASE(test_pthread_fork, fork_main_thread);
	RUN_TEST_CASE(test_pthread_fork, fork_non_main_thread);
	RUN_TEST_CASE(test_pthread_fork, fork_non_main_thread_with_other_threads);
	RUN_TEST_CASE(test_pthread_fork, fork_after_detached_thread_exit);
	RUN_TEST_CASE(test_pthread_fork, fork_during_thread_churn);
}
