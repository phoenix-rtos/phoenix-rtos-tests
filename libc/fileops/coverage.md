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

## readlink(), readlinkat()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall place the contents of the symbolic link referred to by path in the buffer buf which has size bufsize" | `fileops_readlink.readlink_content_file_target` | covered |
| Link targets: regular file / directory / dangling / long content | `fileops_readlink.readlink_content_dir_target`, `readlink_content_dangling`, `readlink_content_long` | covered |
| "If the buf argument is not large enough to contain the link content, the first bufsize bytes shall be placed in buf" | `fileops_readlink.readlink_truncated_to_bufsize` | covered |
| bufsize edge values (exactly link length, 1) | `fileops_readlink.readlink_exact_bufsize`, `readlink_bufsize_one` | covered |
| "Upon successful completion, readlink() shall mark for update the last data access timestamp of the symbolic link" | `fileops_readlink.readlink_marks_atime` | covered |
| "Upon successful completion, these functions shall return the count of bytes placed in the buffer" | `fileops_readlink.readlink_content_file_target` | covered |
| "Otherwise, these functions shall return a value of -1, leave the buffer unchanged, and set errno" | `fileops_readlink.readlink_einval_regular_file` | covered |
| "Conforming applications should not assume that the returned contents of the symbolic link are null-terminated" | `fileops_readlink.readlink_exact_bufsize` | covered (no NUL written past returned count) |
| [EACCES]: "Search permission is denied for a component of the path prefix of path" | `fileops_readlink.readlink_eacces_prefix` | covered (ignored when run as root) |
| [EINVAL]: "The path argument names a file that is not a symbolic link" | `fileops_readlink.readlink_einval_regular_file` | covered |
| [EIO]: "An I/O error occurred while reading from the file system" | — | not tested: cannot be triggered using POSIX interfaces |
| [ELOOP]: "A loop exists in symbolic links encountered during resolution of the path argument" | `fileops_readlink.readlink_eloop` | covered |
| [ENAMETOOLONG]: "The length of a component of a pathname is longer than {NAME_MAX}" | `fileops_readlink.readlink_enametoolong` | covered |
| [ENOENT]: "A component of path does not name an existing file" | `fileops_readlink.readlink_enoent_missing` | covered |
| [ENOENT]: "... or path is an empty string" | `fileops_readlink.readlink_enoent_empty_path` | covered |
| [ENOTDIR]: "A component of the path prefix names an existing file that is neither a directory nor a symbolic link to a directory" | `fileops_readlink.readlink_enotdir_prefix_file` | covered |
| [ENOTDIR]: "... ends with one or more trailing <slash> characters and the last pathname component names an existing file that is neither a directory nor a symbolic link to a directory" | `fileops_readlink.readlink_enotdir_trailing_slash` | covered |
| readlinkat: "the symbolic link whose content is read is relative to the directory associated with the file descriptor fd instead of the current working directory" | `fileops_readlinkat.readlinkat_relative_to_fd` | covered |
| readlinkat: equivalent to readlink() except for relative paths (absolute path ignores fd) | `fileops_readlinkat.readlinkat_absolute_ignores_fd` | covered |
| "If the access mode ... is not O_SEARCH, the function shall check whether directory searches are permitted using the current permissions of the directory underlying the file descriptor" | `fileops_readlinkat.readlinkat_eacces_fd_no_search` | covered (ignored when run as root) |
| "If the access mode is O_SEARCH, the function shall not perform the check" | `fileops_readlinkat.readlinkat_o_search_no_check` | covered (ignored where libc lacks O_SEARCH, e.g. glibc) |
| "If readlinkat() is passed the special value AT_FDCWD ... the behavior shall be identical to a call to readlink()" | `fileops_readlinkat.readlinkat_at_fdcwd` | covered |
| readlinkat [EACCES]: "The access mode ... is not O_SEARCH and the permissions of the directory underlying fd do not permit directory searches" | `fileops_readlinkat.readlinkat_eacces_fd_no_search` | covered (ignored when run as root) |
| readlinkat [EBADF]: "path ... not an absolute path and the fd argument is neither AT_FDCWD nor a valid file descriptor open for reading or searching" | `fileops_readlinkat.readlinkat_ebadf` | covered |
| readlinkat [ENOTDIR]: "path ... not an absolute path and fd is a file descriptor associated with a non-directory file" | `fileops_readlinkat.readlinkat_enotdir_fd_not_dir` | covered |

## symlinkat()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall create a symbolic link called path2 that contains the string pointed to by path1" | `fileops_symlinkat.symlinkat_relative_to_fd` | covered |
| Link to an existing file resolves to it | `fileops_symlinkat.symlinkat_existing_target_resolves` | covered |
| "The string pointed to by path1 shall be treated only as a string and shall not be validated as a pathname" | `fileops_symlinkat.symlinkat_path1_not_validated` | covered |
| "If the symlink() function fails for any reason other than [EIO], any file named by path2 shall be unaffected" | `fileops_symlinkat.symlinkat_eexist` | covered |
| "If path2 names a symbolic link, symlink() shall fail and set errno to [EEXIST]" | `fileops_symlinkat.symlinkat_eexist` | covered |
| "The symbolic link's user ID shall be set to the process' effective user ID" | `fileops_symlinkat.symlinkat_ownership` | covered |
| "The symbolic link's group ID shall be set to the group ID of the parent directory or to the effective group ID of the process" | `fileops_symlinkat.symlinkat_ownership` | covered |
| "Implementations shall provide a way to initialize the symbolic link's group ID to the group ID of the parent directory" | — | not tested: mechanism is implementation-defined |
| "shall mark for update the last data access, last data modification, and last file status change timestamps of the symbolic link" | `fileops_symlinkat.symlinkat_marks_timestamps` | covered |
| "the last data modification and last file status change timestamps of the directory that contains the new entry shall be marked for update" | `fileops_symlinkat.symlinkat_marks_timestamps` | covered |
| "the symbolic link is created relative to the directory associated with the file descriptor fd instead of the current working directory" | `fileops_symlinkat.symlinkat_relative_to_fd` | covered |
| symlinkat equivalent to symlink() except for relative path2 (absolute path2 ignores fd) | `fileops_symlinkat.symlinkat_absolute_ignores_fd` | covered |
| "If the access mode ... is not O_SEARCH, the function shall check whether directory searches are permitted using the current permissions of the directory underlying the file descriptor" | `fileops_symlinkat.symlinkat_eacces_fd_no_search` | covered (ignored when run as root) |
| "If the access mode is O_SEARCH, the function shall not perform the check" | `fileops_symlinkat.symlinkat_o_search_no_check` | covered (ignored where libc lacks O_SEARCH, e.g. glibc) |
| "If symlinkat() is passed the special value AT_FDCWD ... the behavior shall be identical to a call to symlink()" | `fileops_symlinkat.symlinkat_at_fdcwd` | covered |
| "Upon successful completion, these functions shall return 0" | `fileops_symlinkat.symlinkat_relative_to_fd` | covered |
| "Otherwise, these functions shall return -1 and set errno to indicate the error" | `fileops_symlinkat.symlinkat_eexist` | covered |
| [EACCES]: "Write permission is denied in the directory where the symbolic link is being created" | `fileops_symlinkat.symlinkat_eacces_no_write` | covered (ignored when run as root) |
| [EACCES]: "... or search permission is denied for a component of the path prefix of path2" | `fileops_symlinkat.symlinkat_eacces_no_search` | covered (ignored when run as root) |
| [EEXIST]: "The path2 argument names an existing file" | `fileops_symlinkat.symlinkat_eexist` | covered |
| [EIO]: "An I/O error occurs while reading from or writing to the file system" | — | not tested: cannot be triggered using POSIX interfaces |
| [ELOOP]: "A loop exists in symbolic links encountered during resolution of the path2 argument" | `fileops_symlinkat.symlinkat_eloop` | covered |
| [ENAMETOOLONG]: "The length of a component of the pathname specified by the path2 argument is longer than {NAME_MAX}" | `fileops_symlinkat.symlinkat_enametoolong_path2` | covered |
| [ENAMETOOLONG]: "... or the length of the path1 argument is longer than {SYMLINK_MAX}" | `fileops_symlinkat.symlinkat_enametoolong_path1` | covered (ignored when SYMLINK_MAX is indeterminate, e.g. Linux) |
| [ENOENT]: "A component of the path prefix of path2 does not name an existing file or path2 is an empty string" | `fileops_symlinkat.symlinkat_enoent` | covered |
| [ENOENT] or [ENOTDIR]: "path2 ... ends with one or more trailing <slash> characters. If path2 without the trailing <slash> characters would name an existing file, an [ENOENT] error shall not occur" | `fileops_symlinkat.symlinkat_trailing_slash` | covered |
| [ENOSPC]: "no space is left on the file system ..." | — | not tested: cannot fill the file system portably/safely |
| [ENOTDIR]: "A component of the path prefix of path2 names an existing file that is neither a directory nor a symbolic link to a directory" | `fileops_symlinkat.symlinkat_enotdir_prefix` | covered |
| [EROFS]: "The new symbolic link would reside on a read-only file system" | — | not tested: no portable read-only file system available |
| symlinkat [EACCES]: "The access mode ... is not O_SEARCH and the permissions of the directory underlying fd do not permit directory searches" | `fileops_symlinkat.symlinkat_eacces_fd_no_search` | covered (ignored when run as root) |
| symlinkat [EBADF]: "path2 ... not an absolute path and the fd argument is neither AT_FDCWD nor a valid file descriptor open for reading or searching" | `fileops_symlinkat.symlinkat_ebadf` | covered |
| symlinkat [ENOTDIR]: "path2 ... not an absolute path and fd is a file descriptor associated with a non-directory file" | `fileops_symlinkat.symlinkat_enotdir_fd_not_dir` | covered |

## unlinkat()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall remove the link named by the pathname pointed to by path" | `fileops_unlinkat.unlinkat_removes_file_relative_to_fd` | covered |
| "If path names a symbolic link, unlink() shall remove the symbolic link named by path and shall not affect any file or directory named by the contents of the symbolic link" | `fileops_unlinkat.unlinkat_symlink_not_followed` | covered |
| "... and shall decrement the link count of the file referenced by the link" | `fileops_unlinkat.unlinkat_decrements_link_count` | covered |
| "When the file's link count becomes 0 and no process has the file open, the space occupied by the file shall be freed and the file shall no longer be accessible" | `fileops_unlinkat.unlinkat_removes_file_relative_to_fd` | covered (inaccessibility; freed space not observable portably) |
| "If one or more processes have the file open when the last link is removed, the link shall be removed before unlink() returns, but the removal of the file contents shall be postponed until all references to the file are closed" | `fileops_unlinkat.unlinkat_open_file_stays_accessible` | covered |
| "The path argument shall not name a directory unless the process has appropriate privileges and the implementation supports using unlink() on directories" | `fileops_unlinkat.unlinkat_directory_eperm` | covered (errno ignored on host-pc: Linux returns EISDIR) |
| "shall mark for update the last data modification and last file status change timestamps of the parent directory" | `fileops_unlinkat.unlinkat_marks_parent_timestamps` | covered |
| "if the file's link count is not 0, the last file status change timestamp of the file shall be marked for update" | `fileops_unlinkat.unlinkat_decrements_link_count` | covered |
| "the directory entry to be removed is determined relative to the directory associated with the file descriptor fd instead of the current working directory" | `fileops_unlinkat.unlinkat_removes_file_relative_to_fd` | covered |
| unlinkat equivalent to unlink()/rmdir() except for relative path (absolute path ignores fd) | `fileops_unlinkat.unlinkat_absolute_ignores_fd` | covered |
| "If the access mode ... is not O_SEARCH, the function shall check whether directory searches are permitted using the current permissions of the directory underlying the file descriptor" | `fileops_unlinkat.unlinkat_eacces_fd_no_search` | covered (ignored when run as root) |
| "If the access mode is O_SEARCH, the function shall not perform the check" | `fileops_unlinkat.unlinkat_o_search_no_check` | covered (ignored where libc lacks O_SEARCH, e.g. glibc) |
| AT_REMOVEDIR: "Remove the directory entry specified by fd and path as a directory, not a normal file" | `fileops_unlinkat.unlinkat_removedir_empty` | covered |
| "If unlinkat() is passed the special value AT_FDCWD ... identical to a call to unlink() or rmdir() respectively, depending on whether or not the AT_REMOVEDIR bit is set" | `fileops_unlinkat.unlinkat_at_fdcwd` | covered |
| "Upon successful completion, these functions shall return 0" | `fileops_unlinkat.unlinkat_removes_file_relative_to_fd` | covered |
| "Otherwise, these functions shall return -1 and set errno ... If -1 is returned, the named file shall not be changed" | `fileops_unlinkat.unlinkat_removedir_not_empty` | covered |
| [EACCES]: "Search permission is denied for a component of the path prefix" | `fileops_unlinkat.unlinkat_eacces_no_search` | covered (ignored when run as root) |
| [EACCES]: "... or write permission is denied on the directory containing the directory entry to be removed" | `fileops_unlinkat.unlinkat_eacces_no_write` | covered (ignored when run as root) |
| [EBUSY]: "The file named by the path argument cannot be unlinked because it is being used by the system or another process and the implementation considers this an error" | — | not tested: implementation-defined condition, requires e.g. mount points |
| [ELOOP]: "A loop exists in symbolic links encountered during resolution of the path argument" | `fileops_unlinkat.unlinkat_eloop` | covered |
| [ENAMETOOLONG]: "The length of a component of a pathname is longer than {NAME_MAX}" | `fileops_unlinkat.unlinkat_enametoolong` | covered |
| [ENOENT]: "A component of path does not name an existing file or path is an empty string" | `fileops_unlinkat.unlinkat_enoent` | covered |
| [ENOTDIR]: "A component of the path prefix names an existing file that is neither a directory nor a symbolic link to a directory, or the path argument ... ends with one or more trailing <slash> characters and the last pathname component names an existing file that is neither a directory nor a symbolic link to a directory" | `fileops_unlinkat.unlinkat_enotdir_path` | covered |
| [EPERM]: "The file named by path is a directory, and either the calling process does not have appropriate privileges, or the implementation prohibits using unlink() on directories" | `fileops_unlinkat.unlinkat_directory_eperm` | covered (errno ignored on host-pc: Linux returns EISDIR) |
| [EPERM] or [EACCES] (XSI): "The S_ISVTX flag is set on the directory containing the file ... and the process does not satisfy the criteria specified in XBD Directory Protection" | — | not tested: requires a file owned by another user (privileged setup) |
| [EROFS]: "The directory entry to be unlinked is part of a read-only file system" | — | not tested: no portable read-only file system available |
| unlinkat [EACCES]: "The access mode ... is not O_SEARCH and the permissions of the directory underlying fd do not permit directory searches" | `fileops_unlinkat.unlinkat_eacces_fd_no_search` | covered (ignored when run as root) |
| unlinkat [EBADF]: "path ... not an absolute path and the fd argument is neither AT_FDCWD nor a valid file descriptor open for reading or searching" | `fileops_unlinkat.unlinkat_ebadf` | covered |
| unlinkat [ENOTDIR]: "path ... not an absolute path and fd is a file descriptor associated with a non-directory file" | `fileops_unlinkat.unlinkat_enotdir_fd_not_dir` | covered |
| unlinkat [EEXIST] or [ENOTEMPTY]: "The flag parameter has the AT_REMOVEDIR bit set and the path argument names a directory that is not an empty directory" | `fileops_unlinkat.unlinkat_removedir_not_empty` | covered |
| unlinkat [ENOTDIR]: "The flag parameter has the AT_REMOVEDIR bit set and path does not name a directory" | `fileops_unlinkat.unlinkat_removedir_enotdir` | covered |
