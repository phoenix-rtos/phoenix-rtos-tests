/*	$NetBSD: t_ifunc.c,v 1.13 2022/06/21 16:24:37 christos Exp $	*/

/*
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND
 * CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>

#include <unity_fixture.h>
#include <string.h>
#include <errno.h>
#include <NetBSD/dlfcn.h>
#include <stdlib.h>
#include <unistd.h>

#include <NetBSD/cdefs.h>

#include "libexecassert/execassert.h"


#ifndef _RTLD_TEST_SRCDIR
#error "_RTLD_TEST_SRCDIR" not defined!
#endif

/* Phoenix doesn't have util.h header. */
#define easprintf(str, fmt, ...) TEST_ASSERT(asprintf(str, fmt, __VA_ARGS__) != -1)

#if (defined(__aarch64__) || \
	defined(__arm__) || \
	defined(__i386__) || \
	defined(__powerpc__) || \
	defined(__sparc__) || \
	defined(__x86_64__)) && !defined(NOMMU)
#define LINKER_SUPPORT 1
#else
#define LINKER_SUPPORT 0
#endif


void *handle;


TEST_GROUP(t_ifunc);


TEST_SETUP(t_ifunc)
{
	handle = NULL;
}


TEST_TEAR_DOWN(t_ifunc)
{
	if (handle != NULL) {
		(void)dlclose(handle);
	}
}


/* ifunc functions are resolved */
TEST(t_ifunc, rtld_ifunc)
{
	const char *envstr[] = {
		"0", "1"
	};
	long long expected_result[] = {
		0xdeadbeefll, 0xbeefdeadll
	};
	long long (*sym)(void);
	long long result;
	const char *error;
	size_t i;

	if (!LINKER_SUPPORT)
		TEST_IGNORE_MESSAGE("Missing linker support for ifunc relocations");

	for (i = 0; i < __arraycount(envstr); ++i) {
		setenv("USE_IFUNC2", envstr[i], 1);

		handle = dlopen("libh_helper_ifunc_dso.so", RTLD_LAZY);
		error = dlerror();
		TEST_ASSERT(error == NULL);
		TEST_ASSERT(handle != NULL);

		sym = dlsym(handle, "ifunc");
		error = dlerror();
		TEST_ASSERT(error == NULL);
		TEST_ASSERT(sym != NULL);

		result = (*sym)();
		TEST_ASSERT(result == expected_result[i]);

		dlclose(handle);
		/* Mark as NULL to avoid closing again in TEARDOWN. */
		handle = NULL;
		error = dlerror();
		TEST_ASSERT(error == NULL);

		/* clang-format off */
		char *command = _RTLD_TEST_SRCDIR "/" "h_ifunc";
		/* clang-format on */
		char exp_res[32];
		(void)snprintf(exp_res, sizeof(exp_res), "%lld", expected_result[i]);
		char *const argv[] = { command, exp_res, NULL };
		int retCode = EXIT_SUCCESS;
		execAssert_execve(command, argv, environ, &retCode, NULL, NULL);
	}
}

/* hidden ifunc functions are resolved */
TEST(t_ifunc, rtld_hidden_ifunc)
{
	const char *envstr[] = {
		"0", "1"
	};
	long long expected_result[] = {
		0xdeadbeefll, 0xbeefdeadll
	};
	long long (*sym)(void);
	long long (*(*sym2)(void))(void);
	long long result;
	const char *error;
	size_t i;

	if (!LINKER_SUPPORT)
		TEST_IGNORE_MESSAGE("Missing linker support for ifunc relocations");

	for (i = 0; i < __arraycount(envstr); ++i) {
		setenv("USE_IFUNC2", envstr[i], 1);

		handle = dlopen("libh_helper_ifunc_dso.so", RTLD_LAZY);
		error = dlerror();
		TEST_ASSERT(error == NULL);
		TEST_ASSERT(handle != NULL);

		sym = dlsym(handle, "ifunc_plt");
		error = dlerror();
		TEST_ASSERT(error == NULL);
		TEST_ASSERT(sym != NULL);

		result = (*sym)();
		TEST_ASSERT(result == expected_result[!i]);

		sym2 = dlsym(handle, "ifunc_indirect");
		error = dlerror();
		TEST_ASSERT(error == NULL);
		TEST_ASSERT(sym2 != NULL);

		sym = (*sym2)();
		result = (*sym)();
		TEST_ASSERT(result == expected_result[!i]);

		dlclose(handle);
		/* Mark as NULL to avoid closing again in TEARDOWN. */
		handle = NULL;
		error = dlerror();
		TEST_ASSERT(error == NULL);

		/* clang-format off */
		char *command = _RTLD_TEST_SRCDIR "/" "h_ifunc";
		/* clang-format on */
		char exp_res[32];
		(void)snprintf(exp_res, sizeof(exp_res), "%lld", expected_result[i]);
		char *const argv[] = { command, exp_res, NULL };
		int retCode = EXIT_SUCCESS;
		execAssert_execve(command, argv, environ, &retCode, NULL, NULL);
	}
}

#if LINKER_SUPPORT
static long long
ifunc_helper(void)
{
	return 0xdeadbeefll;
}

static __attribute__((used)) long long (*resolve_ifunc(void))(void)
{
	return ifunc_helper;
}
__hidden_ifunc(ifunc, resolve_ifunc);
#endif
long long ifunc(void);

/* ifunc functions are resolved in the executable */
TEST(t_ifunc, rtld_main_ifunc)
{
	if (!LINKER_SUPPORT)
		TEST_IGNORE_MESSAGE("Missing linker support for ifunc relocations");
	TEST_ASSERT(ifunc() == 0xdeadbeefll);
}


TEST_GROUP_RUNNER(t_ifunc)
{
	RUN_TEST_CASE(t_ifunc, rtld_main_ifunc);
	RUN_TEST_CASE(t_ifunc, rtld_hidden_ifunc);
	RUN_TEST_CASE(t_ifunc, rtld_ifunc);
}


void runner(void)
{
	RUN_TEST_GROUP(t_ifunc);
}


int main(int argc, char **argv)
{
	return UnityMain(argc, (const char **)argv, runner) == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
