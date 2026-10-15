/*
 * Phoenix-RTOS
 *
 * Disturber
 *
 * Common definitions of disturber modules
 *
 * Copyright 2026 Phoenix Systems
 * Author: Jakub Klimek
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _DISTURBER_H_
#define _DISTURBER_H_


#define DISTURBER_THR_STACK    0x1000U
#define DISTURBER_THR_PRIORITY 0


typedef struct {
	const char *name;
	int nthreads;
	int firstThreadId;
	void (*init)(void);
	void *(*worker)(void *arg);
} disturber_t;


/* Sleep between worker iterations [ms] */
extern unsigned int disturber_sleepMs;


extern const disturber_t disturber_cpu;


extern const disturber_t disturber_mem;


extern const disturber_t disturber_fs;


#endif
