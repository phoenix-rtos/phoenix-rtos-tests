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
#include <stdlib.h>
#include <sys/wait.h>

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

	/* disable any active alarm timer */
	alarm(0);

	/* set default signal disposition for all signals */
	for (int signo = 1; signo < USERSPACE_NSIG; ++signo) {
		signal(signo, SIG_DFL);
	}
}


static volatile sig_atomic_t handler_haveSignal;
static volatile sigset_t handler_sigset;
static volatile sig_atomic_t handler_countdown;
static volatile struct sigaction handler_sigaction;


static pid_t safe_fork(void)
{
	pid_t pid;
	if ((pid = fork()) < 0) {
		if (errno == ENOSYS) {
			TEST_IGNORE_MESSAGE("fork syscall not supported");
		}
		else {
			FAIL("fork");
		}
	}
	return pid;
}


static void sighandler(int sig)
{
	sigset_t set;
	sigprocmask(SIG_SETMASK, NULL, &set);

	/* WARN: the write might not be atomic */
	handler_sigset = set;

	handler_haveSignal |= (1u << sig);
}


static void sighandlerRecursive(int sig)
{
	if (handler_countdown > 0) {
		handler_countdown--;
		sighandlerRecursive(sig);
	}
}


static void sighandlerReraise(int sig)
{
	sigset_t set;
	sigprocmask(SIG_SETMASK, NULL, &set);

	/* WARN: the write might not be atomic */
	handler_sigset = set;

	if (handler_countdown > 0) {
		handler_countdown--;
		raise(sig);
	}
}


static void sighandlerAction(int sig)
{
	struct sigaction sa;
	sa = handler_sigaction;
	sigaction(sig, &sa, NULL);
	if (sig == SIGUSR1) {
		raise(sig);
	}
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


TEST(handler, unblock_pending_signal)
{
	sigset_t set;
	handler_haveSignal = 0;

	TEST_ASSERT_EQUAL_INT(0, sigprocmask(0, NULL, &set));
	TEST_ASSERT_EQUAL_INT(0, sigaddset(&set, SIGUSR1));
	TEST_ASSERT_EQUAL_INT(0, sigdelset(&set, SIGALRM));
	TEST_ASSERT_EQUAL_INT(0, sigprocmask(SIG_SETMASK, &set, NULL));

	signal(SIGALRM, sighandler);
	signal(SIGUSR1, sighandler);

	/* send signal and wait 1s to be sure it won't arrive */
	TEST_ASSERT_EQUAL_INT(0, alarm(1));
	TEST_ASSERT_EQUAL_INT(0, raise(SIGUSR1));

	if (handler_haveSignal == 0) {
		errno = 0;
		TEST_ASSERT_EQUAL_INT(-1, pause());
		TEST_ASSERT_EQUAL_INT(EINTR, errno);
	}

	/* check we timeouted as expected */
	TEST_ASSERT_EQUAL_HEX32((1u << SIGALRM), handler_haveSignal);
	handler_haveSignal = 0;

	/* set timeout and unblock pending SIGUSR1 */
	TEST_ASSERT_EQUAL_INT(0, alarm(1));
	TEST_ASSERT_EQUAL_INT(0, sigprocmask(SIG_UNBLOCK, &set, NULL));

	if (handler_haveSignal == 0) {
		errno = 0;
		TEST_ASSERT_EQUAL_INT(-1, pause());
		TEST_ASSERT_EQUAL_INT(EINTR, errno);
	}

	/* check we received SIGUSR1 not timeouted */
	TEST_ASSERT_EQUAL_HEX32((1u << SIGUSR1), handler_haveSignal);
}

/* TODO: test sa_flags - especially SA_NODEFER */


TEST_GROUP_RUNNER(handler)
{
	RUN_TEST_CASE(handler, sighandler_sa_mask);
	RUN_TEST_CASE(handler, sighandler_signal_in_signal);
	RUN_TEST_CASE(handler, unblock_pending_signal);
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


TEST_GROUP(sigaction);


TEST_SETUP(sigaction)
{
	handler_haveSignal = 0u;
	handler_countdown = 5;
}


TEST_TEAR_DOWN(sigaction)
{
	/* unblock all signals */
	sigset_t set;
	sigemptyset(&set);
	sigprocmask(SIG_SETMASK, &set, NULL);

	/* disable any active alarm timer */
	alarm(0);

	/* set default signal disposition for all signals */
	for (int signo = 1; signo < USERSPACE_NSIG; ++signo) {
		signal(signo, SIG_DFL);
	}
}


TEST(sigaction, signal_termination_statuscode)
{
	static const int termination_signals[] = {
		SIGILL, SIGSEGV, SIGHUP, SIGINT, SIGQUIT, SIGTRAP, SIGABRT, SIGFPE, SIGKILL, SIGBUS,
		SIGSYS, SIGPIPE, SIGALRM, SIGTERM, SIGIO, SIGXCPU, SIGXFSZ, SIGVTALRM, SIGPROF, SIGUSR1, SIGUSR2
	};

	for (int i = 0; i < sizeof(termination_signals) / sizeof(termination_signals[0]); ++i) {
		int sig = termination_signals[i];
		pid_t pid = safe_fork();
		TEST_ASSERT_GREATER_OR_EQUAL(0, pid);
		if (pid > 0) {
			int code;
			waitpid(pid, &code, 0);
			TEST_ASSERT_EQUAL_INT(true, WIFSIGNALED(code));
			TEST_ASSERT_EQUAL_HEX32(sig, WTERMSIG(code));
		}
		else {
			signal(sig, SIG_DFL);
			raise(sig);
			exit(0);
		}
	}
}


TEST(sigaction, signal_default_ignored)
{
	static const int ignored_signals[] = { SIGURG, SIGCHLD, SIGWINCH };

	for (int i = 0; i < sizeof(ignored_signals) / sizeof(ignored_signals[0]); ++i) {
		int sig = ignored_signals[i];
		pid_t pid = safe_fork();
		TEST_ASSERT_GREATER_OR_EQUAL(0, pid);
		if (pid > 0) {
			int code;
			waitpid(pid, &code, 0);
			TEST_ASSERT_EQUAL_INT(true, WIFEXITED(code));
			TEST_ASSERT_EQUAL_HEX32(0, WEXITSTATUS(code));
		}
		else {
			signal(sig, SIG_DFL);
			raise(sig);
			exit(0);
		}
	}
}


/* check if signal action is performed on unmasking after being changed */
TEST(sigaction, unmask_changed_action_handler_to_ignore)
{
	sigset_t masked, empty;
	sigemptyset(&empty);
	sigemptyset(&masked);
	sigaddset(&masked, SIGUSR1);

	sigprocmask(SIG_SETMASK, &masked, NULL);
	signal(SIGUSR1, sighandler);
	raise(SIGUSR1);
	signal(SIGUSR1, SIG_IGN);
	sigprocmask(SIG_SETMASK, &empty, NULL);
	TEST_ASSERT_EQUAL_HEX32(0, handler_haveSignal);
	signal(SIGUSR1, sighandler);
	TEST_ASSERT_EQUAL_HEX32(0, handler_haveSignal);
}


TEST(sigaction, unmask_changed_action_default_to_ignore)
{
	sigset_t masked, empty;
	sigemptyset(&empty);
	sigemptyset(&masked);
	sigaddset(&masked, SIGUSR1);

	sigprocmask(SIG_SETMASK, &masked, NULL);
	signal(SIGUSR1, SIG_DFL);
	raise(SIGUSR1);
	signal(SIGUSR1, SIG_IGN);
	sigprocmask(SIG_SETMASK, &empty, NULL);
	TEST_ASSERT_EQUAL_HEX32(0, handler_haveSignal);
	signal(SIGUSR1, SIG_DFL);
	TEST_ASSERT_EQUAL_HEX32(0, handler_haveSignal);
}


TEST(sigaction, unmask_changed_action_default_to_handler)
{
	sigset_t masked, empty;
	sigemptyset(&empty);
	sigemptyset(&masked);
	sigaddset(&masked, SIGUSR1);

	sigprocmask(SIG_SETMASK, &masked, NULL);
	signal(SIGUSR1, SIG_DFL);
	raise(SIGUSR1);
	signal(SIGUSR1, sighandler);
	sigprocmask(SIG_SETMASK, &empty, NULL);
	TEST_ASSERT_EQUAL_HEX32((1u << SIGUSR1), handler_haveSignal);
}


TEST(sigaction, unmask_changed_action_handler_to_default_ignored)
{
	sigset_t masked, empty;
	sigemptyset(&empty);
	sigemptyset(&masked);
	sigaddset(&masked, SIGURG);

	sigprocmask(SIG_SETMASK, &masked, NULL);
	signal(SIGURG, sighandler);
	raise(SIGURG);
	signal(SIGURG, SIG_DFL);
	sigprocmask(SIG_SETMASK, &empty, NULL);
	TEST_ASSERT_EQUAL_HEX32(0, handler_haveSignal);
	signal(SIGURG, sighandler);
	TEST_ASSERT_EQUAL_HEX32(0, handler_haveSignal);
}


TEST(sigaction, unmask_changed_action_handler_to_default)
{
	sigset_t masked, empty;
	sigemptyset(&empty);
	sigemptyset(&masked);
	sigaddset(&masked, SIGUSR1);

	pid_t pid = safe_fork();
	TEST_ASSERT_GREATER_OR_EQUAL(0, pid);
	if (pid > 0) {
		int code;
		wait(&code);
		TEST_ASSERT_EQUAL_INT(true, WIFSIGNALED(code));
	}
	else {
		handler_haveSignal = 0u;
		sigprocmask(SIG_SETMASK, &masked, NULL);
		signal(SIGUSR1, sighandler);
		raise(SIGUSR1);
		signal(SIGUSR1, SIG_DFL);
		sigprocmask(SIG_SETMASK, &empty, NULL);
		/* POSIX: after pthread_sigmask() changes the currently blocked set of signals it shall determine
		 * whether there are any pending unblocked signals; if there are any, then at least one of those signals
		 * shall be delivered before the call to pthread_sigmask() returns
		 */

		exit(0);
	}
}


TEST(sigaction, handler_recursion_direct)
{
	sigset_t empty;
	sigemptyset(&empty);

	struct sigaction act = {
		.sa_handler = sighandlerRecursive,
		.sa_flags = 0,
		.sa_mask = empty,
	};
	sigaction(SIGUSR1, &act, NULL);
	raise(SIGUSR1);
	TEST_ASSERT_EQUAL_INT(0, handler_countdown);
}


TEST(sigaction, handler_recursion_raise)
{
	sigset_t set;
	sigset_t empty;
	sigemptyset(&empty);

	struct sigaction act = {
		.sa_handler = sighandlerReraise,
		.sa_flags = 0,
		.sa_mask = empty,
	};

	sigaction(SIGUSR1, &act, NULL);
	raise(SIGUSR1);
	while (handler_countdown > 0) {
		sigsuspend(&empty);
	}
	TEST_ASSERT_EQUAL_INT(0, handler_countdown);
	set = handler_sigset;
	TEST_ASSERT_EQUAL_INT(1, sigismember(&set, SIGUSR1));
}


TEST(sigaction, handler_recursion_raise_nodefer)
{
	sigset_t set;
	sigset_t empty;
	sigemptyset(&empty);

	struct sigaction act = {
		.sa_handler = sighandlerReraise,
		.sa_flags = SA_NODEFER,
		.sa_mask = empty,
	};

	sigaction(SIGUSR1, &act, NULL);
	raise(SIGUSR1);
	TEST_ASSERT_EQUAL_INT(0, handler_countdown);
	set = handler_sigset;
	TEST_ASSERT_EQUAL_INT(0, sigismember(&set, SIGUSR1));
}


TEST(sigaction, sigaction_in_handler_handle)
{
	sigset_t empty;
	sigemptyset(&empty);

	struct sigaction action = {
		.sa_handler = sighandlerAction,
		.sa_flags = 0,
		.sa_mask = empty,
	};
	handler_sigaction.sa_handler = sighandler;
	handler_sigaction.sa_flags = 0;
	handler_sigaction.sa_mask = empty;
	sigaction(SIGUSR2, &action, NULL);
	raise(SIGUSR2);
	TEST_ASSERT_EQUAL_HEX32(0u, handler_haveSignal);
	raise(SIGUSR2);
	TEST_ASSERT_EQUAL_HEX32((1u << SIGUSR2), handler_haveSignal);
}


TEST(sigaction, sigaction_in_handler_handle_reraise)
{
	sigset_t empty;
	sigemptyset(&empty);

	handler_sigaction.sa_handler = sighandler;
	handler_sigaction.sa_flags = 0;
	handler_sigaction.sa_mask = empty;
	struct sigaction action = {
		.sa_handler = sighandlerAction,
		.sa_flags = 0,
		.sa_mask = empty,
	};
	sigaction(SIGUSR1, &action, NULL);
	raise(SIGUSR1);
	if (handler_haveSignal == 0u) {
		sigsuspend(&empty);
	}
	TEST_ASSERT_EQUAL_HEX32((1u << SIGUSR1), handler_haveSignal);
}


TEST(sigaction, sigaction_in_handler_nodefer_handle_reraise)
{
	sigset_t empty;
	sigemptyset(&empty);

	handler_sigaction.sa_handler = sighandler;
	handler_sigaction.sa_flags = 0;
	handler_sigaction.sa_mask = empty;
	struct sigaction actionNodefer = {
		.sa_handler = sighandlerAction,
		.sa_flags = SA_NODEFER,
		.sa_mask = empty,
	};
	sigaction(SIGUSR1, &actionNodefer, NULL);
	raise(SIGUSR1);
	TEST_ASSERT_EQUAL_HEX32((1u << SIGUSR1), handler_haveSignal);
}


TEST(sigaction, sigaction_in_handler_ignore)
{
	sigset_t empty;
	sigemptyset(&empty);

	handler_sigaction.sa_handler = SIG_IGN;
	handler_sigaction.sa_flags = 0;
	handler_sigaction.sa_mask = empty;
	signal(SIGUSR1, &sighandlerAction);
	raise(SIGUSR1);
	raise(SIGUSR1);
	raise(SIGUSR1);
	TEST_ASSERT_EQUAL_PTR(SIG_IGN, signal(SIGUSR1, SIG_IGN));
}


TEST(sigaction, sigaction_in_handler_default)
{
	sigset_t empty;
	sigemptyset(&empty);

	struct sigaction actionNodefer = {
		.sa_handler = sighandlerAction,
		.sa_flags = SA_NODEFER,
		.sa_mask = empty,
	};

	pid_t pid = safe_fork();
	TEST_ASSERT_GREATER_OR_EQUAL(0, pid);
	if (pid > 0) {
		int code;
		wait(&code);
		TEST_ASSERT_EQUAL_INT(true, WIFSIGNALED(code));
		TEST_ASSERT_EQUAL_INT(SIGUSR1, WTERMSIG(code));
	}
	else {
		handler_sigaction.sa_handler = SIG_DFL;
		handler_sigaction.sa_flags = 0;
		handler_sigaction.sa_mask = empty;
		sigaction(SIGUSR1, &actionNodefer, NULL);
		raise(SIGUSR1);
		exit(0);
	}
}


TEST_GROUP_RUNNER(sigaction)
{
	RUN_TEST_CASE(sigaction, signal_termination_statuscode);
	RUN_TEST_CASE(sigaction, signal_default_ignored);

	RUN_TEST_CASE(sigaction, unmask_changed_action_handler_to_ignore);
	RUN_TEST_CASE(sigaction, unmask_changed_action_default_to_ignore);
	RUN_TEST_CASE(sigaction, unmask_changed_action_default_to_handler);
	RUN_TEST_CASE(sigaction, unmask_changed_action_handler_to_default_ignored);
	RUN_TEST_CASE(sigaction, unmask_changed_action_handler_to_default);
	/* initial SIG_IGN is omitted, as:
	 * POSIX: setting sigaction to SIG_IGN can release pending signal
	 */

	RUN_TEST_CASE(sigaction, handler_recursion_direct);
	RUN_TEST_CASE(sigaction, handler_recursion_raise);
	RUN_TEST_CASE(sigaction, handler_recursion_raise_nodefer);

	RUN_TEST_CASE(sigaction, sigaction_in_handler_handle);
	RUN_TEST_CASE(sigaction, sigaction_in_handler_handle_reraise);
	RUN_TEST_CASE(sigaction, sigaction_in_handler_nodefer_handle_reraise);
	RUN_TEST_CASE(sigaction, sigaction_in_handler_ignore);
	RUN_TEST_CASE(sigaction, sigaction_in_handler_default);
}
