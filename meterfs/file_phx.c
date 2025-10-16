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
#include <unity.h>

#include "file.h"

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


/*
 * Function with msg pass, to not allocate two messages in case of open
 * FIXME: reconsider abstraction API
 */
static int lookupMsg(msg_t *msg, const char *name)
{
	msg->type = mtLookup;
	msg->oid = (oid_t) { .port = meterfs.port, .id = -1 };
	msg->i.data = name;
	msg->i.size = strlen(name) + 1;
	msg->o.data = NULL;
	msg->o.size = 0;

	TEST_ASSERT_EQUAL(0, msgSend(meterfs.port, msg));

	if (msg->o.err < 0) {
		return msg->o.err;
	}

	TEST_ASSERT_LESS_OR_EQUAL_INT_MESSAGE(INT_MAX, msg->o.lookup.fil.id, "TEST ERROR: file ID too big");

	return msg->o.lookup.fil.id;
}


int file_lookup(const char *name)
{
	msg_t msg;
	return lookupMsg(&msg, name);
}


int file_open(const char *name)
{
	msg_t msg;
	int id;


	id = lookupMsg(&msg, name);
	if (id < 0) {
		return id;
	}

	msg.type = mtOpen;
	msg.oid = (oid_t) { .port = meterfs.port, .id = (id_t)id };
	msg.i.data = NULL;
	msg.i.size = 0;
	msg.o.data = NULL;
	msg.o.size = 0;
	msg.i.openclose.flags = 0;

	TEST_ASSERT_EQUAL(0, msgSend(meterfs.port, &msg));

	return (msg.o.err < 0) ? msg.o.err : id;
}


int file_close(id_t fid)
{
	msg_t msg;

	msg.type = mtClose;
	msg.i.data = NULL;
	msg.i.size = 0;
	msg.o.data = NULL;
	msg.o.size = 0;
	msg.oid.port = meterfs.port;
	msg.oid.id = fid;

	TEST_ASSERT_EQUAL(0, msgSend(meterfs.port, &msg));

	return msg.o.err;
}


int file_write(id_t fid, const void *buff, size_t bufflen)
{
	msg_t msg;

	msg.type = mtWrite;
	msg.oid.port = meterfs.port;
	msg.oid.id = fid;
	msg.i.io.offs = 0;
	msg.i.io.len = bufflen;
	msg.i.io.mode = 0;
	msg.i.data = buff;
	msg.i.size = bufflen;
	msg.o.data = NULL;
	msg.o.size = 0;

	TEST_ASSERT_EQUAL(0, msgSend(meterfs.port, &msg));

	return msg.o.err;
}

int file_read(id_t fid, off_t offset, void *buff, size_t bufflen)
{
	msg_t msg;

	msg.type = mtRead;
	msg.oid.port = meterfs.port;
	msg.oid.id = fid;
	msg.i.io.offs = offset;
	msg.i.io.len = bufflen;
	msg.i.io.mode = 0;
	msg.o.data = buff;
	msg.o.size = bufflen;
	msg.i.data = NULL;
	msg.i.size = 0;

	TEST_ASSERT_EQUAL(0, msgSend(meterfs.port, &msg));

	return msg.o.err;
}


int file_allocate(const char *name, size_t sectors, size_t filesz, size_t recordsz)
{
	msg_t msg;
	meterfs_i_devctl_t *iptr = (meterfs_i_devctl_t *)msg.i.raw;
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

	return msg.o.err;
}


int file_resize(id_t fid, size_t filesz, size_t recordsz)
{
	msg_t msg;
	meterfs_i_devctl_t *iptr = (meterfs_i_devctl_t *)msg.i.raw;

	file_prepareDevCtl(&msg);

	iptr->type = meterfs_resize;
	iptr->resize.id = fid;
	iptr->resize.filesz = filesz;
	iptr->resize.recordsz = recordsz;

	TEST_ASSERT_EQUAL(0, msgSend(meterfs.port, &msg));

	return msg.o.err;
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

	if (msg.o.err < 0) {
		return msg.o.err;
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

	file_prepareDevCtl(&msg);

	iptr->type = meterfs_chiperase;

	TEST_ASSERT_EQUAL(0, msgSend(meterfs.port, &msg));

	return msg.o.err;
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

	return msg.o.err;
}


int file_init(const char *path)
{
	pathPrefix = path;
	return lookup(path, NULL, &meterfs);
}
