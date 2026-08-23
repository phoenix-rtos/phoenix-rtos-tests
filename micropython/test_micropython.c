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
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#define MICROPYTHON_BIN "/bin/micropython"
#define PATH_TO_TESTS   "/usr/test/micropython/"

#define CMDLINE_PREFIX     "# cmdline: "
#define CMDLINE_PREFIX_LEN (sizeof(CMDLINE_PREFIX) - 1)

/* Only in this directory there are tests for UPyth options */
#define DIR_WITH_OPT_TESTS "cmdline"

/* micropython + options + script path + NULL */
#define MAX_UPYTH_ARGS 32

static char const *PROG_NAME;


void upyth_errMsg(const char *msg)
{
	fprintf(stderr, "Error: %s - %s\n", PROG_NAME, msg);
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
		*newLine = '\0';
	}

	/* Drop trailing spaces left after stripping the newline */
	optionsLen = (int)strlen(*options);
	while (optionsLen > 0 && (*options)[optionsLen - 1] == ' ') {
		(*options)[--optionsLen] = '\0';
	}

	return EXIT_SUCCESS;
}


/* Build argv for execv from optional space-separated options and the script path.
 * optionsBuf ownership stays with the caller; tokens point into it (strtok mutates it).
 */
static int upyth_buildArgv(char *optionsBuf, char *testfile, char **argv, int argvMax)
{
	int argc = 0;
	char *tok;

	if (argvMax < 3) {
		return -1;
	}

	argv[argc++] = (char *)MICROPYTHON_BIN;

	if (optionsBuf != NULL && optionsBuf[0] != '\0') {
		tok = strtok(optionsBuf, " \t");
		while (tok != NULL) {
			if (argc >= argvMax - 2) {
				upyth_errMsg("Too many micropython options");
				return -1;
			}
			argv[argc++] = tok;
			tok = strtok(NULL, " \t");
		}
	}

	argv[argc++] = testfile;
	argv[argc] = NULL;

	return argc;
}


/* Run micropython via fork+execv so no /bin/sh (or busybox) is required. */
static int upyth_run(char *optionsBuf, char *testfile)
{
	char *argv[MAX_UPYTH_ARGS];
	pid_t pid;
	int status;
	int argc;

	argc = upyth_buildArgv(optionsBuf, testfile, argv, MAX_UPYTH_ARGS);
	if (argc < 0) {
		return EXIT_FAILURE;
	}

	pid = fork();
	if (pid < 0) {
		upyth_errMsg("fork() failed while starting micropython");
		return EXIT_FAILURE;
	}

	if (pid == 0) {
		execv(MICROPYTHON_BIN, argv);
		fprintf(stderr, "Error: %s - execv(%s) failed: %s\n", PROG_NAME, MICROPYTHON_BIN, strerror(errno));
		_exit(127);
	}

	if (waitpid(pid, &status, 0) < 0) {
		upyth_errMsg("waitpid() failed while running micropython");
		return EXIT_FAILURE;
	}

	if (!WIFEXITED(status)) {
		upyth_errMsg("micropython terminated abnormally");
		return EXIT_FAILURE;
	}

	return WEXITSTATUS(status);
}


int main(int argc, char **argv)
{
	char *options, *testfile, *tmp;
	int res;

	PROG_NAME = argv[0];

	if (argc != 2) {
		upyth_errMsg("There is no required argument - please execute this function with an internal path to the micropython test");
		return 1;
	}

	testfile = strrchr(argv[1], '/');
	if ((strncmp(argv[1], DIR_WITH_OPT_TESTS, strlen(DIR_WITH_OPT_TESTS)) == 0) || (testfile == NULL)) {
		/* cmdline tests are run from different location */
		testfile = argv[1];
		tmp = strdup(PATH_TO_TESTS);
		if (tmp == NULL) {
			upyth_errMsg("strdup error");
			return 1;
		}
	}
	else {
		*testfile = '\0';
		tmp = malloc(strlen(PATH_TO_TESTS) + strlen(argv[1]) + 1);
		if (tmp == NULL) {
			upyth_errMsg("malloc error");
			return 1;
		}
		sprintf(tmp, "%s%s", PATH_TO_TESTS, argv[1]);
		testfile[0] = '/';
		testfile++;
	}

	res = chdir(tmp);
	free(tmp);
	if (res != 0) {
		upyth_errMsg("There is no such a micropython test to run, build project with \"LONG_TEST=y\"");
		return 1;
	}

	if (access(testfile, F_OK) != 0) {
		upyth_errMsg("There is no such a micropython test to run, build project with \"LONG_TEST=y\"");
		return 1;
	}

	printf("Running test: %s%s\n", PATH_TO_TESTS, argv[1]);

	/* Some tests needs additional options to run. */
	/* In these tests first line in file contains "# cmdline: " and after needed options. */
	/* All of them are stored in DIR_WITH_OPT_TESTS */
	if (upyth_optionsGet(testfile, &options) != 0) {
		return EXIT_FAILURE;
	}

	res = upyth_run(options, testfile);
	free(options);

	if (res != 0) {
		upyth_errMsg("There was an error during execution micropython test");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}