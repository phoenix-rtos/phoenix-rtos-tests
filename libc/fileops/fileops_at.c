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

#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>

#include "fileops_at.h"
#include "unity_fixture.h"

#define TEST_WAIT_NS    10000000L
#define TEST_WAIT_TRIES 300


void test_atInit(test_atCtx_t *ctx)
{
	ctx->dirFd = -1;
	ctx->origCwd[0] = '\0';
}


int test_atOpenDir(test_atCtx_t *ctx, const char *dir)
{
	ctx->dirFd = open(dir, O_RDONLY);
	return ctx->dirFd;
}


int test_atChdir(test_atCtx_t *ctx, const char *dir)
{
	if (getcwd(ctx->origCwd, sizeof(ctx->origCwd)) == NULL) {
		ctx->origCwd[0] = '\0';
		return -1;
	}
	return chdir(dir);
}


int test_atRelease(test_atCtx_t *ctx)
{
	int ret = 0;

	if (ctx->dirFd >= 0) {
		close(ctx->dirFd);
		ctx->dirFd = -1;
	}
	if (ctx->origCwd[0] != '\0') {
		ret = chdir(ctx->origCwd);
		ctx->origCwd[0] = '\0';
	}
	return ret;
}


int test_atClosedFd(const char *dir)
{
	const int fd = open(dir, O_RDONLY);

	if (fd < 0) {
		return -1;
	}
	if (close(fd) != 0) {
		return -1;
	}
	return fd;
}


void test_waitNextSecond(time_t t)
{
	const struct timespec delay = { 0, TEST_WAIT_NS };
	int tries = 0;

	while ((time(NULL) <= t) && (tries < TEST_WAIT_TRIES)) {
		(void)nanosleep(&delay, NULL);
		tries++;
	}
	TEST_ASSERT_GREATER_THAN_INT64((int64_t)t, (int64_t)time(NULL));
}
