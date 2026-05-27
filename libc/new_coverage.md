# Test Coverage — New Modules

## pwd.h — endpwent(), getpwent(), getpwnam_r(), getpwuid_r(), setpwent()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "getpwent() shall return a pointer to a structure containing the broken-out fields of an entry in the user database" | `pwd_getpwent.getpwent_returns_first_entry` | covered |
| "Thereafter, it shall return a pointer to a passwd structure containing the next entry in the user database" | `pwd_getpwent.getpwent_iterates_multiple_entries` | covered |
| "If an end-of-file or an error is encountered on reading, getpwent() shall return a null pointer" | `pwd_getpwent.getpwent_returns_null_at_end` | covered |
| "setpwent() shall rewind the user database so that the next getpwent() call returns the first entry" | `pwd_getpwent.setpwent_rewinds_database` | covered |
| "endpwent() shall close the user database" | `pwd_getpwent.endpwent_allows_fresh_iteration` | covered |
| "setpwent() and endpwent() shall not change the setting of errno if successful" | `pwd_getpwent.setpwent_no_errno_on_success` | covered |
| "getpwnam_r() shall return zero on success or if the requested entry was not found" | `pwd_getpwnam_r.getpwnam_r_finds_root` | covered |
| "A null pointer shall be returned at the location pointed to by result on error or if the requested entry is not found" | `pwd_getpwnam_r.getpwnam_r_not_found` | covered |
| "[ERANGE] Insufficient storage was supplied via buffer and bufsize" | `pwd_getpwnam_r.getpwnam_r_buffer_too_small` | covered |
| "getpwuid_r() shall return zero on success" | `pwd_getpwuid_r.getpwuid_r_finds_root` | covered |
| "A null pointer shall be returned at the location pointed to by result if the requested entry is not found" | `pwd_getpwuid_r.getpwuid_r_not_found` | covered |
| "[ERANGE] Insufficient storage was supplied via buffer and bufsize" (getpwuid_r) | `pwd_getpwuid_r.getpwuid_r_buffer_too_small` | covered |

## sched.h — sched_getparam(), sched_getscheduler(), sched_rr_get_interval(), sched_setparam(), sched_setscheduler()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "sched_getparam() shall return the scheduling parameters of a process specified by pid" | `sched_param.getparam_self_pid_zero` | covered |
| "If pid is zero, the scheduling parameters for the calling process shall be returned" | `sched_param.getparam_self_own_pid` | covered |
| "[ESRCH] No process can be found corresponding to that specified by pid" (getparam) | `sched_param.getparam_esrch_invalid_pid` | covered |
| "sched_setparam() shall set the scheduling parameters" | `sched_param.setparam_self` | covered |
| "[EINVAL] One or more of the requested scheduling parameters is outside the range" | `sched_param.setparam_einval_out_of_range` | covered |
| "[ESRCH] No process can be found corresponding to that specified by pid" (setparam) | `sched_param.setparam_esrch_invalid_pid` | covered |
| "sched_getscheduler() shall return the scheduling policy of the process specified by pid" | `sched_scheduler.getscheduler_self_pid_zero` | covered |
| "If pid is zero, the scheduling policy shall be returned for the calling process" | `sched_scheduler.getscheduler_self_own_pid` | covered |
| "[ESRCH] No process can be found corresponding to that specified by pid" (getscheduler) | `sched_scheduler.getscheduler_esrch_invalid_pid` | covered |
| "Upon successful completion, the function shall return the former scheduling policy" | `sched_scheduler.setscheduler_returns_former_policy` | covered |
| "[EINVAL] The value of the policy parameter is invalid" | `sched_scheduler.setscheduler_einval_invalid_policy` | covered |
| "[EINVAL] one or more of the parameters contained in param is outside the valid range" | `sched_scheduler.setscheduler_einval_invalid_priority` | covered |
| "[ESRCH] No process can be found corresponding to that specified by pid" (setscheduler) | `sched_scheduler.setscheduler_esrch_invalid_pid` | covered |
| "sched_rr_get_interval() shall update the timespec structure to contain the current execution time limit" | `sched_rr_get_interval.self_pid_zero` | covered |
| "[ESRCH] No process can be found corresponding to that specified by pid" (rr_get_interval) | `sched_rr_get_interval.esrch_invalid_pid` | covered |

## semaphore.h — sem_init(), sem_destroy(), sem_post(), sem_wait(), sem_trywait(), sem_timedwait(), sem_getvalue(), sem_open(), sem_close(), sem_unlink()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "sem_init() shall initialize the unnamed semaphore referred to by sem" | `sem_unnamed.init_zero_value` | covered |
| "The value of the initialized semaphore shall be value" | `sem_unnamed.init_positive_value` | covered |
| "[EINVAL] The value argument exceeds {SEM_VALUE_MAX}" | `sem_unnamed.init_einval_exceeds_max` | covered |
| "sem_destroy() shall destroy the unnamed semaphore" | `sem_unnamed.destroy_initialized` | covered |
| "sem_post() shall unlock the semaphore by performing a semaphore unlock operation" | `sem_unnamed.post_increments_value` | covered |
| "sem_wait() shall lock the semaphore referenced by sem" | `sem_unnamed.wait_decrements_value` | covered |
| "sem_trywait() shall lock the semaphore only if the semaphore is currently not locked" | `sem_unnamed.trywait_success` | covered |
| "[EAGAIN] The semaphore was already locked, so it cannot be immediately locked" | `sem_unnamed.trywait_eagain_when_locked` | covered |
| "sem_timedwait() shall lock the semaphore... this wait shall be terminated when the specified timeout expires" | `sem_unnamed.timedwait_success_immediate` | covered |
| "[ETIMEDOUT] The semaphore could not be locked before the specified timeout expired" | `sem_unnamed.timedwait_etimedout` | covered |
| "[EINVAL] the abstime parameter specified a nanoseconds field value less than zero or >= 1000 million" | `sem_unnamed.timedwait_einval_bad_nsec` | covered |
| "sem_getvalue() shall update the location referenced by sval to have the value of the semaphore" | `sem_unnamed.getvalue_returns_current` | covered |
| "If sem is locked, then sval shall be set to zero or negative" | `sem_unnamed.getvalue_zero_when_locked` | covered |
| "sem_open() shall establish a connection between a named semaphore and a process" | `sem_named.open_create_new` | covered |
| "The semaphore is created with an initial value of value" | `sem_named.open_create_initial_value` | covered |
| "[EEXIST] O_CREAT and O_EXCL are set and the named semaphore already exists" | `sem_named.open_eexist_with_excl` | covered |
| "[ENOENT] O_CREAT is not set and the named semaphore does not exist" | `sem_named.open_enoent_no_create` | covered |
| "[EINVAL] O_CREAT was specified and value was greater than {SEM_VALUE_MAX}" | `sem_named.open_einval_value_exceeds_max` | covered |
| "sem_close() shall indicate that the calling process is finished using the named semaphore" | `sem_named.close_success` | covered |
| "sem_unlink() shall remove the semaphore named by the string name" | `sem_named.unlink_success` | covered |
| "[ENOENT] The named semaphore does not exist" (sem_unlink) | `sem_named.unlink_enoent_nonexistent` | covered |
| "Calls to sem_open() to recreate or reconnect refer to a new semaphore after sem_unlink()" | `sem_named.unlink_removes_name` | covered |
| "same name shall return same semaphore address" | `sem_named.open_same_name_returns_same_address` | covered |

## signal.h — killpg(), raise(), sigaltstack(), signal(), sigpending(), sigqueue(), sigtimedwait(), sigwait(), sigwaitinfo()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "raise() shall send the signal sig to the executing thread or process" | `signal_raise.raise_delivers_signal` | covered |
| "raise() shall not return until after the signal handler does" | `signal_raise.raise_returns_before_handler_done` | covered |
| "[EINVAL] The value of the sig argument is an invalid signal number" (raise) | `signal_raise.raise_einval_invalid_signal` | covered |
| "signal() chooses one of three ways in which receipt of the signal is to be handled" | `signal_raise.signal_set_handler` | covered |
| "If the value of func is SIG_IGN, the signal shall be ignored" | `signal_raise.signal_sig_ign` | covered |
| "signal() shall return the value of func for the most recent call" | `signal_raise.signal_returns_previous_handler` | covered |
| "[EINVAL] sig is not a valid signal number or attempt to catch uncatchable signal" | `signal_raise.signal_einval_invalid_signal` | covered |
| "sigpending() shall store the set of signals that are blocked and pending" | `signal_pending.sigpending_shows_blocked_pending` | covered |
| "sigwait() shall select a pending signal, atomically clear it, and return that signal number" | `signal_pending.sigwait_consumes_pending` | covered |
| "sigwaitinfo() returns selected signal number and fills siginfo_t" | `signal_pending.sigwaitinfo_returns_signal_number` | covered |
| "sigtimedwait() immediate return when signal pending" | `signal_pending.sigtimedwait_immediate_pending` | covered |
| "[EAGAIN] No signal was generated within the specified timeout period" | `signal_pending.sigtimedwait_eagain_timeout` | covered |
| "[EINVAL] tv_nsec value less than zero or >= 1000 million" (sigtimedwait) | `signal_pending.sigtimedwait_einval_bad_nsec` | covered |
| "sigqueue() shall cause the signal to be sent with the value specified" | `signal_pending.sigqueue_delivers_signal` | covered |
| "[EINVAL] signo is an invalid or unsupported signal number" (sigqueue) | `signal_pending.sigqueue_einval_invalid_signal` | covered |
| "[ESRCH] The process pid does not exist" (sigqueue) | `signal_pending.sigqueue_esrch_invalid_pid` | covered |
| "killpg(pgrp, sig) shall be equivalent to kill(-pgrp, sig)" | `signal_pending.killpg_delivers_to_group` | covered |
| "[EINVAL] sig is invalid" (killpg) | `signal_pending.killpg_einval_invalid_signal` | covered |
| "[ESRCH] No process group found" (killpg) | `signal_pending.killpg_esrch_invalid_pgrp` | covered |
| "sigaltstack() allows defining alternate stack for signal handlers" | `signal_pending.sigaltstack_set_and_get` | covered |
| "SS_DISABLE disables the stack" | `signal_pending.sigaltstack_disable` | covered |
| "[EINVAL] ss_flags contains flags other than SS_DISABLE" | `signal_pending.sigaltstack_einval_bad_flags` | covered |
| "[ENOMEM] The size of the alternate stack area is less than MINSIGSTKSZ" | `signal_pending.sigaltstack_enomem_too_small` | covered |
| "[EPERM] An attempt was made to modify an active stack" | — | not tested: cannot reliably trigger from within signal handler in test harness |

## stdio.h — getc_unlocked(), putc_unlocked(), renameat(), tempnam(), tmpnam(), vdprintf()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "getc_unlocked() shall be functionally equivalent to getc()" | `stdio_unlocked.getc_unlocked_reads_chars` | covered |
| "putc_unlocked() shall be functionally equivalent to putc()" | `stdio_unlocked.putc_unlocked_writes_chars` | covered |
| "EOF at end-of-file" (getc_unlocked) | `stdio_unlocked.getc_unlocked_eof_sets_indicator` | covered |
| "renameat() shall be equivalent to rename() except relative paths resolved relative to fd" | `stdio_renameat.renameat_at_fdcwd` | covered |
| "If AT_FDCWD, current working directory shall be used" | `stdio_renameat.renameat_with_fd` | covered |
| "If link named by new exists, it shall be removed and old renamed" | `stdio_renameat.renameat_overwrites_existing` | covered |
| "[ENOENT] The link named by old does not name an existing file" | `stdio_renameat.renameat_enoent_source_missing` | covered |
| "tmpnam() shall generate a string that is a valid pathname" | `stdio_tmpnam.tmpnam_null_returns_string` | covered |
| "If s is not null, tmpnam() shall write result in that array and return the argument" | `stdio_tmpnam.tmpnam_with_buffer` | covered |
| "tmpnam() generates a different string each time it is called" | `stdio_tmpnam.tmpnam_unique_names` | covered |
| "tempnam() shall allocate space for a string and return a pointer to it" | `stdio_tmpnam.tempnam_basic` | covered |
| "dir argument points to the directory name" | `stdio_tmpnam.tempnam_with_dir` | covered |
| "pfx argument used as beginning of filename" | `stdio_tmpnam.tempnam_with_prefix` | covered |
| "vdprintf() shall be equivalent to dprintf() except called with va_list" | `stdio_vdprintf.basic_string` | covered |
| "returns number of bytes transmitted" | `stdio_vdprintf.returns_char_count` | covered |

## stdlib.h — system()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "If command is NULL, system() shall return non-zero" | `stdlib_system.null_command_returns_nonzero` | covered |
| "system() shall return the termination status in the format specified by waitpid()" | `stdlib_system.true_command_success` | covered |
| "nonzero exit status preserved" | `stdlib_system.false_command_nonzero_exit` | covered |
| "exit code preserved through waitpid format" | `stdlib_system.exit_code_preserved` | covered |
| "if the command language interpreter cannot execute, return as if exit(127)" | `stdlib_system.nonexistent_command_exit_127` | covered |

## sys/resource.h — getpriority(), getrlimit(), getrusage(), setpriority(), setrlimit()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "getpriority() shall obtain the nice value" (PRIO_PROCESS) | `resource_priority.getpriority_self_process` | covered |
| "getpriority() with PRIO_PGRP" | `resource_priority.getpriority_self_pgrp` | covered |
| "getpriority() with PRIO_USER" | `resource_priority.getpriority_self_user` | covered |
| "[EINVAL] which argument was not recognized" (getpriority) | `resource_priority.getpriority_einval_bad_which` | covered |
| "[ESRCH] No process could be located" (getpriority) | `resource_priority.getpriority_esrch_invalid_pid` | covered |
| "setpriority() shall set the nice value" | `resource_priority.setpriority_self_process` | covered |
| "[EINVAL] which argument was not recognized" (setpriority) | `resource_priority.setpriority_einval_bad_which` | covered |
| "[ESRCH] No process could be located" (setpriority) | `resource_priority.setpriority_esrch_invalid_pid` | covered |
| "getrlimit() shall get limits on the consumption of resources" (RLIMIT_NOFILE) | `resource_rlimit.getrlimit_nofile` | covered |
| "RLIMIT_STACK" | `resource_rlimit.getrlimit_stack` | covered |
| "RLIMIT_CORE" | `resource_rlimit.getrlimit_core` | covered |
| "RLIMIT_CPU" | `resource_rlimit.getrlimit_cpu` | covered |
| "RLIMIT_DATA" | `resource_rlimit.getrlimit_data` | covered |
| "RLIMIT_FSIZE" | `resource_rlimit.getrlimit_fsize` | covered |
| "RLIMIT_AS" | `resource_rlimit.getrlimit_as` | covered |
| "[EINVAL] An invalid resource was specified" (getrlimit) | `resource_rlimit.getrlimit_einval_invalid_resource` | covered |
| "setrlimit() shall set limits" | `resource_rlimit.setrlimit_lower_soft` | covered |
| "[EINVAL] rlim_cur exceeds rlim_max" | `resource_rlimit.setrlimit_einval_cur_exceeds_max` | covered |
| "[EINVAL] An invalid resource was specified" (setrlimit) | `resource_rlimit.setrlimit_einval_invalid_resource` | covered |
| "[EPERM] limit would have raised maximum without appropriate privileges" | `resource_rlimit.setrlimit_eperm_raise_hard` | covered |
| "getrusage() shall provide measures of resources used" (RUSAGE_SELF) | `resource_rusage.getrusage_self` | covered |
| "RUSAGE_CHILDREN information about terminated and waited-for children" | `resource_rusage.getrusage_children` | covered |
| "[EINVAL] The value of who is not valid" | `resource_rusage.getrusage_einval_bad_who` | covered |

## Functions not tested

| Function | Reason |
|---|---|
| at_quick_exit() | Not in POSIX.1-2017 (no spec available) |
| quick_exit() | Not in POSIX.1-2017 (no spec available) |
| sem_clockwait() | Not in POSIX.1-2017 (no spec available) |
| fattach() | STREAMS obsolescent extension [OB XSR], not implemented on Linux/glibc |
| fdetach() | STREAMS obsolescent extension [OB XSR], not implemented on Linux/glibc |
| isastream() | STREAMS obsolescent extension [OB XSR], not implemented on Linux/glibc |
| gets() | Obsolescent [OB], undefined behavior on overflow; fgets already tested |
| fgets() | Already tested in existing stdio_file.c |
| getchar_unlocked() | Requires stdin redirection not portably available in test harness |
| putchar_unlocked() | Requires stdout capture not portably available in test harness |
