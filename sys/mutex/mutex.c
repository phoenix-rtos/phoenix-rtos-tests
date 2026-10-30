/*
 * Phoenix-RTOS
 *
 * test-sys-mutex
 *
 * Test for mutexes
 *
 * Copyright 2024 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/sched.h>
#include <sys/threads.h>

#include <unity_fixture.h>


/*
///////////////////////////////////////////////////////////////////////////////////////////////
*/


TEST_GROUP(mutex_invalid_params);
TEST_GROUP(mutex_single_thread);
TEST_GROUP(mutex_multithreaded);
TEST_GROUP(mutex_priority);


/*
 *--------------------------------- INVALID PARAMS TESTS -----------------------------------*
 */


TEST_SETUP(mutex_invalid_params)
{
}


TEST_TEAR_DOWN(mutex_invalid_params)
{
}


TEST(mutex_invalid_params, invalid_attr)
{
	handle_t mutex;
	struct lockAttr attr = { .type = -1 };

	TEST_ASSERT_EQUAL_INT(-EINVAL, mutexCreateWithAttr(&mutex, &attr));
}


TEST(mutex_invalid_params, invalid_mutex)
{
	TEST_ASSERT_EQUAL_INT(-EINVAL, mutexLock(-1));
	TEST_ASSERT_EQUAL_INT(-EINVAL, mutexUnlock(-1));
	TEST_ASSERT_EQUAL_INT(-EINVAL, resourceDestroy(-1));
}


/*
 *--------------------------------- SINGLE THREADED TESTS ---------------------------------*
 */


TEST_SETUP(mutex_single_thread)
{
}


TEST_TEAR_DOWN(mutex_single_thread)
{
}


TEST(mutex_single_thread, no_attr)
{
	handle_t mutex;

	TEST_ASSERT_EQUAL_INT(0, mutexCreate(&mutex));
	TEST_ASSERT_EQUAL_INT(0, mutexLock(mutex));
	TEST_ASSERT_EQUAL_INT(0, mutexUnlock(mutex));
	TEST_ASSERT_EQUAL_INT(0, resourceDestroy(mutex));
}


TEST(mutex_single_thread, type_default)
{
	/* Should behave the same way as with no attributes */
	handle_t mutex;
	struct lockAttr attr = { .type = PH_LOCK_NORMAL };

	TEST_ASSERT_EQUAL_INT(0, mutexCreateWithAttr(&mutex, &attr));
	TEST_ASSERT_EQUAL_INT(0, mutexLock(mutex));
	TEST_ASSERT_EQUAL_INT(0, mutexUnlock(mutex));
	TEST_ASSERT_EQUAL_INT(0, resourceDestroy(mutex));
}


TEST(mutex_single_thread, type_errorcheck)
{
	handle_t mutex;
	struct lockAttr attr = { .type = PH_LOCK_ERRORCHECK };

	TEST_ASSERT_EQUAL_INT(0, mutexCreateWithAttr(&mutex, &attr));
	TEST_ASSERT_EQUAL_INT(0, mutexLock(mutex));
	TEST_ASSERT_EQUAL_INT(-EDEADLK, mutexLock(mutex));
	TEST_ASSERT_EQUAL_INT(0, mutexUnlock(mutex));
	/* DEBUG build will catch not-locked unlock, don't test it here */
	TEST_ASSERT_EQUAL_INT(0, resourceDestroy(mutex));
}


TEST(mutex_single_thread, type_recursive)
{
	handle_t mutex;
	struct lockAttr attr = { .type = PH_LOCK_RECURSIVE };

	TEST_ASSERT_EQUAL_INT(0, mutexCreateWithAttr(&mutex, &attr));
	TEST_ASSERT_EQUAL_INT(0, mutexLock(mutex));
	TEST_ASSERT_EQUAL_INT(0, mutexLock(mutex));
	TEST_ASSERT_EQUAL_INT(0, mutexUnlock(mutex));
	TEST_ASSERT_EQUAL_INT(0, mutexUnlock(mutex));
	TEST_ASSERT_EQUAL_INT(0, resourceDestroy(mutex));
}


TEST(mutex_single_thread, type_recursive_try)
{
	handle_t mutex;
	struct lockAttr attr = { .type = PH_LOCK_RECURSIVE };

	TEST_ASSERT_EQUAL_INT(0, mutexCreateWithAttr(&mutex, &attr));
	TEST_ASSERT_EQUAL_INT(0, mutexTry(mutex));
	TEST_ASSERT_EQUAL_INT(0, mutexTry(mutex));
	TEST_ASSERT_EQUAL_INT(0, mutexUnlock(mutex));
	TEST_ASSERT_EQUAL_INT(0, mutexUnlock(mutex));
	TEST_ASSERT_EQUAL_INT(0, resourceDestroy(mutex));
}


/*
 *--------------------------------- MULTITHREADED TESTS -----------------------------------*
 */

typedef struct {
	int id;
	useconds_t delay;
} threadArg_t;


static struct {
	handle_t mutex;
	volatile int counter;
	volatile int thrErrors[2];
	char stack[2][4096] __attribute__((aligned(8)));
} mt_common;


static void no_attr_thread(void *arg)
{
	threadArg_t *targ = (threadArg_t *)arg;
	for (int i = 0; i < 100; i++) {
		if ((mutexLock(mt_common.mutex)) < 0) {
			mt_common.thrErrors[targ->id]++;
			endthread();
		}
		usleep(targ->delay);
		if (targ->id == 0) {
			mt_common.counter++;
		}
		else {
			mt_common.counter--;
		}
		if ((mutexUnlock(mt_common.mutex)) < 0) {
			mt_common.thrErrors[targ->id]++;
			endthread();
		}
	}
	endthread();
}


static void errorcheck_thread(void *arg)
{
	threadArg_t *targ = (threadArg_t *)arg;
	for (int i = 0; i < 100; i++) {
		if ((mutexLock(mt_common.mutex)) < 0) {
			mt_common.thrErrors[targ->id]++;
			endthread();
		}
		if (mutexLock(mt_common.mutex) != -EDEADLK) {
			mt_common.thrErrors[targ->id]++;
		}
		usleep(targ->delay);
		if (targ->id == 0) {
			mt_common.counter++;
		}
		else {
			mt_common.counter--;
		}
		if ((mutexUnlock(mt_common.mutex)) < 0) {
			mt_common.thrErrors[targ->id]++;
			endthread();
		}
	}
	endthread();
}


static void recursive_thread(void *arg)
{
	threadArg_t *targ = (threadArg_t *)arg;
	for (int i = 0; i < 100; i++) {
		if ((mutexLock(mt_common.mutex)) < 0) {
			mt_common.thrErrors[targ->id]++;
			endthread();
		}
		if ((mutexLock(mt_common.mutex)) < 0) {
			mt_common.thrErrors[targ->id]++;
			mutexUnlock(mt_common.mutex);
			endthread();
		}
		if (targ->id == 0) {
			mt_common.counter++;
		}
		else {
			mt_common.counter--;
		}
		usleep(targ->delay);
		if ((mutexUnlock(mt_common.mutex)) < 0) {
			mt_common.thrErrors[targ->id]++;
			endthread();
		}
		if ((mutexUnlock(mt_common.mutex)) < 0) {
			mt_common.thrErrors[targ->id]++;
			endthread();
		}
	}
	endthread();
}


static void recursive_try_thread(void *arg)
{
	threadArg_t *targ = (threadArg_t *)arg;
	for (int i = 0; i < 100; i++) {
		if ((mutexLock(mt_common.mutex)) < 0) {
			mt_common.thrErrors[targ->id]++;
			endthread();
		}
		if ((mutexTry(mt_common.mutex)) < 0) {
			mt_common.thrErrors[targ->id]++;
			mutexUnlock(mt_common.mutex);
			endthread();
		}
		if (targ->id == 0) {
			mt_common.counter++;
		}
		else {
			mt_common.counter--;
		}
		usleep(targ->delay);
		if ((mutexUnlock(mt_common.mutex)) < 0) {
			mt_common.thrErrors[targ->id]++;
			endthread();
		}
		if ((mutexUnlock(mt_common.mutex)) < 0) {
			mt_common.thrErrors[targ->id]++;
			endthread();
		}
	}
	endthread();
}


TEST_SETUP(mutex_multithreaded)
{
	mt_common.counter = 0;
	memset((void *)mt_common.thrErrors, 0, sizeof(mt_common.thrErrors));
}


TEST_TEAR_DOWN(mutex_multithreaded)
{
	TEST_ASSERT_EQUAL_INT(0, resourceDestroy(mt_common.mutex));
}


TEST(mutex_multithreaded, no_attr)
{
	handle_t tid1, tid2;
	threadArg_t arg1 = { .id = 0, .delay = 1 };
	threadArg_t arg2 = { .id = 1, .delay = 3 };

	TEST_ASSERT_EQUAL_INT(0, mutexCreate(&mt_common.mutex));
	TEST_ASSERT_EQUAL_INT(0, beginthreadex(no_attr_thread, 4, mt_common.stack[0], sizeof(mt_common.stack[0]), &arg1, &tid1));
	TEST_ASSERT_EQUAL_INT(0, beginthreadex(no_attr_thread, 4, mt_common.stack[1], sizeof(mt_common.stack[1]), &arg2, &tid2));
	TEST_ASSERT_EQUAL_INT(tid1, threadJoin(tid1, 0));
	TEST_ASSERT_EQUAL_INT(tid2, threadJoin(tid2, 0));
	TEST_ASSERT_EQUAL_INT(0, mt_common.counter);
	TEST_ASSERT_EQUAL_INT(0, mt_common.thrErrors[0]);
	TEST_ASSERT_EQUAL_INT(0, mt_common.thrErrors[1]);
}


TEST(mutex_multithreaded, type_default)
{
	/* Should behave the same way as with no attributes */
	handle_t tid1, tid2;
	struct lockAttr attr = { .type = PH_LOCK_NORMAL };
	threadArg_t arg1 = { .id = 0, .delay = 1 };
	threadArg_t arg2 = { .id = 1, .delay = 3 };

	TEST_ASSERT_EQUAL_INT(0, mutexCreateWithAttr(&mt_common.mutex, &attr));
	TEST_ASSERT_EQUAL_INT(0, beginthreadex(no_attr_thread, 4, mt_common.stack[0], sizeof(mt_common.stack[0]), &arg1, &tid1));
	TEST_ASSERT_EQUAL_INT(0, beginthreadex(no_attr_thread, 4, mt_common.stack[1], sizeof(mt_common.stack[1]), &arg2, &tid2));
	TEST_ASSERT_EQUAL_INT(tid1, threadJoin(tid1, 0));
	TEST_ASSERT_EQUAL_INT(tid2, threadJoin(tid2, 0));
	TEST_ASSERT_EQUAL_INT(0, mt_common.counter);
	TEST_ASSERT_EQUAL_INT(0, mt_common.thrErrors[0]);
	TEST_ASSERT_EQUAL_INT(0, mt_common.thrErrors[1]);
}


TEST(mutex_multithreaded, type_errorcheck)
{
	handle_t tid1, tid2;
	struct lockAttr attr = { .type = PH_LOCK_ERRORCHECK };
	threadArg_t arg1 = { .id = 0, .delay = 1 };
	threadArg_t arg2 = { .id = 1, .delay = 3 };

	TEST_ASSERT_EQUAL_INT(0, mutexCreateWithAttr(&mt_common.mutex, &attr));
	TEST_ASSERT_EQUAL_INT(0, beginthreadex(errorcheck_thread, 4, mt_common.stack[0], sizeof(mt_common.stack[0]), &arg1, &tid1));
	TEST_ASSERT_EQUAL_INT(0, beginthreadex(errorcheck_thread, 4, mt_common.stack[1], sizeof(mt_common.stack[1]), &arg2, &tid2));
	TEST_ASSERT_EQUAL_INT(tid1, threadJoin(tid1, 0));
	TEST_ASSERT_EQUAL_INT(tid2, threadJoin(tid2, 0));
	TEST_ASSERT_EQUAL_INT(0, mt_common.counter);
	TEST_ASSERT_EQUAL_INT(0, mt_common.thrErrors[0]);
	TEST_ASSERT_EQUAL_INT(0, mt_common.thrErrors[1]);
}


TEST(mutex_multithreaded, type_recursive)
{
	handle_t tid1, tid2;
	struct lockAttr attr = { .type = PH_LOCK_RECURSIVE };
	threadArg_t arg1 = { .id = 0, .delay = 1 };
	threadArg_t arg2 = { .id = 1, .delay = 3 };

	TEST_ASSERT_EQUAL_INT(0, mutexCreateWithAttr(&mt_common.mutex, &attr));
	TEST_ASSERT_EQUAL_INT(0, beginthreadex(recursive_thread, 4, mt_common.stack[0], sizeof(mt_common.stack[0]), &arg1, &tid1));
	TEST_ASSERT_EQUAL_INT(0, beginthreadex(recursive_thread, 4, mt_common.stack[1], sizeof(mt_common.stack[1]), &arg2, &tid2));
	TEST_ASSERT_EQUAL_INT(tid1, threadJoin(tid1, 0));
	TEST_ASSERT_EQUAL_INT(tid2, threadJoin(tid2, 0));
	TEST_ASSERT_EQUAL_INT(0, mt_common.counter);
	TEST_ASSERT_EQUAL_INT(0, mt_common.thrErrors[0]);
	TEST_ASSERT_EQUAL_INT(0, mt_common.thrErrors[1]);
}


TEST(mutex_multithreaded, type_recursive_try)
{
	handle_t tid1, tid2;
	struct lockAttr attr = { .type = PH_LOCK_RECURSIVE };
	threadArg_t arg1 = { .id = 0, .delay = 1 };
	threadArg_t arg2 = { .id = 1, .delay = 3 };

	TEST_ASSERT_EQUAL_INT(0, mutexCreateWithAttr(&mt_common.mutex, &attr));
	TEST_ASSERT_EQUAL_INT(0, beginthreadex(recursive_try_thread, 4, mt_common.stack[0], sizeof(mt_common.stack[0]), &arg1, &tid1));
	TEST_ASSERT_EQUAL_INT(0, beginthreadex(recursive_try_thread, 4, mt_common.stack[1], sizeof(mt_common.stack[1]), &arg2, &tid2));
	TEST_ASSERT_EQUAL_INT(tid1, threadJoin(tid1, 0));
	TEST_ASSERT_EQUAL_INT(tid2, threadJoin(tid2, 0));
	TEST_ASSERT_EQUAL_INT(0, mt_common.counter);
	TEST_ASSERT_EQUAL_INT(0, mt_common.thrErrors[0]);
	TEST_ASSERT_EQUAL_INT(0, mt_common.thrErrors[1]);
}


/*
 *--------------------------------- PRIORITY INHERITANCE TESTS ----------------------------*
 */


#define PRIO_MUTEXES 3
#define PRIO_THREADS 4


static struct {
	handle_t mutex[PRIO_MUTEXES];
	handle_t tid[PRIO_THREADS];
	int mutexes;
	int threads;
	int mainPrio;
	volatile int step;
	volatile int release;
	volatile int thrErrors;
	char stack[PRIO_THREADS][4096] __attribute__((aligned(8)));
} prio_common;


/*
 * TEST_ASSERT_EQUAL_INT that releases the spinning lock owners first on a failure:
 * an owner left at a raised priority could starve the console printing the failure.
 */
#define ASSERT_EQUAL_OR_RELEASE(expected, actual) \
	do { \
		int value = (actual); \
		if (value != (expected)) { \
			prio_common.release = 1; \
		} \
		TEST_ASSERT_EQUAL_INT((expected), value); \
	} while (0)


static int prio_get(handle_t tid)
{
	sched_params_t params;

	ASSERT_EQUAL_OR_RELEASE(0, schedGet(0, tid, &params));

	return params.priority;
}


/* Waits up to 2 s for the thread to reach the priority, returns the last one seen */
static int prio_wait(handle_t tid, int expected)
{
	int prio = prio_get(tid);

	for (int i = 0; (i < 2000) && (prio != expected); i++) {
		usleep(1000);
		prio = prio_get(tid);
	}

	return prio;
}


static int prio_waitStep(int step)
{
	for (int i = 0; (i < 2000) && (prio_common.step < step) && (prio_common.release == 0); i++) {
		usleep(1000);
	}

	return prio_common.step;
}


static int prio_start(void (*start)(void *), int prio, void *arg)
{
	int i = prio_common.threads;
	int err;

	if (i >= PRIO_THREADS) {
		return -ENOMEM;
	}

	err = beginthreadex(start, prio, prio_common.stack[i], sizeof(prio_common.stack[i]), arg, &prio_common.tid[i]);
	if (err == 0) {
		prio_common.threads++;
	}

	return err;
}


/*
 * Lock owners spin instead of sleeping: any syscall releasing a kernel lock
 * recomputes the caller's priority, which would hide a stale one.
 */


/* Holds mutex[0] until released */
static void prio_holdThread(void *arg)
{
	if (mutexLock(prio_common.mutex[0]) < 0) {
		prio_common.thrErrors++;
		endthread();
	}

	prio_common.step = 1;

	while (prio_common.release == 0) {
	}

	mutexUnlock(prio_common.mutex[0]);
	endthread();
}


/* Holds mutex[0], raises itself above its waiter, lowers itself below it, then unlocks mutex[0] */
static void prio_lowerThread(void *arg)
{
	if (mutexLock(prio_common.mutex[0]) < 0) {
		prio_common.thrErrors++;
		endthread();
	}

	prio_common.step = 1;

	if (prio_waitStep(2) >= 2) {
		setPriority(2);
		setPriority(6);
		prio_common.step = 3;
	}

	while ((prio_common.step < 4) && (prio_common.release == 0)) {
	}

	mutexUnlock(prio_common.mutex[0]);
	prio_common.step = 5;

	while (prio_common.release == 0) {
	}

	endthread();
}


/* Holds mutex[i] while waiting for mutex[i - 1] */
static void prio_chainThread(void *arg)
{
	unsigned int i = (unsigned int)(uintptr_t)arg;

	if (mutexLock(prio_common.mutex[i]) < 0) {
		prio_common.thrErrors++;
		endthread();
	}

	if (mutexLock(prio_common.mutex[i - 1]) < 0) {
		prio_common.thrErrors++;
	}
	else {
		mutexUnlock(prio_common.mutex[i - 1]);
	}

	mutexUnlock(prio_common.mutex[i]);
	endthread();
}


static void prio_lockThread(void *arg)
{
	handle_t mutex = *(handle_t *)arg;

	if (mutexLock(mutex) < 0) {
		prio_common.thrErrors++;
		endthread();
	}

	mutexUnlock(mutex);
	endthread();
}


static void prio_timedLockThread(void *arg)
{
	handle_t mutex = *(handle_t *)arg;

	if (mutexLockClockWait(mutex, 500 * 1000, PH_CLOCK_RELATIVE) != -ETIME) {
		prio_common.thrErrors++;
		mutexUnlock(mutex);
	}

	endthread();
}


/* Locks mutex[0], then mutex[1]: the opposite order to prio_cycleTimedThread */
static void prio_cycleThread(void *arg)
{
	if (mutexLock(prio_common.mutex[0]) < 0) {
		prio_common.thrErrors++;
		endthread();
	}

	prio_common.step = 1;

	while ((prio_common.step < 2) && (prio_common.release == 0)) {
	}

	prio_common.step = 3;

	if (mutexLock(prio_common.mutex[1]) < 0) {
		prio_common.thrErrors++;
	}
	else {
		mutexUnlock(prio_common.mutex[1]);
	}

	mutexUnlock(prio_common.mutex[0]);
	endthread();
}


/* Locks mutex[1], waits 1.5 s for mutex[0], then holds mutex[1] until step 5 */
static void prio_cycleTimedThread(void *arg)
{
	if (mutexLock(prio_common.mutex[1]) < 0) {
		prio_common.thrErrors++;
		endthread();
	}

	prio_common.step = 2;

	if (mutexLockClockWait(prio_common.mutex[0], 1500 * 1000, PH_CLOCK_RELATIVE) != -ETIME) {
		prio_common.thrErrors++;
		mutexUnlock(prio_common.mutex[0]);
	}

	prio_common.step = 4;

	while ((prio_common.step < 5) && (prio_common.release == 0)) {
	}

	mutexUnlock(prio_common.mutex[1]);
	endthread();
}


TEST_SETUP(mutex_priority)
{
	memset(&prio_common, 0, sizeof(prio_common));

	/* Observe the spinning threads from above */
	prio_common.mainPrio = getPriority();
	setPriority(0);

	for (int i = 0; i < PRIO_MUTEXES; i++) {
		TEST_ASSERT_EQUAL_INT(0, mutexCreate(&prio_common.mutex[i]));
		prio_common.mutexes++;
	}
}


TEST_TEAR_DOWN(mutex_priority)
{
	int errors = 0;

	prio_common.release = 1;
	for (int i = 0; i < prio_common.threads; i++) {
		if (threadJoin(prio_common.tid[i], 0) != prio_common.tid[i]) {
			errors++;
		}
	}

	setPriority(prio_common.mainPrio);

	for (int i = 0; i < prio_common.mutexes; i++) {
		if (resourceDestroy(prio_common.mutex[i]) != 0) {
			errors++;
		}
	}

	TEST_ASSERT_EQUAL_INT(0, errors);
	TEST_ASSERT_EQUAL_INT(0, prio_common.thrErrors);
}


/*
 * A mutex owner raises and then lowers its own priority while a better thread waits for the mutex.
 * The owner used to keep the raised priority (2) instead of the inherited one (3).
 */
TEST(mutex_priority, lower_keeps_inherited)
{
	/* 1. The owner (tid[0], priority 5) locks mutex[0] */
	ASSERT_EQUAL_OR_RELEASE(0, prio_start(prio_lowerThread, 5, NULL));
	ASSERT_EQUAL_OR_RELEASE(1, prio_waitStep(1));

	/* 2. The waiter (tid[1], priority 3) blocks on mutex[0]: the owner inherits 3 */
	ASSERT_EQUAL_OR_RELEASE(0, prio_start(prio_lockThread, 3, &prio_common.mutex[0]));
	ASSERT_EQUAL_OR_RELEASE(3, prio_wait(prio_common.tid[0], 3));

	/* 3. The owner calls setPriority(2), then setPriority(6) */
	prio_common.step = 2;
	ASSERT_EQUAL_OR_RELEASE(3, prio_waitStep(3));

	/* 4. The owner must run at the waiter's 3, which is better than its new base 6 */
	ASSERT_EQUAL_OR_RELEASE(3, prio_get(prio_common.tid[0]));

	/* 5. The owner unlocks mutex[0]: it goes back to its base 6 */
	prio_common.step = 4;
	ASSERT_EQUAL_OR_RELEASE(5, prio_waitStep(5));
	ASSERT_EQUAL_OR_RELEASE(6, prio_get(prio_common.tid[0]));
}


/*
 * A chain of three mutexes, each owner blocked on the next mutex down: only the holder can run.
 * Only the direct owner used to inherit: the holder stayed at 5, and any ready thread at 3 or 4
 * kept it, and with it the top waiter at 2, off the CPU.
 */
TEST(mutex_priority, chain)
{
	/* 1. The holder (tid[0], priority 6) locks mutex[0] */
	ASSERT_EQUAL_OR_RELEASE(0, prio_start(prio_holdThread, 6, NULL));
	ASSERT_EQUAL_OR_RELEASE(1, prio_waitStep(1));

	/* 2. chain#1 (tid[1], priority 5) locks mutex[1] and blocks on mutex[0]: the holder inherits 5 */
	ASSERT_EQUAL_OR_RELEASE(0, prio_start(prio_chainThread, 5, (void *)(uintptr_t)1));
	ASSERT_EQUAL_OR_RELEASE(5, prio_wait(prio_common.tid[0], 5));

	/* 3. chain#2 (tid[2], priority 4) locks mutex[2] and blocks on mutex[1]: chain#1 and the holder inherit 4 */
	ASSERT_EQUAL_OR_RELEASE(0, prio_start(prio_chainThread, 4, (void *)(uintptr_t)2));
	ASSERT_EQUAL_OR_RELEASE(4, prio_wait(prio_common.tid[1], 4));
	ASSERT_EQUAL_OR_RELEASE(4, prio_get(prio_common.tid[0]));

	/* 4. The top waiter (tid[3], priority 2) blocks on mutex[2]: chain#2, chain#1 and the holder inherit 2 */
	ASSERT_EQUAL_OR_RELEASE(0, prio_start(prio_lockThread, 2, &prio_common.mutex[2]));
	ASSERT_EQUAL_OR_RELEASE(2, prio_wait(prio_common.tid[2], 2));
	ASSERT_EQUAL_OR_RELEASE(2, prio_get(prio_common.tid[1]));
	ASSERT_EQUAL_OR_RELEASE(2, prio_get(prio_common.tid[0]));
}


/*
 * The top waiter of a chain times out: the priority it lent must be withdrawn from every owner
 * down the chain, or the holder keeps running at 2 with nobody at 2 waiting for it.
 */
TEST(mutex_priority, chain_timeout)
{
	/* 1. The holder (tid[0], priority 6) locks mutex[0] */
	ASSERT_EQUAL_OR_RELEASE(0, prio_start(prio_holdThread, 6, NULL));
	ASSERT_EQUAL_OR_RELEASE(1, prio_waitStep(1));

	/* 2. chain#1 (tid[1], priority 5) locks mutex[1] and blocks on mutex[0]: the holder inherits 5 */
	ASSERT_EQUAL_OR_RELEASE(0, prio_start(prio_chainThread, 5, (void *)(uintptr_t)1));
	ASSERT_EQUAL_OR_RELEASE(5, prio_wait(prio_common.tid[0], 5));

	/* 3. The top waiter (tid[2], priority 2) blocks on mutex[1] for 500 ms: chain#1 and the holder inherit 2 */
	ASSERT_EQUAL_OR_RELEASE(0, prio_start(prio_timedLockThread, 2, &prio_common.mutex[1]));
	ASSERT_EQUAL_OR_RELEASE(2, prio_wait(prio_common.tid[1], 2));
	ASSERT_EQUAL_OR_RELEASE(2, prio_get(prio_common.tid[0]));

	/* 4. The top waiter times out: chain#1 and the holder go back to 5 */
	ASSERT_EQUAL_OR_RELEASE(5, prio_wait(prio_common.tid[1], 5));
	ASSERT_EQUAL_OR_RELEASE(5, prio_wait(prio_common.tid[0], 5));
}


/*
 * Another thread changes the priority of a blocked waiter with schedSet(): the owner of the
 * mutex must follow. The owner used to keep the waiter's old priority.
 */
TEST(mutex_priority, waiter_raised)
{
	sched_params_t params = { .priorityBase = 2 };

	/* 1. The holder (tid[0], priority 6) locks mutex[0] */
	ASSERT_EQUAL_OR_RELEASE(0, prio_start(prio_holdThread, 6, NULL));
	ASSERT_EQUAL_OR_RELEASE(1, prio_waitStep(1));

	/* 2. The waiter (tid[1], priority 5) blocks on mutex[0]: the holder inherits 5 */
	ASSERT_EQUAL_OR_RELEASE(0, prio_start(prio_lockThread, 5, &prio_common.mutex[0]));
	ASSERT_EQUAL_OR_RELEASE(5, prio_wait(prio_common.tid[0], 5));

	/* 3. schedSet() raises the waiter to 2: the holder inherits 2 */
	ASSERT_EQUAL_OR_RELEASE(0, schedSet(0, prio_common.tid[1], SCHED_RR, &params));
	ASSERT_EQUAL_OR_RELEASE(2, prio_get(prio_common.tid[1]));
	ASSERT_EQUAL_OR_RELEASE(2, prio_get(prio_common.tid[0]));

	/* 4. schedSet() lowers the waiter back to 5: the holder goes back to 5 */
	params.priorityBase = 5;
	ASSERT_EQUAL_OR_RELEASE(0, schedSet(0, prio_common.tid[1], SCHED_RR, &params));
	ASSERT_EQUAL_OR_RELEASE(5, prio_get(prio_common.tid[1]));
	ASSERT_EQUAL_OR_RELEASE(5, prio_get(prio_common.tid[0]));
}


/*
 * Two threads lock two mutexes in opposite order and wait for each other until one wait times out.
 * The priority walk must stop when it comes back round the cycle, and both must recover their priorities.
 */
TEST(mutex_priority, cycle)
{
	/* 1. A (tid[0], priority 6) locks mutex[0] */
	ASSERT_EQUAL_OR_RELEASE(0, prio_start(prio_cycleThread, 6, NULL));
	ASSERT_EQUAL_OR_RELEASE(1, prio_waitStep(1));

	/* 2. B (tid[1], priority 5) locks mutex[1] and blocks on mutex[0] for 1.5 s: A inherits 5 */
	ASSERT_EQUAL_OR_RELEASE(0, prio_start(prio_cycleTimedThread, 5, NULL));
	ASSERT_EQUAL_OR_RELEASE(5, prio_wait(prio_common.tid[0], 5));

	/* 3. A blocks on mutex[1]: A and B wait for each other */
	ASSERT_EQUAL_OR_RELEASE(3, prio_waitStep(3));

	/*
	 * 4. H (tid[2], priority 2) blocks on mutex[0] for 500 ms: A and B inherit 2. The walk goes
	 * from A to B and back to A, where it must stop.
	 */
	ASSERT_EQUAL_OR_RELEASE(0, prio_start(prio_timedLockThread, 2, &prio_common.mutex[0]));
	ASSERT_EQUAL_OR_RELEASE(2, prio_wait(prio_common.tid[0], 2));
	ASSERT_EQUAL_OR_RELEASE(2, prio_wait(prio_common.tid[1], 2));

	/*
	 * 5. B's wait times out, which breaks the cycle: A goes back to 6 and B to 5. H's timeout alone
	 * does not lower them: while both are blocked, each one's inherited 2 comes from the other.
	 */
	ASSERT_EQUAL_OR_RELEASE(4, prio_waitStep(4));
	ASSERT_EQUAL_OR_RELEASE(6, prio_get(prio_common.tid[0]));
	ASSERT_EQUAL_OR_RELEASE(5, prio_get(prio_common.tid[1]));
	prio_common.step = 5;
}


/*
///////////////////////////////////////////////////////////////////////////////////////////////
*/


TEST_GROUP_RUNNER(mutex_invalid_params)
{
	RUN_TEST_CASE(mutex_invalid_params, invalid_attr);
	RUN_TEST_CASE(mutex_invalid_params, invalid_mutex);
}


TEST_GROUP_RUNNER(mutex_single_thread)
{
	RUN_TEST_CASE(mutex_single_thread, no_attr);
	RUN_TEST_CASE(mutex_single_thread, type_default);
	RUN_TEST_CASE(mutex_single_thread, type_errorcheck);
	RUN_TEST_CASE(mutex_single_thread, type_recursive);
	RUN_TEST_CASE(mutex_single_thread, type_recursive_try);
}


TEST_GROUP_RUNNER(mutex_multithreaded)
{
	RUN_TEST_CASE(mutex_multithreaded, no_attr);
	RUN_TEST_CASE(mutex_multithreaded, type_default);
	RUN_TEST_CASE(mutex_multithreaded, type_errorcheck);
	RUN_TEST_CASE(mutex_multithreaded, type_recursive);
	RUN_TEST_CASE(mutex_multithreaded, type_recursive_try);
}


TEST_GROUP_RUNNER(mutex_priority)
{
	RUN_TEST_CASE(mutex_priority, lower_keeps_inherited);
	RUN_TEST_CASE(mutex_priority, chain);
	RUN_TEST_CASE(mutex_priority, chain_timeout);
	RUN_TEST_CASE(mutex_priority, waiter_raised);
	RUN_TEST_CASE(mutex_priority, cycle);
}


void runner(void)
{
	RUN_TEST_GROUP(mutex_invalid_params);
	RUN_TEST_GROUP(mutex_single_thread);
	RUN_TEST_GROUP(mutex_multithreaded);
	RUN_TEST_GROUP(mutex_priority);
}


int main(int argc, char *argv[])
{
	return (UnityMain(argc, (const char **)argv, runner) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
