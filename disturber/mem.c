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
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "disturber.h"


#define ENOMEM_SLEEP_MS 10000U


static int max_alloc(void)
{
	int n = 1;
	int *array = malloc(sizeof(*array));

	if (array == NULL) {
		return 0;
	}

	while (n <= INT32_MAX / 2) {
		int *newArray = realloc(array, (size_t)(n * 2) * sizeof(*array));
		if (newArray == NULL) {
			fprintf(stderr, "Disturber MEM: ENOMEM reached for array of size %d\n", n * 2);
			break;
		}

		array = newArray;
		n *= 2;
	}

	/* Ensure whole memory is usable */
	for (int i = 0; i < n; i++) {
		array[i] = i;
	}
	usleep(ENOMEM_SLEEP_MS * 1000U);

	for (int i = 0; i < n; i++) {
		if (array[i] != i) {
			fprintf(stderr, "Disturber MEM: Memory corruption detected at index %d\n", i);
			break;
		}
	}
	free(array);
	return n;
}


static double matrix_multiply(int n)
{
	double *a, *b, *result;
	double checksum = 0;
	size_t count;

	if ((n <= 0) || ((size_t)n > SIZE_MAX / (size_t)n)) {
		return 0;
	}

	count = (size_t)n * (size_t)n;
	if (count > SIZE_MAX / sizeof(*a)) {
		return 0;
	}

	a = malloc(count * sizeof(*a));
	b = malloc(count * sizeof(*b));
	result = malloc(count * sizeof(*result));
	if ((a == NULL) || (b == NULL) || (result == NULL)) {
		free(a);
		free(b);
		free(result);
		fprintf(stderr, "Disturber MEM: Failed to allocate memory for matrix multiplication\n");
		return 0;
	}

	for (int row = 0; row < n; ++row) {
		for (int column = 0; column < n; ++column) {
			a[(size_t)row * n + column] = (double)((row + column) % 17);
			b[(size_t)row * n + column] = (double)((row * 3 + column) % 19);
		}
	}

	for (int row = 0; row < n; ++row) {
		for (int column = 0; column < n; ++column) {
			double value = 0;
			for (int index = 0; index < n; ++index) {
				value += a[(size_t)row * n + index] * b[(size_t)index * n + column];
			}
			result[(size_t)row * n + column] = value;
			checksum += value;
		}
	}

	free(a);
	free(b);
	free(result);
	return checksum;
}


static double eratosthenes_sieve(int n)
{
	unsigned char *isPrime;
	int count = 0;

	if (n < 2) {
		return 0;
	}

	isPrime = malloc((size_t)n + 1);
	if (isPrime == NULL) {
		fprintf(stderr, "Disturber MEM: Failed to allocate memory for sieve\n");
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

	free(isPrime);
	return (double)count;
}


static void *worker_thread(void *arg)
{
	int threadId = (int)(intptr_t)arg;
	while (1) {
		usleep(disturber_sleepMs * 1000U);
		matrix_multiply(100);
		eratosthenes_sieve(10000);
		max_alloc();

		printf("Disturber MEM: Thread %d full iteration completed\n", threadId);
	}

	return NULL;
}


const disturber_t disturber_mem = {
	.name = "mem",
	.nthreads = 1,
	.firstThreadId = 0,
	.init = NULL,
	.worker = worker_thread,
};
