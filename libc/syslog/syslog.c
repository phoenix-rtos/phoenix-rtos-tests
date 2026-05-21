/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <syslog.h>
 * TESTED:
 *    - syslog()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Lukasz Kruszynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <syslog.h>
#include <errno.h>
#include <string.h>

#include "unity_fixture.h"

#define ALL_PRIORITIES_MASK \
	(LOG_MASK(LOG_EMERG) | LOG_MASK(LOG_ALERT) | LOG_MASK(LOG_CRIT) | \
	LOG_MASK(LOG_ERR) | LOG_MASK(LOG_WARNING) | LOG_MASK(LOG_NOTICE) | \
	LOG_MASK(LOG_INFO) | LOG_MASK(LOG_DEBUG))

#define SYSLOG_TEST_IDENT "test-syslog"


TEST_GROUP(syslog_syslog);


TEST_SETUP(syslog_syslog)
{
	closelog();
	setlogmask(ALL_PRIORITIES_MASK);
	openlog(SYSLOG_TEST_IDENT, LOG_NDELAY, LOG_USER);
}


TEST_TEAR_DOWN(syslog_syslog)
{
	setlogmask(ALL_PRIORITIES_MASK);
	closelog();
}


TEST(syslog_syslog, syslog_does_not_crash_all_severities)
{
	/* syslog() returns void — verify it doesn't crash for each severity */
	syslog(LOG_EMERG, "test emerg message %d", 1);
	syslog(LOG_ALERT, "test alert message %d", 2);
	syslog(LOG_CRIT, "test crit message %d", 3);
	syslog(LOG_ERR, "test err message %d", 4);
	syslog(LOG_WARNING, "test warning message %d", 5);
	syslog(LOG_NOTICE, "test notice message %d", 6);
	syslog(LOG_INFO, "test info message %d", 7);
	syslog(LOG_DEBUG, "test debug message %d", 8);

	/* If we reach here, no crash occurred */
}


TEST(syslog_syslog, syslog_with_facility_or)
{
	/* "Values of the priority argument are formed by OR'ing together
	 *  a severity-level value and an optional facility value" */
	syslog(LOG_LOCAL0 | LOG_ERR, "local0 err %s", "test");
	syslog(LOG_LOCAL7 | LOG_DEBUG, "local7 debug %s", "test");
	syslog(LOG_USER | LOG_WARNING, "user warning %s", "test");
}


TEST(syslog_syslog, syslog_percent_m_conversion)
{
	/* "%m shall convert no arguments, shall cause the output of the error
	 * message string associated with the value of errno on entry to syslog()" */
	errno = ENOENT;
	syslog(LOG_ERR, "file error: %m");

	errno = EINVAL;
	syslog(LOG_ERR, "invalid: %m with extra %s", "text");

	/* No crash, %m consumed no varargs */
}


TEST(syslog_syslog, syslog_printf_format_specifiers)
{
	/* "The message body is generated from the message and following arguments
	 *  in the same manner as if these were arguments to printf()" */
	syslog(LOG_INFO, "int=%d str=%s hex=%x", 42, "hello", 0xff);
	syslog(LOG_INFO, "long=%ld unsigned=%u", 123456L, 99U);
	syslog(LOG_INFO, "char=%c float=%.2f", 'A', 3.14);
}


TEST(syslog_syslog, syslog_masked_priority_not_logged)
{
	/* "Calls by the current process to syslog() with a priority not set
	 *  in maskpri shall be rejected" — verify no crash when masked */
	setlogmask(LOG_MASK(LOG_ERR));

	/* These should be silently rejected */
	syslog(LOG_DEBUG, "should be rejected");
	syslog(LOG_INFO, "should be rejected");
	syslog(LOG_WARNING, "should be rejected");

	/* Only ERR should pass through */
	syslog(LOG_ERR, "should pass");
}


TEST(syslog_syslog, syslog_without_prior_openlog)
{
	/* "It is not necessary to call openlog() prior to calling syslog()" */
	closelog();

	syslog(LOG_INFO, "message without openlog %d", 123);
}


TEST(syslog_syslog, syslog_minimal_message)
{
	/* Edge case: minimal single-character message */
	syslog(LOG_INFO, ".");
}


TEST(syslog_syslog, syslog_long_message)
{
	static char longMsg[1024];

	memset(longMsg, 'A', sizeof(longMsg) - 1);
	longMsg[sizeof(longMsg) - 1] = '\0';

	syslog(LOG_INFO, "%s", longMsg);
}


TEST(syslog_syslog, syslog_default_facility_is_log_user)
{
	/* "The initial default facility is LOG_USER" — when no facility is
	 * specified in priority, LOG_USER is used. No crash expected. */
	closelog();

	/* Priority without facility OR'd in */
	syslog(LOG_ERR, "default facility test");
}


TEST_GROUP_RUNNER(syslog_syslog)
{
	RUN_TEST_CASE(syslog_syslog, syslog_does_not_crash_all_severities);
	RUN_TEST_CASE(syslog_syslog, syslog_with_facility_or);
	RUN_TEST_CASE(syslog_syslog, syslog_percent_m_conversion);
	RUN_TEST_CASE(syslog_syslog, syslog_printf_format_specifiers);
	RUN_TEST_CASE(syslog_syslog, syslog_masked_priority_not_logged);
	RUN_TEST_CASE(syslog_syslog, syslog_without_prior_openlog);
	RUN_TEST_CASE(syslog_syslog, syslog_minimal_message);
	RUN_TEST_CASE(syslog_syslog, syslog_long_message);
	RUN_TEST_CASE(syslog_syslog, syslog_default_facility_is_log_user);
}
