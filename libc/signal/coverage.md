# sigaction() — POSIX.1-2017 Test Coverage

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "allows the calling process to examine and/or specify the action to be associated with a specific signal" | `signal_sigaction.install_handler` | covered |
| "If the argument act is not a null pointer, it points to a structure specifying the action to be associated with the specified signal" | `signal_sigaction.install_handler` | covered |
| "If the argument oact is not a null pointer, the action previously associated with the signal is stored in the location pointed to by the argument oact" | `signal_sigaction.retrieve_previous_action` | covered |
| "If the argument act is a null pointer, signal handling is unchanged; thus, the call can be used to enquire about the current handling of a given signal" | `signal_sigaction.query_current_action` | covered |
| "The SIGKILL and SIGSTOP signals shall not be added to the signal mask using this mechanism; this restriction shall be enforced by the system without causing an error to be indicated" | `signal_sigaction.sigkill_sigstop_not_in_sa_mask` | covered |
| "If the SA_SIGINFO flag is cleared in the sa_flags field, the sa_handler field identifies the action to be associated with the specified signal" | `signal_sigaction.install_handler` | covered |
| "If the SA_SIGINFO flag is set in the sa_flags field, the sa_sigaction field specifies a signal-catching function" | `signal_sigaction.sa_siginfo_handler` | covered |
| SA_NODEFER: "If set and sig is caught, sig shall not be added to the thread's signal mask on entry to the signal handler unless it is included in sa_mask" | `signal_sigaction.sa_nodefer` | covered |
| Default (no SA_NODEFER): "sig shall always be added to the thread's signal mask on entry to the signal handler" | `signal_sigaction.caught_signal_blocked_in_handler` | covered |
| SA_RESETHAND: "the disposition of the signal shall be reset to SIG_DFL and the SA_SIGINFO flag shall be cleared on entry to the signal handler" | `signal_sigaction.sa_resethand` | covered |
| SA_RESETHAND: "SA_SIGINFO flag shall be cleared" | `signal_sigaction.sa_resethand_clears_siginfo` | covered |
| "a new signal mask is calculated and installed for the duration of the signal-catching function... formed by taking the union of the current signal mask and the value of the sa_mask for the signal being delivered" | `signal_sigaction.sa_mask_blocked_in_handler` | covered |
| "unless SA_NODEFER or SA_RESETHAND is set, then including the signal being delivered" | `signal_sigaction.caught_signal_blocked_in_handler` | covered |
| "If and when the user's signal handler returns normally, the original signal mask is restored" | `signal_sigaction.mask_restored_after_handler` | covered |
| "Once an action is installed for a specific signal, it shall remain installed until another action is explicitly requested" | `signal_sigaction.action_persists` | covered |
| "If sigaction() fails, no new signal handler is installed" | `signal_sigaction.failure_no_change` | covered |
| "Upon successful completion, sigaction() shall return 0" | `signal_sigaction.install_handler` | covered |
| "otherwise, -1 shall be returned, errno shall be set to indicate the error" | `signal_sigaction.einval_zero_signal` | covered |
| EINVAL: "The sig argument is not a valid signal number" | `signal_sigaction.einval_zero_signal` | covered |
| EINVAL: invalid signal — negative | `signal_sigaction.einval_negative_signal` | covered |
| EINVAL: invalid signal — exceeds range | `signal_sigaction.einval_large_signal` | covered |
| EINVAL: "an attempt is made to catch a signal that cannot be caught" (SIGKILL) | `signal_sigaction.einval_catch_sigkill` | covered |
| EINVAL: "an attempt is made to catch a signal that cannot be caught" (SIGSTOP) | `signal_sigaction.einval_catch_sigstop` | covered |
| EINVAL: "ignore a signal that cannot be ignored" (SIGKILL) | `signal_sigaction.einval_ignore_sigkill` | covered |
| EINVAL: "ignore a signal that cannot be ignored" (SIGSTOP) | `signal_sigaction.einval_ignore_sigstop` | covered |
| SIG_IGN causes signal to be ignored | `signal_sigaction.sig_ign` | covered |
| SIG_DFL restores default disposition | `signal_sigaction.sig_dfl_restore` | covered |
| SA_SIGINFO: "si_signo member contains the system-generated signal number" | `signal_sigaction.sa_siginfo_si_signo` | covered |
| SA_NODEFER: sa_mask signals still blocked | `signal_sigaction.sa_nodefer_sa_mask_still_applied` | covered |
| Works for all valid catchable signals | `signal_sigaction.all_catchable_signals` | covered |
| SA_SIGINFO flag preserved in oact | `signal_sigaction.sa_siginfo_flag_preserved` | covered |
| sa_mask preserved in oact | `signal_sigaction.sa_mask_preserved_in_oact` | covered |
| Querying SIGKILL/SIGSTOP with act=NULL is valid | `signal_sigaction.query_sigkill_allowed` | covered |
| Replacing a handler with a new sigaction() call | `signal_sigaction.replace_handler` | covered |
| SA_NOCLDSTOP: "Do not generate SIGCHLD when children stop" | — | not tested: requires job control and child process stopping which is unreliable to trigger portably |
| SA_ONSTACK: "signal shall be delivered on alternate stack" | — | not tested: requires sigaltstack() which is XSI extension |
| SA_RESTART: "interruptible functions interrupted by this signal shall restart" | — | not tested: requires reliable timing of signal delivery during blocking call; inherently racy |
| SA_NOCLDWAIT: zombie avoidance for SIGCHLD | — | not tested: XSI extension |
