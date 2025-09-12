/*
 * Phoenix-RTOS
 *
 * libtrace tests
 *
 * Copyright 2025 by Phoenix Systems
 * Author: Adam Greloch
 *
 * %LICENSE%
 */


#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#include <trace.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "unity_fixture.h"

/* assumes TMP_DIR path is an (existing) ramdisk - otherwise tests will take *much* longer */
#define TMP_DIR  "/tmp"
#define BUF_SIZE (1 << 16)

#define FILE_NOT_DIR_PATH (TMP_DIR "/file")


TEST_GROUP(test_libtrace);


TEST_SETUP(test_libtrace)
{
	struct stat st;
	errno = 0;
	if (stat(TMP_DIR, &st) < 0) {
		FAIL(TMP_DIR " not found");
	}

	int fd = open(FILE_NOT_DIR_PATH, (O_WRONLY | O_CREAT | O_TRUNC), DEFFILEMODE);
	if (fd >= 0) {
		close(fd);
	}
	else {
		FAIL("open");
	}
}


TEST_TEAR_DOWN(test_libtrace)
{
	remove(FILE_NOT_DIR_PATH);
}


TEST(test_libtrace, test_libtrace_err)
{
	trace_ctx_t ctx, og_ctx;

	TEST_ASSERT_EQUAL_INT(-EINVAL, trace_init(NULL, true));

	TEST_ASSERT_EQUAL_INT(0, trace_init(&ctx, true));
	memcpy(&og_ctx, &ctx, sizeof(trace_ctx_t));

	TEST_ASSERT_EQUAL_INT(-ENOENT, trace_record(&ctx, 100, 1000, BUF_SIZE, FILE_NOT_DIR_PATH));
	TEST_ASSERT_EQUAL_INT(0, memcmp(&ctx, &og_ctx, sizeof(trace_ctx_t)));

	/* no corresponding trace_start() */
	TEST_ASSERT_EQUAL_INT(-EINVAL, trace_stopAndGather(&ctx, BUF_SIZE, TMP_DIR "/libtrace_err"));
	TEST_ASSERT_EQUAL_INT(0, memcmp(&ctx, &og_ctx, sizeof(trace_ctx_t)));

	TEST_ASSERT_EQUAL_INT(-EINVAL, trace_record(&ctx, 100, 1000, 0, TMP_DIR "/libtrace_err"));
	TEST_ASSERT_EQUAL_INT(0, memcmp(&ctx, &og_ctx, sizeof(trace_ctx_t)));

	TEST_ASSERT_EQUAL_INT(0, trace_start(&ctx));
	TEST_ASSERT_EQUAL_INT(-EINVAL, trace_stopAndGather(&ctx, 0, TMP_DIR "/libtrace_err"));
	TEST_ASSERT_EQUAL_INT(0, memcmp(&ctx, &og_ctx, sizeof(trace_ctx_t)));

	/*
	 * on EINVAL, trace_stopAndGather should still be able to stop the trace
	 * if trace_start(&ctx) succeeds, it means we have started a new trace
	 */
	TEST_ASSERT_EQUAL_INT(0, trace_start(&ctx));
	TEST_ASSERT_EQUAL_INT(0, trace_stopAndGather(&ctx, BUF_SIZE, TMP_DIR "/libtrace_err"));
	TEST_ASSERT_EQUAL_INT(0, memcmp(&ctx, &og_ctx, sizeof(trace_ctx_t)));
}


TEST(test_libtrace, test_libtrace_start_stop)
{
	trace_ctx_t ctx, og_ctx;

	TEST_ASSERT_EQUAL_INT(0, trace_init(&ctx, true));
	memcpy(&og_ctx, &ctx, sizeof(trace_ctx_t));

	for (size_t rep = 0; rep < 3; rep++) {
		TEST_ASSERT_EQUAL_INT(0, trace_start(&ctx));

		struct timespec ts;
		for (size_t i = 0; i < 100; i++) {
			clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
		}

		/* may print a warning about read buffer utilization - it is ok */
		TEST_ASSERT_EQUAL_INT(0, trace_stopAndGather(&ctx, BUF_SIZE, TMP_DIR "/libtrace_start_stop"));
		TEST_ASSERT_EQUAL_INT(0, memcmp(&ctx, &og_ctx, sizeof(trace_ctx_t)));
	}
}


TEST(test_libtrace, test_libtrace_record)
{
	trace_ctx_t ctx, og_ctx;

	TEST_ASSERT_EQUAL_INT(0, trace_init(&ctx, true));
	memcpy(&og_ctx, &ctx, sizeof(trace_ctx_t));

	for (size_t rep = 0; rep < 3; rep++) {
		TEST_ASSERT_EQUAL_INT(0, trace_record(&ctx, 100, 100, BUF_SIZE, TMP_DIR "/libtrace_record"));
		TEST_ASSERT_EQUAL_INT(0, memcmp(&ctx, &og_ctx, sizeof(trace_ctx_t)));
	}
}


TEST_GROUP_RUNNER(test_libtrace)
{
	RUN_TEST_CASE(test_libtrace, test_libtrace_err);
	RUN_TEST_CASE(test_libtrace, test_libtrace_start_stop);
	RUN_TEST_CASE(test_libtrace, test_libtrace_record);
}


void runner(void)
{
	RUN_TEST_GROUP(test_libtrace);
}


int main(int argc, char *argv[])
{
	return UnityMain(argc, (const char **)argv, runner) == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
