/*
 * Phoenix-RTOS
 *
 * phoenix-rtos-tests
 *
 * test/pthread
 *
 * Copyright 2022 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _TEST_PTHREAD_THREAD_FUNCTIONS_H_
#define _TEST_PTHREAD_THREAD_FUNCTIONS_H_


#include <sys/types.h>


typedef struct thread_err_t {
	int err1;
	int err2;
	int err3;
} thread_err_t;


typedef struct thread_args_t {
	pthread_mutex_t count_lock;
	pthread_cond_t count_nonzero;
	int count;
} thread_args_t;


thread_args_t thread_args;


void *decrement_count_wait(void *args);


void *decrement_count_timed_wait_pass(void *args);


void *decrement_count_timed_wait_fail_incorrect_timeout(void *args);


void *decrement_count_timed_wait_fail_too_short_timeout(void *args);


void *increment_count_signal(void *args);


void *increment_count_broadcast(void *args);


#endif /* _TEST_PTHREAD_THREAD_FUNCTIONS_H_ */
