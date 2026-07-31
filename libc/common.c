/*
 * Phoenix-RTOS
 *
 * libc-tests
 *
 * Helpers for common steps during unit testing.
 *
 * Copyright 2021 Phoenix Systems
 * Author: Marek Bialowas
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "common.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <unity_fixture.h>


int _create_file(const char *path, const char *data)
{
	int fd;

	if ((fd = open(path, O_WRONLY | O_CREAT, 0666)) < 0)
		return -1;

	/* NOTE: assuming non-interrupted write */
	if (data) {
		int datasz = strlen(data);
		if (write(fd, data, datasz) != datasz) {
			close(fd);
			return -1;
		}
	}

	close(fd);
	return 0;
}


int _read_file(const char *path, char *buf, size_t bufsz)
{
	int fd, ret = 0;

	if ((fd = open(path, O_RDONLY)) < 0)
		return -1;

	/* NOTE: assuming non-interrupted read */
	if (buf) {
		ret = read(fd, buf, bufsz);
	}

	close(fd);
	return ret;
}


int libc_createDirIfMissing(const char *path)
{
	struct stat buffer;

	if (stat(path, &buffer) == 0) {
		return 0;
	}

	if (errno != ENOENT) {
		fprintf(stderr, "stat() on %s directory failed: %s\n", path, strerror(errno));
		return -1;
	}

	if (mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0) {
		fprintf(stderr, "Creating %s directory by mkdir failed: %s\n", path, strerror(errno));
		return -1;
	}

	return 0;
}


int libc_createFileIfMissing(const char *path, const char *data)
{
	struct stat buffer;

	if (stat(path, &buffer) == 0) {
		return 0;
	}

	if (errno != ENOENT) {
		fprintf(stderr, "stat() on %s file failed: %s\n", path, strerror(errno));
		return -1;
	}

	if (_create_file(path, data) != 0) {
		fprintf(stderr, "Creating %s file failed: %s\n", path, strerror(errno));
		return -1;
	}

	return 0;
}
