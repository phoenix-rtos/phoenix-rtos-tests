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
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <limits.h>

#include <sys/threads.h>

#include "disturber.h"

#define MAX_THREADS 16


static const disturber_t *const disturbers[] = { &disturber_cpu, &disturber_mem, &disturber_fs };

#define NDISTURBERS (sizeof(disturbers) / sizeof(disturbers[0]))

#define DEFAULT_SLEEP_MS 0U
#define DEFAULT_REPORT_S 5U
#define STARTUP_DELAY_S  5U


unsigned int disturber_sleepMs = DEFAULT_SLEEP_MS;


typedef struct {
	time_t idle; /* Summed cpuTime of idle threads [us] */
	int ncpu;    /* Number of idle threads == number of CPUs */
	int nready;  /* Number of disturber threads in ready (running/runnable) state */
	int nthr;    /* Number of disturber threads */
	int ownMem;  /* vmem of this process [B], -1 if not found */
	int fsMem;   /* vmem of dummyfs process [B], -1 if not found */
} report_sample_t;


static struct {
	threadinfo_t *info;
	int n;
	time_t prevIdle;
	time_t prevTime;
} report_common;


static time_t report_now(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (time_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}


/* Checks whether the first word of process name (argv joined with spaces) has the given basename */
static int report_isProcess(const char *name, const char *bin)
{
	const char *start = name, *p;
	size_t len;

	for (p = name; (*p != '\0') && (*p != ' '); p++) {
		if (*p == '/') {
			start = p + 1;
		}
	}
	len = strlen(bin);

	return (((size_t)(p - start) == len) && (strncmp(start, bin, len) == 0)) ? 1 : 0;
}


/*
 * Idle time: sums cpuTime [us] of the kernel idle threads (one per CPU).
 * Kernel threads without a process are reported with pid 0 and name "[idle]" - that includes
 * the usrv thread too, so only those with the lowest criticality (highest priority value) are taken.
 * Memory: vmem reported per thread is the size of all map entries of its process (as in ps).
 */
static int report_sample(report_sample_t *s)
{
	threadinfo_t *rinfo;
	int i, cnt, prio = INT_MIN;
	pid_t pid = getpid();

	/* Grow the buffer until it fits all threads in the system */
	while ((cnt = threadsinfo(report_common.n, report_common.info)) >= report_common.n) {
		rinfo = realloc(report_common.info, 2 * report_common.n * sizeof(threadinfo_t));
		if (rinfo == NULL) {
			return -ENOMEM;
		}
		report_common.info = rinfo;
		report_common.n *= 2;
	}

	s->idle = 0;
	s->ncpu = 0;
	s->nready = 0;
	s->nthr = 0;
	s->ownMem = -1;
	s->fsMem = -1;
	for (i = 0; i < cnt; i++) {
		threadinfo_t *t = &report_common.info[i];

		if ((pid_t)t->pid == pid) {
			s->ownMem = t->vmem;
			s->nthr++;
			if (t->state == 0) {
				s->nready++;
			}
			continue;
		}
		if (t->pid != 0) {
			if ((s->fsMem < 0) && (report_isProcess(t->name, "dummyfs") != 0)) {
				s->fsMem = t->vmem;
			}
			continue;
		}
		if (t->priority > prio) {
			prio = t->priority;
			s->idle = 0;
			s->ncpu = 0;
		}
		if (t->priority == prio) {
			s->idle += t->cpuTime;
			s->ncpu++;
		}
	}

	/* Exclude the main (reporting) thread - it is always ready while sampling */
	if (s->nthr > 0) {
		s->nthr--;
		s->nready = (s->nready > 0) ? (s->nready - 1) : 0;
	}

	return (s->ncpu > 0) ? 0 : -ENOENT;
}


static int report_init(void)
{
	report_sample_t s;
	int err;

	report_common.n = 64;
	report_common.info = malloc(report_common.n * sizeof(threadinfo_t));
	if (report_common.info == NULL) {
		return -ENOMEM;
	}
	report_common.prevTime = report_now();

	err = report_sample(&s);
	report_common.prevIdle = s.idle;

	return err;
}


static void report_printMem(const char *name, int mem)
{
	if (mem < 0) {
		printf(", %s mem: n/a", name);
	}
	else {
		printf(", %s mem: %u KB", name, (unsigned int)mem / 1024U);
	}
}


/* Prints overall system CPU load [%] since the previous call (100% - idle threads share) and memory usage */
static void report_load(void)
{
	report_sample_t s;
	time_t now, delta, didle;
	unsigned int load;

	if (report_sample(&s) < 0) {
		printf("disturber: load report - cannot read thread info\n");
		return;
	}
	now = report_now();
	delta = (now - report_common.prevTime) * s.ncpu;
	didle = (s.idle > report_common.prevIdle) ? (s.idle - report_common.prevIdle) : 0;

	/* Load in per mille, as in top */
	load = (delta > didle) ? (unsigned int)((delta - didle) * 1000 / delta) : 0;

	printf("disturber: system CPU load in last %u ms: %u.%u%%",
			(unsigned int)((now - report_common.prevTime) / 1000), load / 10, load % 10);
	printf(", disturber threads running: %d ready/%d total", s.nready, s.nthr);
	report_printMem("disturber", s.ownMem);
	report_printMem("dummyfs", s.fsMem);
	report_printMem("total", (s.ownMem < 0) ? s.fsMem : ((s.fsMem < 0) ? s.ownMem : (s.ownMem + s.fsMem)));
	printf("\n");

	report_common.prevIdle = s.idle;
	report_common.prevTime = now;
}


static void usage(const char *progname)
{
	size_t i;

	fprintf(stderr, "Usage: %s [-s sleep_ms] [-r report_s] [disturber...]\n", progname);
	fprintf(stderr, "  -s sleep_ms  sleep between worker iterations in ms (default %u)\n", DEFAULT_SLEEP_MS);
	fprintf(stderr, "  -r report_s  print system CPU load and disturber/dummyfs memory every report_s seconds, 0 disables (default %u)\n", DEFAULT_REPORT_S);
	fprintf(stderr, "Runs all disturbers if none is given. Available:");
	for (i = 0; i < NDISTURBERS; ++i) {
		fprintf(stderr, " %s", disturbers[i]->name);
	}
	fprintf(stderr, "\n");
}


int setMinPriority(void)
{
	for (int i = sched_get_priority_min(SCHED_RR); i < sched_get_priority_max(SCHED_RR); ++i) {
		if (setPriority(i) == EOK) {
			return i;
		}
	}
	return sched_get_priority_min(SCHED_RR); /* Cannot set any priority, keep trying with minimum */
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
	int minPriority;
	unsigned int reportS = DEFAULT_REPORT_S;

	minPriority = setMinPriority();

	while ((opt = getopt(argc, argv, "s:r:h")) != -1) {
		switch (opt) {
			case 's':
				val = strtoul(optarg, &end, 10);
				if ((*optarg == '\0') || (*end != '\0') || (val > UINT32_MAX / 1000U)) {
					fprintf(stderr, "%s: Invalid sleep value '%s'\n", argv[0], optarg);
					return 1;
				}
				disturber_sleepMs = (unsigned int)val;
				break;
			case 'r':
				val = strtoul(optarg, &end, 10);
				if ((*optarg == '\0') || (*end != '\0') || (val > UINT32_MAX)) {
					fprintf(stderr, "%s: Invalid report period '%s'\n", argv[0], optarg);
					return 1;
				}
				reportS = (unsigned int)val;
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

	if (reportS != 0U) {
		/* Allocate before disturbers start (mem disturber exhausts the heap) */
		if (report_init() < 0) {
			fprintf(stderr, "%s: Failed to init load report\n", argv[0]);
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

	struct sched_param sched = { .sched_priority = minPriority };
	if (pthread_attr_setschedparam(&attr, &sched) != 0) {
		fprintf(stderr, "%s: Failed to set thread priority\n", argv[0]);
		return 1;
	}

	/* Let other processes (e.g. dummyfs) start before disturbers grab resources */
	sleep(STARTUP_DELAY_S);

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

	/* Workers never exit - main thread reports load periodically */
	while (reportS != 0U) {
		sleep(reportS);
		report_load();
	}

	for (j = 0; j < nthreads; ++j) {
		pthread_join(threads[j], NULL);
	}

	fprintf(stderr, "%s: All threads exited!\n", argv[0]);

	return 0;
}
