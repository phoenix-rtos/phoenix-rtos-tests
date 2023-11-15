#ifndef _DIRENT_HELPER_FUNCTIONS_H
#define _DIRENT_HELPER_FUNCTIONS_H

#include <dirent.h>
#include <sys/stat.h>
#include <string.h>

#define INO_T_TEST_MAX_DIRS 10


void test_mkdir_asserted(char *path, mode_t mode);

int test_create_directories(int num_of_dirs);

int d_ino_in(ino_t arg, ino_t *arr);

DIR *test_opendir_asserted(const char *path);


#endif