/*
 * Phoenix-RTOS
 *
 * Filesystem benchmark - based on lmbench (https://github.com/intel/lmbench/blob/master/src/lat_fs.c)
 *
 * Copyright 2020 Phoenix Systems
 * Author: Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "errno.h"
#include "fcntl.h"
#include "inttypes.h"
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"

#include "sys/stat.h"
#include "sys/time.h"


/* Misc definitions */
#define DIR_NAME      "test_fs_XXXXXX" /* Test directory name template */
#define DIR_NAME_FMT  "%s/" DIR_NAME   /* Test directory path format */
#define DIR_MAX_FILES 100              /* Max number of files per directory */
#define NFILES        1000             /* Number of files to create/remove per test */


typedef struct timeval timeval_t;


/* Files sizes */
static const unsigned int fsizes[] = { 0x0, 0x400, 0x1000, 0x2800 };


typedef struct {
	char *tmp;           /* Root directory */
	char **dirs;         /* Directory names */
	char **names;        /* File names */
	unsigned int ndirs;  /* Number of directories */
	unsigned int nfiles; /* Number of files */
	unsigned int fmax;   /* Max number of files per directory */
} test_fs_state_t;


/* Calculates time delta in usec */
static uint64_t test_fs_gettime(timeval_t *start, timeval_t *end)
{
	timeval_t diff;

	diff.tv_sec = end->tv_sec - start->tv_sec;
	diff.tv_usec = end->tv_usec - start->tv_usec;

	if ((diff.tv_usec < 0) && (diff.tv_sec > 0)) {
		diff.tv_sec--;
		diff.tv_usec += 1000000;
	}

	if ((diff.tv_usec < 0) || (end->tv_sec < start->tv_sec)) {
		diff.tv_sec = 0;
		diff.tv_usec = 0;
	}

	return diff.tv_usec + 1000000 * diff.tv_sec;
}


/* Counts digits */
static unsigned int test_fs_digits(unsigned int n, unsigned int base)
{
	unsigned int d = 1;

	while (n /= base)
		d++;

	return d;
}


/* Test cleanup - removes files and directories */
static void test_fs_cleanup(test_fs_state_t *state)
{
	unsigned int i;

	for (i = 0; i < state->nfiles; i++) {
		unlink(state->names[i]);
		free(state->names[i]);
	}
	free(state->names);

	for (i = state->ndirs; i--;) {
		rmdir(state->dirs[i]);
		free(state->dirs[i]);
	}
	free(state->dirs);
}


/* Creates directory structure and generates filenames */
static int test_fs_setupr(unsigned int *foffs, unsigned int *doffs, unsigned int depth, test_fs_state_t *state)
{
	unsigned int i, err, ndirs, plen;
	char *pdir;

	pdir = state->dirs[*doffs];
	plen = strlen(pdir);

	if (depth > 0) {
		for (i = 0, ndirs = 1; i < depth; i++)
			ndirs *= state->fmax;
		ndirs = (state->nfiles - *foffs) / ndirs + 1;

		for (i = 0; (i < state->fmax) && (i < ndirs) && (*foffs < state->nfiles); i++) {
			if ((state->dirs[*doffs + 1] = (char *)malloc(plen + test_fs_digits(i, 10) + 2)) == NULL)
				return -ENOMEM;
			sprintf(state->dirs[*doffs + 1], "%s/%u", pdir, i);

			if ((err = mkdir(state->dirs[++(*doffs)], 0777)) < 0)
				return err;

			if ((err = test_fs_setupr(foffs, doffs, depth - 1, state)) < 0)
				return err;
		}
	}
	else {
		for (i = 0; (i < state->fmax) && (*foffs < state->nfiles); i++) {
			if ((state->names[*foffs] = (char *)malloc(plen + test_fs_digits(i, 10) + 2)) == NULL)
				return -ENOMEM;
			sprintf(state->names[(*foffs)++], "%s/%u", pdir, i);
		}
	}

	return EOK;
}


/* Calculates number of directories per depth level */
static inline int test_fs_dirs(unsigned int nfiles, unsigned int fmax)
{
	return nfiles / fmax + !!(nfiles % fmax);
}


/* Test setup - creates directory structure and generates filenames */
static int test_fs_setup(test_fs_state_t *state)
{
	unsigned int i, err, depth, foffs = 0, doffs = 0;
	char *dir;

	if (!state->nfiles || (state->fmax < 2))
		return -EINVAL;

	state->ndirs = test_fs_dirs(state->nfiles, state->fmax);
	for (i = state->ndirs, depth = 0; i > 1; state->ndirs += (i = test_fs_dirs(i, state->fmax)), depth++);

	if ((state->names = (char **)malloc(state->nfiles * sizeof(char *))) == NULL)
		return -ENOMEM;

	for (i = 0; i < state->nfiles; i++)
		state->names[i] = NULL;

	if ((state->dirs = (char **)malloc(state->ndirs * sizeof(char *))) == NULL) {
		free(state->names);
		return -ENOMEM;
	}

	for (i = 0; i < state->ndirs; i++)
		state->dirs[i] = NULL;

	if ((dir = (char *)malloc(strlen(state->tmp) + sizeof(DIR_NAME) + 1)) == NULL) {
		free(state->dirs);
		free(state->names);
		return -ENOMEM;
	}

	sprintf(dir, DIR_NAME_FMT, state->tmp);

	if (mkdtemp(dir) == NULL) {
		free(dir);
		free(state->dirs);
		free(state->names);
		return -EEXIST;
	}

	state->dirs[0] = dir;
	if ((err = test_fs_setupr(&foffs, &doffs, depth, state)) < 0) {
		test_fs_cleanup(state);
		return err;
	}

	return EOK;
}


/* Creates new file and fills it with len bytes of data */
static int test_fs_mkfile(char *name, unsigned int len)
{
	char buff[fsizes[sizeof(fsizes) / sizeof(fsizes[0]) - 1]];
	int fd, ret;

	if((fd = creat(name, DEFFILEMODE)) < 0) {
		fprintf(stderr, "test_fs: failed to create and open file %s\n", name);
		return fd;
	}

	while (len > 0) {
		if ((ret = write(fd, buff, len)) < 0) {
			fprintf(stderr, "test_fs: failed to write to file %s\n", name);
			unlink(name);
			return ret;
		}
		len -= ret;
	}

	if ((ret = close(fd)) < 0) {
		fprintf(stderr, "test_fs: failed to close file %s\n", name);
		unlink(name);
		return ret;
	}

	return EOK;
}


/* Measures file create/remove time */
static int test_fs_run(test_fs_state_t *state)
{
	timeval_t start, end;
	unsigned int i, j;
	uint64_t time;
	int err;

	for (i = 0; i < sizeof(fsizes) / sizeof(fsizes[0]); i++) {
		for (j = 0, time = 0; j < state->nfiles; j++) {
			gettimeofday(&start, NULL);

			if ((err = test_fs_mkfile(state->names[j], fsizes[i])) < 0)
				return err;

			gettimeofday(&end, NULL);
			time += test_fs_gettime(&start, &end);
		}
		printf("test_fs: average %uKB file create time: %" PRIu64 "ms\n", fsizes[i] / (1 << 10), time / 1000 / state->nfiles);

		for (j = 0, time = 0; j < state->nfiles; j++) {
			gettimeofday(&start, NULL);

			if ((err = unlink(state->names[j])) < 0) {
				fprintf(stderr, "test_fs: failed to remove file %s\n", state->names[j]);
				return err;
			}

			gettimeofday(&end, NULL);
			time += test_fs_gettime(&start, &end);
		}
		printf("test_fs: average %uKB file remove time: %" PRIu64 "ms\n", fsizes[i] / (1 << 10), time / 1000 /state->nfiles);
	}

	return EOK;
}


int main(int argc, char *argv[])
{
	test_fs_state_t state = { .nfiles = NFILES, .fmax = DIR_MAX_FILES };
	int err;

	if (argc != 2) {
		printf("Usage: %s <tmp dir>\n", argv[0]);
		return EOK;
	}
	state.tmp = argv[1];

	printf("test_fs: starting, main is at %p\n", main);

	if ((err = test_fs_setup(&state)) < 0) {
		fprintf(stderr, "test_fs: failed on test setup\n");
		return err;
	}

	err = test_fs_run(&state);
	test_fs_cleanup(&state);

	return err;
}
