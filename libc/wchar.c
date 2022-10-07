/*
 * Phoenix-RTOS
 *
 * libc-tests
 *
 * Testing wchar.h
 *
 * Copyright 2022 Phoenix Systems
 * Author: Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <wchar.h>

#include <unity_fixture.h>


TEST_GROUP(wchar_wcscmp);


TEST_SETUP(wchar_wcscmp)
{
}


TEST_TEAR_DOWN(wchar_wcscmp)
{
}


TEST(wchar_wcscmp, basic)
{
	int i;
	const wchar_t wstr1[] = L"a";
	const wchar_t wstr2[] = L"b";
	const wchar_t wstr3[] = L"1234567890";
	const wchar_t wstr4[] = L"20000";
	const wchar_t wstr5[] = L"1";
	const wchar_t wstr6[] = L"10";
	wchar_t wstr7[99] = {
		L'\0',
	};
	wchar_t wstr8[99] = {
		L'\0',
	};

	for (i = 0; i < 98; i++) {
		wstr7[i] = L'B';
		wstr8[i] = L'B';
	}
	/* Only last character differs */
	wstr8[i - 1] = L'A';

	TEST_ASSERT_LESS_THAN_INT(0, wcscmp(wstr1, wstr2));
	TEST_ASSERT_GREATER_THAN_INT(0, wcscmp(wstr2, wstr1));
	TEST_ASSERT_EQUAL_INT(0, wcscmp(wstr1, wstr1));
	TEST_ASSERT_EQUAL_INT(0, wcscmp(wstr2, wstr2));

	TEST_ASSERT_LESS_THAN_INT(0, wcscmp(wstr3, wstr4));
	TEST_ASSERT_GREATER_THAN_INT(0, wcscmp(wstr4, wstr3));
	TEST_ASSERT_EQUAL_INT(0, wcscmp(wstr3, wstr3));
	TEST_ASSERT_EQUAL_INT(0, wcscmp(wstr4, wstr4));

	TEST_ASSERT_LESS_THAN_INT(0, wcscmp(wstr5, wstr6));
	TEST_ASSERT_GREATER_THAN_INT(0, wcscmp(wstr6, wstr5));
	TEST_ASSERT_EQUAL_INT(0, wcscmp(wstr5, wstr5));
	TEST_ASSERT_EQUAL_INT(0, wcscmp(wstr6, wstr6));

	TEST_ASSERT_LESS_THAN_INT(0, wcscmp(wstr8, wstr7));
	TEST_ASSERT_GREATER_THAN_INT(0, wcscmp(wstr7, wstr8));
	TEST_ASSERT_EQUAL_INT(0, wcscmp(wstr7, wstr7));
	TEST_ASSERT_EQUAL_INT(0, wcscmp(wstr8, wstr8));
}


TEST(wchar_wcscmp, empty)
{
	const wchar_t wstr1[1] = L"";
	const wchar_t wstr2[2] = L"@";

	TEST_ASSERT_LESS_THAN_INT(0, wcscmp(wstr1, wstr2));
	TEST_ASSERT_GREATER_THAN_INT(0, wcscmp(wstr2, wstr1));
	TEST_ASSERT_EQUAL_INT(0, wcscmp(wstr1, wstr1));
}


TEST(wchar_wcscmp, edge)
{
	/* 16-bit and 32-bit wchar */
	const wchar_t wstr1[2] = { 0x7FFF, L'\0' };
	const wchar_t wstr2[2] = { 0x7FFE, L'\0' };
	const wchar_t wstr3[1] = { L'\0' };
	wchar_t wstr4[2];
	wchar_t wstr5[2];

	TEST_ASSERT_LESS_THAN_INT(0, wcscmp(wstr2, wstr1));
	TEST_ASSERT_GREATER_THAN_INT(0, wcscmp(wstr1, wstr2));
	TEST_ASSERT_EQUAL_INT(0, wcscmp(wstr1, wstr1));
	TEST_ASSERT_LESS_THAN_INT(0, wcscmp(wstr3, wstr1));
	TEST_ASSERT_GREATER_THAN_INT(0, wcscmp(wstr1, wstr3));

	/* 32-bit wchar */
	if (sizeof(wchar_t) >= 4) {
		wstr4[0] = 0x7FFFFFFF;
		wstr4[1] = L'\0';
		wstr5[0] = 0x7FFFFFFE;
		wstr5[1] = L'\0';

		TEST_ASSERT_LESS_THAN_INT(0, wcscmp(wstr5, wstr4));
		TEST_ASSERT_GREATER_THAN_INT(0, wcscmp(wstr4, wstr5));
		TEST_ASSERT_EQUAL_INT(0, wcscmp(wstr4, wstr4));
		TEST_ASSERT_LESS_THAN_INT(0, wcscmp(wstr3, wstr4));
		TEST_ASSERT_GREATER_THAN_INT(0, wcscmp(wstr4, wstr3));
	}
}

TEST_GROUP_RUNNER(wchar_wcscmp)
{
	RUN_TEST_CASE(wchar_wcscmp, basic);
	RUN_TEST_CASE(wchar_wcscmp, empty);
	RUN_TEST_CASE(wchar_wcscmp, edge);
}
