# syslog coverage

Functions: `closelog()`, `openlog()`, `setlogmask()`, `syslog()`

## closelog()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "closelog() shall close any open file descriptors allocated by previous calls to openlog() or syslog()" | `syslog_closelog.closelog_after_openlog` | covered |
| closelog() closes fds allocated by syslog() alone (without prior openlog()) | `syslog_closelog.closelog_after_syslog_without_openlog` | covered |
| closelog() shall not return a value | — | covered (void return, compile-time) |

## openlog()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "The ident argument is a string that is prepended to every message" | `syslog_openlog.openlog_with_ident` | covered |
| "LOG_PID: Log the process ID with each message" | `syslog_openlog.openlog_log_pid_option` | covered |
| "LOG_CONS: Write messages to the system console if they cannot be sent to the logging facility" | `syslog_openlog.openlog_log_cons_option` | covered |
| "LOG_NDELAY: Open the connection to the logging facility immediately" | `syslog_openlog.openlog_log_ndelay_option` | covered |
| "LOG_ODELAY: Delay open until syslog() is called" | `syslog_openlog.openlog_log_odelay_option` | covered |
| "LOG_NOWAIT: Do not wait for child processes that may have been created during the course of logging the message" | `syslog_openlog.openlog_log_nowait_option` | covered |
| "The facility argument encodes a default facility to be assigned to all messages that do not have an explicit facility already encoded" | `syslog_openlog.openlog_facility_log_local0_through_local7` | covered |
| "The initial default facility is LOG_USER" | `syslog_openlog.openlog_facility_log_user` | covered |
| "It is not necessary to call openlog() prior to calling syslog()" | `syslog_syslog.syslog_without_prior_openlog` | covered |
| "The openlog() and syslog() functions may allocate a file descriptor" | `syslog_closelog.closelog_after_syslog_without_openlog` | covered |
| openlog() shall not return a value | — | covered (void return, compile-time) |

## setlogmask()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "setlogmask() shall set the log priority mask for the current process to maskpri and return the previous mask" | `syslog_setlogmask.setlogmask_returns_previous_mask` | covered |
| "If the maskpri argument is 0, the current log mask is not modified" | `syslog_setlogmask.setlogmask_zero_does_not_modify` | covered |
| "Calls by the current process to syslog() with a priority not set in maskpri shall be rejected" | `syslog_syslog.syslog_masked_priority_not_logged` | covered |
| "The default log mask allows all priorities to be logged" | `syslog_setlogmask.setlogmask_default_allows_all` | covered |
| "A call to openlog() is not required prior to calling setlogmask()" | `syslog_setlogmask.setlogmask_no_openlog_required` | covered |
| setlogmask() shall return the previous log priority mask | `syslog_setlogmask.setlogmask_successive_calls_return_previous` | covered |

## syslog()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "syslog() shall send a message to an implementation-defined logging facility" | `syslog_syslog.syslog_does_not_crash_all_severities` | covered |
| "The message body is generated from the message and following arguments in the same manner as if these were arguments to printf()" | `syslog_syslog.syslog_printf_format_specifiers` | covered |
| "the additional conversion specification %m shall be recognized; it shall convert no arguments, shall cause the output of the error message string associated with the value of errno on entry to syslog()" | `syslog_syslog.syslog_percent_m_conversion` | covered |
| "Values of the priority argument are formed by OR'ing together a severity-level value and an optional facility value" | `syslog_syslog.syslog_with_facility_or` | covered |
| "If no facility value is specified, the current default facility value is used" | `syslog_syslog.syslog_default_facility_is_log_user` | covered |
| "It is not necessary to call openlog() prior to calling syslog()" | `syslog_syslog.syslog_without_prior_openlog` | covered |
| syslog() shall not return a value | — | covered (void return, compile-time) |
| "The logged message shall include a message header and a message body" | — | not tested: message content is sent to an implementation-defined logging facility; cannot portably inspect output |
| "The message header contains at least a timestamp and a tag string" | — | not tested: cannot portably inspect syslog output |
