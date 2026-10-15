/*
 * Phoenix-RTOS
 *
 * Disturber
 *
 * Runs selected CPU/memory/filesystem disturbers in a single process
 *
 * Copyright 2026 Phoenix Systems
 * Author: Jakub Klimek
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include "disturber.h"

#define MAX_THREADS 16


static const disturber_t *const disturbers[] = { &disturber_cpu, &disturber_mem, &disturber_fs };

#define NDISTURBERS (sizeof(disturbers) / sizeof(disturbers[0]))

#define DEFAULT_SLEEP_MS 0U


unsigned int disturber_sleepMs = DEFAULT_SLEEP_MS;


static void usage(const char *progname)
{
	size_t i;

	fprintf(stderr, "Usage: %s [-s sleep_ms] [disturber...]\n", progname);
	fprintf(stderr, "  -s sleep_ms  sleep between worker iterations in ms (default %u)\n", DEFAULT_SLEEP_MS);
	fprintf(stderr, "Runs all disturbers if none is given. Available:");
	for (i = 0; i < NDISTURBERS; ++i) {
		fprintf(stderr, " %s", disturbers[i]->name);
	}
	fprintf(stderr, "\n");
}


int main(int argc, char **argv)
{
	pthread_t threads[MAX_THREADS];
	int enabled[NDISTURBERS] = { 0 };
	pthread_attr_t attr;
	int nthreads = 0;
	size_t i;
	int j, opt;
	char *end;
	unsigned long val;

	setPriority(1);

	while ((opt = getopt(argc, argv, "s:h")) != -1) {
		switch (opt) {
			case 's':
				val = strtoul(optarg, &end, 10);
				if ((*optarg == '\0') || (*end != '\0') || (val > UINT32_MAX / 1000U)) {
					fprintf(stderr, "%s: Invalid sleep value '%s'\n", argv[0], optarg);
					return 1;
				}
				disturber_sleepMs = (unsigned int)val;
				break;
			case 'h':
			default:
				usage(argv[0]);
				return (opt == 'h') ? 0 : 1;
		}
	}

	if (optind >= argc) {
		for (i = 0; i < NDISTURBERS; ++i) {
			enabled[i] = 1;
		}
	}

	for (j = optind; j < argc; ++j) {
		for (i = 0; i < NDISTURBERS; ++i) {
			if (strcmp(argv[j], disturbers[i]->name) == 0) {
				enabled[i] = 1;
				break;
			}
		}
		if (i == NDISTURBERS) {
			usage(argv[0]);
			return 1;
		}
	}

	if (pthread_attr_init(&attr) != 0) {
		fprintf(stderr, "%s: Failed to init thread attributes\n", argv[0]);
		return 1;
	}
	if (pthread_attr_setstacksize(&attr, DISTURBER_THR_STACK) != 0) {
		fprintf(stderr, "%s: Failed to set thread stack size\n", argv[0]);
		return 1;
	}

	struct sched_param sched = { .sched_priority = 1 };
	if (pthread_attr_setschedparam(&attr, &sched) != 0) {
		fprintf(stderr, "%s: Failed to set thread priority\n", argv[0]);
		return 1;
	}

	for (i = 0; i < NDISTURBERS; ++i) {
		if (enabled[i] == 0) {
			continue;
		}

		if (disturbers[i]->init != NULL) {
			disturbers[i]->init();
		}

		for (j = 0; j < disturbers[i]->nthreads; ++j) {
			if (nthreads >= MAX_THREADS) {
				fprintf(stderr, "%s: Too many threads\n", argv[0]);
				return 1;
			}
			if (pthread_create(&threads[nthreads], &attr, disturbers[i]->worker, (void *)(intptr_t)(disturbers[i]->firstThreadId + j)) != 0) {
				fprintf(stderr, "%s: Failed to create %s thread %d\n", argv[0], disturbers[i]->name, j);
				return 1;
			}
			++nthreads;
		}
	}

	for (j = 0; j < nthreads; ++j) {
		pthread_join(threads[j], NULL);
	}

	fprintf(stderr, "%s: All threads exited!\n", argv[0]);

	return 0;
}
