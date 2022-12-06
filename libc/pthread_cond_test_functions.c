/*
 * Phoenix-RTOS
 *
 * phoenix-rtos-tests
 *
 * test/libc/pthread
 *
 * Copyright 2022 Phoenix Systems
 * Author: Lukasz Leczkowski, Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "pthread_cond_test_functions.h"
#include "unity_fixture.h"
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>

thread_args_t thread_args;

void *decrement_count_wait(void *args)
{
	thread_err_t *thread_err = (thread_err_t *)args;
	thread_err->err1 = pthread_mutex_lock(&thread_args.count_lock);
	while (thread_args.count == 0) {
		thread_err->err2 = pthread_cond_wait(&thread_args.count_nonzero, &thread_args.count_lock);
	}
	thread_err->err3 = pthread_mutex_unlock(&thread_args.count_lock);
	return NULL;
}


void *decrement_count_timed_wait_pass(void *args)
{
	thread_err_t *thread_err = (thread_err_t *)args;
	struct timespec time;
	clock_gettime(CLOCK_REALTIME, &time);
	time.tv_sec += 15;
	thread_err->err1 = pthread_mutex_lock(&thread_args.count_lock);
	while (thread_args.count == 0) {
		thread_err->err2 = pthread_cond_timedwait(&thread_args.count_nonzero, &thread_args.count_lock, &time);
	}
	thread_err->err3 = pthread_mutex_unlock(&thread_args.count_lock);
	return NULL;
}


void *decrement_count_timed_wait_fail_incorrect_timeout(void *args)
{
	thread_err_t *thread_err = (thread_err_t *)args;
	struct timespec time;
	/* to set past time and avoid overflow */
	time.tv_sec = 0;
	time.tv_nsec = 0;
	thread_err->err1 = pthread_mutex_lock(&thread_args.count_lock);
	while (thread_args.count == 0) {
		thread_err->err2 = pthread_cond_timedwait(&thread_args.count_nonzero, &thread_args.count_lock, &time);
		break;
	}
	thread_err->err3 = pthread_mutex_unlock(&thread_args.count_lock);
	return NULL;
}


void *increment_count_signal(void *args)
{
	thread_err_t *thread_err = (thread_err_t *)args;
	usleep(10000);
	thread_err->err1 = pthread_mutex_lock(&thread_args.count_lock);
	if (thread_args.count == 0) {
		thread_err->err2 = pthread_cond_signal(&thread_args.count_nonzero);
	}
	thread_args.count = thread_args.count + 1;
	thread_err->err3 = pthread_mutex_unlock(&thread_args.count_lock);
	return NULL;
}


void *increment_count_broadcast(void *args)
{
	thread_err_t *thread_err = (thread_err_t *)args;
	usleep(10000);
	thread_err->err1 = pthread_mutex_lock(&thread_args.count_lock);
	if (thread_args.count == 0) {
		thread_err->err2 = pthread_cond_broadcast(&thread_args.count_nonzero);
	}
	thread_args.count = thread_args.count + 2;
	thread_err->err3 = pthread_mutex_unlock(&thread_args.count_lock);
	return NULL;
}
