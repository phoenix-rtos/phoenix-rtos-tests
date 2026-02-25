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
	char *mountPoint = NULL;

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--fs-under-test") == 0) {
			mountPoint = argv[i + 1];
		}
	}

	if (mountPoint == NULL) {
		fprintf(stderr, "No filesystem mount point provided\n");
		return EXIT_FAILURE;
	}

	char fsTestPath[PATH_MAX];
	if (strcmp(mountPoint, "/") == 0) {
		snprintf(fsTestPath, sizeof(fsTestPath), "/fs_test");
	}
	else {
		snprintf(fsTestPath, sizeof(fsTestPath), "%s/fs_test", mountPoint);
	}

	if (mkdir(fsTestPath, S_IWUSR | S_IXUSR) != 0) {
		if (errno != EEXIST) {
			perror("mkdir");
			return EXIT_FAILURE;
		}
	}

	if (chdir(fsTestPath) != 0) {
		perror("chdir");
		return EXIT_FAILURE;
	}

	int ret = UnityMain(argc, (const char **)argv, runner);

	if (rmdir(fsTestPath) != 0) {
		perror("Failed to remove fs_test directory");
		return EXIT_FAILURE;
	}

	return (ret == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
