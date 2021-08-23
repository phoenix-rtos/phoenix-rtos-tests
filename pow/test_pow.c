#include <float.h> /*To have limits of double values*/
#include <errno.h> /*To handle error codes for math functions*/
#include "unity_fixture.h"
#include "math.h"
//#include "common.h"

TEST_GROUP(test_pow);

TEST_SETUP(test_pow)
{
}

TEST_TEAR_DOWN(test_pow)
{
	errno = 0;
}

TEST(test_pow, tc1_normal_range_base_0_and_exponent_0)
{
	double expected = 1;
	double result = pow(0.0, 0.0);
	TEST_ASSERT_EQUAL_DOUBLE(expected,  result);
	TEST_ASSERT_EQUAL_INT(0, errno);
	
}

TEST(test_pow, tc1_1_normal_range_base_0_exponent_less_than_0)
{
	double expected = INFINITY;
	double result = pow(0.0, -1.0);
	TEST_ASSERT_EQUAL_DOUBLE(expected,  result);
	TEST_ASSERT_EQUAL_INT(0, errno);
	
}

TEST(test_pow, tc2_normal_range_base_bigger_than_0_exponent_equal_zero)
{
	double expected = 1.0;
	double result = pow(1.0, 0.0);
	
	TEST_ASSERT_EQUAL_INT(0, errno);
	TEST_ASSERT_EQUAL_DOUBLE(expected, result);
	
}

TEST(test_pow, tc3_normal_range_base_equal_1_exponent_double_max)
{
	double expected = 1.0;
	double result = pow(1.0, DBL_MAX);
	
	TEST_ASSERT_EQUAL_INT(0, errno);
	TEST_ASSERT_EQUAL_DOUBLE(expected, result);
	
	
	result = pow(1.0, -DBL_MAX);
	
	TEST_ASSERT_EQUAL_INT(0, errno);
	TEST_ASSERT_EQUAL_DOUBLE(expected, result);
	
}

/*
TEST(test_pow, tc1_domain_error_expected)
{
	double expected = INFINITY;
	double result = pow(0.0, -10.0);
	
	TEST_ASSERT_EQUAL_INT(EDOM, errno);
	TEST_ASSERT_EQUAL_DOUBLE(expected, result);
}


TEST(test_pow, tc3_domain_error_expected)
{
	
	double result = pow(0.0, -DBL_MAX);
	TEST_ASSERT_EQUAL_INT(EDOM, errno);
}

TEST(test_pow, tc4_domain_error_expected)
{	
	double result = pow(-1.0, 0.5);
	TEST_ASSERT_EQUAL_INT(EDOM, errno);
}

TEST(test_pow, tc5_domain_error_expected)
{
	double result = pow(-1.0 , DBL_MAX-0.5);
	TEST_ASSERT_EQUAL_INT(EDOM, errno);
}

TEST(test_pow, tc6_domain_error_expected)
{	
	double result = pow(-DBL_MAX, 0.5);
	TEST_ASSERT_EQUAL_INT(EDOM, errno);
}

TEST(test_pow, tc7_domain_error_expected)
	
	double result = pow(-1.0, -DBL_MAX + 0.5);
	TEST_ASSERT_EQUAL_INT(EDOM, errno);
}
*/

TEST_GROUP_RUNNER(test_pow)
{
	//RUN_TEST_CASE(test_pow, tc1_domain_error_expected);
	//RUN_TEST_CASE(test_pow, tc3_domain_error_expected);
	//RUN_TEST_CASE(test_pow, tc4_domain_error_expected);
	//RUN_TEST_CASE(test_pow, tc5_domain_error_expected);
	//RUN_TEST_CASE(test_pow, tc6_domain_error_expected);
	//RUN_TEST_CASE(test_pow, tc7_domain_error_expected);
	RUN_TEST_CASE(test_pow, tc1_normal_range_base_0_and_exponent_0);
	RUN_TEST_CASE(test_pow, tc1_1_normal_range_base_0_exponent_less_than_0);
	RUN_TEST_CASE(test_pow, tc2_normal_range_base_bigger_than_0_exponent_equal_zero);
	RUN_TEST_CASE(test_pow, tc3_normal_range_base_equal_1_exponent_double_max);
//	RUN_TEST_CASE(test_pow, out_range_base_and_exponend_equal_DBL_MAX);
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
