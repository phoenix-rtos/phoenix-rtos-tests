/*
 * Phoenix-RTOS
 *
 * test-sys-perf
 *
 * Test for perf subsystem
 *
 * Copyright 2025 Phoenix Systems
 * Author: Adam Greloch
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/perf.h>

#include <unity_fixture.h>


#define BUF_SIZE 64

static char buf[BUF_SIZE];


TEST_GROUP(perf_test_common);
TEST_GROUP(perf_test_trace);


/*
 *--------------------------------- invalid params tests -----------------------------------*
 */


TEST_SETUP(perf_test_common)
{
}


TEST_TEAR_DOWN(perf_test_common)
{
}


TEST(perf_test_common, invalid_mode)
{
	TEST_ASSERT_EQUAL_INT(-ENOSYS, perf_start(-1, 0, NULL));
	TEST_ASSERT_EQUAL_INT(-ENOSYS, perf_read(-1, buf, BUF_SIZE, 0));
	TEST_ASSERT_EQUAL_INT(-ENOSYS, perf_stop(-1));
	TEST_ASSERT_EQUAL_INT(-ENOSYS, perf_finish(-1));
}


TEST(perf_test_common, invalid_calls_when_perf_off)
{
	perf_mode_t modes[] = { perf_mode_threads, perf_mode_trace };

	for (size_t i = 0; i < sizeof(modes) / sizeof(perf_mode_t); i++) {
		TEST_ASSERT_EQUAL_INT(-EINVAL, perf_read(modes[i], buf, BUF_SIZE, 0));

		/* may be -EINVAL or -ENOSYS whether mode supports perf_stop */
		TEST_ASSERT_LESS_THAN_INT(0, perf_stop(modes[i]));

		TEST_ASSERT_EQUAL_INT(-EINVAL, perf_finish(modes[i]));
	}
}


TEST(perf_test_common, start_read_finish)
{
	perf_mode_t modes[] = { perf_mode_threads, perf_mode_trace };
	int rv;

	/* TODO: fix threads mode? */
	for (size_t i = 1; i < sizeof(modes) / sizeof(perf_mode_t); i++) {
		for (size_t rep = 0; rep < 5; rep++) {
			rv = perf_start(modes[i], 0, NULL);
			if (rv != EOK) {
				TEST_ASSERT_EQUAL_INT(-ENOSYS, rv);
			}
			else {
				usleep(100);
				TEST_ASSERT_EQUAL_INT(BUF_SIZE, perf_read(modes[i], buf, BUF_SIZE, 0));
				TEST_ASSERT_EQUAL_INT(EOK, perf_finish(modes[i]));
			}
		}
	}
}


/*
 *--------------------------------- perf_mode_trace tests ---------------------------------*
 */


TEST_SETUP(perf_test_trace)
{
}


TEST_TEAR_DOWN(perf_test_trace)
{
}


TEST(perf_test_trace, trace_start_stop_finish)
{
	int rv;

	for (size_t rep = 0; rep < 5; rep++) {
		rv = perf_start(perf_mode_trace, 0, (void *)-1);
		if (rv != EOK) {
			TEST_ASSERT_EQUAL_INT(-ENOSYS, rv);
			TEST_IGNORE_MESSAGE("RTT perf target untestable on CI");
		}
		usleep(100);
		TEST_ASSERT_EQUAL_INT(EOK, perf_stop(perf_mode_trace));
		TEST_ASSERT_EQUAL_INT(BUF_SIZE, perf_read(perf_mode_trace, buf, BUF_SIZE, 0));
		TEST_ASSERT_EQUAL_INT(EOK, perf_finish(perf_mode_trace));
	}
}


/*
///////////////////////////////////////////////////////////////////////////////////////////////
*/


TEST_GROUP_RUNNER(perf_test_common)
{
	RUN_TEST_CASE(perf_test_common, invalid_mode);
	RUN_TEST_CASE(perf_test_common, invalid_calls_when_perf_off);
	RUN_TEST_CASE(perf_test_common, start_read_finish);
}


TEST_GROUP_RUNNER(perf_test_trace)
{
	RUN_TEST_CASE(perf_test_trace, trace_start_stop_finish);
}


void runner(void)
{
	RUN_TEST_GROUP(perf_test_common);
	RUN_TEST_GROUP(perf_test_trace);
}


int main(int argc, char *argv[])
{
	return (UnityMain(argc, (const char **)argv, runner) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
