/*
 * Phoenix-RTOS
 *
 * ld.elf_so tests
 *
 * test symbol versioning
 *
 * Copyright 2024 Phoenix Systems
 * Author: Hubert Badocha
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <sys/types.h>

#include <unity_fixture.h>
#include <string.h>
#include <errno.h>
#include <NetBSD/dlfcn.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "helpers.h"
#include "libexecassert/execassert.h"


#ifndef _RTLD_TEST_SRCDIR
#error "_RTLD_TEST_SRCDIR" not defined!
#endif

#ifndef _RTLD_TEST_SHARED_LIBS_DIR
#error "_RTLD_TEST_SHARED_LIBS_DIR" not defined!
#endif


TEST_GROUP(t_dl_symver);


TEST_SETUP(t_dl_symver)
{
	/* Nothing to do here. */
}


TEST_TEAR_DOWN(t_dl_symver)
{
	/* Nothing to do here. */
}


/* Checks ELF symbol versioning functions */
TEST(t_dl_symver, dl_symver)
{
	/* clang-format off */
	const char **out[3][3] = {
		{
			(const char *[]) { "0\n", NULL },
			(const char *[]) { "1\n", NULL },
			(const char *[]) { "1\n", NULL },
		},
		{
			NULL,
			(const char *[]) { "1\n", NULL },
			(const char *[]) { "1\n", NULL },
		},
		{
			NULL,
			NULL,
			(const char *[]) { "3\n", NULL },
		},
	};

	const char **err[3][3] = {
		{
			NULL,
			NULL,
			NULL 
		},
		{
			(const char *[]) {
			  _RTLD_TEST_SHARED_LIBS_DIR "/h_helper_symver_dso0/libh_helper_symver_dso.so: version V_1 required by " _RTLD_TEST_SRCDIR "/h_dl_symver_v1 not defined\n",
			  NULL
			},
			NULL,
			NULL 
		},
		{
			(const char *[]) {
			  _RTLD_TEST_SHARED_LIBS_DIR "/h_helper_symver_dso0/libh_helper_symver_dso.so: version V_3 required by " _RTLD_TEST_SRCDIR "/h_dl_symver_v2 not defined\n",
			  NULL
			},
			(const char *[]) {
				_RTLD_TEST_SHARED_LIBS_DIR "/h_helper_symver_dso1/libh_helper_symver_dso.so: version V_3 required by " _RTLD_TEST_SRCDIR "/h_dl_symver_v2 not found\n",
				NULL
			},
			NULL 
		},
	};
	/* clang-format on */

	for (int exeVer = 0; exeVer < 3; exeVer++) {
		for (int libVer = 0; libVer < 3; libVer++) {
			char path[64];
			char lib[128];

			(void)snprintf(lib, sizeof(lib), _RTLD_TEST_SHARED_LIBS_DIR "/h_helper_symver_dso%d", libVer);

			/* Make sure LD_LIBRARY_PATH is set in child process. */
			TEST_ASSERT_EQUAL(0, setenv("LD_LIBRARY_PATH", lib, 1));

			(void)snprintf(path, sizeof(path), _RTLD_TEST_SRCDIR "/h_dl_symver_v%d", exeVer);

			char *argv[] = { path, lib, NULL };

			execAssert_execve(path, argv, environ, NULL, out[exeVer][libVer], err[exeVer][libVer]);
		}
	}
}


TEST_GROUP_RUNNER(t_dl_symver)
{
	RUN_TEST_CASE(t_dl_symver, dl_symver);
}


void runner(void)
{
	RUN_TEST_GROUP(t_dl_symver);
}


int main(int argc, char **argv)
{
	return UnityMain(argc, (const char **)argv, runner) == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
