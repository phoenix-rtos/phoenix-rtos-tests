#include <stdio.h>

int main(int argc, char **argv)
{
	const int arr[] = {1, 2, 3, 4, 4, 5, 6};

	puts("Unique number sequence");
	for (int i = 0; i < sizeof(arr) / sizeof(arr[0]); i++) {
		printf("%d ", arr[i]);
	}
	puts("\nThat's all what I got!");
}
