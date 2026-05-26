# Coverage: sched_get_priority_max / sched_get_priority_min / sched_yield

## sched_get_priority_max / sched_get_priority_min

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall return the appropriate maximum or minimum, respectively, for the scheduling policy specified by policy" | `sched_get_priority.max_sched_fifo`, `sched_get_priority.min_sched_fifo` | covered |
| "The value of policy shall be one of the scheduling policy values defined in <sched.h>" (SCHED_FIFO) | `sched_get_priority.max_sched_fifo`, `sched_get_priority.min_sched_fifo` | covered |
| "The value of policy shall be one of the scheduling policy values defined in <sched.h>" (SCHED_RR) | `sched_get_priority.max_sched_rr`, `sched_get_priority.min_sched_rr` | covered |
| "The value of policy shall be one of the scheduling policy values defined in <sched.h>" (SCHED_OTHER) | `sched_get_priority.max_sched_other`, `sched_get_priority.min_sched_other` | covered |
| "If successful, … shall return the appropriate maximum or minimum values, respectively" (return value >= 0) | `sched_get_priority.max_sched_fifo` | covered |
| "If unsuccessful, they shall return a value of -1 and set errno to indicate the error" | `sched_get_priority.einval_invalid_policy` | covered |
| [EINVAL] "The value of the policy parameter does not represent a defined scheduling policy" | `sched_get_priority.einval_invalid_policy` | covered |
| max >= min for every valid policy (implicit from spec — maximum cannot be less than minimum) | `sched_get_priority.max_ge_min_all_policies` | covered |

## sched_yield

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall force the running thread to relinquish the processor until it again becomes the head of its thread list" | `sched_yield.returns_zero_on_success` | covered (yield completes without error; observable preemption is not testable portably) |
| "shall return 0 if it completes successfully" | `sched_yield.returns_zero_on_success` | covered |
| "otherwise, it shall return a value of -1 and set errno to indicate the error" / "No errors are defined" | — | not tested: no defined error conditions exist; cannot trigger failure portably |
| Repeated calls remain stable | `sched_yield.repeated_calls_succeed` | covered |
