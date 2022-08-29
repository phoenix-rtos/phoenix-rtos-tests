/*
 * Phoenix-RTOS
 *
 * phoenix-rtos-tests: random lifespan threads test
 *
 * Copyright 2021 Phoenix Systems
 * Author: Mateusz Niewiadomski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/threads.h>

#include "unity_fixture.h"


typedef struct {
	unsigned int id;
	unsigned int time;
	unsigned int err;
} thread_params_t;


typedef struct {
	thread_params_t params;
	char stack[512] __attribute__((aligned(8))); /* stack needs to be aligned to 8 */
} thread_self_t;


typedef struct {
	unsigned int fin;
	handle_t finlock;
	unsigned int timemin;      /* in microseconds */
	unsigned int timemax;      /* in microseconds */
	thread_self_t threads[10]; /* set threads amount here */
} threads_common_t;


static threads_common_t threads_common = { .timemin = 100000, .timemax = 300000 };

static void test_thread(void *arg)
{
	thread_params_t *params = (thread_params_t *)arg;
	int err;
	usleep(params->time);

	err = mutexLock(threads_common.finlock);
	if (err == 0) {
		threads_common.fin++;
		err = mutexUnlock(threads_common.finlock);
	}
	params->err = err;

	endthread();
}


TEST_GROUP(threads_rand);


TEST_SETUP(threads_rand)
{
}


TEST_TEAR_DOWN(threads_rand)
{
}


TEST(threads_rand, test_1)
{
	int cthread, err, i, joined = 0;

	cthread = sizeof(threads_common.threads) / sizeof(threads_common.threads[0]);
	err = mutexCreate(&(threads_common.finlock));
	TEST_ASSERT_EQUAL_INT(EOK, err);
	threads_common.fin = 0;

	srand(7);
	/* assign threads parameters */
	for (i = 0; i < cthread; i++) {
		threads_common.threads[i].params.id = i;
		threads_common.threads[i].params.err = EOK;
		/* assign thread lifespan in milliseconds */
		threads_common.threads[i].params.time = threads_common.timemin + ((double)rand() / (double)RAND_MAX) * (threads_common.timemax - threads_common.timemin);
		err = beginthread(test_thread, 6, &(threads_common.threads[i].stack), sizeof(threads_common.threads[i].stack), &(threads_common.threads[i].params));
		TEST_ASSERT_EQUAL_INT(EOK, err);
	}

	/* all threads should end in under one second */
	for (i = 0; i < cthread; i++) {
		err = threadJoin(-1, 0);
		TEST_ASSERT_GREATER_OR_EQUAL_INT(EOK, err);
		joined++;
	}

	for (i = 0; i < cthread; i++)
		TEST_ASSERT_EQUAL_INT(EOK, threads_common.threads[i].params.err);

	err = resourceDestroy(threads_common.finlock);
	TEST_ASSERT_EQUAL_INT(EOK, err);

	TEST_ASSERT_EQUAL_INT(cthread, threads_common.fin);
	TEST_ASSERT_EQUAL_INT(cthread, joined);
}


TEST_GROUP_RUNNER(threads_rand)
{
	RUN_TEST_CASE(threads_rand, test_1);
}


void runner(void)
{
	RUN_TEST_GROUP(threads_rand);
}


int main(int argc, char *argv[])
{
	UnityMain(argc, (const char **)argv, runner);
	return 0;
}
