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

#include "common.h"

static struct {
	char buff[128];
} common;

int common_preallocOpenFile(const char *name, size_t sectors, size_t filesz, size_t recordsz)
{
	int fd;

	TEST_ASSERT_EQUAL(0, file_allocate(name, sectors, filesz, recordsz));
	snprintf(common.buff, sizeof(common.buff), "/%s", name);
	fd = file_open(common.buff);
	TEST_ASSERT_GREATER_OR_EQUAL(0, fd);

	memset(common.buff, 0, sizeof(common.buff));

	return fd;
}


void common_readContent(int fd, size_t offset, void *buff, size_t bufflen, void *content, int contentsz, const char *msg)
{
	int i;

	TEST_ASSERT_FALSE_MESSAGE(bufflen < contentsz, "Wrong arguments bufflen < contentsz!");

	memcpy(common.buff, content, contentsz);
	TEST_ASSERT_EQUAL_MESSAGE(bufflen, file_read(fd, offset, buff, bufflen), msg);
	for (i = contentsz; i < bufflen; ++i)
		common.buff[i] = 0xff;
	TEST_ASSERT_EQUAL_HEX8_ARRAY_MESSAGE(common.buff, buff, bufflen, msg);

	memset(common.buff, 0, sizeof(common.buff));
}


void common_fileInfoCompare(const file_info_t *info, const file_info_t *pattern, const char *msg)
{
	TEST_ASSERT_EQUAL_MESSAGE(pattern->sectors, info->sectors, msg);
	TEST_ASSERT_EQUAL_MESSAGE(pattern->recordcnt, info->recordcnt, msg);
	TEST_ASSERT_EQUAL_MESSAGE(pattern->recordsz, info->recordsz, msg);
	TEST_ASSERT_EQUAL_MESSAGE(pattern->filesz, info->filesz, msg);
}
