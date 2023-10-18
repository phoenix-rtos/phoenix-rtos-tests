/*
 * Phoenix-RTOS
 *
 * libc-tests
 *
 * Main entry point.
 *
 * Copyright 2021, 2022 Phoenix Systems
 * Author: Marek Bialowas, Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "unity_fixture.h"

/* no need for forward declarations, RUN_TEST_GROUP does it by itself */
void runner(void)
{
	RUN_TEST_GROUP(opendir);
	RUN_TEST_GROUP(closedir);
	RUN_TEST_GROUP(readdir);
	RUN_TEST_GROUP(rewinddir);
}


int main(int argc, char *argv[])
{
	UnityMain(argc, (const char **)argv, runner);

	return 0;
}
