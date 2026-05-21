/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <syslog.h>
 * TESTED:
 *    - closelog()
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

#define CLOSELOG_TEST_IDENT "closelog-test"


TEST_GROUP(syslog_closelog);


TEST_SETUP(syslog_closelog)
{
	setlogmask(ALL_PRIORITIES_MASK);
}


TEST_TEAR_DOWN(syslog_closelog)
{
	setlogmask(ALL_PRIORITIES_MASK);
	closelog();
}


TEST(syslog_closelog, closelog_after_openlog)
{
	/* "closelog() shall close any open file descriptors allocated by
	 *  previous calls to openlog() or syslog()" */
	openlog(CLOSELOG_TEST_IDENT, LOG_NDELAY, LOG_USER);

	/* Should not crash */
	closelog();
}


TEST(syslog_closelog, closelog_without_prior_openlog)
{
	/* Calling closelog without prior openlog should be safe */
	closelog();
}


TEST(syslog_closelog, closelog_multiple_times)
{
	/* Calling closelog multiple times should not crash */
	openlog(CLOSELOG_TEST_IDENT, LOG_NDELAY, LOG_USER);

	closelog();
	closelog();
	closelog();
}


TEST(syslog_closelog, closelog_syslog_still_works_after)
{
	/* "It is not necessary to call openlog() prior to calling syslog()"
	 * so syslog must work even after closelog */
	openlog(CLOSELOG_TEST_IDENT, LOG_NDELAY, LOG_USER);
	syslog(LOG_INFO, "before closelog");

	closelog();

	/* syslog should implicitly reopen */
	syslog(LOG_INFO, "after closelog");
}


TEST(syslog_closelog, closelog_setlogmask_persists)
{
	int mask;
	const int customMask = LOG_MASK(LOG_ERR) | LOG_MASK(LOG_CRIT);

	openlog(CLOSELOG_TEST_IDENT, LOG_NDELAY, LOG_USER);
	setlogmask(customMask);

	closelog();

	/* Log mask should persist across closelog */
	mask = setlogmask(0);
	TEST_ASSERT_EQUAL_INT(customMask, mask);
}


TEST(syslog_closelog, closelog_after_syslog_without_openlog)
{
	/* "openlog() and syslog() functions may allocate a file descriptor"
	 * closelog should close fds allocated by syslog alone */
	syslog(LOG_INFO, "implicit open via syslog");

	closelog();
}


TEST_GROUP_RUNNER(syslog_closelog)
{
	RUN_TEST_CASE(syslog_closelog, closelog_after_openlog);
	RUN_TEST_CASE(syslog_closelog, closelog_without_prior_openlog);
	RUN_TEST_CASE(syslog_closelog, closelog_multiple_times);
	RUN_TEST_CASE(syslog_closelog, closelog_syslog_still_works_after);
	RUN_TEST_CASE(syslog_closelog, closelog_setlogmask_persists);
	RUN_TEST_CASE(syslog_closelog, closelog_after_syslog_without_openlog);
}
