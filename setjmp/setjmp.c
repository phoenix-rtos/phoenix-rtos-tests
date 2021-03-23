#include <setjmp.h>
#include <signal.h>
#include <stdio.h>

#include "unity_fixture.h"

TEST_GROUP(test_setjmp);

TEST_SETUP(test_setjmp)
{
}

TEST_TEAR_DOWN(test_setjmp)
{
}

TEST(test_setjmp, setjmp)
{
	int res;
	jmp_buf jb;

	if ((res = setjmp(jb)) == 0) {
		longjmp(jb, 1996);
		FAIL("unreachable code execution");
	}
	else {
		TEST_ASSERT_EQUAL_INT(1996, res);
	}
}

TEST(test_setjmp, _setjmp)
{
	int res;
	jmp_buf jb;

	if ((res = _setjmp(jb)) == 0) {
		longjmp(jb, 0xDA);
		FAIL("unreachable code execution");
	}
	else {
		TEST_ASSERT_EQUAL_INT(0xDA, res);
	}
}

TEST(test_setjmp, sigsetjmp)
{
	int res;
	sigjmp_buf jb;

	if ((res = sigsetjmp(jb, 0)) == 0) {
		siglongjmp(jb, 24);
		FAIL("unreachable code execution");
	}
	else {
		TEST_ASSERT_EQUAL_INT(24, res);
	}
}

TEST(test_setjmp, sigsetjmp_savesigs_0)
{
	int res;
	jmp_buf jb;
	sigset_t old, new, act;

	TEST_ASSERT_EQUAL_INT(0, sigprocmask(SIG_SETMASK, 0, &old));

	TEST_ASSERT_EQUAL_INT(0, sigemptyset(&new));
	TEST_ASSERT_EQUAL_INT(0, sigaddset(&new, SIGUSR1));

	if ((res = sigsetjmp(jb, 0)) == 0) {
		TEST_ASSERT_EQUAL_INT(0, sigprocmask(SIG_BLOCK, &new, NULL));
		siglongjmp(jb, 45);
		FAIL("unreachable code execution");
	}
	else {
		TEST_ASSERT_EQUAL_INT(res, 45);
		TEST_ASSERT_EQUAL_INT(0, sigprocmask(SIG_SETMASK, 0, &act));
		TEST_ASSERT_EQUAL_MEMORY(&new, &act, sizeof(sigset_t));

		// Restore old mask
		TEST_ASSERT_EQUAL_INT(0, sigprocmask(SIG_UNBLOCK, &new, NULL));
	}
}

TEST(test_setjmp, sigsetjmp_savesigs_1)
{
	int res;
	jmp_buf jb;
	sigset_t old, new, act;

	TEST_ASSERT_EQUAL_INT(0, sigprocmask(SIG_SETMASK, 0, &old));

	TEST_ASSERT_EQUAL_INT(0, sigemptyset(&new));
	TEST_ASSERT_EQUAL_INT(0, sigaddset(&new, SIGUSR1));

	if ((res = sigsetjmp(jb, 1)) == 0) {
		TEST_ASSERT_EQUAL_INT(0, sigprocmask(SIG_BLOCK, &new, NULL));
		siglongjmp(jb, 44);
		FAIL("unreachable code execution");
	}
	else {
		TEST_ASSERT_EQUAL_INT(res, 44);
		TEST_ASSERT_EQUAL_INT(0, sigprocmask(SIG_SETMASK, 0, &act));
		TEST_ASSERT_EQUAL_MEMORY(&old, &act, sizeof(sigset_t));

		// Restore old mask, just in case assertion failed
		TEST_ASSERT_EQUAL_INT(0, sigprocmask(SIG_UNBLOCK, &new, NULL));
	}
}

__attribute__((noinline)) void func_longjmp(jmp_buf jb, int res)
{
	longjmp(jb, res);
}

TEST(test_setjmp, setjmp_out_of_frame)
{
	int res;
	jmp_buf jb;

	if ((res = setjmp(jb)) == 0) {
		func_longjmp(jb, 0x333);
		FAIL("unreachable code execution");
	}
	else {
		TEST_ASSERT_EQUAL_INT(0x333, res);
	}
}

__attribute__((noinline)) void func__longjmp(jmp_buf jb, int res)
{
	_longjmp(jb, res);
}

TEST(test_setjmp, _setjmp_out_of_frame)
{
	int res;
	jmp_buf jb;

	if ((res = _setjmp(jb)) == 0) {
		func__longjmp(jb, 0x414243);
		FAIL("unreachable code execution");
	}
	else {
		TEST_ASSERT_EQUAL_INT(0x414243, res);
	}
}

__attribute__((noinline)) void func_siglongjmp(jmp_buf jb, int res)
{
	siglongjmp(jb, res);
}

TEST(test_setjmp, sigsetjmp_out_of_frame)
{
	int res;
	jmp_buf jb;

	if ((res = sigsetjmp(jb, 0)) == 0) {
		func_siglongjmp(jb, 2021);
		FAIL("unreachable code execution");
	}
	else {
		TEST_ASSERT_EQUAL_INT(2021, res);
	}
}

TEST_GROUP_RUNNER(test_setjmp)
{
	RUN_TEST_CASE(test_setjmp, setjmp);
	RUN_TEST_CASE(test_setjmp, _setjmp);
	RUN_TEST_CASE(test_setjmp, sigsetjmp);
	RUN_TEST_CASE(test_setjmp, sigsetjmp_savesigs_0);
	RUN_TEST_CASE(test_setjmp, sigsetjmp_savesigs_1);
	RUN_TEST_CASE(test_setjmp, setjmp_out_of_frame);
	RUN_TEST_CASE(test_setjmp, _setjmp_out_of_frame);
	RUN_TEST_CASE(test_setjmp, sigsetjmp_out_of_frame);
}

void runner(void)
{
	RUN_TEST_GROUP(test_setjmp);
}

int main(int argc, char *argv[])
{
	UnityMain(argc, (const char **)argv, runner);
	return 0;
}
