/*
 * Phoenix-RTOS
 *
 * kernel
 *
 * test/test_fpu
 *
 * Copyright 2023 Phoenix Systems
 * Author: Andrzej Stalke
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <stdio.h>
#include "sys/threads.h"

#define ITERATION_COUNT 5000

static char stacks[16][256];
double results[16][ITERATION_COUNT];

static double simple_sin(double x, size_t n)
{
	double result = 0.0;
	double helper = x;
	for (size_t i = 1; i <= n; ++i) {
		result += helper;
		helper *= -1;
		helper *= x * x;
		helper = helper / (2. * (double)i);
		helper = helper / (2. * (double)i + 1);
	}
	return result;
}

void thread_func(void *arg)
{
	double * volatile result = arg;
	for (size_t i = 0; i < ITERATION_COUNT; ++i) {
		result[i] = simple_sin(3.141592, i);
	}
	endthread();
}

int main(void)
{
	for (size_t i = 0; i < 16; ++i) {
		beginthread(thread_func, 5, stacks[i], 256, &results[i]);
	}
	for (size_t i = 0; i < 16; ++i) {
		threadJoin(-1, 0);
	}
	for (size_t i = 0; i < ITERATION_COUNT; ++i) {
		double val = results[0][i];
		for (size_t j  = 1; j < 16; ++j) {
			if (results[j][i] != val) {
				printf("Values differ: %f %f\n", results[j][i], val);
				return 1;
			}
		}
	}
	return 0;
}
