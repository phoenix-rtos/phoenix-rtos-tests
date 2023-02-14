/*
 * Phoenix-RTOS
 *
 * Meterfs test file abstraction
 *
 * Copyright 2021, 2023 Phoenix Systems
 * Author: Aleksander Kaminski, Andrzej Glowinski, Tomasz Korniluk, Hubert Badocha
 *
 * %LICENSE%
 */

#include <stdio.h>
#include <sys/msg.h>
#include <string.h>
#include <unistd.h>
#include <meterfs.h>
#include <errno.h>

#include "file.h"
#include "unity_fixture.h"

static oid_t meterfs;
static const char *pathPrefix;

static inline void file_prepareDevCtl(msg_t *msg)
{
	msg->type = mtDevCtl;
	msg->i.data = NULL;
	msg->i.size = 0;
	msg->o.data = NULL;
	msg->o.size = 0;
}


static int lookup_rel(const char *name, oid_t *file, oid_t *dev)
{
	char buffer[64];
	if (snprintf(buffer, sizeof(buffer), "%s/%s", pathPrefix, name) >= sizeof(buffer)) {
		return -ENAMETOOLONG;
	}
	return lookup(buffer, file, dev);
}


int file_lookup(const char *name)
{
	oid_t oid;

	return lookup_rel(name, &oid, NULL);
}


int file_open(const char *name)
{
	msg_t msg;
	int err;
	id_t id;

	err = lookup_rel(name, &msg.i.openclose.oid, NULL);
	if (err < 0) {
		return err;
	}

	id = msg.i.openclose.oid.id;

	msg.type = mtOpen;
	msg.i.data = NULL;
	msg.i.size = 0;
	msg.o.data = NULL;
	msg.o.size = 0;
	msg.i.openclose.flags = 0;

	TEST_ASSERT_EQUAL(0, msgSend(meterfs.port, &msg));

	return (msg.o.io.err < 0) ? msg.o.io.err : id;
}


int file_close(id_t fid)
{
	msg_t msg;

	msg.type = mtClose;
	msg.i.data = NULL;
	msg.i.size = 0;
	msg.o.data = NULL;
	msg.o.size = 0;
	msg.i.openclose.oid.port = meterfs.port;
	msg.i.openclose.oid.id = fid;

	TEST_ASSERT_EQUAL(0, msgSend(meterfs.port, &msg));

	return msg.o.io.err;
}


int file_write(id_t fid, const void *buff, size_t bufflen)
{
	msg_t msg;

	msg.type = mtWrite;
	msg.i.io.oid.port = meterfs.port;
	msg.i.io.oid.id = fid;
	msg.i.io.offs = 0;
	msg.i.io.len = bufflen;
	msg.i.io.mode = 0;
	/* FIXME: should be const in kernel msg.h, casting should not be needed. Fixed in posix_rev */
	msg.i.data = (void *)buff;
	msg.i.size = bufflen;
	msg.o.data = NULL;
	msg.o.size = 0;

	TEST_ASSERT_EQUAL(0, msgSend(meterfs.port, &msg));

	return msg.o.io.err;
}

int file_read(id_t fid, off_t offset, void *buff, size_t bufflen)
{
	msg_t msg;

	msg.type = mtRead;
	msg.i.io.oid.port = meterfs.port;
	msg.i.io.oid.id = fid;
	msg.i.io.offs = offset;
	msg.i.io.len = bufflen;
	msg.i.io.mode = 0;
	msg.o.data = buff;
	msg.o.size = bufflen;
	msg.i.data = NULL;
	msg.i.size = 0;

	TEST_ASSERT_EQUAL(0, msgSend(meterfs.port, &msg));

	return msg.o.io.err;
}


int file_allocate(const char *name, size_t sectors, size_t filesz, size_t recordsz)
{
	msg_t msg;
	meterfs_i_devctl_t *iptr = (meterfs_i_devctl_t *)msg.i.raw;
	meterfs_o_devctl_t *optr = (meterfs_o_devctl_t *)msg.o.raw;
	size_t len = 0;

	file_prepareDevCtl(&msg);

	iptr->type = meterfs_allocate;
	len = strnlen(name, sizeof(iptr->allocate.name));
	(void)memcpy(iptr->allocate.name, name, len);
	if (len < sizeof(iptr->allocate.name)) {
		iptr->allocate.name[len] = '\0';
	}
	iptr->allocate.sectors = sectors;
	iptr->allocate.filesz = filesz;
	iptr->allocate.recordsz = recordsz;

	TEST_ASSERT_EQUAL(0, msgSend(meterfs.port, &msg));

	return optr->err;
}


int file_resize(id_t fid, size_t filesz, size_t recordsz)
{
	msg_t msg;
	meterfs_i_devctl_t *iptr = (meterfs_i_devctl_t *)msg.i.raw;
	meterfs_o_devctl_t *optr = (meterfs_o_devctl_t *)msg.o.raw;

	file_prepareDevCtl(&msg);

	iptr->type = meterfs_resize;
	iptr->resize.id = fid;
	iptr->resize.filesz = filesz;
	iptr->resize.recordsz = recordsz;

	TEST_ASSERT_EQUAL(0, msgSend(meterfs.port, &msg));

	return optr->err;
}


int file_getInfo(id_t fid, size_t *sectors, size_t *filesz, size_t *recordsz, size_t *recordcnt)
{
	msg_t msg;
	meterfs_i_devctl_t *iptr = (meterfs_i_devctl_t *)msg.i.raw;
	meterfs_o_devctl_t *optr = (meterfs_o_devctl_t *)msg.o.raw;

	file_prepareDevCtl(&msg);

	iptr->type = meterfs_info;
	iptr->id = fid;

	TEST_ASSERT_EQUAL(0, msgSend(meterfs.port, &msg));

	if (optr->err < 0) {
		return optr->err;
	}

	if (sectors != NULL) {
		(*sectors) = optr->info.sectors;
	}

	if (filesz != NULL) {
		(*filesz) = optr->info.filesz;
	}

	if (recordsz != NULL) {
		(*recordsz) = optr->info.recordsz;
	}

	if (recordcnt != NULL) {
		(*recordcnt) = optr->info.recordcnt;
	}

	return 0;
}


int file_eraseAll(void)
{
	msg_t msg;
	meterfs_i_devctl_t *iptr = (meterfs_i_devctl_t *)msg.i.raw;
	meterfs_o_devctl_t *optr = (meterfs_o_devctl_t *)msg.o.raw;

	file_prepareDevCtl(&msg);

	iptr->type = meterfs_chiperase;

	TEST_ASSERT_EQUAL(0, msgSend(meterfs.port, &msg));

	return optr->err;
}


int file_devInfo(file_fsInfo_t *fsInfo)
{
	msg_t msg;
	meterfs_i_devctl_t *iptr = (meterfs_i_devctl_t *)msg.i.raw;
	meterfs_o_devctl_t *optr = (meterfs_o_devctl_t *)msg.o.raw;

	file_prepareDevCtl(&msg);

	iptr->type = meterfs_fsInfo;

	TEST_ASSERT_EQUAL(0, msgSend(meterfs.port, &msg));

	fsInfo->filecnt = optr->fsInfo.filecnt;
	fsInfo->fileLimit = optr->fsInfo.fileLimit;
	fsInfo->sz = optr->fsInfo.sz;
	fsInfo->sectorsz = optr->fsInfo.sectorsz;

	return optr->err;
}


int file_init(const char *path)
{
	pathPrefix = path;
	return lookup(path, NULL, &meterfs);
}
