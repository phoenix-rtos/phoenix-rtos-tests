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
	RUN_TEST_GROUP(getpwd);
	RUN_TEST_GROUP(resolve_path);
	RUN_TEST_GROUP(unistd_getopt);
	RUN_TEST_GROUP(unistd_uids);
	RUN_TEST_GROUP(unistd_fsdir);
	RUN_TEST_GROUP(unistd_file);
	RUN_TEST_GROUP(wchar_wcscmp);
	RUN_TEST_GROUP(ctype);
	RUN_TEST_GROUP(stat_mode);
	RUN_TEST_GROUP(stat_nlink_size_blk_tim);
	RUN_TEST_GROUP(stat_errno);
}


/* crete directory unless it exists */
static int libc_createDirIfMissing(const char *path)
{
	struct stat buffer;

	if (stat(path, &buffer) != 0) {
		if (errno == ENOENT) {
			if (mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0) {
				fprintf(stderr, "Creating %s directory by mkdir failed: %s\n", path, strerror(errno));
				return -1;
			}
		}
		else {
			fprintf(stderr, "stat() on %s directory failed: %s\n", path, strerror(errno));
			return -1;
		}
	}

	return 0;
}


/* crete file with optional data (may be NULL) unless it exists */
static int libc_createFileIfMissing(const char *path, const char *charData)
{
	struct stat buffer;

	if (stat(path, &buffer) != 0) {
		if (errno == ENOENT) {
			if (_create_file(path, charData) != 0) {
				fprintf(stderr, "Creating %s file failed: %s\n", path, strerror(errno));
				return -1;
			}
		}
		else {
			fprintf(stderr, "stat() on %s file failed: %s\n", path, strerror(errno));
			return -1;
		}
	}

	return 0;
}


int main(int argc, char *argv[])
{
	const char *var = "POSIXLY_CORRECT";

	if (setenv(var, "y", 1) != 0) {
		fprintf(stderr, "Setting %s environment variable failed: %s\n", var, strerror(errno));
		return 1;
	}

	/* the following files may not be present on dummyfd targets,
	create them to make libc tests common */
	if (libc_createDirIfMissing("/tmp") < 0) {
		unsetenv(var);
		return 1;
	}
	if (libc_createDirIfMissing("/etc") < 0) {
		unsetenv(var);
		return 1;
	}
	if (libc_createFileIfMissing("/etc/passwd", "root:0B1ANiYi45IhxkfmUW155/GBd4IRE=:0:0:root:/:/bin/sh")) {
		unsetenv(var);
		return 1;
	}

	UnityMain(argc, (const char **)argv, runner);

	unsetenv(var);

	return 0;
}
