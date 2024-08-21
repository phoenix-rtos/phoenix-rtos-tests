/*
 * Phoenix-RTOS
 *
 * ld.elf_so tests
 *
 * test thread local destructor
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


TEST_GROUP(t_thread_local_dtor);


TEST_SETUP(t_thread_local_dtor)
{
	/* Nothing to do here. */
}


TEST_TEAR_DOWN(t_thread_local_dtor)
{
	/* Nothing to do here. */
}


/* Checks dlclose vs thread_local */
TEST(t_thread_local_dtor, thread_local_dtor)
{
	const char *out[] = { "in ctor: global_dtor\n",
		"in ctor: thread_local\n",
		"before dlclose\n",
		"after dlclose\n",
		"in dtor: thread_local\n",
		"in dtor: global_dtor\n",
		NULL };
	int retCode = EXIT_SUCCESS;
	/* clang-format off */
	char *cmd = _RTLD_TEST_SRCDIR "/" "h_thread_local_dtor";
	/* clang-format on */
	char *const argv[] = { cmd, NULL };
	execAssert_execve(cmd, argv, environ, &retCode, out, NULL);
}


TEST_GROUP_RUNNER(t_thread_local_dtor)
{
	RUN_TEST_CASE(t_thread_local_dtor, thread_local_dtor);
}


void runner(void)
{
	RUN_TEST_GROUP(t_thread_local_dtor);
}


int main(int argc, char **argv)
{
	return UnityMain(argc, (const char **)argv, runner) == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
