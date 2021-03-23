#include <stdio.h>

int main(int argc, char *argv[])
{
	puts("[Test fail-exception started]");
	*(int*)(NULL) = 1;
	puts("[Succeeded!]");
}
