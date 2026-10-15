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
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>

#include "disturber.h"


static unsigned long get_thread_tag(void)
{
	return (unsigned long)(uintptr_t)pthread_self();
}


static void make_name(char *name, size_t size, const char *type, unsigned int index)
{
	(void)snprintf(name, size, ".disturber-%s-%lx-%u", type, get_thread_tag(), index);
}


static void write_read_files(int n)
{
	char name[96];
	char written[64];
	char readback[64];
	ssize_t len;
	int fd;
	int i;

	for (i = 0; i < n; ++i) {
		make_name(name, sizeof(name), "rw", (unsigned int)i);
		len = snprintf(written, sizeof(written), "thread=%lx file=%d\n", get_thread_tag(), i);
		if ((len < 0) || ((size_t)len >= sizeof(written))) {
			fprintf(stderr, "Disturber FS: snprintf failed\n");
			break;
		}

		fd = open(name, O_CREAT | O_TRUNC | O_RDWR, 0600);
		if (fd < 0) {
			fprintf(stderr, "Disturber FS: cannot create %s: %s\n", name, strerror(errno));
			break;
		}

		if (write(fd, written, (size_t)len) != len) {
			fprintf(stderr, "Disturber FS: write failed for %s\n", name);
		}

		close(fd);
	}

	for (i = 0; i < n; ++i) {
		make_name(name, sizeof(name), "rw", (unsigned int)i);
		len = snprintf(written, sizeof(written), "thread=%lx file=%d\n", get_thread_tag(), i);
		if ((len < 0) || ((size_t)len >= sizeof(written))) {
			fprintf(stderr, "Disturber FS: snprintf failed\n");
			break;
		}

		fd = open(name, O_RDONLY);
		if (fd < 0) {
			fprintf(stderr, "Disturber FS: cannot open %s: %s\n", name, strerror(errno));
			break;
		}

		if ((read(fd, readback, (size_t)len) != len) || (memcmp(written, readback, (size_t)len) != 0)) {
			fprintf(stderr, "Disturber FS: read/write verification failed for %s\n", name);
		}

		close(fd);
		unlink(name);
	}
}


static void create_max_files(void)
{
	char name[96];
	int fd;
	unsigned int i = 0;

	while (1) {
		make_name(name, sizeof(name), "file", i);
		fd = open(name, O_CREAT | O_EXCL | O_WRONLY, 0600);
		if (fd >= 0) {
			(void)write(fd, "x", 1);
			close(fd);
		}
		else if (errno != EEXIST) {
			fprintf(stderr, "Disturber FS: %s error reached for file %d\n", strerror(errno), i);
			break;
		}
		++i;
	}
}


static void remove_files(void)
{
	DIR *dir;
	struct dirent *entry;
	char file_prefix[96];
	char rw_prefix[96];
	int file_prefix_len;
	int rw_prefix_len;

	file_prefix_len = snprintf(file_prefix, sizeof(file_prefix), ".disturber-file-%lx-", get_thread_tag());
	rw_prefix_len = snprintf(rw_prefix, sizeof(rw_prefix), ".disturber-rw-%lx-", get_thread_tag());
	if ((file_prefix_len < 0) || ((size_t)file_prefix_len >= sizeof(file_prefix)) ||
			(rw_prefix_len < 0) || ((size_t)rw_prefix_len >= sizeof(rw_prefix))) {
		return;
	}

	dir = opendir(".");
	if (dir == NULL) {
		fprintf(stderr, "Disturber FS: cannot open current directory: %s\n", strerror(errno));
		return;
	}

	while ((entry = readdir(dir)) != NULL) {
		if ((strncmp(entry->d_name, file_prefix, (size_t)file_prefix_len) != 0) &&
				(strncmp(entry->d_name, rw_prefix, (size_t)rw_prefix_len) != 0)) {
			continue;
		}

		if ((unlink(entry->d_name) < 0) && (errno != ENOENT)) {
			fprintf(stderr, "Disturber FS: cannot remove file %s: %s\n", entry->d_name, strerror(errno));
		}
	}

	closedir(dir);
}


static void create_dirs(int n)
{
	unsigned int i;
	char name[96];

	for (i = 0; i < (unsigned int)n; ++i) {
		make_name(name, sizeof(name), "dir", i);
		if ((mkdir(name, 0700) < 0) && (errno != EEXIST)) {
			fprintf(stderr, "Disturber FS: cannot create directory %s: %s\n", name, strerror(errno));
			break;
		}
	}
}


static void create_max_dirs(void)
{
	char name[96];
	unsigned int i = 0;

	while (1) {
		make_name(name, sizeof(name), "dir", i);
		if ((mkdir(name, 0700) < 0) && (errno != EEXIST)) {
			fprintf(stderr, "Disturber FS: %s error reached for directory %d\n", strerror(errno), i);
			break;
		}
		++i;
	}
}


static void remove_dirs(void)
{
	DIR *dir;
	struct dirent *entry;
	char dir_prefix[96];
	int dir_prefix_len;

	dir_prefix_len = snprintf(dir_prefix, sizeof(dir_prefix), ".disturber-dir-%lx-", get_thread_tag());
	if ((dir_prefix_len < 0) || ((size_t)dir_prefix_len >= sizeof(dir_prefix))) {
		return;
	}

	dir = opendir(".");
	if (dir == NULL) {
		fprintf(stderr, "Disturber FS: cannot open current directory: %s\n", strerror(errno));
		return;
	}

	while ((entry = readdir(dir)) != NULL) {
		if (strncmp(entry->d_name, dir_prefix, (size_t)dir_prefix_len) != 0) {
			continue;
		}

		if ((rmdir(entry->d_name) < 0) && (errno != ENOENT)) {
			fprintf(stderr, "Disturber FS: cannot remove directory %s: %s\n", entry->d_name, strerror(errno));
		}
	}

	closedir(dir);
}


static void *worker_thread(void *arg)
{
	int threadId = (int)(intptr_t)arg;

	while (1) {
		usleep(disturber_sleepMs * 1000U);
		if (threadId == 1) {
			write_read_files(1);
			write_read_files(10);
			create_max_files();
			remove_files();
		}
		if (threadId == 2) {
			create_dirs(1);
			create_dirs(10);
			create_max_dirs();
			remove_dirs();
		}

		printf("Disturber FS: Thread %d full iteration completed\n", threadId);
	}
	return NULL;
}


const disturber_t disturber_fs = {
	.name = "fs",
	.nthreads = 2,
	.firstThreadId = 1,
	.init = NULL,
	.worker = worker_thread,
};
