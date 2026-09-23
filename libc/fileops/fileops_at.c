/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 *
 * Shared helpers for *at() function tests (directory fd + cwd handling)
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

#include "fileops_at.h"


void test_atInit(test_atCtx_t *ctx)
{
	ctx->dirFd = -1;
	ctx->origCwd[0] = '\0';
}


int test_atOpenDir(test_atCtx_t *ctx, const char *dir)
{
	ctx->dirFd = open(dir, O_RDONLY | O_DIRECTORY);
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
	const int fd = open(dir, O_RDONLY | O_DIRECTORY);

	if (fd < 0) {
		return -1;
	}
	if (close(fd) != 0) {
		return -1;
	}
	return fd;
}
