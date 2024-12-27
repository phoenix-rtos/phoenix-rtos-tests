/*
 * Phoenix-RTOS
 *
 * lsb_vsx test launcher
 *
 * Copyright 2024 Phoenix Systems
 * Author: Adam Debek
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>


int main(int argc, char **argv)
{
	const char *cmd = "/usr/bin/tcc -j - -e -l ";
	char test_cmd[128];
	int ret;

	if (argc != 2) {
		fprintf(stderr, "Wrong number of arguments, provide only a path to test executable\n");
		exit(EXIT_FAILURE);
	}
	else {
		strcpy(test_cmd, cmd);
		strcat(test_cmd, argv[1]);
	}

	if (chdir("/root/lsb_vsx/test_sets") < 0) {
		perror("chdir");
		exit(EXIT_FAILURE);
	}

	if (setenv("TET_ROOT", "/root/lsb_vsx", 0) < 0) {
		fprintf(stderr, "setenv() - setting \"TET_ROOT\" failed\n");
		exit(EXIT_FAILURE);
	}

	if (setenv("TET_EXECUTE", "/root/lsb_vsx/test_sets/TESTROOT", 0) < 0) {
		fprintf(stderr, "setenv() - setting \"TET_EXECUTE\" failed\n");
		exit(EXIT_FAILURE);
	}

	if ((ret = system(test_cmd)) < 0) {
		perror("system");
		exit(EXIT_FAILURE);
	}

	if (WIFEXITED(ret)) {
		int exit_status = WEXITSTATUS(ret);
		if (exit_status != 0) {
			fprintf(stderr, "Error: Command exited with status %d\n", exit_status);
			exit(EXIT_FAILURE);
		}
	}
	else {
		fprintf(stderr, "Error: Command did not exit normally\n");
		exit(EXIT_FAILURE);
	}

	return EXIT_SUCCESS;
}
