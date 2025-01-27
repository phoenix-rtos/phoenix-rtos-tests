/*
 * Phoenix-RTOS
 *
 * Cleanup after fs_mark (used when files are kept).
 *
 * Copyright 2025 Phoenix Systems
 * Author: Adam Debek
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <limits.h>


static int remove_dir_recursive(const char *dirPath)
{
	char entryPath[PATH_MAX];
	DIR *dir;
	struct dirent *entry;

	if ((dir = opendir(dirPath)) == NULL) {
		perror("opendir");
		return -1;
	}

	while ((entry = readdir(dir)) != NULL) {
		if (!strcmp(".", entry->d_name) || !strcmp("..", entry->d_name)) {
			continue;
		}

		sprintf(entryPath, "%s/%s", dirPath, entry->d_name);

		if (entry->d_type == DT_DIR) {
			errno = 0;
			if (rmdir(entryPath) < 0) {
				if (errno == ENOTEMPTY) {
					if (remove_dir_recursive(entryPath) < 0) {
						closedir(dir);
						return -1;
					}
				}
				else {
					perror("rmdir");
					closedir(dir);
					return -1;
				}
			}
		}
		else {
			if (unlink(entryPath) < 0) {
				perror("unlink");
				closedir(dir);
				return -1;
			}
		}
	}

	if (closedir(dir) < 0) {
		perror("closedir");
		return -1;
	}

	if (rmdir(dirPath)) {
		return -1;
	}

	return 0;
}


int main(int argc, char **argv)
{
	DIR *dir;
	int ret;

	if (argc < 2) {
		fprintf(stderr, "Usage: %s [dir1] ... [dirN]\n", argv[0]);
		return 1;
	}

	for (int i = 1; i < argc; i++) {
		if ((dir = opendir(argv[i])) == NULL) {
			perror("opendir");
			return 1;
		}

		/* Run remove_dir_recursive() multiple times because:
		   Issue: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/900
		   Issue: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/1117
		 */
		for (int j = 0; j < 20; j++) {
			errno = 0;
			ret = remove_dir_recursive(argv[i]);
			if (ret == 0) {
				break;
			}
			else if (ret < 0 && errno == ENOTEMPTY) {
				continue;
			}
			else {
				fprintf(stderr, "Error in remove_dir_recursive() errno: %s\n", strerror(errno));
				return 1;
			}
		}
	}

	return 0;
}
