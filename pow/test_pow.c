#include <math.h>
#include <float.h> /*To have limits of double values*/
#include <errno.h> /*To handle error codes for math functions*/
#include "unity_fixture.h"

TEST_GROUP(test_pow);

TEST_SETUP(test_pow)
{
}

TEST_TEAR_DOWN(test_pow)
{
	errno = 0;
}

TEST(test_pow, tc1_normal_range)
{
	double result = 0;
	double x = 10;
	double y = 2;
	
	result = pow(x,y);
	TEST_ASSERT_EQUAL_INT(0, errno);
	TEST_ASSERT_EQUAL_DOUBLE(100, result);
	
}

TEST(test_pow, tc2_normal_range)
{
	double result = 0;
	double x = 1;
	double y = 0;
	
	result = pow(x,y);
	TEST_ASSERT_EQUAL_INT(0, errno);
	TEST_ASSERT_EQUAL_DOUBLE(1, result);
	
}

TEST(test_pow, tc3_normal_range)
{
	double result = 0;
	double x = 1;
	double y = DBL_MAX;
	
	result = pow(x,y);
	TEST_ASSERT_EQUAL_INT(0, errno);
	TEST_ASSERT_EQUAL_DOUBLE(1, result);
	
	
	result = 0;
	x = 1;
	y = -DBL_MAX;
	
	result = pow(x,y);
	TEST_ASSERT_EQUAL_INT(0, errno);
	TEST_ASSERT_EQUAL_DOUBLE(1, result);
	
}

TEST(test_pow, out_range)
{
	double result = 0;
	double x = DBL_MAX;
	double y = DBL_MAX;
	
	result = pow(x,y);
	TEST_ASSERT_EQUAL_INT(ERANGE, errno);
	TEST_ASSERT_EQUAL_DOUBLE(HUGE_VAL, result);
	
}

TEST(test_pow, tc1_domain_error_expected)
{
	double result = 0;
	double x = 0;
	double y = -10;
	
	result = pow(x,y);
	TEST_ASSERT_EQUAL_INT(EDOM, errno);
}

TEST(test_pow, tc2_domain_error_expected)
{
	double result = 0;
	double x = 0;
	double y = 0;
	
	result = pow(x,y);
	TEST_ASSERT_EQUAL_INT(EDOM, errno);
}

TEST(test_pow, tc3_domain_error_expected)
{
	double result = 0;
	double x = 0;
	double y = -DBL_MAX;
	
	result = pow(x,y);
	TEST_ASSERT_EQUAL_INT(EDOM, errno);
}

TEST(test_pow, tc4_domain_error_expected)
{
	double result = 0;
	double x = -1;
	double y = 0.5;
	
	result = pow(x,y);
	TEST_ASSERT_EQUAL_INT(EDOM, errno);
}

TEST(test_pow, tc5_domain_error_expected)
{
	double result = 0;
	double x = -1;
	double y = DBL_MAX - 0.5;
	
	result = pow(x,y);
	TEST_ASSERT_EQUAL_INT(EDOM, errno);
}

TEST(test_pow, tc6_domain_error_expected)
{
	double result = 0;
	double x = -DBL_MAX;
	double y =  0.5;
	
	result = pow(x,y);
	TEST_ASSERT_EQUAL_INT(EDOM, errno);
}

TEST(test_pow, tc7_domain_error_expected)
{
	double result = 0;
	double x = -1;
	double y =  -DBL_MAX + 0.5;
	
	result = pow(x,y);
	TEST_ASSERT_EQUAL_INT(EDOM, errno);
}



TEST_GROUP_RUNNER(test_pow)
{
	RUN_TEST_CASE(test_pow, tc1_domain_error_expected);
	RUN_TEST_CASE(test_pow, tc2_domain_error_expected);
	RUN_TEST_CASE(test_pow, tc3_domain_error_expected);
	RUN_TEST_CASE(test_pow, tc4_domain_error_expected);
	RUN_TEST_CASE(test_pow, tc5_domain_error_expected);
	RUN_TEST_CASE(test_pow, tc6_domain_error_expected);
	RUN_TEST_CASE(test_pow, tc7_domain_error_expected);
	RUN_TEST_CASE(test_pow, tc1_normal_range);
	RUN_TEST_CASE(test_pow, tc2_normal_range);
	RUN_TEST_CASE(test_pow, tc3_normal_range);
	RUN_TEST_CASE(test_pow, out_range);
}

void runner(void)
{
	RUN_TEST_GROUP(test_pow);
}

int main(int argc, char *argv[])
{
	UnityMain(argc, (const char **)argv, runner);
	return 0;
}
