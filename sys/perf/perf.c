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


static void emit_kernel_events(void)
{
	for (size_t rep = 0; rep < 100; rep++) {
		usleep(1);
	}
}


static void read_events(perf_mode_t mode, size_t nchans)
{
	size_t total = 0;
	int rv;
	for (size_t chan = 0; chan < nchans; chan++) {
		rv = perf_read(mode, buf, BUF_SIZE, chan);
		TEST_ASSERT_GREATER_OR_EQUAL_INT(0, rv);
		total += rv;
	}
	TEST_ASSERT_GREATER_THAN_size_t(0, total);
}


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
	TEST_ASSERT_EQUAL_INT(-ENOSYS, perf_start(-1, 0, NULL, 0));
	TEST_ASSERT_EQUAL_INT(-ENOSYS, perf_read(-1, buf, BUF_SIZE, 0));
	TEST_ASSERT_EQUAL_INT(-ENOSYS, perf_stop(-1));
	TEST_ASSERT_EQUAL_INT(-ENOSYS, perf_finish(-1));
}


TEST(perf_test_common, invalid_calls_when_perf_off)
{
	for (perf_mode_t mode = 0; mode < perf_mode_count; mode++) {
		TEST_ASSERT_EQUAL_INT(-EINVAL, perf_read(mode, buf, BUF_SIZE, 0));

		/* may be -EINVAL or -ENOSYS depending whether mode supports perf_stop */
		TEST_ASSERT_LESS_THAN_INT(0, perf_stop(mode));

		TEST_ASSERT_EQUAL_INT(-EINVAL, perf_finish(mode));
	}
}


TEST(perf_test_common, start_read_finish)
{
	int rv;

	for (perf_mode_t mode = 0; mode < perf_mode_count; mode++) {
		for (size_t rep = 0; rep < 5; rep++) {
			rv = perf_start(mode, 0, NULL, 0);
			if (rv <= 0) {
				TEST_ASSERT_EQUAL_INT(-ENOSYS, rv);
				break;
			}
			else {
				emit_kernel_events();
				read_events(mode, rv);
				TEST_ASSERT_EQUAL_INT(EOK, perf_finish(mode));
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
		rv = perf_start(perf_mode_trace, 0, NULL, 0);
		if (rv <= 0) {
			TEST_ASSERT_EQUAL_INT(-ENOSYS, rv);
			TEST_IGNORE_MESSAGE("RTT perf target untestable on CI");
		}
		usleep(100);
		emit_kernel_events();
		TEST_ASSERT_GREATER_THAN_INT(0, perf_stop(perf_mode_trace));
		read_events(perf_mode_trace, rv);
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
