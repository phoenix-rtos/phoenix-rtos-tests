/*
 * Phoenix-RTOS
 *
 * Meterfs miscellaneous tests group
 *
 * Copyright 2021, 2023 Phoenix Systems
 * Author: Tomasz Korniluk, Hubert Badocha
 *
 *
 * %LICENSE%
 */

#include "common.h"

static struct {
	int fd;
	char buffRec[64];
	char buffMsg[32];
} common;


static file_fsInfo_t fsInfo;


static void writeReadCheck(file_info_t *info)
{
	size_t i;

	for (i = 0; i < (info->filesz / info->recordsz); ++i) {
		(void)snprintf(common.buffMsg, sizeof(common.buffMsg), "a%04zu", i);
		TEST_ASSERT_EQUAL_MESSAGE(info->recordsz, file_write(common.fd, common.buffMsg, strlen(common.buffMsg)), common.buffMsg);
		common_readContent(common.fd, i * info->recordsz, common.buffRec, info->recordsz, common.buffMsg, strlen(common.buffMsg), common.buffMsg);

		(void)memset(common.buffMsg, 0, sizeof(common.buffMsg));
		(void)memset(common.buffRec, 0, sizeof(common.buffRec));
	}

	TEST_ASSERT_EQUAL(0, file_getInfo(common.fd, &info->sectors, &info->filesz, &info->recordsz, &info->recordcnt));
	TEST_ASSERT_EQUAL(info->filesz / info->recordsz, info->recordcnt);
}


TEST_GROUP(meterfs_miscellaneous);


TEST_SETUP(meterfs_miscellaneous)
{
	common.fd = 0;
	TEST_ASSERT_EQUAL(0, file_devInfo(&fsInfo));
}


TEST_TEAR_DOWN(meterfs_miscellaneous)
{
	TEST_ASSERT_EQUAL(0, file_eraseAll());
}


/* Test case of resizing file and getting file info. */
TEST(meterfs_miscellaneous, resize_getinfo)
{
	file_info_t info;
	file_info_t pattern = { 4, fsInfo.sectorsz / 2u, fsInfo.sectorsz / 200u, 0 };

	common.fd = common_preallocOpenFile("file0", pattern.sectors, pattern.filesz, pattern.recordsz);
	TEST_ASSERT_EQUAL(0, file_getInfo(common.fd, &info.sectors, &info.filesz, &info.recordsz, &info.recordcnt));
	common_fileInfoCompare(&info, &pattern, "step1");

	pattern.filesz /= 10;
	pattern.recordsz /= 4;
	TEST_ASSERT_EQUAL(0, file_resize(common.fd, pattern.filesz, pattern.recordsz));
	TEST_ASSERT_EQUAL(0, file_getInfo(common.fd, &info.sectors, &info.filesz, &info.recordsz, &info.recordcnt));
	common_fileInfoCompare(&info, &pattern, "step2");

	writeReadCheck(&info);

	pattern.filesz *= 20;
	pattern.recordsz *= 10;
	TEST_ASSERT_EQUAL(0, file_resize(common.fd, pattern.filesz, pattern.recordsz));
	TEST_ASSERT_EQUAL(0, file_getInfo(common.fd, &info.sectors, &info.filesz, &info.recordsz, &info.recordcnt));
	common_fileInfoCompare(&info, &pattern, "step3");

	writeReadCheck(&info);

	TEST_ASSERT_EQUAL(0, file_close(common.fd));
}


/* Test case of resizing file to size bigger than allowed by sectors num. */
TEST(meterfs_miscellaneous, resize_bigger)
{
	file_info_t pattern = { 2, fsInfo.sectorsz / 2u, fsInfo.sectorsz / 200u, 0 };

	common.fd = common_preallocOpenFile("file0", pattern.sectors, pattern.filesz, pattern.recordsz);
	pattern.filesz = 2u * fsInfo.sectorsz;
	pattern.recordsz *= 2;
	TEST_ASSERT_EQUAL(-EINVAL, file_resize(common.fd, pattern.filesz, pattern.recordsz));

	TEST_ASSERT_EQUAL(0, file_close(common.fd));
}


/* Test case of using lookup multiple times in a row. */
TEST(meterfs_miscellaneous, multi_lookup)
{
	file_info_t info = { 2, fsInfo.sectorsz / 2u, fsInfo.sectorsz / 200u, 0 };
	const char *name = "file0";
	int i;

	TEST_ASSERT_EQUAL(0, file_allocate(name, info.sectors, info.filesz, info.recordsz));

	(void)snprintf(common.buffMsg, sizeof(common.buffMsg), "/%s", name);
	for (i = 0; i < 5; ++i) {
		TEST_ASSERT_GREATER_OR_EQUAL(0, file_lookup(common.buffMsg));
	}

	common.fd = file_open(common.buffMsg);
	TEST_ASSERT_GREATER_OR_EQUAL(0, common.fd);

	TEST_ASSERT_EQUAL(0, file_close(common.fd));
}


TEST_GROUP_RUNNER(meterfs_miscellaneous)
{
	RUN_TEST_CASE(meterfs_miscellaneous, resize_getinfo);
	RUN_TEST_CASE(meterfs_miscellaneous, resize_bigger);
	RUN_TEST_CASE(meterfs_miscellaneous, multi_lookup);
}


void runner(void)
{
	RUN_TEST_GROUP(meterfs_miscellaneous);
}


int main(int argc, char *argv[])
{
	if (argc != 2) {
		(void)printf("Usage: %s /meterfs/mount/path\n", argv[0]);
		return 1;
	}
	if (file_init(argv[1]) != 0) {
		(void)printf("Failed to initialize test\n");
		return 1;
	}
	if (file_eraseAll() != 0) {
		(void)printf("Failed to format meterfs partition\n");
		return 1;
	}

	UnityMain(argc, (const char **)argv, runner);

	return 0;
}
