# Coverage: open(), creat(), poll()

## open()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall establish the connection between a file and a file descriptor" | `fcntl_open.open_rdonly_existing` | covered |
| "shall return a non-negative integer representing the file descriptor" | `fcntl_open.open_return_value_nonnegative` | covered |
| "The FD_CLOEXEC file descriptor flag associated with the new file descriptor shall be cleared unless the O_CLOEXEC flag is set" | `fcntl_open.open_no_cloexec_by_default` | covered |
| O_CLOEXEC: "the FD_CLOEXEC flag for the new file descriptor shall be set" | `fcntl_open.open_cloexec_flag` | covered |
| "The file offset used to mark the current position within the file shall be set to the beginning of the file" | `fcntl_open.open_offset_at_beginning` | covered |
| O_RDONLY: "Open for reading only" | `fcntl_open.open_rdonly_existing` | covered |
| O_WRONLY: "Open for writing only" | `fcntl_open.open_wronly_existing` | covered |
| O_RDWR: "Open for reading and writing" | `fcntl_open.open_rdwr_existing` | covered |
| O_APPEND: "the file offset shall be set to the end of the file prior to each write" | `fcntl_open.open_append` | covered |
| O_CREAT: "if O_DIRECTORY is not set the file shall be created as a regular file" | `fcntl_open.open_creat_new_file` | covered |
| O_CREAT: "access permission bits of the file mode shall be set to the value of the argument ... modified ... by the complement of the process' file mode creation mask" | `fcntl_open.open_creat_new_file` | covered |
| O_CREAT: "If the file exists, this flag has no effect except as noted under O_EXCL" | `fcntl_open.open_creat_existing_no_effect` | covered |
| O_DIRECTORY: "If path resolves to a non-directory file, fail and set errno to [ENOTDIR]" | `fcntl_open.open_directory_enotdir` | covered |
| O_DIRECTORY: open a directory for reading | `fcntl_open.open_directory` | covered |
| O_EXCL + O_CREAT: "open() shall fail if the file exists" | `fcntl_open.open_creat_excl_existing_eexist` | covered |
| O_EXCL + O_CREAT: "if path names a symbolic link, open() shall fail and set errno to [EEXIST]" | `fcntl_open.open_creat_excl_symlink_eexist` | covered |
| O_EXCL + O_CREAT: create succeeds if file does not exist | `fcntl_open.open_creat_excl_new_file` | covered |
| O_NOFOLLOW: "If path names a symbolic link, fail and set errno to [ELOOP]" | `fcntl_open.open_nofollow_eloop` | covered |
| O_NONBLOCK + FIFO + O_RDONLY: "shall return without delay" | `fcntl_open.open_nonblock_fifo_rdonly` | covered |
| O_NONBLOCK + FIFO + O_WRONLY: "shall return an error if no process currently has the file open for reading" | `fcntl_open.open_nonblock_fifo_wronly_enxio` | covered |
| O_TRUNC: "its length shall be truncated to 0" | `fcntl_open.open_trunc_existing` | covered |
| "these functions shall return -1 and set errno to indicate the error" | `fcntl_open.open_failure_returns_minus_one` | covered |
| File Descriptor Allocation: lowest numbered available | `fcntl_open.open_returns_lowest_fd` | covered |
| [EEXIST]: "O_CREAT and O_EXCL are set, and the named file exists" | `fcntl_open.open_creat_excl_existing_eexist` | covered |
| [EISDIR]: "The named file is a directory and oflag includes O_WRONLY or O_RDWR" | `fcntl_open.open_eisdir_wronly` | covered |
| [ELOOP]: "O_NOFOLLOW was specified and the path argument names a symbolic link" | `fcntl_open.open_nofollow_eloop` | covered |
| [ENOENT]: "O_CREAT is not set and a component of path does not name an existing file" | `fcntl_open.open_enoent_no_creat` | covered |
| [ENOENT]: "O_CREAT is set and a component of the path prefix of path does not name an existing file" | `fcntl_open.open_enoent_path_prefix` | covered |
| [ENOENT]: "path points to an empty string" | `fcntl_open.open_enoent_empty_path` | covered |
| [ENOTDIR]: "A component of the path prefix names an existing file that is neither a directory nor a symbolic link to a directory" | `fcntl_open.open_enotdir_prefix` | covered |
| [ENAMETOOLONG]: "The length of a component of a pathname is longer than {NAME_MAX}" | `fcntl_open.open_enametoolong` | covered |
| [ENXIO]: "O_NONBLOCK is set, the named file is a FIFO, O_WRONLY is set, and no process has the file open for reading" | `fcntl_open.open_nonblock_fifo_wronly_enxio` | covered |
| [EACCES]: "Search permission is denied on a component of the path prefix" | — | not tested: requires running as non-root with specific permission setup |
| [EINTR]: "A signal was caught during open()" | — | not tested: cannot reliably trigger during open() on regular files |
| [EMFILE]: "All file descriptors available to the process are currently open" | — | not tested: exhausting fd limit is fragile and may break the test harness |
| [ENFILE]: "The maximum allowable number of files is currently open in the system" | — | not tested: system-wide limit cannot be safely triggered |
| [ENOSPC]: "The directory or file system ... cannot be expanded" | — | not tested: cannot reliably fill filesystem in a portable test |
| [EOVERFLOW]: "size of the file cannot be represented correctly in an object of type off_t" | — | not tested: cannot create such a file portably |
| [EROFS]: "The named file resides on a read-only file system" | — | not tested: requires read-only filesystem mount |

## creat()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall behave as if it is implemented as: open(path, O_WRONLY\|O_CREAT\|O_TRUNC, mode)" — creates new file | `fcntl_creat.creat_new_file` | covered |
| Implies O_WRONLY: fd is write-only | `fcntl_creat.creat_wronly` | covered |
| Implies O_CREAT: mode bits applied with umask | `fcntl_creat.creat_applies_mode_with_umask` | covered |
| Implies O_TRUNC: truncates existing file | `fcntl_creat.creat_truncates_existing` | covered |
| Return value: non-negative fd on success | `fcntl_creat.creat_return_value_nonnegative` | covered |
| Return value: -1 on failure with errno set | `fcntl_creat.creat_failure_returns_minus_one` | covered |
| [ENOENT]: path prefix does not exist | `fcntl_creat.creat_failure_returns_minus_one` | covered |
| [ENOENT]: empty path | `fcntl_creat.creat_enoent_empty_path` | covered |
| [EISDIR]: path names a directory (O_WRONLY on dir) | `fcntl_creat.creat_eisdir` | covered |
| [ENOTDIR]: path prefix component is not a directory | `fcntl_creat.creat_enotdir_prefix` | covered |
| [ENAMETOOLONG]: component longer than NAME_MAX | `fcntl_creat.creat_enametoolong` | covered |

## poll()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "poll() shall examine the given file descriptor for the event(s) specified in events" | `poll_poll.poll_regular_file_pollin_pollout` | covered |
| POLLIN: "Data other than high-priority data may be read without blocking" | `poll_poll.poll_pipe_readable_after_write` | covered |
| POLLOUT: "Normal data may be written without blocking" | `poll_poll.poll_pollout_pipe_writable` | covered |
| POLLERR: "only valid in the revents bitmask; it shall be ignored in the events member" | `poll_poll.poll_pollerr_pollhup_not_in_events` | covered |
| POLLHUP: "A device has been disconnected, or a pipe or FIFO has been closed by the last process that had it open for writing" | `poll_poll.poll_pipe_hangup_on_writer_close` | covered |
| POLLHUP: "only valid in the revents bitmask; it shall be ignored in the events member" | `poll_poll.poll_pollerr_pollhup_not_in_events` | covered |
| POLLNVAL: "The specified fd value is invalid" | `poll_poll.poll_invalid_fd_pollnval` | covered |
| "If the value of fd is less than 0, events shall be ignored, and revents shall be set to 0" | `poll_poll.poll_negative_fd_ignored` | covered |
| "poll() shall clear the revents member" (when no event) | `poll_poll.poll_revents_cleared_if_no_event` | covered |
| "poll() shall set the POLLHUP, POLLERR, and POLLNVAL flag in revents if the condition is true, even if the application did not set the corresponding bit in events" | `poll_poll.poll_pollerr_pollhup_not_in_events` | covered |
| "If the value of timeout is 0, poll() shall return immediately" | `poll_poll.poll_timeout_zero_returns_immediately` | covered |
| "If none of the defined events have occurred ... poll() shall wait at least timeout milliseconds" | `poll_poll.poll_timeout_expires` | covered |
| "Regular files shall always poll TRUE for reading and writing" | `poll_poll.poll_regular_file_pollin_pollout` | covered |
| "A positive value indicates the total number of pollfd structures that have selected events" | `poll_poll.poll_returns_count_of_ready_fds` | covered |
| "A value of 0 indicates that the call timed out and no file descriptors have been selected" | `poll_poll.poll_return_zero_on_timeout` | covered |
| Multiple fds with mixed results | `poll_poll.poll_multiple_fds_mixed` | covered |
| nfds=0: no fds to examine, just wait | `poll_poll.poll_nfds_zero` | covered |
| [EAGAIN]: "The allocation of internal data structures failed" | — | not tested: cannot reliably trigger internal allocation failure |
| [EINTR]: "A signal was caught during poll()" | — | not tested: would require precise signal timing during blocked poll |
| [EINVAL]: "The nfds argument is greater than {OPEN_MAX}" | — | not tested: allocating OPEN_MAX+ pollfd structs is impractical |
