/*
 * Phoenix-RTOS
 *
 * File abstraction
 *
 * Copyright 2021, 2023 Phoenix Systems
 * Author: Tomasz Korniluk, Hubert Badocha
 *
 * %LICENSE%
 */

#ifndef FILE_H
#define FILE_H

#include <sys/types.h>

typedef struct {
	size_t sz;
	size_t sectorsz;
	size_t fileLimit;
	size_t filecnt;
} file_fsInfo_t;


int file_lookup(const char *name);


int file_open(const char *name);


int file_close(id_t fid);


int file_write(id_t fid, const void *buff, size_t bufflen);


int file_read(id_t fid, off_t offset, void *buff, size_t bufflen);


int file_allocate(const char *name, size_t sectors, size_t filesz, size_t recordsz);


int file_resize(id_t fid, size_t filesz, size_t recordsz);


int file_getInfo(id_t fid, size_t *sectors, size_t *filesz, size_t *recordsz, size_t *recordcnt);


int file_eraseAll(void);


int file_devInfo(file_fsInfo_t *fsInfo);


int file_init(const char *path);

#endif
