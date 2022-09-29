/*
 * Phoenix-RTOS
 *
 * phoenix-rtos-tests
 *
 * Libcache tests
 *
 * Copyright 2022 Phoenix Systems
 * Author: Malgorzata Wrobel
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "libcache_utils.h"

#include "unity_fixture.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>


#define LIBCACHE_ADDR_DUMMY  LIBCACHE_SRC_MEM_SIZE / 2 /* Filler address to test error checks */
#define LIBCACHE_ADDR_OFF_57 0x23b9ULL                 /* Offset = 57 to check multi-line write */
#define LIBCACHE_ADDR_OFF_27 0x239bULL
#define LIBCACHE_ADDR_INT    0x23f8ULL /* Address with offset divisible by sizeof(int) for proper integer alignment in cache line */


TEST_GROUP(test_init);

TEST_SETUP(test_init)
{
	ops.readCb = test_readCb;
	ops.writeCb = test_writeCb;
}

TEST_TEAR_DOWN(test_init)
{
}

TEST(test_init, cache_init_srcMemSizeZero)
{
	size_t srcMemSize = 0, lineSize = 64, linesCnt = 32;
	cachectx_t *cache = NULL;

	cache = cache_init(srcMemSize, lineSize, linesCnt, &ops);
	TEST_ASSERT_NULL(cache);
}

TEST(test_init, cache_init_lineSizeZero)
{
	size_t lineSize = 0, linesCnt = 32;
	cachectx_t *cache = NULL;

	cache = cache_init(LIBCACHE_SRC_MEM_SIZE, lineSize, linesCnt, &ops);
	TEST_ASSERT_NULL(cache);
}

TEST(test_init, cache_init_linesCntZero)
{
	size_t lineSize = 64, linesCnt = 0;
	cachectx_t *cache = NULL;

	cache = cache_init(LIBCACHE_SRC_MEM_SIZE, lineSize, linesCnt, &ops);
	TEST_ASSERT_NULL(cache);
}

TEST(test_init, cache_init_linesCntNotDivisibleByNumWays)
{
	size_t lineSize = 64, linesCnt = 19;
	cachectx_t *cache = NULL;

	cache = cache_init(LIBCACHE_SRC_MEM_SIZE, lineSize, linesCnt, &ops);
	TEST_ASSERT_NULL(cache);
}

TEST(test_init, cache_init_sizesNotZero)
{
	int ret = -1;
	cachectx_t *cache = NULL;

	cache = cache_init(LIBCACHE_SRC_MEM_SIZE, LIBCACHE_LINE_SIZE, LIBCACHE_LINES_CNT, &ops);
	TEST_ASSERT_NOT_NULL(cache);

	ret = cache_deinit(cache);
	TEST_ASSERT_EQUAL_INT(EOK, ret);
}

TEST_GROUP_RUNNER(test_init)
{
	RUN_TEST_CASE(test_init, cache_init_srcMemSizeZero);
	RUN_TEST_CASE(test_init, cache_init_lineSizeZero);
	RUN_TEST_CASE(test_init, cache_init_linesCntZero);
	RUN_TEST_CASE(test_init, cache_init_linesCntNotDivisibleByNumWays);
	RUN_TEST_CASE(test_init, cache_init_sizesNotZero);
}

TEST_GROUP(test_deinit);

TEST_SETUP(test_deinit)
{
	ops.readCb = test_readCb;
	ops.writeCb = test_writeCb;
}

TEST_TEAR_DOWN(test_deinit)
{
}

TEST(test_deinit, cache_deinit_initalizedCache)
{
	int ret;
	cachectx_t *cache;

	cache = cache_init(LIBCACHE_SRC_MEM_SIZE, LIBCACHE_LINE_SIZE, LIBCACHE_LINES_CNT, &ops);
	TEST_ASSERT_NOT_NULL(cache);

	ret = cache_deinit(cache);
	TEST_ASSERT_EQUAL_INT(EOK, ret);
}

TEST_GROUP_RUNNER(test_deinit)
{
	RUN_TEST_CASE(test_deinit, cache_deinit_initalizedCache);
}

TEST_GROUP(test_read_write);

TEST_SETUP(test_read_write)
{
	ops.readCb = test_readCb;
	ops.writeCb = test_writeCb;
	offBitsNum = LOG2(LIBCACHE_LINE_SIZE);
	offMask = ((uint64_t)1 << offBitsNum) - 1;
}

TEST_TEAR_DOWN(test_read_write)
{
}

TEST(test_read_write, cache_write_nullBuff)
{
	int ret;
	ssize_t writeCount = 0;
	cachectx_t *cache = NULL;
	size_t count = 10 * sizeof(char);
	uint64_t addr = LIBCACHE_ADDR_DUMMY;
	char *buffer = NULL;

	cache = cache_init(LIBCACHE_SRC_MEM_SIZE, LIBCACHE_LINE_SIZE, LIBCACHE_LINES_CNT, &ops);
	TEST_ASSERT_NOT_NULL(cache);

	writeCount = cache_write(cache, addr, buffer, count, LIBCACHE_WRITE_BACK);
	TEST_ASSERT_EQUAL_INT(-EINVAL, writeCount);

	ret = cache_deinit(cache);
	TEST_ASSERT_EQUAL_INT(EOK, ret);
}

TEST(test_read_write, cache_write_wrongPolicy)
{
	int ret;
	ssize_t writeCount = 0;
	cachectx_t *cache = NULL;
	size_t count = 10 * sizeof(char);
	uint64_t addr = LIBCACHE_ADDR_DUMMY;
	char *buffer = NULL;
	int policy = -2;

	buffer = "^#$^#$^%%$";
	cache = cache_init(LIBCACHE_SRC_MEM_SIZE, LIBCACHE_LINE_SIZE, LIBCACHE_LINES_CNT, &ops);
	TEST_ASSERT_NOT_NULL(cache);

	writeCount = cache_write(cache, addr, buffer, count, policy);
	TEST_ASSERT_EQUAL_INT(-EINVAL, writeCount);

	ret = cache_deinit(cache);
	TEST_ASSERT_EQUAL_INT(EOK, ret);
}

TEST(test_read_write, cache_writeNothing)
{
	int ret = -1;
	ssize_t writeCount = 0;
	cachectx_t *cache = NULL;
	size_t count = 0;
	uint64_t addr = LIBCACHE_ADDR_DUMMY;
	char *buffer = NULL;

	buffer = "^$^$%^^%^$%";
	cache = cache_init(LIBCACHE_SRC_MEM_SIZE, LIBCACHE_LINE_SIZE, LIBCACHE_LINES_CNT, &ops);
	TEST_ASSERT_NOT_NULL(cache);

	writeCount = cache_write(cache, addr, buffer, count, LIBCACHE_WRITE_THROUGH);
	TEST_ASSERT_EQUAL_INT(0, writeCount);

	ret = cache_deinit(cache);
	TEST_ASSERT_EQUAL_INT(EOK, ret);
}

TEST(test_read_write, cache_write_addrOutOfScope)
{
	int ret = -1;
	ssize_t writeCount;
	cachectx_t *cache;
	size_t count = 16 * sizeof(char);
	char *buffer;

	cache = cache_init(LIBCACHE_SRC_MEM_SIZE, LIBCACHE_LINE_SIZE, LIBCACHE_LINES_CNT, &ops);
	TEST_ASSERT_NOT_NULL(cache);

	buffer = "FSDGSGDGSDGDSGDF";

	writeCount = cache_write(cache, LIBCACHE_SRC_MEM_SIZE + 10, buffer, count, LIBCACHE_WRITE_BACK);
	TEST_ASSERT_EQUAL_INT(-EINVAL, writeCount);

	ret = cache_deinit(cache);
	TEST_ASSERT_EQUAL_INT(EOK, ret);
}

TEST(test_read_write, cache_write_addrPartiallyInScope)
{
	int ret = -1;
	ssize_t writeCount;
	cachectx_t *cache;
	size_t count = 16 * sizeof(char);
	char *buffer;

	cache = cache_init(LIBCACHE_SRC_MEM_SIZE, LIBCACHE_LINE_SIZE, LIBCACHE_LINES_CNT, &ops);
	TEST_ASSERT_NOT_NULL(cache);

	buffer = "FSDGSGDGSDGDSGDF";

	writeCount = cache_write(cache, LIBCACHE_SRC_MEM_SIZE - 10, buffer, count, LIBCACHE_WRITE_BACK);
	TEST_ASSERT_EQUAL_INT(10 * sizeof(char), writeCount);

	ret = cache_deinit(cache);
	TEST_ASSERT_EQUAL_INT(EOK, ret);
}

TEST(test_read_write, cache_writeData)
{
	int ret = -1;
	ssize_t writeCount = 0;
	cachectx_t *cache = NULL;
	size_t count = 10 * sizeof(char);
	uint64_t addr = LIBCACHE_ADDR_OFF_57;
	char *buffer = NULL;

	buffer = "^#$^#$^%%$";

	cache = cache_init(LIBCACHE_SRC_MEM_SIZE, LIBCACHE_LINE_SIZE, LIBCACHE_LINES_CNT, &ops);
	TEST_ASSERT_NOT_NULL(cache);

	writeCount = cache_write(cache, addr, buffer, count, LIBCACHE_WRITE_THROUGH);
	TEST_ASSERT_EQUAL_INT(count, writeCount);

	ret = cache_deinit(cache);
	TEST_ASSERT_EQUAL_INT(EOK, ret);
}

TEST(test_read_write, cache_readNullBuff)
{
	int ret = -1;
	ssize_t readCount = 0;
	cachectx_t *cache = NULL;
	size_t count = 1 * sizeof(char);
	uint64_t addr = LIBCACHE_ADDR_DUMMY;
	void *buffer = NULL;

	cache = cache_init(LIBCACHE_SRC_MEM_SIZE, LIBCACHE_LINE_SIZE, LIBCACHE_LINES_CNT, &ops);
	TEST_ASSERT_NOT_NULL(cache);

	readCount = cache_read(cache, addr, buffer, count);
	TEST_ASSERT_EQUAL_INT(-EINVAL, readCount);

	ret = cache_deinit(cache);
	TEST_ASSERT_EQUAL_INT(EOK, ret);
}

TEST(test_read_write, cache_readNothing)
{
	int ret = -1;
	ssize_t writeCount = 0, readCount = 0;
	cachectx_t *cache = NULL;
	size_t countWrite = 10 * sizeof(char), countRead = 0 * sizeof(char);
	uint64_t addr = LIBCACHE_ADDR_DUMMY;
	char *bufferW = NULL, *bufferR = NULL;

	bufferW = "^#$^#$^%%$";
	bufferR = malloc(countWrite);
	TEST_ASSERT_NOT_NULL(bufferR);

	cache = cache_init(LIBCACHE_SRC_MEM_SIZE, LIBCACHE_LINE_SIZE, LIBCACHE_LINES_CNT, &ops);
	TEST_ASSERT_NOT_NULL(cache);

	writeCount = cache_write(cache, addr, bufferW, countWrite, LIBCACHE_WRITE_THROUGH);
	TEST_ASSERT_EQUAL_INT(countWrite, writeCount);

	readCount = cache_read(cache, addr, bufferR, countRead);
	TEST_ASSERT_EQUAL_INT(0, readCount);
	TEST_ASSERT_EMPTY(bufferR);

	free(bufferR);

	ret = cache_deinit(cache);
	TEST_ASSERT_EQUAL_INT(EOK, ret);
}

TEST(test_read_write, cache_read_addrOutOfScope)
{
	int ret = -1;
	ssize_t readCount;
	cachectx_t *cache;
	size_t count = 16 * sizeof(char);
	char *buffer;

	cache = cache_init(LIBCACHE_SRC_MEM_SIZE, LIBCACHE_LINE_SIZE, LIBCACHE_LINES_CNT, &ops);
	TEST_ASSERT_NOT_NULL(cache);

	buffer = malloc(count);
	TEST_ASSERT_NOT_NULL(buffer);
	readCount = cache_read(cache, LIBCACHE_SRC_MEM_SIZE + 10, buffer, count);
	TEST_ASSERT_EQUAL_INT(-EINVAL, readCount);

	ret = cache_deinit(cache);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	free(buffer);
}

TEST(test_read_write, cache_read_addrPartiallyInScope)
{
	int ret = -1;
	ssize_t writeCount, readCount;
	cachectx_t *cache;
	size_t count = 16 * sizeof(char);
	char *bufferW, *bufferR;

	cache = cache_init(LIBCACHE_SRC_MEM_SIZE, LIBCACHE_LINE_SIZE, LIBCACHE_LINES_CNT, &ops);
	TEST_ASSERT_NOT_NULL(cache);

	bufferW = "FSDGSGDGSDGDSGDF";

	writeCount = cache_write(cache, LIBCACHE_SRC_MEM_SIZE - 10, bufferW, count, LIBCACHE_WRITE_BACK);
	TEST_ASSERT_EQUAL_INT(10 * sizeof(char), writeCount);

	bufferR = malloc(count);
	TEST_ASSERT_NOT_NULL(bufferR);
	readCount = cache_read(cache, LIBCACHE_SRC_MEM_SIZE - 5, bufferR, count);
	TEST_ASSERT_EQUAL_INT(5 * sizeof(char), readCount);

	ret = cache_deinit(cache);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	free(bufferR);
}

TEST(test_read_write, cache_readData)
{
	int ret = -1;
	ssize_t writeCount = 0, readCount = 0;
	cachectx_t *cache = NULL;
	size_t count = 10 * sizeof(char);
	uint64_t addr = LIBCACHE_ADDR_OFF_57;
	char *bufferW = NULL, *bufferR = NULL;

	bufferW = "^#$^#$^%%$";
	bufferR = malloc(count);
	TEST_ASSERT_NOT_NULL(bufferR);

	cache = cache_init(LIBCACHE_SRC_MEM_SIZE, LIBCACHE_LINE_SIZE, LIBCACHE_LINES_CNT, &ops);
	TEST_ASSERT_NOT_NULL(cache);

	writeCount = cache_write(cache, addr, bufferW, count, LIBCACHE_WRITE_THROUGH);
	TEST_ASSERT_EQUAL_INT(count, writeCount);

	readCount = cache_read(cache, addr, bufferR, count);
	TEST_ASSERT_EQUAL_INT(count, readCount);

	TEST_ASSERT_EQUAL_MEMORY(bufferW, bufferR, count);

	free(bufferR);

	ret = cache_deinit(cache);
	TEST_ASSERT_EQUAL_INT(EOK, ret);
}

TEST(test_read_write, cache_writeThrough)
{
	int ret = -1;
	ssize_t writeCount = -1, readCount = -1;
	cachectx_t *cache = NULL;
	uint64_t addr = LIBCACHE_ADDR_OFF_57, offset = addr & offMask;
	size_t count = 127 * sizeof(char);
	size_t remainder = 0, flushed = 0;
	char *buffer, *actual, *expected;

	buffer = "^#$%^$#%^&$#&^*$(^*&^)_)_(++(_)_(*)(&^%^*%^$#%$@#$@!# @!$#$#%$ $#%##$^$#%^#$$!@!*!!~~~!@#@$$_#@_+$ 4#$%#$%#%#$%^^#$^$#^#$^%@#$$";

	remainder = (count - (LIBCACHE_LINE_SIZE - offset)) % LIBCACHE_LINE_SIZE;
	flushed = (offset + count + (LIBCACHE_LINE_SIZE - remainder)) * sizeof(char);
	actual = malloc(flushed);
	TEST_ASSERT_NOT_NULL(actual);
	expected = malloc(flushed);
	TEST_ASSERT_NOT_NULL(expected);

	cache = cache_init(LIBCACHE_SRC_MEM_SIZE, LIBCACHE_LINE_SIZE, LIBCACHE_LINES_CNT, &ops);
	TEST_ASSERT_NOT_NULL(cache);

	writeCount = cache_write(cache, addr, buffer, count, LIBCACHE_WRITE_THROUGH);
	TEST_ASSERT_EQUAL_INT(count, writeCount);

	readCount = test_readCb(addr - offset, expected, flushed, NULL);
	TEST_ASSERT_EQUAL_INT(flushed, readCount);
	memcpy(expected + offset, buffer, count);

	readCount = test_readCb(addr - offset, actual, flushed, NULL);
	TEST_ASSERT_EQUAL_INT(flushed, readCount);

	TEST_ASSERT_EQUAL_MEMORY(expected, actual, flushed);

	free(expected);
	free(actual);

	ret = cache_deinit(cache);
	TEST_ASSERT_EQUAL_INT(EOK, ret);
}

TEST(test_read_write, cache_writeBack)
{
	int ret = -1;
	ssize_t writeCount = -1, readCount = -1;
	cachectx_t *cache = NULL;
	uint64_t addr = LIBCACHE_ADDR_OFF_57, offset = addr & offMask;
	size_t count = 127 * sizeof(char);
	size_t remainder = 0, flushed = 0;
	char *buffer, *actual, *expected;

	buffer = "^#$%^$#%^&$#&^*$(^*&^)_)_(++(_)_(*)(&^%^*%^$#%$@#$@!# @!$#$#%$ $#%##$^$#%^#$$!@!*!!~~~!@#@$$_#@_+$ 4#$%#$%#%#$%^^#$^$#^#$^%@#$$";

	remainder = (count - (LIBCACHE_LINE_SIZE - offset)) % LIBCACHE_LINE_SIZE;
	flushed = (offset + count + (LIBCACHE_LINE_SIZE - remainder)) * sizeof(char);
	actual = malloc(flushed);
	TEST_ASSERT_NOT_NULL(actual);
	expected = malloc(flushed);
	TEST_ASSERT_NOT_NULL(expected);

	cache = cache_init(LIBCACHE_SRC_MEM_SIZE, LIBCACHE_LINE_SIZE, LIBCACHE_LINES_CNT, &ops);
	TEST_ASSERT_NOT_NULL(cache);

	writeCount = cache_write(cache, addr, buffer, count, LIBCACHE_WRITE_BACK);
	TEST_ASSERT_EQUAL_INT(count, writeCount);

	ret = cache_deinit(cache);
	TEST_ASSERT_EQUAL_INT(EOK, ret);

	readCount = test_readCb(addr - offset, expected, flushed, NULL);
	TEST_ASSERT_EQUAL_INT(flushed, readCount);
	memcpy(expected + offset, buffer, count);

	readCount = test_readCb(addr - offset, actual, flushed, NULL);
	TEST_ASSERT_EQUAL_INT(flushed, readCount);

	TEST_ASSERT_EQUAL_MEMORY(expected, actual, flushed);

	free(actual);
	free(expected);
}

TEST_GROUP_RUNNER(test_read_write)
{
	RUN_TEST_CASE(test_read_write, cache_write_nullBuff);
	RUN_TEST_CASE(test_read_write, cache_write_wrongPolicy);
	RUN_TEST_CASE(test_read_write, cache_writeNothing);
	RUN_TEST_CASE(test_read_write, cache_write_addrOutOfScope);
	RUN_TEST_CASE(test_read_write, cache_write_addrPartiallyInScope);
	RUN_TEST_CASE(test_read_write, cache_writeData);

	RUN_TEST_CASE(test_read_write, cache_readNullBuff);
	RUN_TEST_CASE(test_read_write, cache_readNothing);
	RUN_TEST_CASE(test_read_write, cache_read_addrOutOfScope);
	RUN_TEST_CASE(test_read_write, cache_read_addrPartiallyInScope);
	RUN_TEST_CASE(test_read_write, cache_readData);

	RUN_TEST_CASE(test_read_write, cache_writeThrough);
	RUN_TEST_CASE(test_read_write, cache_writeBack);
}

TEST_GROUP(test_threads);

TEST_SETUP(test_threads)
{
	ops.readCb = test_readCb;
	ops.writeCb = test_writeCb;
}

TEST_TEAR_DOWN(test_threads)
{
}

TEST(test_threads, thread_write)
{
	int ret;
	cachectx_t *cache = NULL;
	ssize_t readCount = -1;
	pthread_t thread1, thread2, thread3, thread4;
	test_write_args_t args1, args2, args3, args4;
	char *actual1, *actual2, *actual3, *actual4;
	uint64_t addr1 = 0x22dbULL, addr2 = 0x197bULL, addr3 = 0x21cbULL, addr4 = 0x12dbULL;
	char *expected1, *expected2, *expected3, *expected4;
	size_t count1 = 127 * sizeof(char), count2 = 144 * sizeof(char), count3 = 154 * sizeof(char), count4 = 138 * sizeof(char);

	expected1 = "^#$%^$#%^&$#&^*$(^*&^)_)_(++(_)_(*)(&^%^*%^$#%$@#$@!# @!$#$#%$ $#%##$^$#%^#$$!@!*!!~~~!@#@$$_#@_+$ 4#$%#$%#%#$%^^#$^$#^#$^%@#$$";
	expected2 = "DUMMYTEXTDUMMYTEXTDUMMYTEXTDUMMYTEXTDUMMYTEXTDUMMYTEXTDUMMYTEXTDUMMYTEXTDUMMYTEXTDUMMYTEXTDUMMYTEXTDUMMYTEXTDUMMYTEXTDUMMYTEXTDUMMYTEXTDUMMYTEXT";
	expected3 = "PHOENIXRTOSPHOENIXRTOSPHOENIXRTOSPHOENIXRTOSPHOENIXRTOSPHOENIXRTOSPHOENIXRTOSPHOENIXRTOSPHOENIXRTOSPHOENIXRTOSPHOENIXRTOSPHOENIXRTOSPHOENIXRTOSPHOENIXRTOS";
	expected4 = "QAZWSXEDCRFVTGBYHNUJMIKOLPPLOKMIJNUHBYGVTFCRDXESZWAQZXCVBNMASDFGHJKLQWERTYUIOPPOIUYTREWHAQLKJHGFDSAMNBVCXZ[;.[';/.]'/,12332445435324535R43";

	cache = cache_init(LIBCACHE_SRC_MEM_SIZE, LIBCACHE_LINE_SIZE, LIBCACHE_LINES_CNT, &ops);
	TEST_ASSERT_NOT_NULL(cache);

	args1 = (test_write_args_t) { .cache = cache, .addr = addr1, .buffer = expected1, .count = count1, .policy = LIBCACHE_WRITE_THROUGH };
	args2 = (test_write_args_t) { .cache = cache, .addr = addr2, .buffer = expected2, .count = count2, .policy = LIBCACHE_WRITE_THROUGH };
	args3 = (test_write_args_t) { .cache = cache, .addr = addr3, .buffer = expected3, .count = count3, .policy = LIBCACHE_WRITE_THROUGH };
	args4 = (test_write_args_t) { .cache = cache, .addr = addr4, .buffer = expected4, .count = count4, .policy = LIBCACHE_WRITE_THROUGH };

	ret = pthread_create(&thread1, NULL, &test_cache_write, (void *)&args1);
	TEST_ASSERT_EQUAL_INT(0, ret);
	ret = pthread_create(&thread2, NULL, &test_cache_write, (void *)&args2);
	TEST_ASSERT_EQUAL_INT(0, ret);
	ret = pthread_create(&thread3, NULL, &test_cache_write, (void *)&args3);
	TEST_ASSERT_EQUAL_INT(0, ret);
	ret = pthread_create(&thread4, NULL, &test_cache_write, (void *)&args4);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = pthread_join(thread1, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(args1.count, args1.actualCount);

	ret = pthread_join(thread2, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(args2.count, args2.actualCount);

	ret = pthread_join(thread3, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(args3.count, args3.actualCount);

	ret = pthread_join(thread4, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(args4.count, args4.actualCount);

	actual1 = malloc(count1);
	TEST_ASSERT_NOT_NULL(actual1);
	readCount = cache_read(cache, args1.addr, actual1, args1.count);
	TEST_ASSERT_EQUAL_INT(args1.count, readCount);
	TEST_ASSERT_EQUAL_MEMORY(expected1, actual1, args1.count);

	actual2 = malloc(count2);
	TEST_ASSERT_NOT_NULL(actual2);
	readCount = cache_read(cache, args2.addr, actual2, args2.count);
	TEST_ASSERT_EQUAL_INT(args2.count, readCount);
	TEST_ASSERT_EQUAL_MEMORY(expected2, actual2, args2.count);

	actual3 = malloc(count3);
	TEST_ASSERT_NOT_NULL(actual3);
	readCount = cache_read(cache, args3.addr, actual3, args3.count);
	TEST_ASSERT_EQUAL_INT(args3.count, readCount);
	TEST_ASSERT_EQUAL_MEMORY(expected3, actual3, args3.count);

	actual4 = malloc(count4);
	TEST_ASSERT_NOT_NULL(actual4);
	readCount = cache_read(cache, args4.addr, actual4, args4.count);
	TEST_ASSERT_EQUAL_INT(args4.count, readCount);
	TEST_ASSERT_EQUAL_MEMORY(expected4, actual4, args4.count);

	readCount = test_readCb(args1.addr, actual1, args1.count, NULL);
	TEST_ASSERT_EQUAL_INT(args1.count, readCount);
	TEST_ASSERT_EQUAL_MEMORY(expected1, actual1, args1.count);

	readCount = test_readCb(args2.addr, actual2, args2.count, NULL);
	TEST_ASSERT_EQUAL_INT(args2.count, readCount);
	TEST_ASSERT_EQUAL_MEMORY(expected2, actual2, args2.count);

	readCount = test_readCb(args3.addr, actual3, args3.count, NULL);
	TEST_ASSERT_EQUAL_INT(args3.count, readCount);
	TEST_ASSERT_EQUAL_MEMORY(expected3, actual3, args3.count);

	readCount = test_readCb(args4.addr, actual4, args4.count, NULL);
	TEST_ASSERT_EQUAL_INT(args4.count, readCount);
	TEST_ASSERT_EQUAL_MEMORY(expected4, actual4, args4.count);

	free(actual1);
	free(actual2);
	free(actual3);
	free(actual4);

	ret = cache_deinit(cache);
	TEST_ASSERT_EQUAL_INT(EOK, ret);
}

TEST(test_threads, thread_read)
{
	ssize_t readCount = -1;
	int ret1, ret2;
	char *buffer1, *buffer2, *buffer3, *buffer4;
	pthread_t thread1, thread2;
	test_read_args_t args1, args2;
	uint64_t addr1 = LIBCACHE_ADDR_OFF_27, addr2 = 0x23dbULL;
	cachectx_t *cache;
	size_t count1 = 164 * sizeof(char);
	size_t count2 = 10 * sizeof(char);

	buffer1 = malloc(count1 * sizeof(char));
	TEST_ASSERT_NOT_NULL(buffer1);
	buffer2 = malloc(count2 * sizeof(char));
	TEST_ASSERT_NOT_NULL(buffer2);
	buffer3 = malloc(count1 * sizeof(char));
	TEST_ASSERT_NOT_NULL(buffer3);
	buffer4 = malloc(count2 * sizeof(char));
	TEST_ASSERT_NOT_NULL(buffer4);

	cache = cache_init(LIBCACHE_SRC_MEM_SIZE, LIBCACHE_LINE_SIZE, LIBCACHE_LINES_CNT, &ops);
	TEST_ASSERT_NOT_NULL(cache);

	args1 = (test_read_args_t) { .cache = cache, .addr = addr1, .buffer = buffer1, .count = count1 * sizeof(char) };
	args2 = (test_read_args_t) { .cache = cache, .addr = addr2, .buffer = buffer2, .count = count2 * sizeof(char) };

	ret1 = pthread_create(&thread1, NULL, &test_cache_read, (void *)&args1);
	TEST_ASSERT_EQUAL_INT(0, ret1);
	ret2 = pthread_create(&thread2, NULL, &test_cache_read, (void *)&args2);
	TEST_ASSERT_EQUAL_INT(0, ret2);

	ret1 = pthread_join(thread1, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret1);
	TEST_ASSERT_EQUAL_INT(args1.count, args1.actualCount);

	ret2 = pthread_join(thread2, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret2);
	TEST_ASSERT_EQUAL_INT(args2.count, args2.actualCount);

	ret1 = cache_deinit(cache);
	TEST_ASSERT_EQUAL_INT(EOK, ret1);

	cache = cache_init(LIBCACHE_SRC_MEM_SIZE, LIBCACHE_LINE_SIZE, LIBCACHE_LINES_CNT, &ops);
	TEST_ASSERT_NOT_NULL(cache);

	readCount = cache_read(cache, addr1, buffer3, count1);
	TEST_ASSERT_EQUAL_INT(count1, readCount);

	readCount = cache_read(cache, addr2, buffer4, count2);
	TEST_ASSERT_EQUAL_INT(count2, readCount);

	TEST_ASSERT_EQUAL_MEMORY(buffer1, buffer3, count1);
	TEST_ASSERT_EQUAL_MEMORY(buffer2, buffer4, count2);

	free(args1.buffer);
	free(args2.buffer);
	free(buffer3);
	free(buffer4);

	ret1 = cache_deinit(cache);
	TEST_ASSERT_EQUAL_INT(EOK, ret1);
}

TEST_GROUP_RUNNER(test_threads)
{
	RUN_TEST_CASE(test_threads, thread_write);
	RUN_TEST_CASE(test_threads, thread_read);
}

TEST_GROUP(test_flush);

TEST_SETUP(test_flush)
{
	ops.readCb = test_readCb;
	ops.writeCb = test_writeCb;
	offBitsNum = LOG2(LIBCACHE_LINE_SIZE);
	offMask = ((uint64_t)1 << offBitsNum) - 1;
}

TEST_TEAR_DOWN(test_flush)
{
}

TEST(test_flush, cache_flush_badAddrRange)
{
	int ret;
	cachectx_t *cache;
	uint64_t begAddr = LIBCACHE_ADDR_DUMMY, endAddr = LIBCACHE_ADDR_DUMMY / 2;

	cache = cache_init(LIBCACHE_SRC_MEM_SIZE, LIBCACHE_LINE_SIZE, LIBCACHE_LINES_CNT, &ops);
	TEST_ASSERT_NOT_NULL(cache);

	ret = cache_flush(cache, begAddr, endAddr);
	TEST_ASSERT_EQUAL_INT(-EINVAL, ret);

	ret = cache_deinit(cache);
	TEST_ASSERT_EQUAL_INT(EOK, ret);
}

TEST(test_flush, cache_flush_addrOutOfScope)
{
	int ret;
	cachectx_t *cache;
	uint64_t begAddr = LIBCACHE_SRC_MEM_SIZE + 10, endAddr = begAddr + 10;

	cache = cache_init(LIBCACHE_SRC_MEM_SIZE, LIBCACHE_LINE_SIZE, LIBCACHE_LINES_CNT, &ops);
	TEST_ASSERT_NOT_NULL(cache);

	ret = cache_flush(cache, begAddr, endAddr);
	TEST_ASSERT_EQUAL_INT(-EINVAL, ret);

	ret = cache_deinit(cache);
	TEST_ASSERT_EQUAL_INT(EOK, ret);
}

TEST(test_flush, cache_flush_addrPartiallyInScope)
{
	int ret;
	ssize_t writeCount, readCount;
	cachectx_t *cache;
	char *buffer1, *buffer2;
	uint64_t begAddr = LIBCACHE_SRC_MEM_SIZE - 72;
	size_t count = 64 * sizeof(char);

	buffer1 = "^#$%^$#%^&$#&$!@!*!!~~~!@#@$$_#@_+$ 4#$%#$%#%#$%^^#$^$#^#$^%@#$$";

	cache = cache_init(LIBCACHE_SRC_MEM_SIZE, LIBCACHE_LINE_SIZE, LIBCACHE_LINES_CNT, &ops);
	TEST_ASSERT_NOT_NULL(cache);

	writeCount = cache_write(cache, begAddr, buffer1, 64, LIBCACHE_WRITE_BACK);
	TEST_ASSERT_EQUAL_INT(count, writeCount);

	ret = cache_flush(cache, LIBCACHE_SRC_MEM_SIZE - 10, LIBCACHE_SRC_MEM_SIZE + 30);
	TEST_ASSERT_EQUAL_INT(0, ret);

	buffer2 = malloc(count);
	TEST_ASSERT_NOT_NULL(buffer2);
	readCount = test_readCb(begAddr, buffer2, count, NULL);
	TEST_ASSERT_EQUAL_INT(count, readCount);

	free(buffer2);

	ret = cache_deinit(cache);
	TEST_ASSERT_EQUAL_INT(EOK, ret);
}

TEST(test_flush, cache_flush_lines)
{
	int ret;
	ssize_t writeCount, readCount;
	cachectx_t *cache;
	char *buffer1, *buffer2, *actual1, *actual2, *expected1, *expected2;
	uint64_t begAddr = LIBCACHE_ADDR_OFF_57, endAddr = LIBCACHE_ADDR_OFF_57 + 300, offset1, offset2;
	size_t count1 = 213 * sizeof(char), count2 = 57 * sizeof(char);
	size_t remainder1, flushed1, remainder2, flushed2;

	buffer1 = "^&%$^*&^%*&(&*()*(*)_()*^&%#@$^$^%%$^$%^@%$^%#@$^^$#%^$#&$&$%&$#&$#&$%&^%^^@!!!!!@%$%#^#$%^$#%^&$#&^*$(^*&^)_)_(++(_)_(*)(&^%^*%^$#%$@#$@!# @!$#$#%$ $#%##$^$#%^#$$!@!*!!~~~!@#@$$_#@_+$ 4#$%#$%#%#$%^^#$^$#^#$^%@#$$";
	buffer2 = "HGESGEDRFEROFRELBFGHCZSSDQWQREERWEWTREYTYTRHGFVCCXGGHFHTR";

	cache = cache_init(LIBCACHE_SRC_MEM_SIZE, LIBCACHE_LINE_SIZE, LIBCACHE_LINES_CNT, &ops);
	TEST_ASSERT_NOT_NULL(cache);

	writeCount = cache_write(cache, begAddr, buffer1, count1, LIBCACHE_WRITE_BACK);
	TEST_ASSERT_EQUAL_INT(count1, writeCount);
	writeCount = cache_write(cache, endAddr, buffer2, count2, LIBCACHE_WRITE_BACK);
	TEST_ASSERT_EQUAL_INT(count2, writeCount);

	offset1 = begAddr & offMask;
	offset2 = endAddr & offMask;
	remainder1 = (count1 - (LIBCACHE_LINE_SIZE - offset1)) % LIBCACHE_LINE_SIZE;
	remainder2 = (count2 - (LIBCACHE_LINE_SIZE - offset2)) % LIBCACHE_LINE_SIZE;
	flushed1 = (offset1 + count1 + (LIBCACHE_LINE_SIZE - remainder1)) * sizeof(char);
	flushed2 = (offset2 + count2 + (LIBCACHE_LINE_SIZE - remainder2)) * sizeof(char);
	expected1 = malloc(flushed1);
	TEST_ASSERT_NOT_NULL(expected1);
	expected2 = malloc(flushed2);
	TEST_ASSERT_NOT_NULL(expected2);

	actual1 = malloc(flushed1);
	TEST_ASSERT_NOT_NULL(actual1);
	actual2 = malloc(flushed2);
	TEST_ASSERT_NOT_NULL(actual2);

	ret = cache_flush(cache, begAddr, endAddr + LIBCACHE_LINE_SIZE);
	TEST_ASSERT_EQUAL_INT(0, ret);

	readCount = test_readCb(begAddr - offset1, expected1, flushed1, NULL);
	TEST_ASSERT_EQUAL_INT(flushed1, readCount);
	memcpy(expected1 + offset1, buffer1, count1);

	readCount = test_readCb(endAddr - offset2, expected2, flushed2, NULL);
	TEST_ASSERT_EQUAL_INT(flushed2, readCount);
	memcpy(expected2 + offset2, buffer2, count2);

	readCount = test_readCb(begAddr - offset1, actual1, flushed1, NULL);
	TEST_ASSERT_EQUAL_INT(flushed1, readCount);
	TEST_ASSERT_EQUAL_MEMORY(expected1, actual1, flushed1);

	readCount = test_readCb(endAddr - offset2, actual2, flushed2, NULL);
	TEST_ASSERT_EQUAL_INT(flushed2, readCount);
	TEST_ASSERT_EQUAL_MEMORY(expected2, actual2, flushed2);

	free(expected1);
	free(expected2);
	free(actual1);
	free(actual2);

	ret = cache_deinit(cache);
	TEST_ASSERT_EQUAL_INT(EOK, ret);
}

TEST_GROUP_RUNNER(test_flush)
{
	RUN_TEST_CASE(test_flush, cache_flush_badAddrRange);
	RUN_TEST_CASE(test_flush, cache_flush_addrPartiallyInScope);
	RUN_TEST_CASE(test_flush, cache_flush_addrOutOfScope);
	RUN_TEST_CASE(test_flush, cache_flush_lines);
}

TEST_GROUP(test_inv);

TEST_SETUP(test_inv)
{
	ops.readCb = test_readCb;
	ops.writeCb = test_writeCb;
}

TEST_TEAR_DOWN(test_inv)
{
}

TEST(test_inv, cache_invalidate_badAddrRange)
{
	int ret;
	cachectx_t *cache;
	uint64_t begAddr = LIBCACHE_ADDR_DUMMY, endAddr = LIBCACHE_ADDR_DUMMY / 2;

	cache = cache_init(LIBCACHE_SRC_MEM_SIZE, LIBCACHE_LINE_SIZE, LIBCACHE_LINES_CNT, &ops);

	ret = cache_invalidate(cache, begAddr, endAddr);
	TEST_ASSERT_EQUAL_INT(-EINVAL, ret);

	ret = cache_deinit(cache);
	TEST_ASSERT_EQUAL_INT(EOK, ret);
}

TEST(test_inv, cache_invalidate_addrOutOfScope)
{
	int ret;
	cachectx_t *cache;
	uint64_t begAddr = LIBCACHE_SRC_MEM_SIZE + 10, endAddr = begAddr + 10;

	cache = cache_init(LIBCACHE_SRC_MEM_SIZE, LIBCACHE_LINE_SIZE, LIBCACHE_LINES_CNT, &ops);

	ret = cache_invalidate(cache, begAddr, endAddr);
	TEST_ASSERT_EQUAL_INT(-EINVAL, ret);

	ret = cache_deinit(cache);
	TEST_ASSERT_EQUAL_INT(EOK, ret);
}

TEST(test_inv, cache_invalidate_addrPartiallyInScope)
{
	int ret;
	ssize_t writeCount;
	cachectx_t *cache;
	char *buffer;
	uint64_t addr = LIBCACHE_SRC_MEM_SIZE - 64;

	size_t count = 64 * sizeof(char);

	buffer = "^#$%^$#%^&$#&$!@!*!!~~~!@#@$$_#@_+$ 4#$%#$%#%#$%^^#$^$#^#$^%@#$$";

	cache = cache_init(LIBCACHE_SRC_MEM_SIZE, LIBCACHE_LINE_SIZE, LIBCACHE_LINES_CNT, &ops);
	TEST_ASSERT_NOT_NULL(cache);

	writeCount = cache_write(cache, addr, buffer, 64, LIBCACHE_WRITE_BACK);
	TEST_ASSERT_EQUAL_INT(count, writeCount);

	ret = cache_invalidate(cache, LIBCACHE_SRC_MEM_SIZE - 10, LIBCACHE_SRC_MEM_SIZE + 30);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = cache_deinit(cache);
	TEST_ASSERT_EQUAL_INT(EOK, ret);
}

TEST(test_inv, cache_invalidate_lines)
{
	int ret;
	ssize_t readCount, writeCount;
	uint64_t addr = 0x1074ULL;
	cachectx_t *cache;
	char *buffer1, *buffer2, *actual, *expected;
	size_t count1 = 64 * sizeof(char), count2 = 15 * sizeof(char);

	cache = cache_init(LIBCACHE_SRC_MEM_SIZE, LIBCACHE_LINE_SIZE, LIBCACHE_LINES_CNT, &ops);
	TEST_ASSERT_NOT_NULL(cache);

	buffer1 = malloc(count1);
	TEST_ASSERT_NOT_NULL(buffer1);

	/* Read 'undamaged' data from src mem */
	readCount = cache_read(cache, addr, buffer1, count1);
	TEST_ASSERT_EQUAL_INT(count1, readCount);

	/* Prepare 'damaging' data */
	buffer2 = "^^(*(*(&(&*@##$";

	/* Create buffer with data expected from cache_read() after damage */
	expected = malloc(count1);
	TEST_ASSERT_NOT_NULL(expected);
	memcpy(expected, buffer2, count2);
	memcpy(expected + count2, buffer1 + count2, count1 - count2);

	/* Invalidate: next cache_read() should read data from src mem, not from cache */
	ret = cache_invalidate(cache, addr, addr + 2 * LIBCACHE_LINE_SIZE);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* 'Damage' src mem */
	writeCount = test_writeCb(addr, buffer2, count2, NULL);
	TEST_ASSERT_EQUAL_INT(count2, writeCount);

	/* Read a buffer from src mem */
	actual = malloc(count1);
	TEST_ASSERT_NOT_NULL(actual);
	readCount = cache_read(cache, addr, actual, count1);
	TEST_ASSERT_EQUAL_INT(count1, readCount);

	/* Verify if data was actually read from src mem */
	TEST_ASSERT_EQUAL_MEMORY(expected, actual, count1);

	free(buffer1);
	free(expected);
	free(actual);

	ret = cache_deinit(cache);
	TEST_ASSERT_EQUAL_INT(EOK, ret);
}

TEST_GROUP_RUNNER(test_inv)
{
	RUN_TEST_CASE(test_inv, cache_invalidate_badAddrRange);
	RUN_TEST_CASE(test_inv, cache_invalidate_addrOutOfScope);
	RUN_TEST_CASE(test_inv, cache_invalidate_addrPartiallyInScope);
	RUN_TEST_CASE(test_inv, cache_invalidate_lines);
}

TEST_GROUP(test_clean);

TEST_SETUP(test_clean)
{
	ops.readCb = test_readCb;
	ops.writeCb = test_writeCb;
}

TEST_TEAR_DOWN(test_clean)
{
}

TEST(test_clean, cache_clean_badAddrRange)
{
	int ret;
	cachectx_t *cache;
	uint64_t begAddr = LIBCACHE_ADDR_DUMMY, endAddr = LIBCACHE_ADDR_DUMMY / 2;

	cache = cache_init(LIBCACHE_SRC_MEM_SIZE, LIBCACHE_LINE_SIZE, LIBCACHE_LINES_CNT, &ops);

	ret = cache_clean(cache, begAddr, endAddr);
	TEST_ASSERT_EQUAL_INT(-EINVAL, ret);

	ret = cache_deinit(cache);
	TEST_ASSERT_EQUAL_INT(EOK, ret);
}

TEST(test_clean, cache_clean_addrOutOfScope)
{
	int ret;
	cachectx_t *cache;
	uint64_t begAddr = LIBCACHE_SRC_MEM_SIZE + 10, endAddr = begAddr + 10;

	cache = cache_init(LIBCACHE_SRC_MEM_SIZE, LIBCACHE_LINE_SIZE, LIBCACHE_LINES_CNT, &ops);

	ret = cache_clean(cache, begAddr, endAddr);
	TEST_ASSERT_EQUAL_INT(-EINVAL, ret);

	ret = cache_deinit(cache);
	TEST_ASSERT_EQUAL_INT(EOK, ret);
}

TEST(test_clean, cache_clean_addrPartiallyInScope)
{
	int ret;
	ssize_t writeCount;
	cachectx_t *cache;
	char *buffer;
	uint64_t addr = LIBCACHE_SRC_MEM_SIZE - 64;

	size_t count = 64 * sizeof(char);

	buffer = "^#$%^$#%^&$#&$!@!*!!~~~!@#@$$_#@_+$ 4#$%#$%#%#$%^^#$^$#^#$^%@#$$";

	cache = cache_init(LIBCACHE_SRC_MEM_SIZE, LIBCACHE_LINE_SIZE, LIBCACHE_LINES_CNT, &ops);
	TEST_ASSERT_NOT_NULL(cache);

	writeCount = cache_write(cache, addr, buffer, 64, LIBCACHE_WRITE_BACK);
	TEST_ASSERT_EQUAL_INT(count, writeCount);

	ret = cache_clean(cache, LIBCACHE_SRC_MEM_SIZE - 10, LIBCACHE_SRC_MEM_SIZE + 30);
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = cache_deinit(cache);
	TEST_ASSERT_EQUAL_INT(EOK, ret);
}

TEST(test_clean, cache_clean_lines)
{
	int ret;
	ssize_t readCount, writeCount;
	uint64_t addr = 0x1075ULL;
	cachectx_t *cache = NULL;
	char *bufferW, *expected, *bufferD, *actual;
	size_t countW = 21 * sizeof(char), countRead = 64 * sizeof(char), countD = 11 * sizeof(char);

	cache = cache_init(LIBCACHE_SRC_MEM_SIZE, LIBCACHE_LINE_SIZE, LIBCACHE_LINES_CNT, &ops);
	TEST_ASSERT_NOT_NULL(cache);

	bufferW = "^#&&^^%$#@%^@%$$%@&^%";

	/* Write a buffer to cache */
	writeCount = cache_write(cache, addr, bufferW, countW, LIBCACHE_WRITE_BACK);
	TEST_ASSERT_EQUAL_INT(countW, writeCount);

	/* Clean: next cache_read() should read data from src mem, not from cache */
	ret = cache_clean(cache, addr, addr + 2 * LIBCACHE_LINE_SIZE);
	TEST_ASSERT_EQUAL_INT(0, ret);

	/* Check if cache_clean() flushed data to src mem */
	expected = malloc(countRead);
	TEST_ASSERT_NOT_NULL(expected);
	readCount = test_readCb(addr, expected, countRead, NULL);
	TEST_ASSERT_EQUAL_INT(countRead, readCount);
	TEST_ASSERT_EQUAL_INT(0, memcmp(bufferW, expected, countW));

	/* Check if cache_clean() invalidated */

	/* 'Damage' src mem */
	bufferD = "GASFGDHGDER";
	writeCount = test_writeCb(addr, bufferD, countD, NULL);
	TEST_ASSERT_EQUAL_INT(countD, writeCount);

	/* Create buffer with data expected from cache_read() after damage */
	memcpy(expected, bufferD, countD);

	/* Read a buffer from src mem */
	actual = malloc(countRead);
	TEST_ASSERT_NOT_NULL(actual);
	readCount = cache_read(cache, addr, actual, countRead);
	TEST_ASSERT_EQUAL_INT(countRead, readCount);

	/* Verify if data was actually read from src mem */
	TEST_ASSERT_EQUAL_MEMORY(expected, actual, countRead);

	free(expected);
	free(actual);

	ret = cache_deinit(cache);
	TEST_ASSERT_EQUAL_INT(EOK, ret);
}

TEST_GROUP_RUNNER(test_clean)
{
	RUN_TEST_CASE(test_clean, cache_clean_badAddrRange);
	RUN_TEST_CASE(test_clean, cache_clean_addrOutOfScope);
	RUN_TEST_CASE(test_clean, cache_clean_addrPartiallyInScope);
	RUN_TEST_CASE(test_clean, cache_clean_lines);
}

TEST_GROUP(test_callback_err);

TEST_SETUP(test_callback_err)
{
	ops.readCb = test_readCb;
	ops.writeCb = test_writeCb;
}

TEST_TEAR_DOWN(test_callback_err)
{
}

TEST(test_callback_err, cache_write_writeCallbackErr)
{
	int ret = -1;
	ssize_t writeCount;
	cachectx_t *cache = NULL;
	uint64_t addr = LIBCACHE_ADDR_DUMMY;
	char *buffer;
	size_t count = 14 * sizeof(char);

	buffer = "FFE%^E^^W$%#@$";

	ops.writeCb = test_writeCbErr;
	cache = cache_init(LIBCACHE_SRC_MEM_SIZE, LIBCACHE_LINE_SIZE, LIBCACHE_LINES_CNT, &ops);
	TEST_ASSERT_NOT_NULL(cache);

	writeCount = cache_write(cache, addr, buffer, count, LIBCACHE_WRITE_THROUGH);
	TEST_ASSERT_EQUAL_INT(-EIO, writeCount);

	ret = cache_deinit(cache);
	TEST_ASSERT_EQUAL_INT(-EIO, ret);
}

TEST(test_callback_err, cache_write_readCallbackErr)
{
	int ret = -1;
	ssize_t writeCount;
	cachectx_t *cache = NULL;
	uint64_t addr = LIBCACHE_ADDR_DUMMY;
	char *bufferW;
	size_t count = 14 * sizeof(char);

	bufferW = "FFE%^E^^W$%#@$";

	ops.readCb = test_readCbErr;
	cache = cache_init(LIBCACHE_SRC_MEM_SIZE, LIBCACHE_LINE_SIZE, LIBCACHE_LINES_CNT, &ops);
	TEST_ASSERT_NOT_NULL(cache);

	writeCount = cache_write(cache, addr, bufferW, count, LIBCACHE_WRITE_THROUGH);
	TEST_ASSERT_EQUAL_INT(-EIO, writeCount);

	ret = cache_deinit(cache);
	TEST_ASSERT_EQUAL_INT(-EOK, ret);
}

TEST(test_callback_err, cache_read_readCallbackErr)
{
	int ret = -1;
	ssize_t readCount;
	cachectx_t *cache = NULL;
	uint64_t addr = LIBCACHE_ADDR_DUMMY;
	char *bufferR;
	size_t count = 14 * sizeof(char);

	ops.readCb = test_readCbErr;
	cache = cache_init(LIBCACHE_SRC_MEM_SIZE, LIBCACHE_LINE_SIZE, LIBCACHE_LINES_CNT, &ops);
	TEST_ASSERT_NOT_NULL(cache);

	bufferR = malloc(count);
	TEST_ASSERT_NOT_NULL(bufferR);
	readCount = cache_read(cache, addr, bufferR, count);
	TEST_ASSERT_EQUAL_INT(-EIO, readCount);

	free(bufferR);

	ret = cache_deinit(cache);
	TEST_ASSERT_EQUAL_INT(EOK, ret);
}

TEST(test_callback_err, cache_flushCallbackErr)
{
	int ret;
	ssize_t writeCount;
	uint64_t addr = LIBCACHE_ADDR_DUMMY;
	cachectx_t *cache;
	char *buffer;
	size_t count = 213 * sizeof(char);

	buffer = "^&%$^*&^%*&(&*()*(*)_()*^&%#@$^$^%%$^$%^@%$^%#@$^^$#%^$#&$&$%&$#&$#&$%&^%^^@!!!!!@%$%#^#$%^$#%^&$#&^*$(^*&^)_)_(++(_)_(*)(&^%^*%^$#%$@#$@!# @!$#$#%$ $#%##$^$#%^#$$!@!*!!~~~!@#@$$_#@_+$ 4#$%#$%#%#$%^^#$^$#^#$^%@#$$";

	ops.writeCb = test_writeCbErr;
	cache = cache_init(LIBCACHE_SRC_MEM_SIZE, LIBCACHE_LINE_SIZE, LIBCACHE_LINES_CNT, &ops);
	TEST_ASSERT_NOT_NULL(cache);

	writeCount = cache_write(cache, addr, buffer, count, LIBCACHE_WRITE_BACK);
	TEST_ASSERT_EQUAL_INT(count, writeCount);

	ret = cache_flush(cache, addr, addr + 4 * LIBCACHE_LINE_SIZE);
	TEST_ASSERT_EQUAL_INT(-EIO, ret);

	ret = cache_deinit(cache);
	TEST_ASSERT_EQUAL_INT(-EIO, ret);
}

TEST(test_callback_err, cache_cleanCallbackErr)
{
	int ret;
	ssize_t writeCount;
	uint64_t addr = LIBCACHE_ADDR_DUMMY;
	cachectx_t *cache;
	char *buffer;
	size_t count = 213 * sizeof(char);

	buffer = "^&%$^*&^%*&(&*()*(*)_()*^&%#@$^$^%%$^$%^@%$^%#@$^^$#%^$#&$&$%&$#&$#&$%&^%^^@!!!!!@%$%#^#$%^$#%^&$#&^*$(^*&^)_)_(++(_)_(*)(&^%^*%^$#%$@#$@!# @!$#$#%$ $#%##$^$#%^#$$!@!*!!~~~!@#@$$_#@_+$ 4#$%#$%#%#$%^^#$^$#^#$^%@#$$";

	ops.writeCb = test_writeCbErr;
	cache = cache_init(LIBCACHE_SRC_MEM_SIZE, LIBCACHE_LINE_SIZE, LIBCACHE_LINES_CNT, &ops);
	TEST_ASSERT_NOT_NULL(cache);

	writeCount = cache_write(cache, addr, buffer, count, LIBCACHE_WRITE_BACK);
	TEST_ASSERT_EQUAL_INT(count, writeCount);

	ret = cache_clean(cache, addr, addr + 4 * LIBCACHE_LINE_SIZE);
	TEST_ASSERT_EQUAL_INT(-EIO, ret);

	ret = cache_deinit(cache);
	TEST_ASSERT_EQUAL_INT(-EIO, ret);
}

TEST_GROUP_RUNNER(test_callback_err)
{
	RUN_TEST_CASE(test_callback_err, cache_write_writeCallbackErr);
	RUN_TEST_CASE(test_callback_err, cache_write_readCallbackErr);

	RUN_TEST_CASE(test_callback_err, cache_read_readCallbackErr);

	RUN_TEST_CASE(test_callback_err, cache_flushCallbackErr);

	RUN_TEST_CASE(test_callback_err, cache_cleanCallbackErr);
}

TEST_GROUP(test_integers);

TEST_SETUP(test_integers)
{
	ops.readCb = test_readCb;
	ops.writeCb = test_writeCb;
}

TEST_TEAR_DOWN(test_integers)
{
}

TEST(test_integers, cache_write_integers)
{
	int i, ret = -1;
	cachectx_t *cache;
	ssize_t writeCount, readCount;
	uint64_t addr = LIBCACHE_ADDR_INT;
	size_t num = 47;
	size_t count = num * sizeof(int);
	int expected[num], actual[num];
	int *ptr = &expected[0];

	cache = cache_init(LIBCACHE_SRC_MEM_SIZE, LIBCACHE_LINE_SIZE, LIBCACHE_LINES_CNT, &ops);
	TEST_ASSERT_NOT_NULL(cache);

	for (i = 0; i < num; ++i) {
		expected[i] = rand();
	}

	writeCount = cache_write(cache, addr, ptr, count, LIBCACHE_WRITE_BACK);
	TEST_ASSERT_EQUAL_INT(count, writeCount);

	readCount = cache_read(cache, addr, actual, count);
	TEST_ASSERT_EQUAL_INT(count, readCount);

	TEST_ASSERT_EQUAL_MEMORY(expected, actual, count);

	ret = cache_deinit(cache);
	TEST_ASSERT_EQUAL_INT(EOK, ret);
}

TEST(test_integers, cache_read_integers)
{
	int ret = -1;
	cachectx_t *cache;
	ssize_t readCount = 0;
	uint64_t addr = LIBCACHE_ADDR_INT;
	size_t num = 37;
	int expected[num], actual[num];
	size_t count = num * sizeof(int);

	cache = cache_init(LIBCACHE_SRC_MEM_SIZE, LIBCACHE_LINE_SIZE, LIBCACHE_LINES_CNT, &ops);
	TEST_ASSERT_NOT_NULL(cache);

	readCount = test_readCb(addr, expected, count, NULL);
	TEST_ASSERT_EQUAL_INT(readCount, count);

	readCount = cache_read(cache, addr, actual, count);
	TEST_ASSERT_EQUAL_INT(readCount, count);

	TEST_ASSERT_EQUAL_MEMORY(expected, actual, count);

	ret = cache_deinit(cache);
	TEST_ASSERT_EQUAL_INT(EOK, ret);
}

TEST_GROUP_RUNNER(test_integers)
{
	RUN_TEST_CASE(test_integers, cache_read_integers);
	RUN_TEST_CASE(test_integers, cache_write_integers);
}

void runner(void)
{
	/* TODO: remove libcache_test_char.txt and libcache_test_int.txt after test execution
	 * when issue below is resolved:
	 * https://github.com/phoenix-rtos/phoenix-rtos-project/issues/507
	 */
	int ret = -1;

	ops.ctx = NULL; /* Empty device driver context */

	ret = test_genCharFile();
	/* FIXME: workaround, test should fail */
	if (ret > -1) {
		srcMem = open("/var/libcache_test_char.txt", O_RDWR);
		TEST_ASSERT_GREATER_THAN_INT(-1, srcMem);

		RUN_TEST_GROUP(test_init);
		RUN_TEST_GROUP(test_deinit);
		RUN_TEST_GROUP(test_read_write);
		RUN_TEST_GROUP(test_threads);
		RUN_TEST_GROUP(test_inv);
		RUN_TEST_GROUP(test_flush);
		RUN_TEST_GROUP(test_clean);
		RUN_TEST_GROUP(test_callback_err);

		ret = close(srcMem);
		TEST_ASSERT_EQUAL_INT(0, ret);
	}

	ret = test_genIntFile();
	/* FIXME: workaround, test should fail */
	if (ret > -1) {
		srcMem = open("/var/libcache_test_int.txt", O_RDWR);
		TEST_ASSERT_GREATER_THAN_INT(-1, srcMem);

		RUN_TEST_GROUP(test_integers);
		ret = close(srcMem);
		TEST_ASSERT_EQUAL_INT(0, ret);
	}
}

int main(int argc, char *argv[])
{
	UnityMain(argc, (const char **)argv, runner);
	return 0;
}
