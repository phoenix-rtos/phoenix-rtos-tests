# POSIX coverage — additional libc functions

Requirement-to-test mapping for the functions added in this batch. Only
normative (`shall`) requirements are listed; `may fail` errno clauses are
omitted per POSIX conformance-test rules.

Test cases are named `<group>.<case>`. Binaries: `test-libc-inet`,
`test-libc-time`, `test-libc-grp`, `test-libc-mman`, `test-libc-fileops`,
`test-libc-crypt`, `test-libc-libgen`, `test-libc-stdlib`.

## mprotect() — `test-libc-mman`, `mman/mman_mprotect.c`

| Requirement (POSIX) | Test case | Status |
|---|---|---|
| "shall change the access protections to be that specified by *prot* for those whole pages" | `mman_mprotect.read_protection_allows_read` | covered |
| "no implementation shall permit a write to succeed where PROT_WRITE has not been set" | `mman_mprotect.write_faults_without_prot_write` | covered |
| "[nor] permit any access where PROT_NONE alone has been set" | `mman_mprotect.read_faults_with_prot_none` | covered |
| "Implementations shall support at least ... PROT_NONE, PROT_READ, PROT_WRITE, and the bitwise-inclusive OR of PROT_READ and PROT_WRITE" | `mman_mprotect.supported_combinations_succeed` | covered |
| "Upon successful completion, mprotect() shall return 0" | `mman_mprotect.supported_combinations_succeed` | covered |
| "otherwise, it shall return -1 and set *errno*" | `mman_mprotect.unmapped_range_enomem` | covered |
| `ENOMEM`: "Addresses in the range ... are invalid ... or specify one or more pages which are not mapped" | `mman_mprotect.unmapped_range_enomem` | covered |
| `EACCES`: "*prot* argument specifies a protection that violates the access permission the process has to the underlying memory object" | `mman_mprotect.write_on_readonly_object_eacces` | covered |
| `EAGAIN`: PROT_WRITE over MAP_PRIVATE, insufficient memory to lock private page | — | not tested: cannot reliably exhaust private-page locking resources |
| `ENOTSUP`: "implementation does not support the combination of accesses" | — | not tested: all mandatory combinations are supported |

## gettimeofday() — `test-libc-time`, `time/gettimeofday.c`

| Requirement (POSIX) | Test case | Status |
|---|---|---|
| "shall obtain the current time, expressed as seconds and microseconds since the Epoch, and store it in ... *tp*" | `time_gettimeofday.agrees_with_time` | covered |
| microseconds field lies in its defined range `[0, 1000000)` | `time_gettimeofday.returns_zero_and_valid_usec` | covered |
| "shall return 0 and no value shall be reserved to indicate an error" | `time_gettimeofday.returns_zero_and_valid_usec` | covered |
| non-decreasing wall clock across reads | `time_gettimeofday.nondecreasing` | covered |
| "If *tzp* is not a null pointer, the behavior is unspecified" | — | not tested: unspecified behavior |

## getprotobyname(), getprotobynumber() — `test-libc-inet`, `inet/inet_proto.c`

| Requirement (POSIX) | Test case | Status |
|---|---|---|
| getprotobyname "shall ... find the first entry for which the protocol name specified by *name* matches the *p_name* member" | `inet_proto.getprotobyname_returns_entry` | covered |
| getprotobynumber "shall ... find the first entry for which the protocol number specified by *proto* matches the *p_proto* member" | `inet_proto.getprotobynumber_returns_entry` | covered |
| "shall each return a pointer to a *protoent* structure, the members of which shall contain the fields of an entry" | `inet_proto.aliases_array_null_terminated`, `inet_proto.name_number_consistency` | covered |
| "a null pointer if the end of the database was reached or the requested entry was not found" | `inet_proto.unknown_name_returns_null`, `inet_proto.unknown_number_returns_null` | covered |

## getaddrinfo(), freeaddrinfo() — `test-libc-inet`, `inet/inet_getaddrinfo.c`

| Requirement (POSIX) | Test case | Status |
|---|---|---|
| "shall translate ... and shall return a set of socket addresses"; "res ... shall refer to a linked list ... shall include at least one *addrinfo*" | `inet_getaddrinfo.numeric_ipv4` | covered |
| "*ai_family*, *ai_socktype*, and *ai_protocol* shall be usable as the arguments to socket()"; "*ai_addr* and *ai_addrlen* are usable" | `inet_getaddrinfo.numeric_ipv4` | covered |
| AI_PASSIVE with null *nodename*: address set to INADDR_ANY / IN6ADDR_ANY | `inet_getaddrinfo.passive_wildcard` | covered |
| no AI_PASSIVE with null *nodename*: address set to the loopback address | `inet_getaddrinfo.loopback_default` | covered |
| "If *servname* is null, the call shall return network-level addresses" | `inet_getaddrinfo.null_service_zero_port` | covered |
| "A non-zero socket type value shall limit the returned information to values with the specified socket type" | `inet_getaddrinfo.socktype_filter` | covered |
| AI_CANONNAME with numeric host: *ai_canonname* "shall refer to the nodename argument or a string with the same contents" | `inet_getaddrinfo.canonname_numeric` | covered |
| AI_NUMERICHOST with non-numeric *nodename*: `EAI_NONAME` | `inet_getaddrinfo.numerichost_nonnumeric_noname` | covered |
| AI_NUMERICSERV with non-numeric *servname*: `EAI_NONAME` | `inet_getaddrinfo.numericserv_nonnumeric_noname` | covered |
| `EAI_NONAME`: "Neither *nodename* nor *servname* were supplied" | `inet_getaddrinfo.both_null_noname` | covered |
| `EAI_FAMILY`: "The address family was not recognized" | `inet_getaddrinfo.bad_family` | covered |
| AF_INET6 numeric host resolves to an IPv6 socket address | `inet_getaddrinfo.numeric_ipv6` | covered |
| "A zero return value ... indicates successful completion; a non-zero return value indicates failure" | `inet_getaddrinfo.numeric_ipv4`, `inet_getaddrinfo.both_null_noname` | covered |
| freeaddrinfo "shall free ... the entire list"; "shall support the freeing of arbitrary sublists" | `inet_getaddrinfo.freeaddrinfo_frees_sublist` | covered |
| `EAI_AGAIN`, `EAI_FAIL`, `EAI_MEMORY`, `EAI_SYSTEM` | — | not tested: require resolver/system failures not triggerable with numeric-only inputs |
| `EAI_SERVICE`, `EAI_SOCKTYPE` | — | not tested: not reliably triggerable across conforming implementations |

## getnameinfo() — `test-libc-inet`, `inet/inet_getnameinfo.c`

| Requirement (POSIX) | Test case | Status |
|---|---|---|
| "shall translate a socket address to a node name and service location"; results are "always null-terminated strings" | `inet_getnameinfo.numeric_host_and_service` | covered |
| NI_NUMERICHOST / NI_NUMERICSERV return the numeric forms | `inet_getnameinfo.numeric_host_and_service` | covered |
| "If the *node* argument is NULL or the *nodelen* argument is zero, the node name shall not be returned" | `inet_getnameinfo.service_only` | covered |
| "If the *service* argument is NULL or the *servicelen* argument is zero, the service name shall not be returned" | `inet_getnameinfo.node_only` | covered |
| "A zero return value ... indicates successful completion" | `inet_getnameinfo.numeric_host_and_service` | covered |
| `EAI_OVERFLOW`: "buffer pointed to by the *node* argument or the *service* argument was too small" | `inet_getnameinfo.node_overflow`, `inet_getnameinfo.service_overflow` | covered |
| `EAI_FAMILY`: "The address family was not recognized" | `inet_getnameinfo.bad_family` | covered |
| `EAI_NONAME` (NI_NAMEREQD and host name cannot be located) | — | not tested: requires an address that deterministically has no reverse name |
| `EAI_AGAIN`, `EAI_FAIL`, `EAI_MEMORY`, `EAI_SYSTEM` | — | not tested: require resolver/system failures |

## gmtime_r() — `test-libc-time`, `time/time_reentrant.c`

| Requirement (POSIX) | Test case | Status |
|---|---|---|
| "shall convert the time in seconds since the Epoch ... into a broken-down time expressed as Coordinated Universal Time" | `time_gmtime_r.converts_and_returns_buffer` | covered |
| "The broken-down time is stored in the structure referred to by *result*"; "shall also return the address of the same structure" | `time_gmtime_r.converts_and_returns_buffer` | covered |
| values returned in a user-supplied buffer (not overwritten between calls) | `time_gmtime_r.uses_caller_buffer` | covered |
| `EOVERFLOW`: "The result cannot be represented" | — | not tested: 64-bit `time_t` on host cannot overflow the broken-down range |

## localtime_r() — `test-libc-time`, `time/time_reentrant.c`

| Requirement (POSIX) | Test case | Status |
|---|---|---|
| "shall convert the time in seconds since the Epoch ... into a broken-down time ... expressed as a local time" (corrected for timezone) | `time_localtime_r.converts_and_returns_buffer` | covered |
| "shall also return a pointer to that same structure" | `time_localtime_r.converts_and_returns_buffer` | covered |
| values returned in a user-supplied buffer | `time_localtime_r.uses_caller_buffer` | covered |
| `EOVERFLOW`: "The result cannot be represented" | — | not tested: 64-bit `time_t` on host cannot overflow the broken-down range |

## asctime_r() — `test-libc-time`, `time/time_reentrant.c`

| Requirement (POSIX) | Test case | Status |
|---|---|---|
| "shall convert the broken-down time ... into a string" of the fixed form `Sun Sep 16 01:03:52 1973\n\0` | `time_asctime_r.formats_known_times` | covered |
| "placed in the user-supplied buffer pointed to by *buf* ... and then return *buf*" | `time_asctime_r.formats_known_times` | covered |

## ctime_r() — `test-libc-time`, `time/time_reentrant.c`

| Requirement (POSIX) | Test case | Status |
|---|---|---|
| "shall convert the calendar time ... to local time in exactly the same form as ctime()" (`asctime(localtime(clock))`) | `time_ctime_r.converts_local_time` | covered |
| "shall return a pointer to the string pointed to by *buf*" | `time_ctime_r.converts_local_time` | covered |

## getgrent() — `test-libc-grp`, `grp/grp_ent.c`

| Requirement (POSIX) | Test case | Status |
|---|---|---|
| "shall return a pointer to a structure containing the broken-out fields of an entry"; "successive calls may be used to search the entire database" | `grp_getgrent.enumerates_entries` | covered |
| "On end-of-file, getgrent() shall return a null pointer and shall not change the setting of *errno*" | `grp_getgrent.eof_returns_null_without_errno` | covered |

## setgrent() — `test-libc-grp`, `grp/grp_ent.c`

| Requirement (POSIX) | Test case | Status |
|---|---|---|
| "shall rewind the group database so that the next getgrent() call returns the first entry" | `grp_getgrent.setgrent_rewinds` | covered |
| "shall not change the setting of *errno* if successful" | `grp_getgrent.setgrent_endgrent_preserve_errno` | covered |

## endgrent() — `test-libc-grp`, `grp/grp_ent.c`

| Requirement (POSIX) | Test case | Status |
|---|---|---|
| "shall close the group database" (a following setgrent()/getgrent() re-reads from the first entry) | `grp_getgrent.setgrent_rewinds` | covered |
| "shall not change the setting of *errno* if successful" | `grp_getgrent.setgrent_endgrent_preserve_errno` | covered |

## getgrnam_r() — `test-libc-grp`, `grp/grp_ent.c`

| Requirement (POSIX) | Test case | Status |
|---|---|---|
| "shall update the group structure ... and store a pointer to that structure at the location pointed to by *result*"; entry has "a matching name" | `grp_getgrnam_r.returns_entry_for_gid0_name` | covered |
| "shall return zero on success" | `grp_getgrnam_r.returns_entry_for_gid0_name` | covered |
| "return zero ... if the requested entry was not found"; "A null pointer is returned at the location pointed to by *result* ... if the requested entry is not found" | `grp_getgrnam_r.not_found_returns_zero_null` | covered |

## getgrgid_r() — `test-libc-grp`, `grp/grp_ent.c`

| Requirement (POSIX) | Test case | Status |
|---|---|---|
| "shall update the group structure ... and store a pointer to that structure at the location pointed to by *result*"; entry has "a matching gid" | `grp_getgrgid_r.returns_entry_for_gid0` | covered |
| "If successful, ... shall return zero" | `grp_getgrgid_r.returns_entry_for_gid0` | covered |
| "A null pointer shall be returned at the location pointed to by *result* ... if the requested entry is not found" | `grp_getgrgid_r.not_found_returns_zero_null` | covered |

## mknod() — `test-libc-fileops`, `fileops/fileops_mknod.c`

| Requirement (POSIX) | Test case | Status |
|---|---|---|
| "The only portable use ... is to create a FIFO-special file" (`S_IFIFO`, *dev* 0) | `fileops_mknod.fifo_creates_fifo` | covered |
| "The owner, group, and other permission bits of *mode* shall be modified by the file mode creation mask" | `fileops_mknod.perms_masked_by_umask` | covered |
| "shall mark for update the last data access, last data modification, and last file status change timestamps"; containing directory's timestamps marked for update | `fileops_mknod.marks_timestamps` | covered |
| "Upon successful completion ... shall return 0" | `fileops_mknod.fifo_creates_fifo` | covered |
| `EEXIST`: "The named file exists" | `fileops_mknod.existing_file_eexist` | covered |
| "If *path* names a symbolic link, mknod() shall fail and set *errno* to [EEXIST]" | `fileops_mknod.symlink_eexist` | covered |
| `ENOENT`: "A component of the path prefix ... does not name an existing file" | `fileops_mknod.missing_prefix_enoent` | covered |
| `ENOTDIR`: "A component of the path prefix names an existing file that is neither a directory nor a symbolic link to a directory" | `fileops_mknod.file_prefix_enotdir` | covered |
| `ENAMETOOLONG`: "The length of a component of a pathname is longer than {NAME_MAX}" | `fileops_mknod.long_component_enametoolong` | covered |
| `EPERM`: "The invoking process does not have appropriate privileges and the file type is not FIFO-special" | `fileops_mknod.non_fifo_unprivileged_eperm` | covered (skipped when euid == 0) |
| "If -1 is returned, the new file shall not be created" | `fileops_mknod.missing_prefix_enoent` | covered |
| `EACCES` | — | not tested: search/write-permission denial not observable when running as root in CI |
| `EIO`, `ENOSPC`, `EROFS`, `ELOOP` | — | not tested: require an I/O error, a full or read-only file system, or a symlink loop |

## crypt() — `test-libc-crypt`, `crypt/crypt.c`

| Requirement (POSIX) | Test case | Status |
|---|---|---|
| "shall return a pointer to the encoded string" | `crypt.returns_encoded_with_salt_prefix` | covered |
| "The first two bytes of the returned value shall be those of the *salt* argument" | `crypt.returns_encoded_with_salt_prefix`, `crypt.salt_prefix_varies_with_salt` | covered |
| deterministic string-encoding function of (*key*, *salt*) | `crypt.deterministic_for_same_inputs` | covered |
| "The first two bytes of this string may be used to perturb the encoding algorithm" (salt/key alter the encoding) | `crypt.salt_prefix_varies_with_salt`, `crypt.different_keys_differ` | covered |
| `ENOSYS`: "The functionality is not supported on this implementation" | — | not tested: functionality cannot be disabled at runtime |

## dirname() — `test-libc-libgen`, `libgen/libgen.c`

| Requirement (POSIX) | Test case | Status |
|---|---|---|
| "return a pointer to a string that is a pathname of the parent directory"; "Trailing '/' characters ... that are not also leading ... shall not be counted" | `libgen_dirname.returns_parent_component` | covered |
| "If *path* does not contain a '/' ... shall return a pointer to the string '.'"; null/empty *path* returns "." | `libgen_dirname.degenerate_paths_return_dot_or_root` | covered |

## basename() — `test-libc-libgen`, `libgen/libgen.c`

| Requirement (POSIX) | Test case | Status |
|---|---|---|
| "return a pointer to the final component of the pathname, deleting any trailing '/' characters" | `libgen_basename.returns_final_component` | covered |
| all-'/' path returns "/"; null/empty *path* returns "."; "." → "."; ".." → ".." | `libgen_basename.degenerate_paths` | covered |
| "If ... path is exactly '//', it is implementation-defined whether '/' or '//' is returned" | — | not tested: implementation-defined |

## unlockpt() — `test-libc-stdlib`, `stdlib/stdlib_unlockpt.c`

| Requirement (POSIX) | Test case | Status |
|---|---|---|
| "shall unlock the slave pseudo-terminal device associated with the master to which *fildes* refers" (slave becomes openable) | `stdlib_unlockpt.unlocks_slave_for_open` | covered |
| "Upon successful completion, unlockpt() shall return 0" | `stdlib_unlockpt.returns_zero_on_success` | covered |
