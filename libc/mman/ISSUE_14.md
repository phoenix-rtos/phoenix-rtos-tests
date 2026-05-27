# munmap() does not fail with EINVAL when len is 0

## Category
POSIX_NONCOMPLIANCE

## Description
munmap() with len==0 returns 0 (success) instead of failing with EINVAL.

## Expected behavior
munmap() shall return -1 and set errno to EINVAL when len is 0.

## Actual behavior
munmap() returns 0 with errno unchanged.

## POSIX requirement (if applicable)
POSIX.1-2017 (IEEE Std 1003.1-2017), munmap(3p), section ERRORS:
  "[EINVAL] The len argument is 0."

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
	char buf[4096];
	void *ptr;
	int fd, ret;

	memset(buf, 'A', sizeof(buf));
	fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0600);
	write(fd, buf, sizeof(buf));

	ptr = mmap(NULL, 4096, PROT_READ, MAP_PRIVATE, fd, 0);
	if (ptr == MAP_FAILED) {
		printf("mmap failed: %s\n", strerror(errno));
		close(fd);
		unlink(path);
		return 1;
	}

	errno = 0;
	ret = munmap(ptr, 0);
	printf("munmap(ptr, 0): ret=%d, errno=%d (%s)\n",
		ret, errno, strerror(errno));

	munmap(ptr, 4096);
	close(fd);
	unlink(path);
	return 0;
}
```

## Host behavior
```
munmap(ptr, 0): ret=-1, errno=22 (Invalid argument)
```

## Revision
00ac1bc3dfbcfa6e2e6ac7e619910efc5d664d1c

## Target
Reproduced on ia32-generic-qemu, not tested on other targets yet.

## Related tests
- `phoenix-rtos-tests/libc/mman/mman_mmap.c`

These tests are currently skipped on the affected target. After applying a fix,
re-enable them and re-run the suite to verify the fix.
