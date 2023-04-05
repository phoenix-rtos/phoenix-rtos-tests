/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - string.h
 * TESTED:
 *    - fscanf()
 *    - sscanf()
 *    - vfscanf()
 *    - vsscanf()
 *
 * Copyright 2023 Phoenix Systems
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


/*
 * All tests for floats are disabled in this test of
 * issues #652 and #634 which cause problems with scanning these values.
 * #652 https://github.com/phoenix-rtos/phoenix-rtos-project/issues/652
 * #634 https://github.com/phoenix-rtos/phoenix-rtos-project/issues/634
 */


/* Disabled because of #656 issue */
#ifndef __phoenix__
#include <float.h>
#endif

#include <unity_fixture.h>

#include "common.h"




#define TESTFILE_PATH "stdio_fscanf_test"
/* Size enough to hold most of data types int/ptrdif/float(in other formats than %f%F and %lf%lF)/str */
#define BUFF_LEN 256
/* Size big enough for string contains floats (long notation) */
#define BUFF_LEN_FLOAT 290

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


TEST_GROUP(stdio_scanf_d);
TEST_GROUP(stdio_scanf_i);
TEST_GROUP(stdio_scanf_u);
TEST_GROUP(stdio_scanf_o);
TEST_GROUP(stdio_scanf_x);
TEST_GROUP(stdio_scanf_aefg);


/*
///////////////////////////////////////////////////////////////////////////////////////////////
*/


TEST_SETUP(stdio_scanf_d)
{
	filep = fopen(TESTFILE_PATH, "w+");
}


TEST_TEAR_DOWN(stdio_scanf_d)
{
	fclose(filep);
}


TEST(stdio_scanf_d, d)
{
	char buff[BUFF_LEN] = { 0 };
	int max, min, zero, hmin, hmax;
	const char *format = "%d %d %d %d %d";

	sprintf(buff, format, INT_MAX, INT_MAX / 2, 0, INT_MIN / 2, INT_MIN);
	fprintf(filep, "%s", buff);
	rewind(filep);

	test_vfscanfWrapper(filep, format, &max, &hmax, &zero, &hmin, &min);
	TEST_ASSERT_EQUAL_INT(INT_MIN, min);
	TEST_ASSERT_EQUAL_INT(INT_MAX, max);
	TEST_ASSERT_EQUAL_INT(0, zero);
	TEST_ASSERT_EQUAL_INT(INT_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT(INT_MAX / 2, hmax);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(5, fscanf(filep, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT(INT_MIN, min);
	TEST_ASSERT_EQUAL_INT(INT_MAX, max);
	TEST_ASSERT_EQUAL_INT(0, zero);
	TEST_ASSERT_EQUAL_INT(INT_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT(INT_MAX / 2, hmax);

	TEST_ASSERT_EQUAL_INT(5, test_vsscanfWrapper(buff, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT(INT_MIN, min);
	TEST_ASSERT_EQUAL_INT(INT_MAX, max);
	TEST_ASSERT_EQUAL_INT(0, zero);
	TEST_ASSERT_EQUAL_INT(INT_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT(INT_MAX / 2, hmax);

	TEST_ASSERT_EQUAL_INT(5, sscanf(buff, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT(INT_MIN, min);
	TEST_ASSERT_EQUAL_INT(INT_MAX, max);
	TEST_ASSERT_EQUAL_INT(0, zero);
	TEST_ASSERT_EQUAL_INT(INT_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT(INT_MAX / 2, hmax);
}


TEST(stdio_scanf_d, hhd)
{
	char max, min, zero, hmin, hmax, buff[BUFF_LEN] = { 0 };
	const char *format = "%hhd %hhd %hhd %hhd %hhd";

	sprintf(buff, format, CHAR_MAX, CHAR_MAX / 2, (char)0, CHAR_MIN / 2, CHAR_MIN);
	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(5, test_vfscanfWrapper(filep, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT8(CHAR_MIN, min);
	TEST_ASSERT_EQUAL_INT8(CHAR_MAX, max);
	TEST_ASSERT_EQUAL_INT8(0, zero);
	TEST_ASSERT_EQUAL_INT8(CHAR_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT8(CHAR_MAX / 2, hmax);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(5, fscanf(filep, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT8(CHAR_MIN, min);
	TEST_ASSERT_EQUAL_INT8(CHAR_MAX, max);
	TEST_ASSERT_EQUAL_INT8(0, zero);
	TEST_ASSERT_EQUAL_INT8(CHAR_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT8(CHAR_MAX / 2, hmax);

	TEST_ASSERT_EQUAL_INT(5, test_vsscanfWrapper(buff, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT8(CHAR_MIN, min);
	TEST_ASSERT_EQUAL_INT8(CHAR_MAX, max);
	TEST_ASSERT_EQUAL_INT8(0, zero);
	TEST_ASSERT_EQUAL_INT8(CHAR_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT8(CHAR_MAX / 2, hmax);

	TEST_ASSERT_EQUAL_INT(5, sscanf(buff, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT8(CHAR_MIN, min);
	TEST_ASSERT_EQUAL_INT8(CHAR_MAX, max);
	TEST_ASSERT_EQUAL_INT8(0, zero);
	TEST_ASSERT_EQUAL_INT8(CHAR_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT8(CHAR_MAX / 2, hmax);
}


TEST(stdio_scanf_d, hd)
{
	char buff[BUFF_LEN] = { 0 };
	short max, min, zero, hmin, hmax;
	const char *format = "%hd %hd %hd %hd %hd";

	sprintf(buff, format, SHRT_MAX, SHRT_MAX / 2, (short)0, SHRT_MIN / 2, SHRT_MIN);
	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(5, test_vfscanfWrapper(filep, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT16(SHRT_MIN, min);
	TEST_ASSERT_EQUAL_INT16(SHRT_MAX, max);
	TEST_ASSERT_EQUAL_INT16(0, zero);
	TEST_ASSERT_EQUAL_INT16(SHRT_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT16(SHRT_MAX / 2, hmax);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(5, fscanf(filep, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT16(SHRT_MIN, min);
	TEST_ASSERT_EQUAL_INT16(SHRT_MAX, max);
	TEST_ASSERT_EQUAL_INT16(0, zero);
	TEST_ASSERT_EQUAL_INT16(SHRT_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT16(SHRT_MAX / 2, hmax);

	TEST_ASSERT_EQUAL_INT(5, test_vsscanfWrapper(buff, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT16(SHRT_MIN, min);
	TEST_ASSERT_EQUAL_INT16(SHRT_MAX, max);
	TEST_ASSERT_EQUAL_INT16(0, zero);
	TEST_ASSERT_EQUAL_INT16(SHRT_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT16(SHRT_MAX / 2, hmax);

	TEST_ASSERT_EQUAL_INT(5, sscanf(buff, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT16(SHRT_MIN, min);
	TEST_ASSERT_EQUAL_INT16(SHRT_MAX, max);
	TEST_ASSERT_EQUAL_INT16(0, zero);
	TEST_ASSERT_EQUAL_INT16(SHRT_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT16(SHRT_MAX / 2, hmax);
}


TEST(stdio_scanf_d, ld)
{
	char buff[BUFF_LEN] = { 0 };
	long max, min, zero, hmin, hmax;
	const char *format = "%ld %ld %ld %ld %ld";

	sprintf(buff, format, LONG_MAX, LONG_MAX / 2, (long)0, LONG_MIN / 2, LONG_MIN);
	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(5, test_vfscanfWrapper(filep, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT64(LONG_MIN, min);
	TEST_ASSERT_EQUAL_INT64(LONG_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(LONG_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT64(LONG_MAX / 2, hmax);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(5, fscanf(filep, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT64(LONG_MIN, min);
	TEST_ASSERT_EQUAL_INT64(LONG_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(LONG_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT64(LONG_MAX / 2, hmax);

	TEST_ASSERT_EQUAL_INT(5, test_vsscanfWrapper(buff, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT64(LONG_MIN, min);
	TEST_ASSERT_EQUAL_INT64(LONG_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(LONG_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT64(LONG_MAX / 2, hmax);

	TEST_ASSERT_EQUAL_INT(5, sscanf(buff, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT64(LONG_MIN, min);
	TEST_ASSERT_EQUAL_INT64(LONG_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(LONG_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT64(LONG_MAX / 2, hmax);
}


TEST(stdio_scanf_d, lld)
{
	char buff[BUFF_LEN] = { 0 };
	long long max, min, zero, hmin, hmax;
	const char *format = "%lld %lld %lld %lld %lld";

	sprintf(buff, format, LLONG_MAX, LLONG_MAX / 2, (long long)0, LLONG_MIN / 2, LLONG_MIN);
	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(5, test_vfscanfWrapper(filep, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT64(LLONG_MIN, min);
	TEST_ASSERT_EQUAL_INT64(LLONG_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(LLONG_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT64(LLONG_MAX / 2, hmax);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(5, fscanf(filep, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT64(LLONG_MIN, min);
	TEST_ASSERT_EQUAL_INT64(LLONG_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(LLONG_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT64(LLONG_MAX / 2, hmax);

	TEST_ASSERT_EQUAL_INT(5, test_vsscanfWrapper(buff, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT64(LLONG_MIN, min);
	TEST_ASSERT_EQUAL_INT64(LLONG_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(LLONG_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT64(LLONG_MAX / 2, hmax);

	TEST_ASSERT_EQUAL_INT(5, sscanf(buff, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT64(LLONG_MIN, min);
	TEST_ASSERT_EQUAL_INT64(LLONG_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(LLONG_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT64(LLONG_MAX / 2, hmax);
}


TEST(stdio_scanf_d, jd)
{
	char buff[BUFF_LEN] = { 0 };
	intmax_t max, min, zero, hmin, hmax;
	const char *format = "%jd %jd %jd %jd %jd";

	sprintf(buff, format, (intmax_t)INTMAX_MAX, (intmax_t)INTMAX_MAX / 2, (intmax_t)0, (intmax_t)INTMAX_MIN / 2, (intmax_t)INTMAX_MIN);
	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(5, test_vfscanfWrapper(filep, format, &max, &hmax, &zero, &hmin, &min));
	rewind(filep);
	TEST_ASSERT_EQUAL_INT64(INTMAX_MIN, min);
	TEST_ASSERT_EQUAL_INT64(INTMAX_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(INTMAX_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT64(INTMAX_MAX / 2, hmax);

	rewind(filep);
	TEST_ASSERT_EQUAL_INT(5, fscanf(filep, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT64(INTMAX_MIN, min);
	TEST_ASSERT_EQUAL_INT64(INTMAX_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(INTMAX_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT64(INTMAX_MAX / 2, hmax);

	TEST_ASSERT_EQUAL_INT(5, test_vsscanfWrapper(buff, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT64(INTMAX_MIN, min);
	TEST_ASSERT_EQUAL_INT64(INTMAX_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(INTMAX_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT64(INTMAX_MAX / 2, hmax);

	TEST_ASSERT_EQUAL_INT(5, sscanf(buff, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT64(INTMAX_MIN, min);
	TEST_ASSERT_EQUAL_INT64(INTMAX_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(INTMAX_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT64(INTMAX_MAX / 2, hmax);
}


TEST(stdio_scanf_d, zd)
{
	char buff[BUFF_LEN] = { 0 };
	size_t max, zero, hmax;
	const char *format = "%zd %zd %zd";

	sprintf(buff, format, (size_t)SSIZE_MAX, (size_t)SSIZE_MAX / 2, (size_t)0);

	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(3, test_vfscanfWrapper(filep, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_INT64(SSIZE_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(SSIZE_MAX / 2, hmax);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(3, fscanf(filep, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_INT64(SSIZE_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(SSIZE_MAX / 2, hmax);

	TEST_ASSERT_EQUAL_INT(3, test_vsscanfWrapper(buff, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_INT64(SSIZE_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(SSIZE_MAX / 2, hmax);

	TEST_ASSERT_EQUAL_INT(3, sscanf(buff, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_INT64(SSIZE_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(SSIZE_MAX / 2, hmax);
}


TEST(stdio_scanf_d, td)
{
	char buff[BUFF_LEN] = { 0 };
	ptrdiff_t max, min, zero, hmin, hmax;
	const char *format = "%td %td %td %td %td";

	sprintf(buff, format, (ptrdiff_t)PTRDIFF_MAX, (ptrdiff_t)PTRDIFF_MAX / 2, (ptrdiff_t)0, (ptrdiff_t)PTRDIFF_MIN / 2,
		(ptrdiff_t)PTRDIFF_MIN);

	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(5, test_vfscanfWrapper(filep, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT64(PTRDIFF_MIN, min);
	TEST_ASSERT_EQUAL_INT64(PTRDIFF_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(PTRDIFF_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT64(PTRDIFF_MAX / 2, hmax);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(5, fscanf(filep, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT64(PTRDIFF_MIN, min);
	TEST_ASSERT_EQUAL_INT64(PTRDIFF_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(PTRDIFF_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT64(PTRDIFF_MAX / 2, hmax);

	TEST_ASSERT_EQUAL_INT(5, test_vsscanfWrapper(buff, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT64(PTRDIFF_MIN, min);
	TEST_ASSERT_EQUAL_INT64(PTRDIFF_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(PTRDIFF_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT64(PTRDIFF_MAX / 2, hmax);

	TEST_ASSERT_EQUAL_INT(5, sscanf(buff, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT64(PTRDIFF_MIN, min);
	TEST_ASSERT_EQUAL_INT64(PTRDIFF_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(PTRDIFF_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT64(PTRDIFF_MAX / 2, hmax);
}


/*
///////////////////////////////////////////////////////////////////////////////////////////////
*/


TEST_SETUP(stdio_scanf_i)
{
	filep = fopen(TESTFILE_PATH, "w+");
}


TEST_TEAR_DOWN(stdio_scanf_i)
{
	fclose(filep);
}


TEST(stdio_scanf_i, i)
{
	char buff[BUFF_LEN] = { 0 };
	int max, min, zero, hmin, hmax;
	const char *format = "%i %i %i %i %i";

	sprintf(buff, format, INT_MAX, INT_MAX / 2, 0, INT_MIN / 2, INT_MIN);

	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(5, test_vfscanfWrapper(filep, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT(INT_MIN, min);
	TEST_ASSERT_EQUAL_INT(INT_MAX, max);
	TEST_ASSERT_EQUAL_INT(0, zero);
	TEST_ASSERT_EQUAL_INT(INT_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT(INT_MAX / 2, hmax);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(5, fscanf(filep, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT(INT_MIN, min);
	TEST_ASSERT_EQUAL_INT(INT_MAX, max);
	TEST_ASSERT_EQUAL_INT(0, zero);
	TEST_ASSERT_EQUAL_INT(INT_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT(INT_MAX / 2, hmax);

	TEST_ASSERT_EQUAL_INT(5, test_vsscanfWrapper(buff, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT(INT_MIN, min);
	TEST_ASSERT_EQUAL_INT(INT_MAX, max);
	TEST_ASSERT_EQUAL_INT(0, zero);
	TEST_ASSERT_EQUAL_INT(INT_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT(INT_MAX / 2, hmax);

	TEST_ASSERT_EQUAL_INT(5, sscanf(buff, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT(INT_MIN, min);
	TEST_ASSERT_EQUAL_INT(INT_MAX, max);
	TEST_ASSERT_EQUAL_INT(0, zero);
	TEST_ASSERT_EQUAL_INT(INT_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT(INT_MAX / 2, hmax);
}


TEST(stdio_scanf_i, hhi)
{
	char max, min, zero, hmin, hmax, buff[BUFF_LEN] = { 0 };
	const char *format = "%hhi %hhi %hhi %hhi %hhi";

	sprintf(buff, format, CHAR_MAX, CHAR_MAX / 2, (char)0, CHAR_MIN / 2, CHAR_MIN);

	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(5, test_vfscanfWrapper(filep, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT8(CHAR_MIN, min);
	TEST_ASSERT_EQUAL_INT8(CHAR_MAX, max);
	TEST_ASSERT_EQUAL_INT8(0, zero);
	TEST_ASSERT_EQUAL_INT8(CHAR_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT8(CHAR_MAX / 2, hmax);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(5, fscanf(filep, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT8(CHAR_MIN, min);
	TEST_ASSERT_EQUAL_INT8(CHAR_MAX, max);
	TEST_ASSERT_EQUAL_INT8(0, zero);
	TEST_ASSERT_EQUAL_INT8(CHAR_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT8(CHAR_MAX / 2, hmax);

	TEST_ASSERT_EQUAL_INT(5, test_vsscanfWrapper(buff, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT8(CHAR_MIN, min);
	TEST_ASSERT_EQUAL_INT8(CHAR_MAX, max);
	TEST_ASSERT_EQUAL_INT8(0, zero);
	TEST_ASSERT_EQUAL_INT8(CHAR_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT8(CHAR_MAX / 2, hmax);

	TEST_ASSERT_EQUAL_INT(5, sscanf(buff, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT8(CHAR_MIN, min);
	TEST_ASSERT_EQUAL_INT8(CHAR_MAX, max);
	TEST_ASSERT_EQUAL_INT8(0, zero);
	TEST_ASSERT_EQUAL_INT8(CHAR_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT8(CHAR_MAX / 2, hmax);
}


TEST(stdio_scanf_i, hi)
{
	char buff[BUFF_LEN] = { 0 };
	short max, min, zero, hmin, hmax;
	const char *format = "%hi %hi %hi %hi %hi";

	sprintf(buff, format, SHRT_MAX, SHRT_MAX / 2, (short)0, SHRT_MIN / 2, SHRT_MIN);

	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(5, test_vfscanfWrapper(filep, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT16(SHRT_MIN, min);
	TEST_ASSERT_EQUAL_INT16(SHRT_MAX, max);
	TEST_ASSERT_EQUAL_INT16(0, zero);
	TEST_ASSERT_EQUAL_INT16(SHRT_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT16(SHRT_MAX / 2, hmax);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(5, fscanf(filep, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT16(SHRT_MIN, min);
	TEST_ASSERT_EQUAL_INT16(SHRT_MAX, max);
	TEST_ASSERT_EQUAL_INT16(0, zero);
	TEST_ASSERT_EQUAL_INT16(SHRT_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT16(SHRT_MAX / 2, hmax);

	TEST_ASSERT_EQUAL_INT(5, test_vsscanfWrapper(buff, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT16(SHRT_MIN, min);
	TEST_ASSERT_EQUAL_INT16(SHRT_MAX, max);
	TEST_ASSERT_EQUAL_INT16(0, zero);
	TEST_ASSERT_EQUAL_INT16(SHRT_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT16(SHRT_MAX / 2, hmax);

	TEST_ASSERT_EQUAL_INT(5, sscanf(buff, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT16(SHRT_MIN, min);
	TEST_ASSERT_EQUAL_INT16(SHRT_MAX, max);
	TEST_ASSERT_EQUAL_INT16(0, zero);
	TEST_ASSERT_EQUAL_INT16(SHRT_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT16(SHRT_MAX / 2, hmax);
}


TEST(stdio_scanf_i, li)
{
	char buff[BUFF_LEN] = { 0 };
	long max, min, zero, hmin, hmax;
	const char *format = "%li %li %li %li %li";

	sprintf(buff, format, LONG_MAX, LONG_MAX / 2, (long)0, LONG_MIN / 2, LONG_MIN);

	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(5, test_vfscanfWrapper(filep, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT64(LONG_MIN, min);
	TEST_ASSERT_EQUAL_INT64(LONG_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(LONG_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT64(LONG_MAX / 2, hmax);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(5, fscanf(filep, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT64(LONG_MIN, min);
	TEST_ASSERT_EQUAL_INT64(LONG_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(LONG_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT64(LONG_MAX / 2, hmax);

	TEST_ASSERT_EQUAL_INT(5, test_vsscanfWrapper(buff, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT64(LONG_MIN, min);
	TEST_ASSERT_EQUAL_INT64(LONG_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(LONG_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT64(LONG_MAX / 2, hmax);

	TEST_ASSERT_EQUAL_INT(5, sscanf(buff, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT64(LONG_MIN, min);
	TEST_ASSERT_EQUAL_INT64(LONG_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(LONG_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT64(LONG_MAX / 2, hmax);
}


TEST(stdio_scanf_i, lli)
{
	char buff[BUFF_LEN] = { 0 };
	long long max, min, zero, hmin, hmax;
	const char *format = "%lli %lli %lli %lli %lli";

	sprintf(buff, format, LLONG_MAX, LLONG_MAX / 2, (long long)0, LLONG_MIN / 2, LLONG_MIN);

	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(5, test_vfscanfWrapper(filep, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT64(LLONG_MIN, min);
	TEST_ASSERT_EQUAL_INT64(LLONG_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(LLONG_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT64(LLONG_MAX / 2, hmax);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(5, fscanf(filep, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT64(LLONG_MIN, min);
	TEST_ASSERT_EQUAL_INT64(LLONG_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(LLONG_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT64(LLONG_MAX / 2, hmax);

	TEST_ASSERT_EQUAL_INT(5, test_vsscanfWrapper(buff, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT64(LLONG_MIN, min);
	TEST_ASSERT_EQUAL_INT64(LLONG_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(LLONG_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT64(LLONG_MAX / 2, hmax);

	TEST_ASSERT_EQUAL_INT(5, sscanf(buff, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT64(LLONG_MIN, min);
	TEST_ASSERT_EQUAL_INT64(LLONG_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(LLONG_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT64(LLONG_MAX / 2, hmax);
}


TEST(stdio_scanf_i, ji)
{
	char buff[BUFF_LEN] = { 0 };
	intmax_t max, min, zero, hmin, hmax;
	const char *format = "%ji %ji %ji %ji %ji";

	sprintf(buff, format, (intmax_t)INTMAX_MAX, (intmax_t)INTMAX_MAX / 2, (intmax_t)0, (intmax_t)INTMAX_MIN / 2, (intmax_t)INTMAX_MIN);
	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(5, test_vfscanfWrapper(filep, format, &max, &hmax, &zero, &hmin, &min));
	rewind(filep);
	TEST_ASSERT_EQUAL_INT64(INTMAX_MIN, min);
	TEST_ASSERT_EQUAL_INT64(INTMAX_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(INTMAX_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT64(INTMAX_MAX / 2, hmax);

	rewind(filep);
	TEST_ASSERT_EQUAL_INT(5, fscanf(filep, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT64(INTMAX_MIN, min);
	TEST_ASSERT_EQUAL_INT64(INTMAX_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(INTMAX_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT64(INTMAX_MAX / 2, hmax);

	TEST_ASSERT_EQUAL_INT(5, test_vsscanfWrapper(buff, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT64(INTMAX_MIN, min);
	TEST_ASSERT_EQUAL_INT64(INTMAX_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(INTMAX_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT64(INTMAX_MAX / 2, hmax);

	TEST_ASSERT_EQUAL_INT(5, sscanf(buff, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT64(INTMAX_MIN, min);
	TEST_ASSERT_EQUAL_INT64(INTMAX_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(INTMAX_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT64(INTMAX_MAX / 2, hmax);
}


TEST(stdio_scanf_i, zi)
{
	char buff[BUFF_LEN] = { 0 };
	size_t max, zero, hmax;
	const char *format = "%zi %zi %zi";

	sprintf(buff, format, (size_t)SSIZE_MAX, (size_t)SSIZE_MAX / 2, (size_t)0);

	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(3, test_vfscanfWrapper(filep, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_INT64(SSIZE_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(SSIZE_MAX / 2, hmax);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(3, fscanf(filep, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_INT64(SSIZE_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(SSIZE_MAX / 2, hmax);

	TEST_ASSERT_EQUAL_INT(3, test_vsscanfWrapper(buff, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_INT64(SSIZE_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(SSIZE_MAX / 2, hmax);

	TEST_ASSERT_EQUAL_INT(3, sscanf(buff, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_INT64(SSIZE_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(SSIZE_MAX / 2, hmax);
}


TEST(stdio_scanf_i, ti)
{
	char buff[BUFF_LEN] = { 0 };
	ptrdiff_t max, min, zero, hmin, hmax;
	const char *format = "%ti %ti %ti %ti %ti";

	sprintf(buff, format, (ptrdiff_t)PTRDIFF_MAX, (ptrdiff_t)PTRDIFF_MAX / 2, (ptrdiff_t)0, (ptrdiff_t)PTRDIFF_MIN / 2,
		(ptrdiff_t)PTRDIFF_MIN);

	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(5, test_vfscanfWrapper(filep, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT64(PTRDIFF_MIN, min);
	TEST_ASSERT_EQUAL_INT64(PTRDIFF_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(PTRDIFF_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT64(PTRDIFF_MAX / 2, hmax);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(5, fscanf(filep, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT64(PTRDIFF_MIN, min);
	TEST_ASSERT_EQUAL_INT64(PTRDIFF_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(PTRDIFF_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT64(PTRDIFF_MAX / 2, hmax);

	TEST_ASSERT_EQUAL_INT(5, test_vsscanfWrapper(buff, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT64(PTRDIFF_MIN, min);
	TEST_ASSERT_EQUAL_INT64(PTRDIFF_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(PTRDIFF_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT64(PTRDIFF_MAX / 2, hmax);

	TEST_ASSERT_EQUAL_INT(5, sscanf(buff, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT64(PTRDIFF_MIN, min);
	TEST_ASSERT_EQUAL_INT64(PTRDIFF_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(PTRDIFF_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT64(PTRDIFF_MAX / 2, hmax);
}


/*
//////////////////////////////////////////////////////////////////////////////////////////////////
*/


TEST_SETUP(stdio_scanf_u)
{
	filep = fopen(TESTFILE_PATH, "w+");
}


TEST_TEAR_DOWN(stdio_scanf_u)
{
	fclose(filep);
}


TEST(stdio_scanf_u, u)
{
	char buff[BUFF_LEN] = { 0 };
	unsigned int max, zero, hmax;
	const char *format = "%u %u %u";

	sprintf(buff, format, UINT_MAX, UINT_MAX / 2, 0);

	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(3, test_vfscanfWrapper(filep, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_UINT(UINT_MAX, max);
	TEST_ASSERT_EQUAL_UINT(UINT_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_UINT(0, zero);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(3, fscanf(filep, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_UINT(UINT_MAX, max);
	TEST_ASSERT_EQUAL_UINT(UINT_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_UINT(0, zero);

	TEST_ASSERT_EQUAL_INT(3, test_vsscanfWrapper(buff, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_UINT(UINT_MAX, max);
	TEST_ASSERT_EQUAL_UINT(UINT_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_UINT(0, zero);

	TEST_ASSERT_EQUAL_INT(3, sscanf(buff, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_UINT(UINT_MAX, max);
	TEST_ASSERT_EQUAL_UINT(UINT_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_UINT(0, zero);
}


TEST(stdio_scanf_u, hhu)
{
	char buff[BUFF_LEN] = { 0 };
	unsigned char max, zero, hmax;
	const char *format = "%hhu %hhu %hhu";

	sprintf(buff, format, UCHAR_MAX, UCHAR_MAX / 2, (unsigned char)0);

	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(3, test_vfscanfWrapper(filep, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_UINT8(UCHAR_MAX, max);
	TEST_ASSERT_EQUAL_UINT8(UCHAR_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_UINT8(0, zero);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(3, fscanf(filep, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_UINT8(UCHAR_MAX, max);
	TEST_ASSERT_EQUAL_UINT8(UCHAR_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_UINT8(0, zero);

	TEST_ASSERT_EQUAL_INT(3, test_vsscanfWrapper(buff, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_UINT8(UCHAR_MAX, max);
	TEST_ASSERT_EQUAL_UINT8(UCHAR_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_UINT8(0, zero);

	TEST_ASSERT_EQUAL_INT(3, sscanf(buff, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_UINT8(UCHAR_MAX, max);
	TEST_ASSERT_EQUAL_UINT8(UCHAR_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_UINT8(0, zero);
}


TEST(stdio_scanf_u, hu)
{
	char buff[BUFF_LEN] = { 0 };
	unsigned short max, zero, hmax;
	const char *format = "%hu %hu %hu";

	sprintf(buff, format, USHRT_MAX, USHRT_MAX / 2, (unsigned short)0);

	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(3, test_vfscanfWrapper(filep, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_UINT16(USHRT_MAX, max);
	TEST_ASSERT_EQUAL_UINT16(USHRT_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_UINT16(0, zero);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(3, fscanf(filep, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_UINT16(USHRT_MAX, max);
	TEST_ASSERT_EQUAL_UINT16(USHRT_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_UINT16(0, zero);

	TEST_ASSERT_EQUAL_INT(3, test_vsscanfWrapper(buff, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_UINT16(USHRT_MAX, max);
	TEST_ASSERT_EQUAL_UINT16(USHRT_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_UINT16(0, zero);

	TEST_ASSERT_EQUAL_INT(3, sscanf(buff, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_UINT16(USHRT_MAX, max);
	TEST_ASSERT_EQUAL_UINT16(USHRT_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_UINT16(0, zero);
}


TEST(stdio_scanf_u, lu)
{
	char buff[BUFF_LEN] = { 0 };
	unsigned long max, zero, hmax;
	const char *format = "%lu %lu %lu";

	sprintf(buff, format, ULONG_MAX, ULONG_MAX / 2, (unsigned long)0);

	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(3, test_vfscanfWrapper(filep, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_UINT64(ULONG_MAX, max);
	TEST_ASSERT_EQUAL_UINT64(ULONG_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_UINT64(0, zero);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(3, fscanf(filep, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_UINT64(ULONG_MAX, max);
	TEST_ASSERT_EQUAL_UINT64(ULONG_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_UINT64(0, zero);

	TEST_ASSERT_EQUAL_INT(3, test_vsscanfWrapper(buff, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_UINT64(ULONG_MAX, max);
	TEST_ASSERT_EQUAL_UINT64(ULONG_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_UINT64(0, zero);

	TEST_ASSERT_EQUAL_INT(3, sscanf(buff, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_UINT64(ULONG_MAX, max);
	TEST_ASSERT_EQUAL_UINT64(ULONG_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_UINT64(0, zero);
}


TEST(stdio_scanf_u, llu)
{
	char buff[BUFF_LEN] = { 0 };
	unsigned long long max, zero, hmax;
	const char *format = "%llu %llu %llu";

	sprintf(buff, format, ULLONG_MAX, ULLONG_MAX / 2, (unsigned long long)0);

	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(3, test_vfscanfWrapper(filep, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_UINT64(ULLONG_MAX, max);
	TEST_ASSERT_EQUAL_UINT64(ULLONG_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_UINT64(0, zero);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(3, fscanf(filep, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_UINT64(ULLONG_MAX, max);
	TEST_ASSERT_EQUAL_UINT64(ULLONG_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_UINT64(0, zero);

	TEST_ASSERT_EQUAL_INT(3, test_vsscanfWrapper(buff, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_UINT64(ULLONG_MAX, max);
	TEST_ASSERT_EQUAL_UINT64(ULLONG_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_UINT64(0, zero);

	TEST_ASSERT_EQUAL_INT(3, sscanf(buff, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_UINT64(ULLONG_MAX, max);
	TEST_ASSERT_EQUAL_UINT64(ULLONG_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_UINT64(0, zero);
}


TEST(stdio_scanf_u, ju)
{
	char buff[BUFF_LEN] = { 0 };
	intmax_t max, zero, hmax;
	const char *format = "%ju %ju %ju";

	sprintf(buff, format, UINTMAX_MAX, UINTMAX_MAX / 2, (intmax_t)0);

	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(3, test_vfscanfWrapper(filep, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_UINT64(UINTMAX_MAX, max);
	TEST_ASSERT_EQUAL_UINT64(UINTMAX_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_UINT64(0, zero);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(3, fscanf(filep, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_UINT64(UINTMAX_MAX, max);
	TEST_ASSERT_EQUAL_UINT64(UINTMAX_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_UINT64(0, zero);

	TEST_ASSERT_EQUAL_INT(3, test_vsscanfWrapper(buff, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_UINT64(UINTMAX_MAX, max);
	TEST_ASSERT_EQUAL_UINT64(UINTMAX_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_UINT64(0, zero);

	TEST_ASSERT_EQUAL_INT(3, sscanf(buff, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_UINT64(UINTMAX_MAX, max);
	TEST_ASSERT_EQUAL_UINT64(UINTMAX_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_UINT64(0, zero);
}


TEST(stdio_scanf_u, zu)
{
	char buff[BUFF_LEN] = { 0 };
	size_t max, zero, hmax;
	const char *format = "%zu %zu %zu";

	sprintf(buff, format, (size_t)SIZE_MAX, (size_t)SIZE_MAX / 2, (size_t)0);

	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(3, test_vfscanfWrapper(filep, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_UINT64(SIZE_MAX, max);
	TEST_ASSERT_EQUAL_UINT64(0, zero);
	TEST_ASSERT_EQUAL_UINT64(SIZE_MAX / 2, hmax);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(3, fscanf(filep, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_UINT64(SIZE_MAX, max);
	TEST_ASSERT_EQUAL_UINT64(0, zero);
	TEST_ASSERT_EQUAL_UINT64(SIZE_MAX / 2, hmax);

	TEST_ASSERT_EQUAL_INT(3, test_vsscanfWrapper(buff, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_UINT64(SIZE_MAX, max);
	TEST_ASSERT_EQUAL_UINT64(0, zero);
	TEST_ASSERT_EQUAL_UINT64(SIZE_MAX / 2, hmax);

	TEST_ASSERT_EQUAL_INT(3, sscanf(buff, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_UINT64(SIZE_MAX, max);
	TEST_ASSERT_EQUAL_UINT64(0, zero);
	TEST_ASSERT_EQUAL_UINT64(SIZE_MAX / 2, hmax);
}


TEST(stdio_scanf_u, tu)
{
	char buff[BUFF_LEN] = { 0 };
	ptrdiff_t max, zero, hmax;
	const char *format = "%tu %tu %tu";

	sprintf(buff, format, (ptrdiff_t)PTRDIFF_MAX, (ptrdiff_t)PTRDIFF_MAX / 2, (ptrdiff_t)0);

	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(3, test_vfscanfWrapper(filep, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_UINT64(PTRDIFF_MAX, max);
	TEST_ASSERT_EQUAL_UINT64(PTRDIFF_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_UINT64(0, zero);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(3, fscanf(filep, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_UINT64(PTRDIFF_MAX, max);
	TEST_ASSERT_EQUAL_UINT64(PTRDIFF_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_UINT64(0, zero);

	TEST_ASSERT_EQUAL_INT(3, test_vsscanfWrapper(buff, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_UINT64(PTRDIFF_MAX, max);
	TEST_ASSERT_EQUAL_UINT64(PTRDIFF_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_UINT64(0, zero);

	TEST_ASSERT_EQUAL_INT(3, sscanf(buff, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_UINT64(PTRDIFF_MAX, max);
	TEST_ASSERT_EQUAL_UINT64(PTRDIFF_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_UINT64(0, zero);
}


/*
//////////////////////////////////////////////////////////////////////////////////////////////////
*/


TEST_SETUP(stdio_scanf_o)
{
	filep = fopen(TESTFILE_PATH, "w+");
}


TEST_TEAR_DOWN(stdio_scanf_o)
{
	fclose(filep);
}


TEST(stdio_scanf_o, o)
{
	char buff[BUFF_LEN] = { 0 };
	unsigned int max, zero, hmax;
	const char *format = "%o %o %o";

	sprintf(buff, format, UINT_MAX, UINT_MAX / 2, (unsigned int)0);

	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(3, test_vfscanfWrapper(filep, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_INT(UINT_MAX, max);
	TEST_ASSERT_EQUAL_INT(UINT_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_INT(0, zero);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(3, fscanf(filep, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_INT(UINT_MAX, max);
	TEST_ASSERT_EQUAL_INT(UINT_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_INT(0, zero);

	TEST_ASSERT_EQUAL_INT(3, test_vsscanfWrapper(buff, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_INT(UINT_MAX, max);
	TEST_ASSERT_EQUAL_INT(UINT_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_INT(0, zero);

	TEST_ASSERT_EQUAL_INT(3, sscanf(buff, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_INT(UINT_MAX, max);
	TEST_ASSERT_EQUAL_INT(UINT_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_INT(0, zero);
}


TEST(stdio_scanf_o, hho)
{
	char max, min, zero, hmin, hmax, buff[BUFF_LEN] = { 0 };
	unsigned char umax;
	const char *format = "%hho %hho %hho %hho %hho %hho";

	sprintf(buff, format, CHAR_MAX, CHAR_MAX / 2, (char)0, CHAR_MIN / 2, CHAR_MIN, UCHAR_MAX);

	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(6, test_vfscanfWrapper(filep, format, &max, &hmax, &zero, &hmin, &min, &umax));

	TEST_ASSERT_EQUAL_INT8(CHAR_MIN, min);
	TEST_ASSERT_EQUAL_INT8(CHAR_MAX, max);
	TEST_ASSERT_EQUAL_INT8(0, zero);
	TEST_ASSERT_EQUAL_INT8(CHAR_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT8(CHAR_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_UINT8(UCHAR_MAX, umax);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(6, fscanf(filep, format, &max, &hmax, &zero, &hmin, &min, &umax));

	TEST_ASSERT_EQUAL_INT8(CHAR_MIN, min);
	TEST_ASSERT_EQUAL_INT8(CHAR_MAX, max);
	TEST_ASSERT_EQUAL_INT8(0, zero);
	TEST_ASSERT_EQUAL_INT8(CHAR_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT8(CHAR_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_UINT8(UCHAR_MAX, umax);

	TEST_ASSERT_EQUAL_INT(6, test_vsscanfWrapper(buff, format, &max, &hmax, &zero, &hmin, &min, &umax));

	TEST_ASSERT_EQUAL_INT8(CHAR_MIN, min);
	TEST_ASSERT_EQUAL_INT8(CHAR_MAX, max);
	TEST_ASSERT_EQUAL_INT8(0, zero);
	TEST_ASSERT_EQUAL_INT8(CHAR_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT8(CHAR_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_UINT8(UCHAR_MAX, umax);

	TEST_ASSERT_EQUAL_INT(6, sscanf(buff, format, &max, &hmax, &zero, &hmin, &min, &umax));

	TEST_ASSERT_EQUAL_INT8(CHAR_MIN, min);
	TEST_ASSERT_EQUAL_INT8(CHAR_MAX, max);
	TEST_ASSERT_EQUAL_INT8(0, zero);
	TEST_ASSERT_EQUAL_INT8(CHAR_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT8(CHAR_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_UINT8(UCHAR_MAX, umax);
}


TEST(stdio_scanf_o, ho)
{
	char buff[BUFF_LEN] = { 0 };
	short max, min, zero, hmin, hmax;
	unsigned short umax;
	const char *format = "%ho %ho %ho %ho %ho %ho";

	sprintf(buff, format, SHRT_MAX, SHRT_MAX / 2, (short)0, SHRT_MIN / 2, SHRT_MIN, USHRT_MAX);

	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(6, test_vfscanfWrapper(filep, format, &max, &hmax, &zero, &hmin, &min, &umax));

	TEST_ASSERT_EQUAL_INT16(SHRT_MIN, min);
	TEST_ASSERT_EQUAL_INT16(SHRT_MAX, max);
	TEST_ASSERT_EQUAL_INT16(0, zero);
	TEST_ASSERT_EQUAL_INT16(SHRT_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT16(SHRT_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_UINT16(USHRT_MAX, umax);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(6, fscanf(filep, format, &max, &hmax, &zero, &hmin, &min, &umax));

	TEST_ASSERT_EQUAL_INT16(SHRT_MIN, min);
	TEST_ASSERT_EQUAL_INT16(SHRT_MAX, max);
	TEST_ASSERT_EQUAL_INT16(0, zero);
	TEST_ASSERT_EQUAL_INT16(SHRT_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT16(SHRT_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_UINT16(USHRT_MAX, umax);

	TEST_ASSERT_EQUAL_INT(6, test_vsscanfWrapper(buff, format, &max, &hmax, &zero, &hmin, &min, &umax));

	TEST_ASSERT_EQUAL_INT16(SHRT_MIN, min);
	TEST_ASSERT_EQUAL_INT16(SHRT_MAX, max);
	TEST_ASSERT_EQUAL_INT16(0, zero);
	TEST_ASSERT_EQUAL_INT16(SHRT_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT16(SHRT_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_UINT16(USHRT_MAX, umax);

	TEST_ASSERT_EQUAL_INT(6, sscanf(buff, format, &max, &hmax, &zero, &hmin, &min, &umax));

	TEST_ASSERT_EQUAL_INT16(SHRT_MIN, min);
	TEST_ASSERT_EQUAL_INT16(SHRT_MAX, max);
	TEST_ASSERT_EQUAL_INT16(0, zero);
	TEST_ASSERT_EQUAL_INT16(SHRT_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT16(SHRT_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_UINT16(USHRT_MAX, umax);
}


TEST(stdio_scanf_o, lo)
{
	char buff[BUFF_LEN] = { 0 };
	long max, min, zero, hmin, hmax;
	unsigned long umax;
	const char *format = "%lo %lo %lo %lo %lo %lo";

	sprintf(buff, format, LONG_MAX, LONG_MAX / 2, (long)0, LONG_MIN / 2, LONG_MIN, ULONG_MAX);

	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(6, test_vfscanfWrapper(filep, format, &max, &hmax, &zero, &hmin, &min, &umax));

	TEST_ASSERT_EQUAL_INT64(LONG_MIN, min);
	TEST_ASSERT_EQUAL_INT64(LONG_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(LONG_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT64(LONG_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_UINT64(ULONG_MAX, umax);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(6, fscanf(filep, format, &max, &hmax, &zero, &hmin, &min, &umax));

	TEST_ASSERT_EQUAL_INT64(LONG_MIN, min);
	TEST_ASSERT_EQUAL_INT64(LONG_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(LONG_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT64(LONG_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_UINT64(ULONG_MAX, umax);

	TEST_ASSERT_EQUAL_INT(6, test_vsscanfWrapper(buff, format, &max, &hmax, &zero, &hmin, &min, &umax));

	TEST_ASSERT_EQUAL_INT64(LONG_MIN, min);
	TEST_ASSERT_EQUAL_INT64(LONG_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(LONG_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT64(LONG_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_UINT64(ULONG_MAX, umax);

	TEST_ASSERT_EQUAL_INT(6, sscanf(buff, format, &max, &hmax, &zero, &hmin, &min, &umax));

	TEST_ASSERT_EQUAL_INT64(LONG_MIN, min);
	TEST_ASSERT_EQUAL_INT64(LONG_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(LONG_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT64(LONG_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_UINT64(ULONG_MAX, umax);
}


TEST(stdio_scanf_o, llo)
{
	char buff[BUFF_LEN] = { 0 };
	long long max, min, zero, hmin, hmax;
	unsigned long long umax;
	const char *format = "%llo %llo %llo %llo %llo %llo";

	sprintf(buff, format, LLONG_MAX, LLONG_MAX / 2, (long long)0, LLONG_MIN / 2, LLONG_MIN, ULLONG_MAX);

	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(6, test_vfscanfWrapper(filep, format, &max, &hmax, &zero, &hmin, &min, &umax));

	TEST_ASSERT_EQUAL_INT64(LLONG_MIN, min);
	TEST_ASSERT_EQUAL_INT64(LLONG_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(LLONG_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT64(LLONG_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_UINT64(ULLONG_MAX, umax);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(6, fscanf(filep, format, &max, &hmax, &zero, &hmin, &min, &umax));

	TEST_ASSERT_EQUAL_INT64(LLONG_MIN, min);
	TEST_ASSERT_EQUAL_INT64(LLONG_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(LLONG_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT64(LLONG_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_UINT64(ULLONG_MAX, umax);

	TEST_ASSERT_EQUAL_INT(6, test_vsscanfWrapper(buff, format, &max, &hmax, &zero, &hmin, &min, &umax));

	TEST_ASSERT_EQUAL_INT64(LLONG_MIN, min);
	TEST_ASSERT_EQUAL_INT64(LLONG_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(LLONG_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT64(LLONG_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_UINT64(ULLONG_MAX, umax);

	TEST_ASSERT_EQUAL_INT(6, sscanf(buff, format, &max, &hmax, &zero, &hmin, &min, &umax));

	TEST_ASSERT_EQUAL_INT64(LLONG_MIN, min);
	TEST_ASSERT_EQUAL_INT64(LLONG_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(LLONG_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT64(LLONG_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_UINT64(ULLONG_MAX, umax);
}


TEST(stdio_scanf_o, jo)
{
	char buff[BUFF_LEN] = { 0 };
	uintmax_t umax, uzero, uhmax;
	const char *format = "%jo %jo %jo";

	sprintf(buff, format, UINTMAX_MAX, UINTMAX_MAX / 2, (intmax_t)0);
	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(3, test_vfscanfWrapper(filep, format, &umax, &uhmax, &uzero));

	TEST_ASSERT_EQUAL_UINT64(UINTMAX_MAX, umax);
	TEST_ASSERT_EQUAL_UINT64(UINTMAX_MAX / 2, uhmax);
	TEST_ASSERT_EQUAL_UINT64(0, uzero);

	rewind(filep);
	TEST_ASSERT_EQUAL_INT(3, fscanf(filep, format, &umax, &uhmax, &uzero));

	TEST_ASSERT_EQUAL_UINT64(UINTMAX_MAX, umax);
	TEST_ASSERT_EQUAL_UINT64(UINTMAX_MAX / 2, uhmax);
	TEST_ASSERT_EQUAL_UINT64(0, uzero);

	TEST_ASSERT_EQUAL_INT(3, test_vsscanfWrapper(buff, format, &umax, &uhmax, &uzero));

	TEST_ASSERT_EQUAL_UINT64(UINTMAX_MAX, umax);
	TEST_ASSERT_EQUAL_UINT64(UINTMAX_MAX / 2, uhmax);
	TEST_ASSERT_EQUAL_UINT64(0, uzero);

	TEST_ASSERT_EQUAL_INT(3, sscanf(buff, format, &umax, &uhmax, &uzero));

	TEST_ASSERT_EQUAL_UINT64(UINTMAX_MAX, umax);
	TEST_ASSERT_EQUAL_UINT64(UINTMAX_MAX / 2, uhmax);
	TEST_ASSERT_EQUAL_UINT64(0, uzero);
}


TEST(stdio_scanf_o, zo)
{
	char buff[BUFF_LEN] = { 0 };
	size_t max, zero, hmax;
	const char *format = "%zo %zo %zo";

	sprintf(buff, format, (size_t)SIZE_MAX, (size_t)SIZE_MAX / 2, (size_t)0);

	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(3, test_vfscanfWrapper(filep, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_INT64(SIZE_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(SIZE_MAX / 2, hmax);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(3, fscanf(filep, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_INT64(SIZE_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(SIZE_MAX / 2, hmax);

	TEST_ASSERT_EQUAL_INT(3, test_vsscanfWrapper(buff, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_INT64(SIZE_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(SIZE_MAX / 2, hmax);

	TEST_ASSERT_EQUAL_INT(3, sscanf(buff, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_INT64(SIZE_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(SIZE_MAX / 2, hmax);
}


TEST(stdio_scanf_o, to)
{
	char buff[BUFF_LEN] = { 0 };
	ptrdiff_t max, min, zero, hmin, hmax;
	const char *format = "%to %to %to %to %to";

	sprintf(buff, format, (ptrdiff_t)PTRDIFF_MAX, (ptrdiff_t)PTRDIFF_MAX / 2, (ptrdiff_t)0, (ptrdiff_t)PTRDIFF_MIN / 2,
		(ptrdiff_t)PTRDIFF_MIN);

	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(5, test_vfscanfWrapper(filep, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT64(PTRDIFF_MIN, min);
	TEST_ASSERT_EQUAL_INT64(PTRDIFF_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(PTRDIFF_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT64(PTRDIFF_MAX / 2, hmax);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(5, fscanf(filep, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT64(PTRDIFF_MIN, min);
	TEST_ASSERT_EQUAL_INT64(PTRDIFF_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(PTRDIFF_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT64(PTRDIFF_MAX / 2, hmax);

	TEST_ASSERT_EQUAL_INT(5, test_vsscanfWrapper(buff, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT64(PTRDIFF_MIN, min);
	TEST_ASSERT_EQUAL_INT64(PTRDIFF_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(PTRDIFF_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT64(PTRDIFF_MAX / 2, hmax);

	TEST_ASSERT_EQUAL_INT(5, sscanf(buff, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_INT64(PTRDIFF_MIN, min);
	TEST_ASSERT_EQUAL_INT64(PTRDIFF_MAX, max);
	TEST_ASSERT_EQUAL_INT64(0, zero);
	TEST_ASSERT_EQUAL_INT64(PTRDIFF_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_INT64(PTRDIFF_MAX / 2, hmax);
}


/*
//////////////////////////////////////////////////////////////////////////////////////////////////
*/


TEST_SETUP(stdio_scanf_x)
{
	filep = fopen(TESTFILE_PATH, "w+");
}


TEST_TEAR_DOWN(stdio_scanf_x)
{
	fclose(filep);
}


TEST(stdio_scanf_x, x)
{
	char buff[BUFF_LEN] = { 0 };
	unsigned int max, zero, hmax;
	const char *format = "%x %x %x";

	sprintf(buff, format, UINT_MAX, UINT_MAX / 2, 0);

	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(3, test_vfscanfWrapper(filep, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_HEX(UINT_MAX, max);
	TEST_ASSERT_EQUAL_HEX(UINT_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_HEX(0, zero);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(3, fscanf(filep, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_HEX(UINT_MAX, max);
	TEST_ASSERT_EQUAL_HEX(UINT_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_HEX(0, zero);

	TEST_ASSERT_EQUAL_INT(3, test_vsscanfWrapper(buff, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_HEX(UINT_MAX, max);
	TEST_ASSERT_EQUAL_HEX(UINT_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_HEX(0, zero);

	TEST_ASSERT_EQUAL_INT(3, sscanf(buff, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_HEX(UINT_MAX, max);
	TEST_ASSERT_EQUAL_HEX(UINT_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_HEX(0, zero);
}


TEST(stdio_scanf_x, hhx)
{
	char max, min, zero, hmin, hmax, buff[BUFF_LEN] = { 0 };
	unsigned char umax;
	const char *format = "%hhx %hhx %hhx %hhx %hhx %hhx";

	sprintf(buff, format, CHAR_MAX, CHAR_MAX / 2, (char)0, CHAR_MIN / 2, CHAR_MIN, UCHAR_MAX);

	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(6, test_vfscanfWrapper(filep, format, &max, &hmax, &zero, &hmin, &min, &umax));

	TEST_ASSERT_EQUAL_HEX8(CHAR_MIN, min);
	TEST_ASSERT_EQUAL_HEX8(CHAR_MAX, max);
	TEST_ASSERT_EQUAL_HEX8(0, zero);
	TEST_ASSERT_EQUAL_HEX8(CHAR_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_HEX8(CHAR_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_HEX8(UCHAR_MAX, umax);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(6, fscanf(filep, format, &max, &hmax, &zero, &hmin, &min, &umax));

	TEST_ASSERT_EQUAL_HEX8(CHAR_MIN, min);
	TEST_ASSERT_EQUAL_HEX8(CHAR_MAX, max);
	TEST_ASSERT_EQUAL_HEX8(0, zero);
	TEST_ASSERT_EQUAL_HEX8(CHAR_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_HEX8(CHAR_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_HEX8(UCHAR_MAX, umax);

	TEST_ASSERT_EQUAL_INT(6, test_vsscanfWrapper(buff, format, &max, &hmax, &zero, &hmin, &min, &umax));

	TEST_ASSERT_EQUAL_HEX8(CHAR_MIN, min);
	TEST_ASSERT_EQUAL_HEX8(CHAR_MAX, max);
	TEST_ASSERT_EQUAL_HEX8(0, zero);
	TEST_ASSERT_EQUAL_HEX8(CHAR_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_HEX8(CHAR_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_HEX8(UCHAR_MAX, umax);

	TEST_ASSERT_EQUAL_INT(6, sscanf(buff, format, &max, &hmax, &zero, &hmin, &min, &umax));

	TEST_ASSERT_EQUAL_HEX8(CHAR_MIN, min);
	TEST_ASSERT_EQUAL_HEX8(CHAR_MAX, max);
	TEST_ASSERT_EQUAL_HEX8(0, zero);
	TEST_ASSERT_EQUAL_HEX8(CHAR_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_HEX8(CHAR_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_HEX8(UCHAR_MAX, umax);
}


TEST(stdio_scanf_x, hx)
{
	char buff[BUFF_LEN] = { 0 };
	short max, min, zero, hmin, hmax;
	unsigned short umax;
	const char *format = "%hx %hx %hx %hx %hx %hx";

	sprintf(buff, format, SHRT_MAX, SHRT_MAX / 2, (short)0, SHRT_MIN / 2, SHRT_MIN, USHRT_MAX);

	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(6, test_vfscanfWrapper(filep, format, &max, &hmax, &zero, &hmin, &min, &umax));

	TEST_ASSERT_EQUAL_HEX16(SHRT_MIN, min);
	TEST_ASSERT_EQUAL_HEX16(SHRT_MAX, max);
	TEST_ASSERT_EQUAL_HEX16(0, zero);
	TEST_ASSERT_EQUAL_HEX16(SHRT_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_HEX16(SHRT_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_HEX16(USHRT_MAX, umax);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(6, fscanf(filep, format, &max, &hmax, &zero, &hmin, &min, &umax));

	TEST_ASSERT_EQUAL_HEX16(SHRT_MIN, min);
	TEST_ASSERT_EQUAL_HEX16(SHRT_MAX, max);
	TEST_ASSERT_EQUAL_HEX16(0, zero);
	TEST_ASSERT_EQUAL_HEX16(SHRT_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_HEX16(SHRT_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_HEX16(USHRT_MAX, umax);

	TEST_ASSERT_EQUAL_INT(6, test_vsscanfWrapper(buff, format, &max, &hmax, &zero, &hmin, &min, &umax));

	TEST_ASSERT_EQUAL_HEX16(SHRT_MIN, min);
	TEST_ASSERT_EQUAL_HEX16(SHRT_MAX, max);
	TEST_ASSERT_EQUAL_HEX16(0, zero);
	TEST_ASSERT_EQUAL_HEX16(SHRT_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_HEX16(SHRT_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_HEX16(USHRT_MAX, umax);

	TEST_ASSERT_EQUAL_INT(6, sscanf(buff, format, &max, &hmax, &zero, &hmin, &min, &umax));

	TEST_ASSERT_EQUAL_HEX16(SHRT_MIN, min);
	TEST_ASSERT_EQUAL_HEX16(SHRT_MAX, max);
	TEST_ASSERT_EQUAL_HEX16(0, zero);
	TEST_ASSERT_EQUAL_HEX16(SHRT_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_HEX16(SHRT_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_HEX16(USHRT_MAX, umax);
}


TEST(stdio_scanf_x, lx)
{
	char buff[BUFF_LEN] = { 0 };
	long max, min, zero, hmin, hmax;
	unsigned long umax;
	const char *format = "%lx %lx %lx %lx %lx %lx";

	sprintf(buff, format, LONG_MAX, LONG_MAX / 2, (long)0, LONG_MIN / 2, LONG_MIN, ULONG_MAX);

	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(6, test_vfscanfWrapper(filep, format, &max, &hmax, &zero, &hmin, &min, &umax));

	TEST_ASSERT_EQUAL_HEX64(LONG_MIN, min);
	TEST_ASSERT_EQUAL_HEX64(LONG_MAX, max);
	TEST_ASSERT_EQUAL_HEX64(0, zero);
	TEST_ASSERT_EQUAL_HEX64(LONG_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_HEX64(LONG_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_HEX64(ULONG_MAX, umax);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(6, fscanf(filep, format, &max, &hmax, &zero, &hmin, &min, &umax));

	TEST_ASSERT_EQUAL_HEX64(LONG_MIN, min);
	TEST_ASSERT_EQUAL_HEX64(LONG_MAX, max);
	TEST_ASSERT_EQUAL_HEX64(0, zero);
	TEST_ASSERT_EQUAL_HEX64(LONG_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_HEX64(LONG_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_HEX64(ULONG_MAX, umax);

	TEST_ASSERT_EQUAL_INT(6, test_vsscanfWrapper(buff, format, &max, &hmax, &zero, &hmin, &min, &umax));

	TEST_ASSERT_EQUAL_HEX64(LONG_MIN, min);
	TEST_ASSERT_EQUAL_HEX64(LONG_MAX, max);
	TEST_ASSERT_EQUAL_HEX64(0, zero);
	TEST_ASSERT_EQUAL_HEX64(LONG_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_HEX64(LONG_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_HEX64(ULONG_MAX, umax);

	TEST_ASSERT_EQUAL_INT(6, sscanf(buff, format, &max, &hmax, &zero, &hmin, &min, &umax));

	TEST_ASSERT_EQUAL_HEX64(LONG_MIN, min);
	TEST_ASSERT_EQUAL_HEX64(LONG_MAX, max);
	TEST_ASSERT_EQUAL_HEX64(0, zero);
	TEST_ASSERT_EQUAL_HEX64(LONG_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_HEX64(LONG_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_HEX64(ULONG_MAX, umax);
}


TEST(stdio_scanf_x, llx)
{
	char buff[BUFF_LEN] = { 0 };
	long long max, min, zero, hmin, hmax;
	unsigned long long umax;
	const char *format = "%llx %llx %llx %llx %llx %llx";

	sprintf(buff, format, LLONG_MAX, LLONG_MAX / 2, (long long)0, LLONG_MIN / 2, LLONG_MIN, ULLONG_MAX);

	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(6, test_vfscanfWrapper(filep, format, &max, &hmax, &zero, &hmin, &min, &umax));

	TEST_ASSERT_EQUAL_HEX64(LLONG_MIN, min);
	TEST_ASSERT_EQUAL_HEX64(LLONG_MAX, max);
	TEST_ASSERT_EQUAL_HEX64(0, zero);
	TEST_ASSERT_EQUAL_HEX64(LLONG_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_HEX64(LLONG_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_HEX64(ULLONG_MAX, umax);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(6, fscanf(filep, format, &max, &hmax, &zero, &hmin, &min, &umax));

	TEST_ASSERT_EQUAL_HEX64(LLONG_MIN, min);
	TEST_ASSERT_EQUAL_HEX64(LLONG_MAX, max);
	TEST_ASSERT_EQUAL_HEX64(0, zero);
	TEST_ASSERT_EQUAL_HEX64(LLONG_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_HEX64(LLONG_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_HEX64(ULLONG_MAX, umax);

	TEST_ASSERT_EQUAL_INT(6, test_vsscanfWrapper(buff, format, &max, &hmax, &zero, &hmin, &min, &umax));

	TEST_ASSERT_EQUAL_HEX64(LLONG_MIN, min);
	TEST_ASSERT_EQUAL_HEX64(LLONG_MAX, max);
	TEST_ASSERT_EQUAL_HEX64(0, zero);
	TEST_ASSERT_EQUAL_HEX64(LLONG_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_HEX64(LLONG_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_HEX64(ULLONG_MAX, umax);

	TEST_ASSERT_EQUAL_INT(6, sscanf(buff, format, &max, &hmax, &zero, &hmin, &min, &umax));

	TEST_ASSERT_EQUAL_HEX64(LLONG_MIN, min);
	TEST_ASSERT_EQUAL_HEX64(LLONG_MAX, max);
	TEST_ASSERT_EQUAL_HEX64(0, zero);
	TEST_ASSERT_EQUAL_HEX64(LLONG_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_HEX64(LLONG_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_HEX64(ULLONG_MAX, umax);
}


TEST(stdio_scanf_x, jx)
{
	char buff[BUFF_LEN] = { 0 };
	intmax_t max, hmax, zero, hmin, min;
	uintmax_t umax, uzero, uhmax;

	const char *format = "%jx %jx %jx %jx %jx";
	const char *uformat = "%jx %jx %jx";

	sprintf(buff, format, (intmax_t)INTMAX_MAX, (intmax_t)INTMAX_MAX / 2, (intmax_t)0, (intmax_t)INTMAX_MIN / 2, (intmax_t)INTMAX_MIN);
	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(5, test_vfscanfWrapper(filep, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_UINT64(INTMAX_MAX, max);
	TEST_ASSERT_EQUAL_UINT64(INTMAX_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_UINT64(0, zero);
	TEST_ASSERT_EQUAL_UINT64(INTMAX_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_UINT64(INTMAX_MIN, min);

	rewind(filep);
	TEST_ASSERT_EQUAL_INT(5, fscanf(filep, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_UINT64(INTMAX_MAX, max);
	TEST_ASSERT_EQUAL_UINT64(INTMAX_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_UINT64(0, zero);
	TEST_ASSERT_EQUAL_UINT64(INTMAX_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_UINT64(INTMAX_MIN, min);

	TEST_ASSERT_EQUAL_INT(5, test_vsscanfWrapper(buff, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_UINT64(INTMAX_MAX, max);
	TEST_ASSERT_EQUAL_UINT64(INTMAX_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_UINT64(0, zero);
	TEST_ASSERT_EQUAL_UINT64(INTMAX_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_UINT64(INTMAX_MIN, min);

	TEST_ASSERT_EQUAL_INT(5, sscanf(buff, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_UINT64(INTMAX_MAX, max);
	TEST_ASSERT_EQUAL_UINT64(INTMAX_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_UINT64(0, zero);
	TEST_ASSERT_EQUAL_UINT64(INTMAX_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_UINT64(INTMAX_MIN, min);

	fclose(filep);
	filep = fopen(TESTFILE_PATH, "w+");
	rewind(filep);
	sprintf(buff, uformat, UINTMAX_MAX, UINTMAX_MAX / 2, (intmax_t)0);
	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(3, test_vfscanfWrapper(filep, uformat, &umax, &uhmax, &uzero));

	TEST_ASSERT_EQUAL_UINT64(UINTMAX_MAX, umax);
	TEST_ASSERT_EQUAL_UINT64(UINTMAX_MAX / 2, uhmax);
	TEST_ASSERT_EQUAL_UINT64(0, uzero);

	rewind(filep);
	TEST_ASSERT_EQUAL_INT(3, fscanf(filep, uformat, &umax, &uhmax, &uzero));

	TEST_ASSERT_EQUAL_UINT64(UINTMAX_MAX, umax);
	TEST_ASSERT_EQUAL_UINT64(UINTMAX_MAX / 2, uhmax);
	TEST_ASSERT_EQUAL_UINT64(0, uzero);

	TEST_ASSERT_EQUAL_INT(3, test_vsscanfWrapper(buff, uformat, &umax, &uhmax, &uzero));

	TEST_ASSERT_EQUAL_UINT64(UINTMAX_MAX, umax);
	TEST_ASSERT_EQUAL_UINT64(UINTMAX_MAX / 2, uhmax);
	TEST_ASSERT_EQUAL_UINT64(0, uzero);

	TEST_ASSERT_EQUAL_INT(3, sscanf(buff, uformat, &umax, &uhmax, &uzero));

	TEST_ASSERT_EQUAL_UINT64(UINTMAX_MAX, umax);
	TEST_ASSERT_EQUAL_UINT64(UINTMAX_MAX / 2, uhmax);
	TEST_ASSERT_EQUAL_UINT64(0, uzero);
}


TEST(stdio_scanf_x, zx)
{
	char buff[BUFF_LEN] = { 0 };
	size_t max, zero, hmax;
	const char *format = "%zx %zx %zx";

	sprintf(buff, format, (size_t)SSIZE_MAX, (size_t)SSIZE_MAX / 2, (size_t)0);

	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(3, test_vfscanfWrapper(filep, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_HEX64(SSIZE_MAX, max);
	TEST_ASSERT_EQUAL_HEX64(0, zero);
	TEST_ASSERT_EQUAL_HEX64(SSIZE_MAX / 2, hmax);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(3, fscanf(filep, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_HEX64(SSIZE_MAX, max);
	TEST_ASSERT_EQUAL_HEX64(0, zero);
	TEST_ASSERT_EQUAL_HEX64(SSIZE_MAX / 2, hmax);

	TEST_ASSERT_EQUAL_INT(3, test_vsscanfWrapper(buff, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_HEX64(SSIZE_MAX, max);
	TEST_ASSERT_EQUAL_HEX64(0, zero);
	TEST_ASSERT_EQUAL_HEX64(SSIZE_MAX / 2, hmax);

	TEST_ASSERT_EQUAL_INT(3, sscanf(buff, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_HEX64(SSIZE_MAX, max);
	TEST_ASSERT_EQUAL_HEX64(0, zero);
	TEST_ASSERT_EQUAL_HEX64(SSIZE_MAX / 2, hmax);
}


TEST(stdio_scanf_x, tx)
{
	char buff[BUFF_LEN] = { 0 };
	ptrdiff_t max, min, zero, hmin, hmax;
	const char *format = "%tx %tx %tx %tx %tx";

	sprintf(buff, format, (ptrdiff_t)PTRDIFF_MAX, (ptrdiff_t)PTRDIFF_MAX / 2, (ptrdiff_t)0, (ptrdiff_t)PTRDIFF_MIN / 2,
		(ptrdiff_t)PTRDIFF_MIN);

	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(5, test_vfscanfWrapper(filep, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_HEX64(PTRDIFF_MIN, min);
	TEST_ASSERT_EQUAL_HEX64(PTRDIFF_MAX, max);
	TEST_ASSERT_EQUAL_HEX64(0, zero);
	TEST_ASSERT_EQUAL_HEX64(PTRDIFF_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_HEX64(PTRDIFF_MAX / 2, hmax);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(5, fscanf(filep, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_HEX64(PTRDIFF_MIN, min);
	TEST_ASSERT_EQUAL_HEX64(PTRDIFF_MAX, max);
	TEST_ASSERT_EQUAL_HEX64(0, zero);
	TEST_ASSERT_EQUAL_HEX64(PTRDIFF_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_HEX64(PTRDIFF_MAX / 2, hmax);

	TEST_ASSERT_EQUAL_INT(5, test_vsscanfWrapper(buff, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_HEX64(PTRDIFF_MIN, min);
	TEST_ASSERT_EQUAL_HEX64(PTRDIFF_MAX, max);
	TEST_ASSERT_EQUAL_HEX64(0, zero);
	TEST_ASSERT_EQUAL_HEX64(PTRDIFF_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_HEX64(PTRDIFF_MAX / 2, hmax);

	TEST_ASSERT_EQUAL_INT(5, sscanf(buff, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_HEX64(PTRDIFF_MIN, min);
	TEST_ASSERT_EQUAL_HEX64(PTRDIFF_MAX, max);
	TEST_ASSERT_EQUAL_HEX64(0, zero);
	TEST_ASSERT_EQUAL_HEX64(PTRDIFF_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_HEX64(PTRDIFF_MAX / 2, hmax);
}


/*
//////////////////////////////////////////////////////////////////////////////////////////////////
*/


TEST(stdio_scanf_x, X)
{
	char buff[BUFF_LEN] = { 0 };
	unsigned int max, min, zero, hmin, hmax;
	const char *format = "%X %X %X";

	sprintf(buff, format, UINT_MAX, UINT_MAX / 2, 0);

	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(3, test_vfscanfWrapper(filep, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_HEX(UINT_MAX, max);
	TEST_ASSERT_EQUAL_HEX(UINT_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_HEX(0, zero);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(3, fscanf(filep, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_HEX(UINT_MAX, max);
	TEST_ASSERT_EQUAL_HEX(UINT_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_HEX(0, zero);

	TEST_ASSERT_EQUAL_INT(3, test_vsscanfWrapper(buff, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_HEX(UINT_MAX, max);
	TEST_ASSERT_EQUAL_HEX(UINT_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_HEX(0, zero);

	TEST_ASSERT_EQUAL_INT(3, sscanf(buff, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_HEX(UINT_MAX, max);
	TEST_ASSERT_EQUAL_HEX(UINT_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_HEX(0, zero);
}


TEST(stdio_scanf_x, hhX)
{
	char max, min, zero, hmin, hmax, buff[BUFF_LEN] = { 0 };
	const char *format = "%hhX %hhX %hhX %hhX %hhX";

	sprintf(buff, format, CHAR_MAX, CHAR_MAX / 2, (char)0, CHAR_MIN / 2, CHAR_MIN);

	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(5, test_vfscanfWrapper(filep, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_HEX8(CHAR_MIN, min);
	TEST_ASSERT_EQUAL_HEX8(CHAR_MAX, max);
	TEST_ASSERT_EQUAL_HEX8(0, zero);
	TEST_ASSERT_EQUAL_HEX8(CHAR_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_HEX8(CHAR_MAX / 2, hmax);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(5, fscanf(filep, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_HEX8(CHAR_MIN, min);
	TEST_ASSERT_EQUAL_HEX8(CHAR_MAX, max);
	TEST_ASSERT_EQUAL_HEX8(0, zero);
	TEST_ASSERT_EQUAL_HEX8(CHAR_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_HEX8(CHAR_MAX / 2, hmax);

	TEST_ASSERT_EQUAL_INT(5, test_vsscanfWrapper(buff, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_HEX8(CHAR_MIN, min);
	TEST_ASSERT_EQUAL_HEX8(CHAR_MAX, max);
	TEST_ASSERT_EQUAL_HEX8(0, zero);
	TEST_ASSERT_EQUAL_HEX8(CHAR_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_HEX8(CHAR_MAX / 2, hmax);

	TEST_ASSERT_EQUAL_INT(5, sscanf(buff, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_HEX8(CHAR_MIN, min);
	TEST_ASSERT_EQUAL_HEX8(CHAR_MAX, max);
	TEST_ASSERT_EQUAL_HEX8(0, zero);
	TEST_ASSERT_EQUAL_HEX8(CHAR_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_HEX8(CHAR_MAX / 2, hmax);
}


TEST(stdio_scanf_x, hX)
{
	char buff[BUFF_LEN] = { 0 };
	short max, min, zero, hmin, hmax;
	const char *format = "%hX %hX %hX %hX %hX";

	sprintf(buff, format, SHRT_MAX, SHRT_MAX / 2, (short)0, SHRT_MIN / 2, SHRT_MIN);

	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(5, test_vfscanfWrapper(filep, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_HEX16(SHRT_MIN, min);
	TEST_ASSERT_EQUAL_HEX16(SHRT_MAX, max);
	TEST_ASSERT_EQUAL_HEX16(0, zero);
	TEST_ASSERT_EQUAL_HEX16(SHRT_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_HEX16(SHRT_MAX / 2, hmax);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(5, fscanf(filep, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_HEX16(SHRT_MIN, min);
	TEST_ASSERT_EQUAL_HEX16(SHRT_MAX, max);
	TEST_ASSERT_EQUAL_HEX16(0, zero);
	TEST_ASSERT_EQUAL_HEX16(SHRT_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_HEX16(SHRT_MAX / 2, hmax);

	TEST_ASSERT_EQUAL_INT(5, test_vsscanfWrapper(buff, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_HEX16(SHRT_MIN, min);
	TEST_ASSERT_EQUAL_HEX16(SHRT_MAX, max);
	TEST_ASSERT_EQUAL_HEX16(0, zero);
	TEST_ASSERT_EQUAL_HEX16(SHRT_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_HEX16(SHRT_MAX / 2, hmax);

	TEST_ASSERT_EQUAL_INT(5, sscanf(buff, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_HEX16(SHRT_MIN, min);
	TEST_ASSERT_EQUAL_HEX16(SHRT_MAX, max);
	TEST_ASSERT_EQUAL_HEX16(0, zero);
	TEST_ASSERT_EQUAL_HEX16(SHRT_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_HEX16(SHRT_MAX / 2, hmax);
}


TEST(stdio_scanf_x, lX)
{
	char buff[BUFF_LEN] = { 0 };
	long max, min, zero, hmin, hmax;
	const char *format = "%lX %lX %lX %lX %lX";

	sprintf(buff, format, LONG_MAX, LONG_MAX / 2, (long)0, LONG_MIN / 2, LONG_MIN);

	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(5, test_vfscanfWrapper(filep, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_HEX64(LONG_MIN, min);
	TEST_ASSERT_EQUAL_HEX64(LONG_MAX, max);
	TEST_ASSERT_EQUAL_HEX64(0, zero);
	TEST_ASSERT_EQUAL_HEX64(LONG_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_HEX64(LONG_MAX / 2, hmax);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(5, fscanf(filep, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_HEX64(LONG_MIN, min);
	TEST_ASSERT_EQUAL_HEX64(LONG_MAX, max);
	TEST_ASSERT_EQUAL_HEX64(0, zero);
	TEST_ASSERT_EQUAL_HEX64(LONG_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_HEX64(LONG_MAX / 2, hmax);

	TEST_ASSERT_EQUAL_INT(5, test_vsscanfWrapper(buff, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_HEX64(LONG_MIN, min);
	TEST_ASSERT_EQUAL_HEX64(LONG_MAX, max);
	TEST_ASSERT_EQUAL_HEX64(0, zero);
	TEST_ASSERT_EQUAL_HEX64(LONG_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_HEX64(LONG_MAX / 2, hmax);

	TEST_ASSERT_EQUAL_INT(5, sscanf(buff, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_HEX64(LONG_MIN, min);
	TEST_ASSERT_EQUAL_HEX64(LONG_MAX, max);
	TEST_ASSERT_EQUAL_HEX64(0, zero);
	TEST_ASSERT_EQUAL_HEX64(LONG_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_HEX64(LONG_MAX / 2, hmax);
}


TEST(stdio_scanf_x, llX)
{
	char buff[BUFF_LEN] = { 0 };
	long long max, min, zero, hmin, hmax;
	const char *format = "%llX %llX %llX %llX %llX";

	sprintf(buff, format, LLONG_MAX, LLONG_MAX / 2, (long long)0, LLONG_MIN / 2, LLONG_MIN);

	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(5, test_vfscanfWrapper(filep, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_HEX64(LLONG_MIN, min);
	TEST_ASSERT_EQUAL_HEX64(LLONG_MAX, max);
	TEST_ASSERT_EQUAL_HEX64(0, zero);
	TEST_ASSERT_EQUAL_HEX64(LLONG_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_HEX64(LLONG_MAX / 2, hmax);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(5, fscanf(filep, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_HEX64(LLONG_MIN, min);
	TEST_ASSERT_EQUAL_HEX64(LLONG_MAX, max);
	TEST_ASSERT_EQUAL_HEX64(0, zero);
	TEST_ASSERT_EQUAL_HEX64(LLONG_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_HEX64(LLONG_MAX / 2, hmax);

	TEST_ASSERT_EQUAL_INT(5, test_vsscanfWrapper(buff, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_HEX64(LLONG_MIN, min);
	TEST_ASSERT_EQUAL_HEX64(LLONG_MAX, max);
	TEST_ASSERT_EQUAL_HEX64(0, zero);
	TEST_ASSERT_EQUAL_HEX64(LLONG_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_HEX64(LLONG_MAX / 2, hmax);

	TEST_ASSERT_EQUAL_INT(5, sscanf(buff, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_HEX64(LLONG_MIN, min);
	TEST_ASSERT_EQUAL_HEX64(LLONG_MAX, max);
	TEST_ASSERT_EQUAL_HEX64(0, zero);
	TEST_ASSERT_EQUAL_HEX64(LLONG_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_HEX64(LLONG_MAX / 2, hmax);
}


TEST(stdio_scanf_x, jX)
{
	char buff[BUFF_LEN] = { 0 };
	intmax_t max, zero, hmax;
	uintmax_t umax, uzero, uhmax;
	const char *format = "%jX %jX %jX";

	sprintf(buff, format, (intmax_t)INTMAX_MAX, (intmax_t)INTMAX_MAX / 2, (intmax_t)0);
	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(3, test_vfscanfWrapper(filep, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_UINT64(INTMAX_MAX, max);
	TEST_ASSERT_EQUAL_UINT64(INTMAX_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_UINT64(0, zero);

	rewind(filep);
	TEST_ASSERT_EQUAL_INT(3, fscanf(filep, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_UINT64(INTMAX_MAX, max);
	TEST_ASSERT_EQUAL_UINT64(INTMAX_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_UINT64(0, zero);

	TEST_ASSERT_EQUAL_INT(3, test_vsscanfWrapper(buff, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_UINT64(INTMAX_MAX, max);
	TEST_ASSERT_EQUAL_UINT64(INTMAX_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_UINT64(0, zero);


	TEST_ASSERT_EQUAL_INT(3, sscanf(buff, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_UINT64(INTMAX_MAX, max);
	TEST_ASSERT_EQUAL_UINT64(INTMAX_MAX / 2, hmax);
	TEST_ASSERT_EQUAL_UINT64(0, zero);

	filep = fopen(TESTFILE_PATH, "w+");
	rewind(filep);
	sprintf(buff, format, UINTMAX_MAX, UINTMAX_MAX / 2, (intmax_t)0);
	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(3, test_vfscanfWrapper(filep, format, &umax, &uhmax, &uzero));

	TEST_ASSERT_EQUAL_UINT64(UINTMAX_MAX, umax);
	TEST_ASSERT_EQUAL_UINT64(UINTMAX_MAX / 2, uhmax);
	TEST_ASSERT_EQUAL_UINT64(0, uzero);

	rewind(filep);
	TEST_ASSERT_EQUAL_INT(3, fscanf(filep, format, &umax, &uhmax, &uzero));

	TEST_ASSERT_EQUAL_UINT64(UINTMAX_MAX, umax);
	TEST_ASSERT_EQUAL_UINT64(UINTMAX_MAX / 2, uhmax);
	TEST_ASSERT_EQUAL_UINT64(0, uzero);

	TEST_ASSERT_EQUAL_INT(3, test_vsscanfWrapper(buff, format, &umax, &uhmax, &uzero));

	TEST_ASSERT_EQUAL_UINT64(UINTMAX_MAX, umax);
	TEST_ASSERT_EQUAL_UINT64(UINTMAX_MAX / 2, uhmax);
	TEST_ASSERT_EQUAL_UINT64(0, uzero);

	TEST_ASSERT_EQUAL_INT(3, sscanf(buff, format, &umax, &uhmax, &uzero));

	TEST_ASSERT_EQUAL_UINT64(UINTMAX_MAX, umax);
	TEST_ASSERT_EQUAL_UINT64(UINTMAX_MAX / 2, uhmax);
	TEST_ASSERT_EQUAL_UINT64(0, uzero);
}


TEST(stdio_scanf_x, zX)
{
	char buff[BUFF_LEN] = { 0 };
	size_t max, zero, hmax;
	const char *format = "%zX %zX %zX";

	sprintf(buff, format, (size_t)SSIZE_MAX, (size_t)SSIZE_MAX / 2, (size_t)0);

	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(3, test_vfscanfWrapper(filep, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_HEX64(SSIZE_MAX, max);
	TEST_ASSERT_EQUAL_HEX64(0, zero);
	TEST_ASSERT_EQUAL_HEX64(SSIZE_MAX / 2, hmax);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(3, fscanf(filep, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_HEX64(SSIZE_MAX, max);
	TEST_ASSERT_EQUAL_HEX64(0, zero);
	TEST_ASSERT_EQUAL_HEX64(SSIZE_MAX / 2, hmax);

	TEST_ASSERT_EQUAL_INT(3, test_vsscanfWrapper(buff, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_HEX64(SSIZE_MAX, max);
	TEST_ASSERT_EQUAL_HEX64(0, zero);
	TEST_ASSERT_EQUAL_HEX64(SSIZE_MAX / 2, hmax);

	TEST_ASSERT_EQUAL_INT(3, sscanf(buff, format, &max, &hmax, &zero));

	TEST_ASSERT_EQUAL_HEX64(SSIZE_MAX, max);
	TEST_ASSERT_EQUAL_HEX64(0, zero);
	TEST_ASSERT_EQUAL_HEX64(SSIZE_MAX / 2, hmax);
}


TEST(stdio_scanf_x, tX)
{
	char buff[BUFF_LEN] = { 0 };
	ptrdiff_t max, min, zero, hmin, hmax;
	const char *format = "%tX %tX %tX %tX %tX";

	sprintf(buff, format, (ptrdiff_t)PTRDIFF_MAX, (ptrdiff_t)PTRDIFF_MAX / 2, (ptrdiff_t)0, (ptrdiff_t)PTRDIFF_MIN / 2,
		(ptrdiff_t)PTRDIFF_MIN);

	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(5, test_vfscanfWrapper(filep, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_HEX64(PTRDIFF_MIN, min);
	TEST_ASSERT_EQUAL_HEX64(PTRDIFF_MAX, max);
	TEST_ASSERT_EQUAL_HEX64(0, zero);
	TEST_ASSERT_EQUAL_HEX64(PTRDIFF_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_HEX64(PTRDIFF_MAX / 2, hmax);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(5, fscanf(filep, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_HEX64(PTRDIFF_MIN, min);
	TEST_ASSERT_EQUAL_HEX64(PTRDIFF_MAX, max);
	TEST_ASSERT_EQUAL_HEX64(0, zero);
	TEST_ASSERT_EQUAL_HEX64(PTRDIFF_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_HEX64(PTRDIFF_MAX / 2, hmax);

	TEST_ASSERT_EQUAL_INT(5, test_vsscanfWrapper(buff, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_HEX64(PTRDIFF_MIN, min);
	TEST_ASSERT_EQUAL_HEX64(PTRDIFF_MAX, max);
	TEST_ASSERT_EQUAL_HEX64(0, zero);
	TEST_ASSERT_EQUAL_HEX64(PTRDIFF_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_HEX64(PTRDIFF_MAX / 2, hmax);

	TEST_ASSERT_EQUAL_INT(5, sscanf(buff, format, &max, &hmax, &zero, &hmin, &min));

	TEST_ASSERT_EQUAL_HEX64(PTRDIFF_MIN, min);
	TEST_ASSERT_EQUAL_HEX64(PTRDIFF_MAX, max);
	TEST_ASSERT_EQUAL_HEX64(0, zero);
	TEST_ASSERT_EQUAL_HEX64(PTRDIFF_MIN / 2, hmin);
	TEST_ASSERT_EQUAL_HEX64(PTRDIFF_MAX / 2, hmax);
}


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
	/*
	 * <posix incmpliance> Scanf doesn't support longer floating-point numbers
	 * Problems occur when scanf needs to read big numbers after coma.
	 * example: 0.234234345345 scanf will read
	 * example 2.212121e-100 scanf won't read because this is along float after a coma.
	 * Issue link: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/652
	 */

#ifdef __phoenix__
	TEST_IGNORE();
#endif

	char buff[BUFF_LEN_FLOAT] = { 0 };
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

	TEST_ASSERT_EQUAL_INT(7, test_vfscanfWrapper(filep, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	/* This block contains all asserts from min to max for float */
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(7, fscanf(filep, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);

	TEST_ASSERT_EQUAL_INT(7, test_vsscanfWrapper(buff, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);

	TEST_ASSERT_EQUAL_INT(7, sscanf(buff, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);
}


/*
////////////////////////////////////////////////////////////////////////////////////////////////////
*/


TEST(stdio_scanf_aefg, F)
{
	/*
	 * <posix incmpliance> Scanf doesn't support longer floating-point numbers
	 * Problems occur when scanf needs to read big numbers after coma.
	 * example: 0.234234345345 scanf will read
	 * example 2.212121e-100 scanf won't read because this is along float after a coma.
	 * Issue link: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/652
	 */

#ifdef __phoenix__
	TEST_IGNORE();
#endif

	char buff[BUFF_LEN_FLOAT] = { 0 };
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

	TEST_ASSERT_EQUAL_INT(7, test_vfscanfWrapper(filep, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	/* This block contains all asserts from min to max for float */
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(7, fscanf(filep, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);

	TEST_ASSERT_EQUAL_INT(7, test_vsscanfWrapper(buff, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);

	TEST_ASSERT_EQUAL_INT(7, sscanf(buff, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);
}


/*
////////////////////////////////////////////////////////////////////////////////////////////////////
*/


TEST(stdio_scanf_aefg, a)
{
	/*
	 * <posix incmpliance> Scanf doesn't support longer floating-point numbers
	 * Problems occur when scanf needs to read big numbers after coma.
	 * example: 0.234234345345 scanf will read
	 * example 2.212121e-100 scanf won't read because this is along float after a coma.
	 * Issue link: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/652
	 */

#ifdef __phoenix__
	TEST_IGNORE();
#endif

	char buff[BUFF_LEN] = { 0 };
	float fltMax, fltMaxH, zero, fltMin, negFltMax, negFltMaxH, negFltMin;
	const char *format = "%a %a %a %a %a %a %a";

	sprintf(buff, format, FLT_MAX, FLT_MAX / 2, FLT_MIN, 0.f, FLT_MIN * -1, (FLT_MAX / 2) * -1, FLT_MAX * -1);
	fprintf(filep, "%s", buff);
	rewind(filep);

	test_vfscanfWrapper(filep, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax);
	/* This block contains all asserts from min to max for float */
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(7, fscanf(filep, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);

	TEST_ASSERT_EQUAL_INT(7, test_vsscanfWrapper(buff, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);

	TEST_ASSERT_EQUAL_INT(7, sscanf(buff, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);
}


/*
////////////////////////////////////////////////////////////////////////////////////////////////////
*/


TEST(stdio_scanf_aefg, A)
{
	/*
	 * <posix incmpliance> Scanf doesn't support longer floating-point numbers
	 * Problems occur when scanf needs to read big numbers after coma.
	 * example: 0.234234345345 scanf will read
	 * example 2.212121e-100 scanf won't read because this is along float after a coma.
	 * Issue link: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/652
	 */

#ifdef __phoenix__
	TEST_IGNORE();
#endif

	char buff[BUFF_LEN] = { 0 };
	float fltMax, fltMaxH, zero, fltMin, negFltMax, negFltMaxH, negFltMin;
	const char *format = "%A %A %A %A %A %A %A";

	sprintf(buff, format, FLT_MAX, FLT_MAX / 2, FLT_MIN, 0.f, FLT_MIN * -1, (FLT_MAX / 2) * -1, FLT_MAX * -1);
	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(7, test_vfscanfWrapper(filep, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	/* This block contains all asserts from min to max for float */
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(7, fscanf(filep, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);

	TEST_ASSERT_EQUAL_INT(7, test_vsscanfWrapper(buff, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);

	TEST_ASSERT_EQUAL_INT(7, sscanf(buff, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);
}


/*
////////////////////////////////////////////////////////////////////////////////////////////////////
*/


TEST(stdio_scanf_aefg, e)
{
	/*
	 * <posix incmpliance> Scanf doesn't support longer floating-point numbers
	 * Problems occur when scanf needs to read big numbers after coma.
	 * example: 0.234234345345 scanf will read
	 * example 2.212121e-100 scanf won't read because this is along float after a coma.
	 * Issue link: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/652
	 */

#ifdef __phoenix__
	TEST_IGNORE();
#endif

	char buff[BUFF_LEN] = { 0 };
	float fltMax, fltMaxH, zero, fltMin, negFltMax, negFltMaxH, negFltMin;
	const char *format = "%e %e %e %e %e %e %e";

	sprintf(buff, format, FLT_MAX, FLT_MAX / 2, FLT_MIN, 0.f, FLT_MIN * -1, (FLT_MAX / 2) * -1, FLT_MAX * -1);
	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(7, test_vfscanfWrapper(filep, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	/* This block contains all asserts from min to max for float */
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(7, fscanf(filep, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);

	TEST_ASSERT_EQUAL_INT(7, test_vsscanfWrapper(buff, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);

	TEST_ASSERT_EQUAL_INT(7, sscanf(buff, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);
}


/*
////////////////////////////////////////////////////////////////////////////////////////////////////
*/


TEST(stdio_scanf_aefg, E)
{
	/*
	 * <posix incmpliance> Scanf doesn't support longer floating-point numbers
	 * Problems occur when scanf needs to read big numbers after coma.
	 * example: 0.234234345345 scanf will read
	 * example 2.212121e-100 scanf won't read because this is along float after a coma.
	 * Issue link: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/652
	 */

#ifdef __phoenix__
	TEST_IGNORE();
#endif

	char buff[BUFF_LEN] = { 0 };
	float fltMax, fltMaxH, zero, fltMin, negFltMax, negFltMaxH, negFltMin;
	const char *format = "%E %E %E %E %E %E %E";

	sprintf(buff, format, FLT_MAX, FLT_MAX / 2, FLT_MIN, 0.f, FLT_MIN * -1, (FLT_MAX / 2) * -1, FLT_MAX * -1);
	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(7, test_vfscanfWrapper(filep, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	/* This block contains all asserts from min to max for float */
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(7, fscanf(filep, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);

	TEST_ASSERT_EQUAL_INT(7, test_vsscanfWrapper(buff, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);

	TEST_ASSERT_EQUAL_INT(7, sscanf(buff, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);
}


/*
////////////////////////////////////////////////////////////////////////////////////////////////////
*/


TEST(stdio_scanf_aefg, g)
{
	/*
	 * <posix incmpliance> Scanf doesn't support longer floating-point numbers
	 * Problems occur when scanf needs to read big numbers after coma.
	 * example: 0.234234345345 scanf will read
	 * example 2.212121e-100 scanf won't read because this is along float after a coma.
	 * Issue link: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/652
	 */

#ifdef __phoenix__
	TEST_IGNORE();
#endif

	char buff[BUFF_LEN] = { 0 };
	float fltMax, fltMaxH, zero, fltMin, negFltMax, negFltMaxH, negFltMin;
	const char *format = "%g %g %g %g %g %g %g";

	sprintf(buff, format, FLT_MAX, FLT_MAX / 2, FLT_MIN, 0.f, FLT_MIN * -1, (FLT_MAX / 2) * -1, FLT_MAX * -1);
	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(7, test_vfscanfWrapper(filep, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	/* This block contains all asserts from min to max for float */
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(7, fscanf(filep, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);

	TEST_ASSERT_EQUAL_INT(7, test_vsscanfWrapper(buff, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);

	TEST_ASSERT_EQUAL_INT(7, sscanf(buff, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);
}


/*
////////////////////////////////////////////////////////////////////////////////////////////////////
*/


TEST(stdio_scanf_aefg, G)
{
	/*
	 * <posix incmpliance> Scanf doesn't support longer floating-point numbers
	 * Problems occur when scanf needs to read big numbers after coma.
	 * example: 0.234234345345 scanf will read
	 * example 2.212121e-100 scanf won't read because this is along float after a coma.
	 * Issue link: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/652
	 */

#ifdef __phoenix__
	TEST_IGNORE();
#endif

	char buff[BUFF_LEN] = { 0 };
	float fltMax, fltMaxH, zero, fltMin, negFltMax, negFltMaxH, negFltMin;
	const char *format = "%G %G %G %G %G %G %G";

	sprintf(buff, format, FLT_MAX, FLT_MAX / 2, FLT_MIN, 0.f, FLT_MIN * -1, (FLT_MAX / 2) * -1, FLT_MAX * -1);
	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(7, test_vfscanfWrapper(filep, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	/* This block contains all asserts from min to max for float */
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(7, fscanf(filep, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);

	TEST_ASSERT_EQUAL_INT(7, test_vsscanfWrapper(buff, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);

	TEST_ASSERT_EQUAL_INT(7, sscanf(buff, format, &fltMax, &fltMaxH, &fltMin, &zero, &negFltMin, &negFltMaxH, &negFltMax));
	TEST_ASSERT_FLOAT_SET(fltMax, fltMaxH, fltMin, zero, negFltMin, negFltMaxH, negFltMax);
}


/*
////////////////////////////////////////////////////////////////////////////////////////////////////
*/


TEST(stdio_scanf_aefg, inf_nan_f)
{
	/*
	 * <posix incmpliance> Scanf Inf and Nan handling problem
	 * There is a problem with Scanf where it is unable to read any representation of valInf or valNan.
	 * Issue link: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/634
	 */
#ifdef __phoenix__
	TEST_IGNORE();
#endif

	char buff[BUFF_LEN_FLOAT] = { 0 };
	double valInf, valNan, valNegInf;
	const char *format = "%lf %lf %lf";

	sprintf(buff, format, INFINITY, INFINITY * -1, NAN);
	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(3, test_vfscanfWrapper(filep, format, &valInf, &valNegInf, &valNan));

	TEST_ASSERT_EQUAL_DOUBLE(INFINITY, valInf);
	TEST_ASSERT_EQUAL_DOUBLE(INFINITY * -1, valNegInf);
	TEST_ASSERT_EQUAL_DOUBLE(NAN, valNan);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(3, fscanf(filep, format, &valInf, &valNegInf, &valNan));

	TEST_ASSERT_EQUAL_DOUBLE(INFINITY, valInf);
	TEST_ASSERT_EQUAL_DOUBLE(INFINITY * -1, valNegInf);
	TEST_ASSERT_EQUAL_DOUBLE(NAN, valNan);

	TEST_ASSERT_EQUAL_INT(3, test_vsscanfWrapper(buff, format, &valInf, &valNegInf, &valNan));

	TEST_ASSERT_EQUAL_DOUBLE(INFINITY, valInf);
	TEST_ASSERT_EQUAL_DOUBLE(INFINITY * -1, valNegInf);
	TEST_ASSERT_EQUAL_DOUBLE(NAN, valNan);

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
	/*
	 * <posix incmpliance> Scanf Inf and Nan handling problem
	 * There is a problem with Scanf where it is unable to read any representation of valInf or valNan.
	 * Issue link: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/634
	 */
#ifdef __phoenix__
	TEST_IGNORE();
#endif

	char buff[BUFF_LEN_FLOAT] = { 0 };
	double valInf, valNan, valNegInf;
	const char *format = "%la %la %la";

	sprintf(buff, format, INFINITY, INFINITY * -1, NAN);
	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(3, test_vfscanfWrapper(filep, format, &valInf, &valNegInf, &valNan));

	TEST_ASSERT_EQUAL_DOUBLE(INFINITY, valInf);
	TEST_ASSERT_EQUAL_DOUBLE(INFINITY * -1, valNegInf);
	TEST_ASSERT_EQUAL_DOUBLE(NAN, valNan);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(3, fscanf(filep, format, &valInf, &valNegInf, &valNan));

	TEST_ASSERT_EQUAL_DOUBLE(INFINITY, valInf);
	TEST_ASSERT_EQUAL_DOUBLE(INFINITY * -1, valNegInf);
	TEST_ASSERT_EQUAL_DOUBLE(NAN, valNan);

	TEST_ASSERT_EQUAL_INT(3, test_vsscanfWrapper(buff, format, &valInf, &valNegInf, &valNan));

	TEST_ASSERT_EQUAL_DOUBLE(INFINITY, valInf);
	TEST_ASSERT_EQUAL_DOUBLE(INFINITY * -1, valNegInf);
	TEST_ASSERT_EQUAL_DOUBLE(NAN, valNan);

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
	/*
	 * <posix incmpliance> Scanf Inf and Nan handling problem
	 * There is a problem with Scanf where it is unable to read any representation of valInf or valNan.
	 * Issue link: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/634
	 */
#ifdef __phoenix__
	TEST_IGNORE();
#endif

	char buff[BUFF_LEN_FLOAT] = { 0 };
	double valInf, valNan, valNegInf;
	const char *format = "%le %le %le";

	sprintf(buff, format, INFINITY, INFINITY * -1, NAN);
	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(3, test_vfscanfWrapper(filep, format, &valInf, &valNegInf, &valNan));

	TEST_ASSERT_EQUAL_DOUBLE(INFINITY, valInf);
	TEST_ASSERT_EQUAL_DOUBLE(INFINITY * -1, valNegInf);
	TEST_ASSERT_EQUAL_DOUBLE(NAN, valNan);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(3, fscanf(filep, format, &valInf, &valNegInf, &valNan));

	TEST_ASSERT_EQUAL_DOUBLE(INFINITY, valInf);
	TEST_ASSERT_EQUAL_DOUBLE(INFINITY * -1, valNegInf);
	TEST_ASSERT_EQUAL_DOUBLE(NAN, valNan);

	TEST_ASSERT_EQUAL_INT(3, test_vsscanfWrapper(buff, format, &valInf, &valNegInf, &valNan));

	TEST_ASSERT_EQUAL_DOUBLE(INFINITY, valInf);
	TEST_ASSERT_EQUAL_DOUBLE(INFINITY * -1, valNegInf);
	TEST_ASSERT_EQUAL_DOUBLE(NAN, valNan);

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
	/*
	 * <posix incmpliance> Scanf Inf and Nan handling problem
	 * There is a problem with Scanf where it is unable to read any representation of valInf or valNan.
	 * Issue link: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/634
	 */
#ifdef __phoenix__
	TEST_IGNORE();
#endif

	char buff[BUFF_LEN_FLOAT] = { 0 };
	double valInf, valNan, valNegInf;
	const char *format = "%lg %lg %lg";

	sprintf(buff, format, INFINITY, INFINITY * -1, NAN);
	fprintf(filep, "%s", buff);
	rewind(filep);

	TEST_ASSERT_EQUAL_INT(3, test_vfscanfWrapper(filep, format, &valInf, &valNegInf, &valNan));

	TEST_ASSERT_EQUAL_DOUBLE(INFINITY, valInf);
	TEST_ASSERT_EQUAL_DOUBLE(INFINITY * -1, valNegInf);
	TEST_ASSERT_EQUAL_DOUBLE(NAN, valNan);

	rewind(filep);

	TEST_ASSERT_EQUAL_INT(3, fscanf(filep, format, &valInf, &valNegInf, &valNan));

	TEST_ASSERT_EQUAL_DOUBLE(INFINITY, valInf);
	TEST_ASSERT_EQUAL_DOUBLE(INFINITY * -1, valNegInf);
	TEST_ASSERT_EQUAL_DOUBLE(NAN, valNan);

	TEST_ASSERT_EQUAL_INT(3, test_vsscanfWrapper(buff, format, &valInf, &valNegInf, &valNan));

	TEST_ASSERT_EQUAL_DOUBLE(INFINITY, valInf);
	TEST_ASSERT_EQUAL_DOUBLE(INFINITY * -1, valNegInf);
	TEST_ASSERT_EQUAL_DOUBLE(NAN, valNan);

	TEST_ASSERT_EQUAL_INT(3, sscanf(buff, format, &valInf, &valNegInf, &valNan));

	TEST_ASSERT_EQUAL_DOUBLE(INFINITY, valInf);
	TEST_ASSERT_EQUAL_DOUBLE(INFINITY * -1, valNegInf);
	TEST_ASSERT_EQUAL_DOUBLE(NAN, valNan);
}


/*
////////////////////////////////////////////////////////////////////////////////////////////////////
*/


TEST_GROUP_RUNNER(stdio_scanf_d)
{
	RUN_TEST_CASE(stdio_scanf_d, d);
	RUN_TEST_CASE(stdio_scanf_d, hhd);
	RUN_TEST_CASE(stdio_scanf_d, hd);
	RUN_TEST_CASE(stdio_scanf_d, ld);
	RUN_TEST_CASE(stdio_scanf_d, lld);
	RUN_TEST_CASE(stdio_scanf_d, jd);
	RUN_TEST_CASE(stdio_scanf_d, zd);
	RUN_TEST_CASE(stdio_scanf_d, td);
	remove(TESTFILE_PATH);
}


TEST_GROUP_RUNNER(stdio_scanf_i)
{
	RUN_TEST_CASE(stdio_scanf_i, i);
	RUN_TEST_CASE(stdio_scanf_i, hhi);
	RUN_TEST_CASE(stdio_scanf_i, hi);
	RUN_TEST_CASE(stdio_scanf_i, li);
	RUN_TEST_CASE(stdio_scanf_i, lli);
	RUN_TEST_CASE(stdio_scanf_i, ji);
	RUN_TEST_CASE(stdio_scanf_i, zi);
	RUN_TEST_CASE(stdio_scanf_i, ti);
	remove(TESTFILE_PATH);
}


TEST_GROUP_RUNNER(stdio_scanf_u)
{
	RUN_TEST_CASE(stdio_scanf_u, u);
	RUN_TEST_CASE(stdio_scanf_u, hhu);
	RUN_TEST_CASE(stdio_scanf_u, hu);
	RUN_TEST_CASE(stdio_scanf_u, lu);
	RUN_TEST_CASE(stdio_scanf_u, llu);
	RUN_TEST_CASE(stdio_scanf_u, ju);
	RUN_TEST_CASE(stdio_scanf_u, zu);
	RUN_TEST_CASE(stdio_scanf_u, tu);
	remove(TESTFILE_PATH);
}


TEST_GROUP_RUNNER(stdio_scanf_o)
{
	RUN_TEST_CASE(stdio_scanf_o, o);
	RUN_TEST_CASE(stdio_scanf_o, hho);
	RUN_TEST_CASE(stdio_scanf_o, ho);
	RUN_TEST_CASE(stdio_scanf_o, lo);
	RUN_TEST_CASE(stdio_scanf_o, llo);
	RUN_TEST_CASE(stdio_scanf_o, jo);
	RUN_TEST_CASE(stdio_scanf_o, zo);
	RUN_TEST_CASE(stdio_scanf_o, to);
	remove(TESTFILE_PATH);
}


TEST_GROUP_RUNNER(stdio_scanf_x)
{
	RUN_TEST_CASE(stdio_scanf_x, x);
	RUN_TEST_CASE(stdio_scanf_x, hhx);
	RUN_TEST_CASE(stdio_scanf_x, hx);
	RUN_TEST_CASE(stdio_scanf_x, lx);
	RUN_TEST_CASE(stdio_scanf_x, llx);
	RUN_TEST_CASE(stdio_scanf_x, jx);
	RUN_TEST_CASE(stdio_scanf_x, zx);
	RUN_TEST_CASE(stdio_scanf_x, tx);
	RUN_TEST_CASE(stdio_scanf_x, X);
	RUN_TEST_CASE(stdio_scanf_x, hhX);
	RUN_TEST_CASE(stdio_scanf_x, hX);
	RUN_TEST_CASE(stdio_scanf_x, lX);
	RUN_TEST_CASE(stdio_scanf_x, llX);
	RUN_TEST_CASE(stdio_scanf_x, jX);
	RUN_TEST_CASE(stdio_scanf_x, zX);
	RUN_TEST_CASE(stdio_scanf_x, tX);
	remove(TESTFILE_PATH);
}


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
