/*
 * Phoenix-RTOS
 *
 * test-libc-pthread
 *
 * Main entry point.
 *
 * Copyright 2023, 2026 Phoenix Systems
 * Author: Mateusz Bloch, Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "unity_fixture.h"

void runner(void)
{
	RUN_TEST_GROUP(test_pthread_cond);
	RUN_TEST_GROUP(test_pthread_cleanup);
	RUN_TEST_GROUP(pthread_attr);
	RUN_TEST_GROUP(pthread_attr_guardsize);
	RUN_TEST_GROUP(pthread_attr_inheritsched);
	RUN_TEST_GROUP(pthread_attr_setscope);
	RUN_TEST_GROUP(pthread_attr_setstack);
	RUN_TEST_GROUP(pthread_mutex);
	RUN_TEST_GROUP(pthread_mutex_consistent);
	RUN_TEST_GROUP(pthread_mutex_prioceiling);
	RUN_TEST_GROUP(pthread_mutex_timedlock);
	RUN_TEST_GROUP(pthread_mutexattr);
	RUN_TEST_GROUP(pthread_mutexattr_prioceiling);
	RUN_TEST_GROUP(pthread_mutexattr_protocol);
	RUN_TEST_GROUP(pthread_mutexattr_pshared);
	RUN_TEST_GROUP(pthread_mutexattr_robust);
	RUN_TEST_GROUP(pthread_lifecycle);
	RUN_TEST_GROUP(pthread_once);
	RUN_TEST_GROUP(pthread_key);
	RUN_TEST_GROUP(pthread_cancel);
	RUN_TEST_GROUP(pthread_cancel_type);
	RUN_TEST_GROUP(pthread_atfork);
	RUN_TEST_GROUP(pthread_kill);
	RUN_TEST_GROUP(pthread_sigmask);
	RUN_TEST_GROUP(pthread_rwlock);
	RUN_TEST_GROUP(pthread_rwlockattr);
	RUN_TEST_GROUP(pthread_spin);
	RUN_TEST_GROUP(pthread_sched);
	RUN_TEST_GROUP(pthread_getcpuclockid);
}


int main(int argc, char *argv[])
{
	const char *var = "POSIXLY_CORRECT";

	if (setenv(var, "y", 1) != 0) {
		fprintf(stderr, "Setting %s environment variable failed: %s\n", var, strerror(errno));
		return 1;
	}

	return (UnityMain(argc, (const char **)argv, runner) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
