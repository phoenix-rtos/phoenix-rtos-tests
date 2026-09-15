/*
 * Phoenix-RTOS
 *
 * libc feature detection macros - GENERATED FILE, DO NOT EDIT.
 *
 * Regenerate with phoenix-rtos-tests/scripts/gen_libc_features.py
 * Prefixes: pthread_
 *
 * A HAS_<FUNC> macro is defined for every matching function that is
 * present in the scanned libc build. Missing functions leave the
 * corresponding macro undefined.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBC_FEATURES_H_
#define _LIBC_FEATURES_H_

/* clang-format off */

#define HAS_PTHREAD_ATFORK                   1
#define HAS_PTHREAD_ATTR_DESTROY             1
#define HAS_PTHREAD_ATTR_GETDETACHSTATE      1
#define HAS_PTHREAD_ATTR_GETGUARDSIZE        1
#define HAS_PTHREAD_ATTR_GETINHERITSCHED     1
#define HAS_PTHREAD_ATTR_GETSCHEDPARAM       1
#define HAS_PTHREAD_ATTR_GETSCHEDPOLICY      1
#define HAS_PTHREAD_ATTR_GETSCOPE            1
#define HAS_PTHREAD_ATTR_GETSTACK            1
#define HAS_PTHREAD_ATTR_GETSTACKADDR        1
#define HAS_PTHREAD_ATTR_GETSTACKSIZE        1
#define HAS_PTHREAD_ATTR_INIT                1
#define HAS_PTHREAD_ATTR_SETDETACHSTATE      1
#define HAS_PTHREAD_ATTR_SETGUARDSIZE        1
#define HAS_PTHREAD_ATTR_SETINHERITSCHED     1
#define HAS_PTHREAD_ATTR_SETSCHEDPARAM       1
#define HAS_PTHREAD_ATTR_SETSCHEDPOLICY      1
#define HAS_PTHREAD_ATTR_SETSCOPE            1
#define HAS_PTHREAD_ATTR_SETSTACK            1
#define HAS_PTHREAD_ATTR_SETSTACKADDR        1
#define HAS_PTHREAD_ATTR_SETSTACKSIZE        1
#define HAS_PTHREAD_CANCEL                   1
#define HAS_PTHREAD_CLEANUP_POP              1
#define HAS_PTHREAD_CLEANUP_PUSH             1
#define HAS_PTHREAD_COND_BROADCAST           1
#define HAS_PTHREAD_COND_CLOCKWAIT           1
#define HAS_PTHREAD_COND_CLOCKWAIT_PHX       1
#define HAS_PTHREAD_COND_DESTROY             1
#define HAS_PTHREAD_COND_INIT                1
#define HAS_PTHREAD_COND_SIGNAL              1
#define HAS_PTHREAD_COND_TIMEDWAIT           1
#define HAS_PTHREAD_COND_WAIT                1
#define HAS_PTHREAD_CONDATTR_DESTROY         1
#define HAS_PTHREAD_CONDATTR_GETCLOCK        1
#define HAS_PTHREAD_CONDATTR_GETPSHARED      1
#define HAS_PTHREAD_CONDATTR_INIT            1
#define HAS_PTHREAD_CONDATTR_SETCLOCK        1
#define HAS_PTHREAD_CONDATTR_SETPSHARED      1
#define HAS_PTHREAD_CREATE                   1
#define HAS_PTHREAD_DETACH                   1
#define HAS_PTHREAD_DO_EXIT                  1
#define HAS_PTHREAD_EQUAL                    1
#define HAS_PTHREAD_EXIT                     1
#define HAS_PTHREAD_FIND                     1
#define HAS_PTHREAD_GETSCHEDPARAM            1
#define HAS_PTHREAD_GETSPECIFIC              1
#define HAS_PTHREAD_JOIN                     1
#define HAS_PTHREAD_KEY_CLEANUP              1
#define HAS_PTHREAD_KEY_CREATE               1
#define HAS_PTHREAD_KEY_DELETE               1
#define HAS_PTHREAD_KILL                     1
#define HAS_PTHREAD_MUTEX_CLOCKLOCK          1
#define HAS_PTHREAD_MUTEX_CONSISTENT         1
#define HAS_PTHREAD_MUTEX_DESTROY            1
#define HAS_PTHREAD_MUTEX_GETPRIOCEILING     1
#define HAS_PTHREAD_MUTEX_INIT               1
#define HAS_PTHREAD_MUTEX_LOCK               1
#define HAS_PTHREAD_MUTEX_SETPRIOCEILING     1
#define HAS_PTHREAD_MUTEX_TIMEDLOCK          1
#define HAS_PTHREAD_MUTEX_TRYLOCK            1
#define HAS_PTHREAD_MUTEX_UNLOCK             1
#define HAS_PTHREAD_MUTEXATTR_DESTROY        1
#define HAS_PTHREAD_MUTEXATTR_GETPRIOCEILING 1
#define HAS_PTHREAD_MUTEXATTR_GETPROTOCOL    1
#define HAS_PTHREAD_MUTEXATTR_GETPSHARED     1
#define HAS_PTHREAD_MUTEXATTR_GETROBUST      1
#define HAS_PTHREAD_MUTEXATTR_GETTYPE        1
#define HAS_PTHREAD_MUTEXATTR_INIT           1
#define HAS_PTHREAD_MUTEXATTR_SETPRIOCEILING 1
#define HAS_PTHREAD_MUTEXATTR_SETPROTOCOL    1
#define HAS_PTHREAD_MUTEXATTR_SETPSHARED     1
#define HAS_PTHREAD_MUTEXATTR_SETROBUST      1
#define HAS_PTHREAD_MUTEXATTR_SETTYPE        1
#define HAS_PTHREAD_ONCE                     1
#define HAS_PTHREAD_RWLOCK_CLOCKRDLOCK       1
#define HAS_PTHREAD_RWLOCK_CLOCKWRLOCK       1
#define HAS_PTHREAD_RWLOCK_DESTROY           1
#define HAS_PTHREAD_RWLOCK_INIT              1
#define HAS_PTHREAD_RWLOCK_INIT_CB           1
#define HAS_PTHREAD_RWLOCK_RDLOCK            1
#define HAS_PTHREAD_RWLOCK_RDLOCK_EX         1
#define HAS_PTHREAD_RWLOCK_TIMEDRDLOCK       1
#define HAS_PTHREAD_RWLOCK_TIMEDWRLOCK       1
#define HAS_PTHREAD_RWLOCK_TRYRDLOCK         1
#define HAS_PTHREAD_RWLOCK_TRYWRLOCK         1
#define HAS_PTHREAD_RWLOCK_UNLOCK            1
#define HAS_PTHREAD_RWLOCK_WRLOCK            1
#define HAS_PTHREAD_RWLOCK_WRLOCK_EX         1
#define HAS_PTHREAD_RWLOCKATTR_DESTROY       1
#define HAS_PTHREAD_RWLOCKATTR_GETPSHARED    1
#define HAS_PTHREAD_RWLOCKATTR_INIT          1
#define HAS_PTHREAD_RWLOCKATTR_SETPSHARED    1
#define HAS_PTHREAD_SELF                     1
#define HAS_PTHREAD_SETCANCELSTATE           1
#define HAS_PTHREAD_SETSCHEDPARAM            1
#define HAS_PTHREAD_SETSCHEDPRIO             1
#define HAS_PTHREAD_SETSPECIFIC              1
#define HAS_PTHREAD_SIGMASK                  1
#define HAS_PTHREAD_SPIN_DESTROY             1
#define HAS_PTHREAD_SPIN_INIT                1
#define HAS_PTHREAD_SPIN_LOCK                1
#define HAS_PTHREAD_SPIN_TRYLOCK             1
#define HAS_PTHREAD_SPIN_UNLOCK              1
#define HAS_PTHREAD_START_POINT              1
#define HAS_PTHREAD_TESTCANCEL               1

/* clang-format on */

#endif /* _LIBC_FEATURES_H_ */
