/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <syslog.h>
 * TESTED:
 *    - setlogmask()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Lukasz Kruszynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <syslog.h>

#include "unity_fixture.h"

#define ALL_PRIORITIES_MASK \
	(LOG_MASK(LOG_EMERG) | LOG_MASK(LOG_ALERT) | LOG_MASK(LOG_CRIT) | \
	LOG_MASK(LOG_ERR) | LOG_MASK(LOG_WARNING) | LOG_MASK(LOG_NOTICE) | \
	LOG_MASK(LOG_INFO) | LOG_MASK(LOG_DEBUG))


TEST_GROUP(syslog_setlogmask);


TEST_SETUP(syslog_setlogmask)
{
	/* Restore default mask (all priorities) before each test */
	setlogmask(ALL_PRIORITIES_MASK);
}


TEST_TEAR_DOWN(syslog_setlogmask)
{
	/* Restore default mask so subsequent test groups are unaffected */
	setlogmask(ALL_PRIORITIES_MASK);
}


TEST(syslog_setlogmask, setlogmask_default_allows_all)
{
	int mask;

	/* "The default log mask allows all priorities to be logged" */
	/* After restoring in setup, querying with 0 should show all bits */
	mask = setlogmask(0);
	TEST_ASSERT_EQUAL_INT(ALL_PRIORITIES_MASK, mask & ALL_PRIORITIES_MASK);
}


TEST(syslog_setlogmask, setlogmask_returns_previous_mask)
{
	int prev;
	int newMask;

	newMask = LOG_MASK(LOG_ERR) | LOG_MASK(LOG_CRIT);

	/* Set to known state */
	setlogmask(ALL_PRIORITIES_MASK);

	/* Set new mask, should return old */
	prev = setlogmask(newMask);
	TEST_ASSERT_EQUAL_INT(ALL_PRIORITIES_MASK, prev & ALL_PRIORITIES_MASK);
}


TEST(syslog_setlogmask, setlogmask_zero_does_not_modify)
{
	int mask1;
	int mask2;
	const int customMask = LOG_MASK(LOG_WARNING) | LOG_MASK(LOG_ERR);

	/* Set a custom mask */
	setlogmask(customMask);

	/* "If the maskpri argument is 0, the current log mask is not modified" */
	mask1 = setlogmask(0);
	TEST_ASSERT_EQUAL_INT(customMask, mask1);

	/* Call again — should still be unchanged */
	mask2 = setlogmask(0);
	TEST_ASSERT_EQUAL_INT(customMask, mask2);
}


TEST(syslog_setlogmask, setlogmask_set_single_priority)
{
	int prev;
	int current;
	const int singleMask = LOG_MASK(LOG_INFO);

	prev = setlogmask(singleMask);
	(void)prev;

	current = setlogmask(0);
	TEST_ASSERT_EQUAL_INT(singleMask, current);
}


TEST(syslog_setlogmask, setlogmask_set_multiple_priorities)
{
	int current;
	const int multiMask = LOG_MASK(LOG_ERR) | LOG_MASK(LOG_WARNING) | LOG_MASK(LOG_NOTICE);

	setlogmask(multiMask);

	current = setlogmask(0);
	TEST_ASSERT_EQUAL_INT(multiMask, current);
}


TEST(syslog_setlogmask, setlogmask_successive_calls_return_previous)
{
	int ret1;
	int ret2;
	const int mask1 = LOG_MASK(LOG_EMERG);
	const int mask2 = LOG_MASK(LOG_DEBUG);

	setlogmask(mask1);

	ret1 = setlogmask(mask2);
	TEST_ASSERT_EQUAL_INT(mask1, ret1);

	ret2 = setlogmask(mask1);
	TEST_ASSERT_EQUAL_INT(mask2, ret2);
}


TEST(syslog_setlogmask, setlogmask_log_upto_macro)
{
	int current;
	const int uptoWarning = LOG_MASK(LOG_EMERG) | LOG_MASK(LOG_ALERT) |
		LOG_MASK(LOG_CRIT) | LOG_MASK(LOG_ERR) | LOG_MASK(LOG_WARNING);

	/* LOG_UPTO should include all priorities from EMERG up to the given level */
	setlogmask(LOG_UPTO(LOG_WARNING));

	current = setlogmask(0);
	TEST_ASSERT_EQUAL_INT(uptoWarning, current);
}


TEST(syslog_setlogmask, setlogmask_log_mask_individual_bits)
{
	/* Verify LOG_MASK produces distinct single-bit values for each priority */
	TEST_ASSERT_NOT_EQUAL_INT(0, LOG_MASK(LOG_EMERG));
	TEST_ASSERT_NOT_EQUAL_INT(0, LOG_MASK(LOG_ALERT));
	TEST_ASSERT_NOT_EQUAL_INT(0, LOG_MASK(LOG_CRIT));
	TEST_ASSERT_NOT_EQUAL_INT(0, LOG_MASK(LOG_ERR));
	TEST_ASSERT_NOT_EQUAL_INT(0, LOG_MASK(LOG_WARNING));
	TEST_ASSERT_NOT_EQUAL_INT(0, LOG_MASK(LOG_NOTICE));
	TEST_ASSERT_NOT_EQUAL_INT(0, LOG_MASK(LOG_INFO));
	TEST_ASSERT_NOT_EQUAL_INT(0, LOG_MASK(LOG_DEBUG));

	/* Each should be a unique bit */
	TEST_ASSERT_NOT_EQUAL_INT(LOG_MASK(LOG_EMERG), LOG_MASK(LOG_ALERT));
	TEST_ASSERT_NOT_EQUAL_INT(LOG_MASK(LOG_ALERT), LOG_MASK(LOG_CRIT));
	TEST_ASSERT_NOT_EQUAL_INT(LOG_MASK(LOG_CRIT), LOG_MASK(LOG_ERR));
	TEST_ASSERT_NOT_EQUAL_INT(LOG_MASK(LOG_ERR), LOG_MASK(LOG_WARNING));
	TEST_ASSERT_NOT_EQUAL_INT(LOG_MASK(LOG_WARNING), LOG_MASK(LOG_NOTICE));
	TEST_ASSERT_NOT_EQUAL_INT(LOG_MASK(LOG_NOTICE), LOG_MASK(LOG_INFO));
	TEST_ASSERT_NOT_EQUAL_INT(LOG_MASK(LOG_INFO), LOG_MASK(LOG_DEBUG));
}


TEST(syslog_setlogmask, setlogmask_no_openlog_required)
{
	int mask;

	/* "A call to openlog() is not required prior to calling setlogmask()" */
	/* This test runs without any prior openlog(); should not crash */
	closelog();
	mask = setlogmask(LOG_MASK(LOG_ERR));
	(void)mask;

	/* Verify it took effect */
	mask = setlogmask(0);
	TEST_ASSERT_EQUAL_INT(LOG_MASK(LOG_ERR), mask);
}


TEST_GROUP_RUNNER(syslog_setlogmask)
{
	RUN_TEST_CASE(syslog_setlogmask, setlogmask_default_allows_all);
	RUN_TEST_CASE(syslog_setlogmask, setlogmask_returns_previous_mask);
	RUN_TEST_CASE(syslog_setlogmask, setlogmask_zero_does_not_modify);
	RUN_TEST_CASE(syslog_setlogmask, setlogmask_set_single_priority);
	RUN_TEST_CASE(syslog_setlogmask, setlogmask_set_multiple_priorities);
	RUN_TEST_CASE(syslog_setlogmask, setlogmask_successive_calls_return_previous);
	RUN_TEST_CASE(syslog_setlogmask, setlogmask_log_upto_macro);
	RUN_TEST_CASE(syslog_setlogmask, setlogmask_log_mask_individual_bits);
	RUN_TEST_CASE(syslog_setlogmask, setlogmask_no_openlog_required);
}
