# mmap() does not fail with EACCES for PROT_WRITE and MAP_SHARED on read-only file descriptor

## Category
POSIX_NONCOMPLIANCE

## Description
mmap() with PROT_WRITE and MAP_SHARED on a file descriptor opened with O_RDONLY succeeds instead of failing with EACCES.

## Expected behavior
mmap() shall fail and set errno to EACCES when fildes is not open for write and PROT_WRITE was specified for a MAP_SHARED type mapping.

## Actual behavior
mmap() returns a valid pointer (success) with errno=0.

## POSIX requirement
POSIX.1-2017 (IEEE Std 1003.1-2017), mmap(3p), section ERRORS:
  "[EACCES] The fildes argument is not open for read, regardless of the protection specified, or fildes is not open for write and PROT_WRITE was specified for a MAP_SHARED type mapping."

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
	int fd, rdfd;

	memset(buf, 'A', sizeof(buf));
	fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0600);
	write(fd, buf, sizeof(buf));
	close(fd);

	rdfd = open(path, O_RDONLY);
	errno = 0;
	ptr = mmap(NULL, 4096, PROT_WRITE, MAP_SHARED, rdfd, 0);
	printf("PROT_WRITE on rdonly fd: ptr=%s, errno=%d (%s)\n",
		ptr == MAP_FAILED ? "MAP_FAILED" : "valid",
		errno, strerror(errno));
	if (ptr != MAP_FAILED)
		munmap(ptr, 4096);

	close(rdfd);
	unlink(path);
	return 0;
}
```

## Host behavior
```
PROT_WRITE on rdonly fd: ptr=MAP_FAILED, errno=13 (Permission denied)
```

## Revision
00ac1bc3dfbcfa6e2e6ac7e619910efc5d664d1c

## Target
Reproduced on ia32-generic-qemu, not tested on other targets yet.

## Related tests
- `phoenix-rtos-tests/libc/mman/mman_mmap.c`

These tests are currently skipped on the affected target. After applying a fix,
re-enable them and re-run the suite to verify the fix.
