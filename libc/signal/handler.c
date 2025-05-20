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
#include <unistd.h>
#include <errno.h>

#include <unity_fixture.h>


TEST_GROUP(handler);


TEST_SETUP(handler)
{
}


TEST_TEAR_DOWN(handler)
{
	/* unblock all signals */
	sigset_t set;
	sigemptyset(&set);
	sigprocmask(SIG_SETMASK, &set, NULL);

	/* set default signal disposition for all signals */
	for (int signo = 1; signo < USERSPACE_NSIG; ++signo) {
		signal(signo, SIG_DFL);
	}
}


static volatile sig_atomic_t handler_haveSignal;
static volatile sigset_t handler_sigset;


void sighandler(int sig)
{
	sigset_t set;
	sigprocmask(SIG_SETMASK, NULL, &set);

	/* WARN: the write might not be atomic */
	handler_sigset = set;

	handler_haveSignal |= (1u << sig);
}


/* check if signal mask is set correctly inside and after the sighandler */
TEST(handler, sighandler_sa_mask)
{
	sigset_t set;
	struct sigaction sa = { 0 };

	TEST_ASSERT_EQUAL_INT(0, sigemptyset(&set));
	TEST_ASSERT_EQUAL_INT(0, sigaddset(&set, SIGUSR1));
	TEST_ASSERT_EQUAL_INT(0, sigprocmask(SIG_SETMASK, &set, NULL));

	sa.sa_handler = sighandler;
	sa.sa_flags = 0;

	TEST_ASSERT_EQUAL_INT(0, sigemptyset(&sa.sa_mask));
	TEST_ASSERT_EQUAL_INT(0, sigaddset(&sa.sa_mask, SIGUSR2));

	handler_haveSignal = 0u;
	TEST_ASSERT_EQUAL_INT(0, sigaction(SIGPIPE, &sa, NULL));
	TEST_ASSERT_EQUAL_INT(0, kill(getpid(), SIGPIPE));

	/* signal handler should be called on syscall exit from kill */

	TEST_ASSERT_EQUAL_HEX32((1u << SIGPIPE), handler_haveSignal);

	/* check the signal mask inside sighandler */
	sigset_t local_sigset = handler_sigset;
	TEST_ASSERT_EQUAL_INT_MESSAGE(1, sigismember(&local_sigset, SIGPIPE), "no caught signal in mask");
	TEST_ASSERT_EQUAL_INT_MESSAGE(1, sigismember(&local_sigset, SIGUSR1), "no current process mask signal in mask");
	TEST_ASSERT_EQUAL_INT_MESSAGE(1, sigismember(&local_sigset, SIGUSR2), "no sa_mask signal in mask");


	TEST_ASSERT_EQUAL_INT(0, sigprocmask(SIG_SETMASK, NULL, &set));

	/* after the signal handler - the original mask should be restored - only SIGUSR1 blocked */
	TEST_ASSERT_EQUAL_INT(0, sigismember(&set, SIGPIPE));
	TEST_ASSERT_EQUAL_INT(1, sigismember(&set, SIGUSR1));
	TEST_ASSERT_EQUAL_INT(0, sigismember(&set, SIGUSR2));
}


/* check mask consistiency on signal handler - whether signal handler will be called if the signal is only unblocked in sa_mask of the other signal
 * should not be called as sa_mask is ORed with current thread signal mask */
TEST(handler, sighandler_signal_in_signal)
{
	sigset_t set, oldset;
	struct sigaction sa = { 0 };

	/* SIGUSR2 - block in normal execution, unblock in sa_mask */
	TEST_ASSERT_EQUAL_INT(0, sigemptyset(&set));
	TEST_ASSERT_EQUAL_INT(0, sigaddset(&set, SIGUSR2));
	TEST_ASSERT_EQUAL_INT(0, sigprocmask(SIG_SETMASK, &set, NULL));

	sa.sa_handler = sighandler;
	sa.sa_flags = 0;

	TEST_ASSERT_EQUAL_INT(0, sigfillset(&sa.sa_mask));
	TEST_ASSERT_EQUAL_INT(0, sigdelset(&sa.sa_mask, SIGUSR2));

	handler_haveSignal = 0u;
	TEST_ASSERT_EQUAL_INT(0, sigaction(SIGPIPE, &sa, NULL));
	TEST_ASSERT_EQUAL_INT(0, sigaction(SIGUSR2, &sa, NULL));

	/* send SIGUSR2, verify it's pending */
	TEST_ASSERT_EQUAL_INT(0, kill(getpid(), SIGUSR2));
	TEST_ASSERT_EQUAL_HEX32(0, handler_haveSignal);

	TEST_ASSERT_EQUAL_INT(0, kill(getpid(), SIGPIPE));

	/* signal handler should be called on syscall exit from kill - only SIGPIPE should be delivered */
	TEST_ASSERT_EQUAL_INT((1u << SIGPIPE), handler_haveSignal);


	/* unblock SIGUSR2 */
	handler_haveSignal = 0u;
	TEST_ASSERT_EQUAL_INT(0, sigdelset(&set, SIGUSR2));
	TEST_ASSERT_EQUAL_INT(0, sigprocmask(SIG_SETMASK, &set, &oldset));

	/* SIGUSR2 should be delivered now */
	TEST_ASSERT_EQUAL_HEX32((1u << SIGUSR2), handler_haveSignal);

	/* after the signal handler - the original mask should be restored - only SIGUSR2 blocked */
	TEST_ASSERT_EQUAL_INT(0, sigismember(&oldset, SIGPIPE));
	TEST_ASSERT_EQUAL_INT(0, sigismember(&oldset, SIGUSR1));
	TEST_ASSERT_EQUAL_INT(1, sigismember(&oldset, SIGUSR2));
}

/* TODO: test sa_flags - especially SA_NODEFER */


TEST_GROUP_RUNNER(handler)
{
	RUN_TEST_CASE(handler, sighandler_sa_mask);
	RUN_TEST_CASE(handler, sighandler_signal_in_signal);
}


TEST_GROUP(sigsuspend);


TEST_SETUP(sigsuspend)
{
	struct sigaction sa = {
		.sa_handler = sighandler,
	};

	sigemptyset(&sa.sa_mask);
	TEST_ASSERT_EQUAL_INT(0, sigaddset(&sa.sa_mask, SIGPIPE));

	TEST_ASSERT_EQUAL_INT(0, sigaction(SIGALRM, &sa, NULL));
	TEST_ASSERT_EQUAL_INT(0, sigaction(SIGUSR1, &sa, NULL));
	TEST_ASSERT_EQUAL_INT(0, sigaction(SIGUSR2, &sa, NULL));
}


TEST_TEAR_DOWN(sigsuspend)
{
	/* unblock all signals */
	sigset_t set;
	sigemptyset(&set);
	sigprocmask(SIG_SETMASK, &set, NULL);

	/* set default signal disposition for all signals used is sigsuspend tests (@see TEST_SETUP(sigsuspend)) */
	signal(SIGALRM, SIG_DFL);
	signal(SIGUSR1, SIG_DFL);
	signal(SIGUSR2, SIG_DFL);
}


/* test sigsuspend critical section implementation pattern - send signal before sigsuspend */
TEST(sigsuspend, signal_before_handler)
{
	sigset_t all_blocked, all_unblocked, test_set;
	TEST_ASSERT_EQUAL_INT(0, sigfillset(&all_blocked));
	TEST_ASSERT_EQUAL_INT(0, sigemptyset(&all_unblocked));


	/* enter critical section */
	TEST_ASSERT_EQUAL_INT(0, sigprocmask(SIG_SETMASK, &all_blocked, NULL));

	handler_haveSignal = 0u;
	TEST_ASSERT_EQUAL_INT(0, kill(getpid(), SIGUSR1));
	TEST_ASSERT_EQUAL_HEX32(0, handler_haveSignal); /* signals are blocked, signal handler should not be called */

	errno = 0;
	sigsuspend(&all_unblocked);
	TEST_ASSERT_EQUAL_INT(EINTR, errno);

	/* SIGUSR1 should be handled now */
	TEST_ASSERT_EQUAL_HEX32((1u << SIGUSR1), handler_haveSignal);

	/* exit critical section */
	TEST_ASSERT_EQUAL_INT(0, sigprocmask(SIG_SETMASK, &all_unblocked, &test_set));

	/* check if sigsuspend restored all_blocked sigmask */
	for (int signo = 1; signo < USERSPACE_NSIG; ++signo) {
		if (signal_is_unblockable(signo)) {
			continue;
		}

		TEST_ASSERT_EQUAL_INT(1, sigismember(&test_set, signo));
	}

	/* check sigmask in sighandler */
	sigset_t local_sigset = handler_sigset;
	for (int signo = 1; signo < USERSPACE_NSIG; ++signo) {
		/* should be all_unblocked | sa_mask | [current_signal] */
		TEST_ASSERT_EQUAL_INT(((signo == SIGUSR1) || (signo == SIGPIPE)), sigismember(&local_sigset, signo));
	}
}

/* test sigsuspend critical section implementation pattern - send signal before sigsuspend */
TEST(sigsuspend, signal_before_two_signals)
{
	sigset_t all_blocked, all_unblocked, test_set;
	TEST_ASSERT_EQUAL_INT(0, sigfillset(&all_blocked));
	TEST_ASSERT_EQUAL_INT(0, sigemptyset(&all_unblocked));


	/* enter critical section */
	TEST_ASSERT_EQUAL_INT(0, sigprocmask(SIG_SETMASK, &all_blocked, NULL));

	handler_haveSignal = 0u;
	TEST_ASSERT_EQUAL_INT(0, kill(getpid(), SIGUSR1));
	TEST_ASSERT_EQUAL_INT(0, kill(getpid(), SIGUSR2));
	TEST_ASSERT_EQUAL_HEX32(0, handler_haveSignal); /* signals are blocked, signal handler should not be called */

	errno = 0;
	sigsuspend(&all_unblocked);
	TEST_ASSERT_EQUAL_INT(EINTR, errno);

	/* SIGUSR1 and SIGUSR2 should be handled now */
	TEST_ASSERT_EQUAL_HEX32((1u << SIGUSR1) | (1u << SIGUSR2), handler_haveSignal);

	/* exit critical section */
	TEST_ASSERT_EQUAL_INT(0, sigprocmask(SIG_SETMASK, &all_unblocked, &test_set));

	/* check if sigsuspend restored all_blocked sigmask */
	for (int signo = 1; signo < USERSPACE_NSIG; ++signo) {
		if (signal_is_unblockable(signo)) {
			continue;
		}
		TEST_ASSERT_EQUAL_INT(1, sigismember(&test_set, signo));
	}
}

/* test sigsuspend critical section implementation pattern - send signal after sigsuspend */
TEST(sigsuspend, signal_after)
{
	sigset_t all_blocked, all_unblocked, test_set;
	TEST_ASSERT_EQUAL_INT(0, sigfillset(&all_blocked));
	TEST_ASSERT_EQUAL_INT(0, sigemptyset(&all_unblocked));


	/* enter critical section */
	TEST_ASSERT_EQUAL_INT(0, sigprocmask(SIG_SETMASK, &all_blocked, NULL));

	handler_haveSignal = 0u;
	alarm(1);
	TEST_ASSERT_EQUAL_HEX32(0, handler_haveSignal); /* signals are blocked, signal handler should not be called */

	errno = 0;
	sigsuspend(&all_unblocked);
	TEST_ASSERT_EQUAL_INT(EINTR, errno);

	/* SIGALRM should be handled now */
	TEST_ASSERT_EQUAL_HEX32((1u << SIGALRM), handler_haveSignal);

	/* exit critical section */
	TEST_ASSERT_EQUAL_INT(0, sigprocmask(SIG_SETMASK, &all_unblocked, &test_set));

	/* check if sigsuspend restored all_blocked sigmask */
	for (int signo = 1; signo < USERSPACE_NSIG; ++signo) {
		if (signal_is_unblockable(signo)) {
			continue;
		}
		TEST_ASSERT_EQUAL_INT(1, sigismember(&test_set, signo));
	}
}


TEST_GROUP_RUNNER(sigsuspend)
{
	RUN_TEST_CASE(sigsuspend, signal_after);
	RUN_TEST_CASE(sigsuspend, signal_before_handler);
	RUN_TEST_CASE(sigsuspend, signal_before_two_signals);
}
