/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 *
 * Shared helpers for fileops tests (directory fd + cwd handling, timestamp waits)
 *
 * Copyright 2026 Phoenix Systems
 * Author: Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _TEST_FILEOPS_AT_H
#define _TEST_FILEOPS_AT_H

#include <limits.h>
#include <time.h>


typedef struct {
	int dirFd;
	char origCwd[PATH_MAX];
} test_atCtx_t;


/* Resets ctx to the "nothing to release" state; call first in TEST_SETUP. */
void test_atInit(test_atCtx_t *ctx);


/* Opens dir as ctx->dirFd (O_RDONLY); returns the fd or -1. */
int test_atOpenDir(test_atCtx_t *ctx, const char *dir);


/* Saves the current working directory in ctx and changes it to dir; returns 0 or -1. */
int test_atChdir(test_atCtx_t *ctx, const char *dir);


/* Closes ctx->dirFd and restores the saved cwd; returns chdir() result. Safe to call from TEST_TEAR_DOWN. */
int test_atRelease(test_atCtx_t *ctx);


/* Returns a descriptor number that was valid for dir and has been closed, or -1 on error. */
int test_atClosedFd(const char *dir);


/* Waits (with short sleeps) until the wall clock passes second t; asserts it did. */
void test_waitNextSecond(time_t t);


#endif /* _TEST_FILEOPS_AT_H */
