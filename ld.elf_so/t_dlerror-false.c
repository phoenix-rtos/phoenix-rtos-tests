/*	$NetBSD: t_dlerror-false.c,v 1.3 2022/01/14 07:34:07 skrll Exp $	*/

/*
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
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
#include <stdlib.h>
#include <NetBSD/dlfcn.h>


void *handle;


TEST_GROUP(t_dlerror_false);


TEST_SETUP(t_dlerror_false)
{
	handle = NULL;
}


TEST_TEAR_DOWN(t_dlerror_false)
{
	/* Guarantee dlclose being run at the end of the test. 
	 * each dlclose in test case must always assign corresponding variable to NULL.
	 * each dlopen must assign to one of the variables. */
	if (handle != NULL) {
		(void)dlclose(handle);
	}
}


TEST(t_dlerror_false, rtld_dlerror_false)
{
	void *sym;
	char *error;

	/*
	 *
	 * Test for dlerror() being set by a successful library open.
	 * Requires that the rpath be set to something that does not
	 * include libm.so.
	 */

	handle = dlopen("libphoenix.so.3", RTLD_LAZY);
	error = dlerror();
	TEST_ASSERT(error == NULL);
	TEST_ASSERT(handle != NULL);

	sym = dlsym(handle, "sin");
	error = dlerror();
	TEST_ASSERT(sym != NULL);
	TEST_ASSERT(error == NULL);

	dlclose(handle);
	/* Mark as NULL to avoid closing again in TEARDOWN. */
	handle = NULL;
	error = dlerror();

	TEST_ASSERT(error == NULL);
}


TEST_GROUP_RUNNER(t_dlerror_false)
{
	RUN_TEST_CASE(t_dlerror_false, rtld_dlerror_false);
}


void runner(void)
{
	RUN_TEST_GROUP(t_dlerror_false);
}


int main(int argc, char **argv)
{
	return UnityMain(argc, (const char **)argv, runner) == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}