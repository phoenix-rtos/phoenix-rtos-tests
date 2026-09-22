/*
 * Phoenix-RTOS
 *
 * libc/semaphore
 *
 * tests for named POSIX semaphores (sem_open/sem_close/sem_unlink)
 *
 * Copyright 2026 Phoenix Systems
 * Author: Ziemowit Leszczynski
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "unity_fixture.h"

#include "common.h"

#define SEM_NAME    "/test_libc_sem"
#define SEM_NAME_2  "/test_libc_sem2"
#define SEM_NAME_N  "/test_libc_semN%d"
#define WAITERS_CNT 4

/* how far past the advertised limit the exhaustion test is willing to go */
#define NSEMS_SLACK 8

static sem_t *sem, *sem2;
static sem_t *waiterSem;
static int waiterRet[WAITERS_CNT];
static int nsemsCreated;


static void named_unlinkRange(int count)
{
	char name[64];
	int i;

	for (i = 0; i < count; i++) {
		snprintf(name, sizeof(name), SEM_NAME_N, i);
		sem_unlink(name);
	}
}


static void *named_waiter(void *arg)
{
	int *ret = (int *)arg;

	*ret = sem_wait(waiterSem);

	return NULL;
}


TEST_GROUP(sem_named);


TEST_SETUP(sem_named)
{
	sem = NULL;
	sem2 = NULL;
	waiterSem = NULL;
	nsemsCreated = 0;

	/* leftovers from an interrupted run */
	sem_unlink(SEM_NAME);
	sem_unlink(SEM_NAME_2);
}


TEST_TEAR_DOWN(sem_named)
{
	if (sem != NULL) {
		sem_close(sem);
		sem = NULL;
	}

	if (sem2 != NULL) {
		sem_close(sem2);
		sem2 = NULL;
	}

	if (waiterSem != NULL) {
		sem_close(waiterSem);
		waiterSem = NULL;
	}

	sem_unlink(SEM_NAME);
	sem_unlink(SEM_NAME_2);

	/* nsems_max_enforced() may have left some behind if it failed part-way */
	named_unlinkRange(nsemsCreated);
	nsemsCreated = 0;
}


static sem_t *named_create(const char *name, unsigned int value)
{
	sem_t *s = sem_open(name, O_CREAT | O_EXCL, 0666, value);

	TEST_ASSERT_NOT_EQUAL_MESSAGE(SEM_FAILED, s, "sem_open(O_CREAT|O_EXCL) failed");

	return s;
}


TEST(sem_named, open_close)
{
	sem = named_create(SEM_NAME, 1);

	TEST_ASSERT_EQUAL_INT(0, sem_close(sem));
	sem = NULL;
}


TEST(sem_named, open_null_name)
{
#ifndef __phoenix__
	/* POSIX leaves a NULL `name` undefined, and glibc declares sem_open() nonnull */
	TEST_IGNORE_MESSAGE("phoenix-specific");
#else
	errno = 0;
	TEST_ASSERT_EQUAL_PTR(SEM_FAILED, sem_open(NULL, 0));
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
#endif
}


TEST(sem_named, open_missing)
{
	errno = 0;
	TEST_ASSERT_EQUAL_PTR(SEM_FAILED, sem_open(SEM_NAME, 0));
	TEST_ASSERT_NOT_EQUAL_INT(0, errno);
}


TEST(sem_named, open_excl_twice)
{
	sem = named_create(SEM_NAME, 1);

	errno = 0;
	TEST_ASSERT_EQUAL_PTR(SEM_FAILED, sem_open(SEM_NAME, O_CREAT | O_EXCL, 0666, 1u));
	TEST_ASSERT_EQUAL_INT(EEXIST, errno);
}


/* without O_EXCL an existing semaphore must be attached to, not rejected */
TEST(sem_named, open_existing_attaches)
{
	int value = -1;

	sem = named_create(SEM_NAME, 3);

	sem2 = sem_open(SEM_NAME, O_CREAT, 0666, 99u);
	TEST_ASSERT_NOT_EQUAL(SEM_FAILED, sem2);

	/* the existing semaphore, so the original value - not 99 */
	TEST_ASSERT_EQUAL_INT(0, sem_getvalue(sem2, &value));
	TEST_ASSERT_EQUAL_INT(3, value);

	/* and the same object: a wait through one handle is visible through the other */
	TEST_ASSERT_EQUAL_INT(0, sem_wait(sem));
	TEST_ASSERT_EQUAL_INT(0, sem_getvalue(sem2, &value));
	TEST_ASSERT_EQUAL_INT(2, value);
}


TEST(sem_named, open_no_oexcl_creates)
{
	int value = -1;

	sem = sem_open(SEM_NAME, O_CREAT, 0666, 5u);
	TEST_ASSERT_NOT_EQUAL(SEM_FAILED, sem);

	TEST_ASSERT_EQUAL_INT(0, sem_getvalue(sem, &value));
	TEST_ASSERT_EQUAL_INT(5, value);
}


/* a leading '/' is optional and refers to the same semaphore */
TEST(sem_named, open_leading_slash_equivalent)
{
	int value = -1;

	sem = named_create(SEM_NAME, 1);

	sem2 = sem_open(SEM_NAME + 1, 0);
	TEST_ASSERT_NOT_EQUAL(SEM_FAILED, sem2);

	TEST_ASSERT_EQUAL_INT(0, sem_wait(sem2));
	TEST_ASSERT_EQUAL_INT(0, sem_getvalue(sem, &value));
	TEST_ASSERT_EQUAL_INT(0, value);
}


/* the name is bounded, and an oversized one must be rejected, not truncated */
TEST(sem_named, open_name_too_long)
{
	char name[NAME_MAX + 64];

	memset(name, 'a', sizeof(name) - 1);
	name[0] = '/';
	name[sizeof(name) - 1] = '\0';

	errno = 0;
	TEST_ASSERT_EQUAL_PTR(SEM_FAILED, sem_open(name, O_CREAT, 0666, 1u));
#ifdef __phoenix__
	TEST_ASSERT_EQUAL_INT(ENAMETOOLONG, errno);
#else
	/*
	 * glibc prepends "sem." when building /dev/shm/sem.<name>, so it reports
	 * ENAMETOOLONG only for a name of 253..256 characters and EINVAL beyond -
	 * while libphoenix accepts a full NAME_MAX name. No length gives
	 * ENAMETOOLONG on both, so this branch assumes a name past 256.
	 */
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
#endif
}


TEST(sem_named, open_embedded_slash)
{
	errno = 0;
	TEST_ASSERT_EQUAL_PTR(SEM_FAILED, sem_open("/test_libc/sem", O_CREAT, 0666, 1u));
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST(sem_named, open_empty_name)
{
	errno = 0;
	TEST_ASSERT_EQUAL_PTR(SEM_FAILED, sem_open("/", O_CREAT, 0666, 1u));
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


/* the value must actually cross the message boundary */
TEST(sem_named, getvalue_initial)
{
	int value = -1;

	sem = named_create(SEM_NAME, 4);

	TEST_ASSERT_EQUAL_INT(0, sem_getvalue(sem, &value));
	TEST_ASSERT_EQUAL_INT(4, value);
}


TEST(sem_named, wait_post_getvalue)
{
	int value = -1;

	sem = named_create(SEM_NAME, 2);

	TEST_ASSERT_EQUAL_INT(0, sem_wait(sem));
	TEST_ASSERT_EQUAL_INT(0, sem_getvalue(sem, &value));
	TEST_ASSERT_EQUAL_INT(1, value);

	TEST_ASSERT_EQUAL_INT(0, sem_post(sem));
	TEST_ASSERT_EQUAL_INT(0, sem_getvalue(sem, &value));
	TEST_ASSERT_EQUAL_INT(2, value);
}


TEST(sem_named, trywait_success)
{
	sem = named_create(SEM_NAME, 1);

	TEST_ASSERT_EQUAL_INT(0, sem_trywait(sem));
}


TEST(sem_named, trywait_empty)
{
	sem = named_create(SEM_NAME, 0);

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, sem_trywait(sem));
	TEST_ASSERT_EQUAL_INT(EAGAIN, errno);
}


/* SEM_VALUE_MAX itself must be reachable, matching the unnamed path */
TEST(sem_named, post_to_max)
{
	int value = -1;

	sem = named_create(SEM_NAME, (unsigned int)SEM_VALUE_MAX - 1u);

	TEST_ASSERT_EQUAL_INT(0, sem_post(sem));
	TEST_ASSERT_EQUAL_INT(0, sem_getvalue(sem, &value));
	TEST_ASSERT_EQUAL_INT(SEM_VALUE_MAX, value);
}


TEST(sem_named, post_overflow)
{
	int value = -1;

	sem = named_create(SEM_NAME, (unsigned int)SEM_VALUE_MAX);

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, sem_post(sem));
	TEST_ASSERT_EQUAL_INT(EOVERFLOW, errno);

	TEST_ASSERT_EQUAL_INT(0, sem_getvalue(sem, &value));
	TEST_ASSERT_EQUAL_INT(SEM_VALUE_MAX, value);
}


TEST(sem_named, timedwait_success)
{
	struct timespec ts;

	sem = named_create(SEM_NAME, 1);

	sem_test_deadline(&ts, 500);
	TEST_ASSERT_EQUAL_INT(0, sem_timedwait(sem, &ts));
}


TEST(sem_named, timedwait_timeout)
{
	struct timespec ts, start;

	sem = named_create(SEM_NAME, 0);

	clock_gettime(CLOCK_REALTIME, &start);
	sem_test_deadline(&ts, 200);

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, sem_timedwait(sem, &ts));
	TEST_ASSERT_EQUAL_INT(ETIMEDOUT, errno);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(150, sem_test_elapsedMs(&start));
}


/*
 * The server must still be usable after a timeout if the request was cleanly
 * removed from both the wait queue and the timeout tree.
 */
TEST(sem_named, timedwait_timeout_then_reuse)
{
	struct timespec ts;
	int value = -1;
	int i;

	sem = named_create(SEM_NAME, 0);

	for (i = 0; i < 3; i++) {
		sem_test_deadline(&ts, 100);
		errno = 0;
		TEST_ASSERT_EQUAL_INT(-1, sem_timedwait(sem, &ts));
		TEST_ASSERT_EQUAL_INT(ETIMEDOUT, errno);
	}

	TEST_ASSERT_EQUAL_INT(0, sem_post(sem));
	TEST_ASSERT_EQUAL_INT(0, sem_getvalue(sem, &value));
	TEST_ASSERT_EQUAL_INT(1, value);
	TEST_ASSERT_EQUAL_INT(0, sem_wait(sem));
}


TEST(sem_named, timedwait_expired_but_available)
{
	struct timespec ts;

	sem = named_create(SEM_NAME, 1);

	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec -= 10;

	TEST_ASSERT_EQUAL_INT(0, sem_timedwait(sem, &ts));
}


/*
 * The already-expired case is decided by posixsrv, not by the client.
 * The semaphore is empty, so it must come back ETIMEDOUT rather than blocking.
 */
TEST(sem_named, timedwait_expired_and_empty)
{
	struct timespec ts, start;

	sem = named_create(SEM_NAME, 0);

	clock_gettime(CLOCK_REALTIME, &start);
	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec -= 10;

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, sem_timedwait(sem, &ts));
	TEST_ASSERT_EQUAL_INT(ETIMEDOUT, errno);

	/* and it must not have waited for the deadline it was given */
	TEST_ASSERT_LESS_THAN_INT(1000, sem_test_elapsedMs(&start));
}


/*
 * The epoch is an ordinary already-passed deadline, not "no deadline". posixsrv
 * carries the deadline in microseconds, where 0 would otherwise be ambiguous.
 */
TEST(sem_named, timedwait_epoch_and_empty)
{
	struct timespec ts = { .tv_sec = 0, .tv_nsec = 0 };
	struct timespec start;

	sem = named_create(SEM_NAME, 0);

	clock_gettime(CLOCK_REALTIME, &start);

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, sem_timedwait(sem, &ts));
	TEST_ASSERT_EQUAL_INT(ETIMEDOUT, errno);

	TEST_ASSERT_LESS_THAN_INT(1000, sem_test_elapsedMs(&start));
}


TEST(sem_named, timedwait_invalid_nsec)
{
	struct timespec ts;

	sem = named_create(SEM_NAME, 0);

	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_nsec = -1;

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, sem_timedwait(sem, &ts));
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


/* unlinking a name that does not exist must report ENOENT */
TEST(sem_named, unlink_missing)
{
	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, sem_unlink(SEM_NAME));
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);
}


TEST(sem_named, unlink_then_open_fails)
{
	sem = named_create(SEM_NAME, 1);

	TEST_ASSERT_EQUAL_INT(0, sem_close(sem));
	sem = NULL;

	TEST_ASSERT_EQUAL_INT(0, sem_unlink(SEM_NAME));

	errno = 0;
	TEST_ASSERT_EQUAL_PTR(SEM_FAILED, sem_open(SEM_NAME, 0));
	TEST_ASSERT_NOT_EQUAL_INT(0, errno);
}


TEST(sem_named, unlink_twice)
{
	sem = named_create(SEM_NAME, 1);

	TEST_ASSERT_EQUAL_INT(0, sem_close(sem));
	sem = NULL;

	TEST_ASSERT_EQUAL_INT(0, sem_unlink(SEM_NAME));

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, sem_unlink(SEM_NAME));
	TEST_ASSERT_EQUAL_INT(ENOENT, errno);
}


/* the name goes at once, the semaphore only when the last handle closes */
TEST(sem_named, unlink_while_open)
{
	int value = -1;

	sem = named_create(SEM_NAME, 1);

	TEST_ASSERT_EQUAL_INT(0, sem_unlink(SEM_NAME));

	/* the name is gone... */
	errno = 0;
	TEST_ASSERT_EQUAL_PTR(SEM_FAILED, sem_open(SEM_NAME, 0));
	TEST_ASSERT_NOT_EQUAL_INT(0, errno);

	/* ...but the open handle still works */
	TEST_ASSERT_EQUAL_INT(0, sem_getvalue(sem, &value));
	TEST_ASSERT_EQUAL_INT(1, value);
	TEST_ASSERT_EQUAL_INT(0, sem_wait(sem));
	TEST_ASSERT_EQUAL_INT(0, sem_post(sem));

	TEST_ASSERT_EQUAL_INT(0, sem_close(sem));
	sem = NULL;

	/* the name is now free for a fresh semaphore */
	sem = sem_open(SEM_NAME, O_CREAT | O_EXCL, 0666, 9u);
	TEST_ASSERT_NOT_EQUAL(SEM_FAILED, sem);
	TEST_ASSERT_EQUAL_INT(0, sem_getvalue(sem, &value));
	TEST_ASSERT_EQUAL_INT(9, value);
}


/* two openers - the semaphore survives until both close */
TEST(sem_named, unlink_while_two_open)
{
	int value = -1;

	sem = named_create(SEM_NAME, 1);

	sem2 = sem_open(SEM_NAME, 0);
	TEST_ASSERT_NOT_EQUAL(SEM_FAILED, sem2);

	TEST_ASSERT_EQUAL_INT(0, sem_unlink(SEM_NAME));

	TEST_ASSERT_EQUAL_INT(0, sem_close(sem));
	sem = NULL;

	/* one handle left: still fully usable */
	TEST_ASSERT_EQUAL_INT(0, sem_getvalue(sem2, &value));
	TEST_ASSERT_EQUAL_INT(1, value);
	TEST_ASSERT_EQUAL_INT(0, sem_wait(sem2));
	TEST_ASSERT_EQUAL_INT(0, sem_post(sem2));

	TEST_ASSERT_EQUAL_INT(0, sem_close(sem2));
	sem2 = NULL;
}


/* `rm` on the node must reclaim the object, not orphan it */
TEST(sem_named, unlink_node_directly)
{
#ifndef __phoenix__
	/* the backing /dev node is a phoenix implementation detail */
	TEST_IGNORE_MESSAGE("phoenix-specific");
#else
	char path[PATH_MAX];
	int value = -1;

	sem = named_create(SEM_NAME, 1);

	snprintf(path, sizeof(path), "/dev/posix/sem%s", SEM_NAME);
	TEST_ASSERT_EQUAL_INT_MESSAGE(0, unlink(path), "unlink() of the /dev node failed");

	/* the open handle keeps working, exactly as after sem_unlink() */
	TEST_ASSERT_EQUAL_INT(0, sem_getvalue(sem, &value));
	TEST_ASSERT_EQUAL_INT(1, value);

	TEST_ASSERT_EQUAL_INT(0, sem_close(sem));
	sem = NULL;

	/* and the name is reusable afterwards */
	sem = sem_open(SEM_NAME, O_CREAT | O_EXCL, 0666, 1u);
	TEST_ASSERT_NOT_EQUAL(SEM_FAILED, sem);
#endif
}


TEST(sem_named, post_wakes_waiter)
{
	pthread_t tid;

	waiterSem = named_create(SEM_NAME, 0);
	waiterRet[0] = -1;

	TEST_ASSERT_EQUAL_INT(0, pthread_create(&tid, NULL, named_waiter, &waiterRet[0]));

	usleep(100 * 1000);
	TEST_ASSERT_EQUAL_INT(0, sem_post(waiterSem));

	TEST_ASSERT_EQUAL_INT(0, pthread_join(tid, NULL));
	TEST_ASSERT_EQUAL_INT(0, waiterRet[0]);
}


/*
 * Sseveral waiters queued at once, then released one post at a time.
 * Exercises the ordered insert and the removal of interior queue entries.
 */
TEST(sem_named, multiple_waiters)
{
	pthread_t tid[WAITERS_CNT];
	int value = -1;
	int i;

	waiterSem = named_create(SEM_NAME, 0);

	for (i = 0; i < WAITERS_CNT; i++) {
		waiterRet[i] = -1;
		TEST_ASSERT_EQUAL_INT(0, pthread_create(&tid[i], NULL, named_waiter, &waiterRet[i]));
	}

	usleep(200 * 1000);

	for (i = 0; i < WAITERS_CNT; i++) {
		TEST_ASSERT_EQUAL_INT(0, sem_post(waiterSem));
	}

	for (i = 0; i < WAITERS_CNT; i++) {
		TEST_ASSERT_EQUAL_INT(0, pthread_join(tid[i], NULL));
		TEST_ASSERT_EQUAL_INT(0, waiterRet[i]);
	}

	/* every post was consumed by a waiter, none leaked into the value */
	TEST_ASSERT_EQUAL_INT(0, sem_getvalue(waiterSem, &value));
	TEST_ASSERT_EQUAL_INT(0, value);
}


/*
 * sysconf(_SC_SEM_NSEMS_MAX) advertises a limit, so creating past it must fail
 * with ENOSPC rather than consuming server memory without bound. Also covers the
 * accounting being given back: after unlinking, creating must work again.
 */
TEST(sem_named, nsems_max_enforced)
{
	char name[64];
	long limit = sysconf(_SC_SEM_NSEMS_MAX);
	int i, created = 0;
	sem_t *s;

	if (limit == -1) {
		TEST_IGNORE_MESSAGE("SEM_NSEMS_MAX has no fixed limit");
	}

	TEST_ASSERT_GREATER_THAN_INT(0, limit);

	if (limit > 1024) {
		TEST_IGNORE_MESSAGE("SEM_NSEMS_MAX too large to exhaust in a unit test");
	}

	for (i = 0; i < limit + NSEMS_SLACK; i++) {
		snprintf(name, sizeof(name), SEM_NAME_N, i);

		errno = 0;
		s = sem_open(name, O_CREAT | O_EXCL, 0666, 1u);
		if (s == SEM_FAILED) {
			break;
		}

		/*
		 * Keep the semaphore linked but hold no descriptor, so the test does not
		 * run into OPEN_MAX before it reaches SEM_NSEMS_MAX.
		 */
		TEST_ASSERT_EQUAL_INT(0, sem_close(s));
		created++;
		nsemsCreated = created;
	}

	if (created >= limit) {
		TEST_ASSERT_EQUAL_INT_MESSAGE(ENOSPC, errno, "creating past the limit must fail with ENOSPC");
		TEST_ASSERT_LESS_OR_EQUAL_INT(limit, created);
	}

	named_unlinkRange(created);
	nsemsCreated = 0;

	/* the slots were returned, so the name space is usable again */
	snprintf(name, sizeof(name), SEM_NAME_N, 0);
	s = sem_open(name, O_CREAT | O_EXCL, 0666, 1u);
	TEST_ASSERT_NOT_EQUAL_MESSAGE(SEM_FAILED, s, "slots were not released on unlink");
	TEST_ASSERT_EQUAL_INT(0, sem_close(s));
	TEST_ASSERT_EQUAL_INT(0, sem_unlink(name));
}


/*
 * Post racing against an expiring timeout: whoever wins, the client must be
 * answered exactly once and the semaphore must stay consistent.
 */
TEST(sem_named, post_vs_timeout)
{
	struct timespec ts;
	int value = -1;
	int i, ret, acquired = 0;

	sem = named_create(SEM_NAME, 0);

	for (i = 0; i < 20; i++) {
		sem_test_deadline(&ts, 20);

		TEST_ASSERT_EQUAL_INT(0, sem_post(sem));
		ret = sem_timedwait(sem, &ts);
		if (ret == 0) {
			acquired++;
		}
		else {
			TEST_ASSERT_EQUAL_INT(ETIMEDOUT, errno);
		}
	}

	/* every post either satisfied a wait or is still in the value */
	TEST_ASSERT_EQUAL_INT(0, sem_getvalue(sem, &value));
	TEST_ASSERT_EQUAL_INT(20, acquired + value);
}


TEST_GROUP_RUNNER(sem_named)
{
	RUN_TEST_CASE(sem_named, open_close);
	RUN_TEST_CASE(sem_named, open_null_name);
	RUN_TEST_CASE(sem_named, open_missing);
	RUN_TEST_CASE(sem_named, open_excl_twice);
	RUN_TEST_CASE(sem_named, open_existing_attaches);
	RUN_TEST_CASE(sem_named, open_no_oexcl_creates);
	RUN_TEST_CASE(sem_named, open_leading_slash_equivalent);
	RUN_TEST_CASE(sem_named, open_name_too_long);
	RUN_TEST_CASE(sem_named, open_embedded_slash);
	RUN_TEST_CASE(sem_named, open_empty_name);
	RUN_TEST_CASE(sem_named, getvalue_initial);
	RUN_TEST_CASE(sem_named, wait_post_getvalue);
	RUN_TEST_CASE(sem_named, trywait_success);
	RUN_TEST_CASE(sem_named, trywait_empty);
	RUN_TEST_CASE(sem_named, post_to_max);
	RUN_TEST_CASE(sem_named, post_overflow);
	RUN_TEST_CASE(sem_named, timedwait_success);
	RUN_TEST_CASE(sem_named, timedwait_timeout);
	RUN_TEST_CASE(sem_named, timedwait_timeout_then_reuse);
	RUN_TEST_CASE(sem_named, timedwait_expired_but_available);
	RUN_TEST_CASE(sem_named, timedwait_expired_and_empty);
	RUN_TEST_CASE(sem_named, timedwait_epoch_and_empty);
	RUN_TEST_CASE(sem_named, timedwait_invalid_nsec);
	RUN_TEST_CASE(sem_named, unlink_missing);
	RUN_TEST_CASE(sem_named, unlink_then_open_fails);
	RUN_TEST_CASE(sem_named, unlink_twice);
	RUN_TEST_CASE(sem_named, unlink_while_open);
	RUN_TEST_CASE(sem_named, unlink_while_two_open);
	RUN_TEST_CASE(sem_named, unlink_node_directly);
	RUN_TEST_CASE(sem_named, post_wakes_waiter);
	RUN_TEST_CASE(sem_named, multiple_waiters);
	RUN_TEST_CASE(sem_named, post_vs_timeout);
	RUN_TEST_CASE(sem_named, nsems_max_enforced);
}
