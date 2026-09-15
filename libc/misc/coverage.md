# Coverage: `umask()`

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "The umask() function shall set the file mode creation mask of the process to cmask" | `misc_umask.umask_returns_previous_value` | covered |
| "and return the previous value of the mask" | `misc_umask.umask_returns_previous_value` | covered |
| "Only the file permission bits of cmask are used" | `misc_umask.umask_only_permission_bits_used` | covered |
| "The file mode creation mask of the process is used to turn off permission bits in the mode argument supplied during calls to... open()" | `misc_umask.umask_affects_open_creat` | covered |
| "The file mode creation mask of the process is used to turn off permission bits in the mode argument supplied during calls to... mkdir()" | `misc_umask.umask_affects_mkdir` | covered |
| "The file mode creation mask of the process is used to turn off permission bits in the mode argument supplied during calls to... mkfifo()" | `misc_umask.umask_affects_mkfifo` | covered |
| "Bit positions that are set in cmask are cleared in the mode of the created file." | `misc_umask.umask_clears_bits_in_mode` | covered |
| "The file permission bits in the value returned by umask() shall be the previous value of the file mode creation mask." | `misc_umask.umask_returns_previous_value` | covered |
| "a subsequent call to umask() with the returned value as cmask shall leave the state of the mask the same as its state before the first call, including any unspecified use of those bits." | `misc_umask.umask_roundtrip_preserves_mask` | covered |
| "No errors are defined." | `misc_umask.umask_no_errors_defined` | covered |
| umask(0) allows all requested bits | `misc_umask.umask_affects_open_creat_zero_mask` | covered |
| umask(0777) clears all permission bits | `misc_umask.umask_set_all_permission_bits` | covered |
| Individual permission bits (S_IRUSR, S_IWUSR, etc.) each work independently | `misc_umask.umask_individual_bits` | covered |
| File mode creation mask is inherited by child processes across fork() | `misc_umask.umask_inherited_by_fork` | covered |

# Coverage: `setpgid()`

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall join an existing process group or create a new process group within the session of the calling process" | `unistd_setpgid.setpgid_joins_existing_group`, `unistd_setpgid.setpgid_creates_new_group_for_child` | covered |
| "The process group ID of a session leader shall not change." | `unistd_setpgid.setpgid_eperm_child_is_session_leader`, `unistd_setpgid.setpgid_eperm_self_is_session_leader` | covered |
| "If pid is 0, the process ID of the calling process shall be used" | `unistd_setpgid.setpgid_self_keeps_session` | covered |
| "if pgid is 0, the process ID of the indicated process shall be used" | `unistd_setpgid.setpgid_creates_new_group_for_child`, `unistd_setpgid.setpgid_self_keeps_session` | covered |
| "Upon successful completion, setpgid() shall return 0" | `unistd_setpgid.setpgid_creates_new_group_for_child` | covered |
| "otherwise, -1 shall be returned and errno set to indicate the error" | `unistd_setpgid.setpgid_einval_negative_pgid` | covered |
| [EACCES]: "The value of the pid argument matches the process ID of a child process of the calling process and the child process has successfully executed one of the exec functions" | `unistd_setpgid.setpgid_eacces_child_execd` | covered |
| [EINVAL]: "The value of the pgid argument is less than 0, or is not a value supported by the implementation" | `unistd_setpgid.setpgid_einval_negative_pgid` | covered |
| [EPERM]: "The process indicated by the pid argument is a session leader" | `unistd_setpgid.setpgid_eperm_child_is_session_leader`, `unistd_setpgid.setpgid_eperm_self_is_session_leader` | covered |
| [EPERM]: "The value of the pid argument matches the process ID of a child process of the calling process and the child process is not in the same session as the calling process" | `unistd_setpgid.setpgid_eperm_child_is_session_leader` | covered |
| [EPERM]: "The value of the pgid argument is valid but does not match the process ID of the process indicated by the pid argument and there is no process with a process group ID that matches the value of the pgid argument in the same session as the calling process" | `unistd_setpgid.setpgid_eperm_unused_pgid`, `unistd_setpgid.setpgid_eperm_pgid_in_other_session` | covered |
| [ESRCH]: "The value of the pid argument does not match the process ID of the calling process or of a child process of the calling process" | `unistd_setpgid.setpgid_esrch_unused_pid`, `unistd_setpgid.setpgid_esrch_not_own_child` | covered |
| A group change leaves the session ID untouched | `unistd_setpgid.setpgid_self_keeps_session`, `unistd_setpgid.setpgid_creates_new_group_for_child` | covered |
| Setting a process to the group it already belongs to is not an error | `unistd_setpgid.setpgid_idempotent_on_own_group` | covered |
| Moving a child does not change the caller's own group | `unistd_setpgid.setpgid_child_group_not_inherited_backwards` | covered |
| [EPERM]: the same rule reached by a child that is *not* a session leader - the caller left the session instead | `unistd_setpgid.setpgid_eperm_child_left_in_old_session` | covered |
| exec() preserves the process group and the session | `unistd_setpgid.exec_keeps_group_and_session` | covered |

# Coverage: `setsid()` (permission rules)

The happy path is covered by `libc/proc` (`proc_setsid`); the cases below cover
the [EPERM] conditions and cross-process visibility of the new session.

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| [EPERM]: "The calling process is already a process group leader" | `unistd_setsid.setsid_eperm_already_group_leader` | covered |
| [EPERM]: "...or the process group ID of a process other than the calling process matches the process ID of the calling process" | `unistd_setsid.setsid_eperm_pgid_of_other_process_matches_pid` | covered |
| "The calling process shall be the session leader of this new session, shall be the process group leader of a new process group" | `unistd_setsid.setsid_new_session_visible_to_parent` | covered |
| The new session ID is observable from other processes, not only from the caller | `unistd_setsid.setsid_new_session_visible_to_parent` | covered |
| Creating a session does not move any other process into it | `unistd_setsid.setsid_new_session_visible_to_parent` | covered |
| A process forked after setsid() belongs to the new session | `unistd_setsid.setsid_new_session_inherited_by_grandchild` | covered |
