/*
 * Phoenix-RTOS
 *
 * test-libc-dirent
 *
 * Main entry point.
 *
 * Copyright 2023 Phoenix Systems
 * Author: Arkadiusz Kozlowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "unity_fixture.h"

/* No need for forward declarations, RUN_TEST_GROUP does it by itself */
void runner(void)
{
	RUN_TEST_GROUP(dirent_opendir);
	RUN_TEST_GROUP(dirent_closedir);
	RUN_TEST_GROUP(dirent_readdir);
	RUN_TEST_GROUP(dirent_rewinddir);
}


int main(int argc, char *argv[])
{
	UnityMain(argc, (const char **)argv, runner);

	return 0;
}
