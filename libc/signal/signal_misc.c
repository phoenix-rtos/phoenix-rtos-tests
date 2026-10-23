/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <signal.h>
 * TESTED:
 *    - killpg()
 *    - raise()
 *    - sigaltstack()
 *    - signal()
 *    - sigpending()
 *    - sigqueue()
 *    - sigtimedwait()
 *    - sigwait()
 *    - sigwaitinfo()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "sig_internal.h"

#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <unity_fixture.h>


TEST_GROUP(signal_raise);

static volatile sig_atomic_t test_handlerCalled;
static volatile sig_atomic_t test_handlerSig;


static void test_sigHandler(int sig)
{
	test_handlerCalled = 1;
	test_handlerSig = sig;
}


TEST_SETUP(signal_raise)
{
	test_handlerCalled = 0;
	test_handlerSig = 0;
}

TEST_TEAR_DOWN(signal_raise)
{
	signal(SIGUSR1, SIG_DFL);
	signal(SIGUSR2, SIG_DFL);
}


TEST(signal_raise, raise_delivers_signal)
{
	void (*prev)(int);
	int ret;

	prev = signal(SIGUSR1, test_sigHandler);
	TEST_ASSERT_TRUE(prev != SIG_ERR);

	test_handlerCalled = 0;
	ret = raise(SIGUSR1);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(1, test_handlerCalled);
	TEST_ASSERT_EQUAL_INT(SIGUSR1, test_handlerSig);
}


TEST(signal_raise, raise_returns_before_handler_done)
{
	void (*prev)(int);
	int ret;

	prev = signal(SIGUSR2, test_sigHandler);
	TEST_ASSERT_TRUE(prev != SIG_ERR);

	test_handlerCalled = 0;
	ret = raise(SIGUSR2);
	TEST_ASSERT_EQUAL_INT(0, ret);
	/* POSIX: raise shall not return until after the signal handler does */
	TEST_ASSERT_EQUAL_INT(1, test_handlerCalled);
}


TEST(signal_raise, raise_einval_invalid_signal)
{
	int ret;

	errno = 0;
	ret = raise(-1);
	TEST_ASSERT_TRUE(ret != 0);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST(signal_raise, signal_set_handler)
{
	void (*prev)(int);

	prev = signal(SIGUSR1, test_sigHandler);
	TEST_ASSERT_TRUE(prev != SIG_ERR);

	test_handlerCalled = 0;
	raise(SIGUSR1);
	TEST_ASSERT_EQUAL_INT(1, test_handlerCalled);
}


TEST(signal_raise, signal_sig_ign)
{
	void (*prev)(int);

	prev = signal(SIGUSR1, SIG_IGN);
	TEST_ASSERT_TRUE(prev != SIG_ERR);

	/* Should not crash or invoke handler */
	raise(SIGUSR1);
}


TEST(signal_raise, signal_returns_previous_handler)
{
	void (*prev1)(int);
	void (*prev2)(int);

	prev1 = signal(SIGUSR1, test_sigHandler);
	TEST_ASSERT_TRUE(prev1 != SIG_ERR);

	prev2 = signal(SIGUSR1, SIG_DFL);
	TEST_ASSERT_TRUE(prev2 == test_sigHandler);
}


TEST(signal_raise, signal_einval_invalid_signal)
{
	void (*ret)(int);

	errno = 0;
	ret = signal(-1, test_sigHandler);
	TEST_ASSERT_TRUE(ret == SIG_ERR);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST(signal_raise, signal_einval_sigkill)
{
	void (*ret)(int);

	errno = 0;
	ret = signal(SIGKILL, test_sigHandler);
	TEST_ASSERT_TRUE(ret == SIG_ERR);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST(signal_raise, signal_einval_sigstop)
{
	void (*ret)(int);

	errno = 0;
	ret = signal(SIGSTOP, test_sigHandler);
	TEST_ASSERT_TRUE(ret == SIG_ERR);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST_GROUP_RUNNER(signal_raise)
{
	RUN_TEST_CASE(signal_raise, raise_delivers_signal);
	RUN_TEST_CASE(signal_raise, raise_returns_before_handler_done);
	RUN_TEST_CASE(signal_raise, raise_einval_invalid_signal);
	RUN_TEST_CASE(signal_raise, signal_set_handler);
	RUN_TEST_CASE(signal_raise, signal_sig_ign);
	RUN_TEST_CASE(signal_raise, signal_returns_previous_handler);
	RUN_TEST_CASE(signal_raise, signal_einval_invalid_signal);
	RUN_TEST_CASE(signal_raise, signal_einval_sigkill);
	RUN_TEST_CASE(signal_raise, signal_einval_sigstop);
}


TEST_GROUP(signal_pending);

TEST_SETUP(signal_pending)
{
	test_handlerCalled = 0;
	test_handlerSig = 0;
}

TEST_TEAR_DOWN(signal_pending)
{
	sigset_t pending, unblock;

	/* Consume any pending signals before unblocking to avoid termination */
	sigemptyset(&unblock);
	sigaddset(&unblock, SIGUSR1);
	sigaddset(&unblock, SIGUSR2);

#ifndef __phoenix__
	struct timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = 0;
	sigpending(&pending);
	if (sigismember(&pending, SIGUSR1)) {
		sigtimedwait(&unblock, NULL, &ts);
	}
	sigpending(&pending);
	if (sigismember(&pending, SIGUSR2)) {
		sigtimedwait(&unblock, NULL, &ts);
	}
#else
	(void)pending;
	signal(SIGUSR1, SIG_IGN);
	signal(SIGUSR2, SIG_IGN);
	sigprocmask(SIG_UNBLOCK, &unblock, NULL);
#endif

	sigprocmask(SIG_UNBLOCK, &unblock, NULL);
	signal(SIGUSR1, SIG_DFL);
	signal(SIGUSR2, SIG_DFL);
}


TEST(signal_pending, sigpending_empty_when_none_blocked)
{
	sigset_t pending;
	int ret;

	/* Unblock SIGUSR1 to ensure it's not pending */
	sigset_t unblock;
	ret = sigemptyset(&unblock);
	TEST_ASSERT_EQUAL_INT(0, ret);
	ret = sigaddset(&unblock, SIGUSR1);
	TEST_ASSERT_EQUAL_INT(0, ret);
	ret = sigprocmask(SIG_UNBLOCK, &unblock, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = sigpending(&pending);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, sigismember(&pending, SIGUSR1));
}


TEST(signal_pending, sigpending_shows_blocked_pending)
{
	sigset_t block, pending;
	int ret;

	/* Block SIGUSR1 */
	ret = sigemptyset(&block);
	TEST_ASSERT_EQUAL_INT(0, ret);
	ret = sigaddset(&block, SIGUSR1);
	TEST_ASSERT_EQUAL_INT(0, ret);
	ret = sigprocmask(SIG_BLOCK, &block, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Send SIGUSR1 to self - it becomes pending */
	ret = raise(SIGUSR1);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = sigpending(&pending);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(1, sigismember(&pending, SIGUSR1));
}


TEST(signal_pending, sigpending_shows_process_directed_pending)
{
	sigset_t block, pending;
	int ret;

	ret = sigemptyset(&block);
	TEST_ASSERT_EQUAL_INT(0, ret);
	ret = sigaddset(&block, SIGUSR2);
	TEST_ASSERT_EQUAL_INT(0, ret);
	ret = sigprocmask(SIG_BLOCK, &block, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* kill() makes the signal pending on the process, raise() on the thread */
	ret = kill(getpid(), SIGUSR2);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = sigpending(&pending);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(1, sigismember(&pending, SIGUSR2));
	TEST_ASSERT_EQUAL_INT(0, sigismember(&pending, SIGUSR1));
}


TEST(signal_pending, sigpending_sig_ign_discards_pending)
{
	sigset_t block, pending;
	struct sigaction act;
	int ret;

	ret = sigemptyset(&block);
	TEST_ASSERT_EQUAL_INT(0, ret);
	ret = sigaddset(&block, SIGUSR1);
	TEST_ASSERT_EQUAL_INT(0, ret);
	ret = sigprocmask(SIG_BLOCK, &block, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = raise(SIGUSR1);
	TEST_ASSERT_EQUAL_INT(0, ret);

	memset(&act, 0, sizeof(act));
	act.sa_handler = SIG_IGN;
	ret = sigaction(SIGUSR1, &act, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = sigpending(&pending);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, sigismember(&pending, SIGUSR1));
}


TEST(signal_pending, sigpending_sig_dfl_discards_default_ignored)
{
	sigset_t block, pending;
	struct sigaction act;
	int ret;

	ret = sigemptyset(&block);
	TEST_ASSERT_EQUAL_INT(0, ret);
	ret = sigaddset(&block, SIGCHLD);
	TEST_ASSERT_EQUAL_INT(0, ret);
	ret = sigprocmask(SIG_BLOCK, &block, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/*
	 * "it is unspecified whether the signal is discarded immediately upon generation or remains pending",
	 * in both cases it is not pending once the action is set to SIG_DFL
	 */
	ret = raise(SIGCHLD);
	TEST_ASSERT_EQUAL_INT(0, ret);

	memset(&act, 0, sizeof(act));
	act.sa_handler = SIG_DFL;
	ret = sigaction(SIGCHLD, &act, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = sigpending(&pending);
	sigprocmask(SIG_UNBLOCK, &block, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, sigismember(&pending, SIGCHLD));
}


static void *test_blockAndWait(void *arg)
{
	sigset_t block;

	sigemptyset(&block);
	sigaddset(&block, SIGUSR1);
	pthread_sigmask(SIG_BLOCK, &block, NULL);
	sem_post((sem_t *)arg);

	/* Wait for the main thread to make the signal pending here and check its own set */
	sem_wait((sem_t *)arg + 1);

	return NULL;
}


TEST(signal_pending, sigpending_excludes_other_thread_pending)
{
	sigset_t block, pending;
	pthread_t thread;
	sem_t sems[2];
	int ret;

	ret = sigemptyset(&block);
	TEST_ASSERT_EQUAL_INT(0, ret);
	ret = sigaddset(&block, SIGUSR1);
	TEST_ASSERT_EQUAL_INT(0, ret);
	ret = sigprocmask(SIG_BLOCK, &block, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	TEST_ASSERT_EQUAL_INT(0, sem_init(&sems[0], 0, 0));
	TEST_ASSERT_EQUAL_INT(0, sem_init(&sems[1], 0, 0));
	TEST_ASSERT_EQUAL_INT(0, pthread_create(&thread, NULL, test_blockAndWait, sems));
	TEST_ASSERT_EQUAL_INT(0, sem_wait(&sems[0]));

	ret = pthread_kill(thread, SIGUSR1);

	sigemptyset(&pending);
	sigpending(&pending);

	sem_post(&sems[1]);
	pthread_join(thread, NULL);
	sem_destroy(&sems[0]);
	sem_destroy(&sems[1]);

	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, sigismember(&pending, SIGUSR1));
}


TEST(signal_pending, sigwait_consumes_pending)
{
	sigset_t block, waitSet;
	int ret, sig;

	/* Block SIGUSR1 */
	ret = sigemptyset(&block);
	TEST_ASSERT_EQUAL_INT(0, ret);
	ret = sigaddset(&block, SIGUSR1);
	TEST_ASSERT_EQUAL_INT(0, ret);
	ret = sigprocmask(SIG_BLOCK, &block, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Send SIGUSR1 to self */
	ret = raise(SIGUSR1);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Wait for SIGUSR1 */
	ret = sigemptyset(&waitSet);
	TEST_ASSERT_EQUAL_INT(0, ret);
	ret = sigaddset(&waitSet, SIGUSR1);
	TEST_ASSERT_EQUAL_INT(0, ret);

	sig = 0;
	ret = sigwait(&waitSet, &sig);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(SIGUSR1, sig);
}


TEST(signal_pending, sigwaitinfo_returns_signal_number)
{
	sigset_t block, waitSet;
	int ret;
	siginfo_t info;

	/* Block SIGUSR2 */
	ret = sigemptyset(&block);
	TEST_ASSERT_EQUAL_INT(0, ret);
	ret = sigaddset(&block, SIGUSR2);
	TEST_ASSERT_EQUAL_INT(0, ret);
	ret = sigprocmask(SIG_BLOCK, &block, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Send SIGUSR2 to self */
	ret = raise(SIGUSR2);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = sigemptyset(&waitSet);
	TEST_ASSERT_EQUAL_INT(0, ret);
	ret = sigaddset(&waitSet, SIGUSR2);
	TEST_ASSERT_EQUAL_INT(0, ret);

	memset(&info, 0, sizeof(info));
	ret = sigwaitinfo(&waitSet, &info);
	TEST_ASSERT_EQUAL_INT(SIGUSR2, ret);
	TEST_ASSERT_EQUAL_INT(SIGUSR2, info.si_signo);
}


struct test_sigsender {
	pthread_t target;
	int sigs[2];
};


static void *test_sendLater(void *arg)
{
	struct test_sigsender *s = arg;
	int i;

	for (i = 0; i < 2; i++) {
		if (s->sigs[i] != 0) {
			usleep(10000);
			pthread_kill(s->target, s->sigs[i]);
		}
	}

	return NULL;
}


TEST(signal_pending, sigwait_blocks_until_signal)
{
	struct test_sigsender sender = { .target = pthread_self(), .sigs = { SIGUSR1, 0 } };
	sigset_t set;
	pthread_t thread;
	int ret, sig = 0;

	sigemptyset(&set);
	sigaddset(&set, SIGUSR1);
	TEST_ASSERT_EQUAL_INT(0, sigprocmask(SIG_BLOCK, &set, NULL));
	TEST_ASSERT_EQUAL_INT(0, pthread_create(&thread, NULL, test_sendLater, &sender));

	ret = sigwait(&set, &sig);
	pthread_join(thread, NULL);

	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(SIGUSR1, sig);
}


TEST(signal_pending, sigwait_not_interrupted_by_handler)
{
	struct test_sigsender sender = { .target = pthread_self(), .sigs = { SIGUSR2, SIGUSR1 } };
	sigset_t set;
	pthread_t thread;
	int ret, sig = 0;

	TEST_ASSERT_NOT_EQUAL(SIG_ERR, signal(SIGUSR2, test_sigHandler));
	sigemptyset(&set);
	sigaddset(&set, SIGUSR1);
	TEST_ASSERT_EQUAL_INT(0, sigprocmask(SIG_BLOCK, &set, NULL));
	TEST_ASSERT_EQUAL_INT(0, pthread_create(&thread, NULL, test_sendLater, &sender));

	ret = sigwait(&set, &sig);
	pthread_join(thread, NULL);

	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(SIGUSR1, sig);
	TEST_ASSERT_EQUAL_INT(SIGUSR2, test_handlerSig);
}


TEST(signal_pending, sigwaitinfo_eintr_on_handler)
{
	struct test_sigsender sender = { .target = pthread_self(), .sigs = { SIGUSR2, 0 } };
	sigset_t set;
	siginfo_t info;
	pthread_t thread;
	int ret;

	TEST_ASSERT_NOT_EQUAL(SIG_ERR, signal(SIGUSR2, test_sigHandler));
	sigemptyset(&set);
	sigaddset(&set, SIGUSR1);
	TEST_ASSERT_EQUAL_INT(0, sigprocmask(SIG_BLOCK, &set, NULL));
	TEST_ASSERT_EQUAL_INT(0, pthread_create(&thread, NULL, test_sendLater, &sender));

	errno = 0;
	ret = sigwaitinfo(&set, &info);
	pthread_join(thread, NULL);

	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EINTR, errno);
	TEST_ASSERT_EQUAL_INT(SIGUSR2, test_handlerSig);
}


struct test_sigwaiter {
	sigset_t set;
	sem_t ready;
	sem_t done;
	int sig;
	int ret;
};


static void *test_sigwaitThread(void *arg)
{
	struct test_sigwaiter *w = arg;

	sem_post(&w->ready);
	w->ret = sigwait(&w->set, &w->sig);
	sem_post(&w->done);

	return NULL;
}


static int test_semWaitSec(sem_t *sem)
{
	struct timespec ts;

	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += 1;

	return sem_timedwait(sem, &ts);
}


TEST(signal_pending, sigwait_process_directed_reaches_matching_waiter)
{
	static const int sigs[2] = { SIGUSR1, SIGUSR2 };
	struct test_sigwaiter w[2];
	pthread_t threads[2];
	sigset_t block;
	int i, done[2];

	sigemptyset(&block);
	sigaddset(&block, SIGUSR1);
	sigaddset(&block, SIGUSR2);
	TEST_ASSERT_EQUAL_INT(0, sigprocmask(SIG_BLOCK, &block, NULL));

	for (i = 0; i < 2; i++) {
		sigemptyset(&w[i].set);
		sigaddset(&w[i].set, sigs[i]);
		w[i].sig = 0;
		w[i].ret = -1;
		TEST_ASSERT_EQUAL_INT(0, sem_init(&w[i].ready, 0, 0));
		TEST_ASSERT_EQUAL_INT(0, sem_init(&w[i].done, 0, 0));
		TEST_ASSERT_EQUAL_INT(0, pthread_create(&threads[i], NULL, test_sigwaitThread, &w[i]));
		TEST_ASSERT_EQUAL_INT(0, sem_wait(&w[i].ready));
	}
	usleep(10000);

	/* Each waiter must get the signal in its own set, whichever thread the kernel checks first */
	TEST_ASSERT_EQUAL_INT(0, kill(getpid(), SIGUSR2));
	done[1] = test_semWaitSec(&w[1].done);
	TEST_ASSERT_EQUAL_INT(0, kill(getpid(), SIGUSR1));
	done[0] = test_semWaitSec(&w[0].done);

	for (i = 0; i < 2; i++) {
		TEST_ASSERT_EQUAL_INT(0, done[i]);
		pthread_join(threads[i], NULL);
		TEST_ASSERT_EQUAL_INT(0, w[i].ret);
		TEST_ASSERT_EQUAL_INT(sigs[i], w[i].sig);
		sem_destroy(&w[i].ready);
		sem_destroy(&w[i].done);
	}
}


TEST(signal_pending, sigtimedwait_immediate_pending)
{
	sigset_t block, waitSet;
	int ret;
	siginfo_t info;
	struct timespec ts;

	/* Block SIGUSR1 */
	ret = sigemptyset(&block);
	TEST_ASSERT_EQUAL_INT(0, ret);
	ret = sigaddset(&block, SIGUSR1);
	TEST_ASSERT_EQUAL_INT(0, ret);
	ret = sigprocmask(SIG_BLOCK, &block, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Send SIGUSR1 */
	ret = raise(SIGUSR1);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = sigemptyset(&waitSet);
	TEST_ASSERT_EQUAL_INT(0, ret);
	ret = sigaddset(&waitSet, SIGUSR1);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ts.tv_sec = 5;
	ts.tv_nsec = 0;
	memset(&info, 0, sizeof(info));
	ret = sigtimedwait(&waitSet, &info, &ts);
	TEST_ASSERT_EQUAL_INT(SIGUSR1, ret);
	TEST_ASSERT_EQUAL_INT(SIGUSR1, info.si_signo);
}


TEST(signal_pending, sigtimedwait_eagain_timeout)
{
	sigset_t block, waitSet;
	int ret;
	struct timespec ts;

	/* Block SIGUSR1 but don't send it */
	ret = sigemptyset(&block);
	TEST_ASSERT_EQUAL_INT(0, ret);
	ret = sigaddset(&block, SIGUSR1);
	TEST_ASSERT_EQUAL_INT(0, ret);
	ret = sigprocmask(SIG_BLOCK, &block, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = sigemptyset(&waitSet);
	TEST_ASSERT_EQUAL_INT(0, ret);
	ret = sigaddset(&waitSet, SIGUSR1);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Zero timeout - immediate return */
	ts.tv_sec = 0;
	ts.tv_nsec = 0;
	errno = 0;
	ret = sigtimedwait(&waitSet, NULL, &ts);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EAGAIN, errno);
}


TEST(signal_pending, sigtimedwait_einval_bad_nsec)
{
	sigset_t block, waitSet;
	int ret;
	struct timespec ts;

	/* Block SIGUSR1 but don't send it */
	ret = sigemptyset(&block);
	TEST_ASSERT_EQUAL_INT(0, ret);
	ret = sigaddset(&block, SIGUSR1);
	TEST_ASSERT_EQUAL_INT(0, ret);
	ret = sigprocmask(SIG_BLOCK, &block, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = sigemptyset(&waitSet);
	TEST_ASSERT_EQUAL_INT(0, ret);
	ret = sigaddset(&waitSet, SIGUSR1);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ts.tv_sec = 0;
	ts.tv_nsec = -1;
	errno = 0;
	ret = sigtimedwait(&waitSet, NULL, &ts);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST(signal_pending, sigtimedwait_einval_negative_sec)
{
	struct timespec ts = { .tv_sec = -1, .tv_nsec = 0 };
	sigset_t set;
	int ret;

	sigemptyset(&set);
	sigaddset(&set, SIGUSR1);
	TEST_ASSERT_EQUAL_INT(0, sigprocmask(SIG_BLOCK, &set, NULL));

	errno = 0;
	ret = sigtimedwait(&set, NULL, &ts);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST(signal_pending, sigtimedwait_eagain_after_timeout)
{
	struct timespec ts = { .tv_sec = 0, .tv_nsec = 50 * 1000 * 1000 };
	struct timespec start, stop;
	sigset_t set;
	long long elapsedUs;
	int ret;

	sigemptyset(&set);
	sigaddset(&set, SIGUSR1);
	TEST_ASSERT_EQUAL_INT(0, sigprocmask(SIG_BLOCK, &set, NULL));

	clock_gettime(CLOCK_MONOTONIC, &start);
	errno = 0;
	ret = sigtimedwait(&set, NULL, &ts);
	clock_gettime(CLOCK_MONOTONIC, &stop);

	elapsedUs = (stop.tv_sec - start.tv_sec) * 1000000LL + (stop.tv_nsec - start.tv_nsec) / 1000;
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EAGAIN, errno);
	TEST_ASSERT_GREATER_OR_EQUAL(50 * 1000, elapsedUs);
}


TEST(signal_pending, sigtimedwait_receives_during_wait)
{
	struct test_sigsender sender = { .target = pthread_self(), .sigs = { SIGUSR1, 0 } };
	struct timespec ts = { .tv_sec = 5, .tv_nsec = 0 };
	sigset_t set;
	siginfo_t info;
	pthread_t thread;
	int ret;

	sigemptyset(&set);
	sigaddset(&set, SIGUSR1);
	TEST_ASSERT_EQUAL_INT(0, sigprocmask(SIG_BLOCK, &set, NULL));
	TEST_ASSERT_EQUAL_INT(0, pthread_create(&thread, NULL, test_sendLater, &sender));

	memset(&info, 0, sizeof(info));
	ret = sigtimedwait(&set, &info, &ts);
	pthread_join(thread, NULL);

	TEST_ASSERT_EQUAL_INT(SIGUSR1, ret);
	TEST_ASSERT_EQUAL_INT(SIGUSR1, info.si_signo);
}


#ifndef __phoenix__
TEST(signal_pending, sigqueue_delivers_signal)
{
	sigset_t block, waitSet;
	int ret;
	siginfo_t info;
	struct timespec ts;
	union sigval val;

	/* Block SIGUSR1 */
	ret = sigemptyset(&block);
	TEST_ASSERT_EQUAL_INT(0, ret);
	ret = sigaddset(&block, SIGUSR1);
	TEST_ASSERT_EQUAL_INT(0, ret);
	ret = sigprocmask(SIG_BLOCK, &block, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Set up SA_SIGINFO handler to allow queued values */
	struct sigaction sa, oldsa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = (void (*)(int, siginfo_t *, void *))SIG_DFL;
	sa.sa_flags = SA_SIGINFO;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGUSR1, &sa, &oldsa);

	val.sival_int = 42;
	errno = 0;
	ret = sigqueue(getpid(), SIGUSR1, val);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, errno);

	ret = sigemptyset(&waitSet);
	TEST_ASSERT_EQUAL_INT(0, ret);
	ret = sigaddset(&waitSet, SIGUSR1);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ts.tv_sec = 5;
	ts.tv_nsec = 0;
	memset(&info, 0, sizeof(info));
	ret = sigtimedwait(&waitSet, &info, &ts);
	TEST_ASSERT_EQUAL_INT(SIGUSR1, ret);
	TEST_ASSERT_EQUAL_INT(SIGUSR1, info.si_signo);
	TEST_ASSERT_EQUAL_INT(42, info.si_value.sival_int);

	sigaction(SIGUSR1, &oldsa, NULL);
}


TEST(signal_pending, sigqueue_einval_invalid_signal)
{
	union sigval val;
	int ret;

	val.sival_int = 0;
	errno = 0;
	ret = sigqueue(getpid(), -1, val);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST(signal_pending, sigqueue_esrch_invalid_pid)
{
	union sigval val;
	int ret;

	val.sival_int = 0;
	errno = 0;
	ret = sigqueue(99999, SIGUSR1, val);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(ESRCH, errno);
}
#endif /* !__phoenix__ */


TEST(signal_pending, killpg_delivers_to_group)
{
	sigset_t block, unblock;
	int ret;
	pid_t pgrp;

	test_handlerCalled = 0;
	test_handlerSig = 0;
	signal(SIGUSR1, test_sigHandler);

	/* Block SIGUSR1 */
	ret = sigemptyset(&block);
	TEST_ASSERT_EQUAL_INT(0, ret);
	ret = sigaddset(&block, SIGUSR1);
	TEST_ASSERT_EQUAL_INT(0, ret);
	ret = sigprocmask(SIG_BLOCK, &block, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	pgrp = getpgrp();
	/* killpg requires pgrp > 1 */
	if (pgrp <= 1) {
		TEST_IGNORE_MESSAGE("process group ID <= 1, cannot test killpg");
	}

	errno = 0;
	ret = killpg(pgrp, SIGUSR1);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, errno);

	/* Unblock SIGUSR1 to let the handler run */
	ret = sigemptyset(&unblock);
	TEST_ASSERT_EQUAL_INT(0, ret);
	ret = sigaddset(&unblock, SIGUSR1);
	TEST_ASSERT_EQUAL_INT(0, ret);
	ret = sigprocmask(SIG_UNBLOCK, &unblock, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	TEST_ASSERT_EQUAL_INT(1, test_handlerCalled);
	TEST_ASSERT_EQUAL_INT(SIGUSR1, test_handlerSig);
}


TEST(signal_pending, killpg_einval_invalid_signal)
{
	int ret;
	pid_t pgrp;

	pgrp = getpgrp();
	if (pgrp <= 1) {
		TEST_IGNORE_MESSAGE("process group ID <= 1, cannot test killpg");
	}

	errno = 0;
	ret = killpg(pgrp, -1);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST(signal_pending, killpg_esrch_invalid_pgrp)
{
	int ret;

	errno = 0;
	ret = killpg(99999, SIGUSR1);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(ESRCH, errno);
}


#ifdef SIGSTKSZ
TEST(signal_pending, sigaltstack_set_and_get)
{
	stack_t ss, oss;
	static char altStack[SIGSTKSZ];
	int ret;

	ss.ss_sp = altStack;
	ss.ss_size = sizeof(altStack);
	ss.ss_flags = 0;

	errno = 0;
	ret = sigaltstack(&ss, &oss);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, errno);

	/* Verify we can query it back */
	stack_t query;
	ret = sigaltstack(NULL, &query);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_TRUE(query.ss_sp == altStack);
	TEST_ASSERT_EQUAL_INT((int)sizeof(altStack), (int)query.ss_size);
	TEST_ASSERT_EQUAL_INT(0, query.ss_flags & SS_DISABLE);

	/* Disable it */
	ss.ss_flags = SS_DISABLE;
	ret = sigaltstack(&ss, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


TEST(signal_pending, sigaltstack_disable)
{
	stack_t ss, query;
	static char altStack[SIGSTKSZ];
	int ret;

	/* First enable */
	ss.ss_sp = altStack;
	ss.ss_size = sizeof(altStack);
	ss.ss_flags = 0;
	ret = sigaltstack(&ss, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Now disable */
	ss.ss_flags = SS_DISABLE;
	ret = sigaltstack(&ss, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = sigaltstack(NULL, &query);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_TRUE((query.ss_flags & SS_DISABLE) != 0);
}


TEST(signal_pending, sigaltstack_einval_bad_flags)
{
	stack_t ss;
	static char altStack[SIGSTKSZ];
	int ret;

	ss.ss_sp = altStack;
	ss.ss_size = sizeof(altStack);
	/* Invalid flags (not SS_DISABLE) */
	ss.ss_flags = SS_DISABLE | SS_ONSTACK;

	errno = 0;
	ret = sigaltstack(&ss, NULL);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST(signal_pending, sigaltstack_enomem_too_small)
{
	stack_t ss;
	static char tinyStack[1];
	int ret;

	ss.ss_sp = tinyStack;
	ss.ss_size = sizeof(tinyStack);
	ss.ss_flags = 0;

	errno = 0;
	ret = sigaltstack(&ss, NULL);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(ENOMEM, errno);
}


TEST(signal_pending, sigaltstack_get_only)
{
	stack_t oss;
	int ret;

	/* Passing NULL for ss should just query */
	errno = 0;
	ret = sigaltstack(NULL, &oss);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, errno);
}
#endif /* SIGSTKSZ */


TEST_GROUP_RUNNER(signal_pending)
{
	RUN_TEST_CASE(signal_pending, sigpending_empty_when_none_blocked);
	RUN_TEST_CASE(signal_pending, sigpending_shows_blocked_pending);
	RUN_TEST_CASE(signal_pending, sigpending_shows_process_directed_pending);
	RUN_TEST_CASE(signal_pending, sigpending_sig_ign_discards_pending);
	RUN_TEST_CASE(signal_pending, sigpending_sig_dfl_discards_default_ignored);
	RUN_TEST_CASE(signal_pending, sigpending_excludes_other_thread_pending);
	RUN_TEST_CASE(signal_pending, sigwait_consumes_pending);
	RUN_TEST_CASE(signal_pending, sigwaitinfo_returns_signal_number);
	RUN_TEST_CASE(signal_pending, sigwait_blocks_until_signal);
	RUN_TEST_CASE(signal_pending, sigwait_not_interrupted_by_handler);
	RUN_TEST_CASE(signal_pending, sigwaitinfo_eintr_on_handler);
	RUN_TEST_CASE(signal_pending, sigwait_process_directed_reaches_matching_waiter);
	RUN_TEST_CASE(signal_pending, sigtimedwait_immediate_pending);
	RUN_TEST_CASE(signal_pending, sigtimedwait_eagain_timeout);
	RUN_TEST_CASE(signal_pending, sigtimedwait_einval_bad_nsec);
	RUN_TEST_CASE(signal_pending, sigtimedwait_einval_negative_sec);
	RUN_TEST_CASE(signal_pending, sigtimedwait_eagain_after_timeout);
	RUN_TEST_CASE(signal_pending, sigtimedwait_receives_during_wait);
#ifndef __phoenix__
	RUN_TEST_CASE(signal_pending, sigqueue_delivers_signal);
	RUN_TEST_CASE(signal_pending, sigqueue_einval_invalid_signal);
	RUN_TEST_CASE(signal_pending, sigqueue_esrch_invalid_pid);
#endif
	RUN_TEST_CASE(signal_pending, killpg_delivers_to_group);
	RUN_TEST_CASE(signal_pending, killpg_einval_invalid_signal);
	RUN_TEST_CASE(signal_pending, killpg_esrch_invalid_pgrp);
#ifdef SIGSTKSZ
	RUN_TEST_CASE(signal_pending, sigaltstack_set_and_get);
	RUN_TEST_CASE(signal_pending, sigaltstack_disable);
	RUN_TEST_CASE(signal_pending, sigaltstack_einval_bad_flags);
	RUN_TEST_CASE(signal_pending, sigaltstack_enomem_too_small);
	RUN_TEST_CASE(signal_pending, sigaltstack_get_only);
#endif
}
