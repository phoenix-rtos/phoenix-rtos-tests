/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <pthread.h>
 * TESTED:
 *    - pthread_attr_getschedparam()
 *    - pthread_attr_setschedparam()
 *    - pthread_getschedparam()
 *    - pthread_mutex_getprioceiling()
 *    - pthread_mutex_setprioceiling()
 *    - pthread_mutexattr_getprioceiling()
 *    - pthread_mutexattr_setprioceiling()
 *    - pthread_setschedparam()
 *    - pthread_setschedprio()
 *
 * Enforces the POSIX priority range contract on the pthread API: every value
 * inside [sched_get_priority_min, sched_get_priority_max] is accepted and
 * round-trips unchanged, everything outside is rejected with EINVAL, and a
 * greater value means a higher criticality.
 *
 * Copyright 2026 Phoenix Systems
 * Author: Adam Greloch
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <unistd.h>

#ifdef __phoenix__
#include <sys/features.h>
#include <sys/threads.h>
#endif

#include "unity_fixture.h"
#include "libc_features.h"

/*
 * POSIX defines mutex priority ceilings in terms of the SCHED_FIFO range.
 * POSIX-DEVIATION: the Phoenix kernel supports SCHED_RR only, so libphoenix
 * checks ceilings against the SCHED_RR range instead.
 */
#ifdef __phoenix__
#define CEILING_POLICY SCHED_RR
#else
#define CEILING_POLICY SCHED_FIFO
#endif


TEST_GROUP(pthread_prio_range);

static struct {
	int origPolicy;
	struct sched_param origParam;
	int minPrio;
	int maxPrio;
	int ceilMinPrio;
	int ceilMaxPrio;
} prio_common;


TEST_SETUP(pthread_prio_range)
{
	TEST_ASSERT_EQUAL_INT(0, pthread_getschedparam(pthread_self(), &prio_common.origPolicy, &prio_common.origParam));

	errno = 0;
	prio_common.minPrio = sched_get_priority_min(prio_common.origPolicy);
	TEST_ASSERT_GREATER_THAN_INT(-1, prio_common.minPrio);
	TEST_ASSERT_EQUAL_INT(0, errno);

	errno = 0;
	prio_common.maxPrio = sched_get_priority_max(prio_common.origPolicy);
	TEST_ASSERT_GREATER_THAN_INT(-1, prio_common.maxPrio);
	TEST_ASSERT_EQUAL_INT(0, errno);

	errno = 0;
	prio_common.ceilMinPrio = sched_get_priority_min(CEILING_POLICY);
	TEST_ASSERT_GREATER_THAN_INT(-1, prio_common.ceilMinPrio);
	TEST_ASSERT_EQUAL_INT(0, errno);

	errno = 0;
	prio_common.ceilMaxPrio = sched_get_priority_max(CEILING_POLICY);
	TEST_ASSERT_GREATER_THAN_INT(-1, prio_common.ceilMaxPrio);
	TEST_ASSERT_EQUAL_INT(0, errno);
}


TEST_TEAR_DOWN(pthread_prio_range)
{
	pthread_setschedparam(pthread_self(), prio_common.origPolicy, &prio_common.origParam);
	errno = 0;
}


static void prio_setSelf(int prio)
{
	int ret;

	ret = pthread_setschedprio(pthread_self(), prio);
	if (ret == EPERM) {
		TEST_IGNORE_MESSAGE("insufficient privileges to change the scheduling priority");
	}
	TEST_ASSERT_EQUAL_INT(0, ret);
}


static int prio_getSelf(void)
{
	struct sched_param param;
	int policy;

	param.sched_priority = INT_MIN;
	TEST_ASSERT_EQUAL_INT(0, pthread_getschedparam(pthread_self(), &policy, &param));

	return param.sched_priority;
}


/* pthread_setschedprio: both bounds are inclusive and round-trip unchanged */
TEST(pthread_prio_range, setschedprio_accepts_range_bounds)
{
#ifndef HAS_PTHREAD_SETSCHEDPRIO
	TEST_IGNORE_MESSAGE("pthread_setschedprio is not implemented");
#else
	prio_setSelf(prio_common.minPrio);
	TEST_ASSERT_EQUAL_INT(prio_common.minPrio, prio_getSelf());

	prio_setSelf(prio_common.maxPrio);
	TEST_ASSERT_EQUAL_INT(prio_common.maxPrio, prio_getSelf());
#endif
}


/* pthread_setschedprio: no value inside the range may be clamped or inverted */
TEST(pthread_prio_range, setschedprio_roundtrips_whole_range)
{
#ifndef HAS_PTHREAD_SETSCHEDPRIO
	TEST_IGNORE_MESSAGE("pthread_setschedprio is not implemented");
#else
	char msg[64];
	int prio;

	for (prio = prio_common.minPrio; prio <= prio_common.maxPrio; prio++) {
		prio_setSelf(prio);
		(void)snprintf(msg, sizeof(msg), "priority %d did not round-trip", prio);
		TEST_ASSERT_EQUAL_INT_MESSAGE(prio, prio_getSelf(), msg);
	}
#endif
}


/* pthread_setschedprio: EINVAL outside the range, with no change applied */
TEST(pthread_prio_range, setschedprio_rejects_out_of_range)
{
#ifndef HAS_PTHREAD_SETSCHEDPRIO
	TEST_IGNORE_MESSAGE("pthread_setschedprio is not implemented");
#else
	static const int offsets[] = { -1, 1 };
	char msg[64];
	int bad[4];
	int before, i;

	bad[0] = prio_common.minPrio + offsets[0];
	bad[1] = prio_common.maxPrio + offsets[1];
	bad[2] = INT_MIN;
	bad[3] = INT_MAX;

	prio_setSelf(prio_common.maxPrio);
	before = prio_getSelf();

	for (i = 0; i < (int)(sizeof(bad) / sizeof(bad[0])); i++) {
		(void)snprintf(msg, sizeof(msg), "priority %d was not rejected", bad[i]);
		TEST_ASSERT_EQUAL_INT_MESSAGE(EINVAL, pthread_setschedprio(pthread_self(), bad[i]), msg);
		TEST_ASSERT_EQUAL_INT_MESSAGE(before, prio_getSelf(), msg);
	}
#endif
}


/* pthread_setschedparam: EINVAL outside the range, with no change applied */
TEST(pthread_prio_range, setschedparam_rejects_out_of_range)
{
#ifndef HAS_PTHREAD_SETSCHEDPARAM
	TEST_IGNORE_MESSAGE("pthread_setschedparam is not implemented");
#else
	struct sched_param param;
	int before;

	before = prio_getSelf();

	param.sched_priority = prio_common.maxPrio + 1;
	TEST_ASSERT_EQUAL_INT(EINVAL, pthread_setschedparam(pthread_self(), prio_common.origPolicy, &param));
	TEST_ASSERT_EQUAL_INT(before, prio_getSelf());

	param.sched_priority = prio_common.minPrio - 1;
	TEST_ASSERT_EQUAL_INT(EINVAL, pthread_setschedparam(pthread_self(), prio_common.origPolicy, &param));
	TEST_ASSERT_EQUAL_INT(before, prio_getSelf());
#endif
}


/* pthread_attr_setschedparam: the attribute stores what the caller asked for */
TEST(pthread_prio_range, attr_schedparam_roundtrips_whole_range)
{
#ifndef HAS_PTHREAD_ATTR_SETSCHEDPARAM
	TEST_IGNORE_MESSAGE("pthread_attr_setschedparam is not implemented");
#else
	pthread_attr_t attr;
	struct sched_param param;
	char msg[64];
	int prio;

	TEST_ASSERT_EQUAL_INT(0, pthread_attr_init(&attr));

	for (prio = prio_common.minPrio; prio <= prio_common.maxPrio; prio++) {
		param.sched_priority = prio;
		(void)snprintf(msg, sizeof(msg), "priority %d did not round-trip", prio);
		TEST_ASSERT_EQUAL_INT_MESSAGE(0, pthread_attr_setschedparam(&attr, &param), msg);

		param.sched_priority = INT_MIN;
		TEST_ASSERT_EQUAL_INT_MESSAGE(0, pthread_attr_getschedparam(&attr, &param), msg);
		TEST_ASSERT_EQUAL_INT_MESSAGE(prio, param.sched_priority, msg);
	}

	TEST_ASSERT_EQUAL_INT(0, pthread_attr_destroy(&attr));
#endif
}


/* pthread_attr_setschedparam: EINVAL outside the range */
TEST(pthread_prio_range, attr_schedparam_rejects_out_of_range)
{
#ifndef HAS_PTHREAD_ATTR_SETSCHEDPARAM
	TEST_IGNORE_MESSAGE("pthread_attr_setschedparam is not implemented");
#else
	pthread_attr_t attr;
	struct sched_param param;

	TEST_ASSERT_EQUAL_INT(0, pthread_attr_init(&attr));

	param.sched_priority = prio_common.maxPrio + 1;
	TEST_ASSERT_EQUAL_INT(EINVAL, pthread_attr_setschedparam(&attr, &param));

	param.sched_priority = prio_common.minPrio - 1;
	TEST_ASSERT_EQUAL_INT(EINVAL, pthread_attr_setschedparam(&attr, &param));

	param.sched_priority = INT_MAX;
	TEST_ASSERT_EQUAL_INT(EINVAL, pthread_attr_setschedparam(&attr, &param));

	param.sched_priority = INT_MIN;
	TEST_ASSERT_EQUAL_INT(EINVAL, pthread_attr_setschedparam(&attr, &param));

	TEST_ASSERT_EQUAL_INT(0, pthread_attr_destroy(&attr));
#endif
}


/* A default-initialized attribute must not expose a priority outside the range */
TEST(pthread_prio_range, attr_default_schedparam_is_in_range)
{
#ifndef HAS_PTHREAD_ATTR_GETSCHEDPARAM
	TEST_IGNORE_MESSAGE("pthread_attr_getschedparam is not implemented");
#else
	pthread_attr_t attr;
	struct sched_param param;

	TEST_ASSERT_EQUAL_INT(0, pthread_attr_init(&attr));
	TEST_ASSERT_EQUAL_INT(0, pthread_attr_getschedparam(&attr, &param));

	TEST_ASSERT_GREATER_OR_EQUAL_INT(prio_common.minPrio, param.sched_priority);
	TEST_ASSERT_LESS_OR_EQUAL_INT(prio_common.maxPrio, param.sched_priority);

	TEST_ASSERT_EQUAL_INT(0, pthread_attr_destroy(&attr));
#endif
}


#ifdef HAS_PTHREAD_CREATE
static void *reportPrioThread(void *arg)
{
	struct sched_param param;
	int policy;

	param.sched_priority = INT_MIN;
	if (pthread_getschedparam(pthread_self(), &policy, &param) != 0) {
		param.sched_priority = INT_MIN;
	}
	*(int *)arg = param.sched_priority;

	return NULL;
}
#endif


/*
 * End-to-end check: the priority a thread is created with is the priority it
 * reports, in POSIX terms, from both inside and outside the thread.
 */
TEST(pthread_prio_range, created_thread_keeps_attr_priority)
{
#if !defined(HAS_PTHREAD_CREATE) || !defined(HAS_PTHREAD_ATTR_SETSCHEDPARAM)
	TEST_IGNORE_MESSAGE("pthread_create or pthread_attr_setschedparam is not implemented");
#else
	pthread_attr_t attr;
	pthread_t thr;
	struct sched_param param;
	int observed = INT_MIN;
	int policy, ret, target;

	/* Deliberately not the inherited one, so an unconverted value cannot pass by luck */
	target = (prio_common.maxPrio > prio_common.minPrio) ? prio_common.minPrio + 1 : prio_common.minPrio;

	TEST_ASSERT_EQUAL_INT(0, pthread_attr_init(&attr));
	TEST_ASSERT_EQUAL_INT(0, pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED));

	param.sched_priority = target;
	TEST_ASSERT_EQUAL_INT(0, pthread_attr_setschedparam(&attr, &param));

	ret = pthread_create(&thr, &attr, reportPrioThread, &observed);
	if (ret == EPERM) {
		(void)pthread_attr_destroy(&attr);
		TEST_IGNORE_MESSAGE("insufficient privileges to set the thread priority");
	}
	TEST_ASSERT_EQUAL_INT(0, ret);

	param.sched_priority = INT_MIN;
	TEST_ASSERT_EQUAL_INT(0, pthread_getschedparam(thr, &policy, &param));
	TEST_ASSERT_EQUAL_INT(target, param.sched_priority);

	TEST_ASSERT_EQUAL_INT(0, pthread_join(thr, NULL));
	TEST_ASSERT_EQUAL_INT(target, observed);

	TEST_ASSERT_EQUAL_INT(0, pthread_attr_destroy(&attr));
#endif
}


/* Priority ceilings live in the same range and must round-trip the same way */
TEST(pthread_prio_range, mutexattr_prioceiling_roundtrips_whole_range)
{
#ifndef HAS_PTHREAD_MUTEXATTR_SETPRIOCEILING
	TEST_IGNORE_MESSAGE("pthread_mutexattr_setprioceiling is not implemented");
#else
	pthread_mutexattr_t attr;
	char msg[64];
	int ceiling, prio;

	TEST_ASSERT_EQUAL_INT(0, pthread_mutexattr_init(&attr));

	for (prio = prio_common.ceilMinPrio; prio <= prio_common.ceilMaxPrio; prio++) {
		(void)snprintf(msg, sizeof(msg), "ceiling %d did not round-trip", prio);
		TEST_ASSERT_EQUAL_INT_MESSAGE(0, pthread_mutexattr_setprioceiling(&attr, prio), msg);

		ceiling = INT_MIN;
		TEST_ASSERT_EQUAL_INT_MESSAGE(0, pthread_mutexattr_getprioceiling(&attr, &ceiling), msg);
		TEST_ASSERT_EQUAL_INT_MESSAGE(prio, ceiling, msg);
	}

	TEST_ASSERT_EQUAL_INT(0, pthread_mutexattr_destroy(&attr));
#endif
}


TEST(pthread_prio_range, mutexattr_prioceiling_rejects_out_of_range)
{
#ifndef HAS_PTHREAD_MUTEXATTR_SETPRIOCEILING
	TEST_IGNORE_MESSAGE("pthread_mutexattr_setprioceiling is not implemented");
#else
	pthread_mutexattr_t attr;

	TEST_ASSERT_EQUAL_INT(0, pthread_mutexattr_init(&attr));

	TEST_ASSERT_EQUAL_INT(EINVAL, pthread_mutexattr_setprioceiling(&attr, prio_common.ceilMaxPrio + 1));
	TEST_ASSERT_EQUAL_INT(EINVAL, pthread_mutexattr_setprioceiling(&attr, prio_common.ceilMinPrio - 1));
	TEST_ASSERT_EQUAL_INT(EINVAL, pthread_mutexattr_setprioceiling(&attr, INT_MAX));
	TEST_ASSERT_EQUAL_INT(EINVAL, pthread_mutexattr_setprioceiling(&attr, INT_MIN));

	TEST_ASSERT_EQUAL_INT(0, pthread_mutexattr_destroy(&attr));
#endif
}


/* A default-initialized mutex attribute must not expose a ceiling outside the range */
TEST(pthread_prio_range, mutexattr_default_prioceiling_is_in_range)
{
#ifndef HAS_PTHREAD_MUTEXATTR_GETPRIOCEILING
	TEST_IGNORE_MESSAGE("pthread_mutexattr_getprioceiling is not implemented");
#else
	pthread_mutexattr_t attr;
	int ceiling = INT_MIN;

	TEST_ASSERT_EQUAL_INT(0, pthread_mutexattr_init(&attr));
	TEST_ASSERT_EQUAL_INT(0, pthread_mutexattr_getprioceiling(&attr, &ceiling));

	TEST_ASSERT_GREATER_OR_EQUAL_INT(prio_common.ceilMinPrio, ceiling);
	TEST_ASSERT_LESS_OR_EQUAL_INT(prio_common.ceilMaxPrio, ceiling);

	TEST_ASSERT_EQUAL_INT(0, pthread_mutexattr_destroy(&attr));
#endif
}


/*
 * The ceiling a mutex is created with must come back unchanged, and
 * pthread_mutex_setprioceiling() must report the previous one, not the new one.
 * The ceiling starts at the maximum because a thread may only lock a
 * PTHREAD_PRIO_PROTECT mutex whose ceiling is at least its own priority.
 */
TEST(pthread_prio_range, mutex_prioceiling_roundtrips)
{
#if !defined(HAS_PTHREAD_MUTEX_SETPRIOCEILING) || !defined(HAS_PTHREAD_MUTEX_GETPRIOCEILING)
	TEST_IGNORE_MESSAGE("pthread_mutex_setprioceiling or pthread_mutex_getprioceiling is not implemented");
#else
	pthread_mutexattr_t attr;
	pthread_mutex_t mutex;
	int ceiling, oldCeiling, ret;

	TEST_ASSERT_EQUAL_INT(0, pthread_mutexattr_init(&attr));
	ret = pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_PROTECT);
	if (ret == ENOTSUP) {
		(void)pthread_mutexattr_destroy(&attr);
		TEST_IGNORE_MESSAGE("PTHREAD_PRIO_PROTECT is not supported");
	}
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, pthread_mutexattr_setprioceiling(&attr, prio_common.ceilMaxPrio));
	TEST_ASSERT_EQUAL_INT(0, pthread_mutex_init(&mutex, &attr));

	ceiling = INT_MIN;
	TEST_ASSERT_EQUAL_INT(0, pthread_mutex_getprioceiling(&mutex, &ceiling));
	TEST_ASSERT_EQUAL_INT(prio_common.ceilMaxPrio, ceiling);

	/* Lowering the ceiling returns the previous one */
	oldCeiling = INT_MIN;
	TEST_ASSERT_EQUAL_INT(0, pthread_mutex_setprioceiling(&mutex, prio_common.ceilMinPrio, &oldCeiling));
	TEST_ASSERT_EQUAL_INT(prio_common.ceilMaxPrio, oldCeiling);

	ceiling = INT_MIN;
	TEST_ASSERT_EQUAL_INT(0, pthread_mutex_getprioceiling(&mutex, &ceiling));
	TEST_ASSERT_EQUAL_INT(prio_common.ceilMinPrio, ceiling);

	TEST_ASSERT_EQUAL_INT(0, pthread_mutex_destroy(&mutex));
	TEST_ASSERT_EQUAL_INT(0, pthread_mutexattr_destroy(&attr));
#endif
}


TEST(pthread_prio_range, mutex_prioceiling_rejects_out_of_range)
{
#ifndef HAS_PTHREAD_MUTEX_SETPRIOCEILING
	TEST_IGNORE_MESSAGE("pthread_mutex_setprioceiling is not implemented");
#else
	pthread_mutexattr_t attr;
	pthread_mutex_t mutex;
	int oldCeiling, ret;

	TEST_ASSERT_EQUAL_INT(0, pthread_mutexattr_init(&attr));
	ret = pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_PROTECT);
	if (ret == ENOTSUP) {
		(void)pthread_mutexattr_destroy(&attr);
		TEST_IGNORE_MESSAGE("PTHREAD_PRIO_PROTECT is not supported");
	}
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, pthread_mutexattr_setprioceiling(&attr, prio_common.ceilMaxPrio));
	TEST_ASSERT_EQUAL_INT(0, pthread_mutex_init(&mutex, &attr));

	TEST_ASSERT_EQUAL_INT(EINVAL, pthread_mutex_setprioceiling(&mutex, prio_common.ceilMaxPrio + 1, &oldCeiling));
	TEST_ASSERT_EQUAL_INT(EINVAL, pthread_mutex_setprioceiling(&mutex, prio_common.ceilMinPrio - 1, &oldCeiling));
	TEST_ASSERT_EQUAL_INT(EINVAL, pthread_mutex_setprioceiling(&mutex, INT_MAX, &oldCeiling));
	TEST_ASSERT_EQUAL_INT(EINVAL, pthread_mutex_setprioceiling(&mutex, INT_MIN, &oldCeiling));

	TEST_ASSERT_EQUAL_INT(0, pthread_mutex_destroy(&mutex));
	TEST_ASSERT_EQUAL_INT(0, pthread_mutexattr_destroy(&attr));
#endif
}


#ifdef __phoenix__
static volatile int lowPrioRan;


static void *lowPrioThread(void *arg)
{
	(void)arg;
	lowPrioRan = 1;

	return NULL;
}


/*
 * The ordering, observed through the scheduler rather than through a round-trip:
 * a thread created at a lower POSIX priority must not run while a higher-priority
 * thread is runnable, and must run once that thread blocks.
 *
 * Assumes a uniprocessor system - every Phoenix test target is currently
 * single-core, and sysconf() reports -1 ("unknown") there rather than a count.
 */
TEST(pthread_prio_range, lower_priority_thread_does_not_preempt)
{
#if !defined(HAS_PTHREAD_CREATE) || !defined(HAS_PTHREAD_SETSCHEDPRIO)
	TEST_IGNORE_MESSAGE("pthread_create or pthread_setschedprio is not implemented");
#else
	pthread_attr_t attr;
	pthread_t thr;
	struct sched_param param;
	volatile unsigned long i;

	if (sysconf(_SC_NPROCESSORS_ONLN) > 1) {
		TEST_IGNORE_MESSAGE("test assumes a uniprocessor system");
	}

	if (prio_common.maxPrio - prio_common.minPrio < 2) {
		TEST_IGNORE_MESSAGE("priority range too narrow to order two threads");
	}

	lowPrioRan = 0;

	TEST_ASSERT_EQUAL_INT(0, pthread_attr_init(&attr));
	TEST_ASSERT_EQUAL_INT(0, pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED));
	param.sched_priority = prio_common.minPrio + 1;
	TEST_ASSERT_EQUAL_INT(0, pthread_attr_setschedparam(&attr, &param));

	prio_setSelf(prio_common.maxPrio);
	TEST_ASSERT_EQUAL_INT(0, pthread_create(&thr, &attr, lowPrioThread, NULL));

	/* Busy-wait without yielding - the lower-priority thread must stay starved */
	for (i = 0; i < 1000000UL; i++) {
		/* spin */
	}
	TEST_ASSERT_EQUAL_INT(0, lowPrioRan);

	/* Blocking in join lets it through */
	TEST_ASSERT_EQUAL_INT(0, pthread_join(thr, NULL));
	TEST_ASSERT_EQUAL_INT(1, lowPrioRan);

	TEST_ASSERT_EQUAL_INT(0, pthread_attr_destroy(&attr));
#endif
}
#endif /* __phoenix__ */


TEST_GROUP_RUNNER(pthread_prio_range)
{
	RUN_TEST_CASE(pthread_prio_range, setschedprio_accepts_range_bounds);
	RUN_TEST_CASE(pthread_prio_range, setschedprio_roundtrips_whole_range);
	RUN_TEST_CASE(pthread_prio_range, setschedprio_rejects_out_of_range);
	RUN_TEST_CASE(pthread_prio_range, setschedparam_rejects_out_of_range);
	RUN_TEST_CASE(pthread_prio_range, attr_schedparam_roundtrips_whole_range);
	RUN_TEST_CASE(pthread_prio_range, attr_schedparam_rejects_out_of_range);
	RUN_TEST_CASE(pthread_prio_range, attr_default_schedparam_is_in_range);
	RUN_TEST_CASE(pthread_prio_range, created_thread_keeps_attr_priority);
	RUN_TEST_CASE(pthread_prio_range, mutexattr_prioceiling_roundtrips_whole_range);
	RUN_TEST_CASE(pthread_prio_range, mutexattr_prioceiling_rejects_out_of_range);
	RUN_TEST_CASE(pthread_prio_range, mutexattr_default_prioceiling_is_in_range);
	RUN_TEST_CASE(pthread_prio_range, mutex_prioceiling_roundtrips);
	RUN_TEST_CASE(pthread_prio_range, mutex_prioceiling_rejects_out_of_range);
#ifdef __phoenix__
	RUN_TEST_CASE(pthread_prio_range, lower_priority_thread_does_not_preempt);
#endif
}
