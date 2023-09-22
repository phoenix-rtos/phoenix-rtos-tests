/*
 * Phoenix-RTOS
 *
 * libphoenix
 *
 * test/test_strftime.c
 *
 * Copyright 2020 Phoenix Systems
 * Author: Marcin Brzykcy
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include <stdio.h>
#include <stdlib.h>

static int test_verbosity;

static inline void save_env(void)
{
	char *c = getenv("VERBOSE_TEST");

	if (c != NULL && c[0] >= '0' && c[0] <= '9')
		test_verbosity = (c[0] - '0');
	else
		test_verbosity = 0;
}


static inline int verbose_test(void)
{
	return test_verbosity;
}

#endif
