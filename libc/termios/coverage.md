# Coverage: termios module

## tcdrain()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall wait until all output written to the object referred to by fildes is transmitted" | `termios_tcdrain.tcdrain_success_pty` | covered |
| "Upon successful completion, 0 shall be returned" | `termios_tcdrain.tcdrain_success_pty` | covered |
| "Otherwise, -1 shall be returned and errno set to indicate the error" | `termios_tcdrain.tcdrain_ebadf_invalid` | covered |
| [EBADF]: "The fildes argument is not a valid file descriptor" | `termios_tcdrain.tcdrain_ebadf_invalid` | covered |
| [ENOTTY]: "The file associated with fildes is not a terminal" | `termios_tcdrain.tcdrain_enotty_regular_file` | covered |
| [EINTR]: "A signal interrupted tcdrain()" | — | not tested: requires precise timing of signal delivery during drain |

## tcflow()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall suspend or restart transmission or reception of data on the object referred to by fildes, depending on the value of action" | `termios_tcflow.tcflow_tcooff` | covered |
| TCOOFF: "Suspend output" | `termios_tcflow.tcflow_tcooff` | covered |
| TCOON: "Restart suspended output" | `termios_tcflow.tcflow_tcoon` | covered |
| TCIOFF: "Transmit a STOP character, which is intended to cause the terminal device to stop transmitting data" | `termios_tcflow.tcflow_tcioff` | covered |
| TCION: "Transmit a START character, which is intended to cause the terminal device to start transmitting data" | `termios_tcflow.tcflow_tcion` | covered |
| "Upon successful completion, 0 shall be returned" | `termios_tcflow.tcflow_tcooff` | covered |
| "Otherwise, -1 shall be returned and errno set to indicate the error" | `termios_tcflow.tcflow_ebadf` | covered |
| [EBADF]: "The fildes argument is not a valid file descriptor" | `termios_tcflow.tcflow_ebadf` | covered |
| [EINVAL]: "The action argument is not a supported value" | `termios_tcflow.tcflow_einval_bad_action` | covered |
| [ENOTTY]: "The file associated with fildes is not a terminal" | `termios_tcflow.tcflow_enotty_regular_file` | covered |

## tcflush()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall discard data written to the object referred to by fildes ... or data received but not yet read" | `termios_tcflush.tcflush_tciflush` | covered |
| TCIFLUSH: "Flush data received but not read" | `termios_tcflush.tcflush_tciflush` | covered |
| TCOFLUSH: "Flush data written but not transmitted" | `termios_tcflush.tcflush_tcoflush` | covered |
| TCIOFLUSH: "Flush both data received but not read and data written but not transmitted" | `termios_tcflush.tcflush_tcioflush` | covered |
| "Upon successful completion, 0 shall be returned" | `termios_tcflush.tcflush_tciflush` | covered |
| "Otherwise, -1 shall be returned and errno set to indicate the error" | `termios_tcflush.tcflush_ebadf` | covered |
| [EBADF]: "The fildes argument is not a valid file descriptor" | `termios_tcflush.tcflush_ebadf` | covered |
| [EINVAL]: "The queue_selector argument is not a supported value" | `termios_tcflush.tcflush_einval_bad_selector` | covered |
| [ENOTTY]: "The file associated with fildes is not a terminal" | `termios_tcflush.tcflush_enotty_regular_file` | covered |

## tcgetsid()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall return the process group ID of the session for which the terminal specified by fildes is the controlling terminal" | — | not tested: requires controlling terminal association in test environment |
| "Upon successful completion, tcgetsid() shall return the process group ID ... as a value of type pid_t" | — | not tested: requires controlling terminal |
| "Otherwise, a value of (pid_t)-1 shall be returned and errno set to indicate the error" | `termios_tcgetsid.tcgetsid_ebadf` | covered |
| [EBADF]: "The fildes argument is not a valid file descriptor" | `termios_tcgetsid.tcgetsid_ebadf` | covered |
| [ENOTTY]: "The file is not a terminal" | `termios_tcgetsid.tcgetsid_enotty_regular_file` | covered |
| [ENOTTY]: "The calling process does not have a controlling terminal, or fildes does not refer to the controlling terminal" | `termios_tcgetsid.tcgetsid_enotty_noctty_pty` | covered |

## tcsetattr()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall set the parameters associated with the terminal ... from the termios structure referenced by termios_p" | `termios_tcsetattr.tcsetattr_tcsanow` | covered |
| TCSANOW: "The change shall occur immediately" | `termios_tcsetattr.tcsetattr_tcsanow` | covered |
| TCSADRAIN: "The change shall occur after all output written to fildes is transmitted" | `termios_tcsetattr.tcsetattr_tcsadrain` | covered |
| TCSAFLUSH: "The change shall occur after all output written ... is transmitted, and all input so far received but not read shall be discarded" | `termios_tcsetattr.tcsetattr_tcsaflush` | covered |
| "Upon successful completion, 0 shall be returned" | `termios_tcsetattr.tcsetattr_tcsanow` | covered |
| "Otherwise, -1 shall be returned and errno set to indicate the error" | `termios_tcsetattr.tcsetattr_ebadf` | covered |
| "the termios structure ... shall not be modified" by the call | `termios_tcsetattr.tcsetattr_preserves_struct` | covered |
| [EBADF]: "The fildes argument is not a valid file descriptor" | `termios_tcsetattr.tcsetattr_ebadf` | covered |
| [EINVAL]: "The optional_actions argument is not a supported value" | `termios_tcsetattr.tcsetattr_einval_bad_action` | covered |
| [ENOTTY]: "The file associated with fildes is not a terminal" | `termios_tcsetattr.tcsetattr_enotty_regular_file` | covered |

## tcgetpgrp()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall return the value of the process group ID of the foreground process group associated with the terminal" | — | not tested: requires controlling terminal with foreground process group |
| "Upon successful completion, tcgetpgrp() shall return the value of the process group ID of the foreground process group" | — | not tested: requires controlling terminal |
| "Otherwise, -1 shall be returned and errno set to indicate the error" | `termios_tcgetpgrp.tcgetpgrp_ebadf` | covered |
| [EBADF]: "The fildes argument is not a valid file descriptor" | `termios_tcgetpgrp.tcgetpgrp_ebadf` | covered |
| [ENOTTY]: "The calling process does not have a controlling terminal, or the file is not the controlling terminal" | `termios_tcgetpgrp.tcgetpgrp_enotty_noctty_pty` | covered |
| [ENOTTY]: "fildes does not refer to a terminal" | `termios_tcgetpgrp.tcgetpgrp_enotty_regular_file` | covered |

## tcsetpgrp()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall set the foreground process group ID associated with the terminal to pgid_id" | — | not tested: requires controlling terminal |
| "the process group to be set must ... be in the same session as the calling process" | — | not tested: requires controlling terminal |
| "Upon successful completion, 0 shall be returned" | — | not tested: requires controlling terminal |
| "Otherwise, -1 shall be returned and errno set to indicate the error" | `termios_tcsetpgrp.tcsetpgrp_ebadf` | covered |
| [EBADF]: "The fildes argument is not a valid file descriptor" | `termios_tcsetpgrp.tcsetpgrp_ebadf` | covered |
| [ENOTTY]: "The calling process does not have a controlling terminal, or the file is not the controlling terminal" | `termios_tcsetpgrp.tcsetpgrp_enotty_noctty_pty` | covered |
| [ENOTTY]: "fildes does not refer to a terminal" | `termios_tcsetpgrp.tcsetpgrp_enotty_regular_file` | covered |
| [EINVAL]: "The value of pgid_id is not supported" | — | not tested: cannot portably create an invalid pgid value that passes other checks |
| [EPERM]: "The value of pgid_id is ... not in the same session as the calling process" | — | not tested: unreliable in container/Docker environments without controlling terminal |

# Coverage: controlling terminal (`termios_ctty`)

These cases acquire a pty as the controlling terminal of a new session
(`setsid()` + `TIOCSCTTY`) and so reach the rules that the cases above, which
only use EBADF and a regular file, cannot.

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| tcgetpgrp: [ENOTTY] "The calling process does not have a controlling terminal" | `termios_ctty.tcgetpgrp_enotty_without_ctty` | covered |
| tcgetsid: [ENOTTY] "The calling process does not have a controlling terminal, or fildes does not refer to the controlling terminal" | `termios_ctty.tcgetsid_enotty_without_ctty` | covered |
| tcsetpgrp: "shall set the foreground process group ID" / tcgetpgrp: "shall return the value of the process group ID of the foreground process group" | `termios_ctty.tcsetpgrp_then_tcgetpgrp_roundtrip` | covered |
| tcgetsid: "shall return the process group ID of the session for which the terminal specified by fildes is the controlling terminal" | `termios_ctty.tcgetsid_reports_session_not_foreground_group` | covered |
| tcsetpgrp: [EINVAL] "The value of the pgid_id argument is not supported" | `termios_ctty.tcsetpgrp_einval_non_positive_pgid` | covered for a negative pgid. pgid 0 is not asserted: Linux rejects only pgid < 0 with EINVAL and lets 0 reach the lookup, which reports ESRCH, while libtty refuses everything <= 0 with EINVAL. |
| tcsetpgrp: [EPERM] "The value of pgid_id is ... not in the same session as the calling process" | `termios_ctty.tcsetpgrp_eperm_pgid_in_other_session` | covered |
| tcsetpgrp/tcgetpgrp: [ENOTTY] "the file is not the controlling terminal" (caller outside the owning session) | `termios_ctty.tcsetpgrp_refused_from_other_session` | covered |
| A terminal given up with TIOCNOTTY is no longer any session's controlling terminal | `termios_ctty.tiocnotty_releases_terminal` | covered |
| "the process group to be set must ... be in the same session as the calling process" | `termios_ctty.tcsetpgrp_eperm_pgid_in_other_session` | covered |
| A terminal whose owning session has ended can be claimed and used by a later session | `termios_ctty.ctty_reclaimed_after_session_leader_exits` | covered |
| setsid: "The process shall have no controlling terminal" - an inherited one is given up | `termios_ctty.setsid_releases_inherited_ctty` | covered |
| A terminal owned by a living session cannot be taken over by another session | `termios_ctty.ctty_cannot_be_stolen_from_a_live_session` | covered (the refusal, not the errno: Linux reports EPERM, libtty ENOTTY, and POSIX specifies neither) |
| Only a session leader may acquire a controlling terminal for its session | `termios_ctty.tiocsctty_refused_from_non_leader` | covered |
| _exit: "If the process is a controlling process, the SIGHUP signal shall be sent to each process in the foreground process group of the controlling terminal" | `termios_ctty.sighup_to_foreground_group_when_controlling_process_exits` | not implemented: the test is written and passes on Linux, but is `TEST_IGNORE`d under `__phoenix__`. The kernel reports no exits to terminal drivers, so libtty only notices a dead session leader lazily, on the next ctty ioctl. |
| Hangup dissociates the terminal from its session, so the leader may acquire another one | `pty_ctty.ctty_released_by_hangup` (libc/posixsrv) | covered for ptys, which are the only hangup source in the tree; `_libtty_hangup()` signals the foreground group and releases the session's claim. |
| tcsetpgrp: [EPERM] pgid_id does not match the process group ID of any process in the session | `termios_ctty.tcsetpgrp_eperm_pgid_without_any_process` | covered on Phoenix only: POSIX mandates EPERM for a pgid that names no process group, but Linux accepts a plain pid there (`session_of_pgrp()` falls back to the pid) and reports ESRCH when even that finds nothing, so the case is compiled in under `__phoenix__`. |
| Background terminal access raises SIGTTIN/SIGTTOU or fails with EIO | — | not tested: the default disposition of both signals is to stop the process, which libtty does not implement yet (`TODO: SIGTTIN/SIGTTOU?` in `_libtty_cttyIoctl()`). The cases above deliberately keep the caller in the foreground group so as not to depend on it. |
