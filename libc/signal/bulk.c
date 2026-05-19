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

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <unity_fixture.h>

#pragma GCC diagnostic ignored "-Wunused-function"


#define SPAWN_CNT 1000

/* Signal action types for generalized tests */
#define SIG_TEST_CUSTOM_HANDLER 0 /* Install custom handler to track signal */
#define SIG_TEST_IGNORE         1 /* Install SIG_IGN */
#define SIG_TEST_DEFAULT        2 /* Use default action (SIG_DFL) */

static void (*test_action)(int signo);
static volatile int handlersignum;
static int handlersignum_exp;
static int retv_exp;
static int rets_exp;
static int ret_undef;
static sigset_t emptyset;


static void handler(int signum)
{
	handlersignum = signum;
}

static void handler_ign(int signum)
{
	(void)signum;
}

static pid_t bulkspawn_pChPair(int (*pFn)(pid_t, void *), int (*chFn)(void *), void *arg)
{
	pid_t chpid;
	pid_t ppid = fork();
	if (ppid == -1) {
		return -1;
	}
	if (ppid == 0) {
		chpid = fork();
		if (chpid == -1) {
			exit(errno);
		}
		if (chpid == 0) {
			exit(chFn(arg));
		}
		exit(pFn(chpid, arg));
	}
	return ppid;
}

static int bulkspawn(int (*pFn)(pid_t, void *), int (*chFn)(void *), size_t cnt, void *arg)
{
	pid_t *p = malloc(cnt * sizeof(pid_t));
	for (size_t i = 0; i < cnt; i++) {
		p[i] = bulkspawn_pChPair(pFn, chFn, arg);
		if (p[i] == -1) {
			printf("Fork failed for process %zu errno %d\n", i, errno);
			return -1;
		}
	}
	for (size_t i = 0; i < cnt; i++) {
		int status;
		if (waitpid(p[i], &status, 0) == -1) {
			printf("Wait failed for process %zu errno %d\n", i, errno);
			free(p);
			return -1;
		}
		if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
			printf("Child process %zu (pid %d) failed with status %d (sig %d)\n", i, p[i], WEXITSTATUS(status), WTERMSIG(status));
			free(p);
			return -1;
		}
	}
	free(p);
	return 0;
}


TEST_GROUP(sigbulk);


TEST_SETUP(sigbulk)
{
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGCONT);  // Use that for synchronization between parent and child
	sigprocmask(SIG_SETMASK, &set, NULL);

	sigaction(SIGCONT, &(struct sigaction) { .sa_handler = handler_ign }, NULL);
	handlersignum = -1;

	sigemptyset(&emptyset);
}


TEST_TEAR_DOWN(sigbulk)
{
}


static int parent(pid_t ch, void *arg)
{
	int s;
	sigsuspend(&emptyset);
	kill(ch, *(int *)arg);
	if (waitpid(ch, &s, 0) == -1) {
		return errno;
	}
	if (ret_undef == 0 && WEXITSTATUS(s) != retv_exp) {
		return -1;
	}
	if (ret_undef == 0 && WTERMSIG(s) != rets_exp) {
		return -1;
	}
	return 0;
}

static int chld_spin(void *arg)
{
	int signum = *(int *)arg;
	struct sigaction sa;

	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = test_action;

	sigaction(signum, &sa, NULL);

	kill(getppid(), SIGCONT);

	while (handlersignum == -1) { }
	if (handlersignum != handlersignum_exp) {
		return -1;
	}
	return 0;
}

static int chld_suspend(void *arg)
{
	int signum = *(int *)arg;
	sigset_t set;
	struct sigaction sa;

	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = test_action;

	sigaction(signum, &sa, NULL);

	sigemptyset(&set);
	sigaddset(&set, signum);
	sigprocmask(SIG_BLOCK, &set, NULL);

	kill(getppid(), SIGCONT);

	sigsuspend(&emptyset);

	/* After suspension, verify handler was called appropriately */
	if (handlersignum != handlersignum_exp) {
		return -1;
	}
	return 0;
}

static int chld_mask(void *arg)
{
	int signum = *(int *)arg;
	sigset_t set;
	struct sigaction sa;

	sigemptyset(&set);
	sigaddset(&set, signum);
	sigprocmask(SIG_SETMASK, &set, NULL);

	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = test_action;

	sigaction(signum, &sa, NULL);

	kill(getppid(), SIGCONT);

	sleep(1);

	/* Signal should be masked, so handler should never be called */
	if (handlersignum != -1) {
		return -1;
	}
	return 0;
}

static int chld_mask_unmask(void *arg)
{
	int signum = *(int *)arg;
	sigset_t set;
	struct sigaction sa;

	sigemptyset(&set);
	sigaddset(&set, signum);
	sigprocmask(SIG_SETMASK, &set, NULL);

	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = test_action;

	sigaction(signum, &sa, NULL);

	kill(getppid(), SIGCONT);

	sleep(1);

	sigprocmask(SIG_SETMASK, &emptyset, NULL);

	/* After unmasking, verify handler behavior matches expectations */
	if (handlersignum != handlersignum_exp) {
		return -1;
	}
	return 0;
}

static int chld_mask_race(void *arg)
{
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, *(int *)arg);

	sigaction(*(int *)arg, &(struct sigaction) { .sa_handler = test_action }, NULL);

	kill(getppid(), SIGCONT);

	sleep(0);  // yield

	sigprocmask(SIG_SETMASK, &set, NULL);

	// This is a race on purpose, results can vary
	if (handlersignum != -1 && handlersignum != handlersignum_exp) {
		return -1;
	}
	return 0;
}

static int chld_sigact_race(void *arg)
{
	kill(getppid(), SIGCONT);

	sleep(0);  // yield

	sigaction(*(int *)arg, &(struct sigaction) { .sa_handler = test_action }, NULL);

	// This is a race on purpose, results can vary
	if (handlersignum != -1 && handlersignum != handlersignum_exp) {
		return -1;
	}
	return 0;
}

TEST(sigbulk, handler)
{
	test_action = handler;
	// Test with default-terminating signal
	int arg = SIGUSR1;

	handlersignum_exp = arg;
	retv_exp = 0;
	rets_exp = 0;
	ret_undef = 0;
	TEST_ASSERT_EQUAL_INT(0, bulkspawn(parent, chld_spin, SPAWN_CNT, &arg));
	printf("Spin test passed\n");
	TEST_ASSERT_EQUAL_INT(0, bulkspawn(parent, chld_suspend, SPAWN_CNT, &arg));
	printf("Suspend test passed\n");
	TEST_ASSERT_EQUAL_INT(0, bulkspawn(parent, chld_mask, SPAWN_CNT, &arg));
	printf("Mask test passed\n");
	TEST_ASSERT_EQUAL_INT(0, bulkspawn(parent, chld_mask_unmask, SPAWN_CNT, &arg));
	printf("Unmask test passed\n");
	ret_undef = 1;
	TEST_ASSERT_EQUAL_INT(0, bulkspawn(parent, chld_mask_race, SPAWN_CNT, &arg));
	printf("Race test passed\n");
	TEST_ASSERT_EQUAL_INT(0, bulkspawn(parent, chld_sigact_race, SPAWN_CNT, &arg));
	printf("Signal action race test passed\n");

	// Test with default-ignored signal
	arg = SIGURG;
	handlersignum_exp = arg;
	retv_exp = 0;
	rets_exp = 0;
	ret_undef = 0;
	TEST_ASSERT_EQUAL_INT(0, bulkspawn(parent, chld_spin, SPAWN_CNT, &arg));
	printf("Spin test passed\n");
	TEST_ASSERT_EQUAL_INT(0, bulkspawn(parent, chld_suspend, SPAWN_CNT, &arg));
	printf("Suspend test passed\n");
	TEST_ASSERT_EQUAL_INT(0, bulkspawn(parent, chld_mask, SPAWN_CNT, &arg));
	printf("Mask test passed\n");
	TEST_ASSERT_EQUAL_INT(0, bulkspawn(parent, chld_mask_unmask, SPAWN_CNT, &arg));
	printf("Unmask test passed\n");
	ret_undef = 1;
	TEST_ASSERT_EQUAL_INT(0, bulkspawn(parent, chld_mask_race, SPAWN_CNT, &arg));
	printf("Race test passed\n");
	TEST_ASSERT_EQUAL_INT(0, bulkspawn(parent, chld_sigact_race, SPAWN_CNT, &arg));
	printf("Signal action race test passed\n");
}
TEST(sigbulk, ignore)
{
	test_action = SIG_IGN;
	// Test with default-terminating signal
	int arg = SIGUSR1;

	handlersignum_exp = -1;
	retv_exp = 0;
	rets_exp = 0;
	ret_undef = 0;

	// Disable as ignored signals will lead to infinite loops/waits
	// TEST_ASSERT_EQUAL_INT(0, bulkspawn(parent, chld_spin, SPAWN_CNT, &arg));
	// printf("Spin test passed\n");
	// TEST_ASSERT_EQUAL_INT(0, bulkspawn(parent, chld_suspend, SPAWN_CNT, &arg));
	// printf("Suspend test passed\n");

	TEST_ASSERT_EQUAL_INT(0, bulkspawn(parent, chld_mask, SPAWN_CNT, &arg));
	printf("Mask test passed\n");
	TEST_ASSERT_EQUAL_INT(0, bulkspawn(parent, chld_mask_unmask, SPAWN_CNT, &arg));
	printf("Unmask test passed\n");
	ret_undef = 1;
	TEST_ASSERT_EQUAL_INT(0, bulkspawn(parent, chld_mask_race, SPAWN_CNT, &arg));
	printf("Race test passed\n");
	TEST_ASSERT_EQUAL_INT(0, bulkspawn(parent, chld_sigact_race, SPAWN_CNT, &arg));
	printf("Signal action race test passed\n");

	// Test with default-ignored signal
	arg = SIGURG;
	handlersignum_exp = -1;
	retv_exp = 0;
	rets_exp = 0;
	ret_undef = 0;

	// Disable as ignored signals will lead to infinite loops/waits
	// TEST_ASSERT_EQUAL_INT(0, bulkspawn(parent, chld_spin, SPAWN_CNT, &arg));
	// printf("Spin test passed\n");
	// TEST_ASSERT_EQUAL_INT(0, bulkspawn(parent, chld_suspend, SPAWN_CNT, &arg));
	// printf("Suspend test passed\n");

	TEST_ASSERT_EQUAL_INT(0, bulkspawn(parent, chld_mask, SPAWN_CNT, &arg));
	printf("Mask test passed\n");
	TEST_ASSERT_EQUAL_INT(0, bulkspawn(parent, chld_mask_unmask, SPAWN_CNT, &arg));
	printf("Unmask test passed\n");
	ret_undef = 1;
	TEST_ASSERT_EQUAL_INT(0, bulkspawn(parent, chld_mask_race, SPAWN_CNT, &arg));
	printf("Race test passed\n");
	TEST_ASSERT_EQUAL_INT(0, bulkspawn(parent, chld_sigact_race, SPAWN_CNT, &arg));
	printf("Signal action race test passed\n");
}
TEST(sigbulk, def_ignore)
{
	test_action = SIG_DFL;
	int arg = SIGURG;

	handlersignum_exp = -1;
	retv_exp = 0;
	rets_exp = 0;
	ret_undef = 0;

	// Disable as ignored signals will lead to infinite loops/waits
	// TEST_ASSERT_EQUAL_INT(0, bulkspawn(parent, chld_spin, SPAWN_CNT, &arg));
	// printf("Spin test passed\n");
	// TEST_ASSERT_EQUAL_INT(0, bulkspawn(parent, chld_suspend, SPAWN_CNT, &arg));
	// printf("Suspend test passed\n");

	TEST_ASSERT_EQUAL_INT(0, bulkspawn(parent, chld_mask, SPAWN_CNT, &arg));
	printf("Mask test passed\n");
	TEST_ASSERT_EQUAL_INT(0, bulkspawn(parent, chld_mask_unmask, SPAWN_CNT, &arg));
	printf("Unmask test passed\n");
	ret_undef = 1;
	TEST_ASSERT_EQUAL_INT(0, bulkspawn(parent, chld_mask_race, SPAWN_CNT, &arg));
	printf("Race test passed\n");
	TEST_ASSERT_EQUAL_INT(0, bulkspawn(parent, chld_sigact_race, SPAWN_CNT, &arg));
	printf("Signal action race test passed\n");
}
TEST(sigbulk, sigkill)
{
	test_action = SIG_DFL;
	int arg = SIGKILL;

	handlersignum_exp = -1;
	retv_exp = 0;
	rets_exp = SIGKILL;
	ret_undef = 0;
	TEST_ASSERT_EQUAL_INT(0, bulkspawn(parent, chld_spin, SPAWN_CNT, &arg));
	printf("Spin test passed\n");
	TEST_ASSERT_EQUAL_INT(0, bulkspawn(parent, chld_suspend, SPAWN_CNT, &arg));
	printf("Suspend test passed\n");
	TEST_ASSERT_EQUAL_INT(0, bulkspawn(parent, chld_mask, SPAWN_CNT, &arg));
	printf("Mask test passed\n");
	TEST_ASSERT_EQUAL_INT(0, bulkspawn(parent, chld_mask_unmask, SPAWN_CNT, &arg));
	printf("Unmask test passed\n");
	ret_undef = 1;
	TEST_ASSERT_EQUAL_INT(0, bulkspawn(parent, chld_mask_race, SPAWN_CNT, &arg));
	printf("Race test passed\n");
	TEST_ASSERT_EQUAL_INT(0, bulkspawn(parent, chld_sigact_race, SPAWN_CNT, &arg));
	printf("Signal action race test passed\n");

	// ensure SIGKILL cannot be ignored or handled
	test_action = handler;
	arg = SIGKILL;

	handlersignum_exp = -1;
	retv_exp = 0;
	rets_exp = SIGKILL;
	ret_undef = 0;
	TEST_ASSERT_EQUAL_INT(0, bulkspawn(parent, chld_spin, SPAWN_CNT, &arg));
	printf("Spin test passed\n");
	TEST_ASSERT_EQUAL_INT(0, bulkspawn(parent, chld_suspend, SPAWN_CNT, &arg));
	printf("Suspend test passed\n");
	TEST_ASSERT_EQUAL_INT(0, bulkspawn(parent, chld_mask, SPAWN_CNT, &arg));
	printf("Mask test passed\n");
	TEST_ASSERT_EQUAL_INT(0, bulkspawn(parent, chld_mask_unmask, SPAWN_CNT, &arg));
	printf("Unmask test passed\n");
	ret_undef = 1;
	TEST_ASSERT_EQUAL_INT(0, bulkspawn(parent, chld_mask_race, SPAWN_CNT, &arg));
	printf("Race test passed\n");
	TEST_ASSERT_EQUAL_INT(0, bulkspawn(parent, chld_sigact_race, SPAWN_CNT, &arg));
	printf("Signal action race test passed\n");
}


TEST_GROUP_RUNNER(sigbulk)
{
	RUN_TEST_CASE(sigbulk, handler);
	RUN_TEST_CASE(sigbulk, ignore);
	RUN_TEST_CASE(sigbulk, def_ignore);
	RUN_TEST_CASE(sigbulk, sigkill);
}
