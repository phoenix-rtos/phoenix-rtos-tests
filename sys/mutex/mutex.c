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


#define PRIO_MUTEXES 1
#define PRIO_THREADS 2


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
