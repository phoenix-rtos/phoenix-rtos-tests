/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <libgen.h>
 * TESTED:
 *    - dirname()
 *    - basename()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Lukasz Kruszynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>
#include <libgen.h>

#include <unity_fixture.h>

#define LIBGEN_BUFSZ 64
#define NULL_LABEL   "(null)"


/*
 * Both functions may modify their argument and may return internal storage, so
 * every case is exercised with a fresh writable copy of the input path.
 */
static void test_checkDirname(const char *input, const char *expected)
{
	char buf[LIBGEN_BUFSZ];
	char *arg;

	if (input == NULL) {
		arg = NULL;
	}
	else {
		strncpy(buf, input, sizeof(buf) - 1);
		buf[sizeof(buf) - 1] = '\0';
		arg = buf;
	}

	TEST_ASSERT_EQUAL_STRING_MESSAGE(expected, dirname(arg), (input != NULL) ? input : NULL_LABEL);
}


static void test_checkBasename(const char *input, const char *expected)
{
	char buf[LIBGEN_BUFSZ];
	char *arg;

	if (input == NULL) {
		arg = NULL;
	}
	else {
		strncpy(buf, input, sizeof(buf) - 1);
		buf[sizeof(buf) - 1] = '\0';
		arg = buf;
	}

	TEST_ASSERT_EQUAL_STRING_MESSAGE(expected, basename(arg), (input != NULL) ? input : NULL_LABEL);
}


TEST_GROUP(libgen_dirname);


TEST_SETUP(libgen_dirname)
{
}


TEST_TEAR_DOWN(libgen_dirname)
{
}


/* dirname() shall return the parent-directory portion of a pathname. */
TEST(libgen_dirname, returns_parent_component)
{
	size_t i;
	static const struct {
		const char *in;
		const char *out;
	} cases[] = {
		{ "/usr/lib", "/usr" },
		{ "a/b/c", "a/b" },
		{ "/a", "/" },
		/* trailing non-leading slashes are not counted as part of the path */
		{ "/usr/", "/" },
		{ "usr/lib/", "usr" },
	};

	for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
		test_checkDirname(cases[i].in, cases[i].out);
	}
}


/* dirname() shall return "." when the path has no '/' or is null/empty. */
TEST(libgen_dirname, degenerate_paths_return_dot_or_root)
{
	size_t i;
	static const struct {
		const char *in;
		const char *out;
	} cases[] = {
		{ "usr", "." },
		{ ".", "." },
		{ "..", "." },
		{ "", "." },
		{ NULL, "." },
		{ "/", "/" },
	};

	for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
		test_checkDirname(cases[i].in, cases[i].out);
	}
}


TEST_GROUP(libgen_basename);


TEST_SETUP(libgen_basename)
{
}


TEST_TEAR_DOWN(libgen_basename)
{
}


/* basename() shall return the final component, with trailing slashes removed. */
TEST(libgen_basename, returns_final_component)
{
	size_t i;
	static const struct {
		const char *in;
		const char *out;
	} cases[] = {
		{ "/usr/lib", "lib" },
		{ "a/b/c", "c" },
		{ "usr", "usr" },
		{ "/usr/", "usr" },
		{ "/usr/lib/", "lib" },
	};

	for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
		test_checkBasename(cases[i].in, cases[i].out);
	}
}


/* basename() shall return "/", ".", "..", or "." for the degenerate paths. */
TEST(libgen_basename, degenerate_paths)
{
	size_t i;
	static const struct {
		const char *in;
		const char *out;
	} cases[] = {
		{ "/", "/" },
		{ ".", "." },
		{ "..", ".." },
		{ "", "." },
		{ NULL, "." },
	};

	for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
		test_checkBasename(cases[i].in, cases[i].out);
	}
}


TEST_GROUP_RUNNER(libgen_dirname)
{
	RUN_TEST_CASE(libgen_dirname, returns_parent_component);
	RUN_TEST_CASE(libgen_dirname, degenerate_paths_return_dot_or_root);
}


TEST_GROUP_RUNNER(libgen_basename)
{
	RUN_TEST_CASE(libgen_basename, returns_final_component);
	RUN_TEST_CASE(libgen_basename, degenerate_paths);
}
