/*
 * Phoenix-RTOS
 *
 * phoenix-rtos-tests
 *
 * test/thread-local
 *
 * Copyright 2022 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <limits.h>

#include "unity_fixture.h"
#include "tls_functions.h"


TEST_GROUP(test_tls);

TEST_SETUP(test_tls)
{
}

TEST_TEAR_DOWN(test_tls)
{
}

TEST(test_tls, test_tls_defaults)
{
	tls_check_t results[THREAD_NUM + 1];
	pthread_t threads[THREAD_NUM];

	tls_assign_defaults(&results[THREAD_NUM]);

	for (int i = 0; i < THREAD_NUM; i++) {
		TEST_ASSERT_EQUAL(0, pthread_create(&threads[i], NULL, tls_assign_defaults, &results[i]));
	}
	for (int i = 0; i < THREAD_NUM; i++) {
		TEST_ASSERT_EQUAL(0, pthread_join(threads[i], NULL));
	}

	for (int i = 0; i < THREAD_NUM + 1; i++) {
		TEST_ASSERT_EQUAL(0, results[i].actual_tbss_value);
		TEST_ASSERT_EQUAL(3, results[i].actual_tdata_value);
		for (int j = i + 1; j < THREAD_NUM; j++) {
			TEST_ASSERT_NOT_EQUAL(results[i].tbss_value_addr, results[j].tbss_value_addr);
			TEST_ASSERT_NOT_EQUAL(results[i].tdata_value_addr, results[j].tdata_value_addr);
			TEST_ASSERT_NOT_EQUAL(results[i].tbss_value_addr, results[i].tdata_value_addr);
		}
	}
}

TEST(test_tls, test_tls_set_tls_variables)
{
	tls_check_t results[THREAD_NUM + 1];
	pthread_t threads[THREAD_NUM];
	srand(420);

	results[THREAD_NUM].expected_tbss_value = rand() % (INT_MAX / 2);
	results[THREAD_NUM].expected_tdata_value = rand() % (INT_MAX / 2);
	tls_change_variables(&results[THREAD_NUM]);

	for (int i = 0; i < THREAD_NUM; i++) {
		results[i].expected_tbss_value = rand() % (INT_MAX / 2);
		results[i].expected_tdata_value = rand() % (INT_MAX / 2);
		TEST_ASSERT_EQUAL(0, pthread_create(&threads[i], NULL, tls_change_variables, &results[i]));
	}
	for (int i = 0; i < THREAD_NUM; i++) {
		TEST_ASSERT_EQUAL(0, pthread_join(threads[i], NULL));
	}
	for (int i = 0; i < THREAD_NUM + 1; i++) {
		TEST_ASSERT_EQUAL(CHECKS, results[i].passed);
		for (int j = i + 1; j < THREAD_NUM + 1; j++) {
			TEST_ASSERT_NOT_EQUAL(results[i].tbss_value_addr, results[j].tbss_value_addr);
			TEST_ASSERT_NOT_EQUAL(results[i].tdata_value_addr, results[j].tdata_value_addr);
			TEST_ASSERT_NOT_EQUAL(results[i].tbss_value_addr, results[i].tdata_value_addr);
		}
	}
}

TEST(test_tls, test_tls_check_errno)
{
	tls_errno_check_t results[THREAD_NUM + 1];
	pthread_t threads[THREAD_NUM];

	results[THREAD_NUM].expected_tls_errno = -ETIME;
	tls_check_errno(&results[THREAD_NUM]);

	for (int i = 0; i < THREAD_NUM; i++) {
		results[i].expected_tls_errno = -ETIME;
		TEST_ASSERT_EQUAL(0, pthread_create(&threads[i], NULL, tls_check_errno, &results[i]));
	}
	for (int i = 0; i < THREAD_NUM; i++) {
		TEST_ASSERT_EQUAL(0, pthread_join(threads[i], NULL));
	}
	for (int i = 0; i < THREAD_NUM + 1; i++) {
		TEST_ASSERT_EQUAL(ERRNO_CHECKS, results[i].passed);
		for (int j = i + 1; j < THREAD_NUM + 1; j++) {
			TEST_ASSERT_NOT_EQUAL(results[i].errno_addr, results[j].errno_addr);
		}
	}
}

TEST_GROUP_RUNNER(test_tls)
{
	RUN_TEST_CASE(test_tls, test_tls_defaults);
	RUN_TEST_CASE(test_tls, test_tls_set_tls_variables);
	RUN_TEST_CASE(test_tls, test_tls_check_errno);
}

void runner(void)
{
	RUN_TEST_GROUP(test_tls);
}

int main(int argc, char **argv)
{
	UnityMain(argc, (const char **)argv, runner);
	return 0;
}
