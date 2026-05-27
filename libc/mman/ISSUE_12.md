# mmap() missing input validation for len and flags arguments

## Category
POSIX_NONCOMPLIANCE

## Description
mmap() does not properly validate its arguments. When called with len==0, it returns ENOMEM instead of EINVAL. When called with flags containing neither MAP_SHARED nor MAP_PRIVATE, it succeeds instead of failing with EINVAL.

## Expected behavior
- mmap() with len==0 shall fail and set errno to EINVAL.
- mmap() with flags lacking both MAP_SHARED and MAP_PRIVATE shall fail and set errno to EINVAL.

## Actual behavior
- len==0: returns MAP_FAILED with errno=ENOMEM (12) instead of EINVAL (22).
- flags=0: returns a valid pointer (success) instead of MAP_FAILED with errno=EINVAL.

## POSIX requirement
POSIX.1-2017 (IEEE Std 1003.1-2017), mmap(3p), section ERRORS:
  "[EINVAL] The value of len is zero."
  "[EINVAL] The value of flags is invalid (neither MAP_PRIVATE nor MAP_SHARED is set)."

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
	int fd;

	memset(buf, 'A', sizeof(buf));
	fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0600);
	write(fd, buf, sizeof(buf));

	errno = 0;
	ptr = mmap(NULL, 0, PROT_READ, MAP_PRIVATE, fd, 0);
	printf("len=0: ptr=%s, errno=%d (%s)\n",
		ptr == MAP_FAILED ? "MAP_FAILED" : "valid",
		errno, strerror(errno));

	errno = 0;
	ptr = mmap(NULL, 4096, PROT_READ, 0, fd, 0);
	printf("flags=0: ptr=%s, errno=%d (%s)\n",
		ptr == MAP_FAILED ? "MAP_FAILED" : "valid",
		errno, strerror(errno));
	if (ptr != MAP_FAILED)
		munmap(ptr, 4096);

	close(fd);
	unlink(path);
	return 0;
}
```

## Host behavior
```
len=0: ptr=MAP_FAILED, errno=22 (Invalid argument)
flags=0: ptr=MAP_FAILED, errno=22 (Invalid argument)
```

## Revision
00ac1bc3dfbcfa6e2e6ac7e619910efc5d664d1c

## Target
Reproduced on ia32-generic-qemu, not tested on other targets yet.

## Related tests (damianloew/posix_tests)
- `phoenix-rtos-tests/libc/mman/mman_mmap.c`

These tests are currently skipped on the affected target. After applying a fix,
re-enable them and re-run the suite to verify the fix.
