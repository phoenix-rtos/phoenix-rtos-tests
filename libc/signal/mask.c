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


TEST_GROUP_RUNNER(mask)
{
	RUN_TEST_CASE(mask, sigset_full);
	RUN_TEST_CASE(mask, sigset_single);
	RUN_TEST_CASE(mask, procmask_set);
}
