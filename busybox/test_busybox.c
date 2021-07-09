#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>

int main(int argc, char **argv)
{
	int ret;
	char cmdargs[20];
	char cmd[140] = "cd /usr/test/busybox/testsuite/ && export PATH=/bin:/sbin:/usr/bin:/usr/sbin && export bindir=/bin && ./runtest";
	DIR *dir = opendir("/usr/test/busybox/testsuite");

	if (dir) {
		closedir(dir);
	}
	else if (ENOENT == errno) {
		fprintf(stderr, "There is no busybox test suite to run, build project with \"LONG_TEST = 'y'\"\n");
		return 1;
	}
	else {
		fprintf(stderr, "There is problem with opening existing /bin/testsuite directory: %s\n", strerror(errno));
		return 1;
	}

	if (argc == 2 && (strlen(argv[1]) <= (sizeof(cmdargs) - 5)))
		sprintf(cmdargs, " -v %s", argv[1]);
	else if (argc == 1)
		sprintf(cmdargs, " -v");
	else {
		fprintf(stderr, "The argument is too long!");
		return 1;
	}
	strncat(cmd, cmdargs, sizeof(cmd) - strlen(cmd) - 1);

	if ((ret = system(cmd)) < 0) {
		fprintf(stderr, "system function failed: %s\n", strerror(errno));
		return 1;
	}

	if (argc == 2)
		printf("\n****A single test of the Busybox Test Suite completed****\n\n");
	else if (argc == 1)
		printf("\n****The Busybox Test Suite completed****\n\n");

	return 0;
}
