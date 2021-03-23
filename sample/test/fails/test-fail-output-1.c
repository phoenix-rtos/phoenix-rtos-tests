#include <stdio.h>

int main(int argc, char **argv)
{
	puts("[Test FAIL TIMEOUT started]");
	puts("1");
	puts("This line will not be parsed");
	while (1) ;

	return 0;
}
