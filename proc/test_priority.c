/*
 * Phoenix-RTOS
 *
 * Threads priority tests
 *
 * Copyright 2022 Phoenix Systems
 * Author: Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <stdlib.h>
#include <unistd.h>

#include <sys/threads.h>


typedef struct {
	handle_t lock;
	handle_t cond;
	volatile int done;
} completion_t;


static struct {
	completion_t comp;  /* Priority inversion test completion */
	completion_t lcomp; /* Low priority thread init completion */
	completion_t scomp; /* Test setup completion */

	/* Thread stacks */
	char stack[3][256] __attribute__((aligned(8)));
} priority_common;


static void completion_wait(completion_t *comp)
{
	mutexLock(comp->lock);

	while (!comp->done) {
		condWait(comp->cond, comp->lock, 0);
	}

	mutexUnlock(comp->lock);
}


static void completion_finish(completion_t *comp)
{
	mutexLock(comp->lock);

	comp->done = 1;
	condSignal(comp->cond);

	mutexUnlock(comp->lock);
}


static void completion_done(completion_t *comp)
{
	resourceDestroy(comp->lock);
	resourceDestroy(comp->cond);
}


static void completion_init(completion_t *comp)
{
	mutexCreate(&comp->lock);
	condCreate(&comp->cond);
	comp->done = 0;
}


static void test_priority_lthr(void *arg)
{
	mutexLock(priority_common.comp.lock);

	completion_finish(&priority_common.lcomp);
	completion_wait(&priority_common.scomp);

	/* Return the test completion lock */
	/* Will not get here unless priority inversion fix is implemented */
	/* (middle priority thread starves us) */
	mutexUnlock(priority_common.comp.lock);

	endthread();
}


static void test_priority_mthr(void *arg)
{
	/* Busy waiting for test completion (starves the low priority thread) */
	while (!priority_common.comp.done)
		;

	endthread();
}


static void test_priority_hthr(void *arg)
{
	/* Finish test completion */
	/* Will not be able to take the test completion lock unless priority inversion fix is implemented */
	/* (low priority thread holds the lock) */
	completion_finish(&priority_common.comp);
	endthread();
}


static void test_priority_inversion(void *arg)
{
	/* Init completions */
	completion_init(&priority_common.comp);
	completion_init(&priority_common.lcomp);
	completion_init(&priority_common.scomp);

	/* Run low priority thread */
	beginthread(test_priority_lthr, 6, priority_common.stack[0], sizeof(priority_common.stack[0]), NULL);
	completion_wait(&priority_common.lcomp);

	/* Run middle priority thread (starves the low priority thread) */
	beginthread(test_priority_mthr, 5, priority_common.stack[1], sizeof(priority_common.stack[1]), NULL);

	/* Finished test setup (signal low priority thread) */
	completion_finish(&priority_common.scomp);

	/* Run high priority thread */
	beginthread(test_priority_hthr, 0, priority_common.stack[2], sizeof(priority_common.stack[2]), NULL);
	completion_wait(&priority_common.comp);

	/* Join the threads */
	threadJoin(-1, 0);
	threadJoin(-1, 0);
	threadJoin(-1, 0);

	/* Clean up completions */
	completion_done(&priority_common.comp);
	completion_done(&priority_common.lcomp);
	completion_done(&priority_common.scomp);

	/* Test finished successfully */
	exit(EXIT_SUCCESS);
}


int main(void)
{
	char stack[256] __attribute__((aligned(8)));

	/* Run priority inversion test thread */
	beginthread(test_priority_inversion, 4, stack, sizeof(stack), NULL);

	/* Wait 2s for test to complete (after that assume deadlock and fail) */
	sleep(2);

	return EXIT_FAILURE;
}
