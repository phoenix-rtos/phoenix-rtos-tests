/*
 * Phoenix-RTOS
 *
 * test-libc-dirent
 *
 * Test helper functions.
 *
 * Copyright 2023-2026 Phoenix Systems
 * Author: Arkadiusz Kozlowski, Lukasz Kruszynski
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
#include <errno.h>
#include <unity.h>
#include <stdbool.h>


static inline bool ensure_new_dir_is_made(const char *path, mode_t mode)
{
	if (mkdir(path, mode) == 0) {
		return true;
	}

	return false;
}

#define TEST_MKDIR_ASSERTED(path, mode) TEST_ASSERT_TRUE(ensure_new_dir_is_made(path, mode))

#define TEST_OPENDIR_ASSERTED(path) \
	({ \
		DIR *dp = opendir(path); \
		TEST_ASSERT_NOT_NULL(dp); \
		dp; \
	})


#endif
