/*
 * Phoenix-RTOS
 *
 *    POSIX.1-2024 standard library functions tests
 *    HEADER:
 *    - stdlib.h
 *    TESTED:
 *    - at_quick_exit()
 *    - quick_exit()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Adam Greloch
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <pthread.h>

#include <unity_fixture.h>

#include "libc_features.h"

/*
 * quick_exit() and at_quick_exit() were adopted by POSIX.1-2024 (Issue 8).
 * libc_features.h is generated from a libphoenix build, so it only describes
 * what Phoenix-RTOS provides - everywhere else the tests are built
 * unconditionally, as a hosted C11 library always provides both functions.
 */
#if !defined(__phoenix__) || (defined(HAS_QUICK_EXIT) && defined(HAS_AT_QUICK_EXIT))
#define TEST_QUICK_EXIT_AVAILABLE 1
#endif

/*
	Aspects required by POSIX, which weren't tested:
		- "It is unspecified whether a call to the at_quick_exit() function that does not happen before the
		  quick_exit() function is called will succeed" -> unspecified, so registering new functions from
		  within a registered function is not checked

		- "If a process calls the quick_exit() function more than once, or calls the exit() function in addition
		  to the quick_exit() function, the behavior is undefined" -> undefined behavior

		- "If a signal is raised while the quick_exit() function is executing, the behavior is undefined"
		  -> undefined behavior

		- "If, during the call to any such function, a call to the longjmp() or siglongjmp() function is made
		  that would terminate the call to the registered function, the behavior is undefined"
		  -> undefined behavior

		- the {ATEXIT_MAX} limit obtained with sysconf() is not exhausted - only the minimum of 32 registrations
		  guaranteed by POSIX is checked
 */

#define TEST_QUICK_EXIT_PATH "quick_exit_test_file"
#define TEST_QUICK_EXIT_STR  "test123"

/* Markers written to TEST_QUICK_EXIT_PATH by the registered functions */
#define TEST_MARK_QUICK1 11
#define TEST_MARK_QUICK2 22
#define TEST_MARK_QUICK3 33
#define TEST_MARK_ATEXIT 44
#define TEST_MARK_SIGNAL 55
#define TEST_MARK_AFTER  66
#define TEST_MARK_BULK   77

/* Number of at_quick_exit() registrations POSIX guarantees */
#define TEST_QUICK_EXIT_MIN_REGS 32

/* Exit statuses used to tell the termination paths apart */
#define TEST_STATUS_MAIN     42
#define TEST_STATUS_THREAD   43
#define TEST_STATUS_NORETURN 44
#define TEST_STATUS_HANDLER  45

/* Disable warnings caused by no checking write() return value, since ASSERT cannot be used in child process */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"


#ifdef TEST_QUICK_EXIT_AVAILABLE
/* Appends a single marker to the test file - safe to use from a child process */
static void test_writeMarker(int val)
{
	int fd;

	fd = open(TEST_QUICK_EXIT_PATH, O_WRONLY | O_APPEND);
	write(fd, &val, sizeof(int));
	close(fd);
}


static void test_quickExitFun1(void)
{
	test_writeMarker(TEST_MARK_QUICK1);
}


static void test_quickExitFun2(void)
{
	test_writeMarker(TEST_MARK_QUICK2);
}


static void test_quickExitFun3(void)
{
	test_writeMarker(TEST_MARK_QUICK3);
}


static void test_quickExitBulk(void)
{
	test_writeMarker(TEST_MARK_BULK);
}


/* Registered function that never returns to quick_exit() */
static void test_quickExitNoReturn(void)
{
	test_writeMarker(TEST_MARK_QUICK2);
	_Exit(TEST_STATUS_HANDLER);
}


static void test_atexitFun(void)
{
	test_writeMarker(TEST_MARK_ATEXIT);
}


static void test_signalHandler(int signum)
{
	test_writeMarker(TEST_MARK_SIGNAL);
}


static void *test_quickExitThread(void *arg)
{
	quick_exit(TEST_STATUS_THREAD);

	return NULL;
}


/* Creates an empty test file and returns its descriptor */
static int test_createFile(void)
{
	int fd;

	fd = open(TEST_QUICK_EXIT_PATH, O_RDWR | O_CREAT | O_TRUNC, S_IFREG | DEFFILEMODE);
	TEST_ASSERT_NOT_EQUAL_INT(-1, fd);

	return fd;
}


/* Reads up to max markers written by the child, returns their count */
static int test_readMarkers(int fd, int *markers, int max)
{
	int cnt = 0;

	TEST_ASSERT_EQUAL_INT(0, lseek(fd, 0, SEEK_SET));

	while ((cnt < max) && (read(fd, &markers[cnt], sizeof(int)) == (ssize_t)sizeof(int))) {
		cnt++;
	}

	return cnt;
}


/* Waits for the child and checks its exit status */
static void test_waitChild(pid_t pid, int expectedStatus)
{
	int status, ret;

	ret = waitpid(pid, &status, 0);
	TEST_ASSERT_EQUAL_INT(pid, ret);
	TEST_ASSERT_TRUE(WIFEXITED(status));
	TEST_ASSERT_EQUAL_INT(expectedStatus, WEXITSTATUS(status));
}
#endif


TEST_GROUP(stdlib_at_quick_exit);


TEST_SETUP(stdlib_at_quick_exit)
{
	remove(TEST_QUICK_EXIT_PATH);
}


TEST_TEAR_DOWN(stdlib_at_quick_exit)
{
	remove(TEST_QUICK_EXIT_PATH);
}


/* "Upon successful completion, at_quick_exit() shall return 0; otherwise, it shall return a non-zero value" */
TEST(stdlib_at_quick_exit, returns_zero)
{
#ifndef TEST_QUICK_EXIT_AVAILABLE
	TEST_IGNORE_MESSAGE("at_quick_exit/quick_exit is not implemented");
#else
	pid_t pid;
	int fd, markers[2], cnt;

	fd = test_createFile();

	pid = fork();
	TEST_ASSERT_GREATER_OR_EQUAL(0, pid);
	/* child */
	if (pid == 0) {
		test_writeMarker(at_quick_exit(test_quickExitFun1));
		quick_exit(TEST_STATUS_MAIN);
	}
	/* parent */
	else {
		test_waitChild(pid, TEST_STATUS_MAIN);

		cnt = test_readMarkers(fd, markers, 2);
		TEST_ASSERT_EQUAL_INT(2, cnt);
		/* Return value of at_quick_exit() */
		TEST_ASSERT_EQUAL_INT(0, markers[0]);
		/* The registered function has been called */
		TEST_ASSERT_EQUAL_INT(TEST_MARK_QUICK1, markers[1]);

		close(fd);
	}
#endif
}


/* "At least 32 functions can be registered with at_quick_exit()" */
TEST(stdlib_at_quick_exit, min_32_registrations)
{
#ifndef TEST_QUICK_EXIT_AVAILABLE
	TEST_IGNORE_MESSAGE("at_quick_exit/quick_exit is not implemented");
#else
	pid_t pid;
	int fd, i, cnt;
	int markers[TEST_QUICK_EXIT_MIN_REGS + 1];

	fd = test_createFile();

	pid = fork();
	TEST_ASSERT_GREATER_OR_EQUAL(0, pid);
	/* child */
	if (pid == 0) {
		int failed = 0;

		for (i = 0; i < TEST_QUICK_EXIT_MIN_REGS; i++) {
			if (at_quick_exit(test_quickExitBulk) != 0) {
				failed++;
			}
		}
		/* Number of registrations that did not succeed */
		test_writeMarker(failed);

		quick_exit(TEST_STATUS_MAIN);
	}
	/* parent */
	else {
		test_waitChild(pid, TEST_STATUS_MAIN);

		cnt = test_readMarkers(fd, markers, TEST_QUICK_EXIT_MIN_REGS + 1);
		TEST_ASSERT_EQUAL_INT(TEST_QUICK_EXIT_MIN_REGS + 1, cnt);
		TEST_ASSERT_EQUAL_INT(0, markers[0]);

		/* Every registered function has been called */
		for (i = 1; i <= TEST_QUICK_EXIT_MIN_REGS; i++) {
			TEST_ASSERT_EQUAL_INT(TEST_MARK_BULK, markers[i]);
		}

		close(fd);
	}
#endif
}


/* exit(): "No functions registered by the at_quick_exit() function shall be called" */
TEST(stdlib_at_quick_exit, not_called_by_exit)
{
#ifndef TEST_QUICK_EXIT_AVAILABLE
	TEST_IGNORE_MESSAGE("at_quick_exit/quick_exit is not implemented");
#else
	pid_t pid;
	int fd, markers[2], cnt;

	fd = test_createFile();

	/* The child calls exit(), which flushes the streams inherited from the parent */
	fflush(NULL);

	pid = fork();
	TEST_ASSERT_GREATER_OR_EQUAL(0, pid);
	/* child */
	if (pid == 0) {
		at_quick_exit(test_quickExitFun1);
		atexit(test_atexitFun);
		exit(TEST_STATUS_MAIN);
	}
	/* parent */
	else {
		test_waitChild(pid, TEST_STATUS_MAIN);

		cnt = test_readMarkers(fd, markers, 2);
		/* Only the atexit() registration has been honored */
		TEST_ASSERT_EQUAL_INT(1, cnt);
		TEST_ASSERT_EQUAL_INT(TEST_MARK_ATEXIT, markers[0]);

		close(fd);
	}
#endif
}


/* _Exit(): "shall not call functions registered with atexit() nor at_quick_exit()" */
TEST(stdlib_at_quick_exit, not_called_by_Exit)
{
#ifndef TEST_QUICK_EXIT_AVAILABLE
	TEST_IGNORE_MESSAGE("at_quick_exit/quick_exit is not implemented");
#else
	pid_t pid;
	int fd, markers[2], cnt;

	fd = test_createFile();

	pid = fork();
	TEST_ASSERT_GREATER_OR_EQUAL(0, pid);
	/* child */
	if (pid == 0) {
		at_quick_exit(test_quickExitFun1);
		atexit(test_atexitFun);
		_Exit(TEST_STATUS_MAIN);
	}
	/* parent */
	else {
		test_waitChild(pid, TEST_STATUS_MAIN);

		cnt = test_readMarkers(fd, markers, 2);
		TEST_ASSERT_EQUAL_INT(0, cnt);

		close(fd);
	}
#endif
}


TEST_GROUP(stdlib_quick_exit);


TEST_SETUP(stdlib_quick_exit)
{
	remove(TEST_QUICK_EXIT_PATH);
}


TEST_TEAR_DOWN(stdlib_quick_exit)
{
	remove(TEST_QUICK_EXIT_PATH);
}


/*
 * "The quick_exit() function shall first call all functions registered by at_quick_exit(),
 * in the reverse order of their registration"
 */
TEST(stdlib_quick_exit, handlers_reverse_order)
{
#ifndef TEST_QUICK_EXIT_AVAILABLE
	TEST_IGNORE_MESSAGE("at_quick_exit/quick_exit is not implemented");
#else
	pid_t pid;
	int fd, markers[4], cnt;

	fd = test_createFile();

	pid = fork();
	TEST_ASSERT_GREATER_OR_EQUAL(0, pid);
	/* child */
	if (pid == 0) {
		at_quick_exit(test_quickExitFun1);
		at_quick_exit(test_quickExitFun2);
		at_quick_exit(test_quickExitFun3);
		quick_exit(TEST_STATUS_MAIN);
	}
	/* parent */
	else {
		test_waitChild(pid, TEST_STATUS_MAIN);

		cnt = test_readMarkers(fd, markers, 4);
		TEST_ASSERT_EQUAL_INT(3, cnt);
		TEST_ASSERT_EQUAL_INT(TEST_MARK_QUICK3, markers[0]);
		TEST_ASSERT_EQUAL_INT(TEST_MARK_QUICK2, markers[1]);
		TEST_ASSERT_EQUAL_INT(TEST_MARK_QUICK1, markers[2]);

		close(fd);
	}
#endif
}


/* "It shall not call functions registered with atexit()" */
TEST(stdlib_quick_exit, no_atexit_handlers)
{
#ifndef TEST_QUICK_EXIT_AVAILABLE
	TEST_IGNORE_MESSAGE("at_quick_exit/quick_exit is not implemented");
#else
	pid_t pid;
	int fd, markers[2], cnt;

	fd = test_createFile();

	pid = fork();
	TEST_ASSERT_GREATER_OR_EQUAL(0, pid);
	/* child */
	if (pid == 0) {
		atexit(test_atexitFun);
		at_quick_exit(test_quickExitFun1);
		quick_exit(TEST_STATUS_MAIN);
	}
	/* parent */
	else {
		test_waitChild(pid, TEST_STATUS_MAIN);

		cnt = test_readMarkers(fd, markers, 2);
		/* Only the at_quick_exit() registration has been honored */
		TEST_ASSERT_EQUAL_INT(1, cnt);
		TEST_ASSERT_EQUAL_INT(TEST_MARK_QUICK1, markers[0]);

		close(fd);
	}
#endif
}


/* "nor any registered signal handlers" */
TEST(stdlib_quick_exit, no_signal_handlers)
{
#ifndef TEST_QUICK_EXIT_AVAILABLE
	TEST_IGNORE_MESSAGE("at_quick_exit/quick_exit is not implemented");
#else
	pid_t pid;
	int fd, markers[2], cnt;

	fd = test_createFile();

	pid = fork();
	TEST_ASSERT_GREATER_OR_EQUAL(0, pid);
	/* child */
	if (pid == 0) {
		struct sigaction sa;

		sa.sa_handler = test_signalHandler;
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = 0;
		sigaction(SIGUSR1, &sa, NULL);
		sigaction(SIGTERM, &sa, NULL);
		sigaction(SIGCHLD, &sa, NULL);

		quick_exit(TEST_STATUS_MAIN);
	}
	/* parent */
	else {
		test_waitChild(pid, TEST_STATUS_MAIN);

		cnt = test_readMarkers(fd, markers, 2);
		TEST_ASSERT_EQUAL_INT(0, cnt);

		close(fd);
	}
#endif
}


/* "The quick_exit() function does not return" */
TEST(stdlib_quick_exit, does_not_return)
{
#ifndef TEST_QUICK_EXIT_AVAILABLE
	TEST_IGNORE_MESSAGE("at_quick_exit/quick_exit is not implemented");
#else
	pid_t pid;
	int fd, markers[2], cnt;

	fd = test_createFile();

	pid = fork();
	TEST_ASSERT_GREATER_OR_EQUAL(0, pid);
	/* child */
	if (pid == 0) {
		quick_exit(TEST_STATUS_NORETURN);

		/* Must never be reached */
		test_writeMarker(TEST_MARK_AFTER);
		_Exit(TEST_STATUS_MAIN);
	}
	/* parent */
	else {
		test_waitChild(pid, TEST_STATUS_NORETURN);

		cnt = test_readMarkers(fd, markers, 2);
		TEST_ASSERT_EQUAL_INT(0, cnt);

		close(fd);
	}
#endif
}


/*
 * "If a function registered by a call to at_quick_exit() fails to return, the remaining registered
 * functions shall not be called and the rest of the quick_exit() processing shall not be completed"
 */
TEST(stdlib_quick_exit, handler_not_returning)
{
#ifndef TEST_QUICK_EXIT_AVAILABLE
	TEST_IGNORE_MESSAGE("at_quick_exit/quick_exit is not implemented");
#else
	pid_t pid;
	int fd, markers[3], cnt;

	fd = test_createFile();

	pid = fork();
	TEST_ASSERT_GREATER_OR_EQUAL(0, pid);
	/* child */
	if (pid == 0) {
		at_quick_exit(test_quickExitFun1);
		/* Registered last, so called first - terminates the process on its own */
		at_quick_exit(test_quickExitNoReturn);
		quick_exit(TEST_STATUS_MAIN);
	}
	/* parent */
	else {
		/* The status comes from the registered function, not from quick_exit() */
		test_waitChild(pid, TEST_STATUS_HANDLER);

		cnt = test_readMarkers(fd, markers, 3);
		/* The remaining registered function has not been called */
		TEST_ASSERT_EQUAL_INT(1, cnt);
		TEST_ASSERT_EQUAL_INT(TEST_MARK_QUICK2, markers[0]);

		close(fd);
	}
#endif
}


/*
 * "Finally, the quick_exit() function shall terminate the process as if by a call to _Exit(status)"
 * - only the least significant 8 bits of status are available to the waiting parent
 */
TEST(stdlib_quick_exit, status_vals)
{
#ifndef TEST_QUICK_EXIT_AVAILABLE
	TEST_IGNORE_MESSAGE("at_quick_exit/quick_exit is not implemented");
#else
	pid_t pid;
	int i;
	/* The 3 most significant bytes have to be cut off */
	const int vals[] = { EXIT_SUCCESS, EXIT_FAILURE, 0, 1, 255, 0x1 << 8, (0x1 << 16) + 1, (0x1 << 24) + 2 };
	const int expected[] = { EXIT_SUCCESS & 0377, EXIT_FAILURE & 0377, 0, 1, 255, 0, 1, 2 };

	for (i = 0; i < (int)(sizeof(vals) / sizeof(vals[0])); i++) {
		pid = fork();
		TEST_ASSERT_GREATER_OR_EQUAL(0, pid);
		/* child */
		if (pid == 0) {
			quick_exit(vals[i]);
		}
		/* parent */
		else {
			test_waitChild(pid, expected[i]);
		}
	}
#endif
}


/* quick_exit() terminates as if by _Exit(), so open streams shall not be flushed */
TEST(stdlib_quick_exit, no_flush)
{
#ifndef TEST_QUICK_EXIT_AVAILABLE
	TEST_IGNORE_MESSAGE("at_quick_exit/quick_exit is not implemented");
#else
	pid_t pid;
	int fd;
	FILE *f;

	f = fopen(TEST_QUICK_EXIT_PATH, "w+");
	TEST_ASSERT_NOT_NULL(f);
	fd = open(TEST_QUICK_EXIT_PATH, O_RDWR);
	TEST_ASSERT_GREATER_OR_EQUAL(0, fd);
	/* Check file is empty */
	TEST_ASSERT_EQUAL_INT(0, lseek(fd, 0, SEEK_END));

	pid = fork();
	TEST_ASSERT_GREATER_OR_EQUAL(0, pid);
	/* child */
	if (pid == 0) {
		fprintf(f, TEST_QUICK_EXIT_STR);
		quick_exit(TEST_STATUS_MAIN);
	}
	/* parent */
	else {
		test_waitChild(pid, TEST_STATUS_MAIN);

		/* If the buffered data had been flushed, the file length would increase */
		TEST_ASSERT_EQUAL_INT(0, lseek(fd, 0, SEEK_END));

		fclose(f);
		close(fd);
	}
#endif
}


/* "The quick_exit() function shall cause normal process termination to occur" - also when called from a thread */
TEST(stdlib_quick_exit, called_from_thread)
{
#ifndef TEST_QUICK_EXIT_AVAILABLE
	TEST_IGNORE_MESSAGE("at_quick_exit/quick_exit is not implemented");
#else
	pid_t pid;
	int fd, i, markers[2], cnt;

	fd = test_createFile();

	pid = fork();
	TEST_ASSERT_GREATER_OR_EQUAL(0, pid);
	/* child */
	if (pid == 0) {
		pthread_t thread;

		at_quick_exit(test_quickExitFun1);

		if (pthread_create(&thread, NULL, test_quickExitThread, NULL) != 0) {
			_Exit(TEST_STATUS_MAIN);
		}

		/* The whole process is expected to be terminated by the thread */
		for (i = 0; i < 500; i++) {
			usleep(10000);
		}

		/* Not terminated within 5 seconds - fail with a different status */
		_Exit(TEST_STATUS_MAIN);
	}
	/* parent */
	else {
		test_waitChild(pid, TEST_STATUS_THREAD);

		cnt = test_readMarkers(fd, markers, 2);
		TEST_ASSERT_EQUAL_INT(1, cnt);
		TEST_ASSERT_EQUAL_INT(TEST_MARK_QUICK1, markers[0]);

		close(fd);
	}
#endif
}


#pragma GCC diagnostic pop


TEST_GROUP_RUNNER(stdlib_at_quick_exit)
{
	RUN_TEST_CASE(stdlib_at_quick_exit, returns_zero);
	RUN_TEST_CASE(stdlib_at_quick_exit, min_32_registrations);
	RUN_TEST_CASE(stdlib_at_quick_exit, not_called_by_exit);
	RUN_TEST_CASE(stdlib_at_quick_exit, not_called_by_Exit);
}


TEST_GROUP_RUNNER(stdlib_quick_exit)
{
	RUN_TEST_CASE(stdlib_quick_exit, handlers_reverse_order);
	RUN_TEST_CASE(stdlib_quick_exit, no_atexit_handlers);
	RUN_TEST_CASE(stdlib_quick_exit, no_signal_handlers);
	RUN_TEST_CASE(stdlib_quick_exit, does_not_return);
	RUN_TEST_CASE(stdlib_quick_exit, handler_not_returning);
	RUN_TEST_CASE(stdlib_quick_exit, status_vals);
	RUN_TEST_CASE(stdlib_quick_exit, no_flush);
	RUN_TEST_CASE(stdlib_quick_exit, called_from_thread);
}
