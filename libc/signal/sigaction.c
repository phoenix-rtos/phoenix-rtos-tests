/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <signal.h>
 * TESTED:
 *    - sigaction()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "sig_internal.h"

#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>

#include <unity_fixture.h>


TEST_GROUP(signal_sigaction);


static volatile sig_atomic_t test_common_handlerCalled;
static volatile sig_atomic_t test_common_handlerSigNo;
static volatile sig_atomic_t test_common_saResetCalled;
static volatile sigset_t test_common_handlerMask;
static volatile siginfo_t test_common_siginfo;
static volatile sig_atomic_t test_common_siginfoCalled;


static void test_handler(int signo)
{
	test_common_handlerCalled = 1;
	test_common_handlerSigNo = signo;
}


static void test_handlerMask(int signo)
{
	sigset_t set;
	sigprocmask(SIG_SETMASK, NULL, &set);
	test_common_handlerMask = set;
	test_common_handlerCalled = 1;
	test_common_handlerSigNo = signo;
}


static void test_handlerReset(int signo)
{
	(void)signo;
	test_common_saResetCalled++;
}


static void test_sigactionHandler(int signo, siginfo_t *info, void *context)
{
	(void)context;
	test_common_siginfoCalled = 1;
	test_common_handlerSigNo = signo;
	test_common_siginfo = *info;
}


TEST_SETUP(signal_sigaction)
{
}


TEST_TEAR_DOWN(signal_sigaction)
{
	sigset_t set;
	sigemptyset(&set);
	sigprocmask(SIG_SETMASK, &set, NULL);

	alarm(0);

	for (int signo = 1; signo < USERSPACE_NSIG; ++signo) {
		if (signal_is_unblockable(signo)) {
			continue;
		}
		signal(signo, SIG_DFL);
	}
}


/* sigaction() with non-NULL act installs new handler */
TEST(signal_sigaction, install_handler)
{
	struct sigaction sa;
	int ret;

	test_common_handlerCalled = 0;
	test_common_handlerSigNo = 0;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = test_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	ret = sigaction(SIGUSR1, &sa, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = raise(SIGUSR1);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(1, test_common_handlerCalled);
	TEST_ASSERT_EQUAL_INT(SIGUSR1, test_common_handlerSigNo);
}


/* sigaction() with act=NULL queries current action without changing it */
TEST(signal_sigaction, query_current_action)
{
	struct sigaction sa, oact;
	int ret;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = test_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	ret = sigaction(SIGUSR1, &sa, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* query without changing */
	memset(&oact, 0, sizeof(oact));
	ret = sigaction(SIGUSR1, NULL, &oact);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_TRUE(oact.sa_handler == test_handler);

	/* verify handler still works */
	test_common_handlerCalled = 0;
	ret = raise(SIGUSR1);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(1, test_common_handlerCalled);
}


/* sigaction() with non-NULL oact stores previous action */
TEST(signal_sigaction, retrieve_previous_action)
{
	struct sigaction sa1, sa2, oact;
	int ret;

	memset(&sa1, 0, sizeof(sa1));
	sa1.sa_handler = test_handler;
	sigemptyset(&sa1.sa_mask);
	sa1.sa_flags = 0;

	ret = sigaction(SIGUSR1, &sa1, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	memset(&sa2, 0, sizeof(sa2));
	sa2.sa_handler = SIG_IGN;
	sigemptyset(&sa2.sa_mask);
	sa2.sa_flags = 0;

	memset(&oact, 0, sizeof(oact));
	ret = sigaction(SIGUSR1, &sa2, &oact);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_TRUE(oact.sa_handler == test_handler);
}


/* sigaction() returns EINVAL for invalid signal number 0 */
TEST(signal_sigaction, einval_zero_signal)
{
	struct sigaction sa;
	int ret;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = test_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	errno = 0;
	ret = sigaction(0, &sa, NULL);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


/* sigaction() returns EINVAL for signal number exceeding valid range */
TEST(signal_sigaction, einval_large_signal)
{
	struct sigaction sa;
	int ret;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = test_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	errno = 0;
	ret = sigaction(NSIG + 1, &sa, NULL);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


/* sigaction() returns EINVAL when trying to catch SIGKILL */
TEST(signal_sigaction, einval_catch_sigkill)
{
	struct sigaction sa;
	int ret;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = test_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	errno = 0;
	ret = sigaction(SIGKILL, &sa, NULL);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


/* sigaction() returns EINVAL when trying to catch SIGSTOP */
TEST(signal_sigaction, einval_catch_sigstop)
{
	struct sigaction sa;
	int ret;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = test_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	errno = 0;
	ret = sigaction(SIGSTOP, &sa, NULL);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


/* sigaction() returns EINVAL when trying to ignore SIGKILL */
TEST(signal_sigaction, einval_ignore_sigkill)
{
	struct sigaction sa;
	int ret;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	errno = 0;
	ret = sigaction(SIGKILL, &sa, NULL);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


/* sigaction() returns EINVAL when trying to ignore SIGSTOP */
TEST(signal_sigaction, einval_ignore_sigstop)
{
	struct sigaction sa;
	int ret;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	errno = 0;
	ret = sigaction(SIGSTOP, &sa, NULL);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


/* sigaction() with SIG_IGN causes signal to be ignored */
TEST(signal_sigaction, sig_ign)
{
	struct sigaction sa;
	int ret;

	test_common_handlerCalled = 0;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	ret = sigaction(SIGUSR1, &sa, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = raise(SIGUSR1);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, test_common_handlerCalled);
}


/* sigaction() with SIG_DFL restores default disposition */
TEST(signal_sigaction, sig_dfl_restore)
{
	struct sigaction sa, oact;
	int ret;

	/* first set to ignore */
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	ret = sigaction(SIGUSR1, &sa, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* then set to default */
	sa.sa_handler = SIG_DFL;
	ret = sigaction(SIGUSR1, &sa, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* verify the previous action was SIG_IGN */
	memset(&oact, 0, sizeof(oact));
	ret = sigaction(SIGUSR1, NULL, &oact);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_TRUE(oact.sa_handler == SIG_DFL);
}


/* sa_mask signals are blocked during signal handler execution */
TEST(signal_sigaction, sa_mask_blocked_in_handler)
{
	struct sigaction sa;
	sigset_t local;
	int ret;

	test_common_handlerCalled = 0;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = test_handlerMask;
	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, SIGUSR2);
	sigaddset(&sa.sa_mask, SIGALRM);
	sa.sa_flags = 0;

	ret = sigaction(SIGUSR1, &sa, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = raise(SIGUSR1);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(1, test_common_handlerCalled);

	/* check that sa_mask signals were blocked inside the handler */
	local = test_common_handlerMask;
	TEST_ASSERT_EQUAL_INT(1, sigismember(&local, SIGUSR2));
	TEST_ASSERT_EQUAL_INT(1, sigismember(&local, SIGALRM));
}


/* caught signal is automatically added to mask during handler (default behavior, no SA_NODEFER) */
TEST(signal_sigaction, caught_signal_blocked_in_handler)
{
	struct sigaction sa;
	sigset_t local;
	int ret;

	test_common_handlerCalled = 0;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = test_handlerMask;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	ret = sigaction(SIGUSR1, &sa, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = raise(SIGUSR1);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(1, test_common_handlerCalled);

	/* the caught signal itself should be blocked in the handler */
	local = test_common_handlerMask;
	TEST_ASSERT_EQUAL_INT(1, sigismember(&local, SIGUSR1));
}


/* original signal mask is restored after handler returns */
TEST(signal_sigaction, mask_restored_after_handler)
{
	struct sigaction sa;
	sigset_t origMask, currentMask;
	int ret;

	/* set a known mask */
	sigemptyset(&origMask);
	sigaddset(&origMask, SIGPIPE);
	ret = sigprocmask(SIG_SETMASK, &origMask, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = test_handler;
	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, SIGUSR2);
	sa.sa_flags = 0;

	test_common_handlerCalled = 0;
	ret = sigaction(SIGUSR1, &sa, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = raise(SIGUSR1);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(1, test_common_handlerCalled);

	/* mask should be restored to original after handler */
	ret = sigprocmask(SIG_SETMASK, NULL, &currentMask);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(1, sigismember(&currentMask, SIGPIPE));
	TEST_ASSERT_EQUAL_INT(0, sigismember(&currentMask, SIGUSR1));
	TEST_ASSERT_EQUAL_INT(0, sigismember(&currentMask, SIGUSR2));
}


/* SA_NODEFER: caught signal is NOT added to thread's signal mask on entry to handler */
TEST(signal_sigaction, sa_nodefer)
{
	struct sigaction sa;
	sigset_t local;
	int ret;

	test_common_handlerCalled = 0;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = test_handlerMask;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_NODEFER;

	ret = sigaction(SIGUSR1, &sa, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = raise(SIGUSR1);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(1, test_common_handlerCalled);

	/* with SA_NODEFER, the caught signal should NOT be in the mask */
	local = test_common_handlerMask;
	TEST_ASSERT_EQUAL_INT(0, sigismember(&local, SIGUSR1));
}


/* SA_NODEFER: sa_mask signals are still blocked even with SA_NODEFER */
TEST(signal_sigaction, sa_nodefer_sa_mask_still_applied)
{
	struct sigaction sa;
	sigset_t local;
	int ret;

	test_common_handlerCalled = 0;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = test_handlerMask;
	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, SIGUSR2);
	sa.sa_flags = SA_NODEFER;

	ret = sigaction(SIGUSR1, &sa, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = raise(SIGUSR1);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(1, test_common_handlerCalled);

	/* sa_mask signal should still be blocked */
	local = test_common_handlerMask;
	TEST_ASSERT_EQUAL_INT(1, sigismember(&local, SIGUSR2));
	/* but caught signal should NOT be blocked */
	TEST_ASSERT_EQUAL_INT(0, sigismember(&local, SIGUSR1));
}


/* SA_RESETHAND: disposition is reset to SIG_DFL on entry to handler */
TEST(signal_sigaction, sa_resethand)
{
	struct sigaction sa, oact;
	int ret;

	test_common_saResetCalled = 0;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = test_handlerReset;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESETHAND | SA_NODEFER;

	ret = sigaction(SIGUSR1, &sa, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = raise(SIGUSR1);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(1, test_common_saResetCalled);

	/* after handler, disposition should be SIG_DFL */
	memset(&oact, 0, sizeof(oact));
	ret = sigaction(SIGUSR1, NULL, &oact);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_TRUE(oact.sa_handler == SIG_DFL);
}


/* SA_SIGINFO: handler is called with three arguments */
TEST(signal_sigaction, sa_siginfo_handler)
{
	struct sigaction sa;
	int ret;

	test_common_siginfoCalled = 0;
	test_common_handlerSigNo = 0;

	memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = test_sigactionHandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_SIGINFO;

	ret = sigaction(SIGUSR1, &sa, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = raise(SIGUSR1);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(1, test_common_siginfoCalled);
	TEST_ASSERT_EQUAL_INT(SIGUSR1, test_common_handlerSigNo);
}


/* SA_SIGINFO: si_signo matches the delivered signal */
TEST(signal_sigaction, sa_siginfo_si_signo)
{
	struct sigaction sa;
	siginfo_t localInfo;
	int ret;

	test_common_siginfoCalled = 0;
	memset((void *)&test_common_siginfo, 0, sizeof(siginfo_t));

	memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = test_sigactionHandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_SIGINFO;

	ret = sigaction(SIGUSR2, &sa, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = raise(SIGUSR2);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(1, test_common_siginfoCalled);

	localInfo = *(siginfo_t *)&test_common_siginfo;
	TEST_ASSERT_EQUAL_INT(SIGUSR2, localInfo.si_signo);
}


/* action persists across multiple signal deliveries (no SA_RESETHAND) */
TEST(signal_sigaction, action_persists)
{
	struct sigaction sa;
	int ret;

	test_common_handlerCalled = 0;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = test_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	ret = sigaction(SIGUSR1, &sa, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = raise(SIGUSR1);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(1, test_common_handlerCalled);

	/* handler should still be active for next delivery */
	test_common_handlerCalled = 0;
	ret = raise(SIGUSR1);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(1, test_common_handlerCalled);

	/* and one more time */
	test_common_handlerCalled = 0;
	ret = raise(SIGUSR1);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(1, test_common_handlerCalled);
}


/* if sigaction() fails, no new signal handler is installed */
TEST(signal_sigaction, failure_no_change)
{
	struct sigaction sa, oact;
	int ret;

	/* install a known handler first */
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = test_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	ret = sigaction(SIGUSR1, &sa, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* try to install on an invalid signal */
	sa.sa_handler = SIG_IGN;
	errno = 0;
	ret = sigaction(0, &sa, NULL);
	TEST_ASSERT_EQUAL_INT(-1, ret);

	/* original handler should still be in place */
	memset(&oact, 0, sizeof(oact));
	ret = sigaction(SIGUSR1, NULL, &oact);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_TRUE(oact.sa_handler == test_handler);
}


/* SIGKILL and SIGSTOP shall not be added to sa_mask (system enforces silently) */
TEST(signal_sigaction, sigkill_sigstop_not_in_sa_mask)
{
	struct sigaction sa;
	sigset_t local;
	int ret;

	test_common_handlerCalled = 0;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = test_handlerMask;
	sigfillset(&sa.sa_mask);
	sa.sa_flags = 0;

	/* installing with SIGKILL/SIGSTOP in sa_mask should succeed (system silently removes them) */
	ret = sigaction(SIGUSR1, &sa, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = raise(SIGUSR1);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(1, test_common_handlerCalled);

	/* SIGKILL and SIGSTOP should not actually be blocked in the handler */
	local = test_common_handlerMask;
	TEST_ASSERT_EQUAL_INT(0, sigismember(&local, SIGKILL));
	TEST_ASSERT_EQUAL_INT(0, sigismember(&local, SIGSTOP));
}


/* sigaction() can be used with each catchable signal */
TEST(signal_sigaction, all_catchable_signals)
{
	struct sigaction sa;
	int ret;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = test_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	for (int signo = 1; signo < USERSPACE_NSIG; ++signo) {
		if (signal_is_unblockable(signo)) {
			continue;
		}

		test_common_handlerCalled = 0;
		test_common_handlerSigNo = 0;

		ret = sigaction(signo, &sa, NULL);
		TEST_ASSERT_EQUAL_INT(0, ret);

		ret = raise(signo);
		TEST_ASSERT_EQUAL_INT(0, ret);
		TEST_ASSERT_EQUAL_INT(1, test_common_handlerCalled);
		TEST_ASSERT_EQUAL_INT(signo, test_common_handlerSigNo);
	}
}


/* querying SIGKILL via oact with act=NULL is allowed */
TEST(signal_sigaction, query_sigkill_allowed)
{
	struct sigaction oact;
	int ret;

	memset(&oact, 0, sizeof(oact));
	ret = sigaction(SIGKILL, NULL, &oact);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


/* querying SIGSTOP via oact with act=NULL is allowed */
TEST(signal_sigaction, query_sigstop_allowed)
{
	struct sigaction oact;
	int ret;

	memset(&oact, 0, sizeof(oact));
	ret = sigaction(SIGSTOP, NULL, &oact);
	TEST_ASSERT_EQUAL_INT(0, ret);
}


/* sigaction() replaces a previously installed handler */
TEST(signal_sigaction, replace_handler)
{
	struct sigaction sa;
	int ret;

	test_common_handlerCalled = 0;
	test_common_saResetCalled = 0;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = test_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	ret = sigaction(SIGUSR1, &sa, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* replace with different handler */
	sa.sa_handler = test_handlerReset;
	ret = sigaction(SIGUSR1, &sa, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = raise(SIGUSR1);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, test_common_handlerCalled);
	TEST_ASSERT_EQUAL_INT(1, test_common_saResetCalled);
}


/* SA_SIGINFO flag is stored and retrievable via oact */
TEST(signal_sigaction, sa_siginfo_flag_preserved)
{
	struct sigaction sa, oact;
	int ret;

	memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = test_sigactionHandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_SIGINFO;

	ret = sigaction(SIGUSR1, &sa, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	memset(&oact, 0, sizeof(oact));
	ret = sigaction(SIGUSR1, NULL, &oact);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_TRUE((oact.sa_flags & SA_SIGINFO) != 0);
}


/* sa_mask is correctly stored and retrieved */
TEST(signal_sigaction, sa_mask_preserved_in_oact)
{
	struct sigaction sa, oact;
	int ret;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = test_handler;
	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, SIGALRM);
	sigaddset(&sa.sa_mask, SIGPIPE);
	sa.sa_flags = 0;

	ret = sigaction(SIGUSR1, &sa, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	memset(&oact, 0, sizeof(oact));
	ret = sigaction(SIGUSR1, NULL, &oact);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(1, sigismember(&oact.sa_mask, SIGALRM));
	TEST_ASSERT_EQUAL_INT(1, sigismember(&oact.sa_mask, SIGPIPE));
	TEST_ASSERT_EQUAL_INT(0, sigismember(&oact.sa_mask, SIGUSR2));
}


/* sigaction() returns -1 for negative signal numbers */
TEST(signal_sigaction, einval_negative_signal)
{
	struct sigaction sa;
	int ret;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = test_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	errno = 0;
	ret = sigaction(-1, &sa, NULL);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


/* SA_RESETHAND: SA_SIGINFO flag is cleared on entry to handler */
TEST(signal_sigaction, sa_resethand_clears_siginfo)
{
	struct sigaction sa, oact;
	int ret;

	test_common_siginfoCalled = 0;

	memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = test_sigactionHandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_SIGINFO | SA_RESETHAND | SA_NODEFER;

	ret = sigaction(SIGUSR1, &sa, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = raise(SIGUSR1);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(1, test_common_siginfoCalled);

	/* after reset, disposition should be SIG_DFL and SA_SIGINFO cleared */
	memset(&oact, 0, sizeof(oact));
	ret = sigaction(SIGUSR1, NULL, &oact);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_TRUE(oact.sa_handler == SIG_DFL);
#ifndef __phoenix__
	if ((oact.sa_flags & SA_SIGINFO) != 0) {
		TEST_IGNORE_MESSAGE("host-pc bug: glibc does not clear SA_SIGINFO on SA_RESETHAND");
	}
#endif
	TEST_ASSERT_EQUAL_INT(0, (oact.sa_flags & SA_SIGINFO));
}


TEST_GROUP_RUNNER(signal_sigaction)
{
	RUN_TEST_CASE(signal_sigaction, install_handler);
	RUN_TEST_CASE(signal_sigaction, query_current_action);
	RUN_TEST_CASE(signal_sigaction, retrieve_previous_action);
	RUN_TEST_CASE(signal_sigaction, einval_zero_signal);
	RUN_TEST_CASE(signal_sigaction, einval_large_signal);
	RUN_TEST_CASE(signal_sigaction, einval_negative_signal);
	RUN_TEST_CASE(signal_sigaction, einval_catch_sigkill);
	RUN_TEST_CASE(signal_sigaction, einval_catch_sigstop);
	RUN_TEST_CASE(signal_sigaction, einval_ignore_sigkill);
	RUN_TEST_CASE(signal_sigaction, einval_ignore_sigstop);
	RUN_TEST_CASE(signal_sigaction, sig_ign);
	RUN_TEST_CASE(signal_sigaction, sig_dfl_restore);
	RUN_TEST_CASE(signal_sigaction, sa_mask_blocked_in_handler);
	RUN_TEST_CASE(signal_sigaction, caught_signal_blocked_in_handler);
	RUN_TEST_CASE(signal_sigaction, mask_restored_after_handler);
	RUN_TEST_CASE(signal_sigaction, sa_nodefer);
	RUN_TEST_CASE(signal_sigaction, sa_nodefer_sa_mask_still_applied);
	RUN_TEST_CASE(signal_sigaction, sa_resethand);
	RUN_TEST_CASE(signal_sigaction, sa_resethand_clears_siginfo);
	RUN_TEST_CASE(signal_sigaction, sa_siginfo_handler);
	RUN_TEST_CASE(signal_sigaction, sa_siginfo_si_signo);
	RUN_TEST_CASE(signal_sigaction, sa_siginfo_flag_preserved);
	RUN_TEST_CASE(signal_sigaction, sa_mask_preserved_in_oact);
	RUN_TEST_CASE(signal_sigaction, action_persists);
	RUN_TEST_CASE(signal_sigaction, failure_no_change);
	RUN_TEST_CASE(signal_sigaction, sigkill_sigstop_not_in_sa_mask);
	RUN_TEST_CASE(signal_sigaction, all_catchable_signals);
	RUN_TEST_CASE(signal_sigaction, query_sigkill_allowed);
	RUN_TEST_CASE(signal_sigaction, query_sigstop_allowed);
	RUN_TEST_CASE(signal_sigaction, replace_handler);
}
