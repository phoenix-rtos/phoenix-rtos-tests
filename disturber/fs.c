/*
 * Phoenix-RTOS
 *
 * Disturber FS
 *
 * Filesystem stressing module
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
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <sched.h>
#include <sys/stat.h>

#include "disturber.h"


/* Size of each fill file */
#define FILL_CHUNK PAGE_SIZE


/* Static buffers */
static unsigned char fillBuf[FILL_CHUNK];
static unsigned char fillReadBuf[FILL_CHUNK];
static unsigned int fillFiles;
static unsigned long long fillTotal;
static unsigned int fillDirs;


static unsigned long get_thread_tag(void)
{
	return (unsigned long)(uintptr_t)pthread_self();
}


static void make_name(char *name, size_t size, const char *type, unsigned int index)
{
	(void)snprintf(name, size, ".disturber-%s-%lx-%u", type, get_thread_tag(), index);
}


/* Fills the current fill file up to one page, returns 0 when no more data fits */
static int fs_fileStep(unsigned long long *gained)
{
	char name[96];
	off_t cur;
	ssize_t len = 0;
	int fd;

	make_name(name, sizeof(name), "fill", fillFiles);
	fd = open(name, O_CREAT | O_WRONLY | O_APPEND, 0600);
	if (fd < 0) {
		return 0;
	}

	/* Resume a file left partially written by a previous ENOSPC */
	cur = lseek(fd, 0, SEEK_END);
	if ((cur >= 0) && (cur < (off_t)sizeof(fillBuf))) {
		len = write(fd, fillBuf + cur, sizeof(fillBuf) - (size_t)cur);
		if (len > 0) {
			*gained += (unsigned long long)len;
			cur += len;
		}
	}
	close(fd);

	if (cur < (off_t)sizeof(fillBuf)) {
		return 0;
	}

	++fillFiles;
	return 1;
}


/* Creates one directory, returns 0 when mkdir fails (e.g. no space or inodes left) */
static int fs_dirStep(unsigned int *gained)
{
	char name[96];

	make_name(name, sizeof(name), "dir", fillDirs);
	if (mkdir(name, 0700) == 0) {
		++(*gained);
	}
	else if (errno != EEXIST) {
		return 0;
	}

	++fillDirs;
	return 1;
}


/* Alternately appends file data and creates directories until neither fits */
static void fs_fill(unsigned long long *bytes, unsigned int *dirs)
{
	int filesOk = 1, dirsOk = 1;

	*bytes = 0;
	*dirs = 0;

	while ((filesOk != 0) || (dirsOk != 0)) {
		if (filesOk != 0) {
			filesOk = fs_fileStep(bytes);
		}
		if (dirsOk != 0) {
			dirsOk = fs_dirStep(dirs);
		}

		/* Let other threads compete for the space instead of grabbing it all at once */
		(void)sched_yield();
	}

	fillTotal += *bytes;
}


/* Reads, verifies and rewrites in place all fill files */
static void fs_io_pass(void)
{
	char name[96];
	unsigned int i;
	off_t off;
	ssize_t len;
	int fd;

	for (i = 0; i <= fillFiles; ++i) {
		make_name(name, sizeof(name), "fill", i);
		fd = open(name, O_RDWR);
		if (fd < 0) {
			continue;
		}

		off = 0;
		while ((len = read(fd, fillReadBuf, sizeof(fillReadBuf))) > 0) {
			if (memcmp(fillReadBuf, fillBuf, (size_t)len) != 0) {
				fprintf(stderr, "Disturber FS: fill data corruption in %s at offset %lld\n", name, (long long)off);
			}

			if (lseek(fd, off, SEEK_SET) == off) {
				(void)write(fd, fillReadBuf, (size_t)len);
			}
			off += len;
			if (lseek(fd, off, SEEK_SET) != off) {
				break;
			}
		}

		close(fd);
	}
}


static void fs_hog(void)
{
	unsigned long long gained;
	unsigned int dirs;

	for (size_t i = 0; i < sizeof(fillBuf); ++i) {
		fillBuf[i] = (unsigned char)i;
	}

	fs_fill(&gained, &dirs);
	printf("Disturber FS: filesystem full, holding %llu bytes in %u full file(s) and %u dir(s)\n", fillTotal, fillFiles, fillDirs);

	while (1) {
		if (disturber_sleepMs > 0) {
			usleep(disturber_sleepMs * 1000U);
		}

		fs_io_pass();

		fs_fill(&gained, &dirs);
		if ((gained != 0U) || (dirs != 0U)) {
			printf("Disturber FS: added %llu bytes and %u dir(s), holding %llu bytes and %u dir(s)\n", gained, dirs, fillTotal, fillDirs);
		}
	}
}


static void *worker_thread(void *arg)
{
	(void)arg;

	fs_hog();

	return NULL;
}


const disturber_t disturber_fs = {
	.name = "fs",
	.nthreads = 1,
	.firstThreadId = 1,
	.init = NULL,
	.worker = worker_thread,
};
