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


enum order {
	DEF_USE_EAGER,
	DEF_USE_LAZY,
	USE_DEF,
	USE_DEF_NOLOAD,
};


/* Define handle to dlopen. tmp is for dlopen not assiging to any value in origin code. */
void *use, *def, *use_dynamic, *use_static, *tmp[3];


TEST_GROUP(t_tls_extern);


TEST_SETUP(t_tls_extern)
{
	use = NULL;
	def = NULL;
	use_dynamic = NULL;
	use_static = NULL;
	for (int i = 0; i < sizeof(tmp) / sizeof(*tmp); i++){
		tmp[i] = NULL;
	}
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
	if (use_dynamic != NULL) {
		(void)dlclose(use_dynamic);
	}
	if (use_static != NULL) {
		(void)dlclose(use_static);
	}
	for (int i = 0; i < sizeof(tmp) / sizeof(*tmp); i++){
		if (tmp[i] != NULL) {
			(void)dlclose(tmp[i]);
		}
	}
}

static void
tls_extern(const char *libdef, const char *libuse, enum order order)
{
	int *(*fdef)(void), *(*fuse)(void);
	int *pdef, *puse;

	(void)dlerror();

	switch (order) {
	case DEF_USE_EAGER:
		TEST_ASSERT_DL(def = dlopen(libdef, 0));
		TEST_ASSERT_DL(fdef = dlsym(def, "fdef"));
		pdef = (*fdef)();
		TEST_ASSERT_DL(use = dlopen(libuse, 0));
		TEST_ASSERT_DL(fuse = dlsym(use, "fuse"));
		puse = (*fuse)();
		break;
	case DEF_USE_LAZY:
		TEST_ASSERT_DL(def = dlopen(libdef, 0));
		TEST_ASSERT_DL(use = dlopen(libuse, 0));
		goto lazy;
	case USE_DEF:
		TEST_ASSERT_DL(use = dlopen(libuse, 0));
		TEST_ASSERT_DL(def = dlopen(libdef, 0));
		goto lazy;
	case USE_DEF_NOLOAD:
		TEST_ASSERT_DL(use = dlopen(libuse, 0));
		TEST_ASSERT_DL(def = dlopen(libdef, RTLD_NOLOAD));
lazy:		TEST_ASSERT_DL(fdef = dlsym(def, "fdef"));
		TEST_ASSERT_DL(fuse = dlsym(use, "fuse"));
		pdef = (*fdef)();
		puse = (*fuse)();
		break;
	}

	TEST_ASSERT_EQ_MSGF(pdef, puse,
	    "%p in defining library != %p in using library",
	    pdef, puse);
}

/* "descr", "extern __thread for TLS works,"
	    " loading static use than dynamic def") */
TEST(t_tls_extern, dynamic_abusedef)
{
	tls_extern("libh_def_dynamic.so", "libh_abuse_dynamic.so", USE_DEF);
}

/* "extern __thread for TLS works,"
	    " loading static use then dynamic def with RTLD_NOLOAD" */
TEST(t_tls_extern, dynamic_abusedefnoload)
{
	tls_extern("libh_def_dynamic.so", "libh_abuse_dynamic.so",
	    USE_DEF_NOLOAD);
}

/* "descr", "dlopen refuses extern __thread for TLS,"
	    " loading dynamic def then static use eagerly" */
TEST(t_tls_extern, dynamic_defabuse_eager)
{
	int *(*fdef)(void);

	TEST_ASSERT_DL(def = dlopen("libh_def_dynamic.so", 0));
	TEST_ASSERT_DL(fdef = dlsym(def, "fdef"));
	(void)(*fdef)();
	TEST_ASSERT_EQ_MSGF(NULL, tmp[0] = dlopen("libh_abuse_dynamic.so", 0),
	    "dlopen failed to detect static-then-dynamic abuse");
}

/* "extern __thread for TLS works,"
	    " loading dynamic def then static use lazily" */
TEST(t_tls_extern, dynamic_defabuse_lazy)
{
	tls_extern("libh_def_dynamic.so", "libh_abuse_dynamic.so",
	    DEF_USE_LAZY);
}

/* "extern __thread for dynamic TLS works,"
	    " loading def then use eagerly" */
TEST(t_tls_extern, dynamic_defuse_eager)
{
	tls_extern("libh_def_dynamic.so", "libh_use_dynamic.so",
	    DEF_USE_EAGER);
}

/* "extern __thread for dynamic TLS works,"
	    " loading def then use lazyly" */
TEST(t_tls_extern, dynamic_defuse_lazy)
{
	tls_extern("libh_def_dynamic.so", "libh_use_dynamic.so",
	    DEF_USE_LAZY);
}

/* "extern __thread for dynamic TLS works,"
	    " loading use then def" */
TEST(t_tls_extern, dynamic_usedef)
{
	tls_extern("libh_def_dynamic.so", "libh_use_dynamic.so",
	    USE_DEF);
}

/* "extern __thread for dynamic TLS works,"
	    " loading use then def with RTLD_NOLOAD" */
TEST(t_tls_extern, dynamic_usedefnoload)
{
	tls_extern("libh_def_dynamic.so", "libh_use_dynamic.so",
	    USE_DEF_NOLOAD);
}

/* "extern __thread for TLS works,"
	    " loading dynamic use then static def" */
TEST(t_tls_extern, static_abusedef)
{
	tls_extern("libh_def_static.so", "libh_abuse_static.so", USE_DEF);
}

/* "extern __thread for TLS works,"
	    " loading dynamic use then static def with RTLD_NOLOAD" */
TEST(t_tls_extern, static_abusedefnoload)
{
	tls_extern("libh_def_static.so", "libh_abuse_static.so",
	    USE_DEF_NOLOAD);
}

/* "extern __thread for TLS works,"
	    " loading static def then dynamic use eagerly" */
TEST(t_tls_extern, static_defabuse_eager)
{
	tls_extern("libh_def_static.so", "libh_abuse_static.so",
	    DEF_USE_EAGER);
}

/* "extern __thread for TLS works,"
	    " loading static def then dynamic use lazyly" */
TEST(t_tls_extern, static_defabuse_lazy)
{
	tls_extern("libh_def_static.so", "libh_abuse_static.so",
	    DEF_USE_LAZY);
}

/* "extern __thread for static TLS works,"
	    " loading def then use eagerly" */
TEST(t_tls_extern, static_defuse_eager)
{
	tls_extern("libh_def_static.so", "libh_use_static.so",
	    DEF_USE_EAGER);
}

/* "extern __thread for static TLS works,"
	    " loading def then use lazyly" */
TEST(t_tls_extern, static_defuse_lazy)
{
	tls_extern("libh_def_static.so", "libh_use_static.so",
	    DEF_USE_LAZY);
}

/* "extern __thread for static TLS works,"
	    " loading use then def" */
TEST(t_tls_extern, static_usedef)
{
	tls_extern("libh_def_static.so", "libh_use_static.so",
	    USE_DEF);
}

/* "extern __thread for static TLS works,"
	    " loading use then def with RTLD_NOLOAD" */
TEST(t_tls_extern, static_usedefnoload)
{
	tls_extern("libh_def_static.so", "libh_use_static.so",
	    USE_DEF_NOLOAD);
}

/* "definition-only library,"
	    " dynamic load and use in ctor, then static load fails" */
TEST(t_tls_extern, onlydef_dynamic_static_ctor)
{

	TEST_ASSERT_DL(tmp[0] = dlopen("libh_onlydef.so", 0));
	TEST_ASSERT_DL(tmp[1] = dlopen("libh_onlyctor_dynamic.so", 0));
	TEST_ASSERT_EQ_MSGF(NULL, tmp[2] = dlopen("libh_onlyuse_static.so", 0),
	    "dlopen failed to detect dynamic-then-static abuse");
}

/* "definition-only library,"
	    " dynamic load and use, then static load fails" */
TEST(t_tls_extern, onlydef_dynamic_static_eager)
{
	int *(*fdynamic)(void);

	TEST_ASSERT_DL(use_dynamic = dlopen("libh_onlyuse_dynamic.so", 0));
	TEST_ASSERT_DL(fdynamic = dlsym(use_dynamic, "fdynamic"));
	(void)(*fdynamic)();
	TEST_ASSERT_EQ_MSGF(NULL, tmp[0] = dlopen("libh_onlyuse_static.so", 0),
	    "dlopen failed to detect dynamic-then-static abuse");
}

/* "extern __thread for TLS works,"
	    " with definition-only library, dynamic and static load and use" */
TEST(t_tls_extern, onlydef_dynamic_static_lazy)
{
	int *(*fdynamic)(void), *(*fstatic)(void);
	int *pdynamic, *pstatic;

	TEST_ASSERT_DL(use_dynamic = dlopen("libh_onlyuse_dynamic.so", 0));
	TEST_ASSERT_DL(use_static = dlopen("libh_onlyuse_static.so", 0));
	TEST_ASSERT_DL(fdynamic = dlsym(use_dynamic, "fdynamic"));
	TEST_ASSERT_DL(fstatic = dlsym(use_static, "fstatic"));
	pdynamic = (*fdynamic)();
	pstatic = (*fstatic)();
	TEST_ASSERT_EQ_MSGF(pdynamic, pstatic,
	    "%p in dynamic tls user != %p in static tls user",
	    pdynamic, pstatic);
}

/* "extern __thread for TLS works,"
	    " with definition-only library,"
	    " static load and use, then dynamic load and use" */
TEST(t_tls_extern, onlydef_static_dynamic_eager)
{
	int *(*fstatic)(void), *(*fdynamic)(void);
	int *pstatic, *pdynamic;

	TEST_ASSERT_DL(tmp[0] = dlopen("libh_onlydef.so", 0));
	TEST_ASSERT_DL(use_static = dlopen("libh_onlyuse_static.so", 0));
	TEST_ASSERT_DL(fstatic = dlsym(use_static, "fstatic"));
	pstatic = (*fstatic)();
	TEST_ASSERT_DL(use_dynamic = dlopen("libh_onlyuse_dynamic.so", 0));
	TEST_ASSERT_DL(fdynamic = dlsym(use_dynamic, "fdynamic"));
	pdynamic = (*fdynamic)();
	TEST_ASSERT_EQ_MSGF(pstatic, pdynamic,
	    "%p in static tls user != %p in dynamic tls user",
	    pstatic, pdynamic);
}

/* "extern __thread for TLS works,"
	    " with definition-only library, static and dynamic load and use" */
TEST(t_tls_extern, onlydef_static_dynamic_lazy)
{
	int *(*fstatic)(void), *(*fdynamic)(void);
	int *pstatic, *pdynamic;

	TEST_ASSERT_DL(tmp[0] = dlopen("libh_onlydef.so", 0));
	TEST_ASSERT_DL(use_static = dlopen("libh_onlyuse_static.so", 0));
	TEST_ASSERT_DL(use_dynamic = dlopen("libh_onlyuse_dynamic.so", 0));
	TEST_ASSERT_DL(fstatic = dlsym(use_static, "fstatic"));
	TEST_ASSERT_DL(fdynamic = dlsym(use_dynamic, "fdynamic"));
	pstatic = (*fstatic)();
	pdynamic = (*fdynamic)();
	TEST_ASSERT_EQ_MSGF(pstatic, pdynamic,
	    "%p in static tls user != %p in dynamic tls user",
	    pstatic, pdynamic);
}

TEST_GROUP_RUNNER(t_tls_extern)
{
	RUN_TEST_CASE(t_tls_extern, dynamic_abusedef);
	RUN_TEST_CASE(t_tls_extern, dynamic_abusedefnoload);
	RUN_TEST_CASE(t_tls_extern, dynamic_defabuse_eager);
	RUN_TEST_CASE(t_tls_extern, dynamic_defabuse_lazy);
	RUN_TEST_CASE(t_tls_extern, dynamic_defuse_eager);
	RUN_TEST_CASE(t_tls_extern, dynamic_defuse_lazy);
	RUN_TEST_CASE(t_tls_extern, dynamic_usedef);
	RUN_TEST_CASE(t_tls_extern, dynamic_usedefnoload);
	RUN_TEST_CASE(t_tls_extern, onlydef_dynamic_static_ctor);
	RUN_TEST_CASE(t_tls_extern, onlydef_dynamic_static_eager);
	RUN_TEST_CASE(t_tls_extern, onlydef_dynamic_static_lazy);
	RUN_TEST_CASE(t_tls_extern, onlydef_static_dynamic_eager);
	RUN_TEST_CASE(t_tls_extern, onlydef_static_dynamic_lazy);
	RUN_TEST_CASE(t_tls_extern, static_abusedef);
	RUN_TEST_CASE(t_tls_extern, static_abusedefnoload);
	RUN_TEST_CASE(t_tls_extern, static_defabuse_eager);
	RUN_TEST_CASE(t_tls_extern, static_defabuse_lazy);
	RUN_TEST_CASE(t_tls_extern, static_defuse_eager);
	RUN_TEST_CASE(t_tls_extern, static_defuse_lazy);
	RUN_TEST_CASE(t_tls_extern, static_usedef);
	RUN_TEST_CASE(t_tls_extern, static_usedefnoload);
}


void runner(void)
{
	RUN_TEST_GROUP(t_tls_extern);
}


int main(int argc, char **argv)
{
	return UnityMain(argc, (const char **)argv, runner) == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
