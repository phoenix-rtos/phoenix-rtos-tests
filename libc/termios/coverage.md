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
