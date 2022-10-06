/*
 * Phoenix-RTOS
 *
 * libc-tests
 *
 * Testing POSIX compatibility of getopt() unistd.h function
 *
 * Copyright 2021 Phoenix Systems
 * Author: Mateusz Niewiadomski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>

#include <unity_fixture.h>

#define NOERR       0
#define OPTREQ      1
#define UNKNOWNOPT  2
#define UNKNOWNCHAR 4
#define NOARG       8
#define ABORT       16


struct getopt_t {
	int aflag;
	int bflag;
	char *cvalue;
	int nonopts;

	unsigned int err;
};


/* updates `results` structure according to getopt() operations */
static int testmain(int argc, char *const argv[], struct getopt_t *results, const char *optstring)
{
	int c;
	int arg;

	while ((c = getopt(argc, argv, optstring)) != -1)
		switch (c) {
			case 'a':
				results->aflag++;
				break;
			case 'b':
				results->bflag++;
				break;
			case 'c':
				results->cvalue = optarg;
				break;
			case '?':
				if (optopt == 'c')
					results->err |= OPTREQ;
				else if (isprint(optopt))
					results->err |= UNKNOWNOPT;
				else
					results->err |= UNKNOWNCHAR;
				break;
			case ':':
				results->err |= NOARG;
				break;
			default:
				results->err |= ABORT;
		}

	for (arg = optind; arg < argc; arg++)
		results->nonopts++;

	return 0;
}


TEST_GROUP(unistd_getopt);

TEST_SETUP(unistd_getopt)
{
	/* reset of getopt() index value optind */
	optind = 0;
}


TEST_TEAR_DOWN(unistd_getopt)
{
}

TEST(unistd_getopt, getopt_zeroargs)
{
	/* mocks of main() arguments */
	char *const testargv[] = { "cmd", NULL };
	const int testargc = sizeof(testargv) / sizeof(testargv[0]) - 1;
	struct getopt_t ret = { 0 };

	testmain(testargc, testargv, &ret, "abc:");

	TEST_ASSERT_EQUAL_INT(NOERR, ret.err);
	TEST_ASSERT_EQUAL_INT(0, ret.aflag);
	TEST_ASSERT_EQUAL_INT(0, ret.bflag);
	TEST_ASSERT_NULL(ret.cvalue);
	TEST_ASSERT_EQUAL_INT(0, ret.nonopts);
}


TEST(unistd_getopt, getopt_normal_flags)
{
	/* mocks of main() arguments */
	char *testargv[] = { "cmd", "-a", "-b", NULL };
	const int testargc = sizeof(testargv) / sizeof(testargv[0]) - 1;
	struct getopt_t ret = { 0 };

	testmain(testargc, testargv, &ret, "abc:");

	TEST_ASSERT_EQUAL_INT(NOERR, ret.err);
	TEST_ASSERT_EQUAL_INT(1, ret.aflag);
	TEST_ASSERT_EQUAL_INT(1, ret.bflag);
	TEST_ASSERT_NULL(ret.cvalue);
	TEST_ASSERT_EQUAL_INT(0, ret.nonopts);
}


TEST(unistd_getopt, getopt_joined_flags)
{
	/* mocks of main() arguments */
	char *testargv[] = { "cmd", "-ab", NULL };
	const int testargc = sizeof(testargv) / sizeof(testargv[0]) - 1;
	struct getopt_t ret = { 0 };

	testmain(testargc, testargv, &ret, "abc:");

	TEST_ASSERT_EQUAL_INT(NOERR, ret.err);
	TEST_ASSERT_EQUAL_INT(1, ret.aflag);
	TEST_ASSERT_EQUAL_INT(1, ret.bflag);
	TEST_ASSERT_NULL(ret.cvalue);
	TEST_ASSERT_EQUAL_INT(0, ret.nonopts);
}


TEST(unistd_getopt, getopt_normal_parameter)
{
	/* mocks of main() arguments */
	char *testargv[] = { "cmd", "-c", "foo", NULL };
	const int testargc = sizeof(testargv) / sizeof(testargv[0]) - 1;
	struct getopt_t ret = { 0 };

	testmain(testargc, testargv, &ret, "abc:");

	TEST_ASSERT_EQUAL_INT(NOERR, ret.err);
	TEST_ASSERT_EQUAL_INT(0, ret.aflag);
	TEST_ASSERT_EQUAL_INT(0, ret.bflag);
	TEST_ASSERT_EQUAL_STRING("foo", ret.cvalue);
	TEST_ASSERT_EQUAL_INT(0, ret.nonopts);
}

TEST(unistd_getopt, getopt_normal_optparameter)
{
	/* mocks of main() arguments */
	char *testargv[] = { "cmd", "-c", "-a", "-b", NULL };
	const int testargc = sizeof(testargv) / sizeof(testargv[0]) - 1;
	struct getopt_t ret = { 0 };

	testmain(testargc, testargv, &ret, "abc:");

	TEST_ASSERT_EQUAL_INT(NOERR, ret.err);
	TEST_ASSERT_EQUAL_INT(0, ret.aflag);
	TEST_ASSERT_EQUAL_INT(1, ret.bflag);
	TEST_ASSERT_EQUAL_STRING("-a", ret.cvalue);
	TEST_ASSERT_EQUAL_INT(0, ret.nonopts);
}


TEST(unistd_getopt, getopt_joined_parameter)
{
	/* mocks of main() arguments */
	char *testargv[] = { "cmd", "-cfoo", NULL };
	const int testargc = sizeof(testargv) / sizeof(testargv[0]) - 1;
	struct getopt_t ret = { 0 };

	testmain(testargc, testargv, &ret, "abc:");

	TEST_ASSERT_EQUAL_INT(NOERR, ret.err);
	TEST_ASSERT_EQUAL_INT(0, ret.aflag);
	TEST_ASSERT_EQUAL_INT(0, ret.bflag);
	TEST_ASSERT_EQUAL_STRING("foo", ret.cvalue);
	TEST_ASSERT_EQUAL_INT(0, ret.nonopts);
}


TEST(unistd_getopt, getopt_nonopt)
{
	/* mocks of main() arguments */
	char *testargv[] = { "cmd", "arg1", NULL };
	const int testargc = sizeof(testargv) / sizeof(testargv[0]) - 1;
	struct getopt_t ret = { 0 };

	testmain(testargc, testargv, &ret, "abc:");

	TEST_ASSERT_EQUAL_INT(NOERR, ret.err);
	TEST_ASSERT_EQUAL_INT(0, ret.aflag);
	TEST_ASSERT_EQUAL_INT(0, ret.bflag);
	TEST_ASSERT_NULL(ret.cvalue);
	TEST_ASSERT_EQUAL_INT(1, ret.nonopts);
}


TEST(unistd_getopt, getopt_parameter_nonopt)
{
	/* mocks of main() arguments */
	char *testargv[] = { "cmd", "-c", "foo", "arg", NULL };
	const int testargc = sizeof(testargv) / sizeof(testargv[0]) - 1;
	struct getopt_t ret = { 0 };

	testmain(testargc, testargv, &ret, "abc:");
	TEST_ASSERT_EQUAL_INT(NOERR, ret.err);
	TEST_ASSERT_EQUAL_INT(0, ret.aflag);
	TEST_ASSERT_EQUAL_INT(0, ret.bflag);
	TEST_ASSERT_EQUAL_STRING("foo", ret.cvalue);
	TEST_ASSERT_EQUAL_INT(1, ret.nonopts);
}


TEST(unistd_getopt, getopt_endofargs_doubledash)
{
	/* mocks of main() arguments */
	char *testargv[] = { "cmd", "-a", "--", "-b", NULL };
	const int testargc = sizeof(testargv) / sizeof(testargv[0]) - 1;
	struct getopt_t ret = { 0 };

	testmain(testargc, testargv, &ret, "abc:");
	TEST_ASSERT_EQUAL_INT(NOERR, ret.err);
	TEST_ASSERT_EQUAL_INT(1, ret.aflag);
	TEST_ASSERT_EQUAL_INT(0, ret.bflag);
	TEST_ASSERT_NULL(ret.cvalue);
	TEST_ASSERT_EQUAL_INT(1, ret.nonopts);
}


TEST(unistd_getopt, getopt_endofargs_singledash)
{
	/* mocks of main() arguments */
	char *testargv[] = { "cmd", "-a", "-", "-b", NULL };
	const int testargc = sizeof(testargv) / sizeof(testargv[0]) - 1;
	struct getopt_t ret = { 0 };

	testmain(testargc, testargv, &ret, "abc:");
	TEST_ASSERT_EQUAL_INT(NOERR, ret.err);
	TEST_ASSERT_EQUAL_INT(1, ret.aflag);
	TEST_ASSERT_EQUAL_INT(0, ret.bflag);
	TEST_ASSERT_NULL(ret.cvalue);
	TEST_ASSERT_EQUAL_INT(2, ret.nonopts);
}

TEST(unistd_getopt, getopt_unknownopt)
{
	/* mocks of main() arguments */
	char *testargv[] = { "cmd", "-axb", "-c", "--", "arg1", "arg2", NULL };
	const int testargc = sizeof(testargv) / sizeof(testargv[0]) - 1;
	struct getopt_t ret = { 0 };

	testmain(testargc, testargv, &ret, "abc:");
	TEST_ASSERT_EQUAL_INT(UNKNOWNOPT, ret.err);
	TEST_ASSERT_EQUAL_INT(1, ret.aflag);
	TEST_ASSERT_EQUAL_INT(1, ret.bflag);
	TEST_ASSERT_EQUAL_STRING("--", ret.cvalue);
	TEST_ASSERT_EQUAL_INT(2, ret.nonopts);
}


TEST(unistd_getopt, getopt_unknownopt_optreq)
{
	/* mocks of main() arguments */
	char *testargv[] = { "cmd", "-axb", "-c", NULL };
	const int testargc = sizeof(testargv) / sizeof(testargv[0]) - 1;
	struct getopt_t ret = { 0 };

	testmain(testargc, testargv, &ret, "abc:");
	TEST_ASSERT_EQUAL_INT(UNKNOWNOPT | OPTREQ, ret.err);
	TEST_ASSERT_EQUAL_INT(1, ret.aflag);
	TEST_ASSERT_EQUAL_INT(1, ret.bflag);
	TEST_ASSERT_NULL(ret.cvalue);
	TEST_ASSERT_EQUAL_INT(0, ret.nonopts);
}

TEST(unistd_getopt, getopt_noarg)
{
	/* mocks of main() arguments */
	char *testargv[] = { "cmd", "-ab", "-c", NULL };
	const int testargc = sizeof(testargv) / sizeof(testargv[0]) - 1;
	struct getopt_t ret = { 0 };

	testmain(testargc, testargv, &ret, ":abc:");
	TEST_ASSERT_EQUAL_INT(NOARG, ret.err);
	TEST_ASSERT_EQUAL_INT(1, ret.aflag);
	TEST_ASSERT_EQUAL_INT(1, ret.bflag);
	TEST_ASSERT_NULL(ret.cvalue);
	TEST_ASSERT_EQUAL_INT(0, ret.nonopts);
}

TEST(unistd_getopt, getopt_unknownopt_noarg)
{
	/* mocks of main() arguments */
	char *testargv[] = { "cmd", "-axb", "-c", NULL };
	const int testargc = sizeof(testargv) / sizeof(testargv[0]) - 1;
	struct getopt_t ret = { 0 };

	testmain(testargc, testargv, &ret, ":abc:");
	TEST_ASSERT_EQUAL_INT(UNKNOWNOPT | NOARG, ret.err);
	TEST_ASSERT_EQUAL_INT(1, ret.aflag);
	TEST_ASSERT_EQUAL_INT(1, ret.bflag);
	TEST_ASSERT_NULL(ret.cvalue);
	TEST_ASSERT_EQUAL_INT(0, ret.nonopts);
}


TEST(unistd_getopt, getopt_unknownopt_multiple)
{
	/* mocks of main() arguments */
	char *testargv1[] = { "cmd", "-xxx", NULL };
	char *testargv2[] = { "cmd", "-ab", "-c", "value", NULL };
	int testargc1 = 2, testargc2 = 4;
	struct getopt_t ret = { 0 };

	testmain(testargc1, testargv1, &ret, "abc:");
	TEST_ASSERT_EQUAL_INT(UNKNOWNOPT, ret.err);
	TEST_ASSERT_EQUAL_INT(0, ret.aflag);
	TEST_ASSERT_EQUAL_INT(0, ret.bflag);
	TEST_ASSERT_NULL(ret.cvalue);
	TEST_ASSERT_EQUAL_INT(0, ret.nonopts);

	/* Assert that parsing argument works correctly after passing multiple unknown options
	   There was issue related to this case */

	/* reset getopt() index value optind and ret structure */
	optind = 0;
	memset(&ret, 0, sizeof(ret));

	testmain(testargc2, testargv2, &ret, "abc:");
	TEST_ASSERT_EQUAL_INT(1, ret.aflag);
	TEST_ASSERT_EQUAL_INT(1, ret.bflag);
	TEST_ASSERT_EQUAL_STRING("value", ret.cvalue);
	TEST_ASSERT_EQUAL_INT(0, ret.nonopts);
}


TEST(unistd_getopt, getopt_doubledash_simple)
{
	/* mocks of main() arguments */
	char *testargv[] = { "cmd", "-a", "--", "-b", NULL };
	const int testargc = sizeof(testargv) / sizeof(testargv[0]) - 1;
	struct getopt_t ret = { 0 };

	testmain(testargc, testargv, &ret, "ab");
	TEST_ASSERT_EQUAL_INT(0, ret.err);
	TEST_ASSERT_EQUAL_INT(1, ret.aflag);
	TEST_ASSERT_EQUAL_INT(0, ret.bflag);
	TEST_ASSERT_NULL(ret.cvalue);
	TEST_ASSERT_EQUAL_INT(1, ret.nonopts);
}


TEST(unistd_getopt, getopt_doubledash_one)
{
	/* mocks of main() arguments */
	char *testargv[] = { "cmd", "--x", "-ab", "nonopt", NULL };
	const int testargc = sizeof(testargv) / sizeof(testargv[0]) - 1;
	struct getopt_t ret = { 0 };

	testmain(testargc, testargv, &ret, "ab");
	TEST_ASSERT_EQUAL_INT(UNKNOWNOPT, ret.err);
	TEST_ASSERT_EQUAL_INT(1, ret.aflag);
	TEST_ASSERT_EQUAL_INT(1, ret.bflag);
	TEST_ASSERT_NULL(ret.cvalue);
	TEST_ASSERT_EQUAL_INT(1, ret.nonopts);
}


TEST(unistd_getopt, getopt_doubledash_multi)
{
	/* mocks of main() arguments */
	char *testargv[] = { "cmd", "--x!?<>;gfngfna", "-b", NULL };
	const int testargc = sizeof(testargv) / sizeof(testargv[0]) - 1;
	struct getopt_t ret = { 0 };

	testmain(testargc, testargv, &ret, "ab");
	TEST_ASSERT_EQUAL_INT(UNKNOWNOPT, ret.err);
	TEST_ASSERT_EQUAL_INT(1, ret.aflag);
	TEST_ASSERT_EQUAL_INT(1, ret.bflag);
	TEST_ASSERT_NULL(ret.cvalue);
	TEST_ASSERT_EQUAL_INT(0, ret.nonopts);
}


TEST(unistd_getopt, getopt_tripledash)
{
	/* mocks of main() arguments */
	char *testargv[] = { "cmd", "-a", "---", "-b", NULL };
	const int testargc = sizeof(testargv) / sizeof(testargv[0]) - 1;
	struct getopt_t ret = { 0 };

	testmain(testargc, testargv, &ret, "ab");
	TEST_ASSERT_EQUAL_INT(UNKNOWNOPT, ret.err);
	TEST_ASSERT_EQUAL_INT(1, ret.aflag);
	TEST_ASSERT_EQUAL_INT(1, ret.bflag);
	TEST_ASSERT_NULL(ret.cvalue);
	TEST_ASSERT_EQUAL_INT(0, ret.nonopts);
}


TEST_GROUP_RUNNER(unistd_getopt)
{
	RUN_TEST_CASE(unistd_getopt, getopt_zeroargs);
	RUN_TEST_CASE(unistd_getopt, getopt_normal_flags);
	RUN_TEST_CASE(unistd_getopt, getopt_joined_flags);

	RUN_TEST_CASE(unistd_getopt, getopt_normal_parameter);
	RUN_TEST_CASE(unistd_getopt, getopt_normal_optparameter);
	RUN_TEST_CASE(unistd_getopt, getopt_joined_parameter);

	RUN_TEST_CASE(unistd_getopt, getopt_nonopt);
	RUN_TEST_CASE(unistd_getopt, getopt_parameter_nonopt);
	RUN_TEST_CASE(unistd_getopt, getopt_endofargs_singledash);
	RUN_TEST_CASE(unistd_getopt, getopt_endofargs_doubledash);
	RUN_TEST_CASE(unistd_getopt, getopt_unknownopt_optreq);
	RUN_TEST_CASE(unistd_getopt, getopt_unknownopt);
	RUN_TEST_CASE(unistd_getopt, getopt_unknownopt_multiple);

	RUN_TEST_CASE(unistd_getopt, getopt_noarg);
	RUN_TEST_CASE(unistd_getopt, getopt_unknownopt_noarg);

	RUN_TEST_CASE(unistd_getopt, getopt_doubledash_simple);
	RUN_TEST_CASE(unistd_getopt, getopt_doubledash_one);
	RUN_TEST_CASE(unistd_getopt, getopt_doubledash_multi);

	RUN_TEST_CASE(unistd_getopt, getopt_tripledash);
}
