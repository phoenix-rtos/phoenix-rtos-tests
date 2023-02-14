/*
 * Phoenix-RTOS
 *
 * Meterfs writing and reading tests group
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
	char buffRec[21];
	char buffMsg[13];
	char pattern[6];
	char buffBigTX[1064];
	char buffBigRX[1064];
} common;


static file_fsInfo_t fsInfo;


static void cleanBuffs(void)
{
	(void)memset(common.pattern, 0, sizeof(common.pattern));
	(void)memset(common.buffMsg, 0, sizeof(common.buffMsg));
	(void)memset(common.buffRec, 0, sizeof(common.buffRec));
}


static void turnCheck(int fd, file_info_t *info, char *bufftx, char *buffrx, unsigned int iterNum)
{
	unsigned int i;

	if ((iterNum >= 10000u) || (info->recordsz < 5u)) {
		return;
	}

	for (i = 0; i < iterNum; ++i) {
		(void)snprintf(bufftx, info->recordsz, "%04u", i);
		TEST_ASSERT_EQUAL(info->recordsz, file_write(fd, bufftx, info->recordsz));

		TEST_ASSERT_EQUAL(info->recordsz, file_read(fd, 0, buffrx, info->recordsz));

		TEST_ASSERT_EQUAL_HEX8_ARRAY(bufftx, buffrx, info->recordsz);

		if (info->recordsz > 2u) {
			(void)memset(buffrx + 1, 'x', info->recordsz - 2u);
			TEST_ASSERT_EQUAL(info->recordsz - 2u, file_read(fd, 1, buffrx + 1, info->recordsz - 2u));
			TEST_ASSERT_EQUAL_HEX8_ARRAY(bufftx, buffrx, info->recordsz);
		}
	}
}


TEST_GROUP(meterfs_writeread);


TEST_SETUP(meterfs_writeread)
{
	common.fd = 0;
	cleanBuffs();
	TEST_ASSERT_EQUAL(0, file_devInfo(&fsInfo));
}


TEST_TEAR_DOWN(meterfs_writeread)
{
	TEST_ASSERT_EQUAL(0, file_eraseAll());
}


/* Test case of writing too small records. */
TEST(meterfs_writeread, small_records)
{
	size_t i, writeLen;
	file_info_t info = { ((5u * 255u) / fsInfo.sectorsz) + 2u, 5 * 255, 5, 0 };

	common.fd = common_preallocOpenFile("file0", info.sectors, info.filesz, info.recordsz);

	for (i = 0; i < 255u; ++i) {
		(void)snprintf(common.buffMsg, sizeof(common.buffMsg), "iter=%zu", i);
		writeLen = i % (info.recordsz + 1u);
		(i % 2u) ? (void)snprintf(common.pattern, sizeof(common.pattern), "aaaaa") : (void)snprintf(common.pattern, sizeof(common.pattern), "zzzzz");

		if (writeLen != 0u) {
			TEST_ASSERT_EQUAL_MESSAGE(info.recordsz, file_write(common.fd, common.pattern, writeLen), common.buffMsg);
			common_readContent(common.fd, info.recordcnt * info.recordsz, common.buffRec, info.recordsz, common.pattern, writeLen, common.buffMsg);
			info.recordcnt++;
		}
		else {
			/* Resolving case of writing zero length record. */
			TEST_ASSERT_EQUAL_MESSAGE(-EINVAL, file_write(common.fd, common.pattern, writeLen), common.buffMsg);
		}

		cleanBuffs();
	}

	TEST_ASSERT_EQUAL(0, file_close(common.fd));
}


/* Test case of writing more records than fit in file. */
TEST(meterfs_writeread, file_overflow)
{
	size_t i;
	file_info_t info = { 2, 10, 5, 0 };

	common.fd = common_preallocOpenFile("file0", info.sectors, info.filesz, info.recordsz);

	for (i = 0; i < 255u; ++i) {
		(void)snprintf(common.buffMsg, sizeof(common.buffMsg), "iter=%zu", i);
		(i % 2u) ? (void)snprintf(common.pattern, sizeof(common.pattern), "aaaaa") : (void)snprintf(common.pattern, sizeof(common.pattern), "zzzzz");

		TEST_ASSERT_EQUAL_MESSAGE(info.recordsz, file_write(common.fd, common.pattern, info.recordsz), common.buffMsg);

		if (i < (info.filesz / info.recordsz)) {
			(void)snprintf(common.pattern, sizeof(common.pattern), "zzzzz");
			common_readContent(common.fd, 0, common.buffRec, info.recordsz, common.pattern, info.recordsz, common.buffMsg);
		}
		else {
			(i % 2u) ? (void)snprintf(common.pattern, sizeof(common.pattern), "zzzzz") : (void)snprintf(common.pattern, sizeof(common.pattern), "aaaaa");
			common_readContent(common.fd, 0, common.buffRec, info.recordsz, common.pattern, info.recordsz, common.buffMsg);
		}

		cleanBuffs();
	}

	TEST_ASSERT_EQUAL(0, file_close(common.fd));
}


/* Test case of writing too big records. */
TEST(meterfs_writeread, big_records)
{
	size_t i, writeLen;
	file_info_t info = { ((5u * 255u) / fsInfo.sectorsz) + 2u, 2 * 255, 2, 0 };

	common.fd = common_preallocOpenFile("file0", info.sectors, info.filesz, info.recordsz);

	for (i = 0; i < 255u; ++i) {
		(void)snprintf(common.buffMsg, sizeof(common.buffMsg), "iter=%zu", i);
		writeLen = i % 6u;
		(i % 2u) ? (void)snprintf(common.pattern, sizeof(common.pattern), "aaaaa") : (void)snprintf(common.pattern, sizeof(common.pattern), "zzzzz");

		if (writeLen > info.recordsz) {
			TEST_ASSERT_EQUAL_MESSAGE(info.recordsz, file_write(common.fd, common.pattern, writeLen), common.buffMsg);
			common_readContent(common.fd, info.recordcnt * info.recordsz, common.buffRec, info.recordsz, common.pattern, info.recordsz, common.buffMsg);
			info.recordcnt++;
		}
		else if (writeLen == 0u) {
			/* Resolving case of writing zero length record. */
			TEST_ASSERT_EQUAL_MESSAGE(-EINVAL, file_write(common.fd, common.pattern, writeLen), common.buffMsg);
		}
		else {
			TEST_ASSERT_EQUAL_MESSAGE(info.recordsz, file_write(common.fd, common.pattern, writeLen), common.buffMsg);
			common_readContent(common.fd, info.recordcnt * info.recordsz, common.buffRec, info.recordsz, common.pattern, writeLen, common.buffMsg);
			info.recordcnt++;
		}

		cleanBuffs();
	}

	TEST_ASSERT_EQUAL(0, file_close(common.fd));
}


/* Test case of reading from out of file. */
TEST(meterfs_writeread, file_end)
{
	int i;
	file_info_t info = { 2, 10, 5, 0 };

	(void)snprintf(common.pattern, sizeof(common.pattern), "a0000");
	common.fd = common_preallocOpenFile("file0", info.sectors, info.filesz, info.recordsz);

	for (i = 0; i < 255; ++i) {
		(void)snprintf(common.buffMsg, sizeof(common.buffMsg), "iter=%d", i);
		TEST_ASSERT_EQUAL(info.recordsz, file_write(common.fd, common.pattern, info.recordsz));
		TEST_ASSERT_EQUAL(0, file_read(common.fd, info.filesz, common.buffRec, info.recordsz));

		cleanBuffs();
	}

	TEST_ASSERT_EQUAL(0, file_close(common.fd));
}


/* Test case of writing and reading a lot of records. */
TEST(meterfs_writeread, many_records)
{
	int i;
	const size_t headerSectorcnt = 6u;
	file_info_t info = { (fsInfo.sz / fsInfo.sectorsz) - headerSectorcnt, 36000, 12, 0 };

	common.fd = common_preallocOpenFile("file0", info.sectors, info.filesz, info.recordsz);

	TEST_ASSERT_EQUAL(0, file_getInfo(common.fd, &info.sectors, &info.filesz, &info.recordsz, &info.recordcnt));
	TEST_ASSERT_EQUAL(0, info.recordcnt);

	for (i = 0; i < 4000; ++i) {
		(void)snprintf(common.buffMsg, sizeof(common.buffMsg), "a0000000%04d", i);
		TEST_ASSERT_EQUAL_MESSAGE(info.recordsz, file_write(common.fd, common.buffMsg, info.recordsz), common.buffMsg);

		if (i >= 3000) {
			(void)snprintf(common.buffMsg, sizeof(common.buffMsg), "a0000000%04d", i - 3000 + 1);
			common_readContent(common.fd, 0, common.buffRec, info.recordsz, common.buffMsg, info.recordsz, common.buffMsg);
		}

		cleanBuffs();
	}

	TEST_ASSERT_EQUAL(0, file_getInfo(common.fd, &info.sectors, &info.filesz, &info.recordsz, &info.recordcnt));
	TEST_ASSERT_EQUAL(3000, info.recordcnt);

	TEST_ASSERT_EQUAL(0, file_close(common.fd));
}


/* Test case of fulfilling all sectors and turning big file to the beginning. */
TEST(meterfs_writeread, file_turn_big)
{
	file_info_t info = { (fsInfo.sz / fsInfo.sectorsz) / 2u, fsInfo.sectorsz / 4u, fsInfo.sectorsz / 4u, 0 };

	common.fd = common_preallocOpenFile("file0", info.sectors, info.filesz, info.recordsz);

	turnCheck(common.fd, &info, common.buffBigTX, common.buffBigRX, 1440);

	TEST_ASSERT_EQUAL(0, file_close(common.fd));
}


/* Test case of fulfilling all sectors and turning small file to the beginning. */
TEST(meterfs_writeread, file_turn_small)
{
	file_info_t info = { 2, fsInfo.sectorsz / 10u, fsInfo.sectorsz / 10u };

	common.fd = common_preallocOpenFile("file0", info.sectors, info.filesz, info.recordsz);

	turnCheck(common.fd, &info, common.buffBigTX, common.buffBigRX, 1440);

	TEST_ASSERT_EQUAL(0, file_close(common.fd));
}


TEST_GROUP_RUNNER(meterfs_writeread)
{
	RUN_TEST_CASE(meterfs_writeread, small_records);
	RUN_TEST_CASE(meterfs_writeread, file_overflow);
	RUN_TEST_CASE(meterfs_writeread, big_records);
	RUN_TEST_CASE(meterfs_writeread, file_end);
	RUN_TEST_CASE(meterfs_writeread, many_records);
	RUN_TEST_CASE(meterfs_writeread, file_turn_big);
	RUN_TEST_CASE(meterfs_writeread, file_turn_small);
}


void runner(void)
{
	RUN_TEST_GROUP(meterfs_writeread);
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
