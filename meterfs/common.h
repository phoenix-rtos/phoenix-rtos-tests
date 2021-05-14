/*
 * Phoenix-RTOS
 *
 * Meterfs tests common part
 *
 * Copyright 2021 Phoenix Systems
 * Author: Tomasz Korniluk
 *
 *
 * %LICENSE%
 */

#ifndef COMMON_H
#define COMMON_H

#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "file.h"
#include <unity_fixture.h>

typedef struct file_info_t {
	size_t sectors;
	size_t filesz;
	size_t recordsz;
	size_t recordcnt;
} file_info_t;

int common_preallocOpenFile(const char *name, size_t sectors, size_t filesz, size_t recordsz);


void common_readContent(int fd, size_t offset, void *buff, size_t bufflen, void *content, int contentsz, const char *msg);


void common_fileInfoCompare(const file_info_t *info, const file_info_t *pattern, const char *msg);

#endif
