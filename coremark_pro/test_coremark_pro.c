#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>

int main(int argc, char **argv)
{
	int ret;
	char cmdargs[40];
	char cmd[150] = "cd /usr/bin/coremark-pro/ && export PATH=/bin:/sbin:/usr/bin:/usr/sbin && ./";
	DIR *dir = opendir("/usr/bin/coremark-pro");

	if (dir) {
		closedir(dir);
	}
	else {
		fprintf(stderr, "There is problem with opening /usr/bin/coremark-pro directory: %s\n", strerror(errno));
		return 1;
	}

	if (argc == 2 && (strlen(argv[1]) <= (sizeof(cmdargs) - 5)))
		sprintf(cmdargs, "%s -v0", argv[1]);
	else {
		fprintf(stderr, "The argument is too long!");
		return 1;
	}
	strncat(cmd, cmdargs, sizeof(cmd) - strlen(cmd) - 1);

	if ((ret = system(cmd)) < 0) {
		fprintf(stderr, "system function failed: %s\n", strerror(errno));
		return 1;
	}

	return 0;
}
