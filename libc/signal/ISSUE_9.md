# sigaction() does not strip SIGKILL and SIGSTOP from sa_mask during handler execution

## Category
POSIX_NONCOMPLIANCE

## Description
When `sigaction()` is called with `sa_mask` containing SIGKILL and SIGSTOP (e.g. via `sigfillset()`), the system does not silently remove these signals from the effective mask during signal handler execution. Instead, SIGKILL and SIGSTOP appear as blocked in the handler's signal mask obtained via `sigprocmask()`.

## Expected behavior
SIGKILL and SIGSTOP shall not appear in the thread's signal mask during handler execution, even if they were included in `sa_mask`. The system shall silently enforce this restriction without indicating an error.

## Actual behavior
`sigismember(&mask, SIGKILL)` returns 1 and `sigismember(&mask, SIGSTOP)` returns 1 when queried inside the signal handler.

## POSIX requirement
POSIX.1-2017 (IEEE Std 1003.1-2017), `sigaction()`(3p), section DESCRIPTION:
  "The SIGKILL and SIGSTOP signals shall not be added to the signal mask using this mechanism; this restriction shall be enforced by the system without causing an error to be indicated."

## Minimal reproduction code

```c
#include <stdio.h>
#include <signal.h>
#include <string.h>

static volatile sig_atomic_t handler_called;
static sigset_t handler_mask;

static void handler(int signo)
{
	(void)signo;
	sigprocmask(SIG_SETMASK, NULL, &handler_mask);
	handler_called = 1;
}

int main(void)
{
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handler;
	sigfillset(&sa.sa_mask);
	sa.sa_flags = 0;

	sigaction(SIGUSR1, &sa, NULL);
	raise(SIGUSR1);

	printf("handler_called=%d\n", handler_called);
	printf("SIGKILL in mask: %d\n", sigismember(&handler_mask, SIGKILL));
	printf("SIGSTOP in mask: %d\n", sigismember(&handler_mask, SIGSTOP));

	return 0;
}
```

## Host behavior
```
handler_called=1
SIGKILL in mask: 0
SIGSTOP in mask: 0
```

## Revision
Discovered on 00ac1bc3dfbcfa6e2e6ac7e619910efc5d664d1c

## Target
Reproduced on ia32-generic-qemu, not tested on other targets yet.

## Related tests (damianloew/posix_tests)
- `phoenix-rtos-tests/libc/signal/sigaction.c`

These tests are currently skipped on the affected target. After applying a fix,
re-enable them and re-run the suite to verify the fix.
