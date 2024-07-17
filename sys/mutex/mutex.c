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
#include <sys/threads.h>

#include <unity_fixture.h>


/*
///////////////////////////////////////////////////////////////////////////////////////////////
*/


TEST_GROUP(mutex_invalid_params);
TEST_GROUP(mutex_single_thread);
TEST_GROUP(mutex_multithreaded);


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
}


TEST_GROUP_RUNNER(mutex_multithreaded)
{
	RUN_TEST_CASE(mutex_multithreaded, no_attr);
	RUN_TEST_CASE(mutex_multithreaded, type_default);
	RUN_TEST_CASE(mutex_multithreaded, type_errorcheck);
	RUN_TEST_CASE(mutex_multithreaded, type_recursive);
}


void runner(void)
{
	RUN_TEST_GROUP(mutex_invalid_params);
	RUN_TEST_GROUP(mutex_single_thread);
	RUN_TEST_GROUP(mutex_multithreaded);
}


int main(int argc, char *argv[])
{
	return (UnityMain(argc, (const char **)argv, runner) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
