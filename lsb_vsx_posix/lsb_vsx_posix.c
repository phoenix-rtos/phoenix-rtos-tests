#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>

int main(int argc, char **argv)
{
	const char *cmd_all_tests = "/usr/test/lsb_vsx_posix/files/bin/tcc -p -e -s /usr/test/lsb_vsx_posix/files/test_sets/scen.exec -j -";
	const char *cmd_single_test = "/usr/test/lsb_vsx_posix/files/bin/tcc -p -e -s /usr/test/lsb_vsx_posix/files/test_sets/scen_single.exec -j -";
	const char *cmd_clean = "/usr/test/lsb_vsx_posix/files/bin/tcc -p -c -s /usr/test/lsb_vsx_posix/files/test_sets/scen_single.exec";
	const char *cwd = "/usr/test/lsb_vsx_posix/files/test_sets";
	const char *resultPath = "/usr/test/lsb_vsx_posix/files/test_sets/results";

	char *line = NULL;
	char total[40];
	size_t len = 0;
	ssize_t nread;
	int single_test_f;
	FILE *all_tests_f;


	if (chdir(cwd) != 0) {
		perror("Error in chdir()");
		return 1;
	}

	/* Set necessary enviroment variables */
	if (setenv("TET_ROOT", "/usr/test/lsb_vsx_posix/files", 0) != 0) {
		perror("setenv() - setting \"TET_ROOT\" failed");
		return 1;
	}

	if (setenv("TET_EXECUTE", "/usr/test/lsb_vsx_posix/files/test_sets/TESTROOT", 0) != 0) {
		perror("setenv() - setting \"TET_EXECUTE\" failed");
		return 1;
	}

	/* Single test execution */
	if (argc == 2) {
		/* File containing all tests */
		all_tests_f = fopen("/usr/test/lsb_vsx_posix/files/test_sets/scen.exec", "r");
		if (all_tests_f == -1) {
			perror("fopen");
			exit(EXIT_FAILURE);
		}
		/* File which we gonna pass to tcc, this file will contain 2 required lines and one test name */
		single_test_f = open("/usr/test/lsb_vsx_posix/files/test_sets/scen_single.exec", O_WRONLY | O_CREAT | O_TRUNC);
		if (single_test_f == NULL) {
			perror("fopen");
			exit(EXIT_FAILURE);
		}
		/* append "all" to start of scenario file in order to follow file format*/
		write(single_test_f, "all\n", 5);

		while ((nread = getline(&line, &len, all_tests_f)) != -1) {
			/* There is also needed second line which resides in processed file before every specific tests subset.
			 * Appropriate line will be stored during processing appropriate subset, so when test will be found
			 * the line we need will reside in buffer "total".
			 */
			if (strstr(line, "total tests in") != NULL) {
				strncpy(total, line, 40);
			}
			if (strstr(line, argv[1]) != NULL) {
				/* Write second line */
				write(single_test_f, total, strlen(total));
				/* Write test name */
				write(single_test_f, line, nread);

				/*Test found, everything we need written, so close and clean */
				fclose(all_tests_f);
				close(single_test_f);
				free(line);
				break;
			}
		}
		if (nread == -1) {
			fprintf(stderr, "No such test\n");
			return -1;
		}

		if ((system(cmd_single_test)) < 0) {
			perror("system function failed:");
			return 1;
		}

		if ((system(cmd_clean)) < 0) {
			perror("system function failed:");
			return 1;
		}
	}
	else if (argc > 2) {
		fprintf(stderr, "Type single test name or nothing to execute all tests\n");
		return -1;
	}
	else {
		if (system(cmd_all_tests) < 0) {
			perror("system function failed:");
			return 1;
		}
	}
}