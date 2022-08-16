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

#ifndef _THREAD_FUNCTIONS_H_
#define _THREAD_FUNCTIONS_H_

#define THREAD_NUM   15
#define CHECKS       100
#define ERRNO_CHECKS 36

typedef struct {
	int expected_tbss_value;
	int expected_tdata_value;
	int actual_tbss_value;
	int actual_tdata_value;
	int *tbss_value_addr;
	int *tdata_value_addr;
	int passed;
} tls_check_t;

typedef struct {
	int expected_tls_errno;
	int actual_tls_errno;
	int *errno_addr;
	int passed;
} tls_errno_check_t;


extern void *tls_assign_defaults(void *args);


extern void *tls_change_variables(void *args);


extern void *tls_check_errno(void *args);


#endif
