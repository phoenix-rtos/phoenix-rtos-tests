# Coverage: time module (new tests in timer.c)

## clock_getcpuclockid()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall return the clock ID of the CPU-time clock of the process specified by pid" | `time_clock_getcpuclockid.getcpuclockid_own_pid` | covered |
| "If pid is zero, the clock_getcpuclockid() function shall return the clock ID of the CPU-time clock of the process making the call" | `time_clock_getcpuclockid.getcpuclockid_self` | covered |
| "Upon successful completion, clock_getcpuclockid() shall return zero" | `time_clock_getcpuclockid.getcpuclockid_self` | covered |
| Returned clock_id can be used with clock_gettime() | `time_clock_getcpuclockid.getcpuclockid_can_be_used_with_gettime` | covered |
| [EPERM]: "The requesting process does not have permission to access the CPU-time clock for the process" | — | not tested: cannot portably trigger permission denial for own process |

## clock_getres()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall return the resolution of any clock" | `time_clock_getres.getres_realtime` | covered |
| "If the argument res is not NULL, the resolution of the specified clock shall be stored in the location pointed to by res" | `time_clock_getres.getres_realtime` | covered |
| "If res is NULL, the clock resolution is not returned" (no crash) | `time_clock_getres.getres_null_res_ptr` | covered |
| Supports CLOCK_REALTIME | `time_clock_getres.getres_realtime` | covered |
| Supports CLOCK_MONOTONIC | `time_clock_getres.getres_monotonic` | covered |
| "A return value of 0 shall indicate that the call succeeded" | `time_clock_getres.getres_realtime` | covered |
| "A return value of -1 shall indicate that an error occurred, and errno shall be set" | `time_clock_getres.getres_einval_bad_clockid` | covered |
| [EINVAL]: "The clock_id argument does not specify a known clock" | `time_clock_getres.getres_einval_bad_clockid` | covered |

## timer_create()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall create a per-process timer using the specified clock, clock_id, as the timing base" | `time_timer_create.timer_create_realtime_default` | covered |
| "shall return, in the location referenced by timerid, a timer ID of type timer_t" | `time_timer_create.timer_create_realtime_default` | covered |
| "The timer whose ID is returned shall be in a disarmed state upon return from timer_create()" | `time_timer_gettime.gettime_disarmed_timer` | covered |
| "All implementations shall support a clock_id of CLOCK_REALTIME" | `time_timer_create.timer_create_realtime_default` | covered |
| "If the Monotonic Clock option is supported, implementations shall support a clock_id of CLOCK_MONOTONIC" | `time_timer_create.timer_create_monotonic` | covered |
| "If the call succeeds, timer_create() shall return zero" | `time_timer_create.timer_create_realtime_default` | covered |
| "If an error occurs, the function shall return a value of -1 and set errno" | `time_timer_create.timer_create_einval_bad_clockid` | covered |
| [EINVAL]: "The specified clock ID is not defined" | `time_timer_create.timer_create_einval_bad_clockid` | covered |
| evp with SIGEV_NONE notification | `time_timer_create.timer_create_sigev_none` | covered |

## timer_delete()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall delete the specified timer, timerid, previously created by the timer_create() function" | `time_timer_create.timer_create_realtime_default` | covered |
| "If the timer is armed when timer_delete() is called, the behavior shall be as if the timer is automatically disarmed before removal" | `time_timer_create.timer_create_realtime_default` | covered |
| "If successful, the timer_delete() function shall return a value of zero" | `time_timer_create.timer_create_realtime_default` | covered |

## timer_settime()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall set the time until the next expiration of the timer specified by timerid from the it_value member of the value argument and arm the timer" | `time_timer_settime.settime_arm_relative` | covered |
| "If the it_value member of value is zero, the timer shall be disarmed" | `time_timer_settime.settime_disarm` | covered |
| "If the flag TIMER_ABSTIME is not set ... the timer shall expire in it_value nanoseconds from when the call is made" | `time_timer_settime.settime_arm_relative` | covered |
| "When a timer is armed with a non-zero it_interval, a periodic (or repetitive) timer is specified" | `time_timer_settime.settime_periodic` | covered |
| "If the argument ovalue is not NULL ... shall store ... a value representing the previous amount of time before the timer would have expired" | `time_timer_settime.settime_returns_old_value` | covered |
| "If the timer_gettime() or timer_settime() functions succeed, a value of 0 shall be returned" | `time_timer_settime.settime_arm_relative` | covered |
| "If an error occurs ... the value -1 shall be returned, and errno set" | `time_timer_settime.settime_einval_bad_nsec` | covered |
| [EINVAL]: "A value structure specified a nanosecond value less than zero or greater than or equal to 1000 million" | `time_timer_settime.settime_einval_bad_nsec` | covered |
| [EINVAL]: nsec >= 1000000000 | `time_timer_settime.settime_einval_nsec_too_large` | covered |

## timer_gettime()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall store the amount of time until the specified timer ... expires and the reload value of the timer into the space pointed to by the value argument" | `time_timer_gettime.gettime_armed_timer_remaining` | covered |
| "The it_value member ... shall contain the amount of time before the timer expires, or zero if the timer is disarmed" | `time_timer_gettime.gettime_disarmed_timer` | covered |
| "The it_interval member of value shall contain the reload value last set by timer_settime()" | `time_timer_settime.settime_periodic` | covered |
| "If ... timer_gettime() ... succeed, a value of 0 shall be returned" | `time_timer_gettime.gettime_disarmed_timer` | covered |

## timer_getoverrun()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "the timer_getoverrun() function shall return the timer expiration overrun count for the specified timer" | `time_timer_getoverrun.getoverrun_returns_nonnegative` | covered |
| "If the timer_getoverrun() function succeeds, it shall return the timer expiration overrun count" | `time_timer_getoverrun.getoverrun_returns_nonnegative` | covered |
| Overrun count with rapid expirations | — | not tested: requires precise signal delivery timing to trigger actual overruns |
