/*
 * Phoenix-RTOS
 *
 * Cleanup after fs_mark (executed with files kept).
 *
 * Copyright 2023 Phoenix Systems
 * Author: Adam Debek
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <stdint.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <limits.h>

static int remove_dir_recursive(const char *dirPath)
{
	DIR *dir;
	char entryPath[PATH_MAX];
	struct dirent *entry;
	int (*remove_func)(const char *);
	int ret, try = 0;

	dir = opendir(dirPath);
	if (dir == NULL) {
		fprintf(stderr, "Opendir() failed errno: %s\n", strerror(errno));
		return -1;
	}

	if (dir) {
		while ((entry = readdir(dir)) != NULL) {
			if (!strcmp(".", entry->d_name) || !strcmp("..", entry->d_name)) {
				continue;
			}
			sprintf(entryPath, "%s/%s", dirPath, entry->d_name);
			remove_func = entry->d_type == DT_DIR ? remove_dir_recursive : remove;
			if (remove_func(entryPath) != 0) {
				closedir(dir);
				return -1;
			}
		}

		if (closedir(dir)) {
			return -1;
		}
	}

	errno = 0;
	while ((ret = rmdir(dirPath)) < 0) {
		if (errno == ENOTEMPTY && try < 5) {
			remove_dir_recursive(dirPath);
			try++;
		}
		else if (errno == ENOENT) {
			errno = 0;
			return 0;
		}
		else {
			return -1;
		}
	}

	return ret;
}

int main(int argc, char **argv)
{
	int i;
	DIR *dir;

	if (argc < 2) {
		fprintf(stderr, "Usage: %s [dir1] ... [dirN]\n", argv[0]);
		return 1;
	}

	for (i = 1; i < argc; i++) {
		if ((dir = opendir(argv[i])) == NULL && errno == ENOENT) {
			fprintf(stderr, "Nonexistent directory name\n");
			return 1;
		}
		/* Clean test directory */
		errno = 0;
		if (remove_dir_recursive(argv[i]) < 0) {
			fprintf(stderr, "Error in remove_dir_recursive() errno: %s\n", strerror(errno));
			return 1;
		}
	}

	fprintf(stderr, "Clean successful\n");
	return 0;
}
