/*
 * Phoenix-RTOS
 *
 *    libc-tests
 *    HEADER:
 *    - string.h
 *    TESTED:
 *    - memset(),
 *    - memmove()
 *
 * Copyright 2023 Phoenix Systems
 * Author: Adam Debek
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <ulimit.h>
#include <stdint.h>
#include <unity_fixture.h>

#define BUF_LEN         32
#define BIG_SIZE        1024
#define TEST_STR        "THIS IS TEST MEM123"
#define TEST_STR_SIZE   sizeof(TEST_STR)
#define OFFSET          40
#define OFFSET_BIG      1500
#define MEM_SIZE        100
#define MEM_SIZE_BIG    4000
#define OVERLAP_LEN     (TEST_STR_SIZE / 2)
#define OVERLAP_LEN_BIG (BIG_SIZE / 2)

static uint8_t buf[BUF_LEN];
static uint8_t buf_byte[256];
static uint8_t buf_byte_exp[256];
static uint8_t *mem;
static uint8_t *mem_big;
static uint8_t big_buf[1024];
static const int values[] = { 0, 1, 16, 50, 127, 128, 200, 255,
	768, 257, 272, 306, 383, 384, 456, 511 };

#define VALUES_NUM sizeof(values) / sizeof(values[0])


static void setup_mem_big(int offset)
{
	TEST_ASSERT_NOT_NULL(mem_big);
	memset(mem_big, 0, MEM_SIZE_BIG);
	uint8_t *const dst = mem_big + OFFSET_BIG;
	uint8_t *const dst_end = dst + BIG_SIZE;
	uint8_t *const mem_end = mem_big + MEM_SIZE_BIG;

	uint8_t *ret = memcpy(dst + offset, big_buf, dst_end - dst);
	TEST_ASSERT_EQUAL_PTR(dst + offset, ret);
	TEST_ASSERT_EACH_EQUAL_UINT8(0, mem_big, (dst - mem_big) + offset);
	TEST_ASSERT_EACH_EQUAL_UINT8(0, dst_end + offset, mem_end - (dst_end + offset));
}


TEST_GROUP(string_memset);
TEST_GROUP(string_memmove);
TEST_GROUP(string_memmove_big);


TEST_SETUP(string_memset)
{
}


TEST_TEAR_DOWN(string_memset)
{
}


TEST(string_memset, basic)
{
	int i;
	for (i = 0; i < VALUES_NUM; i++) {
		uint8_t *ret = memset(buf, values[i], BUF_LEN);
		TEST_ASSERT_EQUAL_PTR(buf, ret);
		TEST_ASSERT_EACH_EQUAL_UINT8((uint8_t)values[i], buf, BUF_LEN);
	}
}


TEST(string_memset, zero)
{
	uint8_t val = 5;
	uint8_t *ret = memset(&val, 0, 0);
	TEST_ASSERT_EQUAL_PTR(&val, ret);
	TEST_ASSERT_EQUAL_UINT8(5, val);
}


TEST(string_memset, big)
{
	int srcOff, lenOff, n = BIG_SIZE - 2 * 7;
	uint8_t *ret;

	/*
	 * Test all combinations with remainders of division by word for len, and src.
	 * It matters when memset() is optimized to copy whole words of data.
	 */
	for (lenOff = 0; lenOff < 8; lenOff++) {
		for (srcOff = 0; srcOff < 8; srcOff++) {
			int setSz = n + lenOff;
			ret = memset(big_buf, 0, BIG_SIZE);
			ret = memset(big_buf + srcOff, 10, setSz);
			TEST_ASSERT_EQUAL_PTR(big_buf + srcOff, ret);
			TEST_ASSERT_EACH_EQUAL_UINT8(10, big_buf + srcOff, setSz);
			if (srcOff != 0) {
				TEST_ASSERT_EACH_EQUAL_UINT8(0, big_buf, srcOff);
			}
			if (lenOff != 7) {
				TEST_ASSERT_EACH_EQUAL_UINT8(0, big_buf + srcOff + setSz, 7 - lenOff);
			}
		}
	}
}


TEST(string_memset, single_byte)
{
	uint8_t val = 5;
	uint8_t *ret = memset(&val, 0, 1);
	TEST_ASSERT_EQUAL_PTR(&val, ret);
	TEST_ASSERT_EQUAL_UINT8(0, val);
}


TEST(string_memset, byte_val)
{
	int i;
	uint8_t *ret;

	for (i = 0; i < sizeof(buf_byte); i++) {
		ret = memset(buf_byte + i, i, 1);
		TEST_ASSERT_EQUAL_PTR(buf_byte + i, ret);
		buf_byte_exp[i] = i;
	}
	TEST_ASSERT_EQUAL_MEMORY(buf_byte_exp, buf_byte, sizeof(buf_byte));
}


TEST_SETUP(string_memmove)
{
	mem = (uint8_t *)calloc(MEM_SIZE, 1);
	TEST_ASSERT_NOT_NULL(mem);
	uint8_t *const dst = mem + OFFSET;
	uint8_t *const mem_end = mem + MEM_SIZE;
	uint8_t *const dst_end = dst + TEST_STR_SIZE;

	uint8_t *ret = memcpy(dst, TEST_STR, dst_end - dst);
	TEST_ASSERT_EQUAL_PTR(dst, ret);
	TEST_ASSERT_EACH_EQUAL_UINT8(0, mem, dst - mem);
	TEST_ASSERT_EACH_EQUAL_UINT8(0, dst_end, mem_end - dst_end);
}


TEST_TEAR_DOWN(string_memmove)
{
	free(mem);
}


TEST(string_memmove, no_overlap_end)
{
	uint8_t *const src = mem + OFFSET;
	uint8_t *const dst = mem + (MEM_SIZE - TEST_STR_SIZE);
	uint8_t *const dst_end = mem + MEM_SIZE;
	uint8_t *const src_end = src + TEST_STR_SIZE;

	/* Move to the end of allocated memory */
	uint8_t *ret = memmove(dst, src, dst_end - dst);
	TEST_ASSERT_EQUAL_PTR(dst, ret);
	TEST_ASSERT_EQUAL_MEMORY(TEST_STR, dst, dst_end - dst);
	TEST_ASSERT_EQUAL_MEMORY(TEST_STR, src, src_end - src);
	TEST_ASSERT_EACH_EQUAL_UINT8(0, src_end, dst - src_end);
	TEST_ASSERT_EACH_EQUAL_UINT8(0, mem, src - mem);
}


TEST(string_memmove, no_overlap_between)
{
	const int move_offset = OFFSET + TEST_STR_SIZE + 10;
	uint8_t *const src = mem + OFFSET;
	uint8_t *const src_end = src + TEST_STR_SIZE;
	uint8_t *const dst = mem + move_offset;
	uint8_t *const dst_end = dst + TEST_STR_SIZE;
	uint8_t *const mem_end = mem + MEM_SIZE;

	/* Move somewhere between */
	uint8_t *ret = memmove(dst, src, dst_end - dst);
	TEST_ASSERT_EQUAL_PTR(dst, ret);
	TEST_ASSERT_EQUAL_MEMORY(TEST_STR, dst, dst_end - dst);
	TEST_ASSERT_EQUAL_MEMORY(TEST_STR, src, src_end - src);
	TEST_ASSERT_EACH_EQUAL_UINT8(0, mem, src - mem);
	TEST_ASSERT_EACH_EQUAL_UINT8(0, src_end, dst - src_end);
	TEST_ASSERT_EACH_EQUAL_UINT8(0, dst_end, mem_end - dst_end);
}


TEST(string_memmove, no_overlap_adjacent)
{
	uint8_t *const src = mem + OFFSET;
	uint8_t *const dst = src + TEST_STR_SIZE;
	uint8_t *const dst_end = dst + TEST_STR_SIZE;
	uint8_t *const mem_end = mem + MEM_SIZE;

	/* Move so that memory blocks are next to each other*/
	uint8_t *ret = memmove(dst, src, dst_end - dst);
	TEST_ASSERT_EQUAL_PTR(dst, ret);
	TEST_ASSERT_EQUAL_MEMORY(TEST_STR, dst, dst_end - dst);
	TEST_ASSERT_EQUAL_MEMORY(TEST_STR, src, dst - src);
	TEST_ASSERT_EACH_EQUAL_UINT8(0, mem, src - mem);
	TEST_ASSERT_EACH_EQUAL_UINT8(0, dst_end, mem_end - dst_end);
}


TEST(string_memmove, whole_overlap)
{
	uint8_t *const src = mem + OFFSET;
	uint8_t *const src_end = src + TEST_STR_SIZE;
	uint8_t *const mem_end = mem + MEM_SIZE;

	/* Move at the same address as block is */
	uint8_t *ret = memmove(src, src, src_end - src);
	TEST_ASSERT_EQUAL_PTR(src, ret);
	TEST_ASSERT_EQUAL_MEMORY(TEST_STR, src, src_end - src);
	TEST_ASSERT_EACH_EQUAL_UINT8(0, mem, src - mem);
	TEST_ASSERT_EACH_EQUAL_UINT8(0, src_end, mem_end - src_end);
}


TEST(string_memmove, right_overlap)
{
	uint8_t *const src = mem + OFFSET;
	uint8_t *const dst = src + OVERLAP_LEN;
	uint8_t *const dst_end = dst + TEST_STR_SIZE;
	uint8_t *const mem_end = mem + MEM_SIZE;

	/* Move data so that data blocks overlap */
	uint8_t *ret = memmove(dst, src, dst_end - dst);
	TEST_ASSERT_EQUAL_PTR(dst, ret);
	TEST_ASSERT_EQUAL_MEMORY(TEST_STR, dst, dst_end - dst);
	TEST_ASSERT_EQUAL_MEMORY(TEST_STR, src, dst - src);
	TEST_ASSERT_EACH_EQUAL_UINT8(0, mem, src - mem);
	TEST_ASSERT_EACH_EQUAL_UINT8(0, dst_end, mem_end - dst_end);
}


TEST(string_memmove, left_overlap)
{
	uint8_t *const src = mem + OFFSET;
	uint8_t *const src_end = src + TEST_STR_SIZE;
	uint8_t *const dst = mem + (OFFSET - OVERLAP_LEN);
	uint8_t *const dst_end = dst + TEST_STR_SIZE;
	uint8_t *const mem_end = mem + MEM_SIZE;
	char *const tstr_ovlp_end = TEST_STR + OVERLAP_LEN;

	/* Move data so that data blocks overlap */
	uint8_t *ret = memmove(dst, src, dst_end - dst);
	TEST_ASSERT_EQUAL_PTR(dst, ret);
	TEST_ASSERT_EQUAL_MEMORY(TEST_STR, dst, dst_end - dst);
	TEST_ASSERT_EQUAL_MEMORY(tstr_ovlp_end, dst_end, tstr_ovlp_end - TEST_STR);
	TEST_ASSERT_EACH_EQUAL_UINT8(0, mem, dst - mem);
	TEST_ASSERT_EACH_EQUAL_UINT8(0, src_end, mem_end - src_end);
}


TEST(string_memmove, move_nothing)
{
	uint8_t *const dst = mem + OFFSET;
	uint8_t *const dst_end = dst + TEST_STR_SIZE;
	uint8_t *const mem_end = mem + MEM_SIZE;

	/* Check if any data is moved */
	uint8_t *ret = memmove(dst, TEST_STR, 0);
	TEST_ASSERT_EQUAL_PTR(dst, ret);
	TEST_ASSERT_EQUAL_MEMORY(TEST_STR, dst, dst_end - dst);
	TEST_ASSERT_EACH_EQUAL_UINT8(0, mem, dst - mem);
	TEST_ASSERT_EACH_EQUAL_UINT8(0, dst_end, mem_end - dst_end);
}


TEST_SETUP(string_memmove_big)
{
	mem_big = (uint8_t *)calloc(MEM_SIZE_BIG, 1);
	TEST_ASSERT_NOT_NULL(mem_big);
	uint8_t *const dst = mem_big + OFFSET_BIG;
	uint8_t *const dst_end = dst + BIG_SIZE;
	uint8_t *const mem_end = mem_big + MEM_SIZE_BIG;

	uint8_t *ret = memcpy(dst, big_buf, dst_end - dst);
	TEST_ASSERT_EQUAL_PTR(dst, ret);
	TEST_ASSERT_EACH_EQUAL_UINT8(0, mem_big, dst - mem_big);
	TEST_ASSERT_EACH_EQUAL_UINT8(0, dst_end, mem_end - dst_end);
}


TEST_TEAR_DOWN(string_memmove_big)
{
	free(mem_big);
}


TEST(string_memmove_big, no_overlap)
{
	/* Move data so that it doesn't overlap */
	uint8_t *const src = mem_big + OFFSET_BIG;
	uint8_t *const src_end = src + BIG_SIZE;
	uint8_t *const dst = mem_big + OFFSET_BIG + BIG_SIZE + 7;
	uint8_t *const mem_end = mem_big + MEM_SIZE_BIG;
	uint8_t *ret;
	int dstOff, srcOff, lenOff, n = BIG_SIZE - 7;

	/*
	 * Test all combinations with remainders of division by word for len, dst and src.
	 * It matters when memmove() is optimized to copy whole words of data.
	 */
	for (lenOff = 0; lenOff < 8; lenOff++) {
		for (srcOff = 0; srcOff < 8; srcOff++) {
			for (dstOff = 0; dstOff < 8; dstOff++) {
				int moveSz = n + lenOff;
				setup_mem_big(srcOff);
				ret = memmove(dst + dstOff, src + srcOff, moveSz);
				TEST_ASSERT_EQUAL_PTR(dst + dstOff, ret);
				TEST_ASSERT_EQUAL_MEMORY(big_buf, dst + dstOff, moveSz);
				TEST_ASSERT_EQUAL_MEMORY(big_buf, src + srcOff, moveSz);
				TEST_ASSERT_EACH_EQUAL_UINT8(0, mem_big, (src - mem_big) + srcOff);
				TEST_ASSERT_EACH_EQUAL_UINT8(0, dst + dstOff + moveSz, mem_end - (dst + dstOff + moveSz));
				if (srcOff != 7 && dstOff != 0) {
					TEST_ASSERT_EACH_EQUAL_UINT8(0, src_end + srcOff, dstOff + (7 - srcOff));
				}
			}
		}
	}
}


TEST(string_memmove_big, whole_overlap)
{
	/* Move at the same address as block is */
	uint8_t *const src = mem_big + OFFSET_BIG;
	uint8_t *const src_end = src + BIG_SIZE;
	uint8_t *const mem_end = mem_big + MEM_SIZE_BIG;
	uint8_t *ret;
	int srcOff, lenOff, n = BIG_SIZE - 8;

	/*
	 * Test all combinations with remainders of division by word for len, dst and src.
	 * It matters when memmove() is optimized to copy whole words of data.
	 */
	for (lenOff = 0; lenOff < 8; lenOff++) {
		for (srcOff = 0; srcOff < 8; srcOff++) {
			int moveSz = n + lenOff;
			setup_mem_big(srcOff);
			ret = memmove(src + srcOff, src + srcOff, moveSz);
			TEST_ASSERT_EQUAL_PTR(src + srcOff, ret);
			TEST_ASSERT_EQUAL_MEMORY(big_buf, src + srcOff, moveSz);
			TEST_ASSERT_EACH_EQUAL_UINT8(0, mem_big, (src - mem_big) + srcOff);
			TEST_ASSERT_EACH_EQUAL_UINT8(0, src_end + srcOff, mem_end - (src_end + srcOff));
		}
	}
}


TEST(string_memmove_big, right_overlap)
{
	/* Move data so that data blocks overlap */
	uint8_t *const src = mem_big + OFFSET_BIG;
	uint8_t *const dst = mem_big + OFFSET_BIG + OVERLAP_LEN_BIG;
	uint8_t *const mem_end = mem_big + MEM_SIZE_BIG;
	uint8_t *ret;
	int dstOff, srcOff, lenOff, n = BIG_SIZE - 7;

	/*
	 * Test all combinations with remainders of division by word for len, dst and src.
	 * It matters when memmove() is optimized to copy whole words of data.
	 */
	for (lenOff = 0; lenOff < 8; lenOff++) {
		for (srcOff = 0; srcOff < 8; srcOff++) {
			for (dstOff = 0; dstOff < 8; dstOff++) {
				int moveSz = n + lenOff;
				setup_mem_big(srcOff);
				ret = memmove(dst + dstOff, src + srcOff, moveSz);
				TEST_ASSERT_EQUAL_PTR(dst + dstOff, ret);
				TEST_ASSERT_EQUAL_MEMORY(big_buf, dst + dstOff, moveSz);
				TEST_ASSERT_EQUAL_MEMORY(big_buf, src + srcOff, (dst + dstOff) - (src + srcOff));
				TEST_ASSERT_EACH_EQUAL_UINT8(0, mem_big, (src - mem_big) + srcOff);
				TEST_ASSERT_EACH_EQUAL_UINT8(0, dst + dstOff + moveSz, mem_end - (dst + dstOff + moveSz));
			}
		}
	}
}


TEST(string_memmove_big, left_overlap)
{
	/* Move data so that data blocks overlap */
	uint8_t *const src = mem_big + OFFSET_BIG;
	uint8_t *const src_end = src + BIG_SIZE;
	uint8_t *const dst = mem_big + (OFFSET_BIG - OVERLAP_LEN_BIG);
	uint8_t *const mem_end = mem_big + MEM_SIZE_BIG;
	uint8_t *ret;
	int srcOff, dstOff, lenOff, n = BIG_SIZE - 7;

	/*
	 * Test all combinations with remainders of division by word for len, dst and src.
	 * It matters when memmove() is optimized to copy whole words of data.
	 */
	for (lenOff = 0; lenOff < 8; lenOff++) {
		for (srcOff = 0; srcOff < 8; srcOff++) {
			for (dstOff = 0; dstOff < 8; dstOff++) {
				int moveSz = n + lenOff;
				setup_mem_big(srcOff);
				ret = memmove(dst + dstOff, src + srcOff, moveSz);
				TEST_ASSERT_EQUAL_PTR(dst + dstOff, ret);
				TEST_ASSERT_EQUAL_MEMORY(big_buf, dst + dstOff, moveSz);
				TEST_ASSERT_EQUAL_MEMORY(big_buf + (dst + dstOff + moveSz - (src + srcOff)), dst + dstOff + moveSz, (src_end + srcOff) - (dst + dstOff + moveSz));
				TEST_ASSERT_EACH_EQUAL_UINT8(0, mem_big, (dst - mem_big) + dstOff);
				TEST_ASSERT_EACH_EQUAL_UINT8(0, src_end + srcOff, mem_end - (src_end + srcOff));
			}
		}
	}
}


TEST_GROUP_RUNNER(string_memset)
{
	RUN_TEST_CASE(string_memset, byte_val);
	RUN_TEST_CASE(string_memset, basic);
	RUN_TEST_CASE(string_memset, zero);
	RUN_TEST_CASE(string_memset, big);
	RUN_TEST_CASE(string_memset, single_byte);
}


TEST_GROUP_RUNNER(string_memmove)
{
	RUN_TEST_CASE(string_memmove, no_overlap_end);
	RUN_TEST_CASE(string_memmove, no_overlap_between);
	RUN_TEST_CASE(string_memmove, no_overlap_adjacent);
	RUN_TEST_CASE(string_memmove, whole_overlap);
	RUN_TEST_CASE(string_memmove, right_overlap);
	RUN_TEST_CASE(string_memmove, left_overlap);
	RUN_TEST_CASE(string_memmove, move_nothing);
}


TEST_GROUP_RUNNER(string_memmove_big)
{
	int i;
	for (i = 0; i < BIG_SIZE; i++) {
		big_buf[i] = i % 256;
	}

	RUN_TEST_CASE(string_memmove_big, no_overlap);
	RUN_TEST_CASE(string_memmove_big, whole_overlap);
	RUN_TEST_CASE(string_memmove_big, right_overlap);
	RUN_TEST_CASE(string_memmove_big, left_overlap);
}
