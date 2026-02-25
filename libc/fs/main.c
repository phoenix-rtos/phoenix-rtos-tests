/*
 * Phoenix-RTOS
 *
 * libc-tests
 *
 * Testing fs related functions
 *
 * Copyright 2022 Phoenix Systems
 * Author: Mateusz Niewiadomski, Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <unity_fixture.h>


void runner(void)
{
	RUN_TEST_GROUP(file);
	RUN_TEST_GROUP(file_pread);
#ifdef __phoenix__
	RUN_TEST_GROUP(file_safe);
	RUN_TEST_GROUP(file_safe_pread);
#endif
	RUN_TEST_GROUP(stat_mode);
	RUN_TEST_GROUP(stat_nlink_size_blk_tim);
	RUN_TEST_GROUP(stat_errno);
	RUN_TEST_GROUP(rmdir);
}


int main(int argc, char **argv)
{
	return (UnityMain(argc, (const char **)argv, runner) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
