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
