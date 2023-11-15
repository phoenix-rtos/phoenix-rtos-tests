#include <dirent.h>
#include <unistd.h>
#include <stdio.h>

#include "unity_fixture.h"
#include "common.h"

#include "dirent_helper_functions.h"

#define INO_T_TEST_MAX_DIRS 10


void test_mkdir_asserted(char *path, mode_t mode)
{
	TEST_ASSERT_TRUE_MESSAGE(mkdir(path, mode) != -1 || errno == EEXIST, strerror(errno));
}


int test_create_directories(int num_of_dirs)
{

	char dirPath[40];
	DIR *dirs[num_of_dirs];
	int opened_dirs = 0;
	int result = 0;


	/* Create directories in batch */
	for (int i = 0; i < num_of_dirs; ++i) {

		sprintf(dirPath, "test_opendir/%d", i);

		test_mkdir_asserted(dirPath, 0777);
	}

	/* Open directories one by one, until one of them fails */
	for (int i = 0; i < num_of_dirs; ++i) {

		sprintf(dirPath, "test_opendir/%d", i);

		/*
		 * Guard clause that skips current take if dir was opened,
		 * Upon failing it proceeds to cleanup
		 */
		if ((dirs[i] = opendir(dirPath)) != NULL) {
			opened_dirs = i;
			continue;
		}

		result = -1;
		break;
	}

	for (int i = 0; i <= opened_dirs; ++i) {
		TEST_ASSERT_EQUAL(0, closedir(dirs[i]));
	}

	for (int i = 0; i < num_of_dirs; ++i) {
		sprintf(dirPath, "test_opendir/%d", i);
		rmdir(dirPath);
	}

	return result;
}


int d_ino_in(ino_t arg, ino_t *arr)
{
	for (int i = 0; i < INO_T_TEST_MAX_DIRS; ++i)
		if (arg == arr[i])
			return i;
	return -1;
}


DIR *test_opendir_asserted(const char *path)
{
	DIR *dp = opendir(path);
	TEST_ASSERT_NOT_NULL(dp);
	return dp;
}
