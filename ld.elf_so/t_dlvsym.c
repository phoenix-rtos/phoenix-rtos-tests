/*	$NetBSD: t_dlvsym.c,v 1.1 2011/06/25 05:45:13 nonaka Exp $	*/

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by NONAKA Kimihiro.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>

#include <unity_fixture.h>
#include <stdlib.h>
#include <NetBSD/dlfcn.h>


void *handle;


TEST_GROUP(t_dlvsym);


TEST_SETUP(t_dlvsym)
{
	handle = NULL;
}


TEST_TEAR_DOWN(t_dlvsym)
{
	/* Guarantee dlclose being run at the end of the test. 
	 * each dlclose in test case must always assign corresponding variable to NULL.
	 * each dlopen must assign to one of the variables. */
	if (handle != NULL) {
		(void)dlclose(handle);
	}
}


/* Check dlvsym() function (V_1) */
TEST(t_dlvsym, rtld_dlvsym_v1)
{
	char *error;
	int (*sym)(void);
	int result;

	/* Clear previous error */
	(void) dlerror();

	handle = dlopen("libh_helper_symver_dso.so", RTLD_LAZY);
	error = dlerror();
	TEST_ASSERT(error == NULL);
	TEST_ASSERT(handle != NULL);

	sym = dlvsym(handle, "testfunc", "V_1");
	error = dlerror();
	TEST_ASSERT(error == NULL);

	result = (*sym)();
	TEST_ASSERT(result == 1);

	dlclose(handle);
	handle = NULL;
	error = dlerror();
	TEST_ASSERT(error == NULL);
}


/* Check dlvsym() function (V_3) */
TEST(t_dlvsym, rtld_dlvsym_v3)
{
	char *error;
	int (*sym)(void);
	int result;

	/* Clear previous error */
	(void) dlerror();

	handle = dlopen("libh_helper_symver_dso.so", RTLD_LAZY);
	error = dlerror();
	TEST_ASSERT(error == NULL);
	TEST_ASSERT(handle != NULL);

	sym = dlvsym(handle, "testfunc", "V_3");
	error = dlerror();
	TEST_ASSERT(error == NULL);

	result = (*sym)();
	TEST_ASSERT(result == 3);

	dlclose(handle);
	handle = NULL;
	error = dlerror();
	TEST_ASSERT(error == NULL);
}


/* Check dlvsym() function (symbol is nonexistent) */
TEST(t_dlvsym, rtld_dlvsym_symbol_nonexistent)
{
	char *error;
	int (*sym)(void);

	/* Clear previous error */
	(void) dlerror();

	handle = dlopen("libh_helper_symver_dso.so", RTLD_LAZY);
	error = dlerror();
	TEST_ASSERT(error == NULL);
	TEST_ASSERT(handle != NULL);

	sym = dlvsym(handle, "symbol_nonexistent", "V_3");
	error = dlerror();
	TEST_ASSERT(sym == NULL);
	TEST_ASSERT(error != NULL);

	dlclose(handle);
	handle = NULL;
	error = dlerror();
	TEST_ASSERT(error == NULL);
}


/* Check dlvsym() function (version is nonexistent) */
TEST(t_dlvsym, rtld_dlvsym_version_nonexistent)
{
	char *error;
	int (*sym)(void);

	/* Clear previous error */
	(void) dlerror();

	handle = dlopen("libh_helper_symver_dso.so", RTLD_LAZY);
	error = dlerror();
	TEST_ASSERT(error == NULL);
	TEST_ASSERT(handle != NULL);

	sym = dlvsym(handle, "testfunc", "");
	error = dlerror();
	TEST_ASSERT(sym == NULL);
	TEST_ASSERT(error != NULL);

	dlclose(handle);
	handle = NULL;
	error = dlerror();
	TEST_ASSERT(error == NULL);
}


/* Check dlvsym() function (version is NULL) */
TEST(t_dlvsym, rtld_dlvsym_version_null)
{
	char *error;
	int (*sym)(void);
	int result;

	/* Clear previous error */
	(void) dlerror();

	handle = dlopen("libh_helper_symver_dso.so", RTLD_LAZY);
	error = dlerror();
	TEST_ASSERT(error == NULL);
	TEST_ASSERT(handle != NULL);

	sym = dlvsym(handle, "testfunc", NULL);
	error = dlerror();
	TEST_ASSERT(error == NULL);

	result = (*sym)();
	TEST_ASSERT(result == 3);

	dlclose(handle);
	handle = NULL;
	error = dlerror();
	TEST_ASSERT(error == NULL);
}


TEST_GROUP_RUNNER(t_dlvsym)
{
	RUN_TEST_CASE(t_dlvsym, rtld_dlvsym_v1);
	RUN_TEST_CASE(t_dlvsym, rtld_dlvsym_v3);
	RUN_TEST_CASE(t_dlvsym, rtld_dlvsym_symbol_nonexistent);
	RUN_TEST_CASE(t_dlvsym, rtld_dlvsym_version_nonexistent);
	RUN_TEST_CASE(t_dlvsym, rtld_dlvsym_version_null);
}


void runner(void)
{
	RUN_TEST_GROUP(t_dlvsym);
}


int main(int argc, char **argv)
{
	return UnityMain(argc, (const char **)argv, runner) == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
