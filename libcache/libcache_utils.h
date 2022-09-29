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

#ifndef _LIBCACHE_UTILS_H_
#define _LIBCACHE_UTILS_H_


#include <cache.h>


#define LIBCACHE_SRC_MEM_SIZE 0x2800ULL /* Imitates the maximum capacity of cached source memory (in bytes) */
#define LIBCACHE_LINES_CNT    32        /* Number of cache lines */
#define LIBCACHE_LINE_SIZE    64        /* Size of a single cache line (in bytes) */

#define LOG2(x) ((uint8_t)(8 * sizeof(unsigned long) - __builtin_clzl((x)) - 1))


typedef struct {
	cachectx_t *cache;
	uint64_t addr;
	void *buffer;
	size_t count;
	int policy;
	ssize_t actualCount;
} test_write_args_t;


typedef struct {
	cachectx_t *cache;
	uint64_t addr;
	void *buffer;
	size_t count;
	ssize_t actualCount;
} test_read_args_t;


struct cache_devCtx_s {
	/* Empty device driver context */
};


int srcMem; /* File descriptor - simulates cached source memory */
uint8_t offBitsNum;
uint64_t offMask;
cache_ops_t ops;


/* Generates a file that simulates a cached source memory and fills it with random chars */
extern int test_genCharFile(void);


/* Generates a file that simulates a cached source memory and fills it with random ints */
extern int test_genIntFile(void);


extern ssize_t test_readCb(uint64_t offset, void *buffer, size_t count, cache_devCtx_t *ctx);


extern ssize_t test_writeCb(uint64_t offset, const void *buffer, size_t count, cache_devCtx_t *ctx);


extern ssize_t test_readCbErr(uint64_t offset, void *buffer, size_t count, cache_devCtx_t *ctx);


extern ssize_t test_writeCbErr(uint64_t offset, const void *buffer, size_t count, cache_devCtx_t *ctx);


extern void *test_cache_write(void *args);


extern void *test_cache_read(void *args);

#endif
