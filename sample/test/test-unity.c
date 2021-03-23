#include <string.h>

#include "unity_fixture.h"

TEST_GROUP(unity_example);

TEST_SETUP(unity_example)
{
}

TEST_TEAR_DOWN(unity_example)
{
}

TEST(unity_example, example_1)
{
	void *p = NULL;

	TEST_ASSERT(1 + 1 == 2);
	TEST_ASSERT_TRUE(0 == 0);

	TEST_ASSERT_NULL(p);
	TEST_ASSERT_NOT_NULL(&p);
	TEST_ASSERT_EQUAL_PTR(p, NULL);
}

TEST(unity_example, example_2)
{
	const char *str1 = "Lorem ipsum";
	char str2[12] = {0};

	strcpy(str2, str1);
	TEST_ASSERT_EQUAL_STRING(str1, str2);

	str2[6] = 'I';
	TEST_ASSERT_EQUAL_STRING_LEN(str1, str2, 6);
}

TEST(unity_example, example_3)
{
	TEST_PASS_MESSAGE("test example_3 succeeded!");
}

IGNORE_TEST(unity_example, example_4)
{
	FAIL("this test is ignored so it shouldn't fail");
}

TEST_GROUP_RUNNER(unity_example)
{
	RUN_TEST_CASE(unity_example, example_1);
	RUN_TEST_CASE(unity_example, example_2);
	RUN_TEST_CASE(unity_example, example_3);
	RUN_TEST_CASE(unity_example, example_4);
}

void runner(void)
{
	RUN_TEST_GROUP(unity_example);
}

int main(int argc, char *argv[])
{
	UnityMain(argc, (const char**)argv, runner);
	return 0;
}
