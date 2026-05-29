# Coverage: proc module

## alarm()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall cause the system to generate a SIGALRM signal for the process after the number of realtime seconds specified by seconds have elapsed" | `proc_alarm.alarm_generates_sigalrm` | covered |
| "If there is a previous alarm() request with time remaining, alarm() shall return a non-zero value that is the number of seconds until the previous request would have generated a SIGALRM signal" | `proc_alarm.alarm_returns_remaining_time` | covered |
| "If seconds is 0, a pending alarm request, if any, is canceled" | `proc_alarm.alarm_cancel_with_zero` | covered |
| "Alarm requests are not stacked; only one SIGALRM generation can be scheduled in this manner. If the SIGALRM signal has not yet been generated, the call shall result in rescheduling the time" | `proc_alarm.alarm_reschedule_replaces_previous` | covered |
| "If there is no previous alarm() request ... the return value is 0" | `proc_alarm.alarm_returns_zero_no_pending` | covered |

## pause()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall suspend the calling thread until delivery of a signal whose action is either to execute a signal-catching function or to terminate the process" | `proc_pause.pause_returns_eintr_on_signal` | covered |
| "Since pause() suspends thread execution indefinitely unless interrupted by a signal, there is no successful completion return value. A value of -1 shall be returned and errno set to [EINTR]" | `proc_pause.pause_returns_eintr_on_signal` | covered |
| [EINTR]: "A signal is caught by the calling process and control is returned from the signal-catching function" | `proc_pause.pause_returns_eintr_on_signal` | covered |

## sleep()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall cause the calling thread to be suspended from execution until either the number of realtime seconds specified by the argument seconds has elapsed or a signal is delivered" | `proc_sleep.sleep_returns_zero_on_completion` | covered |
| "If ... sleep() returns because the requested time has elapsed, the return value shall be 0" | `proc_sleep.sleep_returns_zero_on_completion` | covered |
| "If seconds is 0, a minimum interval shall elapse" (return immediately) | `proc_sleep.sleep_zero_returns_immediately` | covered |
| "If sleep() returns because of ... a signal, the return value shall be the 'unslept' amount (the requested time minus the time actually slept)" | `proc_sleep.sleep_interrupted_returns_unslept` | covered |

## nice()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall add the value of incr to the nice value of the calling process" | `proc_nice.nice_increase_priority_value` | covered |
| "A nice value of a process is a non-negative number for which a more positive value shall result in less favorable scheduling" | `proc_nice.nice_increase_priority_value` | covered |
| "If the resulting nice value ... exceeds the implementation limit, nice() shall set it to the limit" | `proc_nice.nice_clamps_to_maximum` | covered |
| "Upon successful completion, nice() shall return the new nice value minus {NZERO}" | `proc_nice.nice_zero_returns_current` | covered |
| "Otherwise, -1 shall be returned, the nice value of the process shall not be changed, and errno shall be set" | `proc_nice.nice_eperm_decrease_unprivileged` | covered |
| [EPERM]: "The incr argument is negative and the calling process does not have appropriate privileges" | `proc_nice.nice_eperm_decrease_unprivileged` | covered |

## setsid()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall create a new session, if the calling process is not a process group leader" | `proc_setsid.setsid_creates_new_session_in_child` | covered |
| "The calling process shall be the session leader of this new session, shall be the process group leader of a new process group, and shall have no controlling terminal" | `proc_setsid.setsid_creates_new_session_in_child` | covered |
| "The process group ID of the calling process shall be set equal to the process ID of the calling process" | `proc_setsid.setsid_returns_new_pgid` | covered |
| "Upon successful completion, setsid() shall return the value of the new process group ID" | `proc_setsid.setsid_returns_new_pgid` | covered |
| "Otherwise, setsid() shall return -1 and set errno" | `proc_setsid.setsid_eperm_process_group_leader` | covered |
| [EPERM]: "The calling process is already a process group leader, or the process group ID of a process other than the calling process matches the process ID of the calling process" | `proc_setsid.setsid_eperm_process_group_leader` | covered |

## times()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall fill the tms structure pointed to by buffer with time-accounting information" | `proc_times.times_fills_structure` | covered |
| "tms_utime: the CPU time charged for the execution of user instructions of the calling process" | `proc_times.times_user_time_increases_with_work` | covered |
| "tms_stime: the CPU time charged for execution by the system on behalf of the calling process" | `proc_times.times_fills_structure` | covered |
| "tms_cutime: the sum of the tms_utime and the tms_cutime of the child processes" | `proc_times.times_child_times_from_waited_child` | covered |
| "tms_cstime: the sum of the tms_stime and the tms_cstime of the child processes" | `proc_times.times_child_times_from_waited_child` | covered |
| "Upon successful completion, times() shall return the elapsed real time, in clock ticks, since an arbitrary point in the past" | `proc_times.times_return_value_increases` | covered |
| Return value increases across calls | `proc_times.times_return_value_increases` | covered |

## fork()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall create a new process ... a copy of the calling process" | `proc_fork.fork_returns_zero_to_child` | covered |
| "The return value shall be 0 to the child process" | `proc_fork.fork_returns_zero_to_child` | covered |
| "the process ID of the child process to the parent process" | `proc_fork.fork_returns_child_pid_to_parent` | covered |
| "The child process shall have a unique process ID" | `proc_fork.fork_child_has_unique_pid` | covered |
| "The parent process ID of the child process shall be set to the process ID of the parent process" (i.e., different ppid) | `proc_fork.fork_child_has_different_parent_pid` | covered |
| "The child process shall have its own copy of the parent's file descriptors" | `proc_fork.fork_child_inherits_fd` | covered |
| "The set of signals pending for the child process shall be initialized to the empty set" | `proc_fork.fork_child_pending_signals_empty` | covered |
| "Timers created by timer_create() shall not be inherited by the child process" / "alarm() value reset" | `proc_fork.fork_child_alarm_cleared` | covered |

## waitid()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall suspend the calling thread until one child of the process ... changes state" | `proc_waitid.waitid_wexited_normal` | covered |
| "P_PID ... waitid() shall wait for the child with a process ID equal to (pid_t)id" | `proc_waitid.waitid_wexited_normal` | covered |
| "P_PGID ... waitid() shall wait for any child with a process group ID equal to (pid_t)id" | `proc_waitid.waitid_p_pgid` | covered |
| "WEXITED: Wait for processes that have exited" | `proc_waitid.waitid_wexited_normal` | covered |
| "WNOHANG: Do not hang if no status is available" | `proc_waitid.waitid_wnohang_no_child_ready` | covered |
| "si_signo member ... set to SIGCHLD" (on normal exit) | `proc_waitid.waitid_wexited_normal` | covered |
| "si_code ... CLD_EXITED if the child called _exit() or exit()" | `proc_waitid.waitid_wexited_normal` | covered |
| "si_code ... CLD_KILLED if the child was killed with a signal" | `proc_waitid.waitid_wexited_signal_killed` | covered |
| "si_status ... the exit value or signal number" | `proc_waitid.waitid_wexited_normal` | covered |
| [ECHILD]: "The calling process has no existing unwaited-for child processes" | `proc_waitid.waitid_echild_no_children` | covered |
| [EINVAL]: "An invalid value was specified for options (no WEXITED/WSTOPPED/WCONTINUED)" | `proc_waitid.waitid_einval_no_flags` | covered |

## fexecve()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall be equivalent to execve() except that the file to be executed is determined by the file descriptor fd instead of a pathname" | `proc_fexecve.fexecve_executes_program` | covered |
| "shall replace the current process image with a new process image" | `proc_fexecve.fexecve_executes_program` | covered |
| "The argument envp is an array of character pointers to null-terminated strings. These strings shall constitute the environment for the new process image" | `proc_fexecve.fexecve_passes_environment` | covered |
| Exit status of new process image is visible to parent | `proc_fexecve.fexecve_exit_code_nonzero` | covered |
| "File descriptors open in the calling process image shall remain open ... except for those whose close-on-exec flag FD_CLOEXEC is set" | `proc_fexecve.fexecve_cloexec_fd_closed` | covered |
| "There shall be no return from a successful exec" (on failure, returns -1) | `proc_fexecve.fexecve_ebadf` | covered |
| [EBADF]: "fd is not a valid file descriptor" | `proc_fexecve.fexecve_ebadf` | covered |
| [EACCES]: "The new process image file is not a regular file and the implementation does not support execution of files of its type" / file is not executable | `proc_fexecve.fexecve_eacces_not_executable` | covered |
