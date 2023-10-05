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

#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>

#include "unity_fixture.h"
#include "pthread_cond_test_functions.h"


TEST_GROUP(test_pthread_cond);


TEST_SETUP(test_pthread_cond)
{
}


TEST_TEAR_DOWN(test_pthread_cond)
{
}


TEST(test_pthread_cond, pthread_condattr_setclock)
{
	pthread_condattr_t attr;
	TEST_ASSERT_EQUAL(0, pthread_condattr_init(&attr));

	clockid_t clock;

	TEST_ASSERT_EQUAL(0, pthread_condattr_setclock(&attr, CLOCK_MONOTONIC));
	TEST_ASSERT_EQUAL(0, pthread_condattr_getclock(&attr, &clock));
	TEST_ASSERT_EQUAL(CLOCK_MONOTONIC, clock);

	/* glibc don't want to use CLOCK_MONOTONIC_RAW */
#ifdef __phoenix__
	TEST_ASSERT_EQUAL(0, pthread_condattr_setclock(&attr, CLOCK_MONOTONIC_RAW));
	TEST_ASSERT_EQUAL(0, pthread_condattr_getclock(&attr, &clock));
	TEST_ASSERT_EQUAL(CLOCK_MONOTONIC_RAW, clock);
#endif

	TEST_ASSERT_EQUAL(0, pthread_condattr_setclock(&attr, CLOCK_REALTIME));
	TEST_ASSERT_EQUAL(0, pthread_condattr_getclock(&attr, &clock));
	TEST_ASSERT_EQUAL(CLOCK_REALTIME, clock);
}


TEST(test_pthread_cond, pthread_condattr_setpshared)
{
	pthread_condattr_t attr;
	TEST_ASSERT_EQUAL(0, pthread_condattr_init(&attr));
	TEST_ASSERT_EQUAL(0, pthread_condattr_setpshared(&attr, PTHREAD_PROCESS_PRIVATE));

	int pshared;
	/* Only 'PTHREAD_PROCESS_PRIVATE' supported on Phoenix-RTOS */
	TEST_ASSERT_EQUAL(0, pthread_condattr_getpshared(&attr, &pshared));
	TEST_ASSERT_EQUAL(PTHREAD_PROCESS_PRIVATE, pshared);
#ifdef __phoenix__
	TEST_ASSERT_EQUAL(EINVAL, pthread_condattr_setpshared(&attr, PTHREAD_PROCESS_SHARED));
#else
	TEST_ASSERT_EQUAL(0, pthread_condattr_setpshared(&attr, PTHREAD_PROCESS_SHARED));
	TEST_ASSERT_EQUAL(0, pthread_condattr_getpshared(&attr, &pshared));
	TEST_ASSERT_EQUAL(PTHREAD_PROCESS_SHARED, pshared);
#endif
}


TEST(test_pthread_cond, pthread_cond_init)
{
	pthread_cond_t cond;
	TEST_ASSERT_EQUAL(0, pthread_cond_init(&cond, NULL));
}


TEST(test_pthread_cond, pthread_cond_wait_signal)
{
	pthread_t first, second;
	thread_args.count = 0;
	thread_err_t err_first, err_second;

	TEST_ASSERT_EQUAL(0, pthread_mutex_init(&thread_args.count_lock, NULL));
	TEST_ASSERT_EQUAL(0, pthread_cond_init(&thread_args.count_nonzero, NULL));
	TEST_ASSERT_EQUAL(0, pthread_create(&first, NULL, decrement_count_wait, &err_first));
	TEST_ASSERT_EQUAL(0, pthread_create(&second, NULL, increment_count_signal, &err_second));
	TEST_ASSERT_EQUAL(0, pthread_join(first, NULL));
	TEST_ASSERT_EQUAL(0, pthread_join(second, NULL));

	TEST_ASSERT_EQUAL(0, err_first.err1);
	TEST_ASSERT_EQUAL(0, err_first.err2);
	TEST_ASSERT_EQUAL(0, err_first.err3);
	TEST_ASSERT_EQUAL(0, err_second.err1);
	TEST_ASSERT_EQUAL(0, err_second.err2);
	TEST_ASSERT_EQUAL(0, err_second.err3);
}


TEST(test_pthread_cond, pthread_cond_wait_broadcast)
{
	pthread_t first, second, third;
	thread_args.count = 0;
	thread_err_t err_first, err_second, err_third;

	TEST_ASSERT_EQUAL(0, pthread_mutex_init(&thread_args.count_lock, NULL));
	TEST_ASSERT_EQUAL(0, pthread_cond_init(&thread_args.count_nonzero, NULL));
	TEST_ASSERT_EQUAL(0, pthread_create(&first, NULL, decrement_count_wait, &err_first));
	TEST_ASSERT_EQUAL(0, pthread_create(&second, NULL, decrement_count_wait, &err_second));
	TEST_ASSERT_EQUAL(0, pthread_create(&third, NULL, increment_count_broadcast, &err_third));
	TEST_ASSERT_EQUAL(0, pthread_join(first, NULL));
	TEST_ASSERT_EQUAL(0, pthread_join(second, NULL));
	TEST_ASSERT_EQUAL(0, pthread_join(third, NULL));

	TEST_ASSERT_EQUAL(0, err_first.err1);
	TEST_ASSERT_EQUAL(0, err_first.err2);
	TEST_ASSERT_EQUAL(0, err_first.err3);
	TEST_ASSERT_EQUAL(0, err_second.err1);
	TEST_ASSERT_EQUAL(0, err_second.err2);
	TEST_ASSERT_EQUAL(0, err_second.err3);
	TEST_ASSERT_EQUAL(0, err_third.err1);
	TEST_ASSERT_EQUAL(0, err_third.err2);
	TEST_ASSERT_EQUAL(0, err_third.err3);
}


TEST(test_pthread_cond, pthread_cond_timedwait_pass_signal)
{
	pthread_t first, second;
	thread_args.count = 0;
	thread_err_t err_first, err_second;

	TEST_ASSERT_EQUAL(0, pthread_mutex_init(&thread_args.count_lock, NULL));
	TEST_ASSERT_EQUAL(0, pthread_cond_init(&thread_args.count_nonzero, NULL));
	TEST_ASSERT_EQUAL(0, pthread_create(&first, NULL, decrement_count_timed_wait_pass, &err_first));
	TEST_ASSERT_EQUAL(0, pthread_create(&second, NULL, increment_count_signal, &err_second));
	TEST_ASSERT_EQUAL(0, pthread_join(first, NULL));
	TEST_ASSERT_EQUAL(0, pthread_join(second, NULL));

	TEST_ASSERT_EQUAL(0, err_first.err1);
	TEST_ASSERT_EQUAL(0, err_first.err2);
	TEST_ASSERT_EQUAL(0, err_first.err3);
	TEST_ASSERT_EQUAL(0, err_second.err1);
	TEST_ASSERT_EQUAL(0, err_second.err2);
	TEST_ASSERT_EQUAL(0, err_second.err3);
}


TEST(test_pthread_cond, pthread_cond_timedwait_fail_signal_incorrect_timeout)
{
	pthread_t first, second;
	thread_args.count = 0;
	thread_err_t err_first, err_second;

	TEST_ASSERT_EQUAL(0, pthread_mutex_init(&thread_args.count_lock, NULL));
	TEST_ASSERT_EQUAL(0, pthread_cond_init(&thread_args.count_nonzero, NULL));
	TEST_ASSERT_EQUAL(0, pthread_create(&first, NULL, decrement_count_timed_wait_fail_incorrect_timeout, &err_first));
	TEST_ASSERT_EQUAL(0, pthread_create(&second, NULL, increment_count_signal, &err_second));
	TEST_ASSERT_EQUAL(0, pthread_join(first, NULL));
	TEST_ASSERT_EQUAL(0, pthread_join(second, NULL));

	TEST_ASSERT_EQUAL(0, err_first.err1);
	TEST_ASSERT_EQUAL(ETIMEDOUT, err_first.err2);
	TEST_ASSERT_EQUAL(0, err_first.err3);
	TEST_ASSERT_EQUAL(0, err_second.err1);
	TEST_ASSERT_EQUAL(0, err_second.err2);
	TEST_ASSERT_EQUAL(0, err_second.err3);
}


TEST(test_pthread_cond, pthread_cond_timedwait_pass_broadcast)
{
	pthread_t first, second, third;
	thread_args.count = 0;
	thread_err_t err_first, err_second, err_third;

	TEST_ASSERT_EQUAL(0, pthread_mutex_init(&thread_args.count_lock, NULL));
	TEST_ASSERT_EQUAL(0, pthread_cond_init(&thread_args.count_nonzero, NULL));
	TEST_ASSERT_EQUAL(0, pthread_create(&first, NULL, decrement_count_timed_wait_pass, &err_first));
	TEST_ASSERT_EQUAL(0, pthread_create(&second, NULL, decrement_count_timed_wait_pass, &err_second));
	TEST_ASSERT_EQUAL(0, pthread_create(&third, NULL, increment_count_broadcast, &err_third));
	TEST_ASSERT_EQUAL(0, pthread_join(first, NULL));
	TEST_ASSERT_EQUAL(0, pthread_join(second, NULL));
	TEST_ASSERT_EQUAL(0, pthread_join(third, NULL));

	TEST_ASSERT_EQUAL(0, err_first.err1);
	TEST_ASSERT_EQUAL(0, err_first.err2);
	TEST_ASSERT_EQUAL(0, err_first.err3);
	TEST_ASSERT_EQUAL(0, err_second.err1);
	TEST_ASSERT_EQUAL(0, err_second.err2);
	TEST_ASSERT_EQUAL(0, err_second.err3);
	TEST_ASSERT_EQUAL(0, err_third.err1);
	TEST_ASSERT_EQUAL(0, err_third.err2);
	TEST_ASSERT_EQUAL(0, err_third.err3);
}


TEST(test_pthread_cond, pthread_cond_timedwait_fail_broadcast_incorrect_timeout)
{
	pthread_t first, second, third;
	thread_args.count = 0;
	thread_err_t err_first, err_second, err_third;

	TEST_ASSERT_EQUAL(0, pthread_mutex_init(&thread_args.count_lock, NULL));
	TEST_ASSERT_EQUAL(0, pthread_cond_init(&thread_args.count_nonzero, NULL));
	TEST_ASSERT_EQUAL(0, pthread_create(&first, NULL, decrement_count_timed_wait_fail_incorrect_timeout, &err_first));
	TEST_ASSERT_EQUAL(0, pthread_create(&second, NULL, decrement_count_timed_wait_fail_incorrect_timeout, &err_second));
	TEST_ASSERT_EQUAL(0, pthread_create(&third, NULL, increment_count_broadcast, &err_third));
	TEST_ASSERT_EQUAL(0, pthread_join(first, NULL));
	TEST_ASSERT_EQUAL(0, pthread_join(second, NULL));
	TEST_ASSERT_EQUAL(0, pthread_join(third, NULL));

	TEST_ASSERT_EQUAL(0, err_first.err1);
	TEST_ASSERT_EQUAL(ETIMEDOUT, err_first.err2);
	TEST_ASSERT_EQUAL(0, err_first.err3);
	TEST_ASSERT_EQUAL(0, err_second.err1);
	TEST_ASSERT_EQUAL(ETIMEDOUT, err_second.err2);
	TEST_ASSERT_EQUAL(0, err_second.err3);
	TEST_ASSERT_EQUAL(0, err_third.err1);
	TEST_ASSERT_EQUAL(0, err_third.err2);
	TEST_ASSERT_EQUAL(0, err_third.err3);
}


TEST_GROUP_RUNNER(test_pthread_cond)
{
	RUN_TEST_CASE(test_pthread_cond, pthread_cond_init);
	RUN_TEST_CASE(test_pthread_cond, pthread_condattr_setclock);
	RUN_TEST_CASE(test_pthread_cond, pthread_condattr_setpshared);
	RUN_TEST_CASE(test_pthread_cond, pthread_cond_wait_signal);
	RUN_TEST_CASE(test_pthread_cond, pthread_cond_wait_broadcast);
	RUN_TEST_CASE(test_pthread_cond, pthread_cond_timedwait_pass_signal);
	RUN_TEST_CASE(test_pthread_cond, pthread_cond_timedwait_fail_signal_incorrect_timeout);
	RUN_TEST_CASE(test_pthread_cond, pthread_cond_timedwait_pass_broadcast);
	RUN_TEST_CASE(test_pthread_cond, pthread_cond_timedwait_fail_broadcast_incorrect_timeout);
}
