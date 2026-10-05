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

#ifndef _TEST_LIBC_SEMAPHORE_COMMON_H
#define _TEST_LIBC_SEMAPHORE_COMMON_H

#include <time.h>


/* absolute deadline `ms` milliseconds from now, on the clock sem_timedwait() uses */
void sem_test_deadline(struct timespec *ts, int ms);


/* absolute deadline `ms` milliseconds from now, on `clock` */
void sem_test_deadlineOn(struct timespec *ts, clockid_t clock, int ms);


/* milliseconds elapsed since `start` */
long sem_test_elapsedMs(const struct timespec *start);


/* milliseconds elapsed since `start`, measured on `clock` */
long sem_test_elapsedMsOn(const struct timespec *start, clockid_t clock);


/*
 * Moves CLOCK_REALTIME `deltaSec` seconds forward so that it can be told apart
 * from CLOCK_MONOTONIC, which otherwise reads the same on a board with no RTC.
 * Returns 0, or -1 if the clock cannot be set. Undo with sem_test_clockRestore().
 */
int sem_test_clockShift(long deltaSec);


/* undoes sem_test_clockShift(); safe to call when no shift is in effect */
void sem_test_clockRestore(void);


#endif /* _TEST_LIBC_SEMAPHORE_COMMON_H */
