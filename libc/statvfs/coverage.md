# Coverage: `statvfs()`

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "The statvfs() function shall obtain information about the file system containing the file named by path." | `statvfs_statvfs.statvfs_root_success` | covered |
| "the buf argument is a pointer to a statvfs structure that shall be filled" | `statvfs_statvfs.statvfs_struct_fields_consistent` | covered |
| "Read, write, or execute permission of the named file is not required." | `statvfs_statvfs.statvfs_regular_file` | covered |
| ST_RDONLY: "Read-only file system." returned in f_flag | `statvfs_statvfs.statvfs_f_flag_readonly` | covered |
| "Upon successful completion, statvfs() shall return 0." | `statvfs_statvfs.statvfs_root_success` | covered |
| "Otherwise, it shall return -1 and set errno to indicate the error." | `statvfs_statvfs.statvfs_enoent_missing_file` | covered |
| [EIO]: "An I/O error occurred while reading the file system." | — | not tested: cannot reliably trigger I/O error without manipulating hardware or mock fs |
| [EINTR]: "A signal was caught during execution of the function." | — | not tested: cannot reliably trigger EINTR on statvfs without kernel-level instrumentation |
| [EOVERFLOW]: "One of the values to be returned cannot be represented correctly in the structure pointed to by buf." | — | not tested: cannot trigger overflow without a filesystem reporting values exceeding struct limits |
| [EACCES]: "Search permission is denied on a component of the path prefix." | `statvfs_statvfs.statvfs_eacces_no_search_perm` | covered |
| [ELOOP]: "A loop exists in symbolic links encountered during resolution of the path argument." | `statvfs_statvfs.statvfs_eloop_symlink_loop` | covered |
| [ENAMETOOLONG]: "The length of a component of a pathname is longer than {NAME_MAX}." | `statvfs_statvfs.statvfs_enametoolong` | covered |
| [ENOENT]: "A component of path does not name an existing file or path is an empty string." | `statvfs_statvfs.statvfs_enoent_missing_file`, `statvfs_statvfs.statvfs_enoent_empty_string` | covered |
| [ENOTDIR]: "A component of the path prefix names an existing file that is neither a directory nor a symbolic link to a directory" | `statvfs_statvfs.statvfs_enotdir_prefix_not_dir` | covered |
| statvfs works on regular files | `statvfs_statvfs.statvfs_regular_file` | covered |
| statvfs works on directories | `statvfs_statvfs.statvfs_directory` | covered |
| statvfs resolves symbolic links | `statvfs_statvfs.statvfs_symlink` | covered |
| Two paths on same filesystem report consistent parameters | `statvfs_statvfs.statvfs_same_fs_consistent` | covered |

# Coverage: `fstatvfs()`

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "The fstatvfs() function shall obtain information about the file system containing the file referenced by fildes." | `statvfs_fstatvfs.fstatvfs_regular_file` | covered |
| "the buf argument is a pointer to a statvfs structure that shall be filled" | `statvfs_fstatvfs.fstatvfs_struct_fields_consistent` | covered |
| "Read, write, or execute permission of the named file is not required." | `statvfs_fstatvfs.fstatvfs_readonly_fd` | covered |
| "Upon successful completion, statvfs() shall return 0." (applies to fstatvfs) | `statvfs_fstatvfs.fstatvfs_regular_file` | covered |
| "Otherwise, it shall return -1 and set errno to indicate the error." | `statvfs_fstatvfs.fstatvfs_ebadf_invalid_fd` | covered |
| [EIO]: "An I/O error occurred while reading the file system." | — | not tested: cannot reliably trigger I/O error without manipulating hardware or mock fs |
| [EINTR]: "A signal was caught during execution of the function." | — | not tested: cannot reliably trigger EINTR on fstatvfs without kernel-level instrumentation |
| [EOVERFLOW]: "One of the values to be returned cannot be represented correctly in the structure pointed to by buf." | — | not tested: cannot trigger overflow without a filesystem reporting values exceeding struct limits |
| [EBADF]: "The fildes argument is not an open file descriptor." | `statvfs_fstatvfs.fstatvfs_ebadf_invalid_fd` | covered |
| EBADF with closed fd | `statvfs_fstatvfs.fstatvfs_ebadf_closed_fd` | covered |
| EBADF with out-of-range fd | `statvfs_fstatvfs.fstatvfs_ebadf_large_fd` | covered |
| fstatvfs works on directory file descriptors | `statvfs_fstatvfs.fstatvfs_directory_fd` | covered |
| fstatvfs and statvfs return consistent results for same file | `statvfs_fstatvfs.fstatvfs_matches_statvfs` | covered |
