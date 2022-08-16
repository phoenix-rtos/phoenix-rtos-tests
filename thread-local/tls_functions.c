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
#include <errno.h>
#include <unistd.h>
#include "tls_functions.h"

__thread int tbss;
__thread int tdata = 3;

void *tls_assign_defaults(void *args)
{
	tls_check_t *res = args;
	res->actual_tbss_value = tbss;
	res->tbss_value_addr = &tbss;
	usleep(100);
	res->actual_tdata_value = tdata;
	res->tdata_value_addr = &tdata;
	return NULL;
}


void *tls_change_variables(void *args)
{
	tls_check_t *result = args;
	int passed = 0;
	for (int i = 0; i < CHECKS; i++) {
		result->expected_tbss_value += i;
		result->expected_tdata_value += i;
		tbss = result->expected_tbss_value;
		tdata = result->expected_tdata_value;
		result->actual_tbss_value = tbss;
		result->actual_tdata_value = tdata;
		usleep(100);
		if (result->actual_tbss_value == result->expected_tbss_value && result->actual_tdata_value == result->expected_tdata_value) {
			passed++;
		}
	}
	result->tdata_value_addr = &tdata;
	result->tbss_value_addr = &tbss;
	result->passed = passed;

	return NULL;
}


void *tls_check_errno(void *args)
{
	tls_errno_check_t *result = args;
	int passed = 0;
	for (int i = 0; i < ERRNO_CHECKS; i++) {
		SET_ERRNO(result->expected_tls_errno + i);
		usleep(100);
		result->actual_tls_errno = -errno;
		if (result->actual_tls_errno == result->expected_tls_errno + i) {
			passed++;
		}
	}
	result->errno_addr = &errno;
	result->passed = passed;
	return NULL;
}
