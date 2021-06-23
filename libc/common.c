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
#include <fcntl.h>
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
