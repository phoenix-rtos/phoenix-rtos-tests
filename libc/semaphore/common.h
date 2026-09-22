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


/* milliseconds elapsed since `start` */
long sem_test_elapsedMs(const struct timespec *start);


#endif /* _TEST_LIBC_SEMAPHORE_COMMON_H */
