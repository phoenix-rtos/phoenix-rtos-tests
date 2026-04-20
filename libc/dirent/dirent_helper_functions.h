/*
 * Phoenix-RTOS
 *
 * test-libc-dirent
 *
 * Test helper functions.
 *
 * Copyright 2023 Phoenix Systems
 * Author: Arkadiusz Kozlowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef DIRENT_HELPER_FUNCTIONS_H
#define DIRENT_HELPER_FUNCTIONS_H

#include <dirent.h>
#include <sys/stat.h>
#include <string.h>


#define TEST_MKDIR_ASSERTED(path, mode) TEST_ASSERT_TRUE_MESSAGE(mkdir(path, mode) != -1 || errno == EEXIST, strerror(errno))

#define TEST_OPENDIR_ASSERTED(path) \
	({ \
		DIR *dp = opendir(path); \
		TEST_ASSERT_NOT_NULL(dp); \
		dp; \
	})


#endif
