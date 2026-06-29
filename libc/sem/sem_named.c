/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <semaphore.h>
 * TESTED:
 *    - sem_close()
 *    - sem_open()
 *    - sem_unlink()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include "unity_fixture.h"

#ifndef __phoenix__
#include <semaphore.h>
#define SEM_TEST_NAME "/test_sem_named"

/* Tests: sem_open, sem_close, sem_unlink */
TEST_GROUP(sem_named);

static struct {
	sem_t *sem;
} test_common;


TEST_SETUP(sem_named)
{
	test_common.sem = SEM_FAILED;
	/* Clean up any leftover from previous run */
	sem_unlink(SEM_TEST_NAME);
}

TEST_TEAR_DOWN(sem_named)
{
	if (test_common.sem != SEM_FAILED) {
		sem_close(test_common.sem);
		test_common.sem = SEM_FAILED;
	}
	sem_unlink(SEM_TEST_NAME);
}


TEST(sem_named, open_create_new)
{
	errno = 0;
	test_common.sem = sem_open(SEM_TEST_NAME, O_CREAT | O_EXCL, 0644, 1);
	TEST_ASSERT_TRUE(test_common.sem != SEM_FAILED);
	TEST_ASSERT_EQUAL_INT(0, errno);
}


TEST(sem_named, open_create_initial_value)
{
	int ret;
	int sval;

	test_common.sem = sem_open(SEM_TEST_NAME, O_CREAT | O_EXCL, 0644, 5);
	TEST_ASSERT_TRUE(test_common.sem != SEM_FAILED);

	ret = sem_getvalue(test_common.sem, &sval);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(5, sval);
}


TEST(sem_named, open_existing)
{
	sem_t *sem2;

	test_common.sem = sem_open(SEM_TEST_NAME, O_CREAT | O_EXCL, 0644, 3);
	TEST_ASSERT_TRUE(test_common.sem != SEM_FAILED);

	/* Open existing without O_EXCL */
	sem2 = sem_open(SEM_TEST_NAME, 0);
	TEST_ASSERT_TRUE(sem2 != SEM_FAILED);

	sem_close(sem2);
}


TEST(sem_named, open_eexist_with_excl)
{
	sem_t *sem2;

	test_common.sem = sem_open(SEM_TEST_NAME, O_CREAT | O_EXCL, 0644, 1);
	TEST_ASSERT_TRUE(test_common.sem != SEM_FAILED);

	/* Attempt to create again with O_EXCL should fail */
	errno = 0;
	sem2 = sem_open(SEM_TEST_NAME, O_CREAT | O_EXCL, 0644, 1);
	TEST_ASSERT_TRUE(sem2 == SEM_FAILED);
	TEST_ASSERT_EQUAL_INT(EEXIST, errno);
}


TEST(sem_named, open_enoent_no_create)
{
	sem_t *sem2;

	/* Attempt to open non-existent semaphore without O_CREAT */
	errno = 0;
	sem2 = sem_open("/nonexistent_sem_xyzzy_99", 0);
	TEST_ASSERT_TRUE(sem2 == SEM_FAILED);
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);
}


TEST(sem_named, open_einval_value_exceeds_max)
{
	sem_t *semp;

	errno = 0;
	semp = sem_open(SEM_TEST_NAME, O_CREAT | O_EXCL, 0644, (unsigned int)SEM_VALUE_MAX + 1U);
	TEST_ASSERT_TRUE(semp == SEM_FAILED);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST(sem_named, close_success)
{
	int ret;

	test_common.sem = sem_open(SEM_TEST_NAME, O_CREAT | O_EXCL, 0644, 1);
	TEST_ASSERT_TRUE(test_common.sem != SEM_FAILED);

	errno = 0;
	ret = sem_close(test_common.sem);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, errno);
	test_common.sem = SEM_FAILED;
}


TEST(sem_named, unlink_success)
{
	int ret;

	test_common.sem = sem_open(SEM_TEST_NAME, O_CREAT | O_EXCL, 0644, 1);
	TEST_ASSERT_TRUE(test_common.sem != SEM_FAILED);

	errno = 0;
	ret = sem_unlink(SEM_TEST_NAME);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, errno);
}


TEST(sem_named, unlink_enoent_nonexistent)
{
	int ret;

	errno = 0;
	ret = sem_unlink("/nonexistent_sem_xyzzy_99");
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);
}


TEST(sem_named, unlink_removes_name)
{
	int ret;
	sem_t *sem2;

	test_common.sem = sem_open(SEM_TEST_NAME, O_CREAT | O_EXCL, 0644, 1);
	TEST_ASSERT_TRUE(test_common.sem != SEM_FAILED);

	ret = sem_unlink(SEM_TEST_NAME);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* After unlink, opening with same name without O_CREAT should fail */
	errno = 0;
	sem2 = sem_open(SEM_TEST_NAME, 0);
	TEST_ASSERT_TRUE(sem2 == SEM_FAILED);
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);
}


TEST(sem_named, usable_after_open)
{
	int ret;
	int sval;

	test_common.sem = sem_open(SEM_TEST_NAME, O_CREAT | O_EXCL, 0644, 0);
	TEST_ASSERT_TRUE(test_common.sem != SEM_FAILED);

	ret = sem_post(test_common.sem);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = sem_getvalue(test_common.sem, &sval);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(1, sval);

	ret = sem_wait(test_common.sem);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = sem_getvalue(test_common.sem, &sval);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, sval);
}


TEST(sem_named, open_same_name_returns_same_address)
{
	sem_t *sem2;

	test_common.sem = sem_open(SEM_TEST_NAME, O_CREAT | O_EXCL, 0644, 1);
	TEST_ASSERT_TRUE(test_common.sem != SEM_FAILED);

	/* POSIX: same name shall return same address if not closed/unlinked */
	sem2 = sem_open(SEM_TEST_NAME, 0);
	TEST_ASSERT_TRUE(sem2 != SEM_FAILED);
	TEST_ASSERT_TRUE(sem2 == test_common.sem);

	sem_close(sem2);
}


TEST(sem_named, unlink_sem_still_usable_until_close)
{
	int ret;
	int sval;

	test_common.sem = sem_open(SEM_TEST_NAME, O_CREAT | O_EXCL, 0644, 2);
	TEST_ASSERT_TRUE(test_common.sem != SEM_FAILED);

	/* Unlink while still open */
	ret = sem_unlink(SEM_TEST_NAME);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Semaphore should still be usable */
	ret = sem_post(test_common.sem);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = sem_getvalue(test_common.sem, &sval);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(3, sval);
}

TEST_GROUP_RUNNER(sem_named)
{
	RUN_TEST_CASE(sem_named, open_create_new);
	RUN_TEST_CASE(sem_named, open_create_initial_value);
	RUN_TEST_CASE(sem_named, open_existing);
	RUN_TEST_CASE(sem_named, open_eexist_with_excl);
	RUN_TEST_CASE(sem_named, open_enoent_no_create);
	RUN_TEST_CASE(sem_named, open_einval_value_exceeds_max);
	RUN_TEST_CASE(sem_named, close_success);
	RUN_TEST_CASE(sem_named, unlink_success);
	RUN_TEST_CASE(sem_named, unlink_enoent_nonexistent);
	RUN_TEST_CASE(sem_named, unlink_removes_name);
	RUN_TEST_CASE(sem_named, usable_after_open);
	RUN_TEST_CASE(sem_named, open_same_name_returns_same_address);
	RUN_TEST_CASE(sem_named, unlink_sem_still_usable_until_close);
}
#else
TEST_GROUP_UNIMPLEMENTED(sem_named, "semaphore.h is non-existent")
#endif
