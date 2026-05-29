# Coverage: New POSIX Function Tests

## accept() — `socket/accept.c`

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "accept() shall extract the first connection on the queue of pending connections, create a new socket ... and allocate a new file descriptor" | `socket_api_accept.accept_success` | covered |
| "If address is not a null pointer, the address of the peer for the accepted connection shall be stored" | `socket_api_accept.accept_with_address` | covered |
| "address: Either a null pointer" (accept succeeds with NULL address) | `socket_api_accept.accept_null_address` | covered |
| "The original socket remains open and can accept more connections." | `socket_api_accept.accept_original_still_accepts` | covered |
| "Upon successful completion, accept() shall return the non-negative file descriptor of the accepted socket." | `socket_api_accept.accept_success` | covered |
| "Otherwise, -1 shall be returned, errno shall be set to indicate the error" | `socket_api_accept.accept_ebadf` | covered |
| `errno` = `EAGAIN`/`EWOULDBLOCK`: "O_NONBLOCK is set ... and no connections are present" | `socket_api_accept.accept_eagain_nonblock` | covered |
| `errno` = `EBADF`: "The socket argument is not a valid file descriptor." | `socket_api_accept.accept_ebadf` | covered |
| `errno` = `EINVAL`: "The socket is not accepting connections." | `socket_api_accept.accept_einval_not_listening` | covered |
| `errno` = `ENOTSOCK`: "The socket argument does not refer to a socket." | `socket_api_accept.accept_enotsock` | covered |
| `errno` = `EOPNOTSUPP`: "The socket type ... does not support accepting connections." | `socket_api_accept.accept_eopnotsupp_dgram` | covered |
| `errno` = `ECONNABORTED`: "A connection has been aborted." | — | not tested: cannot reliably trigger abort during accept without racing threads |
| `errno` = `EINTR`: "accept() was interrupted by a signal" | — | not tested: requires precise signal timing |
| `errno` = `EMFILE`: "All file descriptors available to the process are currently open." | — | not tested: exhausting fd limit risks destabilizing the test harness |
| `errno` = `ENFILE`: "The maximum number of file descriptors in the system are already open." | — | not tested: system-wide limit cannot be safely exhausted |
| `errno` = `ENOBUFS`/`ENOMEM`: insufficient resources | — | not tested: cannot reliably trigger |

## bind() — `socket/bind.c`

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "bind() shall assign a local socket address address to a socket" | `socket_api_bind.bind_unix_success` | covered |
| bind works for SOCK_DGRAM | `socket_api_bind.bind_unix_dgram_success` | covered |
| bind works for AF_INET with port 0 | `socket_api_bind.bind_inet_success` | covered |
| "Upon successful completion, bind() shall return 0" | `socket_api_bind.bind_unix_success` | covered |
| "If the address family of the socket is AF_UNIX and the pathname in address names a symbolic link, bind() shall fail and set errno to [EADDRINUSE]." | `socket_api_bind.bind_eaddrinuse_symlink` | covered |
| `errno` = `EADDRINUSE`: "The specified address is already in use." | `socket_api_bind.bind_eaddrinuse` | covered |
| `errno` = `EBADF`: "The socket argument is not a valid file descriptor." | `socket_api_bind.bind_ebadf` | covered |
| `errno` = `EINVAL`: "The socket is already bound to an address" | `socket_api_bind.bind_einval_already_bound` | covered |
| `errno` = `ENOTSOCK`: "The socket argument does not refer to a socket." | `socket_api_bind.bind_enotsock` | covered |
| `errno` = `ENOENT`: "A component of the path prefix ... does not name an existing file" | `socket_api_bind.bind_enoent_missing_dir` | covered |
| `errno` = `EAFNOSUPPORT`: "The specified address is not a valid address for the address family" | `socket_api_bind.bind_eafnosupport` | covered |
| `errno` = `EADDRNOTAVAIL`: "The specified address is not available from the local machine." | — | not tested: requires unavailable network address |
| `errno` = `EOPNOTSUPP`: "The socket type ... does not support binding to an address." | — | not tested: no standard socket type triggers this |

## connect() — `socket/connect.c`

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "connect() shall attempt to make a connection on a connection-mode socket" | `socket_api_connect.connect_unix_stream_success` | covered |
| "If the initiating socket is not connection-mode, then connect() shall set the socket's peer address" | `socket_api_connect.connect_unix_dgram_sets_peer` | covered |
| "If the sa_family member of address is AF_UNSPEC, the socket's peer address shall be reset." | `socket_api_connect.connect_dgram_unspec_resets_peer` | covered |
| "If the socket has not already been bound to a local address, connect() shall bind it to an address" | `socket_api_connect.connect_binds_unnamed` | covered |
| "Upon successful completion, connect() shall return 0" | `socket_api_connect.connect_unix_stream_success` | covered |
| `errno` = `EBADF`: "The socket argument is not a valid file descriptor." | `socket_api_connect.connect_ebadf` | covered |
| `errno` = `ECONNREFUSED`: "The target address was not listening for connections or refused the connection request." | `socket_api_connect.connect_econnrefused` | covered |
| `errno` = `EISCONN`: "The specified socket is connection-mode and is already connected." | `socket_api_connect.connect_eisconn` | covered |
| `errno` = `ENOTSOCK`: "The socket argument does not refer to a socket." | `socket_api_connect.connect_enotsock` | covered |
| `errno` = `ENOENT`: "A component of the pathname does not name an existing file" | `socket_api_connect.connect_enoent` | covered |
| `errno` = `EAFNOSUPPORT`: "The specified address is not a valid address for the address family" | `socket_api_connect.connect_eafnosupport` | covered |
| `errno` = `EALREADY`: "A connection request is already in progress" | — | not tested: requires non-blocking connect race |
| `errno` = `EINPROGRESS`: "O_NONBLOCK is set ... and the connection cannot be immediately established" | — | not tested: requires non-blocking inet connect |
| `errno` = `EINTR`: "interrupted by delivery of a signal" | — | not tested: requires precise signal timing |
| `errno` = `ETIMEDOUT`: "The attempt to connect timed out" | — | not tested: requires unreachable host with long timeout |

## getpeername() — `socket/getpeername.c`

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "getpeername() shall retrieve the peer address of the specified socket" | `socket_api_getpeername.getpeername_success_client` | covered |
| "store this address in the sockaddr structure pointed to by the address argument" | `socket_api_getpeername.getpeername_success_accepted` | covered |
| "If the actual length of the address is greater than the length of the supplied sockaddr structure, the stored address shall be truncated." | `socket_api_getpeername.getpeername_truncation` | covered |
| "Upon successful completion, 0 shall be returned." | `socket_api_getpeername.getpeername_success_client` | covered |
| Works for AF_INET sockets | `socket_api_getpeername.getpeername_inet_loopback` | covered |
| `errno` = `EBADF`: "The socket argument is not a valid file descriptor." | `socket_api_getpeername.getpeername_ebadf` | covered |
| `errno` = `ENOTCONN`: "The socket is not connected" | `socket_api_getpeername.getpeername_enotconn` | covered |
| `errno` = `ENOTSOCK`: "The socket argument does not refer to a socket." | `socket_api_getpeername.getpeername_enotsock` | covered |
| `errno` = `EOPNOTSUPP`: "The operation is not supported for the socket protocol." | — | not tested: no standard protocol triggers this with connected socket |

## getsockname() — `socket/getsockname.c`

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "getsockname() shall retrieve the locally-bound name of the specified socket" | `socket_api_getsockname.getsockname_unix_bound` | covered |
| "store the length of this address in the object pointed to by address_len" | `socket_api_getsockname.getsockname_addrlen_updated` | covered |
| "If the actual length of the address is greater than the length of the supplied sockaddr structure, the stored address shall be truncated." | `socket_api_getsockname.getsockname_truncation` | covered |
| "If the socket has not been bound to a local name, the value stored ... is unspecified." (call still succeeds) | `socket_api_getsockname.getsockname_unbound` | covered |
| Works for AF_INET with kernel-assigned port | `socket_api_getsockname.getsockname_inet_bound` | covered |
| getsockname after implicit bind via connect | `socket_api_getsockname.getsockname_after_connect` | covered |
| "Upon successful completion, 0 shall be returned" | `socket_api_getsockname.getsockname_unix_bound` | covered |
| `errno` = `EBADF`: "The socket argument is not a valid file descriptor." | `socket_api_getsockname.getsockname_ebadf` | covered |
| `errno` = `ENOTSOCK`: "The socket argument does not refer to a socket." | `socket_api_getsockname.getsockname_enotsock` | covered |
| `errno` = `EOPNOTSUPP`: "The operation is not supported for this socket's protocol." | — | not tested: no standard protocol triggers this |

## gethostid() — `misc/unistd_host.c`

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "gethostid() shall retrieve a 32-bit identifier for the current host." | `unistd_gethostid.gethostid_returns_value` | covered |
| "Upon successful completion, gethostid() shall return an identifier for the current host." | `unistd_gethostid.gethostid_returns_value` | covered |
| Consistent across calls | `unistd_gethostid.gethostid_consistent` | covered |

## gethostname() — `misc/unistd_host.c`

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "gethostname() shall return the standard host name for the current machine." | `unistd_gethostname.gethostname_success` | covered |
| "The returned name shall be null-terminated, except that if namelen is an insufficient length to hold the host name, then the returned name shall be truncated" | `unistd_gethostname.gethostname_truncation` | covered |
| "Host names are limited to {HOST_NAME_MAX} bytes." | `unistd_gethostname.gethostname_max_length` | covered |
| "Upon successful completion, 0 shall be returned" | `unistd_gethostname.gethostname_success` | covered |
| Exact-length buffer works | `unistd_gethostname.gethostname_exact_length` | covered |
| Consistent across calls | `unistd_gethostname.gethostname_consistent` | covered |

## getpgid() — `misc/unistd_pgrp.c`

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "getpgid() shall return the process group ID of the process whose process ID is equal to pid." | `unistd_getpgid.getpgid_self_pid` | covered |
| "If pid is equal to 0, getpgid() shall return the process group ID of the calling process." | `unistd_getpgid.getpgid_self_zero` | covered |
| getpgid(0) equals getpgrp() | `unistd_getpgid.getpgid_equals_getpgrp` | covered |
| Child inherits parent's process group | `unistd_getpgid.getpgid_child_inherits` | covered |
| `errno` = `ESRCH`: "There is no process with a process ID equal to pid." | `unistd_getpgid.getpgid_esrch_invalid_pid` | covered |
| `errno` = `EPERM`: "process ... is not in the same session" | — | not tested: requires a process in a different session whose PID is known |

## getpgrp() — `misc/unistd_pgrp.c`

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "getpgrp() shall return the process group ID of the calling process." | `unistd_getpgrp.getpgrp_returns_positive` | covered |
| "getpgrp() shall always be successful and no return value is reserved to indicate an error." | `unistd_getpgrp.getpgrp_consistent` | covered |
| Child inherits parent's process group | `unistd_getpgrp.getpgrp_child_inherits` | covered |

## getsid() — `misc/unistd_pgrp.c`

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "getsid() shall obtain the process group ID of the process that is the session leader of the process specified by pid." | `unistd_getsid.getsid_self_zero` | covered |
| "If pid is (pid_t)0, it specifies the calling process." | `unistd_getsid.getsid_self_zero` | covered |
| getsid(getpid()) == getsid(0) | `unistd_getsid.getsid_self_pid` | covered |
| Consistent across calls | `unistd_getsid.getsid_consistent` | covered |
| Child in same session as parent | `unistd_getsid.getsid_child_same_session` | covered |
| `errno` = `ESRCH`: "There is no process with a process ID equal to pid." | `unistd_getsid.getsid_esrch_invalid_pid` | covered |
| `errno` = `EPERM`: "process ... is not in the same session" | — | not tested: requires cross-session access |

## setpgrp() — `misc/unistd_pgrp.c`

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "setpgrp() sets the process group ID of the calling process to the process ID of the calling process." | `unistd_setpgrp.setpgrp_sets_pgid_to_pid` | covered |
| "Upon completion, setpgrp() shall return the process group ID." | `unistd_setpgrp.setpgrp_returns_pgid` | covered |
| "setpgrp() has no effect when the calling process is a session leader." | `unistd_setpgrp.setpgrp_session_leader_no_effect` | covered |

## fsync() — `misc/unistd_fsync.c`

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "fsync() shall request that all data for the open file descriptor ... is to be transferred to the storage device" | `unistd_fsync.fsync_regular_file` | covered |
| "fsync() shall not return until the system has completed that action" | `unistd_fsync.fsync_data_persists` | covered |
| "Upon successful completion, fsync() shall return 0." | `unistd_fsync.fsync_regular_file` | covered |
| Works after multiple writes | `unistd_fsync.fsync_after_multiple_writes` | covered |
| fsync on read-only fd | `unistd_fsync.fsync_readonly_file` | covered |
| `errno` = `EBADF`: "The fildes argument is not a valid descriptor." | `unistd_fsync.fsync_ebadf_invalid_fd` | covered |
| EBADF on closed fd | `unistd_fsync.fsync_ebadf_closed_fd` | covered |
| `errno` = `EINVAL`: "The fildes argument does not refer to a file on which this operation is possible." | `unistd_fsync.fsync_einval_socket` | covered |
| `errno` = `EINTR`: "The fsync() function was interrupted by a signal." | — | not tested: requires precise signal timing during I/O |
| `errno` = `EIO`: "An I/O error occurred" | — | not tested: cannot reliably trigger I/O error |

## clock_gettime() — `time/clock_gettime.c`

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "clock_gettime() shall return the current value tp for the specified clock, clock_id." | `time_clock_gettime.clock_gettime_realtime_success` | covered |
| "All implementations shall support a clock_id of CLOCK_REALTIME" | `time_clock_gettime.clock_gettime_realtime_success` | covered |
| "all implementations shall support a clock_id of CLOCK_MONOTONIC" | `time_clock_gettime.clock_gettime_monotonic_success` | covered |
| CLOCK_MONOTONIC never decreases | `time_clock_gettime.clock_gettime_monotonic_nondecreasing` | covered |
| CLOCK_REALTIME advances over time | `time_clock_gettime.clock_gettime_realtime_advances` | covered |
| tv_nsec in valid range [0, 999999999] | `time_clock_gettime.clock_gettime_nsec_range` | covered |
| "A return value of 0 shall indicate that the call succeeded." | `time_clock_gettime.clock_gettime_realtime_success` | covered |
| `errno` = `EINVAL`: "The clock_id argument does not specify a known clock." | `time_clock_gettime.clock_gettime_einval_invalid_clock` | covered |
| `errno` = `EOVERFLOW`: "The number of seconds will not fit in an object of type time_t." | — | not tested: cannot trigger on systems with 64-bit time_t |

## clock_settime() — `time/clock_gettime.c`

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "clock_settime() shall set the specified clock, clock_id, to the value specified by tp." | `time_clock_settime.clock_settime_realtime_success` | covered |
| "A return value of 0 shall indicate that the call succeeded." | `time_clock_settime.clock_settime_realtime_success` | covered |
| `errno` = `EINVAL`: "The clock_id argument does not specify a known clock." | `time_clock_settime.clock_settime_einval_invalid_clock` | covered |
| `errno` = `EINVAL`: "The tp argument specified a nanosecond value less than zero" | `time_clock_settime.clock_settime_einval_negative_nsec` | covered |
| `errno` = `EINVAL`: "nanosecond value ... greater than or equal to 1000 million." | `time_clock_settime.clock_settime_einval_nsec_too_large` | covered |
| `errno` = `EINVAL`: "The value of the clock_id argument is CLOCK_MONOTONIC." | `time_clock_settime.clock_settime_einval_monotonic` | covered |

## clock_nanosleep() — `time/clock_nanosleep.c`

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "If the flag TIMER_ABSTIME is not set ... clock_nanosleep() shall cause the current thread to be suspended ... until ... the time interval specified by the rqtp argument has elapsed" | `time_clock_nanosleep.clock_nanosleep_relative_success` | covered |
| Zero-duration relative sleep returns immediately | `time_clock_nanosleep.clock_nanosleep_relative_zero` | covered |
| "If the flag TIMER_ABSTIME is set ... suspended ... until ... the time value of the clock ... reaches the absolute time specified by the rqtp argument" | `time_clock_nanosleep.clock_nanosleep_absolute_success` | covered |
| "if ... the time value specified by rqtp is less than or equal to the time value of the specified clock, then clock_nanosleep() shall return immediately" | `time_clock_nanosleep.clock_nanosleep_absolute_past` | covered |
| Relative sleep with CLOCK_REALTIME equivalent to nanosleep | `time_clock_nanosleep.clock_nanosleep_realtime` | covered |
| "If the clock_nanosleep() function returns because the requested time has elapsed, its return value shall be zero." | `time_clock_nanosleep.clock_nanosleep_relative_success` | covered |
| rmtp argument non-NULL (relative, uninterrupted) | `time_clock_nanosleep.clock_nanosleep_rmtp_updated` | covered |
| `EINVAL`: "rqtp argument specified a nanosecond value less than zero" | `time_clock_nanosleep.clock_nanosleep_einval_negative_nsec` | covered |
| `EINVAL`: "greater than or equal to 1000 million" | `time_clock_nanosleep.clock_nanosleep_einval_nsec_too_large` | covered |
| `EINVAL`: "clock_id argument does not specify a known clock" | `time_clock_nanosleep.clock_nanosleep_einval_invalid_clock` | covered |
| `EINTR`: "clock_nanosleep() was interrupted by a signal." | — | not tested: requires precise signal timing |
| `ENOTSUP`: "clock_id argument specifies a clock for which clock_nanosleep() is not supported" | — | not tested: no portable way to obtain such a clock_id |

## nanosleep() — `time/clock_nanosleep.c`

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "nanosleep() shall cause the current thread to be suspended ... until ... the time interval specified by the rqtp argument has elapsed" | `time_nanosleep.nanosleep_success` | covered |
| "the suspension time shall not be less than the time specified by rqtp, as measured by the system clock CLOCK_REALTIME." | `time_nanosleep.nanosleep_minimum_duration` | covered |
| Zero-duration sleep returns immediately | `time_nanosleep.nanosleep_zero` | covered |
| "its return value shall be zero" on success | `time_nanosleep.nanosleep_success` | covered |
| "The rqtp and rmtp arguments can point to the same object." | `time_nanosleep.nanosleep_rmtp_same_object` | covered |
| rmtp may be NULL | `time_nanosleep.nanosleep_rmtp_null` | covered |
| `errno` = `EINVAL`: "rqtp argument specified a nanosecond value less than zero" | `time_nanosleep.nanosleep_einval_negative_nsec` | covered |
| `errno` = `EINVAL`: "greater than or equal to 1000 million" | `time_nanosleep.nanosleep_einval_nsec_too_large` | covered |
| `errno` = `EINTR`: "nanosleep() was interrupted by a signal." | — | not tested: requires precise signal timing |
