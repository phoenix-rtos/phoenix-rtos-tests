/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <sched.h>
 * TESTED:
 *    - sched_get_priority_max()
 *    - sched_get_priority_min()
 *    - sched_getparam()
 *    - sched_setparam()
 *    - sched_setscheduler()
 *
 * Enforces the POSIX priority range contract:
 *  - the range is non-empty, non-negative and holds at least 32 levels,
 *  - every value inside it is accepted and round-trips unchanged,
 *  - every value outside it is rejected with EINVAL and changes nothing,
 *  - a greater value means a higher criticality.
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
#include <sched.h>
#include <stdio.h>
#include <unistd.h>

#ifdef __phoenix__
#include <sys/features.h>
#include <sys/threads.h>
#endif

#include "unity_fixture.h"
#include "libc_features.h"

/* POSIX: SCHED_FIFO and SCHED_RR shall provide at least 32 distinct priority levels */
#define POSIX_PRIO_LEVELS_MIN 32


#if defined(HAS_SCHED_GETPARAM) && defined(HAS_SCHED_SETPARAM)

/* Tests: sched_get_priority_max, sched_get_priority_min, sched_getparam, sched_setparam */
TEST_GROUP(sched_prio_range);

static struct {
	int origPolicy;
	struct sched_param origParam;
	int minPrio;
	int maxPrio;
} prio_common;


TEST_SETUP(sched_prio_range)
{
	prio_common.origPolicy = sched_getscheduler(0);
	TEST_ASSERT_GREATER_THAN_INT(-1, prio_common.origPolicy);
	TEST_ASSERT_EQUAL_INT(0, sched_getparam(0, &prio_common.origParam));

	/*
	 * The bounds are taken from the policy the test actually runs under, so that
	 * the same assertions hold on hosts where SCHED_RR needs privileges.
	 */
	errno = 0;
	prio_common.minPrio = sched_get_priority_min(prio_common.origPolicy);
	TEST_ASSERT_GREATER_THAN_INT(-1, prio_common.minPrio);
	TEST_ASSERT_EQUAL_INT(0, errno);

	errno = 0;
	prio_common.maxPrio = sched_get_priority_max(prio_common.origPolicy);
	TEST_ASSERT_GREATER_THAN_INT(-1, prio_common.maxPrio);
	TEST_ASSERT_EQUAL_INT(0, errno);
}


TEST_TEAR_DOWN(sched_prio_range)
{
	sched_setparam(0, &prio_common.origParam);
	errno = 0;
}


static void prio_set(int prio)
{
	struct sched_param param;
	int ret;

	param.sched_priority = prio;
	errno = 0;
	ret = sched_setparam(0, &param);
	if ((ret == -1) && (errno == EPERM)) {
		TEST_IGNORE_MESSAGE("insufficient privileges to change the scheduling priority");
	}
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, errno);
}


static int prio_get(void)
{
	struct sched_param param;

	param.sched_priority = INT_MIN;
	errno = 0;
	TEST_ASSERT_EQUAL_INT(0, sched_getparam(0, &param));
	TEST_ASSERT_EQUAL_INT(0, errno);

	return param.sched_priority;
}


static void prio_setRejected(int prio)
{
	struct sched_param param;
	char msg[64];
	int before, ret;

	before = prio_get();

	param.sched_priority = prio;
	errno = 0;
	ret = sched_setparam(0, &param);

	(void)snprintf(msg, sizeof(msg), "priority %d was not rejected", prio);
	TEST_ASSERT_EQUAL_INT_MESSAGE(-1, ret, msg);
	TEST_ASSERT_EQUAL_INT_MESSAGE(EINVAL, errno, msg);

	/* A rejected request must not have been applied partially */
	TEST_ASSERT_EQUAL_INT_MESSAGE(before, prio_get(), msg);
}


/* The SCHED_RR range must be non-negative - POSIX allows negatives, ports do not expect them */
TEST(sched_prio_range, rr_range_is_non_negative)
{
	errno = 0;
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, sched_get_priority_min(SCHED_RR));
	TEST_ASSERT_EQUAL_INT(0, errno);

	errno = 0;
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, sched_get_priority_max(SCHED_RR));
	TEST_ASSERT_EQUAL_INT(0, errno);
}


/* POSIX mandates at least 32 distinct SCHED_RR priority levels */
TEST(sched_prio_range, rr_range_has_at_least_32_levels)
{
	int minPrio, maxPrio;

	minPrio = sched_get_priority_min(SCHED_RR);
	maxPrio = sched_get_priority_max(SCHED_RR);
	TEST_ASSERT_GREATER_THAN_INT(minPrio, maxPrio);
	TEST_ASSERT_GREATER_OR_EQUAL_INT(POSIX_PRIO_LEVELS_MIN, maxPrio - minPrio + 1);
}


/* Repeated queries must be stable and must not clobber errno */
TEST(sched_prio_range, range_queries_are_stable)
{
	int i;

	for (i = 0; i < 3; i++) {
		errno = 0;
		TEST_ASSERT_EQUAL_INT(prio_common.minPrio, sched_get_priority_min(prio_common.origPolicy));
		TEST_ASSERT_EQUAL_INT(0, errno);

		errno = 0;
		TEST_ASSERT_EQUAL_INT(prio_common.maxPrio, sched_get_priority_max(prio_common.origPolicy));
		TEST_ASSERT_EQUAL_INT(0, errno);
	}
}


/* Both bounds are inclusive and must round-trip unchanged */
TEST(sched_prio_range, setparam_accepts_range_bounds)
{
	prio_set(prio_common.minPrio);
	TEST_ASSERT_EQUAL_INT(prio_common.minPrio, prio_get());

	prio_set(prio_common.maxPrio);
	TEST_ASSERT_EQUAL_INT(prio_common.maxPrio, prio_get());
}


/* No value inside the range may be clamped, inverted or rejected */
TEST(sched_prio_range, setparam_roundtrips_whole_range)
{
	char msg[64];
	int prio;

	for (prio = prio_common.minPrio; prio <= prio_common.maxPrio; prio++) {
		prio_set(prio);
		(void)snprintf(msg, sizeof(msg), "priority %d did not round-trip", prio);
		TEST_ASSERT_EQUAL_INT_MESSAGE(prio, prio_get(), msg);
	}
}


TEST(sched_prio_range, setparam_rejects_below_min)
{
	prio_setRejected(prio_common.minPrio - 1);
	prio_setRejected(-1);
	prio_setRejected(INT_MIN);
}


TEST(sched_prio_range, setparam_rejects_above_max)
{
	prio_setRejected(prio_common.maxPrio + 1);
	prio_setRejected(INT_MAX);
}


/* sched_setscheduler() must enforce the very same bounds as sched_setparam() */
TEST(sched_prio_range, setscheduler_enforces_same_bounds)
{
#ifndef HAS_SCHED_SETSCHEDULER
	TEST_IGNORE_MESSAGE("sched_setscheduler is not implemented");
#else
	struct sched_param param;
	int ret;

	param.sched_priority = prio_common.maxPrio;
	errno = 0;
	ret = sched_setscheduler(0, prio_common.origPolicy, &param);
	if ((ret == -1) && (errno == EPERM)) {
		TEST_IGNORE_MESSAGE("insufficient privileges to change the scheduling policy");
	}
	TEST_ASSERT_GREATER_THAN_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(prio_common.maxPrio, prio_get());

	param.sched_priority = prio_common.maxPrio + 1;
	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, sched_setscheduler(0, prio_common.origPolicy, &param));
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);

	param.sched_priority = prio_common.minPrio - 1;
	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, sched_setscheduler(0, prio_common.origPolicy, &param));
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);

	/* Neither rejection may have changed the priority */
	TEST_ASSERT_EQUAL_INT(prio_common.maxPrio, prio_get());
#endif
}


#ifdef __phoenix__
/*
 * Phoenix native priorities are inverted - the lower the value, the more critical
 * the thread. The POSIX range must therefore map onto the native one in reverse.
 * getPriority() is the native observable, unaffected by the POSIX translation, so
 * these two tests pin the direction of the mapping instead of just its symmetry.
 */
TEST(sched_prio_range, posix_max_maps_to_most_critical_native)
{
#ifndef __PHOENIX_SCHED_PRIO_POSIX
	TEST_IGNORE_MESSAGE("libphoenix does not define __PHOENIX_SCHED_PRIO_POSIX");
#else
	int nativeAtMin, nativeAtMax;

	prio_set(prio_common.minPrio);
	nativeAtMin = getPriority();

	prio_set(prio_common.maxPrio);
	nativeAtMax = getPriority();

	TEST_ASSERT_LESS_THAN_INT(nativeAtMin, nativeAtMax);
#endif
}


/* Every step up the POSIX range must be exactly one step up in criticality */
TEST(sched_prio_range, posix_range_is_strictly_monotonic_in_native)
{
#ifndef __PHOENIX_SCHED_PRIO_POSIX
	TEST_IGNORE_MESSAGE("libphoenix does not define __PHOENIX_SCHED_PRIO_POSIX");
#else
	char msg[80];
	int prio, native, prev;

	prev = INT_MAX;

	for (prio = prio_common.minPrio; prio <= prio_common.maxPrio; prio++) {
		prio_set(prio);
		native = getPriority();

		(void)snprintf(msg, sizeof(msg), "POSIX priority %d maps to native %d, previous was %d", prio, native, prev);
		TEST_ASSERT_LESS_THAN_INT_MESSAGE(prev, native, msg);
		prev = native;
	}
#endif
}
#endif /* __phoenix__ */


TEST_GROUP_RUNNER(sched_prio_range)
{
	RUN_TEST_CASE(sched_prio_range, rr_range_is_non_negative);
	RUN_TEST_CASE(sched_prio_range, rr_range_has_at_least_32_levels);
	RUN_TEST_CASE(sched_prio_range, range_queries_are_stable);
	RUN_TEST_CASE(sched_prio_range, setparam_accepts_range_bounds);
	RUN_TEST_CASE(sched_prio_range, setparam_roundtrips_whole_range);
	RUN_TEST_CASE(sched_prio_range, setparam_rejects_below_min);
	RUN_TEST_CASE(sched_prio_range, setparam_rejects_above_max);
	RUN_TEST_CASE(sched_prio_range, setscheduler_enforces_same_bounds);
#ifdef __phoenix__
	RUN_TEST_CASE(sched_prio_range, posix_max_maps_to_most_critical_native);
	RUN_TEST_CASE(sched_prio_range, posix_range_is_strictly_monotonic_in_native);
#endif
}

#else
TEST_GROUP_UNIMPLEMENTED(sched_prio_range, "sched_getparam and sched_setparam not implemented")
#endif
