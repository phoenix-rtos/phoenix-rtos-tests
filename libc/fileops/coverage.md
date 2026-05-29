# Coverage: fileops module

## fchmod()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall be equivalent to chmod() except that the file whose permissions are changed is specified by the file descriptor fildes" | `fileops_fchmod.fchmod_set_permissions` | covered |
| "The file permission bits of the file ... shall be modified" | `fileops_fchmod.fchmod_set_permissions` | covered |
| Can set read-only permissions | `fileops_fchmod.fchmod_readonly` | covered |
| Can remove all permissions | `fileops_fchmod.fchmod_remove_all_permissions` | covered |
| "Upon successful completion, fchmod() shall return 0" | `fileops_fchmod.fchmod_returns_zero_on_success` | covered |
| "Otherwise, it shall return -1 and set errno" | `fileops_fchmod.fchmod_ebadf` | covered |
| [EBADF]: "The fildes argument is not an open file descriptor" | `fileops_fchmod.fchmod_ebadf` | covered |
| [EPERM]: "The effective user ID does not match the owner of the file and the process does not have appropriate privileges" | — | not tested: test runs as file owner |

## fchmodat()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall be equivalent to chmod() ... except ... the path argument is resolved relative to the directory associated with the file descriptor fd" | `fileops_fchmodat.fchmodat_relative_path` | covered |
| "If the path argument is absolute, the fd argument shall be ignored" | `fileops_fchmodat.fchmodat_absolute_path` | covered |
| "If fd has the special value AT_FDCWD ... relative path resolved from current working directory" | `fileops_fchmodat.fchmodat_at_fdcwd` | covered |
| "Upon successful completion ... shall return 0" | `fileops_fchmodat.fchmodat_relative_path` | covered |
| "Otherwise ... -1 shall be returned and errno set" | `fileops_fchmodat.fchmodat_enoent` | covered |
| [ENOENT]: "A component of path does not name an existing file" | `fileops_fchmodat.fchmodat_enoent` | covered |
| [EBADF]: "path does not specify an absolute path and fd is neither AT_FDCWD nor a valid file descriptor" | `fileops_fchmodat.fchmodat_ebadf_invalid_fd` | covered |
| [ENOTDIR]: "path is relative and fd is a file descriptor ... that does not refer to a directory" | `fileops_fchmodat.fchmodat_enotdir_fd_not_dir` | covered |

## fchdir()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall be equivalent to chdir() except that the directory that is to be the new current working directory is specified by the file descriptor fildes" | `fileops_fchdir.fchdir_changes_cwd` | covered |
| "Upon successful completion, fchdir() shall return 0" | `fileops_fchdir.fchdir_returns_zero_on_success` | covered |
| "Otherwise, -1 shall be returned and errno set to indicate the error" | `fileops_fchdir.fchdir_ebadf` | covered |
| [EBADF]: "The fildes argument is not an open file descriptor" | `fileops_fchdir.fchdir_ebadf` | covered |
| [ENOTDIR]: "The open file descriptor fildes does not refer to a directory" | `fileops_fchdir.fchdir_enotdir` | covered |

## fstatat()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall be equivalent to stat() ... except ... the path argument is resolved relative to the directory associated with the file descriptor fd" | `fileops_fstatat.fstatat_relative_path` | covered |
| "If fd has the special value AT_FDCWD ... relative path resolved from current working directory" | `fileops_fstatat.fstatat_at_fdcwd` | covered |
| "If the path argument is absolute, the fd argument shall be ignored" | `fileops_fstatat.fstatat_absolute_path_with_fd` | covered |
| "If AT_SYMLINK_NOFOLLOW is set in flag ... status information for the symbolic link itself" | `fileops_fstatat.fstatat_at_symlink_nofollow` | covered |
| "Upon successful completion, 0 shall be returned" | `fileops_fstatat.fstatat_relative_path` | covered |
| "Otherwise, -1 shall be returned and errno set" | `fileops_fstatat.fstatat_enoent` | covered |
| [ENOENT]: "A component of path does not name an existing file" | `fileops_fstatat.fstatat_enoent` | covered |
| [EBADF]: "path does not specify an absolute path and fd is neither AT_FDCWD nor a valid file descriptor" | `fileops_fstatat.fstatat_ebadf` | covered |
| [ENOTDIR]: "path is relative and fd is a file descriptor ... not a directory" | `fileops_fstatat.fstatat_enotdir_fd_not_dir` | covered |

## mkdirat()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall be equivalent to mkdir() ... except ... the path argument is resolved relative to the directory associated with the file descriptor fd" | `fileops_mkdirat.mkdirat_creates_directory` | covered |
| "The file permission bits of the new directory shall be initialized from mode" | `fileops_mkdirat.mkdirat_permissions_applied` | covered |
| "If fd has the special value AT_FDCWD ... relative path resolved from current working directory" | `fileops_mkdirat.mkdirat_at_fdcwd` | covered |
| "Upon successful completion ... shall return 0" | `fileops_mkdirat.mkdirat_creates_directory` | covered |
| "Otherwise ... -1 shall be returned and errno set" | `fileops_mkdirat.mkdirat_eexist` | covered |
| [EEXIST]: "The named file exists" | `fileops_mkdirat.mkdirat_eexist` | covered |
| [ENOENT]: "A component of the path prefix ... does not name an existing directory" | `fileops_mkdirat.mkdirat_enoent_missing_component` | covered |
| [EBADF]: "path does not specify an absolute path and fd is neither AT_FDCWD nor a valid file descriptor" | `fileops_mkdirat.mkdirat_ebadf` | covered |

## mkfifoat()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall be equivalent to mkfifo() ... except ... the path argument is resolved relative to the directory associated with the file descriptor fd" | `fileops_mkfifoat.mkfifoat_creates_fifo` | covered |
| "The file permission bits of the new FIFO shall be initialized from mode" | `fileops_mkfifoat.mkfifoat_permissions_applied` | covered |
| "If fd has the special value AT_FDCWD ... relative path resolved from current working directory" | `fileops_mkfifoat.mkfifoat_at_fdcwd` | covered |
| "Upon successful completion ... shall return 0" | `fileops_mkfifoat.mkfifoat_creates_fifo` | covered |
| "Otherwise ... -1 shall be returned and errno set" | `fileops_mkfifoat.mkfifoat_eexist` | covered |
| [EEXIST]: "The named file already exists" | `fileops_mkfifoat.mkfifoat_eexist` | covered |
| [ENOENT]: "A component of the path ... does not name an existing file" | `fileops_mkfifoat.mkfifoat_enoent` | covered |
| [EBADF]: "path does not specify an absolute path and fd is neither AT_FDCWD nor a valid file descriptor" | `fileops_mkfifoat.mkfifoat_ebadf` | covered |

## futimens()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall set the access and modification times of the file ... the file is specified by a file descriptor" | `fileops_futimens.futimens_set_specific_times` | covered |
| "If times is a null pointer, the access and modification times ... shall be set to the current time" | `fileops_futimens.futimens_null_sets_current_time` | covered |
| "If the tv_nsec field ... has the special value UTIME_NOW, the corresponding timestamp shall be set to the current time" | `fileops_futimens.futimens_utime_now` | covered |
| "If the tv_nsec field ... has the special value UTIME_OMIT, the corresponding timestamp shall be left unchanged" | `fileops_futimens.futimens_utime_omit` | covered |
| "Upon successful completion ... shall return 0" | `fileops_futimens.futimens_set_specific_times` | covered |
| "Otherwise ... -1 shall be returned and errno set" | `fileops_futimens.futimens_ebadf` | covered |
| [EBADF]: "fd is not a valid file descriptor" | `fileops_futimens.futimens_ebadf` | covered |
| [EINVAL]: "tv_nsec value ... less than 0 or greater than or equal to 1000 million, and ... not UTIME_NOW or UTIME_OMIT" | `fileops_futimens.futimens_einval_bad_nsec` | covered |
| [EINVAL]: nsec >= 1000000000 | `fileops_futimens.futimens_einval_nsec_too_large` | covered |

## utimensat()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall set the access and modification times ... relative to the directory associated with the file descriptor fd" | `fileops_utimensat.utimensat_relative_path` | covered |
| "If fd has the special value AT_FDCWD ... relative path resolved from current working directory" | `fileops_utimensat.utimensat_at_fdcwd` | covered |
| "If times is a null pointer, the access and modification times ... shall be set to the current time" | `fileops_utimensat.utimensat_null_times` | covered |
| "Upon successful completion ... shall return 0" | `fileops_utimensat.utimensat_relative_path` | covered |
| "Otherwise ... -1 shall be returned and errno set" | `fileops_utimensat.utimensat_enoent` | covered |
| [ENOENT]: "A component of path does not name an existing file" | `fileops_utimensat.utimensat_enoent` | covered |
| [EBADF]: "path does not specify an absolute path and fd is neither AT_FDCWD nor a valid file descriptor" | `fileops_utimensat.utimensat_ebadf` | covered |
| [EINVAL]: "tv_nsec value ... invalid (not UTIME_NOW, UTIME_OMIT, or in range)" | `fileops_utimensat.utimensat_einval_bad_nsec` | covered |

## fchown()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall be equivalent to chown() except that the file ... is specified by the file descriptor fildes" | `fileops_fchown.fchown_set_own_uid_gid` | covered |
| "If the owner or group is specified as (uid_t)-1 or (gid_t)-1, the corresponding ID ... shall not be changed" | `fileops_fchown.fchown_no_change` | covered |
| "Upon successful completion, fchown() shall return 0" | `fileops_fchown.fchown_set_own_uid_gid` | covered |
| "Otherwise, it shall return -1 and set errno" | `fileops_fchown.fchown_ebadf` | covered |
| [EBADF]: "The fildes argument is not an open file descriptor" | `fileops_fchown.fchown_ebadf` | covered |
| [EPERM]: "The effective user ID does not match the owner ... and the process does not have appropriate privileges" | — | not tested: test runs as file owner |

## fdatasync()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall force all currently queued I/O operations associated with the file ... to the synchronized I/O completion state" | `fileops_fdatasync.fdatasync_success` | covered |
| "shall force a write of currently cached data to the file" (only data, not necessarily metadata) | `fileops_fdatasync.fdatasync_success` | covered |
| "Upon successful completion, fdatasync() shall return 0" | `fileops_fdatasync.fdatasync_empty_file` | covered |
| "Otherwise, -1 shall be returned and errno set" | `fileops_fdatasync.fdatasync_ebadf` | covered |
| [EBADF]: "The fildes argument is not a valid open file descriptor" | `fileops_fdatasync.fdatasync_ebadf` | covered |
| [ENOSPC]: "There was no free space remaining on the device" | — | not tested: cannot portably fill filesystem |
| [EROFS/EIO]: synchronization errors | — | not tested: cannot portably trigger I/O errors |

## lockf()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall lock sections of a file with advisory-mode locks" | `fileops_lockf.lockf_lock_and_unlock` | covered |
| F_LOCK: "Set a section lock. If already locked by another process, shall block until the section becomes unlocked" | `fileops_lockf.lockf_lock_and_unlock` | covered |
| F_TLOCK: "Test ... and lock. If already locked by another process, shall fail and set errno" | `fileops_lockf.lockf_tlock_success` | covered |
| F_TLOCK fails with EAGAIN/EACCES when region held by another process | `fileops_lockf.lockf_tlock_conflict_other_process` | covered |
| F_ULOCK: "Unlock locked sections" | `fileops_lockf.lockf_lock_and_unlock` | covered |
| F_TEST: "Test for a section lock. If unlocked, shall return 0" | `fileops_lockf.lockf_test_unlocked` | covered |
| "If size is 0, the section from the current offset through the largest possible file offset shall be locked" | `fileops_lockf.lockf_lock_size_zero_to_eof` | covered |
| "Upon successful completion, lockf() shall return 0" | `fileops_lockf.lockf_lock_and_unlock` | covered |
| "Otherwise, -1 shall be returned and errno set" | `fileops_lockf.lockf_ebadf` | covered |
| [EBADF]: "The fildes argument is not a valid open file descriptor" | `fileops_lockf.lockf_ebadf` | covered |
| [EINVAL]: "The function argument is not one of the valid values" | `fileops_lockf.lockf_einval_bad_function` | covered |
| [EDEADLK]: "A deadlock was detected" | — | not tested: requires precise multi-process coordination |

## sync()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall cause all information in memory that updates file systems to be scheduled for writing out to all file systems" | `fileops_sync.sync_does_not_crash` | covered |
| "The writing, although scheduled, is not necessarily complete upon return from sync()" | `fileops_sync.sync_does_not_crash` | covered |
| sync() returns void — no error conditions defined | `fileops_sync.sync_does_not_crash` | covered |
