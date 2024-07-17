/*
 * Phoenix-RTOS
 *
 * test-sys-cond
 *
 * Test for condition variables
 *
 * Copyright 2024 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/threads.h>
#include <sys/time.h>

#include <unity_fixture.h>


#define SIGNAL_TEST_TIMEOUT    10000
#define BROADCAST_TEST_TIMEOUT 100000


typedef struct {
	useconds_t delay;
	int id;
} signal_thread_args_t;


typedef struct {
	time_t timeout;
	int id;
	int thrCount;
} worker_thread_args_t;


static struct {
	handle_t mutex;
	handle_t cond;
	handle_t readyMutex;
	handle_t readyCond;
	volatile int readyCounter;
	volatile int counter;
	volatile int thrErrors[2];
	volatile int thrTimeout[2];
	char stack[2][4096] __attribute__((aligned(8)));
} common;


/*
 * Tests for relative condWait expecting timeout are not implemented as they're unreliable
 * (depend on e.g. scheduling/system load) and may fail in some cases.
 */


/*
 *----------------------------------- TESTING THREADS -------------------------------------*
 */


static void signal_thread(void *arg)
{
	signal_thread_args_t *args = (signal_thread_args_t *)arg;
	struct timespec timeout = { .tv_sec = 0, .tv_nsec = args->delay * 1000 }, rem;

	if (args->delay != 0) {
		while (nanosleep(&timeout, &rem) < 0) {
			if (errno != EINTR) {
				common.thrErrors[args->id]++;
			}
			else {
				timeout = rem;
			}
		}
	}

	if (mutexLock(common.mutex) < 0) {
		common.thrErrors[args->id]++;
	}
	common.counter++;
	if (condSignal(common.cond) < 0) {
		common.thrErrors[args->id]++;
	}
	if (mutexUnlock(common.mutex) < 0) {
		common.thrErrors[args->id]++;
	}

	endthread();
}


static void worker_thread_signal_test(void *arg)
{
	worker_thread_args_t *args = (worker_thread_args_t *)arg;

	if (mutexLock(common.mutex) < 0) {
		common.thrErrors[args->id]++;
	}

	if (mutexLock(common.readyMutex) < 0) {
		common.thrErrors[args->id]++;
	}
	common.readyCounter++;
	if (common.readyCounter == args->thrCount) {
		if (condSignal(common.readyCond) < 0) {
			common.thrErrors[args->id]++;
		}
	}
	if (mutexUnlock(common.readyMutex) < 0) {
		common.thrErrors[args->id]++;
	}

	int err = condWait(common.cond, common.mutex, args->timeout);

	if (mutexLock(common.readyMutex) < 0) {
		common.thrErrors[args->id]++;
	}
	if (err == 0) {
		common.counter++;
	}
	else if (err == -ETIME) {
		common.thrTimeout[args->id]++;
	}
	else {
		common.thrErrors[args->id]++;
	}

	common.readyCounter++;

	/* Signal to the waiting thread that we're done */
	if (condSignal(common.readyCond) < 0) {
		common.thrErrors[args->id]++;
	}
	if (mutexUnlock(common.readyMutex) < 0) {
		common.thrErrors[args->id]++;
	}

	if (mutexUnlock(common.mutex) < 0) {
		common.thrErrors[args->id]++;
	}

	endthread();
}


static void worker_thread_broadcast_test(void *arg)
{
	worker_thread_args_t *args = (worker_thread_args_t *)arg;

	if (mutexLock(common.mutex) < 0) {
		common.thrErrors[args->id]++;
	}

	if (mutexLock(common.readyMutex)) {
		common.thrErrors[args->id]++;
	}
	common.readyCounter++;
	if (common.readyCounter == args->thrCount) {
		if (condSignal(common.readyCond) < 0) {
			common.thrErrors[args->id];
		}
	}
	if (mutexUnlock(common.readyMutex) < 0) {
		common.thrErrors[args->id]++;
	}

	int err = condWait(common.cond, common.mutex, args->timeout);
	if (err == 0) {
		common.counter++;
	}
	else if (err == -ETIME) {
		common.thrTimeout[args->id]++;
	}
	else {
		common.thrErrors[args->id]++;
	}
	if (mutexUnlock(common.mutex) < 0) {
		common.thrErrors[args->id]++;
	}

	endthread();
}


/*
///////////////////////////////////////////////////////////////////////////////////////////////
*/


TEST_GROUP(condvar_invalid_params);
TEST_GROUP(condvar_signal);
TEST_GROUP(condvar_broadcast);


/*
 *--------------------------------- INVALID PARAMS TESTS -----------------------------------*
 */


TEST_SETUP(condvar_invalid_params)
{
}


TEST_TEAR_DOWN(condvar_invalid_params)
{
}


TEST(condvar_invalid_params, invalid_attr)
{
	handle_t cond;
	struct condAttr attr = { .clock = -1 };

	TEST_ASSERT_EQUAL_INT(-EINVAL, condCreateWithAttr(&cond, &attr));
}


TEST(condvar_invalid_params, invalid_cond)
{
	TEST_ASSERT_EQUAL_INT(-EINVAL, condWait(-1, -1, 0));
	TEST_ASSERT_EQUAL_INT(-EINVAL, condSignal(-1));
	TEST_ASSERT_EQUAL_INT(-EINVAL, condBroadcast(-1));
}


/*
 *------------------------------------- SIGNAL TESTS ---------------------------------------*
 */


TEST_SETUP(condvar_signal)
{
	common.counter = 0;
	common.readyCounter = 0;
	memset((void *)common.thrErrors, 0, sizeof(common.thrErrors));
	memset((void *)common.thrTimeout, 0, sizeof(common.thrTimeout));
	TEST_ASSERT_EQUAL_INT(0, mutexCreate(&common.mutex));
}


TEST_TEAR_DOWN(condvar_signal)
{
	TEST_ASSERT_EQUAL_INT(0, resourceDestroy(common.mutex));
	TEST_ASSERT_EQUAL_INT(0, resourceDestroy(common.cond));
}


TEST(condvar_signal, default_no_timeout)
{
	handle_t tid;
	signal_thread_args_t args = { .delay = 0, .id = 0 };

	TEST_ASSERT_EQUAL_INT(0, condCreate(&common.cond));

	TEST_ASSERT_EQUAL_INT(0, mutexLock(common.mutex));
	TEST_ASSERT_EQUAL_INT(0, beginthreadex(signal_thread, 4, common.stack[0], sizeof(common.stack[0]), &args, &tid));

	TEST_ASSERT_EQUAL_INT(0, condWait(common.cond, common.mutex, SIGNAL_TEST_TIMEOUT));
	TEST_ASSERT_EQUAL_INT(1, common.counter);
	TEST_ASSERT_EQUAL_INT(0, mutexUnlock(common.mutex));

	TEST_ASSERT_EQUAL_INT(tid, threadJoin(tid, 0));
	TEST_ASSERT_EQUAL_INT(0, common.thrErrors[0]);
}


TEST(condvar_signal, relative_no_timeout)
{
	handle_t tid;
	signal_thread_args_t args = { .delay = 0, .id = 0 };
	struct condAttr attr = { .clock = PH_CLOCK_RELATIVE };

	TEST_ASSERT_EQUAL_INT(0, condCreateWithAttr(&common.cond, &attr));

	TEST_ASSERT_EQUAL_INT(0, mutexLock(common.mutex));
	TEST_ASSERT_EQUAL_INT(0, beginthreadex(signal_thread, 4, common.stack[0], sizeof(common.stack[0]), &args, &tid));

	TEST_ASSERT_EQUAL_INT(0, condWait(common.cond, common.mutex, SIGNAL_TEST_TIMEOUT));
	TEST_ASSERT_EQUAL_INT(1, common.counter);
	TEST_ASSERT_EQUAL_INT(0, mutexUnlock(common.mutex));

	TEST_ASSERT_EQUAL_INT(tid, threadJoin(tid, 0));
	TEST_ASSERT_EQUAL_INT(0, common.thrErrors[0]);
}


TEST(condvar_signal, monotonic_no_timeout)
{
	handle_t tid;
	time_t timeout;
	signal_thread_args_t args = { .delay = 0, .id = 0 };
	struct condAttr attr = { .clock = PH_CLOCK_MONOTONIC };

	TEST_ASSERT_EQUAL_INT(0, condCreateWithAttr(&common.cond, &attr));

	TEST_ASSERT_EQUAL_INT(0, mutexLock(common.mutex));
	TEST_ASSERT_EQUAL_INT(0, beginthreadex(signal_thread, 4, common.stack[0], sizeof(common.stack[0]), &args, &tid));

	TEST_ASSERT_EQUAL_INT(0, gettime(&timeout, NULL));
	timeout += SIGNAL_TEST_TIMEOUT;
	TEST_ASSERT_EQUAL_INT(0, condWait(common.cond, common.mutex, timeout));
	TEST_ASSERT_EQUAL_INT(1, common.counter);
	TEST_ASSERT_EQUAL_INT(0, mutexUnlock(common.mutex));

	TEST_ASSERT_EQUAL_INT(tid, threadJoin(tid, 0));
	TEST_ASSERT_EQUAL_INT(0, common.thrErrors[0]);
}


TEST(condvar_signal, realtime_no_timeout)
{
	handle_t tid;
	time_t timeout, offs;
	signal_thread_args_t args = { .delay = 0, .id = 0 };
	struct condAttr attr = { .clock = PH_CLOCK_REALTIME };

	TEST_ASSERT_EQUAL_INT(0, settime(50000));
	TEST_ASSERT_EQUAL_INT(0, condCreateWithAttr(&common.cond, &attr));

	TEST_ASSERT_EQUAL_INT(0, mutexLock(common.mutex));
	TEST_ASSERT_EQUAL_INT(0, beginthreadex(signal_thread, 4, common.stack[0], sizeof(common.stack[0]), &args, &tid));

	TEST_ASSERT_EQUAL_INT(0, gettime(&timeout, &offs));
	timeout += SIGNAL_TEST_TIMEOUT + offs;
	TEST_ASSERT_EQUAL_INT(0, condWait(common.cond, common.mutex, timeout));
	TEST_ASSERT_EQUAL_INT(1, common.counter);

	TEST_ASSERT_EQUAL_INT(0, mutexUnlock(common.mutex));

	TEST_ASSERT_EQUAL_INT(tid, threadJoin(tid, 0));
	TEST_ASSERT_EQUAL_INT(0, common.thrErrors[0]);
}


TEST(condvar_signal, monotonic_timeout)
{
	handle_t tid;
	time_t timeout;
	signal_thread_args_t args = { .delay = 2000, .id = 0 };
	struct condAttr attr = { .clock = PH_CLOCK_MONOTONIC };

	TEST_ASSERT_EQUAL_INT(0, condCreateWithAttr(&common.cond, &attr));

	TEST_ASSERT_EQUAL_INT(0, mutexLock(common.mutex));
	TEST_ASSERT_EQUAL_INT(0, beginthreadex(signal_thread, 4, common.stack[0], sizeof(common.stack[0]), &args, &tid));

	TEST_ASSERT_EQUAL_INT(0, gettime(&timeout, NULL));
	timeout += 1000;
	TEST_ASSERT_EQUAL_INT(-ETIME, condWait(common.cond, common.mutex, timeout));
	TEST_ASSERT_EQUAL_INT(0, mutexUnlock(common.mutex));

	TEST_ASSERT_EQUAL_INT(tid, threadJoin(tid, 0));
	TEST_ASSERT_EQUAL_INT(0, common.thrErrors[0]);
}


TEST(condvar_signal, realtime_timeout)
{
	handle_t tid;
	time_t timeout, offs;
	signal_thread_args_t args = { .delay = 2000, .id = 0 };
	struct condAttr attr = { .clock = PH_CLOCK_REALTIME };

	TEST_ASSERT_EQUAL_INT(0, settime(50000));
	TEST_ASSERT_EQUAL_INT(0, condCreateWithAttr(&common.cond, &attr));

	TEST_ASSERT_EQUAL_INT(0, mutexLock(common.mutex));
	TEST_ASSERT_EQUAL_INT(0, beginthreadex(signal_thread, 4, common.stack[0], sizeof(common.stack[0]), &args, &tid));

	TEST_ASSERT_EQUAL_INT(0, gettime(&timeout, &offs));
	timeout += 1000 + offs;
	TEST_ASSERT_EQUAL_INT(-ETIME, condWait(common.cond, common.mutex, timeout));
	TEST_ASSERT_EQUAL_INT(0, mutexUnlock(common.mutex));

	TEST_ASSERT_EQUAL_INT(tid, threadJoin(tid, 0));
	TEST_ASSERT_EQUAL_INT(0, common.thrErrors[0]);
}


TEST(condvar_signal, monotonic_past_time)
{
	handle_t tid;
	time_t timeout;
	signal_thread_args_t args = { .delay = 0, .id = 0 };
	struct condAttr attr = { .clock = PH_CLOCK_MONOTONIC };

	TEST_ASSERT_EQUAL_INT(0, condCreateWithAttr(&common.cond, &attr));

	TEST_ASSERT_EQUAL_INT(0, mutexLock(common.mutex));
	TEST_ASSERT_EQUAL_INT(0, beginthreadex(signal_thread, 4, common.stack[0], sizeof(common.stack[0]), &args, &tid));

	TEST_ASSERT_EQUAL_INT(0, gettime(&timeout, NULL));
	timeout -= 1000;
	TEST_ASSERT_EQUAL_INT(-ETIME, condWait(common.cond, common.mutex, timeout));
	TEST_ASSERT_EQUAL_INT(0, mutexUnlock(common.mutex));

	TEST_ASSERT_EQUAL_INT(tid, threadJoin(tid, 0));
	TEST_ASSERT_EQUAL_INT(0, common.thrErrors[0]);
}


TEST(condvar_signal, realtime_past_time)
{
	handle_t tid;
	time_t timeout, offs;
	signal_thread_args_t args = { .delay = 0, .id = 0 };
	struct condAttr attr = { .clock = PH_CLOCK_REALTIME };

	TEST_ASSERT_EQUAL_INT(0, settime(50000));
	TEST_ASSERT_EQUAL_INT(0, condCreateWithAttr(&common.cond, &attr));

	TEST_ASSERT_EQUAL_INT(0, mutexLock(common.mutex));
	TEST_ASSERT_EQUAL_INT(0, beginthreadex(signal_thread, 4, common.stack[0], sizeof(common.stack[0]), &args, &tid));

	TEST_ASSERT_EQUAL_INT(0, gettime(&timeout, &offs));
	timeout -= 1000 + offs;
	TEST_ASSERT_EQUAL_INT(-ETIME, condWait(common.cond, common.mutex, timeout));
	TEST_ASSERT_EQUAL_INT(0, mutexUnlock(common.mutex));

	TEST_ASSERT_EQUAL_INT(tid, threadJoin(tid, 0));
	TEST_ASSERT_EQUAL_INT(0, common.thrErrors[0]);
}


TEST(condvar_signal, multiple_threads)
{
	const int thrCount = 2;
	handle_t tid1, tid2;
	worker_thread_args_t args1 = { .timeout = 0, .thrCount = thrCount, .id = 0 };
	worker_thread_args_t args2 = { .timeout = 0, .thrCount = thrCount, .id = 1 };

	TEST_ASSERT_EQUAL_INT(0, condCreate(&common.cond));
	TEST_ASSERT_EQUAL_INT(0, condCreate(&common.readyCond));
	TEST_ASSERT_EQUAL_INT(0, mutexCreate(&common.readyMutex));

	TEST_ASSERT_EQUAL_INT(0, beginthreadex(worker_thread_signal_test, 3, common.stack[0], sizeof(common.stack[0]), &args1, &tid1));
	TEST_ASSERT_EQUAL_INT(0, beginthreadex(worker_thread_signal_test, 3, common.stack[1], sizeof(common.stack[1]), &args2, &tid2));

	/* Wait for the threads to start */
	TEST_ASSERT_EQUAL_INT(0, mutexLock(common.readyMutex));
	while (common.readyCounter < thrCount) {
		TEST_ASSERT_EQUAL_INT(0, condWait(common.readyCond, common.readyMutex, 0));
	}
	common.readyCounter = 0;
	TEST_ASSERT_EQUAL_INT(0, mutexUnlock(common.readyMutex));

	TEST_ASSERT_EQUAL_INT(0, mutexLock(common.mutex));
	TEST_ASSERT_EQUAL_INT(0, condSignal(common.cond));
	TEST_ASSERT_EQUAL_INT(0, mutexUnlock(common.mutex));

	/* Wait for one thread to wakeup */
	TEST_ASSERT_EQUAL_INT(0, mutexLock(common.readyMutex));
	while (common.readyCounter == 0) {
		TEST_ASSERT_EQUAL_INT(0, condWait(common.readyCond, common.readyMutex, 0));
	}
	TEST_ASSERT_EQUAL_INT(0, mutexUnlock(common.readyMutex));

	TEST_ASSERT_EQUAL_INT(0, mutexLock(common.mutex));
	/* Verify that only 1 thread woke up */
	TEST_ASSERT_EQUAL_INT(1, common.counter);
	TEST_ASSERT_EQUAL_INT(0, mutexUnlock(common.mutex));

	/* Signal to 2nd thread */
	TEST_ASSERT_EQUAL_INT(0, mutexLock(common.mutex));
	TEST_ASSERT_EQUAL_INT(0, condSignal(common.cond));
	TEST_ASSERT_EQUAL_INT(0, mutexUnlock(common.mutex));

	/* Wait for the threads to finish */
	TEST_ASSERT_EQUAL_INT(tid1, threadJoin(tid1, 0));
	TEST_ASSERT_EQUAL_INT(tid2, threadJoin(tid2, 0));

	TEST_ASSERT_EQUAL_INT(0, common.thrErrors[0]);
	TEST_ASSERT_EQUAL_INT(0, common.thrTimeout[0]);
	TEST_ASSERT_EQUAL_INT(0, common.thrErrors[1]);
	TEST_ASSERT_EQUAL_INT(0, common.thrTimeout[1]);
	TEST_ASSERT_EQUAL_INT(thrCount, common.counter);

	TEST_ASSERT_EQUAL_INT(0, resourceDestroy(common.readyMutex));
	TEST_ASSERT_EQUAL_INT(0, resourceDestroy(common.readyCond));
}


/*
 *----------------------------------- BROADCAST TESTS -------------------------------------*
 */


TEST_SETUP(condvar_broadcast)
{
	common.counter = 0;
	common.readyCounter = 0;
	memset((void *)common.thrErrors, 0, sizeof(common.thrErrors));
	memset((void *)common.thrTimeout, 0, sizeof(common.thrTimeout));
	TEST_ASSERT_EQUAL_INT(0, mutexCreate(&common.mutex));
	TEST_ASSERT_EQUAL_INT(0, condCreate(&common.readyCond));
	TEST_ASSERT_EQUAL_INT(0, mutexCreate(&common.readyMutex));
}


TEST_TEAR_DOWN(condvar_broadcast)
{
	TEST_ASSERT_EQUAL_INT(0, resourceDestroy(common.mutex));
	TEST_ASSERT_EQUAL_INT(0, resourceDestroy(common.cond));
	TEST_ASSERT_EQUAL_INT(0, resourceDestroy(common.readyMutex));
	TEST_ASSERT_EQUAL_INT(0, resourceDestroy(common.readyCond));
}


TEST(condvar_broadcast, default_no_timeout)
{
	const int thrCount = 2;
	const time_t timeout = BROADCAST_TEST_TIMEOUT;
	handle_t tid1, tid2;
	worker_thread_args_t args1 = { .timeout = timeout, .id = 0, .thrCount = thrCount };
	worker_thread_args_t args2 = { .timeout = timeout, .id = 1, .thrCount = thrCount };

	TEST_ASSERT_EQUAL_INT(0, condCreate(&common.cond));

	TEST_ASSERT_EQUAL_INT(0, beginthreadex(worker_thread_broadcast_test, 3, common.stack[0], sizeof(common.stack[0]), &args1, &tid1));
	TEST_ASSERT_EQUAL_INT(0, beginthreadex(worker_thread_broadcast_test, 3, common.stack[1], sizeof(common.stack[1]), &args2, &tid2));

	/* Wait for the threads to start */
	TEST_ASSERT_EQUAL_INT(0, mutexLock(common.readyMutex));
	while (common.readyCounter < thrCount) {
		TEST_ASSERT_EQUAL_INT(0, condWait(common.readyCond, common.readyMutex, 0));
	}
	TEST_ASSERT_EQUAL_INT(0, mutexUnlock(common.readyMutex));

	TEST_ASSERT_EQUAL_INT(0, mutexLock(common.mutex));
	TEST_ASSERT_EQUAL_INT(0, condBroadcast(common.cond));
	TEST_ASSERT_EQUAL_INT(0, mutexUnlock(common.mutex));

	TEST_ASSERT_EQUAL_INT(tid1, threadJoin(tid1, 0));
	TEST_ASSERT_EQUAL_INT(tid2, threadJoin(tid2, 0));

	TEST_ASSERT_EQUAL_INT(0, common.thrErrors[0]);
	TEST_ASSERT_EQUAL_INT(0, common.thrTimeout[0]);
	TEST_ASSERT_EQUAL_INT(0, common.thrErrors[1]);
	TEST_ASSERT_EQUAL_INT(0, common.thrTimeout[1]);
	TEST_ASSERT_EQUAL_INT(thrCount, common.counter);
}


TEST(condvar_broadcast, relative_no_timeout)
{
	const int thrCount = 2;
	const time_t timeout = BROADCAST_TEST_TIMEOUT;
	handle_t tid1, tid2;
	worker_thread_args_t args1 = { .timeout = timeout, .id = 0, .thrCount = thrCount };
	worker_thread_args_t args2 = { .timeout = timeout, .id = 1, .thrCount = thrCount };
	struct condAttr attr = { .clock = PH_CLOCK_RELATIVE };

	TEST_ASSERT_EQUAL_INT(0, condCreateWithAttr(&common.cond, &attr));

	TEST_ASSERT_EQUAL_INT(0, beginthreadex(worker_thread_broadcast_test, 3, common.stack[0], sizeof(common.stack[0]), &args1, &tid1));
	TEST_ASSERT_EQUAL_INT(0, beginthreadex(worker_thread_broadcast_test, 3, common.stack[1], sizeof(common.stack[1]), &args2, &tid2));

	/* Wait for the threads to start */
	TEST_ASSERT_EQUAL_INT(0, mutexLock(common.readyMutex));
	while (common.readyCounter < thrCount) {
		TEST_ASSERT_EQUAL_INT(0, condWait(common.readyCond, common.readyMutex, 0));
	}
	TEST_ASSERT_EQUAL_INT(0, mutexUnlock(common.readyMutex));

	TEST_ASSERT_EQUAL_INT(0, mutexLock(common.mutex));
	TEST_ASSERT_EQUAL_INT(0, condBroadcast(common.cond));
	TEST_ASSERT_EQUAL_INT(0, mutexUnlock(common.mutex));

	TEST_ASSERT_EQUAL_INT(tid1, threadJoin(tid1, 0));
	TEST_ASSERT_EQUAL_INT(tid2, threadJoin(tid2, 0));

	TEST_ASSERT_EQUAL_INT(0, common.thrErrors[0]);
	TEST_ASSERT_EQUAL_INT(0, common.thrTimeout[0]);
	TEST_ASSERT_EQUAL_INT(0, common.thrErrors[1]);
	TEST_ASSERT_EQUAL_INT(0, common.thrTimeout[1]);
	TEST_ASSERT_EQUAL_INT(thrCount, common.counter);
}


TEST(condvar_broadcast, monotonic_no_timeout)
{
	const int thrCount = 2;
	handle_t tid1, tid2;
	time_t timeout;
	worker_thread_args_t args1 = { .id = 0, .thrCount = thrCount };
	worker_thread_args_t args2 = { .id = 1, .thrCount = thrCount };
	struct condAttr attr = { .clock = PH_CLOCK_MONOTONIC };

	TEST_ASSERT_EQUAL_INT(0, condCreateWithAttr(&common.cond, &attr));

	TEST_ASSERT_EQUAL_INT(0, gettime(&timeout, NULL));
	timeout += BROADCAST_TEST_TIMEOUT;

	args1.timeout = timeout;
	args2.timeout = timeout;

	TEST_ASSERT_EQUAL_INT(0, beginthreadex(worker_thread_broadcast_test, 3, common.stack[0], sizeof(common.stack[0]), &args1, &tid1));
	TEST_ASSERT_EQUAL_INT(0, beginthreadex(worker_thread_broadcast_test, 3, common.stack[1], sizeof(common.stack[1]), &args2, &tid2));

	/* Wait for the threads to start */
	TEST_ASSERT_EQUAL_INT(0, mutexLock(common.readyMutex));
	while (common.readyCounter < thrCount) {
		TEST_ASSERT_EQUAL_INT(0, condWait(common.readyCond, common.readyMutex, 0));
	}
	TEST_ASSERT_EQUAL_INT(0, mutexUnlock(common.readyMutex));

	TEST_ASSERT_EQUAL_INT(0, mutexLock(common.mutex));
	TEST_ASSERT_EQUAL_INT(0, condBroadcast(common.cond));
	TEST_ASSERT_EQUAL_INT(0, mutexUnlock(common.mutex));

	TEST_ASSERT_EQUAL_INT(tid1, threadJoin(tid1, 0));
	TEST_ASSERT_EQUAL_INT(tid2, threadJoin(tid2, 0));

	TEST_ASSERT_EQUAL_INT(0, common.thrErrors[0]);
	TEST_ASSERT_EQUAL_INT(0, common.thrTimeout[0]);
	TEST_ASSERT_EQUAL_INT(0, common.thrErrors[1]);
	TEST_ASSERT_EQUAL_INT(0, common.thrTimeout[1]);
	TEST_ASSERT_EQUAL_INT(thrCount, common.counter);
}


TEST(condvar_broadcast, realtime_no_timeout)
{
	const int thrCount = 2;
	handle_t tid1, tid2;
	time_t timeout, offs;
	worker_thread_args_t args1 = { .id = 0, .thrCount = thrCount };
	worker_thread_args_t args2 = { .id = 1, .thrCount = thrCount };
	struct condAttr attr = { .clock = PH_CLOCK_REALTIME };

	TEST_ASSERT_EQUAL_INT(0, settime(50000));
	TEST_ASSERT_EQUAL_INT(0, condCreateWithAttr(&common.cond, &attr));

	TEST_ASSERT_EQUAL_INT(0, gettime(&timeout, &offs));
	timeout += BROADCAST_TEST_TIMEOUT + offs;

	args1.timeout = timeout;
	args2.timeout = timeout;

	TEST_ASSERT_EQUAL_INT(0, beginthreadex(worker_thread_broadcast_test, 3, common.stack[0], sizeof(common.stack[0]), &args1, &tid1));
	TEST_ASSERT_EQUAL_INT(0, beginthreadex(worker_thread_broadcast_test, 3, common.stack[1], sizeof(common.stack[1]), &args2, &tid2));

	/* Wait for the threads to start */
	TEST_ASSERT_EQUAL_INT(0, mutexLock(common.readyMutex));
	while (common.readyCounter < thrCount) {
		TEST_ASSERT_EQUAL_INT(0, condWait(common.readyCond, common.readyMutex, 0));
	}
	TEST_ASSERT_EQUAL_INT(0, mutexUnlock(common.readyMutex));

	TEST_ASSERT_EQUAL_INT(0, mutexLock(common.mutex));
	TEST_ASSERT_EQUAL_INT(0, condBroadcast(common.cond));
	TEST_ASSERT_EQUAL_INT(0, mutexUnlock(common.mutex));

	TEST_ASSERT_EQUAL_INT(tid1, threadJoin(tid1, 0));
	TEST_ASSERT_EQUAL_INT(tid2, threadJoin(tid2, 0));

	TEST_ASSERT_EQUAL_INT(0, common.thrErrors[0]);
	TEST_ASSERT_EQUAL_INT(0, common.thrTimeout[0]);
	TEST_ASSERT_EQUAL_INT(0, common.thrErrors[1]);
	TEST_ASSERT_EQUAL_INT(0, common.thrTimeout[1]);
	TEST_ASSERT_EQUAL_INT(thrCount, common.counter);
}


TEST(condvar_broadcast, monotonic_timeout)
{
	const int thrCount = 2;
	handle_t tid1, tid2;
	time_t timeout;
	worker_thread_args_t args1 = { .id = 0, .thrCount = thrCount };
	worker_thread_args_t args2 = { .id = 1, .thrCount = thrCount };
	struct condAttr attr = { .clock = PH_CLOCK_MONOTONIC };
	struct timespec wait = { .tv_sec = 0, .tv_nsec = 2000 * 1000 }, rem;

	TEST_ASSERT_EQUAL_INT(0, condCreateWithAttr(&common.cond, &attr));

	TEST_ASSERT_EQUAL_INT(0, gettime(&timeout, NULL));
	timeout += 1000;

	args1.timeout = timeout;
	args2.timeout = timeout;

	TEST_ASSERT_EQUAL_INT(0, beginthreadex(worker_thread_broadcast_test, 3, common.stack[0], sizeof(common.stack[0]), &args1, &tid1));
	TEST_ASSERT_EQUAL_INT(0, beginthreadex(worker_thread_broadcast_test, 3, common.stack[1], sizeof(common.stack[1]), &args2, &tid2));

	/* Wait for the threads to start */
	TEST_ASSERT_EQUAL_INT(0, mutexLock(common.readyMutex));
	while (common.readyCounter < thrCount) {
		TEST_ASSERT_EQUAL_INT(0, condWait(common.readyCond, common.readyMutex, 0));
	}
	TEST_ASSERT_EQUAL_INT(0, mutexUnlock(common.readyMutex));

	while (nanosleep(&wait, &rem) < 0) {
		TEST_ASSERT_EQUAL(EINTR, errno);
		wait = rem;
	}

	TEST_ASSERT_EQUAL_INT(0, mutexLock(common.mutex));
	TEST_ASSERT_EQUAL_INT(0, condBroadcast(common.cond));
	TEST_ASSERT_EQUAL_INT(0, mutexUnlock(common.mutex));

	TEST_ASSERT_EQUAL_INT(tid1, threadJoin(tid1, 0));
	TEST_ASSERT_EQUAL_INT(tid2, threadJoin(tid2, 0));

	TEST_ASSERT_EQUAL_INT(0, common.thrErrors[0]);
	TEST_ASSERT_EQUAL_INT(1, common.thrTimeout[0]);
	TEST_ASSERT_EQUAL_INT(0, common.thrErrors[1]);
	TEST_ASSERT_EQUAL_INT(1, common.thrTimeout[1]);
	TEST_ASSERT_EQUAL_INT(0, common.counter);
}


TEST(condvar_broadcast, realtime_timeout)
{
	const int thrCount = 2;
	handle_t tid1, tid2;
	time_t timeout, offs;
	worker_thread_args_t args1 = { .id = 0, .thrCount = thrCount };
	worker_thread_args_t args2 = { .id = 1, .thrCount = thrCount };
	struct condAttr attr = { .clock = PH_CLOCK_REALTIME };
	struct timespec wait = { .tv_sec = 0, .tv_nsec = 2000 * 1000 }, rem;

	TEST_ASSERT_EQUAL_INT(0, settime(50000));
	TEST_ASSERT_EQUAL_INT(0, condCreateWithAttr(&common.cond, &attr));

	TEST_ASSERT_EQUAL_INT(0, gettime(&timeout, &offs));
	timeout += 1000 + offs;

	args1.timeout = timeout;
	args2.timeout = timeout;

	TEST_ASSERT_EQUAL_INT(0, beginthreadex(worker_thread_broadcast_test, 3, common.stack[0], sizeof(common.stack[0]), &args1, &tid1));
	TEST_ASSERT_EQUAL_INT(0, beginthreadex(worker_thread_broadcast_test, 3, common.stack[1], sizeof(common.stack[1]), &args2, &tid2));

	/* Wait for the threads to start */
	TEST_ASSERT_EQUAL_INT(0, mutexLock(common.readyMutex));
	while (common.readyCounter < thrCount) {
		TEST_ASSERT_EQUAL_INT(0, condWait(common.readyCond, common.readyMutex, 0));
	}
	TEST_ASSERT_EQUAL_INT(0, mutexUnlock(common.readyMutex));

	while (nanosleep(&wait, &rem) < 0) {
		TEST_ASSERT_EQUAL(EINTR, errno);
		wait = rem;
	}

	TEST_ASSERT_EQUAL_INT(0, mutexLock(common.mutex));
	TEST_ASSERT_EQUAL_INT(0, condBroadcast(common.cond));
	TEST_ASSERT_EQUAL_INT(0, mutexUnlock(common.mutex));

	TEST_ASSERT_EQUAL_INT(tid1, threadJoin(tid1, 0));
	TEST_ASSERT_EQUAL_INT(tid2, threadJoin(tid2, 0));

	TEST_ASSERT_EQUAL_INT(0, common.thrErrors[0]);
	TEST_ASSERT_EQUAL_INT(1, common.thrTimeout[0]);
	TEST_ASSERT_EQUAL_INT(0, common.thrErrors[1]);
	TEST_ASSERT_EQUAL_INT(1, common.thrTimeout[1]);
	TEST_ASSERT_EQUAL_INT(0, common.counter);
}

/*
///////////////////////////////////////////////////////////////////////////////////////////////
*/


TEST_GROUP_RUNNER(condvar_invalid_params)
{
	RUN_TEST_CASE(condvar_invalid_params, invalid_attr);
	RUN_TEST_CASE(condvar_invalid_params, invalid_cond);
}


TEST_GROUP_RUNNER(condvar_signal)
{
	RUN_TEST_CASE(condvar_signal, default_no_timeout);
	RUN_TEST_CASE(condvar_signal, relative_no_timeout);
	RUN_TEST_CASE(condvar_signal, monotonic_no_timeout);
	RUN_TEST_CASE(condvar_signal, realtime_no_timeout);
	RUN_TEST_CASE(condvar_signal, monotonic_timeout);
	RUN_TEST_CASE(condvar_signal, realtime_timeout);
	RUN_TEST_CASE(condvar_signal, monotonic_past_time);
	RUN_TEST_CASE(condvar_signal, realtime_past_time);
	RUN_TEST_CASE(condvar_signal, multiple_threads);
}


TEST_GROUP_RUNNER(condvar_broadcast)
{
	RUN_TEST_CASE(condvar_broadcast, default_no_timeout);
	RUN_TEST_CASE(condvar_broadcast, relative_no_timeout);
	RUN_TEST_CASE(condvar_broadcast, monotonic_no_timeout);
	RUN_TEST_CASE(condvar_broadcast, realtime_no_timeout);
	RUN_TEST_CASE(condvar_broadcast, monotonic_timeout);
	RUN_TEST_CASE(condvar_broadcast, realtime_timeout);
}


void runner(void)
{
	RUN_TEST_GROUP(condvar_invalid_params);
	RUN_TEST_GROUP(condvar_signal);
	RUN_TEST_GROUP(condvar_broadcast);
}


int main(int argc, char *argv[])
{
	return (UnityMain(argc, (const char **)argv, runner) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
