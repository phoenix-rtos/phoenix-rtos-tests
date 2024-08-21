/*	$NetBSD: t_dlerror-cleared.c,v 1.3 2019/07/09 16:24:01 maya Exp $	*/

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
#include <NetBSD/dlfcn.h>
#include <stdlib.h>


void *handle;


TEST_GROUP(t_dlerror_cleared);


TEST_SETUP(t_dlerror_cleared)
{
	handle = NULL;
}


TEST_TEAR_DOWN(t_dlerror_cleared)
{
	/* Guarantee dlclose being run at the end of the test. 
	 * each dlclose in test case must always assign corresponding variable to NULL.
	 * each dlopen must assign to one of the variables. */
	if (handle != NULL) {
		(void)dlclose(handle);
	}
}


TEST(t_dlerror_cleared, rtld_dlerror_cleared)
{
	char *error;

	/*
	 * Test that an error set by dlopen() persists past a successful
	 * dlopen() call.
	 */
	handle = dlopen("libnonexistent.so", RTLD_LAZY);
	TEST_ASSERT(handle == NULL);
	handle = dlopen("libm.so", RTLD_NOW);
	TEST_ASSERT(handle);
	error = dlerror();
	TEST_ASSERT(error);
}	


TEST_GROUP_RUNNER(t_dlerror_cleared)
{
	RUN_TEST_CASE(t_dlerror_cleared, rtld_dlerror_cleared);
}


void runner(void)
{
	RUN_TEST_GROUP(t_dlerror_cleared);
}


int main(int argc, char **argv)
{
	return UnityMain(argc, (const char **)argv, runner) == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
