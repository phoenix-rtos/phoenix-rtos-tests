/*
 * Phoenix-RTOS
 *
 * Meterfs test file abstraction
 *
 * Copyright 2021 Phoenix Systems
 * Author: Tomasz Korniluk
 *
 * %LICENSE%
 */

#include <string.h>
#include <limits.h>
#include <host-flashsrv.h>
#include <meterfs.h>

#include "file.h"

#define FLASHSIZE (4 * 1024 * 1024)
#define SECTORSIZE (4 * 1024)

int file_lookup(const char *name)
{
	id_t id;

	return hostflashsrv_lookup(name, &id);
}


int file_open(const char *name)
{
	int err;
	id_t id;

	if ((err = hostflashsrv_lookup(name, &id)) < 0)
		return err;

	if (id > INT_MAX)
		return -1;

	if ((err = hostflashsrv_open(&id)) < 0)
		return err;

	return (int)id; 
}


int file_close(id_t fid)
{
	return hostflashsrv_close(&fid);
}


int file_write(id_t fid, const void *buff, size_t bufflen)
{
	return hostflashsrv_writeFile(&fid, buff, bufflen);
}


int file_read(id_t fid, off_t offset, void *buff, size_t bufflen)
{
	return hostflashsrv_readFile(&fid, offset, buff, bufflen);
}


int file_allocate(const char *name, size_t sectors, size_t filesz, size_t recordsz)
{
	meterfs_i_devctl_t iptr;
	meterfs_o_devctl_t optr;

	iptr.type = meterfs_allocate;
	strncpy(iptr.allocate.name, name, sizeof(iptr.allocate.name));
	iptr.allocate.sectors = sectors;
	iptr.allocate.filesz = filesz;
	iptr.allocate.recordsz = recordsz;

	return hostflashsrv_devctl(&iptr, &optr);
}


int file_resize(id_t fid, size_t filesz, size_t recordsz)
{
	meterfs_i_devctl_t iptr;
	meterfs_o_devctl_t optr;

	iptr.type = meterfs_resize;
	iptr.resize.id = fid;
	iptr.resize.filesz = filesz;
	iptr.resize.recordsz = recordsz;

	return hostflashsrv_devctl(&iptr, &optr);
}


int file_getInfo(id_t fid, size_t *sectors, size_t *filesz, size_t *recordsz, size_t *recordcnt)
{
	meterfs_i_devctl_t iptr;
	meterfs_o_devctl_t optr;
	int err;

	iptr.type = meterfs_info;
	iptr.id = fid;

	if ((err = hostflashsrv_devctl(&iptr, &optr)) < 0)
		return err;

	if (sectors != NULL)
		(*sectors) = optr.info.sectors;

	if (filesz != NULL)
		(*filesz) = optr.info.filesz;

	if (recordsz != NULL)
		(*recordsz) = optr.info.recordsz;

	if (recordcnt != NULL)
		(*recordcnt) = optr.info.recordcnt;

	return 0;
}


int file_eraseAll(void)
{
	meterfs_i_devctl_t iptr;
	meterfs_o_devctl_t optr;

	iptr.type = meterfs_chiperase;

	return hostflashsrv_devctl(&iptr, &optr);
}


void file_init(const char *path)
{
	size_t filesz = FLASHSIZE;
	size_t sectorsz = SECTORSIZE;

	if (hostflashsrv_init(&filesz, &sectorsz, path) < 0)
		printf("hostflashsrv: init failed\n");
}
