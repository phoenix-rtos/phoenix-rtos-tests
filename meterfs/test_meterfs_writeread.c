/*
 * Phoenix-RTOS
 *
 * Meterfs writing and reading tests group
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
	char buffRec[21];
	char buffMsg[13];
	char pattern[6];
	char buffBigTX[1064];
	char buffBigRX[1064];
} common;

static void cleanBuffs(void)
{
	memset(common.pattern, 0, sizeof(common.pattern));
	memset(common.buffMsg, 0, sizeof(common.buffMsg));
	memset(common.buffRec, 0, sizeof(common.buffRec));
}


static void turnCheck(int fd, file_info_t *info, char *bufftx, char *buffrx, unsigned int iterNum)
{
	int i;

	if (iterNum >= 10000 || info->recordsz < 5)
		return;

	for (i = 0; i < iterNum; ++i) {
		snprintf(bufftx, info->recordsz, "%04d", i);
		TEST_ASSERT_EQUAL(info->recordsz, file_write(fd, bufftx, info->recordsz));

		TEST_ASSERT_EQUAL(info->recordsz, file_read(fd, 0, buffrx, info->recordsz));

		TEST_ASSERT_EQUAL_HEX8_ARRAY(bufftx, buffrx, info->recordsz);
	}
}


TEST_GROUP(meterfs_writeread);


TEST_SETUP(meterfs_writeread)
{
	common.fd = 0;
	cleanBuffs();
}


TEST_TEAR_DOWN(meterfs_writeread)
{
	TEST_ASSERT_EQUAL(0, file_eraseAll());
}


/* Test case of writing too small records. */
TEST(meterfs_writeread, small_records)
{
	int i, wroteLen;
	file_info_t info = { 2, 5 * 255, 5, 0 };

	common.fd = common_preallocOpenFile("file0", info.sectors, info.filesz, info.recordsz);

	for (i = 0; i < 255; ++i) {
		snprintf(common.buffMsg, sizeof(common.buffMsg), "iter=%d", i);
		wroteLen = i % (info.recordsz + 1);
		i % 2 ? snprintf(common.pattern, sizeof(common.pattern), "aaaaa") : snprintf(common.pattern, sizeof(common.pattern), "zzzzz");

		if (wroteLen) {
			TEST_ASSERT_EQUAL_MESSAGE(info.recordsz, file_write(common.fd, common.pattern, wroteLen), common.buffMsg);
			common_readContent(common.fd, info.recordcnt * info.recordsz, common.buffRec, info.recordsz, common.pattern, wroteLen, common.buffMsg);
			info.recordcnt++;
		}
		else {
			/* Resolving case of writing zero length record. */
			TEST_ASSERT_EQUAL_MESSAGE(-EINVAL, file_write(common.fd, common.pattern, wroteLen), common.buffMsg);
		}

		cleanBuffs();
	}

	TEST_ASSERT_EQUAL(0, file_close(common.fd));
}


/* Test case of writing more records than fit in file. */
TEST(meterfs_writeread, file_overflow)
{
	int i;
	file_info_t info = { 2, 10, 5, 0 };

	common.fd = common_preallocOpenFile("file0", info.sectors, info.filesz, info.recordsz);

	for (i = 0; i < 255; ++i) {
		snprintf(common.buffMsg, sizeof(common.buffMsg), "iter=%d", i);
		i % 2 ? snprintf(common.pattern, sizeof(common.pattern), "aaaaa") : snprintf(common.pattern, sizeof(common.pattern), "zzzzz");
	
		TEST_ASSERT_EQUAL_MESSAGE(info.recordsz, file_write(common.fd, common.pattern, info.recordsz), common.buffMsg);

		if (i < (info.filesz / info.recordsz)) {
			snprintf(common.pattern, sizeof(common.pattern), "zzzzz");
			common_readContent(common.fd, 0, common.buffRec, info.recordsz, common.pattern, info.recordsz, common.buffMsg);
		}
		else {
			i % 2 ? snprintf(common.pattern, sizeof(common.pattern), "zzzzz") : snprintf(common.pattern, sizeof(common.pattern), "aaaaa");
			common_readContent(common.fd, 0, common.buffRec, info.recordsz, common.pattern, info.recordsz, common.buffMsg);
		}

		cleanBuffs();
	}

	TEST_ASSERT_EQUAL(0, file_close(common.fd));
}


/* Test case of writing too big records. */
TEST(meterfs_writeread, big_records)
{
	int i, wroteLen;
	file_info_t info = { 2, 2 * 255, 2, 0 };

	common.fd = common_preallocOpenFile("file0", info.sectors, info.filesz, info.recordsz);

	for (i = 0; i < 255; ++i) {
		snprintf(common.buffMsg, sizeof(common.buffMsg), "iter=%d", i);
		wroteLen = i % 6;
		i % 2 ? snprintf(common.pattern, sizeof(common.pattern), "aaaaa") : snprintf(common.pattern, sizeof(common.pattern), "zzzzz");

		if (wroteLen > info.recordsz) {
			TEST_ASSERT_EQUAL_MESSAGE(info.recordsz, file_write(common.fd, common.pattern, wroteLen), common.buffMsg);
			common_readContent(common.fd, info.recordcnt * info.recordsz, common.buffRec, info.recordsz, common.pattern, info.recordsz, common.buffMsg);
			info.recordcnt++;
		}
		else if (wroteLen == 0) {
			/* Resolving case of writing zero length record. */
			TEST_ASSERT_EQUAL_MESSAGE(-EINVAL, file_write(common.fd, common.pattern, wroteLen), common.buffMsg);
		}
		else {
			TEST_ASSERT_EQUAL_MESSAGE(info.recordsz, file_write(common.fd, common.pattern, wroteLen), common.buffMsg);
			common_readContent(common.fd, info.recordcnt * info.recordsz, common.buffRec, info.recordsz, common.pattern, wroteLen, common.buffMsg);
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
	
	snprintf(common.pattern, sizeof(common.pattern), "a0000");
	common.fd = common_preallocOpenFile("file0", info.sectors, info.filesz, info.recordsz);

	for (i = 0; i < 255; ++i) {
		snprintf(common.buffMsg, sizeof(common.buffMsg), "iter=%d", i);
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
	file_info_t info = { 13, 36000, 12, 0 };

	common.fd = common_preallocOpenFile("file0", info.sectors, info.filesz, info.recordsz);

	TEST_ASSERT_EQUAL(0, file_getInfo(common.fd, &info.sectors, &info.filesz, &info.recordsz, &info.recordcnt));
	TEST_ASSERT_EQUAL(0, info.recordcnt);

	for (i = 0; i < 4000; ++i) {
		snprintf(common.buffMsg,  sizeof(common.buffMsg), "a0000000%04d", i);
		TEST_ASSERT_EQUAL_MESSAGE(info.recordsz, file_write(common.fd, common.buffMsg, info.recordsz), common.buffMsg);

		if (i >= 3000) {
			snprintf(common.buffMsg, sizeof(common.buffMsg), "a0000000%04d", i - 3000 + 1);
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
	file_info_t info = { 281, 1064, 1064, 0 };

	common.fd = common_preallocOpenFile("file0", info.sectors, info.filesz, info.recordsz);

	turnCheck(common.fd, &info, common.buffBigTX, common.buffBigRX, 1440);

	TEST_ASSERT_EQUAL(0, file_close(common.fd));
}


/* Test case of fulfilling all sectors and turning small file to the beginning. */
TEST(meterfs_writeread, file_turn_small)
{
	file_info_t info = { 2, 400, 400 };

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
	file_init(argv[1]);
	TEST_ASSERT_EQUAL(0, file_eraseAll());

	UnityMain(argc, (const char**)argv, runner);

	return 0;
}
