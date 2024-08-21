/*	$NetBSD: t_dlinfo.c,v 1.8 2023/11/24 17:40:20 riastradh Exp $	*/

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
#include <string.h>
#include <NetBSD/dlfcn.h>
#include <NetBSD/link_elf.h>

#include "helpers.h"


void *handle, *tmp;


TEST_GROUP(t_dlinfo);


TEST_SETUP(t_dlinfo)
{
	handle = NULL;
	tmp = NULL;
}


TEST_TEAR_DOWN(t_dlinfo)
{
	if (handle != NULL) {
		(void)dlclose(handle);
	}
	if (tmp != NULL) {
		(void)dlclose(tmp);
	}
}

/* dlinfo with RTLD_SELF handle works */
TEST(t_dlinfo, rtld_dlinfo_linkmap_self)
{
	struct link_map *map;
	int rv;

	rv = dlinfo(RTLD_SELF, RTLD_DI_LINKMAP, &map);
	TEST_ASSERT_EQ_MSGF(rv, 0, "dlinfo: %s", dlerror());
	TEST_ASSERT((strstr(map->l_name, "t_dlinfo") != NULL));
}

/* dlinfo with invalid handle fails */
TEST(t_dlinfo, rtld_dlinfo_linkmap_inval)
{
	void *v;
	int rv;

	rv = dlinfo(NULL, RTLD_DI_LINKMAP, &v);
	TEST_ASSERT_EQ_MSGF(rv, -1, "rv=%d", rv);
}

/* dlinfo dlopen'd handle works */
TEST(t_dlinfo, rtld_dlinfo_linkmap_dlopen)
{
	struct link_map *map;
	int rv;

	/* NOTE: libutil can't be used as is nonexistent on phoenix */
	handle = dlopen("libh_helper_ifunc_dso.so", RTLD_LAZY);
	TEST_ASSERT_MSGF(handle, "dlopen: %s", dlerror());

	rv = dlinfo(handle, RTLD_DI_LINKMAP, &map);
	TEST_ASSERT_EQ_MSGF(rv, 0, "dlinfo: %s", dlerror());
	TEST_ASSERT((strstr(map->l_name, "libh_helper_ifunc_dso.so") != NULL));
	TEST_ASSERT_EQ_MSGF(dlclose(handle), 0, "dlclose: %s", dlerror());
	handle = NULL;
}

/* dlopen'd dso's show up in the list */
TEST(t_dlinfo, rtld_dlinfo_linkmap_dlopen_iter)
{
	struct link_map *map;

	/* NOTE: libutil can't be used as is nonexistent on phoenix */
	handle = dlopen("libh_helper_ifunc_dso.so", RTLD_LAZY);
	TEST_ASSERT_MSGF(handle, "dlopen: %s", dlerror());

	TEST_ASSERT_EQ_MSGF(dlinfo(RTLD_SELF, RTLD_DI_LINKMAP, &map), 0,
	    "dlinfo: %s", dlerror());

	for (; map->l_next; map = map->l_next)
		continue;
	for (; map; map = map->l_prev)
		if (strstr(map->l_name, "libh_helper_ifunc_dso.so") != NULL)
			break;

	TEST_ASSERT_MSGF(map, "dlopen()d object not found from linkmap");
	tmp = dlopen(map->l_name, RTLD_LAZY);
	TEST_ASSERT_MSGF(tmp != NULL, "could not dlopen() name in linkmap: %s", dlerror());
}


TEST_GROUP_RUNNER(t_dlinfo)
{
	RUN_TEST_CASE(t_dlinfo, rtld_dlinfo_linkmap_self);
	RUN_TEST_CASE(t_dlinfo, rtld_dlinfo_linkmap_inval);
	RUN_TEST_CASE(t_dlinfo, rtld_dlinfo_linkmap_dlopen);
	RUN_TEST_CASE(t_dlinfo, rtld_dlinfo_linkmap_dlopen_iter);
}


void runner(void)
{
	RUN_TEST_GROUP(t_dlinfo);
}


int main(int argc, char **argv)
{
	return UnityMain(argc, (const char **)argv, runner) == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
