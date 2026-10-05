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


void sem_test_deadlineOn(struct timespec *ts, clockid_t clock, int ms)
{
	clock_gettime(clock, ts);

	ts->tv_sec += ms / 1000;
	ts->tv_nsec += (long)(ms % 1000) * NSEC_PER_MSEC;

	if (ts->tv_nsec >= NSEC_PER_SEC) {
		ts->tv_nsec -= NSEC_PER_SEC;
		ts->tv_sec += 1;
	}
}


void sem_test_deadline(struct timespec *ts, int ms)
{
	sem_test_deadlineOn(ts, CLOCK_REALTIME, ms);
}


long sem_test_elapsedMsOn(const struct timespec *start, clockid_t clock)
{
	struct timespec now;

	clock_gettime(clock, &now);

	long sec_diff = now.tv_sec - start->tv_sec;
	long nsec_diff = now.tv_nsec - start->tv_nsec;

	if (nsec_diff < 0) {
		sec_diff--;
		nsec_diff += NSEC_PER_SEC;
	}

	return (sec_diff * 1000L + nsec_diff / NSEC_PER_MSEC);
}


long sem_test_elapsedMs(const struct timespec *start)
{
	return sem_test_elapsedMsOn(start, CLOCK_REALTIME);
}


static long clockShiftDelta;
static int clockShifted;


int sem_test_clockShift(long deltaSec)
{
	struct timespec real;

	if (clock_gettime(CLOCK_REALTIME, &real) != 0) {
		return -1;
	}

	real.tv_sec += deltaSec;

	/* fails without privilege, which is the usual case on a hosted build */
	if (clock_settime(CLOCK_REALTIME, &real) != 0) {
		return -1;
	}

	clockShiftDelta = deltaSec;
	clockShifted = 1;

	return 0;
}


void sem_test_clockRestore(void)
{
	struct timespec real;

	if (clockShifted == 0) {
		return;
	}

	/* cleared first: a failed restore must not be retried on every teardown */
	clockShifted = 0;

	if (clock_gettime(CLOCK_REALTIME, &real) == 0) {
		real.tv_sec -= clockShiftDelta;
		(void)clock_settime(CLOCK_REALTIME, &real);
	}
}
