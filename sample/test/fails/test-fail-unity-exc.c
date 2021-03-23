#include <stdio.h>

#include "unity_fixture.h"

TEST_GROUP(unity_fail_exc);

TEST_SETUP(unity_fail_exc)
{
}

TEST_TEAR_DOWN(unity_fail_exc)
{
}

TEST(unity_fail_exc, good)
{
	CHECK(1);
}

TEST(unity_fail_exc, fail)
{
	FAIL("BAD");
}

TEST(unity_fail_exc, exception)
{
	*(int*)(NULL) = 1;
}

TEST_GROUP_RUNNER(unity_fail_exc)
{
	RUN_TEST_CASE(unity_fail_exc, good);
	RUN_TEST_CASE(unity_fail_exc, fail);
	RUN_TEST_CASE(unity_fail_exc, exception);
}

void runner(void)
{
	RUN_TEST_GROUP(unity_fail_exc);
}

int main(int argc, char *argv[])
{
	UnityMain(argc, (const char**)argv, runner);
	return 0;
}
