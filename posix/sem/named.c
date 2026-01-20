/*
 * Phoenix-RTOS
 *
 * test-posix-sem
 *
 * POSIX named semaphore tests.
 *
 * Copyright 2026 Phoenix Systems
 * Author: Michal Lach
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/threads.h>

#include <unity_fixture.h>

#define WAIT_TEST_SEMAPHORE_NAME        ("test_wait")
#define TRYWAIT_TEST_SEMAPHORE_NAME     ("test_trywait")
#define TIMEDWAIT_TEST_SEMAPHORE_NAME   ("test_timedwait")
#define FAIR_TEST_SEMAPHORE_NAME        ("test_fair")
#define NONEXISTENT_TEST_SEMAPHORE_NAME ("test_nonexistent")
#define REOPEN_TEST_SEMAPHORE_NAME      ("test_reopen")

#define FAIR_THREAD1 (1 << 0)
#define FAIR_THREAD2 (1 << 1)


struct {
	handle_t lock;
	handle_t cond;
	unsigned int flags;
} shared;


static void *wait_helper_thread_func(void *arg)
{
	sem_t *sem;

	sem = sem_open(WAIT_TEST_SEMAPHORE_NAME, 0);
	sem_wait(sem);

	mutexLock(shared.lock);
	condSignal(shared.cond);
	mutexUnlock(shared.lock);

	usleep(10000);
	sem_post(sem);
	sem_close(sem);

	return NULL;
}


static void *trywait_helper_thread_func(void *arg)
{
	sem_t *sem;

	sem = sem_open(TRYWAIT_TEST_SEMAPHORE_NAME, 0);
	sem_wait(sem);

	mutexLock(shared.lock);
	condSignal(shared.cond);
	mutexUnlock(shared.lock);

	usleep(10000);
	sem_post(sem);
	sem_close(sem);

	return NULL;
}


static void *timedwait_helper_thread_func(void *arg)
{
	sem_t *sem;

	sem = sem_open(TIMEDWAIT_TEST_SEMAPHORE_NAME, 0);
	sem_wait(sem);

	mutexLock(shared.lock);
	condSignal(shared.cond);
	mutexUnlock(shared.lock);

	sleep(10);
	sem_post(sem);
	sem_close(sem);

	return NULL;
}


static void *fair_helper_waiter1_thread_func(void *arg)
{
	sem_t *sem;

	sem = sem_open(FAIR_TEST_SEMAPHORE_NAME, 0);
	sem_wait(sem);

	mutexLock(shared.lock);
	shared.flags |= FAIR_THREAD1;
	condSignal(shared.cond);
	mutexUnlock(shared.lock);

	sem_post(sem);
	sem_close(sem);

	return NULL;
}


static void *fair_helper_waiter2_thread_func(void *arg)
{
	sem_t *sem;

	sem = sem_open(FAIR_TEST_SEMAPHORE_NAME, 0);
	sleep(2);
	sem_wait(sem);

	mutexLock(shared.lock);
	shared.flags |= FAIR_THREAD2;
	condSignal(shared.cond);
	mutexUnlock(shared.lock);

	sem_post(sem);
	sem_close(sem);

	return NULL;
}


TEST_GROUP(named);


TEST_SETUP(named)
{
	TEST_ASSERT_EQUAL(EOK, mutexCreate(&shared.lock));
	TEST_ASSERT_EQUAL(EOK, condCreate(&shared.cond));
}


TEST_TEAR_DOWN(named)
{
	TEST_ASSERT_EQUAL(EOK, resourceDestroy(shared.lock));
	TEST_ASSERT_EQUAL(EOK, resourceDestroy(shared.cond));
}


TEST(named, wait)
{
	pthread_t thread;
	sem_t *sem;
	int ret;
	int count;

	errno = EOK;

	sem = sem_open(WAIT_TEST_SEMAPHORE_NAME, O_CREAT | O_EXCL, 0, 1);
	TEST_ASSERT_EQUAL(EOK, errno);
	TEST_ASSERT_NOT_EQUAL(SEM_FAILED, sem);

	ret = pthread_create(&thread, NULL, wait_helper_thread_func, NULL);

	TEST_ASSERT_EQUAL(EOK, mutexLock(shared.lock));
	TEST_ASSERT_EQUAL(EOK, condWait(shared.cond, shared.lock, 0));
	TEST_ASSERT_EQUAL(EOK, mutexUnlock(shared.lock));

	ret = sem_wait(sem);
	TEST_ASSERT_EQUAL(EOK, errno);
	TEST_ASSERT_EQUAL(EOK, ret);

	ret = sem_getvalue(sem, &count);
	TEST_ASSERT_EQUAL(EOK, errno);
	TEST_ASSERT_EQUAL(EOK, ret);
	TEST_ASSERT_EQUAL(0, count);

	ret = sem_post(sem);
	TEST_ASSERT_EQUAL(EOK, errno);
	TEST_ASSERT_EQUAL(EOK, ret);
	usleep(100);

	ret = sem_getvalue(sem, &count);
	TEST_ASSERT_EQUAL(EOK, errno);
	TEST_ASSERT_EQUAL(EOK, ret);
	TEST_ASSERT_EQUAL(1, count);

	ret = sem_close(sem);
	TEST_ASSERT_EQUAL(EOK, errno);
	TEST_ASSERT_EQUAL(EOK, ret);

	ret = sem_unlink(WAIT_TEST_SEMAPHORE_NAME);
	TEST_ASSERT_EQUAL(EOK, errno);
	TEST_ASSERT_EQUAL(EOK, ret);

	ret = pthread_join(thread, NULL);
	TEST_ASSERT_EQUAL(0, ret);
}


TEST(named, trywait)
{
	pthread_t thread;
	sem_t *sem;
	int ret;
	int count;

	errno = EOK;

	sem = sem_open(TRYWAIT_TEST_SEMAPHORE_NAME, O_CREAT | O_EXCL, 0, 1);
	TEST_ASSERT_EQUAL(EOK, errno);
	TEST_ASSERT_NOT_EQUAL(SEM_FAILED, sem);

	ret = pthread_create(&thread, NULL, trywait_helper_thread_func, NULL);
	TEST_ASSERT_EQUAL(EOK, ret);

	TEST_ASSERT_EQUAL(EOK, mutexLock(shared.lock));
	TEST_ASSERT_EQUAL(EOK, condWait(shared.cond, shared.lock, 0));
	TEST_ASSERT_EQUAL(EOK, mutexUnlock(shared.lock));

	ret = sem_trywait(sem);
	TEST_ASSERT_EQUAL(EAGAIN, errno);
	TEST_ASSERT_EQUAL(-1, ret);

	errno = EOK;

	ret = sem_getvalue(sem, &count);
	TEST_ASSERT_EQUAL(EOK, errno);
	TEST_ASSERT_EQUAL(EOK, ret);
	TEST_ASSERT_EQUAL(0, count);

	ret = pthread_join(thread, NULL);
	TEST_ASSERT_EQUAL(0, ret);

	ret = sem_close(sem);
	TEST_ASSERT_EQUAL(EOK, errno);
	TEST_ASSERT_EQUAL(EOK, ret);

	ret = sem_unlink(TRYWAIT_TEST_SEMAPHORE_NAME);
	TEST_ASSERT_EQUAL(EOK, errno);
	TEST_ASSERT_EQUAL(EOK, ret);
}


TEST(named, timedwait)
{
	struct timespec abs_timeout;
	pthread_t thread;
	sem_t *sem;
	int ret;

	errno = EOK;

	TEST_ASSERT_EQUAL(EOK, clock_gettime(CLOCK_REALTIME, &abs_timeout));
	abs_timeout.tv_sec += 2;

	sem = sem_open(TIMEDWAIT_TEST_SEMAPHORE_NAME, O_CREAT | O_EXCL, 0, 1);
	TEST_ASSERT_EQUAL(EOK, errno);
	TEST_ASSERT_NOT_EQUAL(SEM_FAILED, sem);

	ret = pthread_create(&thread, NULL, timedwait_helper_thread_func, NULL);
	TEST_ASSERT_EQUAL(EOK, ret);

	TEST_ASSERT_EQUAL(EOK, mutexLock(shared.lock));
	TEST_ASSERT_EQUAL(EOK, condWait(shared.cond, shared.lock, 0));
	TEST_ASSERT_EQUAL(EOK, mutexUnlock(shared.lock));

	ret = sem_timedwait(sem, &abs_timeout);
	TEST_ASSERT_EQUAL(ETIMEDOUT, errno);
	TEST_ASSERT_EQUAL(-1, ret);

	ret = pthread_join(thread, NULL);
	TEST_ASSERT_EQUAL(EOK, ret);

	errno = EOK;

	ret = sem_close(sem);
	TEST_ASSERT_EQUAL(EOK, errno);
	TEST_ASSERT_EQUAL(EOK, ret);

	ret = sem_unlink(TIMEDWAIT_TEST_SEMAPHORE_NAME);
	TEST_ASSERT_EQUAL(EOK, errno);
	TEST_ASSERT_EQUAL(EOK, ret);
}


TEST(named, fair)
{
	pthread_t waiter1, waiter2;
	sem_t *sem;
	int ret;
	int flags;

	errno = EOK;

	sem = sem_open(FAIR_TEST_SEMAPHORE_NAME, O_CREAT | O_EXCL, 0, 1);
	TEST_ASSERT_EQUAL(EOK, errno);
	TEST_ASSERT_NOT_EQUAL(SEM_FAILED, sem);

	ret = sem_wait(sem);
	TEST_ASSERT_EQUAL(EOK, errno);
	TEST_ASSERT_EQUAL(EOK, ret);

	ret = pthread_create(&waiter1, NULL, fair_helper_waiter1_thread_func, NULL);
	TEST_ASSERT_EQUAL(EOK, ret);

	ret = pthread_create(&waiter2, NULL, fair_helper_waiter2_thread_func, NULL);
	TEST_ASSERT_EQUAL(EOK, ret);

	ret = sem_post(sem);
	TEST_ASSERT_EQUAL(EOK, errno);
	TEST_ASSERT_EQUAL(EOK, ret);

	TEST_ASSERT_EQUAL(EOK, mutexLock(shared.lock));

	TEST_ASSERT_EQUAL(EOK, condWait(shared.cond, shared.lock, 0));
	flags = shared.flags;

	TEST_ASSERT_EQUAL(EOK, mutexUnlock(shared.lock));

	TEST_ASSERT(((shared.flags & FAIR_THREAD1) > 0) && ((flags & FAIR_THREAD2) == 0));

	ret = pthread_join(waiter1, NULL);
	TEST_ASSERT_EQUAL(EOK, ret);

	ret = pthread_join(waiter2, NULL);
	TEST_ASSERT_EQUAL(EOK, ret);

	ret = sem_close(sem);
	TEST_ASSERT_EQUAL(EOK, errno);
	TEST_ASSERT_EQUAL(EOK, ret);

	ret = sem_unlink(FAIR_TEST_SEMAPHORE_NAME);
	TEST_ASSERT_EQUAL(EOK, errno);
	TEST_ASSERT_EQUAL(EOK, ret);
}


TEST(named, nonexistent)
{
	sem_t *sem;

	errno = EOK;

	sem = sem_open(NONEXISTENT_TEST_SEMAPHORE_NAME, 0);
	TEST_ASSERT_EQUAL(ENOENT, errno);
	TEST_ASSERT_EQUAL_PTR(SEM_FAILED, sem);
}


TEST(named, reopen)
{
	sem_t *sem;
	int ret;

	errno = EOK;

	sem = sem_open(REOPEN_TEST_SEMAPHORE_NAME, O_CREAT | O_EXCL, 0, 1);
	TEST_ASSERT_EQUAL(EOK, errno);
	TEST_ASSERT_NOT_EQUAL(SEM_FAILED, sem);

	ret = sem_close(sem);
	TEST_ASSERT_EQUAL(EOK, errno);
	TEST_ASSERT_EQUAL(EOK, ret);

	ret = sem_unlink(REOPEN_TEST_SEMAPHORE_NAME);
	TEST_ASSERT_EQUAL(EOK, errno);
	TEST_ASSERT_EQUAL(EOK, ret);

	sem = sem_open(REOPEN_TEST_SEMAPHORE_NAME, O_CREAT | O_EXCL, 0, 1);
	TEST_ASSERT_EQUAL(EOK, errno);
	TEST_ASSERT_NOT_EQUAL(SEM_FAILED, sem);

	ret = sem_close(sem);
	TEST_ASSERT_EQUAL(EOK, errno);
	TEST_ASSERT_EQUAL(EOK, ret);

	ret = sem_unlink(REOPEN_TEST_SEMAPHORE_NAME);
	TEST_ASSERT_EQUAL(EOK, errno);
	TEST_ASSERT_EQUAL(EOK, ret);
}


TEST_GROUP_RUNNER(named)
{
	RUN_TEST_CASE(named, wait);
	RUN_TEST_CASE(named, trywait);
	RUN_TEST_CASE(named, timedwait);
	RUN_TEST_CASE(named, fair);
	RUN_TEST_CASE(named, nonexistent);
	RUN_TEST_CASE(named, reopen);
}
