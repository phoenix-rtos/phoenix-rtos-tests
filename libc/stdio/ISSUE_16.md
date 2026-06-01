# tmpnam() always returns NULL

## Category
BUG

## Description
tmpnam() always returns NULL regardless of the argument passed.

## Expected behavior
tmpnam() shall return a pointer to a string that is a valid pathname not naming an existing file. When called with NULL, it shall return a pointer to an internal static object. When called with a non-NULL buffer, it shall write the result into that buffer and return the argument as its value.

## Actual behavior
tmpnam(NULL) returns NULL. tmpnam(buf) returns NULL. The buffer remains unmodified.

## POSIX requirement (if applicable)
POSIX.1-2017 (IEEE Std 1003.1-2017), tmpnam(3p), section RETURN VALUE:
  Upon successful completion, tmpnam() shall return a pointer to a string.

## Minimal reproduction code

```c
#include <stdio.h>
#include <string.h>

int main(void)
{
	char *r1;
	char buf[L_tmpnam + 1];
	char *r2;

	r1 = tmpnam(NULL);
	if (r1 == NULL) {
		printf("tmpnam(NULL) returned NULL\n");
	}
	else {
		printf("tmpnam(NULL) = \"%s\"\n", r1);
	}

	memset(buf, 0, sizeof(buf));
	r2 = tmpnam(buf);
	if (r2 == NULL) {
		printf("tmpnam(buf) returned NULL\n");
	}
	else {
		printf("tmpnam(buf) = \"%s\"\n", r2);
	}

	return 0;
}
```

## Host behavior
```
tmpnam(NULL) = "/tmp/fileoCSATw"
tmpnam(buf) = "/tmp/fileseqQXG"
```

## Revision
Discovered on 40a51fff499cba559f861a76144fc2ba6b58ef1e

## Target
Reproduced on ia32-generic-qemu, not tested on other targets yet.

## Related tests
- `phoenix-rtos-tests/libc/stdio/stdio_misc.c`

These tests are currently skipped on the affected target. After applying a fix,
re-enable them and re-run the suite to verify the fix.
