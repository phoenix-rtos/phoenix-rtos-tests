/*
 * Phoenix-RTOS
 *
 * ld.elf_so tests
 *
 * test RTLD_NOOPEN flag
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


TEST_GROUP(t_df_1_noopen);


TEST_SETUP(t_df_1_noopen)
{
	/* Nothing to do here. */
}


TEST_TEAR_DOWN(t_df_1_noopen)
{
	/* Nothing to do here. */
}


/* Checks DF_1_NOOPEN prevents dlopening of library */
TEST(t_df_1_noopen, df_1_noopen1)
{
	/* clang-format off */
	const char *out[] = { "Cannot dlopen non-loadable " _RTLD_TEST_SHARED_LIBS_DIR "/" "libh_helper_ifunc_dso.so\n", NULL };
	char *cmd = _RTLD_TEST_SRCDIR "/" "h_df_1_noopen1";
	/* clang-format on */
	int retCode = EXIT_FAILURE;
	char *const argv[] = { cmd, NULL };
	execAssert_execve(cmd, argv, environ, &retCode, out, NULL);
}


/* Checks DF_1_NOOPEN is allowed on already loaded library */
TEST(t_df_1_noopen, df_1_noopen2)
{
	/* clang-format off */
	const char *out[] = { "libh_helper_ifunc_dso loaded successfully\n", NULL };
	char *cmd = _RTLD_TEST_SRCDIR "/" "h_df_1_noopen2";
	/* clang-format on */
	int retCode = EXIT_SUCCESS;
	char *const argv[] = { cmd, NULL };
	execAssert_execve(cmd, argv, environ, &retCode, out, NULL);
}


TEST_GROUP_RUNNER(t_df_1_noopen)
{
	RUN_TEST_CASE(t_df_1_noopen, df_1_noopen1);
	RUN_TEST_CASE(t_df_1_noopen, df_1_noopen2);
}


void runner(void)
{
	RUN_TEST_GROUP(t_df_1_noopen);
}


int main(int argc, char **argv)
{
	return UnityMain(argc, (const char **)argv, runner) == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
