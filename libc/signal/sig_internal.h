/*
 * Phoenix-RTOS
 *
 * libc-tests
 *
 * Testing POSIX signals.
 *
 * Copyright 2025 Phoenix Systems
 * Author: Marek Bialowas
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */
#ifndef _LIBC_TEST_SIG_INTERNAL_H
#define _LIBC_TEST_SIG_INTERNAL_H

#include <signal.h>
#include <stdbool.h>
#ifdef __linux__
/* on linux signals 32 and 33 are used internally by glibc NTPL implementation and can't be blocked
 * also: don't test RT signals for now */
#define USERSPACE_NSIG 32
#else
#define USERSPACE_NSIG NSIG
#endif

static inline bool signal_is_unblockable(int sig)
{
	/* POSIX: some signals might be unblockable and they should be silently ignored in signal masks */
	return ((sig == SIGKILL) || (sig == SIGSTOP));
}

#endif /* _LIBC_TEST_SIG_INTERNAL_H */
