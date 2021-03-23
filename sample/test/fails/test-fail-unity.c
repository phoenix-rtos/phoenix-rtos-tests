#include <stdio.h>

#include "unity_fixture.h"

TEST_GROUP(unity_fail_example);

TEST_SETUP(unity_fail_example)
{
}

TEST_TEAR_DOWN(unity_fail_example)
{
}

TEST(unity_fail_example, example_1)
{
	TEST_ASSERT_TRUE(1 == 0);
}

TEST(unity_fail_example, example_2)
{
	const char* str = "Lorem ipsum";
	TEST_ASSERT_NULL(&str);
}

TEST(unity_fail_example, example_3)
{
	FAIL("Example fail");
}

TEST(unity_fail_example, example_4)
{
	TEST_ASSERT_MESSAGE(1 + 1 == 3, "Additional assertion message");
}

__attribute__((noinline)) static void nested_assertion(void)
{
	FAIL("Fail");
}

TEST(unity_fail_example, example_5)
{
	nested_assertion();

	/* If setjmp is set in the unity config then execution
	 * will not reach these statements */
	puts("Setjmp is not set, a further part of the test will be executed");
	puts("Assertions will not be printed");
	FAIL("Another fail that will be not printed");
}

TEST_GROUP_RUNNER(unity_fail_example)
{
	RUN_TEST_CASE(unity_fail_example, example_1);
	RUN_TEST_CASE(unity_fail_example, example_2);
	RUN_TEST_CASE(unity_fail_example, example_3);
	RUN_TEST_CASE(unity_fail_example, example_4);
	RUN_TEST_CASE(unity_fail_example, example_5);
}

void runner(void)
{
	RUN_TEST_GROUP(unity_fail_example);
}

int main(int argc, char *argv[])
{
	UnityMain(argc, (const char**)argv, runner);
	return 0;
}
