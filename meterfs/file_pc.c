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

#define FLASHSIZE  (4 * 1024 * 1024)
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

	err = hostflashsrv_lookup(name, &id);
	if (err < 0) {
		return err;
	}

	if (id > INT_MAX) {
		return -1;
	}

	err = hostflashsrv_open(&id);
	if (err < 0) {
		return err;
	}

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
	size_t len = 0;

	iptr.type = meterfs_allocate;
	len = strnlen(name, sizeof(iptr.allocate.name));
	(void)memcpy(iptr.allocate.name, name, len);
	if (len < sizeof(iptr.allocate.name)) {
		iptr.allocate.name[len] = '\0';
	}
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

	err = hostflashsrv_devctl(&iptr, &optr);
	if (err < 0) {
		return err;
	}

	if (sectors != NULL) {
		(*sectors) = optr.info.sectors;
	}

	if (filesz != NULL) {
		(*filesz) = optr.info.filesz;
	}

	if (recordsz != NULL) {
		(*recordsz) = optr.info.recordsz;
	}

	if (recordcnt != NULL) {
		(*recordcnt) = optr.info.recordcnt;
	}

	return 0;
}


int file_eraseAll(void)
{
	meterfs_i_devctl_t iptr;
	meterfs_o_devctl_t optr;

	iptr.type = meterfs_chiperase;

	return hostflashsrv_devctl(&iptr, &optr);
}


int file_devInfo(file_fsInfo_t *fsInfo)
{
	meterfs_i_devctl_t iptr;
	meterfs_o_devctl_t optr;
	int err;

	iptr.type = meterfs_fsInfo;

	err = hostflashsrv_devctl(&iptr, &optr);
	if (err < 0) {
		return err;
	}

	fsInfo->filecnt = optr.fsInfo.filecnt;
	fsInfo->fileLimit = optr.fsInfo.fileLimit;
	fsInfo->sz = optr.fsInfo.sz;
	fsInfo->sectorsz = optr.fsInfo.sectorsz;

	return 0;
}


int file_init(const char *path)
{
	size_t filesz = FLASHSIZE;
	size_t sectorsz = SECTORSIZE;
	int err;

	err = hostflashsrv_init(&filesz, &sectorsz, path);
	if (err < 0) {
		(void)printf("hostflashsrv: init failed\n");
	}
	return err;
}
