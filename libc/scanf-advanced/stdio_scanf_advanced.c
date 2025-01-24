/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - stdio.h
 * TESTED:
 *    - fscanf()
 *    - sscanf()
 *    - vfscanf()
 *    - vsscanf()
 *    - For all functions the following conversion specifiers:
 * 		- "%a", "%e", "%f", "%g"
 * 		- "%c", "%s", "%ms", "%["
 * 		- "%p"
 * 		- "%n"
 *
 * Copyright 2023, 2024 Phoenix Systems
 * Author: Damian Modzelewski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <float.h>

#include <unity_fixture.h>

#include "common.h"


/* Checking for define NL_ARGMAX*/
#ifndef NL_ARGMAX
#define NL_ARGMAX 32
#endif


#define TESTFILE_PATH "stdio_fscanf_test"
#define TEST_STR      "Lorem ipsum dolor sit amet,Vestibulum ante ipsum primis in faucibus orci luctus 123 et ultrices posuere cubilia curae 0x0005"

/* Size enough to hold most of data types int/ptrdif/float(in other formats than %f%F and %lf%lF)/str */
#define BUFF_LEN 300
/* Size for one word stored in TEST_STR */
/* Size 10 because a longest word in TEST_STR have 10 letters */
#define MAX_TESTSTR_WORDLEN 10

/* Checking for define flt minmax*/
#ifndef FLT_MAX
#define FLT_MAX 3.40282347e+38F
#endif

#ifndef FLT_MIN
#define FLT_MIN 1.17549435e-38F
#endif

#define TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax) \
	{ \
		TEST_ASSERT_EQUAL_FLOAT(FLT_MAX, fltMax); \
		TEST_ASSERT_EQUAL_FLOAT(FLT_MAX / 2, fltMaxH); \
		TEST_ASSERT_EQUAL_FLOAT(FLT_MIN, fltMin); \
		TEST_ASSERT_EQUAL_FLOAT(0.f, zero); \
		TEST_ASSERT_EQUAL_FLOAT((FLT_MIN) * -1, negFltMin); \
		TEST_ASSERT_EQUAL_FLOAT((FLT_MAX / 2) * -1, negFltMaxH); \
		TEST_ASSERT_EQUAL_FLOAT((FLT_MAX) * -1, negFltMax); \
	}

static FILE *filep;

static int test_vsscanfWrapper(const char *str, const char *format, ...)
{
	int rc;
	va_list arg_ptr;

	va_start(arg_ptr, format);
	rc = vsscanf(str, format, arg_ptr);
	va_end(arg_ptr);

	return rc;
}


static int test_vfscanfWrapper(FILE *stream, const char *format, ...)
{
	int rc;
	va_list arg_ptr;

	va_start(arg_ptr, format);
	rc = vfscanf(stream, format, arg_ptr);
	va_end(arg_ptr);

	return rc;
}


/*
///////////////////////////////////////////////////////////////////////////////////////////////
*/


TEST_GROUP(stdio_scanf_aefg);
TEST_GROUP(stdio_scanf_cspn);
TEST_GROUP(stdio_scanf_squareBrackets);
TEST_GROUP(stdio_scanf_rest);


/*
//////////////////////////////////////////////////////////////////////////////////////
*/

TEST_SETUP(stdio_scanf_aefg)
{
	filep = fopen(TESTFILE_PATH, "w+");
}


TEST_TEAR_DOWN(stdio_scanf_aefg)
{
	fclose(filep);
}


TEST(stdio_scanf_aefg, f)
{
	char buff[BUFF_LEN] = { 0 };
	float fltMax, fltMaxH, zero, fltMin, negFltMax, negFltMaxH, negFltMin;
	const char *format = "%f %f %f %f %f %f %f";

	/*
	 * Specific precision for float numbers following into 0.
	 * FLT_MAX and FLT_MIN have first digit different by 0 on 38 place after coma
	 * and to have an accurate reading of this value we read at least 4 digits
	 */
	sprintf(buff, "%f %f %.42f %f %.42f %f %f", FLT_MAX, FLT_MAX / 2, FLT_MIN, 0.f, FLT_MIN * -1, (FLT_MAX / 2) * -1, FLT_MAX * -1);
	fprintf(filep, "%s", buff);
	rewind(filep);

	fltMax = fltMaxH = zero = fltMin = negFltMax = negFltMaxH = negFltMin = 1.0;
	TEST_ASSERT_EQUAL_INT(7, test_vfscanfWrapper(filep, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	/* This block contains all asserts from min to max for float */
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);

	rewind(filep);

	fltMax = fltMaxH = zero = fltMin = negFltMax = negFltMaxH = negFltMin = 1.0;
	TEST_ASSERT_EQUAL_INT(7, fscanf(filep, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);

	fltMax = fltMaxH = zero = fltMin = negFltMax = negFltMaxH = negFltMin = 1.0;
	TEST_ASSERT_EQUAL_INT(7, test_vsscanfWrapper(buff, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);

	fltMax = fltMaxH = zero = fltMin = negFltMax = negFltMaxH = negFltMin = 1.0;
	TEST_ASSERT_EQUAL_INT(7, sscanf(buff, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);
}


/*
////////////////////////////////////////////////////////////////////////////////////////////////////
*/


TEST(stdio_scanf_aefg, F)
{
	char buff[BUFF_LEN] = { 0 };
	float fltMax, fltMaxH, zero, fltMin, negFltMax, negFltMaxH, negFltMin;
	const char *format = "%F %F %F %F %F %F %F";

	/*
	 * Specific precision for float numbers following into 0.
	 * FLT_MAX and FLT_MIN have first digit different by 0 on 38 place after coma
	 * and to have an accurate reading of this value we read at least 4 digits
	 */
	sprintf(buff, "%f %f %.42f %f %.42f %f %f", FLT_MAX, FLT_MAX / 2, FLT_MIN, 0.f, FLT_MIN * -1, (FLT_MAX / 2) * -1, FLT_MAX * -1);
	fprintf(filep, "%s", buff);
	rewind(filep);

	fltMax = fltMaxH = zero = fltMin = negFltMax = negFltMaxH = negFltMin = 1.0;
	TEST_ASSERT_EQUAL_INT(7, test_vfscanfWrapper(filep, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	/* This block contains all asserts from min to max for float */
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);

	rewind(filep);

	fltMax = fltMaxH = zero = fltMin = negFltMax = negFltMaxH = negFltMin = 1.0;
	TEST_ASSERT_EQUAL_INT(7, fscanf(filep, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);

	fltMax = fltMaxH = zero = fltMin = negFltMax = negFltMaxH = negFltMin = 1.0;
	TEST_ASSERT_EQUAL_INT(7, test_vsscanfWrapper(buff, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);

	fltMax = fltMaxH = zero = fltMin = negFltMax = negFltMaxH = negFltMin = 1.0;
	TEST_ASSERT_EQUAL_INT(7, sscanf(buff, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);
}


/*
////////////////////////////////////////////////////////////////////////////////////////////////////
*/


TEST(stdio_scanf_aefg, a)
{
	char buff[BUFF_LEN] = { 0 };
	float fltMax, fltMaxH, zero, fltMin, negFltMax, negFltMaxH, negFltMin;
	const char *format = "%a %a %a %a %a %a %a";

	sprintf(buff, format, FLT_MAX, FLT_MAX / 2, FLT_MIN, 0.f, FLT_MIN * -1, (FLT_MAX / 2) * -1, FLT_MAX * -1);
	fprintf(filep, "%s", buff);
	rewind(filep);

	fltMax = fltMaxH = zero = fltMin = negFltMax = negFltMaxH = negFltMin = 1.0;
	TEST_ASSERT_EQUAL_INT(7, test_vfscanfWrapper(filep, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	/* This block contains all asserts from min to max for float */
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);

	rewind(filep);

	fltMax = fltMaxH = zero = fltMin = negFltMax = negFltMaxH = negFltMin = 1.0;
	TEST_ASSERT_EQUAL_INT(7, fscanf(filep, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);

	fltMax = fltMaxH = zero = fltMin = negFltMax = negFltMaxH = negFltMin = 1.0;
	TEST_ASSERT_EQUAL_INT(7, test_vsscanfWrapper(buff, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);

	fltMax = fltMaxH = zero = fltMin = negFltMax = negFltMaxH = negFltMin = 1.0;
	TEST_ASSERT_EQUAL_INT(7, sscanf(buff, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);
}


/*
////////////////////////////////////////////////////////////////////////////////////////////////////
*/


TEST(stdio_scanf_aefg, A)
{
	char buff[BUFF_LEN] = { 0 };
	float fltMax, fltMaxH, zero, fltMin, negFltMax, negFltMaxH, negFltMin;
	const char *format = "%A %A %A %A %A %A %A";

	sprintf(buff, format, FLT_MAX, FLT_MAX / 2, FLT_MIN, 0.f, FLT_MIN * -1, (FLT_MAX / 2) * -1, FLT_MAX * -1);
	fprintf(filep, "%s", buff);
	rewind(filep);

	fltMax = fltMaxH = zero = fltMin = negFltMax = negFltMaxH = negFltMin = 1.0;
	TEST_ASSERT_EQUAL_INT(7, test_vfscanfWrapper(filep, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	/* This block contains all asserts from min to max for float */
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);

	rewind(filep);

	fltMax = fltMaxH = zero = fltMin = negFltMax = negFltMaxH = negFltMin = 1.0;
	TEST_ASSERT_EQUAL_INT(7, fscanf(filep, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);

	fltMax = fltMaxH = zero = fltMin = negFltMax = negFltMaxH = negFltMin = 1.0;
	TEST_ASSERT_EQUAL_INT(7, test_vsscanfWrapper(buff, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);

	fltMax = fltMaxH = zero = fltMin = negFltMax = negFltMaxH = negFltMin = 1.0;
	TEST_ASSERT_EQUAL_INT(7, sscanf(buff, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);
}


/*
////////////////////////////////////////////////////////////////////////////////////////////////////
*/


TEST(stdio_scanf_aefg, e)
{
#ifdef __TARGET_SPARCV8LEON
	/* Disabled on SPARC because of issue https://github.com/phoenix-rtos/phoenix-rtos-project/issues/970 */
	TEST_IGNORE();
#endif

	char buff[BUFF_LEN] = { 0 };
	float fltMax, fltMaxH, zero, fltMin, negFltMax, negFltMaxH, negFltMin;
	const char *format = "%e %e %e %e %e %e %e";

	sprintf(buff, format, FLT_MAX, FLT_MAX / 2, FLT_MIN, 0.f, FLT_MIN * -1, (FLT_MAX / 2) * -1, FLT_MAX * -1);
	fprintf(filep, "%s", buff);
	rewind(filep);

	fltMax = fltMaxH = zero = fltMin = negFltMax = negFltMaxH = negFltMin = 1.0;
	TEST_ASSERT_EQUAL_INT(7, test_vfscanfWrapper(filep, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	/* This block contains all asserts from min to max for float */
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);

	rewind(filep);

	fltMax = fltMaxH = zero = fltMin = negFltMax = negFltMaxH = negFltMin = 1.0;
	TEST_ASSERT_EQUAL_INT(7, fscanf(filep, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);

	fltMax = fltMaxH = zero = fltMin = negFltMax = negFltMaxH = negFltMin = 1.0;
	TEST_ASSERT_EQUAL_INT(7, test_vsscanfWrapper(buff, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);

	fltMax = fltMaxH = zero = fltMin = negFltMax = negFltMaxH = negFltMin = 1.0;
	TEST_ASSERT_EQUAL_INT(7, sscanf(buff, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);
}


/*
////////////////////////////////////////////////////////////////////////////////////////////////////
*/


TEST(stdio_scanf_aefg, E)
{
#ifdef __TARGET_SPARCV8LEON
	/* Disabled on SPARC because of issue https://github.com/phoenix-rtos/phoenix-rtos-project/issues/970 */
	TEST_IGNORE();
#endif

	char buff[BUFF_LEN] = { 0 };
	float fltMax, fltMaxH, zero, fltMin, negFltMax, negFltMaxH, negFltMin;
	const char *format = "%E %E %E %E %E %E %E";

	sprintf(buff, format, FLT_MAX, FLT_MAX / 2, FLT_MIN, 0.f, FLT_MIN * -1, (FLT_MAX / 2) * -1, FLT_MAX * -1);
	fprintf(filep, "%s", buff);
	rewind(filep);

	fltMax = fltMaxH = zero = fltMin = negFltMax = negFltMaxH = negFltMin = 1.0;
	TEST_ASSERT_EQUAL_INT(7, test_vfscanfWrapper(filep, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	/* This block contains all asserts from min to max for float */
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);

	rewind(filep);

	fltMax = fltMaxH = zero = fltMin = negFltMax = negFltMaxH = negFltMin = 1.0;
	TEST_ASSERT_EQUAL_INT(7, fscanf(filep, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);

	fltMax = fltMaxH = zero = fltMin = negFltMax = negFltMaxH = negFltMin = 1.0;
	TEST_ASSERT_EQUAL_INT(7, test_vsscanfWrapper(buff, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);

	fltMax = fltMaxH = zero = fltMin = negFltMax = negFltMaxH = negFltMin = 1.0;
	TEST_ASSERT_EQUAL_INT(7, sscanf(buff, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);
}


/*
////////////////////////////////////////////////////////////////////////////////////////////////////
*/


TEST(stdio_scanf_aefg, g)
{
#ifdef __TARGET_SPARCV8LEON
	/* Disabled on SPARC because of issue https://github.com/phoenix-rtos/phoenix-rtos-project/issues/970 */
	TEST_IGNORE();
#endif

	char buff[BUFF_LEN] = { 0 };
	float fltMax, fltMaxH, zero, fltMin, negFltMax, negFltMaxH, negFltMin;
	const char *format = "%g %g %g %g %g %g %g";

	sprintf(buff, format, FLT_MAX, FLT_MAX / 2, FLT_MIN, 0.f, FLT_MIN * -1, (FLT_MAX / 2) * -1, FLT_MAX * -1);
	fprintf(filep, "%s", buff);
	rewind(filep);

	fltMax = fltMaxH = zero = fltMin = negFltMax = negFltMaxH = negFltMin = 1.0;
	TEST_ASSERT_EQUAL_INT(7, test_vfscanfWrapper(filep, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	/* This block contains all asserts from min to max for float */
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);

	rewind(filep);

	fltMax = fltMaxH = zero = fltMin = negFltMax = negFltMaxH = negFltMin = 1.0;
	TEST_ASSERT_EQUAL_INT(7, fscanf(filep, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);

	fltMax = fltMaxH = zero = fltMin = negFltMax = negFltMaxH = negFltMin = 1.0;
	TEST_ASSERT_EQUAL_INT(7, test_vsscanfWrapper(buff, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);

	fltMax = fltMaxH = zero = fltMin = negFltMax = negFltMaxH = negFltMin = 1.0;
	TEST_ASSERT_EQUAL_INT(7, sscanf(buff, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);
}


/*
////////////////////////////////////////////////////////////////////////////////////////////////////
*/


TEST(stdio_scanf_aefg, G)
{
#ifdef __TARGET_SPARCV8LEON
	/* Disabled on SPARC because of issue https://github.com/phoenix-rtos/phoenix-rtos-project/issues/970 */
	TEST_IGNORE();
#endif

	char buff[BUFF_LEN] = { 0 };
	float fltMax, fltMaxH, zero, fltMin, negFltMax, negFltMaxH, negFltMin;
	const char *format = "%G %G %G %G %G %G %G";

	sprintf(buff, format, FLT_MAX, FLT_MAX / 2, FLT_MIN, 0.f, FLT_MIN * -1, (FLT_MAX / 2) * -1, FLT_MAX * -1);
	fprintf(filep, "%s", buff);
	rewind(filep);

	fltMax = fltMaxH = zero = fltMin = negFltMax = negFltMaxH = negFltMin = 1.0;
	TEST_ASSERT_EQUAL_INT(7, test_vfscanfWrapper(filep, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	/* This block contains all asserts from min to max for float */
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);

	rewind(filep);

	fltMax = fltMaxH = zero = fltMin = negFltMax = negFltMaxH = negFltMin = 1.0;
	TEST_ASSERT_EQUAL_INT(7, fscanf(filep, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);

	fltMax = fltMaxH = zero = fltMin = negFltMax = negFltMaxH = negFltMin = 1.0;
	TEST_ASSERT_EQUAL_INT(7, test_vsscanfWrapper(buff, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);

	fltMax = fltMaxH = zero = fltMin = negFltMax = negFltMaxH = negFltMin = 1.0;
	TEST_ASSERT_EQUAL_INT(7, sscanf(buff, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);
}


/*
////////////////////////////////////////////////////////////////////////////////////////////////////
*/


TEST(stdio_scanf_aefg, inf_nan_f)
{
	char buff[BUFF_LEN] = { 0 };
	double valInf, valNan, valNegInf;
	const char *format = "%lf %lf %lf";

	sprintf(buff, format, INFINITY, INFINITY * -1, NAN);
	fprintf(filep, "%s", buff);
	rewind(filep);

	valInf = valNan = valNegInf = 1.0;
	TEST_ASSERT_EQUAL_INT(3, test_vfscanfWrapper(filep, format, &valInf, &valNegInf, &valNan));

	TEST_ASSERT_EQUAL_DOUBLE(INFINITY, valInf);
	TEST_ASSERT_EQUAL_DOUBLE(INFINITY * -1, valNegInf);
	TEST_ASSERT_EQUAL_DOUBLE(NAN, valNan);

	rewind(filep);

	valInf = valNan = valNegInf = 1.0;
	TEST_ASSERT_EQUAL_INT(3, fscanf(filep, format, &valInf, &valNegInf, &valNan));

	TEST_ASSERT_EQUAL_DOUBLE(INFINITY, valInf);
	TEST_ASSERT_EQUAL_DOUBLE(INFINITY * -1, valNegInf);
	TEST_ASSERT_EQUAL_DOUBLE(NAN, valNan);

	valInf = valNan = valNegInf = 1.0;
	TEST_ASSERT_EQUAL_INT(3, test_vsscanfWrapper(buff, format, &valInf, &valNegInf, &valNan));

	TEST_ASSERT_EQUAL_DOUBLE(INFINITY, valInf);
	TEST_ASSERT_EQUAL_DOUBLE(INFINITY * -1, valNegInf);
	TEST_ASSERT_EQUAL_DOUBLE(NAN, valNan);

	valInf = valNan = valNegInf = 1.0;
	TEST_ASSERT_EQUAL_INT(3, sscanf(buff, format, &valInf, &valNegInf, &valNan));

	TEST_ASSERT_EQUAL_DOUBLE(INFINITY, valInf);
	TEST_ASSERT_EQUAL_DOUBLE(INFINITY * -1, valNegInf);
	TEST_ASSERT_EQUAL_DOUBLE(NAN, valNan);
}


/*
////////////////////////////////////////////////////////////////////////////////////////////////////
*/


TEST(stdio_scanf_aefg, inf_nan_a)
{
	char buff[BUFF_LEN] = { 0 };
	double valInf, valNan, valNegInf;
	const char *format = "%la %la %la";

	sprintf(buff, format, INFINITY, INFINITY * -1, NAN);
	fprintf(filep, "%s", buff);
	rewind(filep);

	valInf = valNan = valNegInf = 1.0;
	TEST_ASSERT_EQUAL_INT(3, test_vfscanfWrapper(filep, format, &valInf, &valNegInf, &valNan));

	TEST_ASSERT_EQUAL_DOUBLE(INFINITY, valInf);
	TEST_ASSERT_EQUAL_DOUBLE(INFINITY * -1, valNegInf);
	TEST_ASSERT_EQUAL_DOUBLE(NAN, valNan);

	rewind(filep);

	valInf = valNan = valNegInf = 1.0;
	TEST_ASSERT_EQUAL_INT(3, fscanf(filep, format, &valInf, &valNegInf, &valNan));

	TEST_ASSERT_EQUAL_DOUBLE(INFINITY, valInf);
	TEST_ASSERT_EQUAL_DOUBLE(INFINITY * -1, valNegInf);
	TEST_ASSERT_EQUAL_DOUBLE(NAN, valNan);

	valInf = valNan = valNegInf = 1.0;
	TEST_ASSERT_EQUAL_INT(3, test_vsscanfWrapper(buff, format, &valInf, &valNegInf, &valNan));

	TEST_ASSERT_EQUAL_DOUBLE(INFINITY, valInf);
	TEST_ASSERT_EQUAL_DOUBLE(INFINITY * -1, valNegInf);
	TEST_ASSERT_EQUAL_DOUBLE(NAN, valNan);

	valInf = valNan = valNegInf = 1.0;
	TEST_ASSERT_EQUAL_INT(3, sscanf(buff, format, &valInf, &valNegInf, &valNan));

	TEST_ASSERT_EQUAL_DOUBLE(INFINITY, valInf);
	TEST_ASSERT_EQUAL_DOUBLE(INFINITY * -1, valNegInf);
	TEST_ASSERT_EQUAL_DOUBLE(NAN, valNan);
}


/*
////////////////////////////////////////////////////////////////////////////////////////////////////
*/


TEST(stdio_scanf_aefg, inf_nan_e)
{
	char buff[BUFF_LEN] = { 0 };
	double valInf, valNan, valNegInf;
	const char *format = "%le %le %le";

	sprintf(buff, format, INFINITY, INFINITY * -1, NAN);
	fprintf(filep, "%s", buff);
	rewind(filep);

	valInf = valNan = valNegInf = 1.0;
	TEST_ASSERT_EQUAL_INT(3, test_vfscanfWrapper(filep, format, &valInf, &valNegInf, &valNan));

	TEST_ASSERT_EQUAL_DOUBLE(INFINITY, valInf);
	TEST_ASSERT_EQUAL_DOUBLE(INFINITY * -1, valNegInf);
	TEST_ASSERT_EQUAL_DOUBLE(NAN, valNan);

	rewind(filep);

	valInf = valNan = valNegInf = 1.0;
	TEST_ASSERT_EQUAL_INT(3, fscanf(filep, format, &valInf, &valNegInf, &valNan));

	TEST_ASSERT_EQUAL_DOUBLE(INFINITY, valInf);
	TEST_ASSERT_EQUAL_DOUBLE(INFINITY * -1, valNegInf);
	TEST_ASSERT_EQUAL_DOUBLE(NAN, valNan);

	valInf = valNan = valNegInf = 1.0;
	TEST_ASSERT_EQUAL_INT(3, test_vsscanfWrapper(buff, format, &valInf, &valNegInf, &valNan));

	TEST_ASSERT_EQUAL_DOUBLE(INFINITY, valInf);
	TEST_ASSERT_EQUAL_DOUBLE(INFINITY * -1, valNegInf);
	TEST_ASSERT_EQUAL_DOUBLE(NAN, valNan);

	valInf = valNan = valNegInf = 1.0;
	TEST_ASSERT_EQUAL_INT(3, sscanf(buff, format, &valInf, &valNegInf, &valNan));

	TEST_ASSERT_EQUAL_DOUBLE(INFINITY, valInf);
	TEST_ASSERT_EQUAL_DOUBLE(INFINITY * -1, valNegInf);
	TEST_ASSERT_EQUAL_DOUBLE(NAN, valNan);
}


/*
////////////////////////////////////////////////////////////////////////////////////////////////////
*/


TEST(stdio_scanf_aefg, inf_nan_g)
{
	char buff[BUFF_LEN] = { 0 };
	double valInf, valNan, valNegInf;
	const char *format = "%lg %lg %lg";

	sprintf(buff, format, INFINITY, INFINITY * -1, NAN);
	fprintf(filep, "%s", buff);
	rewind(filep);

	valInf = valNan = valNegInf = 1.0;
	TEST_ASSERT_EQUAL_INT(3, test_vfscanfWrapper(filep, format, &valInf, &valNegInf, &valNan));

	TEST_ASSERT_EQUAL_DOUBLE(INFINITY, valInf);
	TEST_ASSERT_EQUAL_DOUBLE(INFINITY * -1, valNegInf);
	TEST_ASSERT_EQUAL_DOUBLE(NAN, valNan);

	rewind(filep);

	valInf = valNan = valNegInf = 1.0;
	TEST_ASSERT_EQUAL_INT(3, fscanf(filep, format, &valInf, &valNegInf, &valNan));

	TEST_ASSERT_EQUAL_DOUBLE(INFINITY, valInf);
	TEST_ASSERT_EQUAL_DOUBLE(INFINITY * -1, valNegInf);
	TEST_ASSERT_EQUAL_DOUBLE(NAN, valNan);

	valInf = valNan = valNegInf = 1.0;
	TEST_ASSERT_EQUAL_INT(3, test_vsscanfWrapper(buff, format, &valInf, &valNegInf, &valNan));

	TEST_ASSERT_EQUAL_DOUBLE(INFINITY, valInf);
	TEST_ASSERT_EQUAL_DOUBLE(INFINITY * -1, valNegInf);
	TEST_ASSERT_EQUAL_DOUBLE(NAN, valNan);

	valInf = valNan = valNegInf = 1.0;
	TEST_ASSERT_EQUAL_INT(3, sscanf(buff, format, &valInf, &valNegInf, &valNan));

	TEST_ASSERT_EQUAL_DOUBLE(INFINITY, valInf);
	TEST_ASSERT_EQUAL_DOUBLE(INFINITY * -1, valNegInf);
	TEST_ASSERT_EQUAL_DOUBLE(NAN, valNan);
}


/*
////////////////////////////////////////////////////////////////////////////////////////////////////
*/


TEST_SETUP(stdio_scanf_cspn)
{
	filep = fopen(TESTFILE_PATH, "w+");
}


TEST_TEAR_DOWN(stdio_scanf_cspn)
{
	fclose(filep);
}


TEST(stdio_scanf_cspn, c)
{
	const char *format = "%corem-ips%cm-dolo%c";
	char buff[BUFF_LEN] = "Lorem-ipsum-dolor";
	char c1, c2, c3;

	fprintf(filep, "%s", buff);
	rewind(filep);

	c1 = c2 = c3 = 0;
	TEST_ASSERT_EQUAL_INT(3, test_vfscanfWrapper(filep, format, &c1, &c2, &c3));
	TEST_ASSERT_EQUAL_CHAR('L', c1);
	TEST_ASSERT_EQUAL_CHAR('u', c2);
	TEST_ASSERT_EQUAL_CHAR('r', c3);

	rewind(filep);

	c1 = c2 = c3 = 0;
	TEST_ASSERT_EQUAL_INT(3, fscanf(filep, format, &c1, &c2, &c3));
	TEST_ASSERT_EQUAL_CHAR('L', c1);
	TEST_ASSERT_EQUAL_CHAR('u', c2);
	TEST_ASSERT_EQUAL_CHAR('r', c3);

	c1 = c2 = c3 = 0;
	TEST_ASSERT_EQUAL_INT(3, test_vsscanfWrapper(buff, format, &c1, &c2, &c3));
	TEST_ASSERT_EQUAL_CHAR('L', c1);
	TEST_ASSERT_EQUAL_CHAR('u', c2);
	TEST_ASSERT_EQUAL_CHAR('r', c3);

	c1 = c2 = c3 = 0;
	TEST_ASSERT_EQUAL_INT(3, sscanf(buff, format, &c1, &c2, &c3));
	TEST_ASSERT_EQUAL_CHAR('L', c1);
	TEST_ASSERT_EQUAL_CHAR('u', c2);
	TEST_ASSERT_EQUAL_CHAR('r', c3);
}


TEST(stdio_scanf_cspn, c_ascii)
{
	char buff[BUFF_LEN] = { 0 };
	char c;
	int i;

	for (i = 1; i < 128; i++) {
		buff[i - 1] = i;
	}

	fprintf(filep, "%s", buff);
	rewind(filep);

	for (i = 1; i < 128; i++) {
		c = 0;
		TEST_ASSERT_EQUAL_INT(1, test_vfscanfWrapper(filep, "%c", &c));
		TEST_ASSERT_EQUAL_CHAR(i, c);

		fseek(filep, i - 1, SEEK_SET);

		c = 0;
		TEST_ASSERT_EQUAL_INT(1, fscanf(filep, "%c", &c));
		TEST_ASSERT_EQUAL_CHAR(i, c);

		/*
		 * This fseek is used because of issue #639
		 * https://github.com/phoenix-rtos/phoenix-rtos-project/issues/639
		 */

#ifdef __phoenix__
		fseek(filep, i, SEEK_SET);
#endif

		c = 0;
		TEST_ASSERT_EQUAL_INT(1, test_vsscanfWrapper(&buff[i - 1], "%c", &c));
		TEST_ASSERT_EQUAL_CHAR(i, c);

		c = 0;
		TEST_ASSERT_EQUAL_INT(1, sscanf(&buff[i - 1], "%c", &c));
		TEST_ASSERT_EQUAL_CHAR(i, c);
	}
}


TEST(stdio_scanf_cspn, s_path)
{
	char buff[BUFF_LEN] = TESTFILE_PATH;
	char res[BUFF_LEN];

	fprintf(filep, "%s", buff);
	rewind(filep);

	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(1, test_vfscanfWrapper(filep, "%s", res));
	TEST_ASSERT_EQUAL_STRING(TESTFILE_PATH, res);

	rewind(filep);

	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(1, fscanf(filep, "%s", res));
	TEST_ASSERT_EQUAL_STRING(TESTFILE_PATH, res);

	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(1, test_vsscanfWrapper(buff, "%s", res));
	TEST_ASSERT_EQUAL_STRING(TESTFILE_PATH, res);

	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(1, sscanf(buff, "%s", res));
	TEST_ASSERT_EQUAL_STRING(TESTFILE_PATH, res);
}


TEST(stdio_scanf_cspn, s_pick)
{
	char buff[BUFF_LEN] = TEST_STR;
	char words[6][MAX_TESTSTR_WORDLEN];
	const char *format = "%s %s %s %s amet,Vestibulum ante ipsum primis in faucibus orci luctus %s et ultrices posuere cubilia "
						 "curae %s";
	fprintf(filep, "%s", buff);
	rewind(filep);

	memset(words, 0, sizeof(words));
	TEST_ASSERT_EQUAL_INT(6, test_vfscanfWrapper(filep, format, words[0], words[1], words[2], words[3], words[4], words[5]));

	TEST_ASSERT_EQUAL_STRING("Lorem", words[0]);
	TEST_ASSERT_EQUAL_STRING("ipsum", words[1]);
	TEST_ASSERT_EQUAL_STRING("dolor", words[2]);
	TEST_ASSERT_EQUAL_STRING("sit", words[3]);
	TEST_ASSERT_EQUAL_STRING("123", words[4]);
	TEST_ASSERT_EQUAL_STRING("0x0005", words[5]);

	rewind(filep);

	memset(words, 0, sizeof(words));
	TEST_ASSERT_EQUAL_INT(6, fscanf(filep, format, words[0], words[1], words[2], words[3], words[4], words[5]));

	TEST_ASSERT_EQUAL_STRING("Lorem", words[0]);
	TEST_ASSERT_EQUAL_STRING("ipsum", words[1]);
	TEST_ASSERT_EQUAL_STRING("dolor", words[2]);
	TEST_ASSERT_EQUAL_STRING("sit", words[3]);
	TEST_ASSERT_EQUAL_STRING("123", words[4]);
	TEST_ASSERT_EQUAL_STRING("0x0005", words[5]);

	memset(words, 0, sizeof(words));
	TEST_ASSERT_EQUAL_INT(6, test_vsscanfWrapper(buff, format, words[0], words[1], words[2], words[3], words[4], words[5]));

	TEST_ASSERT_EQUAL_STRING("Lorem", words[0]);
	TEST_ASSERT_EQUAL_STRING("ipsum", words[1]);
	TEST_ASSERT_EQUAL_STRING("dolor", words[2]);
	TEST_ASSERT_EQUAL_STRING("sit", words[3]);
	TEST_ASSERT_EQUAL_STRING("123", words[4]);
	TEST_ASSERT_EQUAL_STRING("0x0005", words[5]);

	memset(words, 0, sizeof(words));
	TEST_ASSERT_EQUAL_INT(6, sscanf(buff, format, words[0], words[1], words[2], words[3], words[4], words[5]));

	TEST_ASSERT_EQUAL_STRING("Lorem", words[0]);
	TEST_ASSERT_EQUAL_STRING("ipsum", words[1]);
	TEST_ASSERT_EQUAL_STRING("dolor", words[2]);
	TEST_ASSERT_EQUAL_STRING("sit", words[3]);
	TEST_ASSERT_EQUAL_STRING("123", words[4]);
	TEST_ASSERT_EQUAL_STRING("0x0005", words[5]);
}


TEST(stdio_scanf_cspn, s_torn)
{
	char buff[BUFF_LEN] = { 0 };
	const char *txt = "\4399\0ns";

	fprintf(filep, "%s", txt);
	rewind(filep);

	memset(buff, 0, sizeof(buff));
	TEST_ASSERT_EQUAL_INT(1, test_vfscanfWrapper(filep, "%s", buff));
	TEST_ASSERT_EQUAL_CHAR(txt[3], buff[3]);
	TEST_ASSERT_NOT_EQUAL_CHAR(txt[4], buff[4]);
	TEST_ASSERT_NOT_EQUAL_CHAR(txt[5], buff[5]);
	TEST_ASSERT_EQUAL_STRING(txt, buff);

	rewind(filep);

	memset(buff, 0, sizeof(buff));
	TEST_ASSERT_EQUAL_INT(1, fscanf(filep, "%s", buff));
	TEST_ASSERT_EQUAL_CHAR(txt[3], buff[3]);
	TEST_ASSERT_NOT_EQUAL_CHAR(txt[4], buff[4]);
	TEST_ASSERT_NOT_EQUAL_CHAR(txt[5], buff[5]);
	TEST_ASSERT_EQUAL_STRING(txt, buff);

	memset(buff, 0, sizeof(buff));
	TEST_ASSERT_EQUAL_INT(1, test_vsscanfWrapper(txt, "%s", buff));
	TEST_ASSERT_EQUAL_CHAR(txt[3], buff[3]);
	TEST_ASSERT_NOT_EQUAL_CHAR(txt[4], buff[4]);
	TEST_ASSERT_NOT_EQUAL_CHAR(txt[5], buff[5]);
	TEST_ASSERT_EQUAL_STRING(txt, buff);

	memset(buff, 0, sizeof(buff));
	TEST_ASSERT_EQUAL_INT(1, sscanf(txt, "%s", buff));
	TEST_ASSERT_EQUAL_CHAR(txt[3], buff[3]);
	TEST_ASSERT_NOT_EQUAL_CHAR(txt[4], buff[4]);
	TEST_ASSERT_NOT_EQUAL_CHAR(txt[5], buff[5]);
	TEST_ASSERT_EQUAL_STRING(txt, buff);
}


TEST(stdio_scanf_cspn, s_ascii)
{
	char buff[BUFF_LEN] = { 0 };
	char asciiStr[BUFF_LEN] = { 0 };
	int i;

	/*
	 * In ASCII table from number 33 starts printable characters
	 * in this case we want to dodge terminating ASCII signs
	 */
	for (i = 33; i < 127; i++) {
		buff[i - 33] = i;
	}
	buff[i] = '\0';

	fprintf(filep, "%s", buff);
	rewind(filep);


	memset(asciiStr, 0, sizeof(asciiStr));
	TEST_ASSERT_EQUAL_INT(1, fscanf(filep, "%s", asciiStr));
	TEST_ASSERT_EQUAL_STRING(buff, asciiStr);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(1, test_vfscanfWrapper(filep, "%s", asciiStr));
	TEST_ASSERT_EQUAL_STRING(buff, asciiStr);

	TEST_ASSERT_EQUAL_INT(1, test_vsscanfWrapper(buff, "%s", asciiStr));
	TEST_ASSERT_EQUAL_STRING(buff, asciiStr);

	TEST_ASSERT_EQUAL_INT(1, sscanf(buff, "%s", asciiStr));
	TEST_ASSERT_EQUAL_STRING(buff, asciiStr);
}


TEST(stdio_scanf_cspn, percent)
{
	char buff[BUFF_LEN] = "%yes % --- % yes";
	char correct[BUFF_LEN] = { 0 };
	char wrong[BUFF_LEN] = { 0 };
	const char *format = "%%%s%%--- %% %s";

	fprintf(filep, "%s", buff);

	rewind(filep);

	memset(correct, 0, sizeof(correct));
	memset(wrong, 0, sizeof(wrong));
	TEST_ASSERT_EQUAL_INT(1, test_vfscanfWrapper(filep, format, correct, wrong));
	TEST_ASSERT_EQUAL_STRING("yes", correct);
	TEST_ASSERT_EQUAL_STRING("", wrong);

	rewind(filep);

	memset(correct, 0, sizeof(correct));
	memset(wrong, 0, sizeof(wrong));
	TEST_ASSERT_EQUAL_INT(1, fscanf(filep, format, correct, wrong));
	TEST_ASSERT_EQUAL_STRING("yes", correct);
	TEST_ASSERT_EQUAL_STRING("", wrong);

	memset(correct, 0, sizeof(correct));
	memset(wrong, 0, sizeof(wrong));
	TEST_ASSERT_EQUAL_INT(1, test_vsscanfWrapper(buff, format, correct, wrong));
	TEST_ASSERT_EQUAL_STRING("yes", correct);
	TEST_ASSERT_EQUAL_STRING("", wrong);

	memset(correct, 0, sizeof(correct));
	memset(wrong, 0, sizeof(wrong));
	TEST_ASSERT_EQUAL_INT(1, sscanf(buff, format, correct, wrong));
	TEST_ASSERT_EQUAL_STRING("yes", correct);
	TEST_ASSERT_EQUAL_STRING("", wrong);
}


TEST(stdio_scanf_cspn, ptr)
{
	char buff[BUFF_LEN];
	char format[] = "%p %p %p %p";
	void *const expPtr = (void *)0xDEADBEEF;
	void *const expPtrZero = (void *)0x00000000;
	void *const expPtrMax = (void *)INTPTR_MAX;
	void *const expPtrMin = (void *)INTPTR_MIN;
	void *ptrVal, *ptrValZero, *ptrValMax, *ptrValMin;

	sprintf(buff, format, expPtr, expPtrZero, expPtrMax, expPtrMin);
	fprintf(filep, "%s", buff);
	rewind(filep);

	ptrVal = ptrValZero = ptrValMax = ptrValMin = (void *)1;
	TEST_ASSERT_EQUAL_INT(4, test_vfscanfWrapper(filep, format, &ptrVal, &ptrValZero, &ptrValMax, &ptrValMin));
	TEST_ASSERT_EQUAL_PTR(expPtr, ptrVal);
	TEST_ASSERT_EQUAL_PTR(expPtrZero, ptrValZero);
	TEST_ASSERT_EQUAL_PTR(expPtrMax, ptrValMax);
	TEST_ASSERT_EQUAL_PTR(expPtrMin, ptrValMin);

	rewind(filep);

	ptrVal = ptrValZero = ptrValMax = ptrValMin = (void *)1;
	TEST_ASSERT_EQUAL_INT(4, fscanf(filep, format, &ptrVal, &ptrValZero, &ptrValMax, &ptrValMin));
	TEST_ASSERT_EQUAL_PTR(expPtr, ptrVal);
	TEST_ASSERT_EQUAL_PTR(expPtrZero, ptrValZero);
	TEST_ASSERT_EQUAL_PTR(expPtrMax, ptrValMax);
	TEST_ASSERT_EQUAL_PTR(expPtrMin, ptrValMin);

	ptrVal = ptrValZero = ptrValMax = ptrValMin = (void *)1;
	TEST_ASSERT_EQUAL_INT(4, test_vsscanfWrapper(buff, format, &ptrVal, &ptrValZero, &ptrValMax, &ptrValMin));
	TEST_ASSERT_EQUAL_PTR(expPtr, ptrVal);
	TEST_ASSERT_EQUAL_PTR(expPtrZero, ptrValZero);
	TEST_ASSERT_EQUAL_PTR(expPtrMax, ptrValMax);
	TEST_ASSERT_EQUAL_PTR(expPtrMin, ptrValMin);

	ptrVal = ptrValZero = ptrValMax = ptrValMin = (void *)1;
	TEST_ASSERT_EQUAL_INT(4, sscanf(buff, format, &ptrVal, &ptrValZero, &ptrValMax, &ptrValMin));
	TEST_ASSERT_EQUAL_PTR(expPtr, ptrVal);
	TEST_ASSERT_EQUAL_PTR(expPtrZero, ptrValZero);
	TEST_ASSERT_EQUAL_PTR(expPtrMax, ptrValMax);
	TEST_ASSERT_EQUAL_PTR(expPtrMin, ptrValMin);
}


TEST(stdio_scanf_cspn, n)
{
	char buff[BUFF_LEN] = { 0 };
	char res[BUFF_LEN] = { 0 };
	const char *format = "%s %n";
	int counter;

	memset(buff, 'a', sizeof(buff) - 1);

	fprintf(filep, "%s", buff);
	rewind(filep);

	counter = 1;
	test_vfscanfWrapper(filep, format, res, &counter);
	TEST_ASSERT_EQUAL_INT(sizeof(buff) - 1, counter);
	rewind(filep);

	counter = 1;
	TEST_ASSERT_EQUAL_INT(1, fscanf(filep, format, res, &counter));
	TEST_ASSERT_EQUAL_INT(sizeof(buff) - 1, counter);

	counter = 1;
	TEST_ASSERT_EQUAL_INT(1, test_vsscanfWrapper(buff, format, res, &counter));
	TEST_ASSERT_EQUAL_INT(sizeof(buff) - 1, counter);

	counter = 1;
	TEST_ASSERT_EQUAL_INT(1, sscanf(buff, format, res, &counter));
	TEST_ASSERT_EQUAL_INT(sizeof(buff) - 1, counter);
}


/*
////////////////////////////////////////////////////////////////////////////////////////////////////
*/


TEST_SETUP(stdio_scanf_squareBrackets)
{
	filep = fopen(TESTFILE_PATH, "w+");
}


TEST_TEAR_DOWN(stdio_scanf_squareBrackets)
{
	fclose(filep);
}


TEST(stdio_scanf_squareBrackets, simple)
{
	char buff[BUFF_LEN] = "Loremipsumdolorsit";
	char res[BUFF_LEN];
	char *format = "%[Lore]";

	fprintf(filep, "%s", buff);
	rewind(filep);

	memset(res, 0, sizeof(res));
	/* Read input until what is inside brackets match */
	TEST_ASSERT_EQUAL_INT(1, test_vfscanfWrapper(filep, format, res));
	TEST_ASSERT_EQUAL_STRING("Lore", res);

	rewind(filep);

	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(1, fscanf(filep, format, res));
	TEST_ASSERT_EQUAL_STRING("Lore", res);

	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(1, test_vsscanfWrapper(buff, format, res));
	TEST_ASSERT_EQUAL_STRING("Lore", res);

	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(1, sscanf(buff, format, res));
	TEST_ASSERT_EQUAL_STRING("Lore", res);

	rewind(filep);
	format = "%[Lori]";
	/*
	 * Scanf will read filep until elements between brackets and filep match
	 * If it finds incompatibility between them it will stop at the point where it occurred
	 */

	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(1, test_vfscanfWrapper(filep, format, res));
	TEST_ASSERT_EQUAL_STRING("Lor", res);

	rewind(filep);

	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(1, fscanf(filep, format, res));
	TEST_ASSERT_EQUAL_STRING("Lor", res);

	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(1, test_vsscanfWrapper(buff, format, res));
	TEST_ASSERT_EQUAL_STRING("Lor", res);

	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(1, sscanf(buff, format, res));
	TEST_ASSERT_EQUAL_STRING("Lor", res);

	rewind(filep);
	format = "%[Loremipsumdolorsit]";
	/*
	 * Scanf will read filep until the sequence of signs in brackets matches,
	 * in this case, it read the whole string.
	 */

	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(1, test_vfscanfWrapper(filep, format, res));
	TEST_ASSERT_EQUAL_STRING("Loremipsumdolorsit", res);

	rewind(filep);

	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(1, fscanf(filep, format, res));
	TEST_ASSERT_EQUAL_STRING("Loremipsumdolorsit", res);

	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(1, test_vsscanfWrapper(buff, format, res));
	TEST_ASSERT_EQUAL_STRING("Loremipsumdolorsit", res);

	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(1, sscanf(buff, format, res));
	TEST_ASSERT_EQUAL_STRING("Loremipsumdolorsit", res);

	rewind(filep);
	format = "%[x]";
	/*
	 * In this case, scanf is unable to read anything from filep to res
	 * because of 0% coverage what's between brackets and string
	 */

	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(0, test_vfscanfWrapper(filep, format, res));
	rewind(filep);

	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(0, fscanf(filep, format, res));
	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(0, test_vsscanfWrapper(buff, format, res));
	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(0, sscanf(buff, format, res));

	rewind(filep);
	format = "%[t]";
	/*
	 * In this case, scanf is unable to read anything from filep to res
	 * because of 0% coverage what's between brackets and string
	 * "t" is used to check if the read pointer doesn't go over some elements of the string
	 */

	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(0, test_vfscanfWrapper(filep, format, res));

	rewind(filep);

	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(0, fscanf(filep, format, res));
	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(0, test_vsscanfWrapper(buff, format, res));
	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(0, sscanf(buff, format, res));
}


TEST(stdio_scanf_squareBrackets, crircumflex)
{
	char buff[BUFF_LEN] = "Loremipsumdolorsit";
	char res[BUFF_LEN];
	char *format = "%[^x]";

	fprintf(filep, "%s", buff);
	rewind(filep);

	/*
	 * Thanks the circumflex scanf will read everything until find "x"
	 * and store it into res without what was in brackets
	 */

	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(1, test_vfscanfWrapper(filep, format, res));
	TEST_ASSERT_EQUAL_STRING(buff, res);

	rewind(filep);

	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(1, fscanf(filep, format, res));
	TEST_ASSERT_EQUAL_STRING(buff, res);

	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(1, test_vsscanfWrapper(buff, format, res));
	TEST_ASSERT_EQUAL_STRING(buff, res);

	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(1, sscanf(buff, format, res));
	TEST_ASSERT_EQUAL_STRING(buff, res);

	rewind(filep);
	format = "%[^s]";

	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(1, test_vfscanfWrapper(filep, format, res));
	TEST_ASSERT_EQUAL_STRING("Loremip", res);

	rewind(filep);

	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(1, fscanf(filep, format, res));
	TEST_ASSERT_EQUAL_STRING("Loremip", res);

	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(1, test_vsscanfWrapper(buff, format, res));
	TEST_ASSERT_EQUAL_STRING("Loremip", res);

	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(1, sscanf(buff, format, res));
	TEST_ASSERT_EQUAL_STRING("Loremip", res);

	rewind(filep);
	format = "%[^t]";

	memset(res, 0, sizeof(res));
	/* Scanf will read filep until it doesn't find a letter in the brackets */
	TEST_ASSERT_EQUAL_INT(1, test_vfscanfWrapper(filep, format, res));
	TEST_ASSERT_EQUAL_CHAR_ARRAY(buff, res, strlen(buff) - 1);

	rewind(filep);

	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(1, fscanf(filep, format, res));
	TEST_ASSERT_EQUAL_CHAR_ARRAY(buff, res, strlen(buff) - 1);

	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(1, test_vsscanfWrapper(buff, format, res));
	TEST_ASSERT_EQUAL_CHAR_ARRAY(buff, res, strlen(buff) - 1);

	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(1, sscanf(buff, format, res));
	TEST_ASSERT_EQUAL_CHAR_ARRAY(buff, res, strlen(buff) - 1);

	rewind(filep);
	format = "%[^Loremipsumdolorsit]";

	/*
	 * In this case, scanf is unable to read anything from filep to res
	 * because of 100% coverage what's between brackets with option to discard and string
	 */
	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(0, test_vfscanfWrapper(filep, format, res));

	rewind(filep);

	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(0, fscanf(filep, format, res));
	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(0, test_vsscanfWrapper(buff, format, res));
	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(0, sscanf(buff, format, res));

	rewind(filep);
}


TEST(stdio_scanf_squareBrackets, pos)
{
	char buff[BUFF_LEN] = "Loremipsumdolorsit";
	char res[BUFF_LEN] = { 0 };
	char res2[BUFF_LEN] = { 0 };
	char *format = "%[^psu]%s";

	fprintf(filep, "%s", buff);
	rewind(filep);

	/*
	 * Scanf will read the string and match word "psu" as a point of stop and save this in res
	 * After that it will set the file position indicator before "psu"
	 * anything else after that will be saved to "res2"
	 */
	memset(res, 0, sizeof(res));
	memset(res2, 0, sizeof(res2));
	TEST_ASSERT_EQUAL_INT(2, test_vfscanfWrapper(filep, format, res, res2));
	TEST_ASSERT_EQUAL_STRING("Loremi", res);
	TEST_ASSERT_EQUAL_STRING("psumdolorsit", res2);

	rewind(filep);

	memset(res, 0, sizeof(res));
	memset(res2, 0, sizeof(res2));
	TEST_ASSERT_EQUAL_INT(2, fscanf(filep, format, res, res2));
	TEST_ASSERT_EQUAL_STRING("Loremi", res);
	TEST_ASSERT_EQUAL_STRING("psumdolorsit", res2);

	memset(res, 0, sizeof(res));
	memset(res2, 0, sizeof(res2));
	TEST_ASSERT_EQUAL_INT(2, test_vsscanfWrapper(buff, format, res, res2));
	TEST_ASSERT_EQUAL_STRING("Loremi", res);
	TEST_ASSERT_EQUAL_STRING("psumdolorsit", res2);

	memset(res, 0, sizeof(res));
	memset(res2, 0, sizeof(res2));
	TEST_ASSERT_EQUAL_INT(2, sscanf(buff, format, res, res2));
	TEST_ASSERT_EQUAL_STRING("Loremi", res);
	TEST_ASSERT_EQUAL_STRING("psumdolorsit", res2);

	rewind(filep);
	memset(res, 0, sizeof(res));
	memset(res2, 0, sizeof(res2));
	format = "Lor%[^do]%s";

	/*
	 * Scanf will try to find elements after "Lor" and save them to res until first
	 * occurrence of "do" everything after that with the content of [] will be saved to "res2"
	 */
	memset(res, 0, sizeof(res));
	memset(res2, 0, sizeof(res2));
	TEST_ASSERT_EQUAL_INT(2, test_vfscanfWrapper(filep, format, res, res2));
	TEST_ASSERT_EQUAL_STRING("emipsum", res);
	TEST_ASSERT_EQUAL_STRING("dolorsit", res2);

	rewind(filep);

	memset(res, 0, sizeof(res));
	memset(res2, 0, sizeof(res2));
	TEST_ASSERT_EQUAL_INT(2, fscanf(filep, format, res, res2));
	TEST_ASSERT_EQUAL_STRING("emipsum", res);
	TEST_ASSERT_EQUAL_STRING("dolorsit", res2);

	memset(res, 0, sizeof(res));
	memset(res2, 0, sizeof(res2));
	TEST_ASSERT_EQUAL_INT(2, test_vsscanfWrapper(buff, format, res, res2));
	TEST_ASSERT_EQUAL_STRING("emipsum", res);
	TEST_ASSERT_EQUAL_STRING("dolorsit", res2);

	memset(res, 0, sizeof(res));
	memset(res2, 0, sizeof(res2));
	TEST_ASSERT_EQUAL_INT(2, sscanf(buff, format, res, res2));
	TEST_ASSERT_EQUAL_STRING("emipsum", res);
	TEST_ASSERT_EQUAL_STRING("dolorsit", res2);
}


TEST(stdio_scanf_squareBrackets, white_spaces)
{
	char buff[BUFF_LEN] = "Lorem Ipsum Dolor SitAmet ,VESTIBULUM123ANTEIPSUMPRIMIS/0x0005";
	char buffMod[25] = "\n\t\v\f\r";
	char *format = "%[^\n-\t-\v-\f-\r]%*c";

	char res[BUFF_LEN] = { 0 };

	fprintf(filep, "%s", buff);
	rewind(filep);

	/*
	 * Scanf will try to find an occurrence of [] content and store it in res
	 * After that it will discard all chars after [] content
	 */
	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(1, test_vfscanfWrapper(filep, format, res));
	TEST_ASSERT_EQUAL_STRING(buff, res);

	rewind(filep);

	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(1, fscanf(filep, format, res));
	TEST_ASSERT_EQUAL_STRING(buff, res);

	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(1, test_vsscanfWrapper(buff, format, res));
	TEST_ASSERT_EQUAL_STRING(buff, res);

	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(1, sscanf(buff, format, res));
	TEST_ASSERT_EQUAL_STRING(buff, res);

/* Test disabled because of issue: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/679 */
#ifdef __phoenix__
	TEST_IGNORE();
#endif

	fclose(filep);
	filep = fopen(TESTFILE_PATH, "w+");
	format = "%[\n\t\v\f\r]";

	fprintf(filep, "%s", buffMod);
	rewind(filep);

	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(1, test_vfscanfWrapper(filep, format, res));
	TEST_ASSERT_EQUAL_CHAR_ARRAY(buffMod, res, 5);

	rewind(filep);

	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(1, fscanf(filep, format, res));
	TEST_ASSERT_EQUAL_CHAR_ARRAY(buffMod, res, 5);

	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(1, test_vsscanfWrapper(buffMod, format, res));
	TEST_ASSERT_EQUAL_CHAR_ARRAY(buffMod, res, 5);

	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(1, sscanf(buffMod, format, res));
	TEST_ASSERT_EQUAL_CHAR_ARRAY(buffMod, res, 5);
}


TEST(stdio_scanf_squareBrackets, ascii)
{
	char buff[BUFF_LEN - 10] = { 0 };
	char format[BUFF_LEN] = "%[^\n]%*c";
	char res[BUFF_LEN];
	int i;

	for (i = 1; i < 127; i++) {
		if (i == 10) {
			buff[i - 1] = i - 1;
		}
		else {
			buff[i - 1] = i;
		}
	}

	buff[i] = '\0';

	fprintf(filep, "%s", buff);
	rewind(filep);

	/*
	 * Scanf will try to find an occurrence of [] content and store it in res
	 * After that it will discard all chars after [] content
	 */
	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(1, test_vfscanfWrapper(filep, format, res));
	TEST_ASSERT_EQUAL_STRING(buff, res);
	rewind(filep);

	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(1, fscanf(filep, format, res));
	TEST_ASSERT_EQUAL_STRING(buff, res);

	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(1, test_vsscanfWrapper(buff, format, res));
	TEST_ASSERT_EQUAL_STRING(buff, res);

	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(1, sscanf(buff, format, res));
	TEST_ASSERT_EQUAL_STRING(buff, res);


	fclose(filep);
	filep = fopen(TESTFILE_PATH, "w+");


	memset(format, 0, sizeof(format));
	memset(buff, 0, sizeof(buff));
	memset(res, 0, sizeof(res));
	for (i = 1; i < 127; i++) {
		/* Bypass "]" because this sign end brackets formatension */
		if (i == 93) {
			buff[i - 1] = i - 1;
		}

		/* Bypass \n because of issue: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/679 */
#ifdef __phoenix__
		else if (i == 10) {
			buff[i - 1] = i - 1;
		}
#endif

		else {
			buff[i - 1] = i;
		}
	}

	fprintf(filep, "%s", buff);
	rewind(filep);

	sprintf(format, "%%[%s]", buff);

	/*
	 * Scanf will try to find all elements of ascii table in filep
	 * (Without ] sign because it equals the end of format) and store it in res
	 */
	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(1, test_vfscanfWrapper(filep, format, res));
	TEST_ASSERT_EQUAL_STRING(buff, res);

	rewind(filep);

	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(1, fscanf(filep, format, res));
	TEST_ASSERT_EQUAL_STRING(buff, res);

	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(1, test_vsscanfWrapper(buff, format, res));
	TEST_ASSERT_EQUAL_STRING(buff, res);

	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(1, sscanf(buff, format, res));
	TEST_ASSERT_EQUAL_STRING(buff, res);
}


TEST(stdio_scanf_squareBrackets, ranges)
{
	const char *buff = "loremIPSUM IPSUMdolor dolorSitAmet";
	const char *buff2 = "123loremIPSUM IPSUMdolor123 dolor123SitAmet";
	char res[4][BUFF_LEN / 3];
	char *format = "%[A-z] %[A-Z] %[a-z] %[A-z]";

	fprintf(filep, "%s", buff);
	rewind(filep);

	/*
	 * In this situation scanf will search all elements in the range declared
	 * inside brackets and stop right after it will be out of range.
	 * (after finding a white sign in this case but if it will face a matching pattern with space
	 * it will continue matching and stop right after the nonmatching element. The cursor will be placed
	 * on the last white space to match further part of the pattern)
	 */
	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(4, fscanf(filep, format, res[0], res[1], res[2], res[3]));
	TEST_ASSERT_EQUAL_STRING("loremIPSUM", res[0]);
	TEST_ASSERT_EQUAL_STRING("IPSUM", res[1]);
	TEST_ASSERT_EQUAL_STRING("dolor", res[2]);
	TEST_ASSERT_EQUAL_STRING("dolorSitAmet", res[3]);

	rewind(filep);

	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(4, test_vfscanfWrapper(filep, format, res[0], res[1], res[2], res[3]));
	TEST_ASSERT_EQUAL_STRING("loremIPSUM", res[0]);
	TEST_ASSERT_EQUAL_STRING("IPSUM", res[1]);
	TEST_ASSERT_EQUAL_STRING("dolor", res[2]);
	TEST_ASSERT_EQUAL_STRING("dolorSitAmet", res[3]);

	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(4, test_vsscanfWrapper(buff, format, res[0], res[1], res[2], res[3]));
	TEST_ASSERT_EQUAL_STRING("loremIPSUM", res[0]);
	TEST_ASSERT_EQUAL_STRING("IPSUM", res[1]);
	TEST_ASSERT_EQUAL_STRING("dolor", res[2]);
	TEST_ASSERT_EQUAL_STRING("dolorSitAmet", res[3]);

	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(4, sscanf(buff, format, res[0], res[1], res[2], res[3]));
	TEST_ASSERT_EQUAL_STRING("loremIPSUM", res[0]);
	TEST_ASSERT_EQUAL_STRING("IPSUM", res[1]);
	TEST_ASSERT_EQUAL_STRING("dolor", res[2]);
	TEST_ASSERT_EQUAL_STRING("dolorSitAmet", res[3]);

	fclose(filep);

	filep = fopen(TESTFILE_PATH, "w+");
	format = "%[1-9] %[^1-9] %[1-9] %[A-z1-9]";
	fprintf(filep, "%s", buff2);
	rewind(filep);

	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(4, fscanf(filep, format, res[0], res[1], res[2], res[3]));
	TEST_ASSERT_EQUAL_STRING("123", res[0]);
	TEST_ASSERT_EQUAL_STRING("loremIPSUM IPSUMdolor", res[1]);
	TEST_ASSERT_EQUAL_STRING("123", res[2]);
	TEST_ASSERT_EQUAL_STRING("dolor123SitAmet", res[3]);

	rewind(filep);

	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(4, test_vfscanfWrapper(filep, format, res[0], res[1], res[2], res[3]));
	TEST_ASSERT_EQUAL_STRING("123", res[0]);
	TEST_ASSERT_EQUAL_STRING("loremIPSUM IPSUMdolor", res[1]);
	TEST_ASSERT_EQUAL_STRING("123", res[2]);
	TEST_ASSERT_EQUAL_STRING("dolor123SitAmet", res[3]);

	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(4, test_vsscanfWrapper(buff2, format, res[0], res[1], res[2], res[3]));
	TEST_ASSERT_EQUAL_STRING("123", res[0]);
	TEST_ASSERT_EQUAL_STRING("loremIPSUM IPSUMdolor", res[1]);
	TEST_ASSERT_EQUAL_STRING("123", res[2]);
	TEST_ASSERT_EQUAL_STRING("dolor123SitAmet", res[3]);

	memset(res, 0, sizeof(res));
	TEST_ASSERT_EQUAL_INT(4, sscanf(buff2, format, res[0], res[1], res[2], res[3]));
	TEST_ASSERT_EQUAL_STRING("123", res[0]);
	TEST_ASSERT_EQUAL_STRING("loremIPSUM IPSUMdolor", res[1]);
	TEST_ASSERT_EQUAL_STRING("123", res[2]);
	TEST_ASSERT_EQUAL_STRING("dolor123SitAmet", res[3]);
}


/*
////////////////////////////////////////////////////////////////////////////////////////////////////
*/


TEST_SETUP(stdio_scanf_rest)
{
	filep = fopen(TESTFILE_PATH, "w+");
}


TEST_TEAR_DOWN(stdio_scanf_rest)
{
	fclose(filep);
}


TEST(stdio_scanf_rest, modifiers_mix)
{
	int int1, int2, int3, int4;
	unsigned int res2;
	float flt1, flt2, flt3;
	long long int llint1;
	char char1;
	unsigned char uchar1;
	ptrdiff_t ptr1;
	char strTab[4][MAX_TESTSTR_WORDLEN] = { 0 };
	const char *str = "~~1`2l 0.1!_2@lorem#0x233$ 1.2e-5 % nowy 1.200020e-5 nal^ 132 *{}:|?><[]',./5/123456/+56-a(loremipsum\0)";
	const char *format = "~~%d`%ul %f!_%x@%[lorem]#%p$%a %% %s %e nal^ %i *{}:|?><[]',./%o/%lld/+%hhx-%c(%[^ipsum]%s)";

	fprintf(filep, "%s", str);
	rewind(filep);

	ptr1 = int1 = int2 = int3 = int4 = res2 = flt1 = flt2 = flt3 = llint1 = char1 = uchar1 = 1;
	memset(strTab, 0, sizeof(strTab));
	TEST_ASSERT_EQUAL_INT(16, sscanf(str, format, &int1, &res2, &flt1, &int2, strTab[0], &ptr1, &flt2, strTab[1], &flt3, &int3, &int4, &llint1, &uchar1, &char1, strTab[2], strTab[3]));

	TEST_ASSERT_EQUAL_INT(1, int1);
	TEST_ASSERT_EQUAL_UINT(2, res2);
	TEST_ASSERT_EQUAL_FLOAT(0.1, flt1);
	TEST_ASSERT_EQUAL_HEX(2, int2);
	TEST_ASSERT_EQUAL_STRING("lorem", strTab[0]);
	TEST_ASSERT_EQUAL_INT64(563, ptr1);
	TEST_ASSERT_EQUAL_FLOAT(1.200000e-05, flt2);
	TEST_ASSERT_EQUAL_STRING("nowy", strTab[1]);
	TEST_ASSERT_EQUAL_FLOAT(1.200020e-05, flt3);
	TEST_ASSERT_EQUAL_UINT(132, int3);
	TEST_ASSERT_EQUAL_INT(5, int4);
	TEST_ASSERT_EQUAL_INT64(123456, llint1);
	TEST_ASSERT_EQUAL_HEX8(0x56, uchar1);
	TEST_ASSERT_EQUAL_CHAR('a', char1);
	TEST_ASSERT_EQUAL_STRING("lore", strTab[2]);
	TEST_ASSERT_EQUAL_STRING("mipsum", strTab[3]);

	ptr1 = int1 = int2 = int3 = int4 = res2 = flt1 = flt2 = flt3 = llint1 = char1 = 1;
	memset(strTab, 0, sizeof(strTab));
	TEST_ASSERT_EQUAL_INT(16, fscanf(filep, format, &int1, &res2, &flt1, &int2, strTab[0], &ptr1, &flt2, strTab[1], &flt3, &int3, &int4, &llint1, &uchar1, &char1, strTab[2], strTab[3]));

	TEST_ASSERT_EQUAL_INT(1, int1);
	TEST_ASSERT_EQUAL_UINT(2, res2);
	TEST_ASSERT_EQUAL_FLOAT(0.1, flt1);
	TEST_ASSERT_EQUAL_HEX(2, int2);
	TEST_ASSERT_EQUAL_STRING("lorem", strTab[0]);
	TEST_ASSERT_EQUAL_INT64(563, ptr1);
	TEST_ASSERT_EQUAL_FLOAT(1.200000e-05, flt2);
	TEST_ASSERT_EQUAL_STRING("nowy", strTab[1]);
	TEST_ASSERT_EQUAL_FLOAT(1.200020e-05, flt3);
	TEST_ASSERT_EQUAL_UINT(132, int3);
	TEST_ASSERT_EQUAL_INT(5, int4);
	TEST_ASSERT_EQUAL_INT64(123456, llint1);
	TEST_ASSERT_EQUAL_HEX8(0x56, uchar1);
	TEST_ASSERT_EQUAL_CHAR('a', char1);
	TEST_ASSERT_EQUAL_STRING("lore", strTab[2]);
	TEST_ASSERT_EQUAL_STRING("mipsum", strTab[3]);

	ptr1 = int1 = int2 = int3 = int4 = res2 = flt1 = flt2 = flt3 = llint1 = char1 = uchar1 = 1;
	memset(strTab, 0, sizeof(strTab));
	rewind(filep);
	TEST_ASSERT_EQUAL_INT(16, test_vfscanfWrapper(filep, format, &int1, &res2, &flt1, &int2, strTab[0], &ptr1, &flt2, strTab[1], &flt3, &int3, &int4, &llint1, &uchar1, &char1, strTab[2], strTab[3]));

	TEST_ASSERT_EQUAL_INT(1, int1);
	TEST_ASSERT_EQUAL_UINT(2, res2);
	TEST_ASSERT_EQUAL_FLOAT(0.1, flt1);
	TEST_ASSERT_EQUAL_HEX(2, int2);
	TEST_ASSERT_EQUAL_STRING("lorem", strTab[0]);
	TEST_ASSERT_EQUAL_INT64(563, ptr1);
	TEST_ASSERT_EQUAL_FLOAT(1.200000e-05, flt2);
	TEST_ASSERT_EQUAL_STRING("nowy", strTab[1]);
	TEST_ASSERT_EQUAL_FLOAT(1.200020e-05, flt3);
	TEST_ASSERT_EQUAL_UINT(132, int3);
	TEST_ASSERT_EQUAL_INT(5, int4);
	TEST_ASSERT_EQUAL_INT64(123456, llint1);
	TEST_ASSERT_EQUAL_HEX8(0x56, uchar1);
	TEST_ASSERT_EQUAL_CHAR('a', char1);
	TEST_ASSERT_EQUAL_STRING("lore", strTab[2]);
	TEST_ASSERT_EQUAL_STRING("mipsum", strTab[3]);

	ptr1 = int1 = int2 = int3 = int4 = res2 = flt1 = flt2 = flt3 = llint1 = char1 = uchar1 = 1;
	memset(strTab, 0, sizeof(strTab));
	TEST_ASSERT_EQUAL_INT(16, test_vsscanfWrapper(str, format, &int1, &res2, &flt1, &int2, strTab[0], &ptr1, &flt2, strTab[1], &flt3, &int3, &int4, &llint1, &uchar1, &char1, strTab[2], strTab[3]));

	TEST_ASSERT_EQUAL_INT(1, int1);
	TEST_ASSERT_EQUAL_UINT(2, res2);
	TEST_ASSERT_EQUAL_FLOAT(0.1, flt1);
	TEST_ASSERT_EQUAL_HEX(2, int2);
	TEST_ASSERT_EQUAL_STRING("lorem", strTab[0]);
	TEST_ASSERT_EQUAL_INT64(563, ptr1);
	TEST_ASSERT_EQUAL_FLOAT(1.200000e-05, flt2);
	TEST_ASSERT_EQUAL_STRING("nowy", strTab[1]);
	TEST_ASSERT_EQUAL_FLOAT(1.200020e-05, flt3);
	TEST_ASSERT_EQUAL_UINT(132, int3);
	TEST_ASSERT_EQUAL_INT(5, int4);
	TEST_ASSERT_EQUAL_INT64(123456, llint1);
	TEST_ASSERT_EQUAL_HEX8(0x56, uchar1);
	TEST_ASSERT_EQUAL_CHAR('a', char1);
	TEST_ASSERT_EQUAL_STRING("lore", strTab[2]);
	TEST_ASSERT_EQUAL_STRING("mipsum", strTab[3]);
}


TEST(stdio_scanf_rest, m_s)
{
/* Disabled because of issue: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/667 */
#ifdef __phoenix__
	TEST_IGNORE();
#endif

	const char *lorem = "LoremIpsumDolorSitAmet,Vestibulum";
	char *res, chrArray[BUFF_LEN];
	int i;
	const char *format = "%ms";

	for (i = 0; i < BUFF_LEN - 1; i++) {
		chrArray[i] = 'a';
	}
	chrArray[i] = '\0';

	fprintf(filep, "%s", lorem);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(1, fscanf(filep, format, &res));
	TEST_ASSERT_EQUAL_STRING(lorem, res);
	free(res);

	TEST_ASSERT_EQUAL_INT(1, sscanf(lorem, format, &res));
	TEST_ASSERT_EQUAL_STRING(lorem, res);
	free(res);

	rewind(filep);

	test_vfscanfWrapper(filep, format, &res);
	TEST_ASSERT_EQUAL_STRING(lorem, res);
	free(res);

	TEST_ASSERT_EQUAL_INT(1, test_vsscanfWrapper(lorem, format, &res));
	TEST_ASSERT_EQUAL_STRING(lorem, res);
	free(res);

	fclose(filep);
	filep = fopen(TESTFILE_PATH, "w+");

	fprintf(filep, "%s", chrArray);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(1, fscanf(filep, format, &res));
	TEST_ASSERT_EQUAL_STRING(chrArray, res);
	free(res);

	TEST_ASSERT_EQUAL_INT(1, sscanf(chrArray, format, &res));
	TEST_ASSERT_EQUAL_STRING(chrArray, res);
	free(res);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(1, test_vfscanfWrapper(filep, format, &res));
	TEST_ASSERT_EQUAL_STRING(chrArray, res);
	free(res);

	TEST_ASSERT_EQUAL_INT(1, test_vsscanfWrapper(chrArray, format, &res));
	TEST_ASSERT_EQUAL_STRING(chrArray, res);
	free(res);
}


TEST(stdio_scanf_rest, m_brackets)
{
/* Disabled because of issue: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/667 */
#ifdef __phoenix__
	TEST_IGNORE();
#endif

	const char *buff = "loremIPSUM IPSUMdolor dolorSitAmet";
	const char *buff2 = "123loremIPSUM IPSUMdolor123 dolor123SitAmet";
	char *res1, *res2, *res3;
	char *format = "%m[A-z] %m[A-Z] %m[a-z]";

	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(3, fscanf(filep, format, &res1, &res2, &res3));
	TEST_ASSERT_EQUAL_STRING("loremIPSUM", res1);
	TEST_ASSERT_EQUAL_STRING("IPSUM", res2);
	TEST_ASSERT_EQUAL_STRING("dolor", res3);
	free(res1);
	free(res2);
	free(res3);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(3, test_vfscanfWrapper(filep, format, &res1, &res2, &res3));
	TEST_ASSERT_EQUAL_STRING("loremIPSUM", res1);
	TEST_ASSERT_EQUAL_STRING("IPSUM", res2);
	TEST_ASSERT_EQUAL_STRING("dolor", res3);
	free(res1);
	free(res2);
	free(res3);

	TEST_ASSERT_EQUAL_INT(3, test_vsscanfWrapper(buff, format, &res1, &res2, &res3));
	TEST_ASSERT_EQUAL_STRING("loremIPSUM", res1);
	TEST_ASSERT_EQUAL_STRING("IPSUM", res2);
	TEST_ASSERT_EQUAL_STRING("dolor", res3);
	free(res1);
	free(res2);
	free(res3);

	TEST_ASSERT_EQUAL_INT(3, sscanf(buff, format, &res1, &res2, &res3));
	TEST_ASSERT_EQUAL_STRING("loremIPSUM", res1);
	TEST_ASSERT_EQUAL_STRING("IPSUM", res2);
	TEST_ASSERT_EQUAL_STRING("dolor", res3);
	free(res1);
	free(res2);
	free(res3);

	fclose(filep);
	filep = fopen(TESTFILE_PATH, "w+");
	format = "%m[1-9] %m[^1-9] %m[1-9]";

	fprintf(filep, "%s", buff2);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(3, fscanf(filep, format, &res1, &res2, &res3));
	TEST_ASSERT_EQUAL_STRING("123", res1);
	TEST_ASSERT_EQUAL_STRING("loremIPSUM IPSUMdolor", res2);
	TEST_ASSERT_EQUAL_STRING("123", res3);
	free(res1);
	free(res2);
	free(res3);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(3, test_vfscanfWrapper(filep, format, &res1, &res2, &res3));
	TEST_ASSERT_EQUAL_STRING("123", res1);
	TEST_ASSERT_EQUAL_STRING("loremIPSUM IPSUMdolor", res2);
	TEST_ASSERT_EQUAL_STRING("123", res3);
	free(res1);
	free(res2);
	free(res3);

	TEST_ASSERT_EQUAL_INT(3, test_vsscanfWrapper(buff2, format, &res1, &res2, &res3));
	TEST_ASSERT_EQUAL_STRING("123", res1);
	TEST_ASSERT_EQUAL_STRING("loremIPSUM IPSUMdolor", res2);
	TEST_ASSERT_EQUAL_STRING("123", res3);
	free(res1);
	free(res2);
	free(res3);

	TEST_ASSERT_EQUAL_INT(3, sscanf(buff2, format, &res1, &res2, &res3));
	TEST_ASSERT_EQUAL_STRING("123", res1);
	TEST_ASSERT_EQUAL_STRING("loremIPSUM IPSUMdolor", res2);
	TEST_ASSERT_EQUAL_STRING("123", res3);
	free(res1);
	free(res2);
	free(res3);
}


__attribute__((no_sanitize_address))
TEST(stdio_scanf_rest, m_c)
{
/* Disabled because of issue: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/667 */
#ifdef __phoenix__
	TEST_IGNORE();
#endif

	/* Address sanitizer used on Ubuntu 22.04 fails in such cases, because of the following issue:
	 * https://github.com/llvm/llvm-project/issues/61768
	 */
#if defined(__SANITIZE_ADDRESS__)
	TEST_IGNORE();
#endif

	const char buff[] = "Lor";
	char *res1 = NULL, *res2 = NULL, *res3 = NULL;

	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_NULL(res1);
	TEST_ASSERT_NULL(res2);
	TEST_ASSERT_NULL(res3);

	TEST_ASSERT_EQUAL_INT(3, sscanf(buff, "%mc%mc%mc", &res1, &res2, &res3));

	TEST_ASSERT_NOT_NULL(res1);
	TEST_ASSERT_NOT_NULL(res2);
	TEST_ASSERT_NOT_NULL(res3);

	TEST_ASSERT_EQUAL_CHAR(buff[0], res1[0]);
	TEST_ASSERT_EQUAL_CHAR(buff[1], res2[0]);
	TEST_ASSERT_EQUAL_CHAR(buff[2], res3[0]);

	free(res1);
	free(res2);
	free(res3);
}


TEST(stdio_scanf_rest, star)
{
	const char *buff = "Lorem Ipsum Dolor 123 SitAmet c 123 0x233";
	char res1[MAX_TESTSTR_WORDLEN], res;
	int val;
	const char *format = "%*s %*s %s %d %*s %c %*d %*x";

	fprintf(filep, "%s", buff);
	rewind(filep);

	res = val = 0;
	memset(res1, 0, sizeof(res1));
	TEST_ASSERT_EQUAL_INT(3, fscanf(filep, format, res1, &val, &res));
	TEST_ASSERT_EQUAL_STRING("Dolor", res1);
	TEST_ASSERT_EQUAL_INT(123, val);
	TEST_ASSERT_EQUAL_CHAR('c', res);

	rewind(filep);

	res = val = 0;
	memset(res1, 0, sizeof(res1));
	TEST_ASSERT_EQUAL_INT(3, test_vfscanfWrapper(filep, format, res1, &val, &res));
	TEST_ASSERT_EQUAL_STRING("Dolor", res1);
	TEST_ASSERT_EQUAL_INT(123, val);
	TEST_ASSERT_EQUAL_CHAR('c', res);

	res = val = 0;
	memset(res1, 0, sizeof(res1));
	TEST_ASSERT_EQUAL_INT(3, sscanf(buff, format, res1, &val, &res));
	TEST_ASSERT_EQUAL_STRING("Dolor", res1);
	TEST_ASSERT_EQUAL_INT(123, val);
	TEST_ASSERT_EQUAL_CHAR('c', res);

	res = val = 0;
	memset(res1, 0, sizeof(res1));
	TEST_ASSERT_EQUAL_INT(3, test_vsscanfWrapper(buff, format, res1, &val, &res));
	TEST_ASSERT_EQUAL_STRING("Dolor", res1);
	TEST_ASSERT_EQUAL_INT(123, val);
	TEST_ASSERT_EQUAL_CHAR('c', res);
}

TEST(stdio_scanf_rest, field_width)
{
	char buff[BUFF_LEN], valStr[BUFF_LEN], str[] = "LoreIpsumDolorSitAmet";
	int intMax = 2147483647, intMin = -2147483647, valIntMin, valIntMax;
	float fltMax = 3.40282347e+7F, fltMin = 3.40282347e-4F, valFltMin, valFltMax;
	char *format = "%4s %*s %5d %*d %5d %*d %5f %*f %f";

	sprintf(buff, "%s %d %d %f %f", str, intMax, intMin, fltMax, fltMin);
	fprintf(filep, "%s", buff);
	rewind(filep);

	memset(valStr, 0, sizeof(valStr));
	valIntMin = valIntMax = 1;
	valFltMin = valFltMax = 1.0;
	TEST_ASSERT_EQUAL_INT(5, fscanf(filep, format, valStr, &valIntMax, &valIntMin, &valFltMax, &valFltMin));
	TEST_ASSERT_EQUAL_CHAR_ARRAY(str, valStr, strlen(valStr));
	TEST_ASSERT_EQUAL_INT(21474, valIntMax);
	TEST_ASSERT_EQUAL_INT(-2147, valIntMin);
	TEST_ASSERT_EQUAL_FLOAT(34028.000000, valFltMax);
	TEST_ASSERT_EQUAL_FLOAT(0.000340, valFltMin);

	rewind(filep);
	memset(valStr, 0, sizeof(valStr));
	valIntMin = valIntMax = 1;
	valFltMin = valFltMax = 1.0;
	TEST_ASSERT_EQUAL_INT(5, test_vfscanfWrapper(filep, format, valStr, &valIntMax, &valIntMin, &valFltMax, &valFltMin));
	TEST_ASSERT_EQUAL_CHAR_ARRAY(str, valStr, strlen(valStr));
	TEST_ASSERT_EQUAL_INT(21474, valIntMax);
	TEST_ASSERT_EQUAL_INT(-2147, valIntMin);
	TEST_ASSERT_EQUAL_FLOAT(34028.000000, valFltMax);
	TEST_ASSERT_EQUAL_FLOAT(0.000340, valFltMin);

	memset(valStr, 0, sizeof(valStr));
	valIntMin = valIntMax = 1;
	valFltMin = valFltMax = 1.0;
	TEST_ASSERT_EQUAL_INT(5, sscanf(buff, format, valStr, &valIntMax, &valIntMin, &valFltMax, &valFltMin));
	TEST_ASSERT_EQUAL_CHAR_ARRAY(str, valStr, strlen(valStr));
	TEST_ASSERT_EQUAL_INT(21474, valIntMax);
	TEST_ASSERT_EQUAL_INT(-2147, valIntMin);
	TEST_ASSERT_EQUAL_FLOAT(34028.000000, valFltMax);
	TEST_ASSERT_EQUAL_FLOAT(0.000340, valFltMin);

	memset(valStr, 0, sizeof(valStr));
	valIntMin = valIntMax = 1;
	valFltMin = valFltMax = 1.0;
	TEST_ASSERT_EQUAL_INT(5, test_vsscanfWrapper(buff, format, valStr, &valIntMax, &valIntMin, &valFltMax, &valFltMin));
	TEST_ASSERT_EQUAL_CHAR_ARRAY(str, valStr, strlen(valStr));
	TEST_ASSERT_EQUAL_INT(21474, valIntMax);
	TEST_ASSERT_EQUAL_INT(-2147, valIntMin);
	TEST_ASSERT_EQUAL_FLOAT(34028.000000, valFltMax);
	TEST_ASSERT_EQUAL_FLOAT(0.000340, valFltMin);

	rewind(filep);
	format = "%*4s %s";

	memset(valStr, 0, sizeof(valStr));
	TEST_ASSERT_EQUAL_INT(1, fscanf(filep, format, valStr));
	TEST_ASSERT_EQUAL_STRING("IpsumDolorSitAmet", valStr);

	rewind(filep);

	memset(valStr, 0, sizeof(valStr));
	TEST_ASSERT_EQUAL_INT(1, test_vfscanfWrapper(filep, format, valStr));
	TEST_ASSERT_EQUAL_STRING("IpsumDolorSitAmet", valStr);

	memset(valStr, 0, sizeof(valStr));
	TEST_ASSERT_EQUAL_INT(1, sscanf(buff, format, valStr));
	TEST_ASSERT_EQUAL_STRING("IpsumDolorSitAmet", valStr);

	memset(valStr, 0, sizeof(valStr));
	TEST_ASSERT_EQUAL_INT(1, test_vsscanfWrapper(buff, format, valStr));
	TEST_ASSERT_EQUAL_STRING("IpsumDolorSitAmet", valStr);
}


/*
////////////////////////////////////////////////////////////////////////////////////////////////////
*/


TEST_GROUP_RUNNER(stdio_scanf_aefg)
{
	RUN_TEST_CASE(stdio_scanf_aefg, f);
	RUN_TEST_CASE(stdio_scanf_aefg, F);
	RUN_TEST_CASE(stdio_scanf_aefg, a);
	RUN_TEST_CASE(stdio_scanf_aefg, A);
	RUN_TEST_CASE(stdio_scanf_aefg, e);
	RUN_TEST_CASE(stdio_scanf_aefg, E);
	RUN_TEST_CASE(stdio_scanf_aefg, g);
	RUN_TEST_CASE(stdio_scanf_aefg, G);
	RUN_TEST_CASE(stdio_scanf_aefg, inf_nan_f);
	RUN_TEST_CASE(stdio_scanf_aefg, inf_nan_a);
	RUN_TEST_CASE(stdio_scanf_aefg, inf_nan_e);
	RUN_TEST_CASE(stdio_scanf_aefg, inf_nan_g);
	remove(TESTFILE_PATH);
}


TEST_GROUP_RUNNER(stdio_scanf_cspn)
{
	RUN_TEST_CASE(stdio_scanf_cspn, c);
	RUN_TEST_CASE(stdio_scanf_cspn, c_ascii);
	RUN_TEST_CASE(stdio_scanf_cspn, s_path);
	RUN_TEST_CASE(stdio_scanf_cspn, s_torn);
	RUN_TEST_CASE(stdio_scanf_cspn, s_ascii);
	RUN_TEST_CASE(stdio_scanf_cspn, s_pick);
	RUN_TEST_CASE(stdio_scanf_cspn, percent);
	RUN_TEST_CASE(stdio_scanf_cspn, n);
	RUN_TEST_CASE(stdio_scanf_cspn, ptr);
	remove(TESTFILE_PATH);
}


TEST_GROUP_RUNNER(stdio_scanf_squareBrackets)
{
	RUN_TEST_CASE(stdio_scanf_squareBrackets, simple);
	RUN_TEST_CASE(stdio_scanf_squareBrackets, crircumflex);
	RUN_TEST_CASE(stdio_scanf_squareBrackets, pos);
	RUN_TEST_CASE(stdio_scanf_squareBrackets, white_spaces);
	RUN_TEST_CASE(stdio_scanf_squareBrackets, ascii);
	RUN_TEST_CASE(stdio_scanf_squareBrackets, ranges)
	remove(TESTFILE_PATH);
}


TEST_GROUP_RUNNER(stdio_scanf_rest)
{
	RUN_TEST_CASE(stdio_scanf_rest, modifiers_mix);
	RUN_TEST_CASE(stdio_scanf_rest, m_s);
	RUN_TEST_CASE(stdio_scanf_rest, m_brackets);
	RUN_TEST_CASE(stdio_scanf_rest, m_c);
	RUN_TEST_CASE(stdio_scanf_rest, star);
	RUN_TEST_CASE(stdio_scanf_rest, field_width);
	remove(TESTFILE_PATH);
}

void runner(void)
{
	RUN_TEST_GROUP(stdio_scanf_aefg);
	RUN_TEST_GROUP(stdio_scanf_cspn);
	RUN_TEST_GROUP(stdio_scanf_squareBrackets);
	RUN_TEST_GROUP(stdio_scanf_rest);
}

int main(int argc, char *argv[])
{
	return (UnityMain(argc, (const char **)argv, runner) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
