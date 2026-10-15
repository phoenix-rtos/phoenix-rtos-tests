/*
 * Phoenix-RTOS
 *
 * Disturber MEM
 *
 * Memory allocation and access stressing module
 *
 * Copyright 2026 Phoenix Systems
 * Author: Jakub Klimek
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <stdio.h>
#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>

#include "disturber.h"


#define POOL_CHUNK_MAX PAGE_SIZE
#define POOL_RETRY_MS  1000U


typedef struct pool_chunk {
	struct pool_chunk *next; /* Immutable once the chunk is published */
	size_t size;
} pool_chunk_t;


/*
 * Grows only at the head, written only by the hog thread.
 * Chunks are never freed, so readers may traverse and hold them without locking.
 */
static _Atomic(pool_chunk_t *) poolChunks;
static size_t poolTotal;


/* Allocates memory in chunks of decreasing size down to 1 byte until ENOMEM, touching every page */
static size_t pool_fill(void)
{
	size_t pageSize = (size_t)PAGE_SIZE;
	size_t size = POOL_CHUNK_MAX;
	size_t gained = 0;
	pool_chunk_t *chunk;

	while (size > 0U) {
		chunk = malloc(size);
		if (chunk == NULL) {
			size /= 2U;
			continue;
		}

		/* Write every page to force its actual allocation */
		for (size_t off = 0; off < size; off += pageSize) {
			((volatile unsigned char *)chunk)[off] = (unsigned char)(off / pageSize);
		}
		gained += size;

		/* Too small to hold the chunk header - keep it allocated, but out of the pool */
		if (size < sizeof(*chunk)) {
			(void)sched_yield();
			continue;
		}

		chunk->size = size;
		chunk->next = atomic_load_explicit(&poolChunks, memory_order_relaxed);
		/* Publish fully initialized chunk */
		atomic_store_explicit(&poolChunks, chunk, memory_order_release);

		/* Let other threads compete for memory instead of grabbing it all at once */
		(void)sched_yield();
	}

	poolTotal += gained;
	return gained;
}


static void mem_hog(void)
{
	size_t gained;

	(void)pool_fill();
	printf("Disturber MEM: ENOMEM reached, holding %zu bytes\n", poolTotal);

	while (1) {
		usleep(POOL_RETRY_MS * 1000U);

		gained = pool_fill();
		if (gained != 0U) {
			printf("Disturber MEM: Allocated additional %zu bytes, holding %zu bytes\n", gained, poolTotal);
		}
	}
}


static pool_chunk_t *pool_next(pool_chunk_t *cursor)
{
	/* Chunks are never freed, so a pointer held across iterations stays valid */
	if ((cursor == NULL) || (cursor->next == NULL)) {
		/* Wrap around, picking up any chunks added since the last pass */
		return atomic_load_explicit(&poolChunks, memory_order_acquire);
	}

	return cursor->next;
}


/* Runs work on the payload of the next pool chunk */
static double pool_useNext(pool_chunk_t **cursor, double (*work)(void *buf, size_t size))
{
	pool_chunk_t *chunk = pool_next(*cursor);

	*cursor = chunk;
	if ((chunk == NULL) || (chunk->size <= sizeof(*chunk))) {
		return 0;
	}

	return work(chunk + 1, chunk->size - sizeof(*chunk));
}


/* Ensures whole chunk is usable */
static double pattern_check(void *buf, size_t size)
{
	uint32_t *words = buf;
	size_t count = size / sizeof(*words);
	uint32_t seed = (uint32_t)(uintptr_t)buf;

	for (size_t i = 0; i < count; i++) {
		words[i] = (uint32_t)i ^ seed;
	}

	for (size_t i = 0; i < count; i++) {
		if (words[i] != ((uint32_t)i ^ seed)) {
			fprintf(stderr, "Disturber MEM: Memory corruption detected at %p\n", (void *)&words[i]);
			return (double)i;
		}
	}

	return (double)count;
}


static uint32_t xorshift32(uint32_t *state)
{
	uint32_t x = *state;

	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	*state = x;
	return x;
}


/* Writes words at random offsets, then replays the sequence reading them back */
static double random_access(void *buf, size_t size)
{
	static uint32_t rngSeed = 0x9e3779b9U;
	uint32_t *words = buf;
	size_t count = size / sizeof(*words);
	uint32_t seed = xorshift32(&rngSeed);
	uint32_t state;
	size_t i;

	if (count == 0U) {
		return 0;
	}

	/* Value depends only on the offset, so offsets hit multiple times stay consistent */
	state = seed;
	for (size_t n = 0; n < count; n++) {
		i = xorshift32(&state) % count;
		words[i] = (uint32_t)i ^ seed;
	}

	state = seed;
	for (size_t n = 0; n < count; n++) {
		i = xorshift32(&state) % count;
		if (words[i] != ((uint32_t)i ^ seed)) {
			fprintf(stderr, "Disturber MEM: Memory corruption detected at %p\n", (void *)&words[i]);
			return (double)n;
		}
	}

	return (double)count;
}


static double matrix_multiply(void *buf, size_t size)
{
	double *a, *b, *result;
	double checksum = 0;
	size_t count = size / (3U * sizeof(*a));
	size_t n = 0;

	/* Largest n such that three n x n matrices fit in the buffer */
	while ((n + 1U) * (n + 1U) <= count) {
		n++;
	}
	if (n == 0U) {
		return 0;
	}

	count = n * n;
	a = buf;
	b = a + count;
	result = b + count;

	for (size_t row = 0; row < n; ++row) {
		for (size_t column = 0; column < n; ++column) {
			a[row * n + column] = (double)((row + column) % 17U);
			b[row * n + column] = (double)((row * 3U + column) % 19U);
		}
	}

	for (size_t row = 0; row < n; ++row) {
		for (size_t column = 0; column < n; ++column) {
			double value = 0;
			for (size_t index = 0; index < n; ++index) {
				value += a[row * n + index] * b[index * n + column];
			}
			result[row * n + column] = value;
			checksum += value;
		}
	}

	return checksum;
}


static double eratosthenes_sieve(void *buf, size_t size)
{
	unsigned char *isPrime = buf;
	int n = (int)(size - 1U);
	int count = 0;

	if (n < 2) {
		return 0;
	}

	for (int i = 0; i <= n; ++i) {
		isPrime[i] = 1;
	}
	isPrime[0] = 0;
	isPrime[1] = 0;

	for (int prime = 2; prime <= n / prime; ++prime) {
		if (isPrime[prime] != 0) {
			for (int multiple = prime * prime; multiple <= n; multiple += prime) {
				isPrime[multiple] = 0;
			}
		}
	}

	for (int i = 2; i <= n; ++i) {
		count += isPrime[i];
	}

	return (double)count;
}


static void *worker_thread(void *arg)
{
	int threadId = (int)(intptr_t)arg;
	pool_chunk_t *cursor = NULL;

	if (threadId == 1) {
		mem_hog();
	}

	while (1) {
		if (disturber_sleepMs > 0) {
			usleep(disturber_sleepMs * 1000U);
		}
		if (threadId == 2) {
			/* Each workload operates on the next chunk held by the hog thread */
			(void)pool_useNext(&cursor, matrix_multiply);
			(void)pool_useNext(&cursor, eratosthenes_sieve);
			(void)pool_useNext(&cursor, pattern_check);
			(void)pool_useNext(&cursor, random_access);
		}
	}

	return NULL;
}


const disturber_t disturber_mem = {
	.name = "mem",
	.nthreads = 2,
	.firstThreadId = 1,
	.init = NULL,
	.worker = worker_thread,
};
