/*	$NetBSD: t_rtld_r_debug.c,v 1.5 2023/11/24 17:40:20 riastradh Exp $	*/

/*
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
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
#include <stdbool.h>
#include <NetBSD/dlfcn.h>
#include <NetBSD/link_elf.h>

#include "helpers.h"

static long int
getauxval(unsigned int type)
{
	const AuxInfo *aux;

	for (aux = _dlauxinfo(); aux->a_type != AT_NULL; ++aux) {
		if (type == aux->a_type)
			return aux->a_v;
	}

	return 0;
}

static Elf_Dyn *
get_dynamic_section(void)
{
	uintptr_t relocbase = (uintptr_t)~0U;
	const Elf_Phdr *phdr;
	Elf_Half phnum;
	const Elf_Phdr *phlimit, *dynphdr;

	phdr = (void *)getauxval(AT_PHDR);
	phnum = (Elf_Half)getauxval(AT_PHNUM);

	TEST_ASSERT(phdr != NULL);
	TEST_ASSERT(phnum != (Elf_Half)~0);

	phlimit = phdr + phnum;
	dynphdr = NULL;

	for (; phdr < phlimit; ++phdr) {
		if (phdr->p_type == PT_DYNAMIC)
			dynphdr = phdr;
		if (phdr->p_type == PT_PHDR)
			relocbase = (uintptr_t)phdr - phdr->p_vaddr;
	}

	return (Elf_Dyn *)((uint8_t *)dynphdr->p_vaddr + relocbase);
}

static struct r_debug *
get_rtld_r_debug(void)
{
	struct r_debug *debug = NULL;
	Elf_Dyn *dynp;

	for (dynp = get_dynamic_section(); dynp->d_tag != DT_NULL; dynp++) {
		if (dynp->d_tag == DT_DEBUG) {
			debug = (void *)dynp->d_un.d_val;
			break;
		}
	}
	TEST_ASSERT(debug != NULL);

	return debug;
}

static void
check_r_debug_return_link_map(const char *name, struct link_map **rmap)
{
	struct r_debug *debug;
	struct link_map *map;
	void *loader;
	bool found;

	loader = NULL;
	debug = get_rtld_r_debug();
	TEST_ASSERT(debug != NULL);
	TEST_ASSERT_EQ_MSGF(debug->r_version, R_DEBUG_VERSION,
	    "debug->r_version=%d R_DEBUG_VERSION=%d",
	    debug->r_version, R_DEBUG_VERSION);
	map = debug->r_map;
	TEST_ASSERT(map != NULL);

	for (found = false; map; map = map->l_next) {
		if (strstr(map->l_name, name) != NULL) {
			if (rmap)
				*rmap = map;
			found = true;
		} else if (strstr(map->l_name, "ld.elf_so") != NULL) {
#ifdef __FDPIC__
			loader = (void *)map->l_addr.got_value;
#else
			loader = (void *)map->l_addr;
#endif
		}
	}
	TEST_ASSERT(found);
	TEST_ASSERT(loader != NULL);
	TEST_ASSERT(debug->r_brk != NULL);
	TEST_ASSERT_EQ_MSGF(debug->r_state, RT_CONSISTENT,
	    "debug->r_state=%d RT_CONSISTENT=%d",
	    debug->r_state, RT_CONSISTENT);
	TEST_ASSERT_EQ_MSGF(debug->r_ldbase, loader,
	    "debug->r_ldbase=%p loader=%p",
	    debug->r_ldbase, loader);
}


void *handle;


TEST_GROUP(t_rtld_r_debug);


TEST_SETUP(t_rtld_r_debug)
{
	handle = NULL;
}


TEST_TEAR_DOWN(t_rtld_r_debug)
{
	if (handle != NULL) {
		(void)dlclose(handle);
	}
}


/* check whether r_debug is well-formed */
TEST(t_rtld_r_debug, self)
{
	check_r_debug_return_link_map("t_rtld_r_debug", NULL);
}


/* check whether r_debug is well-formed after an dlopen(3) call */
TEST(t_rtld_r_debug, dlopen)
{
	void *handle;
	struct link_map *map, *r_map;

	handle = dlopen("libh_helper_ifunc_dso.so", RTLD_LAZY);
	TEST_ASSERT_MSGF(handle, "dlopen: %s", dlerror());

	check_r_debug_return_link_map("libh_helper_ifunc_dso.so", &r_map);

	TEST_ASSERT_EQ_MSGF(dlinfo(handle, RTLD_DI_LINKMAP, &map), 0,
	    "dlinfo: %s", dlerror());

	TEST_ASSERT_EQ_MSGF(map, r_map, "map=%p r_map=%p", map, r_map);
	TEST_ASSERT_EQ_MSGF(dlclose(handle), 0, "dlclose: %s", dlerror());
	handle = NULL;
}


TEST_GROUP_RUNNER(t_rtld_r_debug)
{
	RUN_TEST_CASE(t_rtld_r_debug, self);
	RUN_TEST_CASE(t_rtld_r_debug, dlopen);
}


void runner(void)
{
	RUN_TEST_GROUP(t_rtld_r_debug);
}


int main(int argc, char **argv)
{
	return UnityMain(argc, (const char **)argv, runner) == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}