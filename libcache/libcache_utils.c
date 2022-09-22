/*
 * Phoenix-RTOS
 *
 * phoenix-rtos-tests
 *
 * Libcache tests utilities
 *
 * Copyright 2022 Phoenix Systems
 * Author: Malgorzata Wrobel
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "libcache_utils.h"
#include "unity_fixture.h"

#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>


void test_genCharFile(void)
{
	FILE *file;
	int i;
	char num;

	file = fopen("/var/read.txt", "w");
	fseek(file, 0, SEEK_SET);

	for (i = 0; i < LIBCACHE_SRC_MEM_SIZE; ++i) {
		num = rand();
		fwrite_unlocked(&num, sizeof(char), 1, file);
	}

	fclose(file);
}

void test_genIntFile(void)
{
	FILE *file;
	int i;
	int num;

	file = fopen("/var/read.txt", "w");
	fseek(file, 0, SEEK_SET);

	for (i = 0; i < LIBCACHE_SRC_MEM_SIZE / 4; ++i) {
		num = rand();
		fwrite_unlocked(&num, sizeof(int), 1, file);
	}

	fclose(file);
}

ssize_t test_readCb(uint64_t offset, void *buffer, size_t count, cache_devCtx_t *ctx)
{
	ssize_t ret = -1;

	lseek(srcMem, offset, SEEK_SET);
	ret = read(srcMem, buffer, count);

	return ret;
}

ssize_t test_writeCb(uint64_t offset, const void *buffer, size_t count, cache_devCtx_t *ctx)
{
	ssize_t ret = -1;

	lseek(srcMem, offset, SEEK_SET);
	ret = write(srcMem, buffer, count);

	return ret;
}

ssize_t test_readCbErr(uint64_t offset, void *buffer, size_t count, cache_devCtx_t *ctx)
{
	return -EIO;
}

ssize_t test_writeCbErr(uint64_t offset, const void *buffer, size_t count, cache_devCtx_t *ctx)
{
	return -EIO;
}

void *test_cache_write(void *args)
{
	ssize_t *ret;
	test_write_args_t *arguments = args;

	ret = malloc(sizeof(ssize_t));
	*ret = cache_write(arguments->cache, arguments->addr, arguments->buffer, arguments->count, arguments->policy);

	return (void *)ret;
}

void *test_cache_read(void *args)
{
	ssize_t *ret;
	test_read_args_t *arguments = args;

	ret = malloc(sizeof(ssize_t));
	*ret = cache_read(arguments->cache, arguments->addr, arguments->buffer, arguments->count);

	return (void *)ret;
}
