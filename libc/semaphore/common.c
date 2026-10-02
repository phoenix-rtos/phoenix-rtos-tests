/*
 * Phoenix-RTOS
 *
 * libc/semaphore
 *
 * shared helpers for the semaphore tests
 *
 * Copyright 2026 Phoenix Systems
 * Author: Ziemowit Leszczynski
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <time.h>

#include "common.h"

#define NSEC_PER_SEC  1000000000L
#define NSEC_PER_MSEC 1000000L


void sem_test_deadline(struct timespec *ts, int ms)
{
	clock_gettime(CLOCK_REALTIME, ts);

	ts->tv_sec += ms / 1000;
	ts->tv_nsec += (long)(ms % 1000) * NSEC_PER_MSEC;

	if (ts->tv_nsec >= NSEC_PER_SEC) {
		ts->tv_nsec -= NSEC_PER_SEC;
		ts->tv_sec += 1;
	}
}


long sem_test_elapsedMs(const struct timespec *start)
{
	struct timespec now;

	clock_gettime(CLOCK_REALTIME, &now);

	return (long)(now.tv_sec - start->tv_sec) * 1000L +
			(now.tv_nsec - start->tv_nsec) / NSEC_PER_MSEC;
}
