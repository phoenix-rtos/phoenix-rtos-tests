/*	$NetBSD: t_tls_extern.c,v 1.16 2024/07/23 18:11:53 riastradh Exp $	*/

/*-
 * Copyright (c) 2023 The NetBSD Foundation, Inc.
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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <NetBSD/dlfcn.h>

#include "helpers.h"


void *use, *def;


TEST_GROUP(t_tls_extern);


TEST_SETUP(t_tls_extern)
{
	use = NULL;
	def = NULL;
}


TEST_TEAR_DOWN(t_tls_extern)
{
	/* Guarantee dlclose being run at the end of the test. 
	 * each dlclose in test case must always assign corresponding variable to NULL.
	 * each dlopen must assign to one of the variables. */
	if (use != NULL) {
		(void)dlclose(use);
	}
	if (def != NULL) {
		(void)dlclose(def);
	}
}

/* "Testing opening and closing in a loop,"
	    " then opening and using dynamic TLS" */
TEST(t_tls_extern, opencloseloop_use)
{
	unsigned i;
	int *(*fdef)(void), *(*fuse)(void);
	int *pdef, *puse;

	/*
	 * Open and close the definition library repeatedly.  This
	 * should trigger allocation of many DTV offsets, which are
	 * (currently) not recycled, so the required DTV offsets should
	 * become very long -- pages past what is actually allocated
	 * before we attempt to use it.
	 *
	 * This way, we will exercise the wrong-way-conditional fast
	 * path of PR lib/58154.
	 */
	for (i = sysconf(_SC_PAGESIZE); i --> 0;) {
		TEST_ASSERT_DL(def = dlopen("libh_def_dynamic.so", 0));
		TEST_ASSERT_EQ_MSGF(dlclose(def), 0,
		    "dlclose(def): %s", dlerror());
		/* Mark as NULL to avoid closing again in TEARDOWN. */
		def = NULL;
	}

	/*
	 * Now open the definition library and keep it open.
	 */
	TEST_ASSERT_DL(def = dlopen("libh_def_dynamic.so", 0));
	TEST_ASSERT_DL(fdef = dlsym(def, "fdef"));

	/*
	 * Open libraries that use the definition and verify they
	 * observe the same pointer.
	 */
	TEST_ASSERT_DL(use = dlopen("libh_use_dynamic.so", 0));
	TEST_ASSERT_DL(fuse = dlsym(use, "fuse"));
	pdef = (*fdef)();
	puse = (*fuse)();
	TEST_ASSERT_EQ_MSGF(pdef, puse,
	    "%p in defining library != %p in using library",
	    pdef, puse);

	/*
	 * Also verify the pointer can be used.
	 */
	*pdef = 123;
	*puse = 456;
	TEST_ASSERT_EQ_MSGF(*pdef, *puse,
	    "%d in defining library != %d in using library",
	    *pdef, *puse);
}


TEST_GROUP_RUNNER(t_tls_extern)
{
	RUN_TEST_CASE(t_tls_extern, opencloseloop_use);
}


void runner(void)
{
	RUN_TEST_GROUP(t_tls_extern);
}


int main(int argc, char **argv)
{
	return UnityMain(argc, (const char **)argv, runner) == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
