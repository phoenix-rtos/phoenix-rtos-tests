# Coverage: pthread tests

## pthread_attr_init / pthread_attr_destroy

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall initialize a thread attributes object attr with the default value for all of the individual attributes" | `pthread_attr.attr_init_success` | covered |
| "A single attributes object can be used in multiple simultaneous calls to pthread_create()" | `pthread_attr.attr_reuse_across_creates` | covered |
| pthread_attr_destroy "shall destroy a thread attributes object" returning 0 | `pthread_attr.attr_destroy_success` | covered |
| "A destroyed attr attributes object can be reinitialized using pthread_attr_init()" | `pthread_attr.attr_destroy_reinit` | covered |
| pthread_attr_init shall fail with [ENOMEM] | — | not tested: cannot reliably exhaust memory |

## pthread_attr_getdetachstate / pthread_attr_setdetachstate

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "The default value of the detachstate attribute shall be PTHREAD_CREATE_JOINABLE" | `pthread_attr.attr_detachstate_default_joinable` | covered |
| "shall get … the detachstate attribute" set to PTHREAD_CREATE_DETACHED or PTHREAD_CREATE_JOINABLE | `pthread_attr.attr_setdetachstate_detached` | covered |
| "A value of PTHREAD_CREATE_JOINABLE shall cause all threads created with attr to be in the joinable state" | `pthread_attr.attr_setdetachstate_joinable` | covered |
| pthread_attr_setdetachstate shall fail with [EINVAL] "The value of detachstate was not valid" | `pthread_attr.attr_setdetachstate_einval` | covered |

## pthread_attr_getschedparam / pthread_attr_setschedparam

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall get and set the scheduling parameter attributes" | `pthread_attr.attr_schedparam_roundtrip` | covered |
| pthread_attr_setschedparam shall fail with [ENOTSUP] "An attempt was made to set the attribute to an unsupported value" | — | not tested: cannot portably determine unsupported sched_priority |

## pthread_attr_getschedpolicy / pthread_attr_setschedpolicy

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall get and set the schedpolicy attribute" supporting SCHED_FIFO | `pthread_attr.attr_schedpolicy_fifo` | covered |
| supporting SCHED_RR | `pthread_attr.attr_schedpolicy_rr` | covered |
| supporting SCHED_OTHER | `pthread_attr.attr_schedpolicy_other` | covered |
| shall fail with [ENOTSUP] for unsupported value | `pthread_attr.attr_setschedpolicy_enotsup` | covered |

## pthread_attr_getscope

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall get … the contentionscope attribute" (PTHREAD_SCOPE_SYSTEM or PTHREAD_SCOPE_PROCESS) | `pthread_attr.attr_getscope_default` | covered |

## pthread_attr_getstack / pthread_attr_setstack (partial — pthread_attr_getstack only listed)

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall get … the thread creation stack attributes stackaddr and stacksize" | `pthread_attr.attr_getstack_after_setstack` | covered |
| pthread_attr_setstack shall fail with [EINVAL] "The value of stacksize is less than {PTHREAD_STACK_MIN}" | `pthread_attr.attr_setstack_einval_small_size` | covered |

## pthread_attr_getstacksize / pthread_attr_setstacksize

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall get … the thread creation stacksize attribute" | `pthread_attr.attr_getstacksize_default` | covered |
| stacksize >= PTHREAD_STACK_MIN | `pthread_attr.attr_getstacksize_default` | covered |
| "shall set the thread creation stacksize attribute" (PTHREAD_STACK_MIN) | `pthread_attr.attr_setstacksize_min` | covered |
| "shall set the thread creation stacksize attribute" (large value) | `pthread_attr.attr_setstacksize_large` | covered |
| shall fail with [EINVAL] "The value of stacksize is less than {PTHREAD_STACK_MIN}" | `pthread_attr.attr_setstacksize_einval_too_small` | covered |

## pthread_mutex_init / pthread_mutex_destroy

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall initialize the mutex referenced by mutex with attributes specified by attr" (NULL = default) | `pthread_mutex.mutex_init_default` | covered |
| init with explicit attributes | `pthread_mutex.mutex_init_with_attr` | covered |
| "PTHREAD_MUTEX_INITIALIZER … equivalent to dynamic initialization … with parameter attr specified as NULL" | `pthread_mutex.mutex_initializer_static` | covered |
| "It shall be safe to destroy an initialized mutex that is unlocked" | `pthread_mutex.mutex_destroy_unlocked` | covered |
| "A destroyed mutex object can be reinitialized using pthread_mutex_init()" | `pthread_mutex.mutex_destroy_reinit` | covered |
| shall fail with [EAGAIN] | — | not tested: cannot exhaust system resources portably |
| shall fail with [ENOMEM] | — | not tested: cannot exhaust memory portably |
| shall fail with [EPERM] | — | not tested: cannot portably trigger privilege failure |

## pthread_mutex_lock / pthread_mutex_trylock / pthread_mutex_unlock

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall be locked by a call to pthread_mutex_lock() that returns zero" | `pthread_mutex.mutex_lock_unlock_basic` | covered |
| pthread_mutex_trylock "equivalent to pthread_mutex_lock(), except that if the mutex … is currently locked … the call shall return immediately" with [EBUSY] | `pthread_mutex.mutex_trylock_ebusy` | covered |
| trylock on unlocked mutex succeeds | `pthread_mutex.mutex_trylock_unlocked` | covered |
| ERRORCHECK: "error returned" on relock [EDEADLK] | `pthread_mutex.mutex_lock_errorcheck_edeadlk` | covered |
| ERRORCHECK: unlock when not owner returns [EPERM] | `pthread_mutex.mutex_unlock_errorcheck_eperm` | covered |
| RECURSIVE: "recursive … lock count shall be incremented" | `pthread_mutex.mutex_lock_recursive` | covered |
| RECURSIVE: trylock increments lock count | `pthread_mutex.mutex_trylock_recursive` | covered |
| RECURSIVE: unlock when not owner returns [EPERM] | `pthread_mutex.mutex_unlock_recursive_eperm` | covered |
| Mutual exclusion between threads | `pthread_mutex.mutex_lock_mutual_exclusion` | covered |

## pthread_mutexattr_init / pthread_mutexattr_destroy

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall initialize a mutex attributes object attr with the default value" returning 0 | `pthread_mutexattr.mutexattr_init_success` | covered |
| "shall destroy a mutex attributes object" returning 0 | `pthread_mutexattr.mutexattr_destroy_success` | covered |
| "A destroyed attr attributes object can be reinitialized using pthread_mutexattr_init()" | `pthread_mutexattr.mutexattr_destroy_reinit` | covered |
| "any function affecting the attributes object … shall not affect any previously initialized mutexes" | `pthread_mutexattr.mutexattr_not_affected_after_init` | covered |
| shall fail with [ENOMEM] | — | not tested: cannot exhaust memory portably |

## pthread_mutexattr_gettype / pthread_mutexattr_settype

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "The default value of the type attribute is PTHREAD_MUTEX_DEFAULT" | `pthread_mutexattr.mutexattr_gettype_default` | covered |
| set PTHREAD_MUTEX_NORMAL | `pthread_mutexattr.mutexattr_settype_normal` | covered |
| set PTHREAD_MUTEX_ERRORCHECK | `pthread_mutexattr.mutexattr_settype_errorcheck` | covered |
| set PTHREAD_MUTEX_RECURSIVE | `pthread_mutexattr.mutexattr_settype_recursive` | covered |
| shall fail with [EINVAL] "The value type is invalid" | `pthread_mutexattr.mutexattr_settype_einval` | covered |

## pthread_cond_broadcast / pthread_cond_signal

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall unblock all threads currently blocked on the specified condition variable cond" | `test_pthread_cond.pthread_cond_wait_broadcast` | covered |
| "shall unblock at least one of the threads that are blocked on the specified condition variable" | `test_pthread_cond.pthread_cond_wait_signal` | covered |
| "shall have no effect if there are no threads currently blocked on cond" | — | not tested: no observable side-effect to verify |

## pthread_cond_destroy / pthread_cond_init (partial — pthread_cond_init tested)

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall initialize the condition variable referenced by cond" returning 0 | `test_pthread_cond.pthread_cond_init` | covered |

## pthread_condattr_init / pthread_condattr_destroy

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall initialize a condition variable attributes object attr with the default value" | `test_pthread_cond.pthread_condattr_setclock` (uses condattr_init) | covered |
| shall fail with [ENOMEM] | — | not tested: cannot exhaust memory portably |

## pthread_condattr_setclock

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall set the clock attribute in an initialized attributes object" | `test_pthread_cond.pthread_condattr_setclock` | covered |

## pthread_self

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall return the thread ID of the calling thread" (always succeeds) | `pthread_lifecycle.self_returns_valid_id` | covered |

## pthread_equal

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall return a non-zero value if t1 and t2 are equal" | `pthread_lifecycle.equal_same_thread` | covered |
| "otherwise, zero shall be returned" | `pthread_lifecycle.equal_different_threads` | covered |

## pthread_exit

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall terminate the calling thread and make the value value_ptr available to any successful join" | `pthread_lifecycle.exit_value_available_via_join` | covered |
| "An implicit call to pthread_exit() is made when a thread … returns from the start routine" | `pthread_lifecycle.exit_implicit_on_return` | covered |

## pthread_join

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall suspend execution of the calling thread until the target thread terminates" | `pthread_lifecycle.join_suspends_until_termination` | covered |
| "the value passed to pthread_exit() … shall be made available in the location referenced by value_ptr" | `pthread_lifecycle.exit_value_available_via_join` | covered |
| "unless the target thread has already terminated" (returns immediately) | `pthread_lifecycle.join_already_terminated` | covered |
| NULL value_ptr acceptable | `pthread_lifecycle.join_null_value_ptr` | covered |

## pthread_detach

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall indicate to the implementation that storage for the thread thread can be reclaimed" returning 0 | `pthread_lifecycle.detach_joinable_thread` | covered |
| "If thread has not terminated, pthread_detach() shall not cause it to terminate" | `pthread_lifecycle.detach_joinable_thread` | covered |
| Thread created with PTHREAD_CREATE_DETACHED | `pthread_lifecycle.detach_created_detached` | covered |

## pthread_once

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "The first call … shall call the init_routine … Subsequent calls … shall not call the init_routine" | `pthread_once.once_called_once` | covered |
| Multiple threads — init_routine called only once | `pthread_once.once_multiple_threads` | covered |
| "On return from pthread_once(), init_routine shall have completed" | `pthread_once.once_completed_on_return` | covered |

## pthread_key_create

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall create a thread-specific data key visible to all threads" returning 0 | `pthread_key.key_create_success` | covered |
| "the value NULL shall be associated with the new key in all active threads" | `pthread_key.key_initial_value_null` | covered |
| "Upon thread creation, the value NULL shall be associated with all defined keys in the new thread" | `pthread_key.key_initial_value_null_in_new_thread` | covered |
| Destructor called at thread exit for non-NULL value | `pthread_key.key_destructor_called_on_thread_exit` | covered |
| Destructor not called if value is NULL | `pthread_key.key_destructor_not_called_for_null` | covered |
| Destructor receives previously associated value | `pthread_key.key_destructor_receives_value` | covered |
| shall fail with [EAGAIN] | — | not tested: cannot exhaust PTHREAD_KEYS_MAX portably |
| shall fail with [ENOMEM] | — | not tested: cannot exhaust memory portably |

## pthread_key_delete

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall delete a thread-specific data key" returning 0 | `pthread_key.key_delete_success` | covered |
| "No destructor functions shall be invoked by pthread_key_delete()" | `pthread_key.key_delete_no_destructor_call` | covered |

## pthread_getspecific

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall return the value currently bound to the specified key on behalf of the calling thread" | `pthread_key.key_setget_same_thread` | covered |
| "If no thread-specific data value is associated with key, then the value NULL shall be returned" | `pthread_key.key_initial_value_null` | covered |

## pthread_setspecific

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall associate a thread-specific value with a key" | `pthread_key.key_setget_same_thread` | covered |
| "Different threads may bind different values to the same key" | `pthread_key.key_per_thread_values` | covered |
| Overwrite previously set value | `pthread_key.key_overwrite_value` | covered |
| Set NULL value | `pthread_key.key_set_null` | covered |
| Multiple keys independent | `pthread_key.key_multiple_keys_independent` | covered |

## pthread_cancel

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall request that thread be canceled" returning 0 | `pthread_cancel.cancel_returns_zero` | covered |
| "the cancellation cleanup handlers for thread shall be called" | `pthread_cancel.cancel_invokes_cleanup_handlers` | covered |
| "thread shall be terminated" — exit status PTHREAD_CANCELED | `pthread_cancel.cancel_exit_status` | covered |
| "cancellation processing … shall run asynchronously with respect to the calling thread returning from pthread_cancel()" | `pthread_cancel.cancel_async_wrt_caller` | covered |

## pthread_setcancelstate

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall atomically both set the calling thread's cancelability state … and return the previous cancelability state" | `pthread_cancel.setcancelstate_returns_oldstate` | covered |
| PTHREAD_CANCEL_DISABLE prevents cancellation | `pthread_cancel.setcancelstate_disable` | covered |
| PTHREAD_CANCEL_ENABLE re-enables cancellation | `pthread_cancel.setcancelstate_enable` | covered |

## pthread_cleanup_push / pthread_cleanup_pop

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall be popped … and invoked with the argument arg when: The thread exits (calls pthread_exit())" | `test_pthread_cleanup.pthread_cleanup_push_exit` | covered |
| "The thread calls pthread_cleanup_pop() with a non-zero execute argument" | `test_pthread_cleanup.pthread_cleanup_push_pop_exec` | covered |
| pthread_cleanup_pop with execute=0 does not invoke handler | `test_pthread_cleanup.pthread_cleanup_push_pop_no_exec` | covered |
| "The thread acts upon a cancellation request" | `pthread_cancel.cancel_invokes_cleanup_handlers` | covered |

## pthread_atfork

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall declare fork handlers to be called before and after fork()" returning 0 | `pthread_atfork.atfork_returns_zero` | covered |
| "If no handling is desired … the corresponding fork handler address(es) may be set to NULL" | `pthread_atfork.atfork_null_handlers` | covered |
| "The prepare fork handler shall be called before fork() processing commences" | `pthread_atfork.atfork_handlers_called_on_fork` | covered |
| "The parent fork handle shall be called after fork() processing completes in the parent process" | `pthread_atfork.atfork_handlers_called_on_fork` | covered |
| "The child fork handler shall be called after fork() processing completes in the child process" | `pthread_atfork.atfork_handlers_called_on_fork` | covered |
| "The prepare fork handlers shall be called in the opposite order" | `pthread_atfork.atfork_prepare_reverse_order` | covered |
| "The parent and child fork handlers shall be called in the order in which they were established" | `pthread_atfork.atfork_prepare_reverse_order` / `pthread_atfork.atfork_child_registration_order` | covered |
| shall fail with [ENOMEM] | — | not tested: cannot exhaust table space portably |
