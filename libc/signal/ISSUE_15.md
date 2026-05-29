# killpg() does not fail with ESRCH for non-existent process group

## Category
POSIX_NONCOMPLIANCE

## Description
`killpg()` returns 0 (success) when called with a process group ID that does not exist on the system. It should return -1 and set `errno` to `ESRCH`.

## Expected behavior
`killpg(99999, SIGUSR1)` shall return -1 and set `errno` to `ESRCH` when process group 99999 does not exist.

## Actual behavior
`killpg(99999, SIGUSR1)` returns 0 with `errno` unchanged (0).

## POSIX requirement (if applicable)
POSIX.1-2017 (IEEE Std 1003.1-2017), `kill()`(3p), section ERRORS:
  "[ESRCH] No process or process group can be found corresponding to that specified by pid."

`killpg()`(3p), section DESCRIPTION:
  "If pgrp is greater than 1, killpg(pgrp, sig) shall be equivalent to kill(-pgrp, sig)."

## Minimal reproduction code

```c
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

int main(void)
{
	int ret;

	errno = 0;
	ret = killpg(99999, SIGUSR1);
	printf("killpg(99999, SIGUSR1) = %d, errno = %d (%s)\n",
	       ret, errno, strerror(errno));

	return 0;
}
```

## Host behavior
```
killpg(99999, SIGUSR1) = -1, errno = 3 (No such process)
```

## Revision
Discovered on 40a51fff499cba559f861a76144fc2ba6b58ef1e

## Target
Reproduced on ia32-generic-qemu, not tested on other targets yet.

## Related tests
- `phoenix-rtos-tests/libc/signal/signal_misc.c`

These tests are currently skipped on the affected target. After applying a fix,
re-enable them and re-run the suite to verify the fix.
