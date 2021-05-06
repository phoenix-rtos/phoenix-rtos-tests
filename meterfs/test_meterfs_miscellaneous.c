/*
 * Phoenix-RTOS
 *
 * Meterfs miscellaneous tests group
 *
 * Copyright 2021 Phoenix Systems
 * Author: Tomasz Korniluk
 *
 *
 * %LICENSE%
 */

#include "common.h"

static struct {
	int fd;
	char buffRec[40];
	char buffMsg[9];
} common;


static void writeReadCheck(file_info_t *info)
{
	int i;

	for (i = 0; i < info->filesz / info->recordsz; ++i) {
		snprintf(common.buffMsg, sizeof(common.buffMsg), "a%04d", i);
		TEST_ASSERT_EQUAL_MESSAGE(info->recordsz, file_write(common.fd, common.buffMsg, strlen(common.buffMsg)), common.buffMsg);
		common_readContent(common.fd, i * info->recordsz, common.buffRec, info->recordsz, common.buffMsg, strlen(common.buffMsg), common.buffMsg);

		memset(common.buffMsg, 0, sizeof(common.buffMsg));
		memset(common.buffRec, 0, sizeof(common.buffRec));
	}

	TEST_ASSERT_EQUAL(0, file_getInfo(common.fd, &info->sectors, &info->filesz, &info->recordsz, &info->recordcnt));
	TEST_ASSERT_EQUAL(info->filesz / info->recordsz, info->recordcnt);
}


TEST_GROUP(meterfs_miscellaneous);


TEST_SETUP(meterfs_miscellaneous)
{
	common.fd = 0;
}


TEST_TEAR_DOWN(meterfs_miscellaneous)
{
	TEST_ASSERT_EQUAL(0, file_eraseAll());
}


/* Test case of resizing file and getting file info. */
TEST(meterfs_miscellaneous, resize_getinfo)
{	
	file_info_t info;
	file_info_t pattern = { 4, 2000, 20, 0 };

	common.fd = common_preallocOpenFile("file0", pattern.sectors, pattern.filesz, pattern.recordsz);
	TEST_ASSERT_EQUAL(0, file_getInfo(common.fd, &info.sectors, &info.filesz, &info.recordsz, &info.recordcnt));
	common_fileInfoCompare(&info, &pattern, "step1");

	pattern.filesz = 200;
	pattern.recordsz = 5;
	TEST_ASSERT_EQUAL(0, file_resize(common.fd, pattern.filesz, pattern.recordsz));
	TEST_ASSERT_EQUAL(0, file_getInfo(common.fd, &info.sectors, &info.filesz, &info.recordsz, &info.recordcnt));
	common_fileInfoCompare(&info, &pattern, "step2");

	writeReadCheck(&info);
	
	pattern.filesz = 4000;
	pattern.recordsz = 40;
	TEST_ASSERT_EQUAL(0, file_resize(common.fd, pattern.filesz, pattern.recordsz));
	TEST_ASSERT_EQUAL(0, file_getInfo(common.fd, &info.sectors, &info.filesz, &info.recordsz, &info.recordcnt));
	common_fileInfoCompare(&info, &pattern, "step3");

	writeReadCheck(&info);

	TEST_ASSERT_EQUAL(0, file_close(common.fd));
}


/* Test case of resizing file to size bigger than allowed by sectors num. */
TEST(meterfs_miscellaneous, resize_bigger)
{
	file_info_t pattern = { 2, 2000, 20, 0 };

	common.fd = common_preallocOpenFile("file0", pattern.sectors, pattern.filesz, pattern.recordsz);
	pattern.filesz = 8000;
	pattern.recordsz = 40;
	TEST_ASSERT_EQUAL(-EINVAL, file_resize(common.fd, pattern.filesz, pattern.recordsz));

	TEST_ASSERT_EQUAL(0, file_close(common.fd));
}


TEST_GROUP_RUNNER(meterfs_miscellaneous)
{
	RUN_TEST_CASE(meterfs_miscellaneous, resize_getinfo);
	RUN_TEST_CASE(meterfs_miscellaneous, resize_bigger);
}


void runner(void)
{
	RUN_TEST_GROUP(meterfs_miscellaneous);
}


int main(int argc, char *argv[])
{
	file_init(argv[1]);
	TEST_ASSERT_EQUAL(0, file_eraseAll());

	UnityMain(argc, (const char**)argv, runner);

	return 0;
}
