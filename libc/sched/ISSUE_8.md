# sched_get_priority_max() and sched_get_priority_min() do not fail with EINVAL for invalid policy

## Category
POSIX_NONCOMPLIANCE

## Description
sched_get_priority_max() and sched_get_priority_min() do not return -1 and set errno to EINVAL when called with an invalid scheduling policy value. Instead, they return a valid-looking priority value (e.g. 22) without signaling an error.

## Expected behavior
Both functions shall return -1 and set errno to EINVAL when the policy argument does not represent a defined scheduling policy.

## Actual behavior
sched_get_priority_max(-1) returns 22 with errno unchanged (0).
sched_get_priority_min(-1) returns the same non-error value.

## POSIX requirement (if applicable)
POSIX.1-2017 (IEEE Std 1003.1-2017), sched_get_priority_max(3p), section ERRORS:
  [EINVAL] The value of the policy parameter does not represent a defined scheduling policy.

The standard uses "shall fail if" wording, making this a mandatory requirement.

## Minimal reproduction code

```c
#include <stdio.h>
#include <sched.h>
#include <errno.h>

int main(void)
{
	int ret;

	errno = 0;
	ret = sched_get_priority_max(-1);
	printf("sched_get_priority_max(-1): ret=%d, errno=%d\n", ret, errno);

	errno = 0;
	ret = sched_get_priority_min(-1);
	printf("sched_get_priority_min(-1): ret=%d, errno=%d\n", ret, errno);

	return 0;
}
```

## Host behavior
```
sched_get_priority_max(-1): ret=-1, errno=22
sched_get_priority_min(-1): ret=-1, errno=22
```

## Revision
00ac1bc3dfbcfa6e2e6ac7e619910efc5d664d1c

## Target
Reproduced on ia32-generic-qemu, not tested on other targets yet.

## Related tests
- `phoenix-rtos-tests/libc/sched/sched_get_priority.c`

These tests are currently skipped on the affected target. After applying a fix,
re-enable them and re-run the suite to verify the fix.
