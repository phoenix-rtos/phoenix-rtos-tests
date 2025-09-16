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
#include <stdlib.h>
#include <pthread.h>
#include <lf-fifo.h>

#include "unity_fixture.h"


#define MAX_FIFO_SIZE 8192

#define SPEED_TEST_OPS  1000000
#define SPEED_TEST_MOPS (SPEED_TEST_OPS / 1000000)

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


TEST_GROUP_RUNNER(test_lf_fifo)
{
	RUN_TEST_CASE(test_lf_fifo, push);
	RUN_TEST_CASE(test_lf_fifo, push_wrap);
	RUN_TEST_CASE(test_lf_fifo, push_many);
	RUN_TEST_CASE(test_lf_fifo, pop_many);
	RUN_TEST_CASE(test_lf_fifo, ow_push);
	RUN_TEST_CASE(test_lf_fifo, ow_push_many);
	RUN_TEST_CASE(test_lf_fifo, ow_pop_many);
#if 0
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
