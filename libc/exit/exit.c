/*
 * Phoenix-RTOS
 *
 *    POSIX.1-2017 standard library functions tests
 *    HEADER:
 *    - stdlib.h
 *    - unistd.h
 *    TESTED:
 *    - exit()
 *    - _exit() (_Exit equivalent)
 *    - atexit()
 *
 * Copyright 2023 Phoenix Systems
 * Author: Adam Debek
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>

#include <unity_fixture.h>

#define CHANGE_TEST_GROUP(to, from, case_name) \
	void TEST_##to##_##case_name##_run(void); \
	void TEST_##to##_##case_name##_run(void) \
	{ \
		UnityTestRunner(TEST_##from##_SETUP, \
			TEST_##from##_##case_name##_, \
			TEST_##from##_TEAR_DOWN, \
			"TEST(" #to ", " #case_name ")", \
			TEST_GROUP_##to, #case_name, \
			__FILE__, __LINE__); \
	}


#define TEST_EXIT_PATH      "exit_test_file"
#define TEST_EXIT_STR       "test123"
#define TEST_EXIT_DUMMY_VAL 12345678

/* Disable warnings caused by no checking write() return value, since ASSERT cannot be used in child process */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"

/*
	Aspects required by POSIX, which wasn't tested:
		- all open named semaphores in the calling process shall be closed
		  -> https://github.com/phoenix-rtos/phoenix-rtos-project/issues/806

		- threads terminated by a call to _Exit() or _exit() shall not invoke their cancellation cleanup handlers
		  -> https://github.com/phoenix-rtos/phoenix-rtos-project/issues/827

		- if the exit of the process causes a process group to become orphaned, and if any member of the newly-orphaned
		  process group is stopped, then a SIGHUP signal followed by a SIGCONT signal shall be sent to each process in
		  the newly-orphaned process group -> https://github.com/phoenix-rtos/phoenix-rtos-project/issues/809

		- If the process is a controlling process, the SIGHUP signal shall be sent to each process in the foreground
		  process group of the controlling terminal belonging to the calling process.
		  If the process is a controlling process, the controlling terminal associated with the session shall be
		  disassociated from the session, allowing it to be acquired by a new controlling process
		  -> Phoenix-RTOS doesn't implement control of terminal by session leader with that being said controlling
		  process is not supported

		- The full value of status shall be available from waitid() and in the siginfo_t passed to a signal handler
		  for SIGCHLD -> https://github.com/phoenix-rtos/phoenix-rtos-project/issues/844,
		  https://github.com/phoenix-rtos/phoenix-rtos-project/issues/845

		- All of the file descriptors, directory streams, conversion descriptors, and message catalog descriptors
		  open in the calling process shall be closed. -> Directory streams closure after exit not possible to check.
		  Same with conversion and message catalog descriptors (not available on Phoenix-RTOS)

		- Memory mappings that were created in the process shall be unmapped before the process is destroyed
		  -> Not possible to test
 */


static struct {
	volatile int test_handlerFlag;
	volatile int test_threadWait;
	void (*test_exitPtr)(int status);
} test_common;


struct test_ThreadArgs {
	volatile int retWaitThr;
	volatile int errnoThr;
	pid_t pid;
	pthread_mutex_t mutex;
};


/* SIGCHLD signal handler */
static void test_sigchldHandler(int signum)
{
	struct sigaction sa;

	sa.sa_handler = SIG_DFL;  // Set the handler to default action
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	TEST_ASSERT_EQUAL_INT(0, sigaction(SIGCHLD, &sa, NULL));

	/* Change value of variable to confirm that handler has been invoked */
	test_common.test_handlerFlag = TEST_EXIT_DUMMY_VAL;
}


static void test_sigusrHandler(int signum)
{
	struct sigaction sa;

	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGUSR1, &sa, NULL);
}


static void test_dummyHandler(int signum)
{
	/* pause() need registered handler to be unblocked */
}


static void test_threadExitHandler(int signum)
{
	struct sigaction sa;

	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGUSR1, &sa, NULL);

	pthread_exit(NULL);
}


static void test_destructor(void *arg)
{
	creat(TEST_EXIT_PATH, DEFFILEMODE);
}


static void *test_threadFunc(void *arg)
{
	pthread_key_t key;
	int thread_specific_data = 0;

	pthread_key_create(&key, test_destructor);
	pthread_setspecific(key, (void *)&thread_specific_data);

	while (!test_common.test_threadWait) {
		;
	}

	test_common.test_exitPtr(0);

	return NULL;
}


static void *test_threadWait(void *args)
{
	int status;
	struct test_ThreadArgs *threadArgs = (struct test_ThreadArgs *)args;
	struct sigaction sa;

	/* Set handler for force exit from the thread in case of stuck in wait() or waitpid() */
	sa.sa_handler = test_threadExitHandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGUSR1, &sa, NULL);

	errno = 0;
	threadArgs->retWaitThr = wait(&status);
	threadArgs->errnoThr = errno;

	return NULL;
}


static void *test_threadWaitpid(void *args)
{
	int status;
	struct test_ThreadArgs *threadArgs = (struct test_ThreadArgs *)args;
	struct sigaction sa;

	/* Set handler for force exit from the thread in case of stuck in wait() or waitpid() */
	sa.sa_handler = test_threadExitHandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGUSR1, &sa, NULL);

	errno = 0;
	threadArgs->retWaitThr = waitpid(threadArgs->pid, &status, 0);
	threadArgs->errnoThr = errno;

	return NULL;
}


/* Functions to be registered by atexit */
static void test_atexit_fun1(void)
{
	int fd, val = 123;

	fd = open(TEST_EXIT_PATH, O_WRONLY | O_APPEND);
	write(fd, &val, sizeof(int));
	close(fd);
}


static void test_atexit_fun2(void)
{
	int fd, val = 1234;

	fd = open(TEST_EXIT_PATH, O_WRONLY | O_APPEND);
	write(fd, &val, sizeof(int));
	close(fd);
}


static void test_atexit_fun3(void)
{
	int fd, val = 12345;

	fd = open(TEST_EXIT_PATH, O_WRONLY | O_APPEND);
	write(fd, &val, sizeof(int));
	close(fd);
}


static void test_atexit_inside(void)
{
	/* test_atexit_fun3 + register test_atexit_fun2 at the start */
	int fd, val = 12345;

	atexit(test_atexit_fun2);

	fd = open(TEST_EXIT_PATH, O_WRONLY | O_APPEND);
	write(fd, &val, sizeof(int));
	close(fd);
}

static void test_no_atexit(void)
{
	creat(TEST_EXIT_PATH, DEFFILEMODE);
}


TEST_GROUP(unistd_exit);


TEST_SETUP(unistd_exit)
{
	test_common.test_handlerFlag = 0;
	test_common.test_threadWait = 0;
	remove(TEST_EXIT_PATH);
}


TEST_TEAR_DOWN(unistd_exit)
{
}


TEST(unistd_exit, status_vals)
{
	/* Return all possible least significant byte values and check whether wait() works */
	pid_t pid;
	int i;
	int val[3];
	/*
	 * Only the least significant 8 bits shall be available to a waiting parent process,
	 * 3 most significant bytes should be cut off so returning val[0] should resolve to returning 0, val[1] to 1 and val[2] to 2
	 */
	val[0] = 0x1 << 8;
	val[1] = (0x1 << 16) + 1;
	val[2] = (0x1 << 24) + 2;

	for (i = 0; i < 256; i++) {
		pid = fork();
		TEST_ASSERT_GREATER_OR_EQUAL(0, pid);
		/* child */
		if (pid == 0) {
			test_common.test_exitPtr(i);
		}
		/* parent */
		else {
			int status, ret;

			ret = wait(&status);
			TEST_ASSERT_EQUAL_INT(pid, ret);
			TEST_ASSERT_EQUAL_INT(i, WEXITSTATUS(status));
		}
	}
	/* Some exceeding values */
	for (i = 0; i < 3; i++) {
		pid = fork();
		TEST_ASSERT_GREATER_OR_EQUAL(0, pid);
		/* child */
		if (pid == 0) {
			test_common.test_exitPtr(val[i]);
		}
		/* parent */
		else {
			int status, ret;

			ret = wait(&status);
			TEST_ASSERT_EQUAL_INT(pid, ret);
			TEST_ASSERT_EQUAL_INT(i, WEXITSTATUS(status));
		}
	}
}


TEST(unistd_exit, exit_status_waitpid)
{
	/* Check if after _exit() waitpid() works */
	pid_t pid;

	pid = fork();
	TEST_ASSERT_GREATER_OR_EQUAL(0, pid);
	/* child */
	if (pid == 0) {
		test_common.test_exitPtr(EXIT_SUCCESS);
	}
	/* parent */
	else {
		int status, ret;

		ret = waitpid(pid, &status, 0);
		TEST_ASSERT_EQUAL_INT(pid, ret);
		TEST_ASSERT_EQUAL_INT(EXIT_SUCCESS, WEXITSTATUS(status));

		errno = 0;
		ret = waitpid(pid, &status, 0);
		TEST_ASSERT_EQUAL_INT(-1, ret);
		TEST_ASSERT_EQUAL_INT(ECHILD, errno);
	}
}


TEST(unistd_exit, chk_if_exits)
{
	/* Check if process terminates after _exit call */
	pid_t pid;

	pid = fork();
	TEST_ASSERT_GREATER_OR_EQUAL(0, pid);
	/* child */
	if (pid == 0) {
		test_common.test_exitPtr(EXIT_SUCCESS);
	}
	/* parent */
	else {
		int status, ret;

		ret = wait(&status);
		TEST_ASSERT_EQUAL_INT(pid, ret);
		TEST_ASSERT_EQUAL_INT(EXIT_SUCCESS, WEXITSTATUS(status));
		/* Try to kill process which exited */
		errno = 0;
		ret = kill(pid, SIGKILL);
		TEST_ASSERT_EQUAL_INT(-1, ret);
		TEST_ASSERT_EQUAL_INT(ESRCH, errno);
	}
}


TEST(unistd_exit, unblock_thread_wait)
{
#ifdef phoenix
	TEST_IGNORE_MESSAGE("#869 issue");
#endif
	/* Check if only one waiting thread (wait()) is unblocked after child terminates */
	pid_t pid;
	pthread_t tid1, tid2;
	struct sigaction sa;

	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGUSR1, &sa, NULL);

	pid = fork();
	TEST_ASSERT_GREATER_OR_EQUAL(0, pid);
	/* child */
	if (pid == 0) {
		struct sigaction sa;

		sa.sa_handler = test_dummyHandler;
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = 0;
		sigaction(SIGUSR1, &sa, NULL);

		pause();
		test_common.test_exitPtr(EXIT_SUCCESS);
	}
	/* parent */
	else {
		struct test_ThreadArgs argsThr1, argsThr2;
		time_t start, curr;
		double timeout = 3.0;
		int ret;

		memset((void *)&argsThr1, 0, sizeof(struct test_ThreadArgs));
		memset((void *)&argsThr2, 0, sizeof(struct test_ThreadArgs));

		ret = pthread_create(&tid1, NULL, test_threadWait, (void *)&argsThr1);
		TEST_ASSERT_EQUAL_INT(0, ret);
		ret = pthread_create(&tid2, NULL, test_threadWait, (void *)&argsThr2);
		TEST_ASSERT_EQUAL_INT(0, ret);

		while (argsThr1.retWaitThr != pid && argsThr2.retWaitThr != pid) {
			kill(pid, SIGUSR1);
			usleep(20000);
		}

		start = curr = time(NULL);
		if (argsThr1.retWaitThr == pid) {
			/* Wait() in second thread should return -1 and set errno to ECHILD */
			while (difftime(curr, start) <= timeout) {
				curr = time(NULL);
				if (argsThr2.retWaitThr == -1 && argsThr2.errnoThr == ECHILD) {
					TEST_PASS();
				}
				else if (argsThr2.retWaitThr == -1 && argsThr2.errnoThr == EINTR) {
					TEST_FAIL_MESSAGE("Error: Thread received unexpected signal");
				}
				else if (argsThr2.retWaitThr > 0) {
					TEST_FAIL_MESSAGE("Error: More than 1 thread unblocked");
				}
			}
			pthread_kill(tid2, SIGUSR1);
			TEST_FAIL_MESSAGE("Error: Second thread still waiting");
		}
		else {
			/* Wait() in first thread should return -1 and set errno to ECHILD */
			while (difftime(curr, start) <= timeout) {
				curr = time(NULL);
				if (argsThr1.retWaitThr == -1 && argsThr1.errnoThr == ECHILD) {
					TEST_PASS();
				}
				else if (argsThr1.retWaitThr == -1 && argsThr1.errnoThr == EINTR) {
					TEST_FAIL_MESSAGE("Error: Thread received unexpected signal");
				}
				else if (argsThr1.retWaitThr > 0) {
					TEST_FAIL_MESSAGE("Error: More than 1 thread unblocked");
				}
			}
			pthread_kill(tid1, SIGUSR1);
			TEST_FAIL_MESSAGE("Error: First thread still waiting");
		}
		pthread_join(tid1, NULL);
		pthread_join(tid2, NULL);
	}
}


TEST(unistd_exit, unblock_thread_waitpid)
{
#ifdef phoenix
	TEST_IGNORE_MESSAGE("#869 issue");
#endif
	/* Check if only one waiting thread (waitpid()) is unblocked after child terminates */
	pid_t pid;
	pthread_t tid1, tid2;
	struct sigaction sa;

	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGUSR1, &sa, NULL);

	pid = fork();
	TEST_ASSERT_GREATER_OR_EQUAL(0, pid);
	/* child */
	if (pid == 0) {
		struct sigaction sa;

		sa.sa_handler = test_dummyHandler;
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = 0;
		sigaction(SIGUSR1, &sa, NULL);

		pause();
		test_common.test_exitPtr(EXIT_SUCCESS);
	}
	/* parent */
	else {
		struct test_ThreadArgs argsThr1, argsThr2;
		time_t start, curr;
		double timeout = 3.0;
		int ret;

		memset((void *)&argsThr1, 0, sizeof(struct test_ThreadArgs));
		memset((void *)&argsThr2, 0, sizeof(struct test_ThreadArgs));

		argsThr1.pid = argsThr2.pid = pid;

		ret = pthread_create(&tid1, NULL, test_threadWaitpid, (void *)&argsThr1);
		TEST_ASSERT_EQUAL_INT(0, ret);
		ret = pthread_create(&tid2, NULL, test_threadWaitpid, (void *)&argsThr2);
		TEST_ASSERT_EQUAL_INT(0, ret);

		while (argsThr1.retWaitThr != pid && argsThr2.retWaitThr != pid) {
			kill(pid, SIGUSR1);
			usleep(20000);
		}

		start = curr = time(NULL);
		if (argsThr1.retWaitThr == pid) {
			/* Waitpid() in second thread should return -1 and set errno to ECHILD */
			while (difftime(curr, start) <= timeout) {
				curr = time(NULL);
				if (argsThr2.retWaitThr == -1 && argsThr2.errnoThr == ECHILD) {
					TEST_PASS();
				}
				else if (argsThr2.retWaitThr == -1 && argsThr2.errnoThr == EINTR) {
					TEST_FAIL_MESSAGE("Error: Thread received unexpected signal");
				}
				else if (argsThr2.retWaitThr > 0) {
					TEST_FAIL_MESSAGE("Error: More than 1 thread unblocked");
				}
			}
			pthread_kill(tid2, SIGUSR1);
			TEST_FAIL_MESSAGE("Error: Second thread still waiting");
		}
		else {
			/* Waitpid() in first thread should return -1 and set errno to ECHILD */
			while (difftime(curr, start) <= timeout) {
				curr = time(NULL);
				if (argsThr1.retWaitThr == -1 && argsThr1.errnoThr == ECHILD) {
					TEST_PASS();
				}
				else if (argsThr1.retWaitThr == -1 && argsThr1.errnoThr == EINTR) {
					TEST_FAIL_MESSAGE("Error: Thread received unexpected signal");
				}
				else if (argsThr1.retWaitThr > 0) {
					TEST_FAIL_MESSAGE("Error: More than 1 thread unblocked");
				}
			}
			pthread_kill(tid1, SIGUSR1);
			TEST_FAIL_MESSAGE("Error: First thread still waiting");
		}
		pthread_join(tid1, NULL);
		pthread_join(tid2, NULL);
	}
}


TEST(unistd_exit, close_streams)
{
	/*
	 * Open pipe, when child exits one side of pipe should be closed,
	 * so that call to write should fail.
	 */
	pid_t pid;
	int pipefd[2];

	TEST_ASSERT_EQUAL_INT(0, pipe(pipefd));
	pid = fork();
	TEST_ASSERT_GREATER_OR_EQUAL(0, pid);
	/* child */
	if (pid == 0) {
		test_common.test_exitPtr(EXIT_SUCCESS);
	}
	/* parent */
	else {
		int status, ret;

		ret = wait(&status);
		TEST_ASSERT_EQUAL_INT(pid, ret);
		TEST_ASSERT_EQUAL_INT(EXIT_SUCCESS, WEXITSTATUS(status));

		close(pipefd[0]); /* close read pipe end */
		errno = 0;
		ret = write(pipefd[1], TEST_EXIT_STR, sizeof(TEST_EXIT_STR));
		TEST_ASSERT_EQUAL_INT(-1, ret);
		TEST_ASSERT_EQUAL_INT(EPIPE, errno);
		close(pipefd[1]); /* close write pipe end */
	}
}


TEST(unistd_exit, orphaned_child)
{
#ifndef phoenix
	TEST_IGNORE_MESSAGE("Lack of init system in docker container");
#endif
	/* Test if parent _exit affect child process */
	pid_t pid;
	int pipefd[2];
	struct sigaction sa;

	sa.sa_handler = test_dummyHandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGUSR1, &sa, NULL);

	/* Pipe needed for communication between grandparent and parent (can't use asserts in child) */
	TEST_ASSERT_EQUAL_INT(0, pipe(pipefd));

	pid = fork();
	TEST_ASSERT_GREATER_OR_EQUAL(0, pid);
	if (pid == 0) {
		pid = fork();
		/* grandchild */
		if (pid == 0) {
			/* Grandchild doesn't need pipe */
			close(pipefd[0]);
			close(pipefd[1]);

			pause();

			creat(TEST_EXIT_PATH, DEFFILEMODE);
			test_common.test_exitPtr(EXIT_SUCCESS);
		}
		/* parent */
		else {
			/* Write fork return value from parent to grandparent */
			close(pipefd[0]);
			write(pipefd[1], &pid, sizeof(pid_t));
			close(pipefd[1]);
			/* Parent exits right away */
			test_common.test_exitPtr(EXIT_SUCCESS);
		}
	}
	/* grandparent */
	else {
		int status, ret, try = 0;

		close(pipefd[1]);
		ret = wait(&status);
		TEST_ASSERT_EQUAL_INT(pid, ret);
		TEST_ASSERT_EQUAL_INT(EXIT_SUCCESS, WEXITSTATUS(status));

		TEST_ASSERT_EQUAL_INT(sizeof(pid_t), read(pipefd[0], &pid, sizeof(pid_t)));
		/* Check if fork in parent succeeded */
		TEST_ASSERT_NOT_EQUAL_INT(-1, pid);
		close(pipefd[0]);

		/* Check if file exists, if so child was unblocked */
		TEST_ASSERT_EQUAL_INT(-1, access(TEST_EXIT_PATH, F_OK));

		/* Send signal to grandchild indicating that parent exited */
		TEST_ASSERT_EQUAL_INT(0, kill(pid, SIGUSR1));

		while (kill(pid, SIGUSR1) == 0 && try <= 100) {
			usleep(10000);
			try++;
		}

		if (try > 100) {
			kill(pid, SIGKILL);
			TEST_FAIL_MESSAGE("Grandchild process couldn't exit");
		}

		TEST_ASSERT_EQUAL_INT(0, access(TEST_EXIT_PATH, F_OK));

		remove(TEST_EXIT_PATH);
	}
}


TEST(unistd_exit, new_parent_id)
{
#ifndef phoenix
	TEST_IGNORE_MESSAGE("Lack of init system in docker container");
#endif
	/* Test that child acquire new parent ID */
	pid_t pid;
	int pipefd[2];
	struct sigaction sa;

	sa.sa_handler = test_dummyHandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGUSR1, &sa, NULL);

	/* Pipe needed for communication between grandparent and grandchild (can't use asserts in child) */
	TEST_ASSERT_EQUAL_INT(0, pipe(pipefd));
	pid = fork();
	TEST_ASSERT_GREATER_OR_EQUAL(0, pid);
	if (pid == 0) {
		pid = fork();
		/* grandchild */
		if (pid == 0) {
			pid_t old_ppid, new_ppid;

			close(pipefd[0]);
			old_ppid = getppid();

			/* Wake parent */
			while (getppid() == old_ppid) {
				kill(old_ppid, SIGUSR1);
				usleep(10000);
			}

			pause();

			/* Parent process id should be 1 (init pid) */
			new_ppid = getppid();
			write(pipefd[1], &old_ppid, sizeof(pid_t));
			write(pipefd[1], &new_ppid, sizeof(pid_t));
			close(pipefd[1]);
			test_common.test_exitPtr(EXIT_SUCCESS);
		}
		/* parent */
		else {
			pause();
			/* Write fork return value from parent to grandparent */
			close(pipefd[0]);
			write(pipefd[1], &pid, sizeof(pid_t));
			close(pipefd[1]);
			test_common.test_exitPtr(EXIT_SUCCESS);
		}
	}
	/* grandparent */
	else {
		pid_t old_ppid, new_ppid;
		int status, ret, try = 0;

		close(pipefd[1]);
		ret = wait(&status);
		TEST_ASSERT_EQUAL_INT(pid, ret);
		TEST_ASSERT_EQUAL_INT(EXIT_SUCCESS, WEXITSTATUS(status));

		/* Get grandchild pid */
		TEST_ASSERT_EQUAL_INT(sizeof(pid_t), read(pipefd[0], &pid, sizeof(pid_t)));
		TEST_ASSERT_NOT_EQUAL_INT(-1, pid);

		/* Loop until grandchild terminates or 100 tries exceeds */
		while (kill(pid, SIGUSR1) == 0 && try <= 100) {
			usleep(10000);
			try++;
		}

		if (try > 100) {
			kill(pid, SIGKILL);
			TEST_FAIL_MESSAGE("Grandchild process couldn't exit");
		}

		/* Get grandchild its old parnet pid and its new parent pid */
		TEST_ASSERT_EQUAL_INT(sizeof(pid_t), read(pipefd[0], &old_ppid, sizeof(pid_t)));
		TEST_ASSERT_EQUAL_INT(sizeof(pid_t), read(pipefd[0], &new_ppid, sizeof(pid_t)));

		TEST_ASSERT_NOT_EQUAL_INT(old_ppid, new_ppid);
		close(pipefd[0]);
	}
}


TEST(unistd_exit, SIGCHLD_sent)
{
	/* Test that SIGCHILD signal is sent after child exits */
	pid_t pid;
	struct sigaction sa;

	sa.sa_handler = test_sigchldHandler;
	TEST_ASSERT_EQUAL_INT(0, sigemptyset(&sa.sa_mask));
	sa.sa_flags = 0;
	TEST_ASSERT_EQUAL_INT(0, sigaction(SIGCHLD, &sa, NULL));

	/* Check handlerFlag has initial value */
	TEST_ASSERT_EQUAL_INT(0, test_common.test_handlerFlag);

	pid = fork();
	TEST_ASSERT_GREATER_OR_EQUAL(0, pid);
	/* child */
	if (pid == 0) {
		/* Exit right away */
		test_common.test_exitPtr(EXIT_SUCCESS);
	}
	/* parent */
	else {
		time_t start, curr;
		double timeout = 3.0;

		start = curr = time(NULL);
		while (difftime(curr, start) <= timeout) {
			curr = time(NULL);
			if (test_common.test_handlerFlag != 0) {
				break;
			}
		}

		TEST_ASSERT_EQUAL_INT(TEST_EXIT_DUMMY_VAL, test_common.test_handlerFlag);

		int status;
		/* Reap a child */
		TEST_ASSERT_EQUAL_INT(pid, wait(&status));
	}
}


TEST(unistd_exit, per_thread_data_destructors)
{
	/* Test that per thread data destructors are not invoked */
	pid_t pid;
	int ret;
	int pipefd[2];

	/* Pipe needed for communication between parent and child (can't use asserts in child) */
	TEST_ASSERT_EQUAL_INT(0, pipe(pipefd));
	pid = fork();
	TEST_ASSERT_GREATER_OR_EQUAL(0, pid);
	/* child */
	if (pid == 0) {
		pthread_t tid;

		close(pipefd[0]);

		ret = pthread_create(&tid, NULL, test_threadFunc, NULL);
		write(pipefd[1], &ret, sizeof(int));
		close(pipefd[1]);
		test_common.test_threadWait = 1;
		pthread_join(tid, NULL);
	}
	/* parent */
	else {
		int status;

		ret = wait(&status);
		TEST_ASSERT_EQUAL_INT(pid, ret);
		TEST_ASSERT_EQUAL_INT(EXIT_SUCCESS, WEXITSTATUS(status));

		close(pipefd[1]);
		/* Get return value from pthread_create() */
		TEST_ASSERT_EQUAL_INT(sizeof(int), read(pipefd[0], &ret, sizeof(int)));
		TEST_ASSERT_EQUAL_INT(0, ret);

		TEST_ASSERT_EQUAL_INT(-1, access(TEST_EXIT_PATH, F_OK));

		close(pipefd[0]);
	}
}


TEST(unistd_exit, no_atexit)
{
	/* Test that _exit() do NOT invoke any funcs registered by atexit */
	pid_t pid;

	pid = fork();
	TEST_ASSERT_GREATER_OR_EQUAL(0, pid);
	/* child */
	if (pid == 0) {
		atexit(test_no_atexit);
		test_common.test_exitPtr(EXIT_SUCCESS);
	}
	/* parent */
	else {
		int status, ret;

		ret = wait(&status);
		TEST_ASSERT_EQUAL_INT(pid, ret);
		TEST_ASSERT_EQUAL_INT(EXIT_SUCCESS, WEXITSTATUS(status));

		TEST_ASSERT_EQUAL_INT(-1, access(TEST_EXIT_PATH, F_OK));

		remove(TEST_EXIT_PATH);
	}
}


TEST(unistd_exit, no_flush)
{
	/* Test that _exit() do NOT forces unwritten buffered data to be flushed */
	pid_t pid;
	int fd;

	FILE *f = fopen(TEST_EXIT_PATH, "w+");
	TEST_ASSERT_NOT_NULL(f);
	fd = open(TEST_EXIT_PATH, O_RDWR);
	TEST_ASSERT_GREATER_OR_EQUAL(0, fd);
	/* Check file is empty */
	TEST_ASSERT_EQUAL_INT(0, lseek(fd, 0, SEEK_END));

	pid = fork();
	TEST_ASSERT_GREATER_OR_EQUAL(0, pid);
	/* child */
	if (pid == 0) {
		fprintf(f, TEST_EXIT_STR);
		test_common.test_exitPtr(EXIT_SUCCESS);
	}
	/* parent */
	else {
		int status, ret;

		ret = wait(&status);
		TEST_ASSERT_EQUAL_INT(pid, ret);
		TEST_ASSERT_EQUAL_INT(EXIT_SUCCESS, WEXITSTATUS(status));

		/* If buffered data was flushed file length should increase */
		TEST_ASSERT_NOT_EQUAL_INT(strlen(TEST_EXIT_STR), lseek(fd, 0, SEEK_END));

		fclose(f);
		close(fd);
		remove(TEST_EXIT_PATH);
	}
}


TEST(unistd_exit, no_handler)
{
	/* Test that handler is not invoked when _exit */
	pid_t pid;

	/* Clear handlerFlag, if handler invoked some dummy value would be set */
	test_common.test_handlerFlag = 0;

	pid = fork();
	TEST_ASSERT_GREATER_OR_EQUAL(0, pid);
	/* child */
	if (pid == 0) {
		struct sigaction sa;

		sa.sa_handler = test_sigusrHandler;
		TEST_ASSERT_EQUAL_INT(0, sigemptyset(&sa.sa_mask));
		sa.sa_flags = 0;
		TEST_ASSERT_EQUAL_INT(0, sigaction(SIGUSR1, &sa, NULL));

		test_common.test_exitPtr(EXIT_SUCCESS);
	}
	/* parent */
	else {
		int status, ret;

		ret = wait(&status);
		TEST_ASSERT_EQUAL_INT(pid, ret);
		TEST_ASSERT_EQUAL_INT(EXIT_SUCCESS, WEXITSTATUS(status));

		TEST_ASSERT_EQUAL_INT(0, test_common.test_handlerFlag);
	}
}


TEST_GROUP(stdlib_exit);


TEST_SETUP(stdlib_exit)
{
	remove(TEST_EXIT_PATH);
}


TEST_TEAR_DOWN(stdlib_exit)
{
}


TEST(stdlib_exit, stream_flush)
{
	/* Test that exit() forces unwritten buffered data to be flushed */
	pid_t pid;
	int fd;

	FILE *f = fopen(TEST_EXIT_PATH, "w+");
	TEST_ASSERT_NOT_NULL(f);
	fd = open(TEST_EXIT_PATH, O_RDWR);
	TEST_ASSERT_GREATER_OR_EQUAL(0, fd);
	/* Check file is empty */
	TEST_ASSERT_EQUAL_INT(0, lseek(fd, 0, SEEK_END));

	pid = fork();
	TEST_ASSERT_GREATER_OR_EQUAL(0, pid);
	/* child */
	if (pid == 0) {
		fprintf(f, TEST_EXIT_STR);
		exit(EXIT_SUCCESS);
	}
	/* parent */
	else {
		int status, ret;

		ret = wait(&status);
		TEST_ASSERT_EQUAL_INT(pid, ret);
		TEST_ASSERT_EQUAL_INT(EXIT_SUCCESS, WEXITSTATUS(status));

		/* If buffered data was flushed file length should increase */
		TEST_ASSERT_EQUAL_INT(strlen(TEST_EXIT_STR), lseek(fd, 0, SEEK_END));

		fclose(f);
		close(fd);
		remove(TEST_EXIT_PATH);
	}
}


TEST(stdlib_exit, atexit_few_calls)
{
	/* Test that exit() invokes funcs registered by atexit */
	pid_t pid;
	int fd;

	fd = open(TEST_EXIT_PATH, O_RDWR | O_CREAT | O_TRUNC, S_IFREG | DEFFILEMODE);
	TEST_ASSERT_NOT_EQUAL_INT(-1, fd);

	pid = fork();
	TEST_ASSERT_GREATER_OR_EQUAL(0, pid);
	/* child */
	if (pid == 0) {
		atexit(test_atexit_fun1);
		atexit(test_atexit_fun2);
		atexit(test_atexit_fun3);
		exit(EXIT_SUCCESS);
	}
	/* parent */
	else {
		int status, ret, buf;

		ret = wait(&status);
		TEST_ASSERT_EQUAL_INT(pid, ret);
		TEST_ASSERT_EQUAL_INT(EXIT_SUCCESS, WEXITSTATUS(status));
		TEST_ASSERT_EQUAL_INT(0, lseek(fd, 0, SEEK_SET));

		TEST_ASSERT_EQUAL_INT(sizeof(int), read(fd, &buf, sizeof(int)));
		TEST_ASSERT_EQUAL_INT(12345, buf);
		TEST_ASSERT_EQUAL_INT(sizeof(int), read(fd, &buf, sizeof(int)));
		TEST_ASSERT_EQUAL_INT(1234, buf);
		TEST_ASSERT_EQUAL_INT(sizeof(int), read(fd, &buf, sizeof(int)));
		TEST_ASSERT_EQUAL_INT(123, buf);

		close(fd);
		remove(TEST_EXIT_PATH);
	}
}


TEST(stdlib_exit, atexit_register_inside)
{
	/* Test that func registered inside previously registered funcs are invoked */
	pid_t pid;
	int fd;

	fd = open(TEST_EXIT_PATH, O_RDONLY | O_CREAT | O_TRUNC, S_IFREG | DEFFILEMODE);
	TEST_ASSERT_NOT_EQUAL_INT(-1, fd);

	pid = fork();
	TEST_ASSERT_GREATER_OR_EQUAL(0, pid);
	/* child */
	if (pid == 0) {
		atexit(test_atexit_fun1);
		atexit(test_atexit_inside);
		exit(EXIT_SUCCESS);
	}
	/* parent */
	else {
		int status, ret, buf;

		ret = wait(&status);
		TEST_ASSERT_EQUAL_INT(pid, ret);
		TEST_ASSERT_EQUAL_INT(EXIT_SUCCESS, WEXITSTATUS(status));
		TEST_ASSERT_EQUAL_INT(0, lseek(fd, 0, SEEK_SET));

		TEST_ASSERT_EQUAL_INT(sizeof(int), read(fd, &buf, sizeof(int)));
		TEST_ASSERT_EQUAL_INT(12345, buf);
		TEST_ASSERT_EQUAL_INT(sizeof(int), read(fd, &buf, sizeof(int)));
		TEST_ASSERT_EQUAL_INT(1234, buf);
		TEST_ASSERT_EQUAL_INT(sizeof(int), read(fd, &buf, sizeof(int)));
		TEST_ASSERT_EQUAL_INT(123, buf);

		close(fd);
		remove(TEST_EXIT_PATH);
	}
}


TEST(stdlib_exit, atexit_two_nodes)
{
	/* Test exit calling all registered functions with more than 1 node */
#ifndef __phoenix__
	/* Phoenix-specific test case based on the implementation */
	TEST_IGNORE();
#else
	pid_t pid;
	int i, fd, nodeSize = 32;

	fd = open(TEST_EXIT_PATH, O_RDWR | O_CREAT | O_TRUNC, S_IFREG | DEFFILEMODE);
	TEST_ASSERT_NOT_EQUAL_INT(-1, fd);

	pid = fork();
	TEST_ASSERT_GREATER_OR_EQUAL(0, pid);
	/* child */
	if (pid == 0) {
		int ret;

		for (i = 1; i < nodeSize; i++) {
			atexit(test_atexit_fun1);
		}
		atexit(test_atexit_fun2);
		/* If 33 functions are registered then  new node is being allocated - check return value then */
		ret = atexit(test_atexit_fun1);
		write(fd, &ret, sizeof(int));

		close(fd);
		exit(EXIT_SUCCESS);
	}
	/* parent */
	else {
		int status, ret, buf;

		ret = wait(&status);
		TEST_ASSERT_EQUAL_INT(pid, ret);
		TEST_ASSERT_EQUAL_INT(EXIT_SUCCESS, WEXITSTATUS(status));

		TEST_ASSERT_EQUAL_INT(0, lseek(fd, 0, SEEK_SET));

		/* Check if memory for atexit node is available */
		TEST_ASSERT_EQUAL_INT(sizeof(int), read(fd, &ret, sizeof(int)));
		TEST_ASSERT_EQUAL_INT(0, ret);

		TEST_ASSERT_EQUAL_INT(sizeof(int), read(fd, &buf, sizeof(int)));
		TEST_ASSERT_EQUAL_INT(123, buf);
		TEST_ASSERT_EQUAL_INT(sizeof(int), read(fd, &buf, sizeof(int)));
		TEST_ASSERT_EQUAL_INT(1234, buf);

		for (i = 1; i < nodeSize; i++) {
			TEST_ASSERT_EQUAL_INT(sizeof(int), read(fd, &buf, sizeof(int)));
			TEST_ASSERT_EQUAL_INT(123, buf);
		}

		close(fd);
		remove(TEST_EXIT_PATH);
	}
#endif
}


TEST_GROUP_RUNNER(unistd_exit)
{
	test_common.test_exitPtr = _exit;

	RUN_TEST_CASE(unistd_exit, status_vals);
	RUN_TEST_CASE(unistd_exit, exit_status_waitpid);
	RUN_TEST_CASE(unistd_exit, chk_if_exits);
	RUN_TEST_CASE(unistd_exit, unblock_thread_wait);
	RUN_TEST_CASE(unistd_exit, unblock_thread_waitpid);
	RUN_TEST_CASE(unistd_exit, close_streams);
	RUN_TEST_CASE(unistd_exit, orphaned_child);
	RUN_TEST_CASE(unistd_exit, new_parent_id);
	RUN_TEST_CASE(unistd_exit, SIGCHLD_sent);
	RUN_TEST_CASE(unistd_exit, per_thread_data_destructors);
	RUN_TEST_CASE(unistd_exit, no_atexit);
	RUN_TEST_CASE(unistd_exit, no_flush);
	RUN_TEST_CASE(unistd_exit, no_handler);
}


TEST_GROUP(unistd_Exit);
/* Changing test group name to unistd_Exit to make error messages more readable */
CHANGE_TEST_GROUP(unistd_Exit, unistd_exit, status_vals)
CHANGE_TEST_GROUP(unistd_Exit, unistd_exit, exit_status_waitpid);
CHANGE_TEST_GROUP(unistd_Exit, unistd_exit, chk_if_exits);
CHANGE_TEST_GROUP(unistd_Exit, unistd_exit, unblock_thread_wait);
CHANGE_TEST_GROUP(unistd_Exit, unistd_exit, unblock_thread_waitpid);
CHANGE_TEST_GROUP(unistd_Exit, unistd_exit, close_streams);
CHANGE_TEST_GROUP(unistd_Exit, unistd_exit, orphaned_child);
CHANGE_TEST_GROUP(unistd_Exit, unistd_exit, new_parent_id);
CHANGE_TEST_GROUP(unistd_Exit, unistd_exit, SIGCHLD_sent);
CHANGE_TEST_GROUP(unistd_Exit, unistd_exit, per_thread_data_destructors);
CHANGE_TEST_GROUP(unistd_Exit, unistd_exit, no_atexit);
CHANGE_TEST_GROUP(unistd_Exit, unistd_exit, no_flush);
CHANGE_TEST_GROUP(unistd_Exit, unistd_exit, no_handler);


TEST_GROUP_RUNNER(unistd_Exit)
{
	test_common.test_exitPtr = _Exit;

	RUN_TEST_CASE(unistd_Exit, status_vals);
	RUN_TEST_CASE(unistd_Exit, exit_status_waitpid);
	RUN_TEST_CASE(unistd_Exit, chk_if_exits);
	RUN_TEST_CASE(unistd_Exit, unblock_thread_wait);
	RUN_TEST_CASE(unistd_Exit, unblock_thread_waitpid);
	RUN_TEST_CASE(unistd_Exit, close_streams);
	RUN_TEST_CASE(unistd_Exit, orphaned_child);
	RUN_TEST_CASE(unistd_Exit, new_parent_id);
	RUN_TEST_CASE(unistd_Exit, SIGCHLD_sent);
	RUN_TEST_CASE(unistd_Exit, per_thread_data_destructors);
	RUN_TEST_CASE(unistd_Exit, no_atexit);
	RUN_TEST_CASE(unistd_Exit, no_flush);
	RUN_TEST_CASE(unistd_Exit, no_handler);
}


TEST_GROUP_RUNNER(stdlib_exit)
{
	RUN_TEST_CASE(stdlib_exit, stream_flush);
	RUN_TEST_CASE(stdlib_exit, atexit_few_calls);
	RUN_TEST_CASE(stdlib_exit, atexit_register_inside);
	RUN_TEST_CASE(stdlib_exit, atexit_two_nodes);
}


void runner(void)
{
	RUN_TEST_GROUP(unistd_exit);
	RUN_TEST_GROUP(unistd_Exit);
	RUN_TEST_GROUP(stdlib_exit);
}


int main(int argc, char *argv[])
{
	UnityMain(argc, (const char **)argv, runner);
}

#pragma GCC diagnostic push
