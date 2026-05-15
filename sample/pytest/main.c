/*
 * Phoenix-RTOS
 *
 * Simple C app for simulating serial communication
 *
 * Copyright 2026 Phoenix Systems
 * Author: Lukasz Kruszynski
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define BUF_SZ 256U


static int main_processCommand(char *input)
{
	const char *echoCmd = "echo ";

	if (strcmp(input, "EXIT") == 0) {
		return 0;
	}
	else if (strncmp(input, echoCmd, strlen(echoCmd)) == 0) {
		(void)printf("%s\n", input + strlen(echoCmd));
	}
	else if (strcmp(input, "echo") == 0 || (strcmp(input, "ping") == 0)) {
		(void)printf("main: [OK]\n");
	}
	else if (strlen(input) > 0U) {
		(void)printf("main: %s [FAIL]\n", input);
	}
	return 1;
}


int main(void)
{
	char c = '\0';
	char buffer[BUF_SZ] = { 0 };
	size_t pos = 0U;
	int returnCode = 1;

	(void)printf("main: [Commence Fake Communication]\n");

	/* Run the loop until `EXIT` */
	while (returnCode != 0) {
		if (read(STDIN_FILENO, &c, 1) != 1) {
			return -1;
		}

		if (c == '\n') {
			buffer[pos] = '\0';
			returnCode = main_processCommand(buffer);
			pos = 0U;
		}
		else if (pos < BUF_SZ - 1U) {
			buffer[pos] = c;
			pos++;
		}
		else {
			(void)printf("main: [Failure!]\n");
			return -1;
		}
	}

	(void)printf("main: [Success!]\n");
	return 0;
}
