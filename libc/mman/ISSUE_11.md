# MAP_SHARED writes via mmap() not visible through read()

## Category
BUG

## Description
When a file is mapped with MAP_SHARED and PROT_WRITE, modifications made through the mapping are not reflected when reading the file via read().

## Expected behavior
After writing through a MAP_SHARED mapping, a subsequent read() from the same file descriptor (after seeking to the modified position) shall return the modified data.

## Actual behavior
read() returns the original file content ('A') instead of the modified content ('B').

## POSIX requirement (if applicable)
POSIX.1-2017 (IEEE Std 1003.1-2017), mmap(3p), section DESCRIPTION:
  "If MAP_SHARED is specified, write references shall change the underlying object."

## Minimal reproduction code

```c
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

int main(void)
{
	const char *path = "mmap_test";
	char buf[1] = { 'A' };
	char readbuf[1];
	char *ptr;
	int fd;

	fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0600);
	if (fd < 0) {
		printf("open failed: %s\n", strerror(errno));
		return 1;
	}
	write(fd, buf, 1);

	ptr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (ptr == MAP_FAILED) {
		printf("mmap failed: %s\n", strerror(errno));
		close(fd);
		unlink(path);
		return 1;
	}

	ptr[0] = 'B';
	lseek(fd, 0, SEEK_SET);
	read(fd, readbuf, 1);
	printf("expected: 'B', got: '%c'\n", readbuf[0]);

	munmap(ptr, 4096);
	close(fd);
	unlink(path);
	return 0;
}
```

## Host behavior
```
expected: 'B', got: 'B'
```

## Revision
00ac1bc3dfbcfa6e2e6ac7e619910efc5d664d1c

## Target
Reproduced on ia32-generic-qemu, not tested on other targets yet.

## Related tests
- `phoenix-rtos-tests/libc/mman/mman_mmap.c`

These tests are currently skipped on the affected target. After applying a fix,
re-enable them and re-run the suite to verify the fix.
