#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define BUF_SZ 256

int process_command(char *input)
{
	const char *hello_prefix = "hello from ";

	if (strcmp(input, "EXIT") == 0)
		return 0;
	else if (strcmp(input, "echo") == 0 ||
			(strcmp(input, "ping") == 0))
		printf("[OK]\n");
	else if (strncmp(input, hello_prefix, strlen(hello_prefix)) == 0)
		printf("hello!\n");
	else if (strlen(input) > 0)
		printf("%s [FAIL]\n", input);
	return 1;
}

int main(int argc, char **argv)
{
	char c = '\0';
	char buffer[BUF_SZ] = { 0 };
	int pos = 0;
	int ret_code = 1;

	printf("[Commence Fake Communication]\n");

	// Run the loop until `EXIT`
	while (ret_code) {
		if (read(STDIN_FILENO, &c, 1) != 1)
			return -1;

		if (c == '\n') {
			buffer[pos] = '\0';
			ret_code = process_command(buffer);
			pos = 0;
		}
		else if (pos < BUF_SZ - 1) {
			buffer[pos++] = c;
		}
		else {
			printf("[Failure!]\n");
			return -1;
		}
	}

	printf("[Success!]\n");
	return 0;
}
