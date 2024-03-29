/*
 * Phoenix-RTOS
 *
 * libtinyaes tests
 *
 * Main entry point.
 *
 * Copyright 2024 by Phoenix Systems
 * Author: Mateusz Bloch
 *
 * This file is a part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#include <stdlib.h>
#include "unity_fixture.h"

void runner(void)
{
	RUN_TEST_GROUP(aes_ccm_s);
	RUN_TEST_GROUP(aes_eax);
	RUN_TEST_GROUP(aes_gcm);
	RUN_TEST_GROUP(aes_kw);
	RUN_TEST_GROUP(aes_cmac);
}


int main(int argc, char *argv[])
{
	return UnityMain(argc, (const char **)argv, runner) == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
