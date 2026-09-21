/*
 * Phoenix-RTOS
 *
 * libc-tests
 *
 * Testing POSIX signals.
 *
 * Copyright 2026 Phoenix Systems
 * Author: Jakub Klimek
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include <signal.h>
#include <errno.h>

#include <unity_fixture.h>

#ifdef __phoenix__
#include "sys/threads.h"
#endif


TEST_GROUP(misc);


TEST_SETUP(misc)
{
}


TEST_TEAR_DOWN(misc)
{
}


TEST(misc, kill_init_thread)
{
#ifndef __phoenix__
	TEST_IGNORE_MESSAGE("phoenix-specific");
#else
	for (int sig = 0; sig < NSIG; ++sig) {
		TEST_ASSERT_EQUAL_INT(-1, kill(1, sig));
		TEST_ASSERT_NOT_EQUAL_INT(0, errno);
	}
	TEST_ASSERT_NOT_EQUAL_INT(0, sys_tkill(1, 0, SIGCANCEL));
#endif
}


TEST_GROUP_RUNNER(misc)
{
	RUN_TEST_CASE(misc, kill_init_thread);
}
