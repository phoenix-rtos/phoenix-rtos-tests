/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - stdlib.h
 *
 * TESTED:
 *    - strtod()
 *    - strtof()
 *    - strtold()
 *    - strtol()
 *    - strtoll()
 *    - strtoul()
 *    - strtoull()
 *
 * Copyright 2023 Phoenix Systems
 * Author: Dawid Szpejna, Mateusz Bloch
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <stdlib.h>
#include <float.h>
#include <math.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <unity_fixture.h>


TEST_GROUP(stdlib_strto);


TEST_SETUP(stdlib_strto)
{
}


TEST_TEAR_DOWN(stdlib_strto)
{
}


TEST(stdlib_strto, strtod_basic)
{
	char *end;
	const char *str[] = { "0", "-0", "-1", "1", "+1", ".1", "3.1415", "-3.1415", "0.1234567891234567",
		"1e0", "1e+0", "1e-0", "1e1", "-.75e+8", "-.75E+8", "3.14E+3", "3.14E-2", "3.14e+3", "-3.14e-2" };
	const double expected[] = { 0, 0, -1, 1, 1, 0.1, 3.1415, -3.1415, 0.1234567891234567, 1.0, 1.0, 1.0, 10.0,
		-.75e+8, -.75E+8, 3140.0, 0.0314, 3140, -0.0314 };

	for (int i = 0; i < (sizeof(str) / sizeof(str[0])); i++) {
		errno = 0;
		end = NULL;
		TEST_ASSERT_EQUAL_DOUBLE(expected[i], strtod(str[i], &end));
		TEST_ASSERT_EQUAL_PTR(str[i] + strlen(str[i]), end);
		TEST_ASSERT_EQUAL_INT(0, errno);
	}
}


TEST(stdlib_strto, strtod_basic_hex)
{
/* Disabled because of #703 issue: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/703 */
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#703 issue");
#endif
	char *end;
	const char *str[] = { "0X0.0P+0", "0X1.0P+0", "-0X1.0P+0", "0X1.0P-126", "0X1.FFFFFEP+127", "-0X1.0P-126", "-0X1.FFFFFEP+127" };
	const double expected[] = { 0.0, 1, -1, FLT_MIN, FLT_MAX, -FLT_MIN, -FLT_MAX };

	for (int i = 0; i < (sizeof(str) / sizeof(str[0])); i++) {
		errno = 0;
		end = NULL;
		TEST_ASSERT_EQUAL_DOUBLE(expected[i], strtod(str[i], &end));
		TEST_ASSERT_EQUAL_PTR(str[i] + strlen(str[i]), end);
		TEST_ASSERT_EQUAL_INT(0, errno);
	}
}


TEST(stdlib_strto, strtod_min_max)
{
	errno = 0;
	TEST_ASSERT_EQUAL_DOUBLE(DBL_MIN, strtod("2.2250738585072013e-308", NULL));
	TEST_ASSERT_EQUAL_INT(0, errno);

	errno = 0;
	TEST_ASSERT_EQUAL_DOUBLE(DBL_MAX, strtod("1.797693134862315e+308", NULL));
	TEST_ASSERT_EQUAL_INT(0, errno);

/* Disabled because of #703 issue: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/703 */
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#703 issue");
#endif

	errno = 0;
	TEST_ASSERT_EQUAL_DOUBLE(DBL_MIN, strtod("0x1p-1022", NULL));
	TEST_ASSERT_EQUAL_INT(0, errno);

	errno = 0;
	TEST_ASSERT_EQUAL_DOUBLE(DBL_MAX, strtod("0x1.fffffffffffffp+1023", NULL));
	TEST_ASSERT_EQUAL_INT(0, errno);
}


TEST(stdlib_strto, strtof_basic)
{
	char *end;
	const char *str[] = { "0", "-0", "-1", "1", "+1", ".1", "3.1415", "-3.1415", "0.1234567891234567",
		"1e0", "1e+0", "1e-0", "1e1", "-.75e+8", "-.75E+8", "3.14E+3", "3.14E-2", "3.14e+3", "-3.14e-2" };
	const float expected[] = { 0, 0, -1, 1, 1, 0.1, 3.1415, -3.1415, 0.1234567891234567, 1.0, 1.0, 1.0, 10.0,
		-.75e+8, -.75E+8, 3140.0, 0.0314, 3140, -0.0314 };

	for (int i = 0; i < (sizeof(str) / sizeof(str[0])); i++) {
		errno = 0;
		end = NULL;
		TEST_ASSERT_EQUAL_FLOAT(expected[i], strtof(str[i], &end));
		TEST_ASSERT_EQUAL_PTR(str[i] + strlen(str[i]), end);
		TEST_ASSERT_EQUAL_INT(0, errno);
	}
}


TEST(stdlib_strto, strtof_basic_hex)
{
/* Disabled because of #703 issue: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/703 */
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#703 issue");
#endif
	char *end;
	const char *str[] = { "0X0.0P+0", "0X1.0P+0", "-0X1.0P+0", "0x1.81cd6e631f8a1p+13", "-0x1.81cd6e631f8a1p+13" };
	const float expected[] = { 0.0, 1, -1, 12345.67890, -12345.67890 };

	for (int i = 0; i < (sizeof(str) / sizeof(str[0])); i++) {
		errno = 0;
		end = NULL;
		TEST_ASSERT_EQUAL_FLOAT(expected[i], strtof(str[i], &end));
		TEST_ASSERT_EQUAL_PTR(str[i] + strlen(str[i]), end);
		TEST_ASSERT_EQUAL_INT(0, errno);
	}
}


TEST(stdlib_strto, strtof_min_max)
{
	errno = 0;
	TEST_ASSERT_EQUAL_FLOAT(FLT_MIN, strtof("1.17549435e-38", NULL));
	TEST_ASSERT_EQUAL_INT(0, errno);

	errno = 0;
	TEST_ASSERT_EQUAL_FLOAT(FLT_MAX, strtof("3.40282347e+38", NULL));
	TEST_ASSERT_EQUAL_INT(0, errno);

/* Disabled because of #703 issue: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/703 */
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#703 issue");
#endif

	errno = 0;
	TEST_ASSERT_EQUAL_FLOAT(FLT_MIN, strtof("0X1.0P-126", NULL));
	TEST_ASSERT_EQUAL_INT(0, errno);

	errno = 0;
	TEST_ASSERT_EQUAL_FLOAT(FLT_MAX, strtof("0X1.FFFFFEP+127", NULL));
	TEST_ASSERT_EQUAL_INT(0, errno);
}


TEST(stdlib_strto, strtold_basic)
{
	char *end;
	const char *str[] = { "0", "-0", "-1", "1", "+1", ".1", "3.1415", "-3.1415", "0.1234567891234567",
		"1e0", "1e+0", "1e-0", "1e1", "-.75e+8", "-.75E+8", "3.14E+3", "3.14E-2", "3.14e+3", "-3.14e-2" };
	const long double expected[] = { 0, 0, -1, 1, 1, 0.1, 3.1415, -3.1415, 0.1234567891234567, 1.0, 1.0, 1.0, 10.0,
		-.75e+8, -.75E+8, 3140.0, 0.0314, 3140, -0.0314 };

	for (int i = 0; i < (sizeof(str) / sizeof(str[0])); i++) {
		errno = 0;
		end = NULL;
		TEST_ASSERT_EQUAL_DOUBLE(expected[i], strtold(str[i], &end));
		TEST_ASSERT_EQUAL_PTR(str[i] + strlen(str[i]), end);
		TEST_ASSERT_EQUAL_INT(0, errno);
	}
}


TEST(stdlib_strto, strtold_basic_hex)
{
/* Disabled because of #703 issue: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/703 */
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#703 issue");
#endif
	char *end;
	const char *str[] = { "0X0.0P+0", "0X1.0P+0", "-0X1.0P+0", "0X1.0P-126", "0X1.FFFFFEP+127", "-0X1.0P-126", "-0X1.FFFFFEP+127", "0x1p-1022", "0x1.fffffffffffffp+1023" };
	const long double expected[] = { 0.0, 1, -1, FLT_MIN, FLT_MAX, -FLT_MIN, -FLT_MAX, DBL_MIN, DBL_MAX };

	for (int i = 0; i < (sizeof(str) / sizeof(str[0])); i++) {
		errno = 0;
		end = NULL;
		TEST_ASSERT_EQUAL_DOUBLE(expected[i], strtold(str[i], &end));
		TEST_ASSERT_EQUAL_PTR(str[i] + strlen(str[i]), end);
		TEST_ASSERT_EQUAL_INT(0, errno);
	}
}


TEST(stdlib_strto, strtold_min_max)
{
	/*
	 * Phoenix-RTOS does not currently support long double numbers.
	 * Consequently, long doubles are not being tested, instead, for this case, we have decided to test doubles
	 * https://github.com/phoenix-rtos/phoenix-rtos-tests/issues/219
	 */
	errno = 0;
	TEST_ASSERT_EQUAL_DOUBLE(DBL_MIN, strtold("2.2250738585072013e-308", NULL));
	TEST_ASSERT_EQUAL_INT(0, errno);

	errno = 0;
	TEST_ASSERT_EQUAL_DOUBLE(DBL_MAX, strtold("1.797693134862315e+308", NULL));
	TEST_ASSERT_EQUAL_INT(0, errno);

/* Disabled because of #703 issue: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/703 */
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#703 issue");
#endif

	errno = 0;
	TEST_ASSERT_EQUAL_DOUBLE(DBL_MIN, strtold("0x1p-1022", NULL));
	TEST_ASSERT_EQUAL_INT(0, errno);

	errno = 0;
	TEST_ASSERT_EQUAL_DOUBLE(DBL_MAX, strtold("0x1.fffffffffffffp+1023", NULL));
	TEST_ASSERT_EQUAL_INT(0, errno);
}


TEST(stdlib_strto, flt_dec_notation)
{
	const char floatMinString[] = "0.000000000000000000000000000000000000011754943508222875";
	const char floatMaxString[] = "340282346638528859811704183484516925440.000000";

	const char numString[] = "340282346638528859811704183484516925440.3402823466385288598117041834845169254401175494350822287575";

	const float num = 340282346638528859811704183484516925440.3402823466385288598117041834845169254401175494350822287575;
	const float num1 = 11704183484516925440.3402823466385288598117041834845169254401175494350822287575;

	errno = 0;
	TEST_ASSERT_EQUAL_FLOAT(FLT_MIN, strtof(floatMinString, NULL));
	TEST_ASSERT_EQUAL_INT(0, errno);

	errno = 0;
	TEST_ASSERT_EQUAL_FLOAT(FLT_MAX, strtof(floatMaxString, NULL));
	TEST_ASSERT_EQUAL_INT(0, errno);

	errno = 0;
	TEST_ASSERT_EQUAL_FLOAT(num, strtof(numString, NULL));
	TEST_ASSERT_EQUAL_INT(0, errno);

	errno = 0;
	TEST_ASSERT_EQUAL_FLOAT(num1, strtof(&numString[19], NULL));
	TEST_ASSERT_EQUAL_INT(0, errno);
}


TEST(stdlib_strto, dbl_dec_notation)
{

	const char numString[] =
		"17976931348623157081452742373170435679807056752584499659891747680315"
		"72607800285387605895586327668781715404589535143824642343213268894641"
		"82768467546703537516986049910576551282076245490090389328944075868508"
		"45513394230458323690322294816580855933212334827479782620414472316873"
		"817718091929988125040402618412485836.1797693134862315708145274237317"
		"04356798070567525844996598917476803157260780028538760589558632766878"
		"17154045895351438246423432132688946418276846754670353751698604991057"
		"65512820762454900903893289440758685084551339423045832369032229481658"
		"08559332123348274797826204144723168738177180919299881250404026184124"
		"85836179769313486231570814527423731704356798070567525844996598917476"
		"80315726078002853876058955863276687817154045895351438246423432132688"
		"94641827684675467035375169860499105765512820762454900903893289440758"
		"68508455133942304583236903222948165808559332123348274797826204144723"
		"16873817718091929988125040402618412485836179769313486231570814527423"
		"73170435679807056752584499659891747680315726078002853876058955863276"
		"68781715404589535143824642343213268894641827684675467035375169860499"
		"10576551282076245490090389328944075868508455133942304583236903222948"
		"16580855933212334827479782620414472316873817718091929988125040402618"
		"41248583617976931348623157081452742373170435679807056752584499659891"
		"74768031572607800285387605895586327668781715404589535143824642343213";

	char buff[310] = { 0 };

	const double num = 17976931348623157081452742373170435679807056752584499659891747680315726078002853876058955863276687817154045895351438246423432132688946418276846754670353751698604991057655128207624549009038932894407586850845513394230458323690322294816580855933212334827479782620414472316873817718091929988125040402618412485836.1797693134862315708145274237317043567980705675258449965989174768031572607800285387605895586327668781715404589535143824642343213268894641827684675467035375169860499105765512820762454900903893289440758685084551339423045832369032229481658085593321233482747978262041447231687381771809192998812504040261841248583617976931348623157081452742373170435679807056752584499659891747680315726078002853876058955863276687817154045895351438246423432132688946418276846754670353751698604991057655128207624549009038932894407586850845513394230458323690322294816580855933212334827479782620414472316873817718091929988125040402618412485836179769313486231570814527423731704356798070567525844996598917476803157260780028538760589558632766878171540458953514382464234321326889464182768467546703537516986049910576551282076245490090389328944075868508455133942304583236903222948165808559332123348274797826204144723168738177180919299881250404026184124858361797693134862315708145274237317043567980705675258449965989174768031572607800285387605895586327668781715404589535143824642343213;

	const double num1 = 8125040402618412485836.1797693134862315708145274237317043567980705675258449965989174768031572607800285387605895586327668781715404589535143824642343213268894641827684675467035375169860499105765512820762454900903893289440758685084551339423045832369032229481658085593321233482747978262041447231687381771809192998812504040261841248583617976931348623157081452742373170435679807056752584499659891747680315726078002853876058955863276687817154045895351438246423432132688946418276846754670353751698604991057655128207624549009038932894407586850845513394230458323690322294816580855933212334827479782620414472316873817718091929988125040402618412485836179769313486231570814527423731704356798070567525844996598917476803157260780028538760589558632766878171540458953514382464234321326889464182768467546703537516986049910576551282076245490090389328944075868508455133942304583236903222948165808559332123348274797826204144723168738177180919299881250404026184124858361797693134862315708145274237317043567980705675258449965989174768031572607800285387605895586327668781715404589535143824642343213;

	const double num2 = 17976931348623157081452742373170435679807056752584499659891747680315726078002853876058955863276687817154045895351438246423432132688946418276846754670353751698604991057655128207624549009038932894407586850845513394230458323690322294816580855933212334827479782620414472316873817718091929988125040402618412485836.17976931348623157081452742373;

	errno = 0;
	TEST_ASSERT_EQUAL_DOUBLE(num, strtod(numString, NULL));
	TEST_ASSERT_EQUAL_INT(0, errno);

	errno = 0;
	TEST_ASSERT_EQUAL_DOUBLE(num, strtold(numString, NULL));
	TEST_ASSERT_EQUAL_INT(0, errno);

	errno = 0;
	TEST_ASSERT_EQUAL_DOUBLE(num1, strtod(&numString[286], NULL));
	TEST_ASSERT_EQUAL_INT(0, errno);

	errno = 0;
	TEST_ASSERT_EQUAL_DOUBLE(num1, strtold(&numString[286], NULL));
	TEST_ASSERT_EQUAL_INT(0, errno);

	memcpy(buff, numString, 308);

	errno = 0;
	TEST_ASSERT_EQUAL_DOUBLE(num2, strtod(buff, NULL));
	TEST_ASSERT_EQUAL_INT(0, errno);

	errno = 0;
	TEST_ASSERT_EQUAL_DOUBLE(num2, strtold(buff, NULL));
	TEST_ASSERT_EQUAL_INT(0, errno);
}


TEST(stdlib_strto, strtol_basic)
{
	char *end;
	const char *str[] = { "0", "1", "-1234567890", "1234567890", "2147483647", "-2147483648" };
	const long expected[] = { 0, 1, -1234567890, 1234567890, INT_MAX, INT_MIN };

	for (int i = 0; i < (sizeof(str) / sizeof(str[0])); i++) {
		errno = 0;
		end = NULL;
		TEST_ASSERT_EQUAL_INT(expected[i], strtol(str[i], &end, 10));
		TEST_ASSERT_EQUAL_PTR(str[i] + strlen(str[i]), end);
		TEST_ASSERT_EQUAL_INT(0, errno);
	}
}


TEST(stdlib_strto, strtol_basic_binary)
{
	char *end;
	const char *str[] = { "0", "1", "10011010010", "-10011010010", "1111111111111111111111111111111", "-10000000000000000000000000000000" };
	const long expected[] = { 0, 1, 1234, -1234, INT_MAX, INT_MIN };

	for (int i = 0; i < (sizeof(str) / sizeof(str[0])); i++) {
		errno = 0;
		end = NULL;
		TEST_ASSERT_EQUAL_INT(expected[i], strtol(str[i], &end, 2));
		TEST_ASSERT_EQUAL_PTR(str[i] + strlen(str[i]), end);
		TEST_ASSERT_EQUAL_INT(0, errno);
	}
}


TEST(stdlib_strto, strtol_basic_hex)
{
	char *end;
	const char *str[] = { "0x0", "0x1", "-0x499602D2", "0x499602D2", "0x7FFFFFFF", "-0x80000000" };
	const long expected[] = { 0, 1, -1234567890, 1234567890, INT_MAX, INT_MIN };

	for (int i = 0; i < (sizeof(str) / sizeof(str[0])); i++) {
		errno = 0;
		end = NULL;
		TEST_ASSERT_EQUAL_INT(expected[i], strtol(str[i], &end, 16));
		TEST_ASSERT_EQUAL_PTR(str[i] + strlen(str[i]), end);
		TEST_ASSERT_EQUAL_INT(0, errno);
	}
}


TEST(stdlib_strto, strtol_other_bases)
{
	char *end;
	errno = 0;
	const char *str[] = { "1333333333333333", "17777777777", "553032005531", "4bb2308a7", "1652ca931", "b5gge57", "1vvvvvv", "zik0zj" };
	const int base[] = { 4, 8, 6, 12, 14, 24, 32, 36 };

	for (int i = 0; i < (sizeof(str) / sizeof(str[0])); i++) {
		errno = 0;
		end = NULL;
		TEST_ASSERT_EQUAL_INT(INT_MAX, strtol(str[i], &end, base[i]));
		TEST_ASSERT_EQUAL_PTR(str[i] + strlen(str[i]), end);
		TEST_ASSERT_EQUAL_INT(0, errno);
	}
}


TEST(stdlib_strto, strtol_other_bases_neg_values)
{
	char *end;
	errno = 0;
	const char *str[] = { "-2000000000000000", "-20000000000", "-553032005532", "-4bb2308a8", "-1652ca932", "-b5gge58", "-2000000", "-zik0zk" };
	const int base[] = { 4, 8, 6, 12, 14, 24, 32, 36 };

	for (int i = 0; i < (sizeof(str) / sizeof(str[0])); i++) {
		errno = 0;
		end = NULL;
		TEST_ASSERT_EQUAL_INT(INT_MIN, strtol(str[i], &end, base[i]));
		TEST_ASSERT_EQUAL_PTR(str[i] + strlen(str[i]), end);
		TEST_ASSERT_EQUAL_INT(0, errno);
	}
}


TEST(stdlib_strto, strtol_min_max)
{
	char str[50];

	errno = 0;
	sprintf(str, "%ld", LONG_MIN);
	TEST_ASSERT_EQUAL_INT(LONG_MIN, strtol(str, NULL, 10));
	TEST_ASSERT_EQUAL_INT(0, errno);

	errno = 0;
	sprintf(str, "%ld", LONG_MAX);
	TEST_ASSERT_EQUAL_INT(LONG_MAX, strtol(str, NULL, 10));
	TEST_ASSERT_EQUAL_INT(0, errno);

	errno = 0;
	sprintf(str, "%lx", LONG_MAX);
	TEST_ASSERT_EQUAL_INT(LONG_MAX, strtol(str, NULL, 16));
	TEST_ASSERT_EQUAL_INT(0, errno);
}


TEST(stdlib_strto, strtoll_basic)
{
	char *end;
	const char *str[] = { "0", "1", "-1234567890", "1234567890", "2147483647", "-2147483648" };
	const long long expected[] = { 0, 1, -1234567890, 1234567890, INT_MAX, INT_MIN };

	for (int i = 0; i < (sizeof(str) / sizeof(str[0])); i++) {
		errno = 0;
		end = NULL;
		TEST_ASSERT_EQUAL_INT(expected[i], strtoll(str[i], &end, 10));
		TEST_ASSERT_EQUAL_PTR(str[i] + strlen(str[i]), end);
		TEST_ASSERT_EQUAL_INT(0, errno);
	}
}


TEST(stdlib_strto, strtoll_basic_binary)
{
	char *end;
	const char *str[] = { "0", "1", "10011010010", "-10011010010", "1111111111111111111111111111111", "-10000000000000000000000000000000" };
	const long long expected[] = { 0, 1, 1234, -1234, INT_MAX, INT_MIN };

	for (int i = 0; i < (sizeof(str) / sizeof(str[0])); i++) {
		errno = 0;
		end = NULL;
		TEST_ASSERT_EQUAL_INT(expected[i], strtoll(str[i], &end, 2));
		TEST_ASSERT_EQUAL_PTR(str[i] + strlen(str[i]), end);
		TEST_ASSERT_EQUAL_INT(0, errno);
	}
}


TEST(stdlib_strto, strtoll_basic_hex)
{
	char *end;
	const char *str[] = { "0x0", "0x1", "-0x499602D2", "0x499602D2", "0x7FFFFFFF", "-0x80000000" };
	const long long expected[] = { 0, 1, -1234567890, 1234567890, INT_MAX, INT_MIN };

	for (int i = 0; i < (sizeof(str) / sizeof(str[0])); i++) {
		errno = 0;
		end = NULL;
		TEST_ASSERT_EQUAL_INT(expected[i], strtoll(str[i], &end, 16));
		TEST_ASSERT_EQUAL_PTR(str[i] + strlen(str[i]), end);
		TEST_ASSERT_EQUAL_INT(0, errno);
	}
}


TEST(stdlib_strto, strtoll_other_bases)
{
	char *end;
	errno = 0;
	const char *str[] = { "1333333333333333", "17777777777", "553032005531", "4bb2308a7", "1652ca931", "b5gge57", "1vvvvvv", "zik0zj" };
	const int base[] = { 4, 8, 6, 12, 14, 24, 32, 36 };

	for (int i = 0; i < (sizeof(str) / sizeof(str[0])); i++) {
		errno = 0;
		end = NULL;
		TEST_ASSERT_EQUAL_INT(INT_MAX, strtoll(str[i], &end, base[i]));
		TEST_ASSERT_EQUAL_PTR(str[i] + strlen(str[i]), end);
		TEST_ASSERT_EQUAL_INT(0, errno);
	}
}


TEST(stdlib_strto, strtoll_other_bases_neg_values)
{
	char *end;
	errno = 0;
	const char *str[] = { "-2000000000000000", "-20000000000", "-553032005532", "-4bb2308a8", "-1652ca932", "-b5gge58", "-2000000", "-zik0zk" };
	const int base[] = { 4, 8, 6, 12, 14, 24, 32, 36 };

	for (int i = 0; i < (sizeof(str) / sizeof(str[0])); i++) {
		errno = 0;
		end = NULL;
		TEST_ASSERT_EQUAL_INT(INT_MIN, strtoll(str[i], &end, base[i]));
		TEST_ASSERT_EQUAL_PTR(str[i] + strlen(str[i]), end);
		TEST_ASSERT_EQUAL_INT(0, errno);
	}
}


TEST(stdlib_strto, strtoll_min_max)
{
	char str[50];

	errno = 0;
	sprintf(str, "%lld", LLONG_MIN);
	TEST_ASSERT_EQUAL_INT(LLONG_MIN, strtoll(str, NULL, 10));
	TEST_ASSERT_EQUAL_INT(0, errno);

	errno = 0;
	sprintf(str, "%lld", LLONG_MAX);
	TEST_ASSERT_EQUAL_INT(LLONG_MAX, strtoll(str, NULL, 10));
	TEST_ASSERT_EQUAL_INT(0, errno);

	errno = 0;
	sprintf(str, "%llx", LLONG_MAX);
	TEST_ASSERT_EQUAL_INT(LLONG_MAX, strtoll(str, NULL, 16));
	TEST_ASSERT_EQUAL_INT(0, errno);
}


TEST(stdlib_strto, strtoul_basic)
{
	char *end;
	const char *str[] = { "0", "1", "-1234567890", "1234567890", "2147483647", "-2147483648" };
	const unsigned long expected[] = { 0, 1, -1234567890, 1234567890, INT_MAX, INT_MIN };

	for (int i = 0; i < (sizeof(str) / sizeof(str[0])); i++) {
		errno = 0;
		end = NULL;
		TEST_ASSERT_EQUAL_INT(expected[i], strtoul(str[i], &end, 10));
		TEST_ASSERT_EQUAL_PTR(str[i] + strlen(str[i]), end);
		TEST_ASSERT_EQUAL_INT(0, errno);
	}
}


TEST(stdlib_strto, strtoul_basic_binary)
{
	char *end;
	const char *str[] = { "0", "1", "10011010010", "-10011010010", "1111111111111111111111111111111", "-10000000000000000000000000000000" };
	const unsigned long expected[] = { 0, 1, 1234, -1234, INT_MAX, INT_MIN };

	for (int i = 0; i < (sizeof(str) / sizeof(str[0])); i++) {
		errno = 0;
		end = NULL;
		TEST_ASSERT_EQUAL_INT(expected[i], strtoul(str[i], &end, 2));
		TEST_ASSERT_EQUAL_PTR(str[i] + strlen(str[i]), end);
		TEST_ASSERT_EQUAL_INT(0, errno);
	}
}


TEST(stdlib_strto, strtoul_basic_hex)
{
	char *end;
	const char *str[] = { "0x0", "0x1", "-0x499602D2", "0x499602D2", "0x7FFFFFFF" };
	const unsigned long expected[] = { 0, 1, -1234567890, 1234567890, INT_MAX };

	for (int i = 0; i < (sizeof(str) / sizeof(str[0])); i++) {
		errno = 0;
		end = NULL;
		TEST_ASSERT_EQUAL_INT(expected[i], strtoul(str[i], &end, 16));
		TEST_ASSERT_EQUAL_PTR(str[i] + strlen(str[i]), end);
		TEST_ASSERT_EQUAL_INT(0, errno);
	}
}


TEST(stdlib_strto, strtoul_other_bases)
{
	char *end;
	errno = 0;
	const char *str[] = { "1333333333333333", "17777777777", "553032005531", "4bb2308a7", "1652ca931", "b5gge57", "1vvvvvv", "zik0zj" };
	const int base[] = { 4, 8, 6, 12, 14, 24, 32, 36 };

	for (int i = 0; i < (sizeof(str) / sizeof(str[0])); i++) {
		errno = 0;
		end = NULL;
		TEST_ASSERT_EQUAL_INT(INT_MAX, strtoul(str[i], &end, base[i]));
		TEST_ASSERT_EQUAL_PTR(str[i] + strlen(str[i]), end);
		TEST_ASSERT_EQUAL_INT(0, errno);
	}
}


TEST(stdlib_strto, strtoul_min_max)
{
	char str[50];

	errno = 0;
	sprintf(str, "%lu", ULONG_MAX);
	TEST_ASSERT_EQUAL_INT(ULONG_MAX, strtoul(str, NULL, 10));
	TEST_ASSERT_EQUAL_INT(0, errno);

	errno = 0;
	sprintf(str, "%lx", ULONG_MAX);
	TEST_ASSERT_EQUAL_INT(ULONG_MAX, strtoul(str, NULL, 16));
	TEST_ASSERT_EQUAL_INT(0, errno);
}


TEST(stdlib_strto, strtoull_basic)
{
	char *end;
	const char *str[] = { "0", "1", "-1234567890", "1234567890", "2147483647", "-2147483648" };
	const unsigned long long expected[] = { 0, 1, -1234567890, 1234567890, INT_MAX, INT_MIN };

	for (int i = 0; i < (sizeof(str) / sizeof(str[0])); i++) {
		errno = 0;
		end = NULL;
		TEST_ASSERT_EQUAL_INT(expected[i], strtoull(str[i], &end, 10));
		TEST_ASSERT_EQUAL_PTR(str[i] + strlen(str[i]), end);
		TEST_ASSERT_EQUAL_INT(0, errno);
	}
}


TEST(stdlib_strto, strtoull_basic_binary)
{
	char *end;
	const char *str[] = { "0", "1", "10011010010", "-10011010010", "1111111111111111111111111111111", "-10000000000000000000000000000000" };
	const unsigned long long expected[] = { 0, 1, 1234, -1234, INT_MAX, INT_MIN };

	for (int i = 0; i < (sizeof(str) / sizeof(str[0])); i++) {
		errno = 0;
		end = NULL;
		TEST_ASSERT_EQUAL_INT(expected[i], strtoull(str[i], &end, 2));
		TEST_ASSERT_EQUAL_PTR(str[i] + strlen(str[i]), end);
		TEST_ASSERT_EQUAL_INT(0, errno);
	}
}


TEST(stdlib_strto, strtoull_basic_hex)
{
	char *end;
	const char *str[] = { "0x0", "0x1", "-0x499602D2", "0x499602D2", "0x7FFFFFFF" };
	const unsigned long long expected[] = { 0, 1, -1234567890, 1234567890, INT_MAX };

	for (int i = 0; i < (sizeof(str) / sizeof(str[0])); i++) {
		errno = 0;
		end = NULL;
		TEST_ASSERT_EQUAL_INT(expected[i], strtoull(str[i], &end, 16));
		TEST_ASSERT_EQUAL_PTR(str[i] + strlen(str[i]), end);
		TEST_ASSERT_EQUAL_INT(0, errno);
	}
}


TEST(stdlib_strto, strtoull_other_bases)
{
	char *end;
	errno = 0;
	const char *str[] = { "1333333333333333", "17777777777", "553032005531", "4bb2308a7", "1652ca931", "b5gge57", "1vvvvvv", "zik0zj" };
	const int base[] = { 4, 8, 6, 12, 14, 24, 32, 36 };

	for (int i = 0; i < (sizeof(str) / sizeof(str[0])); i++) {
		errno = 0;
		end = NULL;
		TEST_ASSERT_EQUAL_INT(INT_MAX, strtoull(str[i], &end, base[i]));
		TEST_ASSERT_EQUAL_PTR(str[i] + strlen(str[i]), end);
		TEST_ASSERT_EQUAL_INT(0, errno);
	}
}


TEST(stdlib_strto, strtoull_min_max)
{
	char str[50];

	errno = 0;
	sprintf(str, "%llu", ULLONG_MAX);
	TEST_ASSERT_EQUAL_INT(ULLONG_MAX, strtoull(str, NULL, 10));
	TEST_ASSERT_EQUAL_INT(0, errno);

	errno = 0;
	sprintf(str, "%llx", ULLONG_MAX);
	TEST_ASSERT_EQUAL_INT(ULLONG_MAX, strtoull(str, NULL, 16));
	TEST_ASSERT_EQUAL_INT(0, errno);
}


TEST(stdlib_strto, inf_nan)
{
/* Disabled because of #704 issue: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/704 */
#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#704 issue");
#endif
	char *end;
	const char *str[] = { "Inf", "-Inf", "INF", "-INF", "NaN", "-NaN", "NAN", "-NAN" };
	const double exp_double[] = { INFINITY, -INFINITY, INFINITY, -INFINITY, NAN, -NAN, NAN, -NAN };
	const float exp_float[] = { INFINITY, -INFINITY, INFINITY, -INFINITY, NAN, -NAN, NAN, -NAN };
	const long double exp_ldoubl[] = { INFINITY, -INFINITY, INFINITY, -INFINITY, NAN, -NAN, NAN, -NAN };

	for (int i = 0; i < (sizeof(str) / sizeof(str[0])); i++) {
		errno = 0;
		end = NULL;
		TEST_ASSERT_EQUAL_DOUBLE(exp_double[i], strtod(str[i], &end));
		TEST_ASSERT_EQUAL_PTR(str[i] + strlen(str[i]), end);
		TEST_ASSERT_EQUAL_INT(0, errno);

		errno = 0;
		end = NULL;
		TEST_ASSERT_EQUAL_FLOAT(exp_float[i], strtof(str[i], &end));
		TEST_ASSERT_EQUAL_PTR(str[i] + strlen(str[i]), end);
		TEST_ASSERT_EQUAL_INT(0, errno);

		errno = 0;
		end = NULL;
		TEST_ASSERT_EQUAL_DOUBLE(exp_ldoubl[i], strtold(str[i], &end));
		TEST_ASSERT_EQUAL_PTR(str[i] + strlen(str[i]), end);
		TEST_ASSERT_EQUAL_INT(0, errno);
	}
}


TEST(stdlib_strto, endptr)
{
	const char str_float[] = "1.23 4.56";
	const char str_int[] = "12345 67890";
	char *end = NULL;

	errno = 0;
	TEST_ASSERT_NULL(end);
	TEST_ASSERT_EQUAL_DOUBLE(1.23, strtod(str_float, &end));
	TEST_ASSERT_EQUAL_DOUBLE(4.56, strtod(end, NULL));
	TEST_ASSERT_EQUAL_PTR(str_float + 4, end);
	TEST_ASSERT_EQUAL_INT(0, errno);

	errno = 0;
	end = NULL;
	TEST_ASSERT_NULL(end);
	TEST_ASSERT_EQUAL_FLOAT(1.23, strtof(str_float, &end));
	TEST_ASSERT_EQUAL_FLOAT(4.56, strtof(end, NULL));
	TEST_ASSERT_EQUAL_PTR(str_float + 4, end);
	TEST_ASSERT_EQUAL_INT(0, errno);

	errno = 0;
	end = NULL;
	TEST_ASSERT_NULL(end);
	TEST_ASSERT_EQUAL_DOUBLE(1.23, strtold(str_float, &end));
	TEST_ASSERT_EQUAL_DOUBLE(4.56, strtold(end, NULL));
	TEST_ASSERT_EQUAL_PTR(str_float + 4, end);
	TEST_ASSERT_EQUAL_INT(0, errno);

	errno = 0;
	end = NULL;
	TEST_ASSERT_NULL(end);
	TEST_ASSERT_EQUAL_INT(12345, strtol(str_int, &end, 10));
	TEST_ASSERT_EQUAL_INT(67890, strtol(end, NULL, 10));
	TEST_ASSERT_EQUAL_PTR(str_int + 5, end);
	TEST_ASSERT_EQUAL_INT(0, errno);

	errno = 0;
	end = NULL;
	TEST_ASSERT_NULL(end);
	TEST_ASSERT_EQUAL_INT(12345, strtoll(str_int, &end, 10));
	TEST_ASSERT_EQUAL_INT(67890, strtoll(end, NULL, 10));
	TEST_ASSERT_EQUAL_PTR(str_int + 5, end);
	TEST_ASSERT_EQUAL_INT(0, errno);

	errno = 0;
	end = NULL;
	TEST_ASSERT_NULL(end);
	TEST_ASSERT_EQUAL_INT(12345, strtoul(str_int, &end, 10));
	TEST_ASSERT_EQUAL_INT(67890, strtoul(end, NULL, 10));
	TEST_ASSERT_EQUAL_PTR(str_int + 5, end);
	TEST_ASSERT_EQUAL_INT(0, errno);

	errno = 0;
	end = NULL;
	TEST_ASSERT_NULL(end);
	TEST_ASSERT_EQUAL_INT(12345, strtoull(str_int, &end, 10));
	TEST_ASSERT_EQUAL_INT(67890, strtoull(end, NULL, 10));
	TEST_ASSERT_EQUAL_PTR(str_int + 5, end);
	TEST_ASSERT_EQUAL_INT(0, errno);
}


TEST(stdlib_strto, empty)
{
	const char str[] = "";
	char *end = NULL;

	/* According to POSIX: When "No conversion could be performed", these functions may set errno to EINVAL. */
	/* So errno in these situations can be either 0 or EINVAL nothing else */
	errno = 0;
	TEST_ASSERT_NULL(end);
	TEST_ASSERT_EQUAL_DOUBLE(0, strtod(str, &end));
	TEST_ASSERT_EQUAL_PTR(str, end);
	TEST_ASSERT_TRUE(errno == EINVAL || errno == 0);

	errno = 0;
	end = NULL;
	TEST_ASSERT_EQUAL_FLOAT(0, strtof(str, &end));
	TEST_ASSERT_EQUAL_PTR(str, end);
	TEST_ASSERT_TRUE(errno == EINVAL || errno == 0);

	errno = 0;
	end = NULL;
	TEST_ASSERT_EQUAL_DOUBLE(0, strtold(str, &end));
	TEST_ASSERT_EQUAL_PTR(str, end);
	TEST_ASSERT_TRUE(errno == EINVAL || errno == 0);

	errno = 0;
	end = NULL;
	TEST_ASSERT_EQUAL_INT(0, strtol(str, &end, 10));
	TEST_ASSERT_EQUAL_PTR(str, end);
	TEST_ASSERT_TRUE(errno == EINVAL || errno == 0);

	errno = 0;
	end = NULL;
	TEST_ASSERT_EQUAL_INT(0, strtoll(str, &end, 10));
	TEST_ASSERT_EQUAL_PTR(str, end);
	TEST_ASSERT_TRUE(errno == EINVAL || errno == 0);

	errno = 0;
	end = NULL;
	TEST_ASSERT_EQUAL_INT(0, strtoul(str, &end, 10));
	TEST_ASSERT_EQUAL_PTR(str, end);
	TEST_ASSERT_TRUE(errno == EINVAL || errno == 0);

	errno = 0;
	end = NULL;
	TEST_ASSERT_EQUAL_INT(0, strtoull(str, &end, 10));
	TEST_ASSERT_EQUAL_PTR(str, end);
	TEST_ASSERT_TRUE(errno == EINVAL || errno == 0);
}


TEST(stdlib_strto, truncate)
{
	const char *str_int[] = { "  123   ", "  123", "	123", "123Alma mam lkorta", "123\n\t ", "123!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~", "123\xe3\x83\x9e\xe3\x83\xaa\xe3\x82\xa2\xe3\x83\xbb" };
	const char *str_float[] = { "  1.23  ", "  1.23", "	1.23", "1.23Alma mam lkorta", "1.23\n\t", "1.23!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~", "1.23\xe3\x83\x9e\xe3\x83\xaa\xe3\x82\xa2\xe3\x83\xbb" };

	for (int i = 0; i < (sizeof(str_int) / sizeof(str_int[0])); i++) {
		errno = 0;
		TEST_ASSERT_EQUAL_DOUBLE(123, strtod(str_int[i], NULL));
		TEST_ASSERT_EQUAL_INT(0, errno);

		errno = 0;
		TEST_ASSERT_EQUAL_FLOAT(123, strtof(str_int[i], NULL));
		TEST_ASSERT_EQUAL_INT(0, errno);

		errno = 0;
		TEST_ASSERT_EQUAL_DOUBLE(123, strtold(str_int[i], NULL));
		TEST_ASSERT_EQUAL_INT(0, errno);

		errno = 0;
		TEST_ASSERT_EQUAL_INT(1.23, strtol(str_float[i], NULL, 10));
		TEST_ASSERT_EQUAL_INT(0, errno);

		errno = 0;
		TEST_ASSERT_EQUAL_INT(1.23, strtoll(str_float[i], NULL, 10));
		TEST_ASSERT_EQUAL_INT(0, errno);

		errno = 0;
		TEST_ASSERT_EQUAL_INT(1.23, strtoul(str_float[i], NULL, 10));
		TEST_ASSERT_EQUAL_INT(0, errno);

		errno = 0;
		TEST_ASSERT_EQUAL_INT(1.23, strtoull(str_float[i], NULL, 10));
		TEST_ASSERT_EQUAL_INT(0, errno);
	}
}


TEST(stdlib_strto, truncate_whitespaces)
{
	const char *str_int[] = { "\t123", "\v123", "\f123", "\n123", "\r123", "\r\n123", " 123", "\t\v\f\n\r123" };
	const char *str_float[] = { "\t1.23", "\v1.23", "\f1.23", "\n1.23", "\r1.23", "\r\n1.23", " 1.23", "\t\v\f\n\r1.23" };

	for (int i = 0; i < (sizeof(str_int) / sizeof(str_int[0])); i++) {
		errno = 0;
		TEST_ASSERT_EQUAL_DOUBLE(123, strtod(str_int[i], NULL));
		TEST_ASSERT_EQUAL_INT(0, errno);

		errno = 0;
		TEST_ASSERT_EQUAL_FLOAT(123, strtof(str_int[i], NULL));
		TEST_ASSERT_EQUAL_INT(0, errno);

		errno = 0;
		TEST_ASSERT_EQUAL_DOUBLE(123, strtold(str_int[i], NULL));
		TEST_ASSERT_EQUAL_INT(0, errno);

		errno = 0;
		TEST_ASSERT_EQUAL_INT(1.23, strtol(str_float[i], NULL, 10));
		TEST_ASSERT_EQUAL_INT(0, errno);

		errno = 0;
		TEST_ASSERT_EQUAL_INT(1.23, strtoll(str_float[i], NULL, 10));
		TEST_ASSERT_EQUAL_INT(0, errno);

		errno = 0;
		TEST_ASSERT_EQUAL_INT(1.23, strtoul(str_float[i], NULL, 10));
		TEST_ASSERT_EQUAL_INT(0, errno);

		errno = 0;
		TEST_ASSERT_EQUAL_INT(1.23, strtoull(str_float[i], NULL, 10));
		TEST_ASSERT_EQUAL_INT(0, errno);
	}
}


TEST(stdlib_strto, too_long_numbers_float)
{
	char *retval = NULL;
	const char num1[] =
		"17976931348623157081452742373170435679807056752584499659891747680315"
		"72607800285387605895586327668781715404589535143824642343213268894641"
		"82768467546703537516986049910576551282076245490090389328944075868508"
		"45513394230458323690322294816580855933212334827479782620414472316873"
		"8177180919299881250404026184124858368123111";
	TEST_ASSERT_DOUBLE_IS_INF(strtod(num1, &retval));
	TEST_ASSERT_EQUAL_STRING("", retval);

	retval = NULL;
	TEST_ASSERT_DOUBLE_IS_INF(strtof(num1, &retval));
	TEST_ASSERT_EQUAL_STRING("", retval);

	retval = NULL;
	TEST_ASSERT_DOUBLE_IS_INF(strtold(num1, &retval));
	TEST_ASSERT_EQUAL_STRING("", retval);

	retval = NULL;
	const char num2[] =
		"       17976931348623157081452742373170435679807056752584499659891747680315"
		"72607800285387605895586327668781715404589535143824642343213268894641"
		"82768467546703537516986049910576551282076245490090389328944075868508"
		"45513394230458323690322294816580855933212334827479782620414472316873"
		"817718091929988125040402618412485836812311122222";
	TEST_ASSERT_DOUBLE_IS_INF(strtod(num2, &retval));
	TEST_ASSERT_EQUAL_STRING("", retval);

	retval = NULL;
	TEST_ASSERT_DOUBLE_IS_INF(strtof(num2, &retval));
	TEST_ASSERT_EQUAL_STRING("", retval);

	retval = NULL;
	TEST_ASSERT_DOUBLE_IS_INF(strtold(num2, &retval));
	TEST_ASSERT_EQUAL_STRING("", retval);

	retval = NULL;
	const char num3[] =
		"27976931348623157081452742373170435679807056752584499659891747680315"
		"72607800285387605895586327668781715404589535143824642343213268894641"
		"82768467546703537516986049910576551282076245490090389328944075868508"
		"45513394230458323690322294816580855933212334827479782620414472316873"
		"8177180919299881250404026184124858368";
	TEST_ASSERT_DOUBLE_IS_INF(strtod(num3, &retval));
	TEST_ASSERT_EQUAL_STRING("", retval);

	retval = NULL;
	TEST_ASSERT_DOUBLE_IS_INF(strtof(num3, &retval));
	TEST_ASSERT_EQUAL_STRING("", retval);

	retval = NULL;
	TEST_ASSERT_DOUBLE_IS_INF(strtold(num3, &retval));
	TEST_ASSERT_EQUAL_STRING("", retval);
}


TEST(stdlib_strto, too_long_numbers_int)
{
	const char num[] = "2797693134862315708145274237317043567980705675258449965989174768031572607800285387605895586327668781715404589535143824642343213268894641";

	errno = 0;
	strtol(num, NULL, 10);
	TEST_ASSERT_EQUAL_INT(ERANGE, errno);

	errno = 0;
	strtoul(num, NULL, 10);
	TEST_ASSERT_EQUAL_INT(ERANGE, errno);

	/*
	 * Disabled because of #543 issue: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/543
	 * strtoll, strtoull doesn't set errno
	 */

#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#543 issue");
#endif

	errno = 0;
	strtoll(num, NULL, 10);
	TEST_ASSERT_EQUAL_INT(ERANGE, errno);

	errno = 0;
	strtoull(num, NULL, 10);
	TEST_ASSERT_EQUAL_INT(ERANGE, errno);
}


TEST(stdlib_strto, invalid)
{
	/* According to POSIX: When "No conversion could be performed", these functions may set errno to EINVAL. */
	/* So errno in these situations can be either 0 or EINVAL nothing else */

	const char *str[] = { "Lorem", "Lorem Ipsum", "abcde", "+", ".", ".e0", "+.e-0" };

	for (int i = 0; i < (sizeof(str) / sizeof(str[0])); i++) {
		errno = 0;
		TEST_ASSERT_EQUAL_DOUBLE(0, strtod(str[i], NULL));
		TEST_ASSERT_TRUE(errno == EINVAL || errno == 0);

		errno = 0;
		TEST_ASSERT_EQUAL_FLOAT(0, strtof(str[i], NULL));
		TEST_ASSERT_TRUE(errno == EINVAL || errno == 0);

		errno = 0;
		TEST_ASSERT_EQUAL_DOUBLE(0, strtold(str[i], NULL));
		TEST_ASSERT_TRUE(errno == EINVAL || errno == 0);

		errno = 0;
		TEST_ASSERT_EQUAL_INT(0, strtol(str[i], NULL, 10));
		TEST_ASSERT_TRUE(errno == EINVAL || errno == 0);

		errno = 0;
		TEST_ASSERT_EQUAL_INT(0, strtoll(str[i], NULL, 10));
		TEST_ASSERT_TRUE(errno == EINVAL || errno == 0);

		errno = 0;
		TEST_ASSERT_EQUAL_INT(0, strtoul(str[i], NULL, 10));
		TEST_ASSERT_TRUE(errno == EINVAL || errno == 0);

		errno = 0;
		TEST_ASSERT_EQUAL_INT(0, strtoull(str[i], NULL, 10));
		TEST_ASSERT_TRUE(errno == EINVAL || errno == 0);
	}
}


TEST(stdlib_strto, invalid_base)
{
	errno = 0;
	strtol("1234", NULL, 1);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);

	errno = 0;
	strtol("1234", NULL, INT_MAX);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);

	errno = 0;
	strtol("1234", NULL, INT_MIN);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);

	errno = 0;
	strtoul("1234", NULL, 1);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);

	errno = 0;
	strtoul("1234", NULL, INT_MAX);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);

	errno = 0;
	strtoul("1234", NULL, INT_MIN);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);

	/*
	 * Disabled because of #543 issue: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/543
	 * strtoll, strtoull doesn't set errno
	 */

#ifdef __phoenix__
	TEST_IGNORE_MESSAGE("#543 issue");
#endif

	errno = 0;
	strtoll("1234", NULL, 1);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);

	errno = 0;
	strtoll("1234", NULL, INT_MAX);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);

	errno = 0;
	strtoll("1234", NULL, INT_MIN);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);

	errno = 0;
	strtoull("1234", NULL, 1);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);

	errno = 0;
	strtoull("1234", NULL, INT_MAX);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);

	errno = 0;
	strtoull("1234", NULL, INT_MIN);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST(stdlib_strto, float_remaining_string)
{
	char *tmp;
	const char *str[] = { "    1.797693134862315e+308", "3.12345Alma mam lkorta", "    14999   ", " .875E+testplus",
		" .875E-phoenix", " .875eonlye", " .875e+ewithplus", " .875e+ewithminus", "aaaaaaaaaa", "-aaaaaaaaaa", "+aaaaaaaaaa" };
	int offset[] = { 26, 7, 9, 5, 5, 5, 5, 5, 0, 0, 0 };
	int i;

	for (i = 0; i < (sizeof(str) / sizeof(str[0])); i++) {
		tmp = NULL;
		strtod(str[i], &tmp);
		TEST_ASSERT_EQUAL_STRING(str[i] + offset[i], tmp);

		tmp = NULL;
		strtof(str[i], &tmp);
		TEST_ASSERT_EQUAL_STRING(str[i] + offset[i], tmp);

		tmp = NULL;
		strtold(str[i], &tmp);
		TEST_ASSERT_EQUAL_STRING(str[i] + offset[i], tmp);
	}
}


TEST(stdlib_strto, int_remaining_string)
{
	char *tmp;
	const char *str[] = { "    2147483647", "312345Alma mam lkorta", "    14999   ", " 1234+testplus", " 1234-phoenix", " 1234eonlye",
		" 1234e+ewithplus", " 1234e+ewithminus", "aaaaaaaaaa", "-aaaaaaaaaa", "+aaaaaaaaaa" };
	int offset[] = { 14, 6, 9, 5, 5, 5, 5, 5, 0, 0, 0 };
	int i;

	for (i = 0; i < (sizeof(str) / sizeof(str[0])); i++) {
		tmp = NULL;
		strtol(str[i], &tmp, 10);
		TEST_ASSERT_EQUAL_STRING(str[i] + offset[i], tmp);

		tmp = NULL;
		strtoll(str[i], &tmp, 10);
		TEST_ASSERT_EQUAL_STRING(str[i] + offset[i], tmp);

		tmp = NULL;
		strtoul(str[i], &tmp, 10);
		TEST_ASSERT_EQUAL_STRING(str[i] + offset[i], tmp);

		tmp = NULL;
		strtoull(str[i], &tmp, 10);
		TEST_ASSERT_EQUAL_STRING(str[i] + offset[i], tmp);
	}
}


TEST_GROUP_RUNNER(stdlib_strto)
{
	RUN_TEST_CASE(stdlib_strto, strtod_basic);
	RUN_TEST_CASE(stdlib_strto, strtod_basic_hex);
	RUN_TEST_CASE(stdlib_strto, strtod_min_max);

	RUN_TEST_CASE(stdlib_strto, strtof_basic);
	RUN_TEST_CASE(stdlib_strto, strtof_basic_hex);
	RUN_TEST_CASE(stdlib_strto, strtof_min_max);

	RUN_TEST_CASE(stdlib_strto, strtold_basic);
	RUN_TEST_CASE(stdlib_strto, strtold_basic_hex);
	RUN_TEST_CASE(stdlib_strto, strtold_min_max);

	RUN_TEST_CASE(stdlib_strto, flt_dec_notation);
	RUN_TEST_CASE(stdlib_strto, dbl_dec_notation);

	RUN_TEST_CASE(stdlib_strto, strtol_basic);
	RUN_TEST_CASE(stdlib_strto, strtol_basic_binary);
	RUN_TEST_CASE(stdlib_strto, strtol_basic_hex);
	RUN_TEST_CASE(stdlib_strto, strtol_other_bases);
	RUN_TEST_CASE(stdlib_strto, strtol_other_bases_neg_values);
	RUN_TEST_CASE(stdlib_strto, strtol_min_max);

	RUN_TEST_CASE(stdlib_strto, strtoll_basic);
	RUN_TEST_CASE(stdlib_strto, strtoll_basic_binary);
	RUN_TEST_CASE(stdlib_strto, strtoll_basic_hex);
	RUN_TEST_CASE(stdlib_strto, strtoll_other_bases);
	RUN_TEST_CASE(stdlib_strto, strtoll_other_bases_neg_values);
	RUN_TEST_CASE(stdlib_strto, strtoll_min_max);

	RUN_TEST_CASE(stdlib_strto, strtoul_basic);
	RUN_TEST_CASE(stdlib_strto, strtoul_basic_binary);
	RUN_TEST_CASE(stdlib_strto, strtoul_basic_hex);
	RUN_TEST_CASE(stdlib_strto, strtoul_other_bases);
	RUN_TEST_CASE(stdlib_strto, strtoul_min_max);

	RUN_TEST_CASE(stdlib_strto, strtoull_basic);
	RUN_TEST_CASE(stdlib_strto, strtoull_basic_binary);
	RUN_TEST_CASE(stdlib_strto, strtoull_basic_hex);
	RUN_TEST_CASE(stdlib_strto, strtoull_other_bases);
	RUN_TEST_CASE(stdlib_strto, strtoull_min_max);

	RUN_TEST_CASE(stdlib_strto, inf_nan);
	RUN_TEST_CASE(stdlib_strto, endptr);
	RUN_TEST_CASE(stdlib_strto, empty);
	RUN_TEST_CASE(stdlib_strto, truncate);
	RUN_TEST_CASE(stdlib_strto, truncate_whitespaces);
	RUN_TEST_CASE(stdlib_strto, too_long_numbers_float)
	RUN_TEST_CASE(stdlib_strto, too_long_numbers_int)
	RUN_TEST_CASE(stdlib_strto, invalid);
	RUN_TEST_CASE(stdlib_strto, invalid_base);
	RUN_TEST_CASE(stdlib_strto, float_remaining_string);
	RUN_TEST_CASE(stdlib_strto, int_remaining_string);
}
