/*
 * Phoenix-RTOS
 *
 * Used for MicroPython testing. Runs MicroPython script.
 *
 * Copyright 2022 Phoenix Systems
 * Author: Piotr Nieciecki
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define MICROPYTHON_BIN "/bin/micropython "
#define PATH_TO_TESTS   "/usr/test/micropython/"

#define CMDLINE_PREFIX     "# cmdline: "
#define CMDLINE_PREFIX_LEN (sizeof(CMDLINE_PREFIX) - 1)

/* Only in this directory there are tests for UPyth options */
#define DIR_WITH_OPT_TESTS "cmdline"

static char const *PROG_NAME;


void upyth_errMsg(const char *msg)
{
	fprintf(stderr, "Error: %s - %s\n", PROG_NAME, msg);
}


char *upyth_concat(char *str1, const char *str2)
{
	int len;
	char *res;

	len = strlen(str1) + strlen(str2) + 1;
	res = realloc(str1, sizeof(char) * len);
	if (res != NULL) {
		strcat(res, str2);
	}

	return res;
}


int upyth_optionsGet(const char *path, char **options)
{
	FILE *f;
	char *line = NULL;
	size_t lineLen = 0;
	int optionsLen;
	char *newLine;

	*options = NULL;

	if (strncmp(path, DIR_WITH_OPT_TESTS, strlen(DIR_WITH_OPT_TESTS)) != 0) {
		return EXIT_SUCCESS;
	}

	f = fopen(path, "r");
	if (f == NULL) {
		upyth_errMsg("There was a problem with opening file with a script");
		return EXIT_FAILURE;
	}

	lineLen = getline(&line, &lineLen, f);
	fclose(f);
	if (lineLen < 0) {
		upyth_errMsg("There was a problem with reading script to execute");
		return EXIT_FAILURE;
	}

	if (strncmp(line, CMDLINE_PREFIX, CMDLINE_PREFIX_LEN) != 0) {
		free(line);
		return EXIT_SUCCESS;
	}

	optionsLen = lineLen - CMDLINE_PREFIX_LEN;

	*options = malloc(sizeof(char) * (optionsLen + 1));
	if (*options == NULL) {
		free(line);
		upyth_errMsg("Malloc error");
		return EXIT_FAILURE;
	}
	(*options)[optionsLen] = '\0';

	strncpy(*options, &line[CMDLINE_PREFIX_LEN], optionsLen);
	free(line);

	/* Removing new line character for result */
	newLine = strchr(*options, '\n');
	if (newLine != NULL) {
		*newLine = ' ';
	}

	/* Changing the last character to ' ' */
	/* This ensures us that the last char is ' ' and we can safely concatenate this string with the path to the test */
	(*options)[optionsLen - 1] = ' ';

	return EXIT_SUCCESS;
}


int main(int argc, char **argv)
{
	char *cmd, *options, *tmp;
	int upythProgRes;

	PROG_NAME = argv[0];

	if (argc != 2) {
		upyth_errMsg("There is no required argument - please execute this function with an internal path to the micropython test");
		return 1;
	}

	if (chdir(PATH_TO_TESTS) != 0) {
		upyth_errMsg("There is no such a micropython test to run, build project with \"LONG_TEST=y\"");
		return 1;
	}

	if (access(argv[1], F_OK) != 0) {
		upyth_errMsg("There is no such a micropython test to run, build project with \"LONG_TEST=y\"");
		return 1;
	}

	printf("Running test: %s%s\n", PATH_TO_TESTS, argv[1]);

	cmd = strdup(MICROPYTHON_BIN);
	if (cmd == NULL) {
		upyth_errMsg("Strdup error");
		return EXIT_FAILURE;
	}

	/* Some tests needs additional options to run. */
	/* In these tests first line in file contains "# cmdline: " and after needed options. */
	/* All of them are stored in DIR_WITH_OPT_TESTS */
	if (upyth_optionsGet(argv[1], &options) != 0) {
		free(cmd);
		return EXIT_FAILURE;
	}

	if (options != NULL) {
		tmp = upyth_concat(cmd, options);
		free(options);
		if (tmp == NULL) {
			free(cmd);
			upyth_errMsg("Realloc error");
			return EXIT_FAILURE;
		}
		cmd = tmp;
	}

	tmp = upyth_concat(cmd, argv[1]);
	if (tmp == NULL) {
		free(cmd);
		upyth_errMsg("Realloc error");
		return EXIT_FAILURE;
	}
	cmd = tmp;

	upythProgRes = system(cmd);
	free(cmd);
	if (upythProgRes == 1) {
		upyth_errMsg("There was an error during execution micropython test. It is possible that there is no BusyBox on system.");
		return EXIT_FAILURE;
	}
	else if (upythProgRes != 0) {
		upyth_errMsg("There was an error caused by function system() (not micropython test)");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
