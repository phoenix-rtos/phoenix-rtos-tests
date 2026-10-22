/*
 * Phoenix-RTOS
 *
 * libc-tests
 *
 * Testing POSIX signals.
 *
 * Copyright 2025 Phoenix Systems
 * Author: Marek Bialowas
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "sig_internal.h"

#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#include <unity_fixture.h>


TEST_GROUP(mask);


TEST_SETUP(mask)
{
}


TEST_TEAR_DOWN(mask)
{
	/* unblock all signals */
	sigset_t set;
	sigemptyset(&set);
	sigprocmask(SIG_SETMASK, &set, NULL);
}


TEST(mask, sigset_full)
{
	sigset_t fullset, emptyset;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnonnull"
	TEST_ASSERT_EQUAL_INT(-1, sigfillset(NULL));
	TEST_ASSERT_EQUAL_INT(-1, sigemptyset(NULL));
#pragma GCC diagnostic pop

	TEST_ASSERT_EQUAL_INT(0, sigfillset(&fullset));
	TEST_ASSERT_EQUAL_INT(0, sigemptyset(&emptyset));

	for (int signo = 1; signo < USERSPACE_NSIG; ++signo) {
		TEST_ASSERT_EQUAL_INT(1, sigismember(&fullset, signo));
		TEST_ASSERT_EQUAL_INT(0, sigismember(&emptyset, signo));
	}
}


TEST(mask, sigset_single)
{
	sigset_t set;

	TEST_ASSERT_EQUAL_INT(0, sigemptyset(&set));

	for (int signo = 1; signo < USERSPACE_NSIG; ++signo) {
		TEST_ASSERT_EQUAL_INT(0, sigaddset(&set, signo));
		for (int checksig = 1; checksig < USERSPACE_NSIG; ++checksig) {
			TEST_ASSERT_EQUAL_INT(signo == checksig, sigismember(&set, checksig));
		}

		TEST_ASSERT_EQUAL_INT(0, sigdelset(&set, signo));
		TEST_ASSERT_EQUAL_INT(0, sigismember(&set, signo));
	}


	TEST_ASSERT_EQUAL_INT(-1, sigaddset(&set, NSIG + 1));
}


TEST(mask, procmask_set)
{
	sigset_t set, oldset, testset;


	/* + SIGUSR1 + SIGUSR2 (test how: SETMASK) */
	TEST_ASSERT_EQUAL_INT(0, sigemptyset(&set));
	TEST_ASSERT_EQUAL_INT(0, sigaddset(&set, SIGUSR1));
	TEST_ASSERT_EQUAL_INT(0, sigaddset(&set, SIGUSR2));

	TEST_ASSERT_EQUAL_INT(0, sigprocmask(SIG_SETMASK, &set, NULL));
	TEST_ASSERT_EQUAL_INT(0, sigprocmask(SIG_SETMASK, NULL, &testset));

	TEST_ASSERT_EQUAL_INT(1, sigismember(&testset, SIGUSR1));
	TEST_ASSERT_EQUAL_INT(1, sigismember(&testset, SIGUSR2));


	/* - SIGUSR1 (test how: UNBLOCK) */
	TEST_ASSERT_EQUAL_INT(0, sigemptyset(&set));
	TEST_ASSERT_EQUAL_INT(0, sigaddset(&set, SIGUSR1));

	TEST_ASSERT_EQUAL_INT(0, sigprocmask(SIG_UNBLOCK, &set, &oldset));
	TEST_ASSERT_EQUAL_INT(0, sigprocmask(SIG_BLOCK, NULL, &testset));

	/* old set should have both signals set */
	TEST_ASSERT_EQUAL_INT(1, sigismember(&oldset, SIGUSR1));
	TEST_ASSERT_EQUAL_INT(1, sigismember(&oldset, SIGUSR2));

	/* current set should have only SIGUSR2 blocked (we've unblocked SIGUSR1) */
	TEST_ASSERT_EQUAL_INT(0, sigismember(&testset, SIGUSR1));
	TEST_ASSERT_EQUAL_INT(1, sigismember(&testset, SIGUSR2));


	/* + SIGPIPE (test how: BLOCK) */
	TEST_ASSERT_EQUAL_INT(0, sigemptyset(&set));
	TEST_ASSERT_EQUAL_INT(0, sigaddset(&set, SIGPIPE));

	TEST_ASSERT_EQUAL_INT(0, sigprocmask(SIG_BLOCK, &set, &oldset));
	TEST_ASSERT_EQUAL_INT(0, sigprocmask(SIG_UNBLOCK, NULL, &testset));

	/* old set should have only SIGUSR2 signal set */
	TEST_ASSERT_EQUAL_INT(0, sigismember(&oldset, SIGPIPE));
	TEST_ASSERT_EQUAL_INT(0, sigismember(&oldset, SIGUSR1));
	TEST_ASSERT_EQUAL_INT(1, sigismember(&oldset, SIGUSR2));

	/* current set should have SIGPIPE + SIGUSR2 blocked */
	TEST_ASSERT_EQUAL_INT(1, sigismember(&testset, SIGPIPE));
	TEST_ASSERT_EQUAL_INT(0, sigismember(&testset, SIGUSR1));
	TEST_ASSERT_EQUAL_INT(1, sigismember(&testset, SIGUSR2));
}


#define INHERIT_TEST_SIGNO SIGUSR1


static volatile sig_atomic_t handler_called;


static void handler(int sig)
{
	(void)sig;
	handler_called = 1;
}


static void *check_mask_and_raise(void *arg)
{
	sigset_t *expected = (sigset_t *)arg;
	sigset_t actual;

	if (sigprocmask(SIG_BLOCK, NULL, &actual)) {
		return (void *)(intptr_t)-1;
	}
	for (int signo = 1; signo < USERSPACE_NSIG; ++signo) {
		if (sigismember(expected, signo) != sigismember(&actual, signo)) {
			return (void *)(intptr_t)-2;
		}
	}

	handler_called = 0;
	if (raise(INHERIT_TEST_SIGNO)) {
		return (void *)(intptr_t)-3;
	}
	if (handler_called != 0) {
		return (void *)(intptr_t)-4;
	}
	return NULL;
}


TEST_GROUP(mask_inheritance);


TEST_SETUP(mask_inheritance)
{
	sigset_t set;
	struct sigaction sa;

	TEST_ASSERT_EQUAL_INT(0, sigemptyset(&set));
	TEST_ASSERT_EQUAL_INT(0, sigaddset(&set, INHERIT_TEST_SIGNO));
	TEST_ASSERT_EQUAL_INT(0, sigprocmask(SIG_SETMASK, &set, NULL));

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handler;
	TEST_ASSERT_EQUAL_INT(0, sigemptyset(&sa.sa_mask));
	TEST_ASSERT_EQUAL_INT(0, sigaction(INHERIT_TEST_SIGNO, &sa, NULL));
}


TEST_TEAR_DOWN(mask_inheritance)
{
	struct sigaction sa;
	sigset_t set;

	/* unblock all signals */
	sigemptyset(&set);
	sigprocmask(SIG_SETMASK, &set, NULL);

	/* remove signal handler */
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SIG_DFL;
	sigaction(INHERIT_TEST_SIGNO, &sa, NULL);
}


TEST(mask_inheritance, sigmask_inherit_vfork)
{
	sigset_t cur;
	pid_t pid;

	TEST_ASSERT_EQUAL_INT(0, sigprocmask(SIG_BLOCK, NULL, &cur));
	TEST_ASSERT_EQUAL_INT(1, sigismember(&cur, INHERIT_TEST_SIGNO));

	pid = vfork();
	TEST_ASSERT_TRUE(pid >= 0);
	if (pid == 0) {
		_exit((intptr_t)check_mask_and_raise(&cur));
	}

	int status;
	TEST_ASSERT_EQUAL(pid, waitpid(pid, &status, 0));
	TEST_ASSERT_TRUE(WIFEXITED(status));
	TEST_ASSERT_EQUAL(0, WEXITSTATUS(status));
}


TEST(mask_inheritance, sigmask_inherit_fork)
{
	sigset_t cur;
	pid_t pid;

	TEST_ASSERT_EQUAL_INT(0, sigprocmask(SIG_BLOCK, NULL, &cur));
	TEST_ASSERT_EQUAL_INT(1, sigismember(&cur, INHERIT_TEST_SIGNO));

	if ((pid = fork()) < 0) {
		if (errno == ENOSYS) {
			TEST_IGNORE_MESSAGE("fork syscall not supported");
		}
		else {
			FAIL("fork");
		}
	}
	if (pid == 0) {
		_exit((intptr_t)check_mask_and_raise(&cur));
	}

	int status;
	TEST_ASSERT_EQUAL(pid, waitpid(pid, &status, 0));
	TEST_ASSERT_TRUE(WIFEXITED(status));
	TEST_ASSERT_EQUAL(0, WEXITSTATUS(status));
}


TEST(mask_inheritance, sigmask_inherit_pthread)
{
	pthread_t tid;
	sigset_t cur;
	void *ret;

	TEST_ASSERT_EQUAL_INT(0, sigprocmask(SIG_BLOCK, NULL, &cur));
	TEST_ASSERT_EQUAL_INT(1, sigismember(&cur, INHERIT_TEST_SIGNO));

	TEST_ASSERT_EQUAL_INT(0, pthread_create(&tid, NULL, check_mask_and_raise, &cur));
	TEST_ASSERT_EQUAL_INT(0, pthread_join(tid, &ret));
	TEST_ASSERT_EQUAL_PTR(0, (intptr_t)ret);
}


TEST_GROUP_RUNNER(mask)
{
	RUN_TEST_CASE(mask, sigset_full);
	RUN_TEST_CASE(mask, sigset_single);
	RUN_TEST_CASE(mask, procmask_set);
}


TEST_GROUP_RUNNER(mask_inheritance)
{
	RUN_TEST_CASE(mask_inheritance, sigmask_inherit_vfork);
	RUN_TEST_CASE(mask_inheritance, sigmask_inherit_fork);
	RUN_TEST_CASE(mask_inheritance, sigmask_inherit_pthread);
}
