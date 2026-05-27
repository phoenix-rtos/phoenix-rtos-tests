# mkstemp() creates file with wrong permissions (0666 instead of 0600)

## Category
POSIX_NONCOMPLIANCE

## Description
mkstemp() creates the temporary file with mode 0666 instead of the required S_IRUSR|S_IWUSR (0600). The implementation appears to pass an incorrect mode argument to the underlying open() call.

## Expected behavior
mkstemp() shall create the file as if by a call to:
    open(pathname, O_RDWR|O_CREAT|O_EXCL, S_IRUSR|S_IWUSR)
The resulting file permissions (before umask) shall be 0600.

## Actual behavior
The file is created with mode 0666 (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH). The umask is 0000 so the mode argument passed to open() is evidently wrong.

## POSIX requirement (if applicable)
POSIX.1-2017 (IEEE Std 1003.1-2017), mkstemp(3p), section DESCRIPTION:
  The mkstemp() function shall create a regular file with a unique name derived from template and return a file descriptor for the file open for reading and writing. [...] The mkstemp() function shall use the resulting pathname to create the file, and obtain a file descriptor for it, as if by a call to:
      open(pathname, O_RDWR|O_CREAT|O_EXCL, S_IRUSR|S_IWUSR)

## Minimal reproduction code

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

int main(void)
{
	char tmpl[] = "mkstemp_testXXXXXX";
	struct stat st;
	mode_t mask;
	int fd;

	mask = umask(0);
	umask(mask);
	printf("umask: %04o\n", (unsigned)mask);

	fd = mkstemp(tmpl);
	if (fd < 0) {
		printf("mkstemp failed: %s\n", strerror(errno));
		return 1;
	}

	if (fstat(fd, &st) < 0) {
		printf("fstat failed: %s\n", strerror(errno));
		close(fd);
		unlink(tmpl);
		return 1;
	}

	printf("mode: %04o (decimal %d)\n", (unsigned)(st.st_mode & 07777), (int)(st.st_mode & 07777));
	printf("expected: %04o (decimal %d)\n", (unsigned)(S_IRUSR | S_IWUSR), (int)(S_IRUSR | S_IWUSR));

	close(fd);
	unlink(tmpl);
	return 0;
}
```

## Host behavior
```
umask: 0002
mode: 0600 (decimal 384)
expected: 0600 (decimal 384)
```

## Revision
Discovered on 00ac1bc3dfbcfa6e2e6ac7e619910efc5d664d1c

## Target
Reproduced on ia32-generic-qemu, not tested on other targets yet.

## Related tests (damianloew/posix_tests)
- `phoenix-rtos-tests/libc/stdlib/stdlib_mktemp.c`

These tests are currently skipped on the affected target. After applying a fix,
re-enable them and re-run the suite to verify the fix.
