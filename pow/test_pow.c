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

TEST(test_pow, tc1_base_0_and_exponent_0)
{
	double result = pow(0.0, 0.0);
	TEST_ASSERT_EQUAL_DOUBLE(1.0,  result);
	TEST_ASSERT_EQUAL_INT(0, errno);
	
	result = pow(0.0, -0.0);
	TEST_ASSERT_EQUAL_DOUBLE(1.0,  result);
	TEST_ASSERT_EQUAL_INT(0, errno);
	
	result = pow(-0.0, 0.0);
	TEST_ASSERT_EQUAL_DOUBLE(1.0,  result);
	TEST_ASSERT_EQUAL_INT(0, errno);
	
	result = pow(-0.0, -0.0);
	TEST_ASSERT_EQUAL_DOUBLE(1.0,  result);
	TEST_ASSERT_EQUAL_INT(0, errno);
	
}

TEST(test_pow, tc2_base_0_exponent_less_than_0)
{
	double result = pow(0.0, -1.0);
	TEST_ASSERT_EQUAL_DOUBLE(INFINITY,  result);
	TEST_ASSERT_EQUAL_INT(0, errno);
	
	result = pow(-0.0, -1.0);
	TEST_ASSERT_EQUAL_DOUBLE(-INFINITY,  result);
	TEST_ASSERT_EQUAL_INT(0, errno);
	
}

TEST(test_pow, tc3_base_bigger_than_0_exponent_equal_zero)
{
	double result = pow(1.0, 0.0);
	TEST_ASSERT_EQUAL_INT(0, errno);
	TEST_ASSERT_EQUAL_DOUBLE(1, result);
	
	result = pow(1.0, -0.0);
	TEST_ASSERT_EQUAL_INT(0, errno);
	TEST_ASSERT_EQUAL_DOUBLE(1, result);
	
	result = pow(DBL_MAX, 0.0);
	TEST_ASSERT_EQUAL_INT(0, errno);
	TEST_ASSERT_EQUAL_DOUBLE(1, result);
	
	result = pow(-DBL_MAX, 0.0);
	TEST_ASSERT_EQUAL_INT(0, errno);
	TEST_ASSERT_EQUAL_DOUBLE(1, result);
	
	
}

TEST(test_pow, tc4_domain_error)
{
	//double result = pow(-1.0, 0.5);
	
	//TEST_ASSERT_EQUAL_INT(EDOM, errno);
	//TEST_ASSERT_EQUAL_DOUBLE(NAN, result);
	
	
	//result = pow(-DBL_MAX, -INT_MAX-0.5);
	//TEST_ASSERT_EQUAL_INT(EDOM, errno);
	//TEST_ASSERT_EQUAL_DOUBLE(NAN, result);
}


TEST(test_pow, tc5_negative_and_positive_exponent_intiger)
{
	double result = pow(-1.0, 3);
	TEST_ASSERT_EQUAL_INT(0, errno);
	TEST_ASSERT_EQUAL_DOUBLE(-1.0, result);
	
	result = pow(-1.0, 2);
	TEST_ASSERT_EQUAL_INT(0, errno);
	TEST_ASSERT_EQUAL_DOUBLE(1.0, result);
}


TEST(test_pow, tc6_exponent_max_value)
{
	
	double result = pow(1.0, INT_MAX);
	TEST_ASSERT_EQUAL_INT(0, errno);
	TEST_ASSERT_EQUAL_DOUBLE(1, result);
	
	result = pow(1.0, INT_MIN);
	TEST_ASSERT_EQUAL_INT(0, errno);
	TEST_ASSERT_EQUAL_DOUBLE(1, result);
	
	//result = pow(2.0, INT_MAX);
	//TEST_ASSERT_EQUAL_INT(ERANGE, errno);
	//TEST_ASSERT_EQUAL_DOUBLE(INFINITY, result);
	
	//result = pow(2.0, INT_MIN);
	//TEST_ASSERT_EQUAL_INT(ERANGE, errno);
	//TEST_ASSERT_EQUAL_DOUBLE(0, result);
}

TEST(test_pow, tc7_exponent_bigger_than_max_value)
{	
	double result = pow(1.0, INT_MAX+1);
	TEST_ASSERT_EQUAL_INT(0, errno);
	TEST_ASSERT_EQUAL_DOUBLE(1, result);
}

TEST(test_pow, tc8_normal_range)
{	
	double result = pow(10.0, 2);
	TEST_ASSERT_EQUAL_INT(0, errno);
	TEST_ASSERT_EQUAL_DOUBLE(100.0, result);
	
	result = pow(2, 2);
	TEST_ASSERT_EQUAL_INT(0, errno);
	TEST_ASSERT_EQUAL_DOUBLE(4.0, result);
	
	result = pow(2, 3);
	TEST_ASSERT_EQUAL_INT(0, errno);
	TEST_ASSERT_EQUAL_DOUBLE(8.0, result);
	
	result = pow(10.0, 2);
	TEST_ASSERT_EQUAL_INT(0, errno);
	TEST_ASSERT_EQUAL_DOUBLE(100.0, result);
	
	result = pow(10.0, 2);
	TEST_ASSERT_EQUAL_INT(0, errno);
	TEST_ASSERT_EQUAL_DOUBLE(100.0, result);
}


TEST_GROUP_RUNNER(test_pow)
{
	RUN_TEST_CASE(test_pow, tc1_base_0_and_exponent_0);
	RUN_TEST_CASE(test_pow, tc2_base_0_exponent_less_than_0);
	RUN_TEST_CASE(test_pow, tc3_base_bigger_than_0_exponent_equal_zero);
	RUN_TEST_CASE(test_pow, tc4_domain_error);
	RUN_TEST_CASE(test_pow, tc5_negative_and_positive_exponent_intiger);
	RUN_TEST_CASE(test_pow, tc6_exponent_max_value);
	RUN_TEST_CASE(test_pow, tc7_exponent_bigger_than_max_value);
	RUN_TEST_CASE(test_pow, tc8_normal_range);
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
