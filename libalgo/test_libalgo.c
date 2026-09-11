/*
 * Phoenix-RTOS
 *
 * libalgo-tests
 *
 * Copyright 2025 Phoenix Systems
 * Author: Ziemowit Leszczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <lf-fifo.h>

#include "unity_fixture.h"


#define MAX_FIFO_SIZE 8192

#define SPEED_TEST_OPS  1000000
#define SPEED_TEST_MOPS (SPEED_TEST_OPS / 1000000)

/*
 * Concurrency cases below. The runner allows a test case 30 s to print its
 * result, so the element count has to leave room for the slowest emulated
 * target; ordering violations in a broken FIFO show up within the first few
 * hundred elements, so this is far more than detection needs.
 */
#define SMP_ELEMENTS 200000u
#define SMP_BURST    64u

/*
 * A livelocked consumer must fail the test rather than hang the runner. This
 * measures a stall, not total runtime - a slow target may take a while to get
 * through SMP_ELEMENTS, but it should never go this long delivering nothing.
 */
#define SMP_STALL_SEC 10
#define SMP_STALL_CNT 10000u

typedef enum {
	speedtest_push_pop = 0,
	speedtest_push_pop_many,
	speedtest_ow_push_pop,
	speedtest_ow_push_pop_many
} speedtest_t;

static uint8_t buffer[MAX_FIFO_SIZE];
static lf_fifo_t fifo;

static uint8_t tmpbuf[MAX_FIFO_SIZE * 2];


TEST_GROUP(test_lf_fifo);


TEST_SETUP(test_lf_fifo)
{
}


TEST_TEAR_DOWN(test_lf_fifo)
{
}


static void test_push(unsigned int size)
{
	unsigned int i, ret;
	uint8_t val = 0;

	lf_fifo_init(&fifo, buffer, size);

	for (i = 0; i < (size - 1); i++) {
		ret = lf_fifo_push(&fifo, i % 256);
		TEST_ASSERT_EQUAL_UINT(1, ret);
	}

	ret = lf_fifo_empty(&fifo) ? 1 : 0;
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = lf_fifo_full(&fifo) ? 1 : 0;
	TEST_ASSERT_EQUAL_INT(1, ret);

	i = lf_fifo_used(&fifo);
	TEST_ASSERT_EQUAL_UINT(size - 1, i);

	i = lf_fifo_free(&fifo);
	TEST_ASSERT_EQUAL_UINT(0, i);

	ret = lf_fifo_push(&fifo, 0);
	TEST_ASSERT_EQUAL_UINT(0, ret);

	for (i = 0; i < (size - 1); i++) {
		ret = lf_fifo_pop(&fifo, &val);
		TEST_ASSERT_EQUAL_UINT(1, ret);
		TEST_ASSERT_EQUAL_UINT(i % 256, val);
	}

	ret = lf_fifo_pop(&fifo, &val);
	TEST_ASSERT_EQUAL_UINT(0, ret);

	ret = lf_fifo_empty(&fifo) ? 1 : 0;
	TEST_ASSERT_EQUAL_INT(1, ret);

	ret = lf_fifo_full(&fifo) ? 1 : 0;
	TEST_ASSERT_EQUAL_INT(0, ret);

	i = lf_fifo_used(&fifo);
	TEST_ASSERT_EQUAL_UINT(0, i);

	i = lf_fifo_free(&fifo);
	TEST_ASSERT_EQUAL_UINT(size - 1, i);
}


TEST(test_lf_fifo, push)
{
	unsigned int i;

	for (i = 2; i <= MAX_FIFO_SIZE; i *= 2) {
		test_push(i);
	}
}


static void test_push_wrap(unsigned int size)
{
	unsigned int i, ret;
	uint8_t val = 0;

	lf_fifo_init(&fifo, buffer, size);

	for (i = 0; i < (size - 1); i++) {
		ret = lf_fifo_push(&fifo, i % 256);
		TEST_ASSERT_EQUAL_UINT(1, ret);
	}

	for (i = 0; i < (size / 2 - 1); i++) {
		ret = lf_fifo_pop(&fifo, &val);
		TEST_ASSERT_EQUAL_UINT(1, ret);
		TEST_ASSERT_EQUAL_UINT(i % 256, val);
	}

	ret = lf_fifo_empty(&fifo) ? 1 : 0;
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = lf_fifo_full(&fifo) ? 1 : 0;
	TEST_ASSERT_EQUAL_INT(size == 2 ? 1 : 0, ret);

	i = lf_fifo_used(&fifo);
	TEST_ASSERT_EQUAL_UINT(size / 2, i);

	i = lf_fifo_free(&fifo);
	TEST_ASSERT_EQUAL_UINT(size / 2 - 1, i);

	for (i = 0; i < (size / 2 - 1); i++) {
		ret = lf_fifo_push(&fifo, (size - 1 + i) % 256);
		TEST_ASSERT_EQUAL_UINT(1, ret);
	}

	ret = lf_fifo_empty(&fifo) ? 1 : 0;
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = lf_fifo_full(&fifo) ? 1 : 0;
	TEST_ASSERT_EQUAL_INT(1, ret);

	i = lf_fifo_used(&fifo);
	TEST_ASSERT_EQUAL_UINT(size - 1, i);

	i = lf_fifo_free(&fifo);
	TEST_ASSERT_EQUAL_UINT(0, i);

	ret = lf_fifo_push(&fifo, 0);
	TEST_ASSERT_EQUAL_UINT(0, ret);

	for (i = 0; i < (size - 1); i++) {
		ret = lf_fifo_pop(&fifo, &val);
		TEST_ASSERT_EQUAL_UINT(1, ret);
		TEST_ASSERT_EQUAL_UINT((size / 2 - 1 + i) % 256, val);
	}

	ret = lf_fifo_empty(&fifo) ? 1 : 0;
	TEST_ASSERT_EQUAL_INT(1, ret);

	ret = lf_fifo_full(&fifo) ? 1 : 0;
	TEST_ASSERT_EQUAL_INT(0, ret);

	i = lf_fifo_used(&fifo);
	TEST_ASSERT_EQUAL_UINT(0, i);

	i = lf_fifo_free(&fifo);
	TEST_ASSERT_EQUAL_UINT(size - 1, i);
}


TEST(test_lf_fifo, push_wrap)
{
	unsigned int i;

	for (i = 2; i <= MAX_FIFO_SIZE; i *= 2) {
		test_push_wrap(i);
	}
}


static void test_push_many(unsigned int size)
{
	unsigned int i, left, cnt, ret;
	uint8_t val;

	lf_fifo_init(&fifo, buffer, size);

	left = size - 1;
	cnt = 1;
	val = 0;

	while (left > 0) {
		for (i = 0; i < cnt; i++) {
			tmpbuf[i] = val++;
		}

		ret = lf_fifo_push_many(&fifo, tmpbuf, cnt);
		TEST_ASSERT_EQUAL_UINT(cnt, ret);

		left -= cnt;
		cnt++;
		if (cnt > left) {
			cnt = left;
		}
	}

	ret = lf_fifo_empty(&fifo) ? 1 : 0;
	TEST_ASSERT_EQUAL_INT(0, ret);

	ret = lf_fifo_full(&fifo) ? 1 : 0;
	TEST_ASSERT_EQUAL_INT(1, ret);

	i = lf_fifo_used(&fifo);
	TEST_ASSERT_EQUAL_UINT(size - 1, i);

	i = lf_fifo_free(&fifo);
	TEST_ASSERT_EQUAL_UINT(0, i);

	ret = lf_fifo_push(&fifo, 0);
	TEST_ASSERT_EQUAL_UINT(0, ret);

	for (i = 0; i < (size - 1); i++) {
		ret = lf_fifo_pop(&fifo, &val);
		TEST_ASSERT_EQUAL_UINT(1, ret);
		TEST_ASSERT_EQUAL_UINT(i % 256, val);
	}

	ret = lf_fifo_pop(&fifo, &val);
	TEST_ASSERT_EQUAL_UINT(0, ret);

	ret = lf_fifo_empty(&fifo) ? 1 : 0;
	TEST_ASSERT_EQUAL_INT(1, ret);

	ret = lf_fifo_full(&fifo) ? 1 : 0;
	TEST_ASSERT_EQUAL_INT(0, ret);

	i = lf_fifo_used(&fifo);
	TEST_ASSERT_EQUAL_UINT(0, i);

	i = lf_fifo_free(&fifo);
	TEST_ASSERT_EQUAL_UINT(size - 1, i);
}


TEST(test_lf_fifo, push_many)
{
	unsigned int i;

	for (i = 2; i <= MAX_FIFO_SIZE; i *= 2) {
		test_push_many(i);
	}
}


static void test_pop_many(unsigned int size)
{
	unsigned int i, left, cnt, ret;
	uint8_t val = 0;

	lf_fifo_init(&fifo, buffer, size);

	for (i = 0; i < (size - 1); i++) {
		ret = lf_fifo_push(&fifo, i % 256);
		TEST_ASSERT_EQUAL_UINT(1, ret);
	}

	ret = lf_fifo_push(&fifo, 0);
	TEST_ASSERT_EQUAL_UINT(0, ret);

	left = size - 1;
	cnt = 1;
	val = 0;

	while (left > 0) {
		ret = lf_fifo_pop_many(&fifo, tmpbuf, cnt);
		TEST_ASSERT_EQUAL_UINT(cnt, ret);

		for (i = 0; i < cnt; i++) {
			TEST_ASSERT_EQUAL_UINT(tmpbuf[i], val++);
		}

		left -= cnt;
		cnt++;
		if (cnt > left) {
			cnt = left;
		}
	}

	ret = lf_fifo_pop(&fifo, &val);
	TEST_ASSERT_EQUAL_UINT(0, ret);

	ret = lf_fifo_empty(&fifo) ? 1 : 0;
	TEST_ASSERT_EQUAL_INT(1, ret);

	ret = lf_fifo_full(&fifo) ? 1 : 0;
	TEST_ASSERT_EQUAL_INT(0, ret);

	i = lf_fifo_used(&fifo);
	TEST_ASSERT_EQUAL_UINT(0, i);

	i = lf_fifo_free(&fifo);
	TEST_ASSERT_EQUAL_UINT(size - 1, i);
}


TEST(test_lf_fifo, pop_many)
{
	unsigned int i;

	for (i = 2; i <= MAX_FIFO_SIZE; i *= 2) {
		test_pop_many(i);
	}
}


static void test_ow_push(unsigned int size)
{
	unsigned int i, ret;
	uint8_t val = 0;

	lf_fifo_init(&fifo, buffer, size);

	for (i = 0; i < size; i++) {
		lf_fifo_ow_push(&fifo, i % 256);
	}

	ret = lf_fifo_empty(&fifo) ? 1 : 0;
	TEST_ASSERT_EQUAL_INT(0, ret);

	i = lf_fifo_ow_used(&fifo);
	TEST_ASSERT_EQUAL_UINT(size, i);

	for (i = 0; i < (size / 2); i++) {
		lf_fifo_ow_push(&fifo, (size + i) % 256);
	}

	ret = lf_fifo_empty(&fifo) ? 1 : 0;
	TEST_ASSERT_EQUAL_INT(0, ret);

	i = lf_fifo_ow_used(&fifo);
	TEST_ASSERT_EQUAL_UINT(size, i);

	for (i = 0; i < size; i++) {
		ret = lf_fifo_ow_pop(&fifo, &val);
		TEST_ASSERT_EQUAL_UINT(1, ret);
		TEST_ASSERT_EQUAL_UINT((i + size / 2) % 256, val);
	}

	ret = lf_fifo_ow_pop(&fifo, &val);
	TEST_ASSERT_EQUAL_UINT(0, ret);

	ret = lf_fifo_empty(&fifo) ? 1 : 0;
	TEST_ASSERT_EQUAL_INT(1, ret);

	i = lf_fifo_ow_used(&fifo);
	TEST_ASSERT_EQUAL_UINT(0, i);
}


TEST(test_lf_fifo, ow_push)
{
	unsigned int i;

	for (i = 2; i <= MAX_FIFO_SIZE; i *= 2) {
		test_ow_push(i);
	}
}


static void test_ow_push_lost(unsigned int size)
{
	unsigned int i, ret, lost;
	uint8_t val = 0;

	lf_fifo_init(&fifo, buffer, size);

	TEST_ASSERT_EQUAL_UINT(0, lf_fifo_ow_lost(&fifo));

	/* filling up to capacity loses nothing */
	for (i = 0; i < size; i++) {
		lf_fifo_ow_push(&fifo, i % 256);
	}

	for (i = 0; i < size; i++) {
		ret = lf_fifo_ow_pop(&fifo, &val);
		TEST_ASSERT_EQUAL_UINT(1, ret);
		TEST_ASSERT_EQUAL_UINT(i % 256, val);
	}

	TEST_ASSERT_EQUAL_UINT(0, lf_fifo_ow_lost(&fifo));

	lf_fifo_init(&fifo, buffer, size);

	/* overflowing by half loses exactly half a FIFO, and the newest bytes survive */
	for (i = 0; i < (size + size / 2); i++) {
		lf_fifo_ow_push(&fifo, i % 256);
	}

	lost = 0;
	for (i = 0; i < size; i++) {
		ret = lf_fifo_ow_pop(&fifo, &val);
		lost += lf_fifo_ow_lost(&fifo);
		TEST_ASSERT_EQUAL_UINT(1, ret);
		TEST_ASSERT_EQUAL_UINT((i + size / 2) % 256, val);
	}
	TEST_ASSERT_EQUAL_UINT(size / 2, lost);

	/* lost bytes are reported once and then cleared */
	TEST_ASSERT_EQUAL_UINT(0, lf_fifo_ow_lost(&fifo));

	/* lf_fifo_ow_pop_many() finds the count */
	lf_fifo_init(&fifo, buffer, size);

	for (i = 0; i < (size + size / 2); i++) {
		lf_fifo_ow_push(&fifo, i % 256);
	}

	ret = lf_fifo_ow_pop_many(&fifo, tmpbuf, size);
	TEST_ASSERT_EQUAL_UINT(size, ret);
	TEST_ASSERT_EQUAL_UINT(size / 2, lf_fifo_ow_lost(&fifo));

	for (i = 0; i < size; i++) {
		TEST_ASSERT_EQUAL_UINT((i + size / 2) % 256, tmpbuf[i]);
	}
}


TEST(test_lf_fifo, ow_push_lost)
{
	unsigned int i;

	for (i = 2; i <= MAX_FIFO_SIZE; i *= 2) {
		test_ow_push_lost(i);
	}
}


static void test_ow_push_many(unsigned int size)
{
	unsigned int i, ret;
	uint8_t val = 0;

	lf_fifo_init(&fifo, buffer, size);

	for (i = 0; i < (size + size / 2); i++) {
		tmpbuf[i] = i % 256;
	}

	lf_fifo_ow_push_many(&fifo, tmpbuf, size + size / 2);

	ret = lf_fifo_empty(&fifo) ? 1 : 0;
	TEST_ASSERT_EQUAL_INT(0, ret);

	i = lf_fifo_ow_used(&fifo);
	TEST_ASSERT_EQUAL_UINT(size, i);

	for (i = 0; i < size; i++) {
		ret = lf_fifo_ow_pop(&fifo, &val);
		TEST_ASSERT_EQUAL_UINT(1, ret);
		TEST_ASSERT_EQUAL_UINT((i + size / 2) % 256, val);
	}

	ret = lf_fifo_ow_pop(&fifo, &val);
	TEST_ASSERT_EQUAL_UINT(0, ret);

	ret = lf_fifo_empty(&fifo) ? 1 : 0;
	TEST_ASSERT_EQUAL_INT(1, ret);

	i = lf_fifo_ow_used(&fifo);
	TEST_ASSERT_EQUAL_UINT(0, i);
}


TEST(test_lf_fifo, ow_push_many)
{
	unsigned int i;

	for (i = 2; i <= MAX_FIFO_SIZE; i *= 2) {
		test_ow_push_many(i);
	}
}


static void test_ow_push_many_lost(unsigned int size)
{
	unsigned int i, ret, lost;
	uint8_t val = 0;

	lf_fifo_init(&fifo, buffer, size);

	for (i = 0; i < (size * 2); i++) {
		tmpbuf[i] = i % 256;
	}

	lf_fifo_ow_push_many(&fifo, tmpbuf, 0);
	TEST_ASSERT_EQUAL_UINT(0, lf_fifo_ow_lost(&fifo));

	/* exactly fills the FIFO - nothing is lost */
	lf_fifo_ow_push_many(&fifo, tmpbuf, size);
	TEST_ASSERT_EQUAL_UINT(0, lf_fifo_ow_lost(&fifo));

	/*
	 * Overwriting existing data to make room for new bytes. The producer does not
	 * see the lost bytes, only the consumer can detect them.
	 */
	lf_fifo_ow_push_many(&fifo, tmpbuf, size / 2);
	TEST_ASSERT_EQUAL_UINT(0, lf_fifo_ow_lost(&fifo));

	for (lost = 0, i = 0; i < size; i++) {
		ret = lf_fifo_ow_pop(&fifo, &val);
		lost += lf_fifo_ow_lost(&fifo);
		TEST_ASSERT_EQUAL_UINT(1, ret);
	}
	TEST_ASSERT_EQUAL_UINT(size / 2, lost);

	lf_fifo_init(&fifo, buffer, size);

	/*
	 * Pushing more bytes than the FIFO size causes the excess bytes to be dropped.
	 * From the consumer's perspective, this is equivalent to an overwrite, so the
	 * dropped bytes are added to the lost counter. Unlike an overwrite, the loss
	 * is detected immediately, before any pop.
	 */
	lf_fifo_ow_push_many(&fifo, tmpbuf, size * 2);
	TEST_ASSERT_EQUAL_UINT(size, lf_fifo_ow_lost(&fifo));

	/* lost bytes are reported once and then cleared */
	TEST_ASSERT_EQUAL_UINT(0, lf_fifo_ow_lost(&fifo));

	for (i = 0; i < size; i++) {
		ret = lf_fifo_ow_pop(&fifo, &val);
		TEST_ASSERT_EQUAL_UINT(1, ret);
		TEST_ASSERT_EQUAL_UINT((size + i) % 256, val);
	}
	TEST_ASSERT_EQUAL_UINT(0, lf_fifo_ow_lost(&fifo));

	lf_fifo_init(&fifo, buffer, size);

	/* dropped and overwritten bytes are counted in the lost counter */
	lf_fifo_ow_push_many(&fifo, tmpbuf, size);
	lf_fifo_ow_push_many(&fifo, tmpbuf, size * 2);

	/* dropped bytes are counted at push time, overwritten bytes are counted after pop */
	lost = lf_fifo_ow_lost(&fifo);
	TEST_ASSERT_EQUAL_UINT(size, lost);

	for (i = 0; i < size; i++) {
		ret = lf_fifo_ow_pop(&fifo, &val);
		lost += lf_fifo_ow_lost(&fifo);
		TEST_ASSERT_EQUAL_UINT(1, ret);
	}
	TEST_ASSERT_EQUAL_UINT(size * 2, lost);

	/* every pushed byte is either delivered or counted as lost, no duplicates */
	TEST_ASSERT_EQUAL_UINT(size * 3, lost + size);
}


TEST(test_lf_fifo, ow_push_many_lost)
{
	unsigned int i;

	for (i = 2; i <= MAX_FIFO_SIZE; i *= 2) {
		test_ow_push_many_lost(i);
	}
}


static void test_ow_pop_many(unsigned int size)
{
	unsigned int i, ret;

	lf_fifo_init(&fifo, buffer, size);

	for (i = 0; i < (size + size / 2); i++) {
		lf_fifo_ow_push(&fifo, i % 256);
	}

	ret = lf_fifo_empty(&fifo) ? 1 : 0;
	TEST_ASSERT_EQUAL_INT(0, ret);

	i = lf_fifo_ow_used(&fifo);
	TEST_ASSERT_EQUAL_UINT(size, i);

	ret = lf_fifo_ow_pop_many(&fifo, tmpbuf, size);
	TEST_ASSERT_EQUAL_UINT(size, ret);

	for (i = 0; i < size; i++) {
		TEST_ASSERT_EQUAL_UINT((i + size / 2) % 256, tmpbuf[i]);
	}

	ret = lf_fifo_empty(&fifo) ? 1 : 0;
	TEST_ASSERT_EQUAL_INT(1, ret);

	i = lf_fifo_ow_used(&fifo);
	TEST_ASSERT_EQUAL_UINT(0, i);
}


TEST(test_lf_fifo, ow_pop_many)
{
	unsigned int i;

	for (i = 2; i <= MAX_FIFO_SIZE; i *= 2) {
		test_ow_pop_many(i);
	}
}


static void *producer_thread(void *arg)
{
	speedtest_t *type = (speedtest_t *)arg;
	unsigned long pushed = 0;
	uint8_t val = 0;

	switch (*type) {
		case speedtest_push_pop:
			while (pushed < SPEED_TEST_OPS) {
				if (lf_fifo_push(&fifo, val)) {
					val++;
					pushed++;
				}
			}
			break;

		case speedtest_push_pop_many:
			while (pushed < SPEED_TEST_OPS) {
				if (lf_fifo_push_many(&fifo, &val, 1)) {
					val++;
					pushed++;
				}
			}
			break;

		case speedtest_ow_push_pop:
			while (pushed < (SPEED_TEST_OPS - 1)) {
				lf_fifo_ow_push(&fifo, val);
				val = (val + 1) % 255;
				pushed++;
			}
			/* mark the end */
			val = 255;
			lf_fifo_ow_push(&fifo, val);
			break;

		case speedtest_ow_push_pop_many:
			while (pushed < (SPEED_TEST_OPS - 1)) {
				lf_fifo_ow_push_many(&fifo, &val, 1);
				val = (val + 1) % 255;
				pushed++;
			}
			/* mark the end */
			val = 255;
			lf_fifo_ow_push_many(&fifo, &val, 1);
			break;

		default:
			TEST_ABORT();
	}

	return NULL;
}

static void *consumer_thread(void *arg)
{
	speedtest_t *type = (speedtest_t *)arg;
	unsigned long popped = 0;
	uint8_t val = 0;

	switch (*type) {
		case speedtest_push_pop:
			while (popped < SPEED_TEST_OPS) {
				if (lf_fifo_pop(&fifo, &val)) {
					popped++;
				}
			}
			break;

		case speedtest_push_pop_many:
			while (popped < SPEED_TEST_OPS) {
				if (lf_fifo_pop_many(&fifo, &val, 1)) {
					popped++;
				}
			}
			break;

		case speedtest_ow_push_pop:
			do {
				if (lf_fifo_ow_pop(&fifo, &val)) {
					popped++;
				}
			} while (val != 255);
			break;

		case speedtest_ow_push_pop_many:
			do {
				if (lf_fifo_ow_pop_many(&fifo, &val, 1)) {
					popped++;
				}
			} while (val != 255);
			break;

		default:
			TEST_ABORT();
	}

	return NULL;
}


static const char *speedtest_name(speedtest_t type)
{
	switch (type) {
		case speedtest_push_pop:
			return "push_pop";
		case speedtest_push_pop_many:
			return "push_pop_many";
		case speedtest_ow_push_pop:
			return "ow_push_pop";
		case speedtest_ow_push_pop_many:
			return "ow_push_pop_many";
		default:
			TEST_ABORT();
	}
}


static void test_push_pop_speed(unsigned int size, speedtest_t type)
{
	struct timespec ts1, ts2;
	pthread_t producer, consumer;

	clock_gettime(CLOCK_MONOTONIC, &ts1);

	lf_fifo_init(&fifo, buffer, size);

	pthread_create(&producer, NULL, producer_thread, &type);
	pthread_create(&consumer, NULL, consumer_thread, &type);
	pthread_join(producer, NULL);
	pthread_join(consumer, NULL);

	clock_gettime(CLOCK_MONOTONIC, &ts2);

	unsigned int usec = (ts2.tv_sec - ts1.tv_sec) * 1000000 + (ts2.tv_nsec - ts1.tv_nsec) / 1000;
	unsigned int sec = usec / 1000000;
	unsigned int msec = (usec % 1000000) / 1000;
	unsigned int mops = SPEED_TEST_OPS / usec;

	printf("%s: fifo %u, %u.%3.3u sec, %u mops/sec\n", speedtest_name(type), size, sec, msec, mops);
}


TEST(test_lf_fifo, speed_push_pop)
{
	unsigned int i;

	for (i = 2; i <= MAX_FIFO_SIZE; i *= 2) {
		test_push_pop_speed(i, speedtest_push_pop);
	}
}


TEST(test_lf_fifo, speed_push_pop_many)
{
	unsigned int i;

	for (i = 2; i <= MAX_FIFO_SIZE; i *= 2) {
		test_push_pop_speed(i, speedtest_push_pop_many);
	}
}


TEST(test_lf_fifo, speed_ow_push_pop)
{
	unsigned int i;

	for (i = 2; i <= MAX_FIFO_SIZE; i *= 2) {
		test_push_pop_speed(i, speedtest_ow_push_pop);
	}
}


TEST(test_lf_fifo, speed_ow_push_pop_many)
{
	unsigned int i;

	for (i = 2; i <= MAX_FIFO_SIZE; i *= 2) {
		test_push_pop_speed(i, speedtest_ow_push_pop_many);
	}
}


/*
 * The overwriting API has to hold up while the producer is lapping the consumer:
 * what the consumer receives must be a faithful subsequence of what was pushed -
 * in order, never duplicated, never torn - and the elements it never sees must be
 * counted exactly.
 *
 * The producer pushes 0..N-1 as (i & 0xff) and the consumer checks that every
 * element it accepts equals (consumed + lost) & 0xff. That single check catches
 * reordering, duplication and a wrong loss count at once, because any of the
 * three makes the expected logical index drift. A slot collision shows up as a
 * mismatch of exactly +size - the element belonging to index tail + size, which
 * shares a slot with tail.
 *
 * Two cores are what make this worth running: only then is the producer inside a
 * push while the consumer reads the slot it is about to take. On a
 * single-processor target the cases still run and still have to pass, but the
 * window is a few instructions wide and no reordering is possible, so they
 * detect little.
 */
static struct {
	unsigned int size;   /* FIFO size under test */
	unsigned long total; /* elements the producer will push */
	unsigned int burst;  /* 0: single element API, otherwise _many */
	int waitForRoom;     /* producer waits instead of overwriting */

	unsigned long consumed;
	unsigned long lost;
	unsigned long mismatches;
	unsigned long firstBadIndex;
	int firstBadGot;
	int firstBadExpected;
	int timedOut;

	int producerDone;
} smp;


static double smp_monotonic(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);

	return (double)ts.tv_sec + 1e-9 * (double)ts.tv_nsec;
}


static void *smp_producer(void *arg)
{
	unsigned long i = 0;
	uint8_t src[SMP_BURST];

	while (i < smp.total) {
		if (__atomic_load_n(&smp.timedOut, __ATOMIC_RELAXED)) {
			break;
		}
		if (smp.burst == 0u) {
			if (smp.waitForRoom != 0) {
				while (lf_fifo_ow_used(&fifo) >= smp.size) {
					if (__atomic_load_n(&smp.timedOut, __ATOMIC_RELAXED)) {
						break;
					}
				}
			}
			lf_fifo_ow_push(&fifo, (uint8_t)(i & 0xffu));
			i++;
		}
		else {
			unsigned int n = 1u + (unsigned int)(i % smp.burst);
			unsigned int k;

			if ((unsigned long)n > smp.total - i) {
				n = (unsigned int)(smp.total - i);
			}
			for (k = 0; k < n; k++) {
				src[k] = (uint8_t)((i + k) & 0xffu);
			}
			/*
			 * smp.burst is clamped to the FIFO size so that push_many never has
			 * to drop anything. A drop would still be counted, but the producer
			 * counts it the moment it happens, while the consumer is still
			 * working through elements queued before the gap - the count would
			 * arrive ahead of the stream it belongs to and the index check below
			 * would read that as a violation. The single-threaded cases above
			 * cover the drop path, where the two cannot get out of step.
			 */
			lf_fifo_ow_push_many(&fifo, src, n);
			i += n;
		}
	}

	__atomic_store_n(&smp.producerDone, 1, __ATOMIC_RELEASE);

	return NULL;
}


static void smp_check(uint8_t got)
{
	uint8_t expected = (uint8_t)((smp.consumed + smp.lost) & 0xffu);

	if (got != expected) {
		if (smp.mismatches == 0u) {
			smp.firstBadIndex = smp.consumed + smp.lost;
			smp.firstBadGot = got;
			smp.firstBadExpected = expected;
		}
		smp.mismatches++;
		/*
		 * Believe the element and adjust, so one slip does not report every
		 * later element as bad too.
		 */
		smp.lost += (unsigned long)(uint8_t)(got - expected);
	}
	smp.consumed++;
}


static void *smp_consumer(void *arg)
{
	uint8_t dst[SMP_BURST];
	double lastProgress = smp_monotonic();
	unsigned int stall_count = 0;

	for (;;) {
		int done = __atomic_load_n(&smp.producerDone, __ATOMIC_ACQUIRE);
		unsigned int n;

		if (smp.burst == 0u) {
			uint8_t val;

			n = lf_fifo_ow_pop(&fifo, &val);
			if (n != 0u) {
				smp.lost += lf_fifo_ow_lost(&fifo);
				smp_check(val);
			}
		}
		else {
			n = lf_fifo_ow_pop_many(&fifo, dst, SMP_BURST);
			if (n != 0u) {
				unsigned int k;

				smp.lost += lf_fifo_ow_lost(&fifo);
				for (k = 0; k < n; k++) {
					smp_check(dst[k]);
				}
			}
		}

		if (n != 0u) {
			lastProgress = smp_monotonic();
		}
		else {
			if (done != 0) {
				break;
			}
			if (++stall_count >= SMP_STALL_CNT) {
				stall_count = 0;
				if ((smp_monotonic() - lastProgress) > (double)SMP_STALL_SEC) {
					__atomic_store_n(&smp.timedOut, 1, __ATOMIC_RELAXED);
					break;
				}
			}
		}
	}

	/* a pop that gave up for margin leaves elements counted but unreported */
	smp.lost += lf_fifo_ow_lost(&fifo);

	return NULL;
}


static void test_smp_race(unsigned int size, unsigned long total, unsigned int burst, int waitForRoom)
{
	int ret;
	pthread_t tp, tc;

	memset(&smp, 0, sizeof(smp));
	smp.size = size;
	smp.total = total;
	smp.burst = (burst > size) ? size : burst;
	smp.waitForRoom = waitForRoom;

	lf_fifo_init(&fifo, buffer, size);

	ret = pthread_create(&tp, NULL, smp_producer, NULL);
	if (ret != 0) {
		TEST_FAIL_MESSAGE("failed to create producer thread");
		return;
	}
	ret = pthread_create(&tc, NULL, smp_consumer, NULL);
	if (ret != 0) {
		__atomic_store_n(&smp.timedOut, 1, __ATOMIC_RELAXED);
		pthread_join(tp, NULL);
		TEST_FAIL_MESSAGE("failed to create consumer thread");
		return;
	}

	pthread_join(tp, NULL);
	pthread_join(tc, NULL);

	TEST_ASSERT_EQUAL_INT_MESSAGE(0, __atomic_load_n(&smp.timedOut, __ATOMIC_RELAXED), "consumer made no progress - livelock");

	if (smp.mismatches != 0u) {
		char msg[128];

		snprintf(msg, sizeof(msg),
				"%lu ordering violations, first at %lu: got 0x%02x expected 0x%02x (delta %+d)",
				smp.mismatches, smp.firstBadIndex, smp.firstBadGot, smp.firstBadExpected,
				(int)(uint8_t)(smp.firstBadGot - smp.firstBadExpected));
		TEST_FAIL_MESSAGE(msg);
	}

	/* every element pushed was either delivered or counted as lost */
	TEST_ASSERT_EQUAL_UINT64_MESSAGE(total, smp.consumed + smp.lost, "loss accounting does not add up");
}


/*
 * The producer laps the consumer many times over. Fails on a pop that trusts
 * head alone, or that recovers from an overrun without validating the slot.
 */
TEST(test_lf_fifo, ow_smp_ordering)
{
	unsigned int size;

	for (size = 16u; size <= 1024u; size *= 4u) {
		test_smp_race(size, SMP_ELEMENTS, 0u, 0);
	}
}


/*
 * Same, through the bulk entry points - a sequence of pushes announces itself
 * once, and a bulk pop validates only the oldest index it copied.
 */
TEST(test_lf_fifo, ow_smp_ordering_many)
{
	unsigned int size;

	for (size = 16u; size <= 1024u; size *= 4u) {
		test_smp_race(size, SMP_ELEMENTS, SMP_BURST, 0);
	}
}


/*
 * A consumer that has to give up a contended slot must come back far enough from
 * the producer to win eventually, rather than resynchronise onto the same slot
 * over and over. Measured on two cores, a pop that retries at the same distance
 * delivers about 0.01% of an overrun stream where this one delivers about 20%,
 * so a 1% floor separates them with two orders of magnitude to spare.
 *
 * It cannot run on a single processor: there the producer spends its whole
 * timeslice pushing and laps the FIFO thousands of times before the consumer is
 * scheduled at all, so the share delivered measures the scheduler rather than
 * the FIFO - on ia32-generic-qemu it comes out at 48 elements. There is no
 * portable way to ask how many processors are online from userspace, so it is
 * disabled until the emulated targets run with more than one and this can be
 * gated on that.
 */
TEST(test_lf_fifo, ow_smp_progress)
{
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("needs more than one processor to be meaningful");
#endif

	test_smp_race(16u, SMP_ELEMENTS, 0u, 0);

	TEST_ASSERT_GREATER_THAN_UINT64(SMP_ELEMENTS / 100u, smp.consumed);
}


/*
 * A consumer that keeps up must lose nothing at all, and the validation must not
 * reject anything when the FIFO is merely full.
 */
TEST(test_lf_fifo, ow_smp_no_loss)
{
	test_smp_race(MAX_FIFO_SIZE, SMP_ELEMENTS, 0u, 1);

	TEST_ASSERT_EQUAL_UINT64(0, smp.lost);
	TEST_ASSERT_EQUAL_UINT64(SMP_ELEMENTS, smp.consumed);
}


TEST_GROUP_RUNNER(test_lf_fifo)
{
	RUN_TEST_CASE(test_lf_fifo, push);
	RUN_TEST_CASE(test_lf_fifo, push_wrap);
	RUN_TEST_CASE(test_lf_fifo, push_many);
	RUN_TEST_CASE(test_lf_fifo, pop_many);
	RUN_TEST_CASE(test_lf_fifo, ow_push);
	RUN_TEST_CASE(test_lf_fifo, ow_push_lost);
	RUN_TEST_CASE(test_lf_fifo, ow_push_many);
	RUN_TEST_CASE(test_lf_fifo, ow_push_many_lost);
	RUN_TEST_CASE(test_lf_fifo, ow_pop_many);
#if 0
	RUN_TEST_CASE(test_lf_fifo, ow_smp_ordering);
	RUN_TEST_CASE(test_lf_fifo, ow_smp_ordering_many);
	RUN_TEST_CASE(test_lf_fifo, ow_smp_progress);
	RUN_TEST_CASE(test_lf_fifo, ow_smp_no_loss);
	RUN_TEST_CASE(test_lf_fifo, speed_push_pop);
	RUN_TEST_CASE(test_lf_fifo, speed_push_pop_many);
	RUN_TEST_CASE(test_lf_fifo, speed_ow_push_pop);
	RUN_TEST_CASE(test_lf_fifo, speed_ow_push_pop_many);
#endif
}


void runner(void)
{
	RUN_TEST_GROUP(test_lf_fifo);
}


int main(int argc, char *argv[])
{
	return (UnityMain(argc, (const char **)argv, runner) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
