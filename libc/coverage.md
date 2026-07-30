# Test Coverage

## fgetpos()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall store the current values of the parse state (if any) and file position indicator" | `stdio_fgetpos.fgetpos_basic` | covered |
| "shall not change the setting of errno if successful" | `stdio_fgetpos.fgetpos_no_errno_change` | covered |
| "shall return 0" on success | `stdio_fgetpos.fgetpos_basic` | covered |
| "shall return a non-zero value and set errno" on failure | `stdio_fgetpos.fgetpos_espipe` | covered |
| `errno` = `EBADF`: "The file descriptor underlying stream is not valid" | `stdio_fgetpos.fgetpos_ebadf` | covered |
| `errno` = `ESPIPE`: "The file descriptor underlying stream is associated with a pipe, FIFO, or socket" | `stdio_fgetpos.fgetpos_espipe` | covered |
| `errno` = `EOVERFLOW`: "The current value of the file position cannot be represented correctly in an object of type fpos_t" | — | not tested: cannot reliably create a file large enough to overflow fpos_t with mandatory POSIX interfaces |

## fsetpos()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall set the file position and state indicators for the stream pointed to by stream according to the value of the object pointed to by pos" | `stdio_fsetpos.fsetpos_basic` | covered |
| "A successful call to the fsetpos() function shall clear the end-of-file indicator for the stream" | `stdio_fsetpos.fsetpos_clears_eof` | covered |
| "undo any effects of ungetc() on the same stream" | `stdio_fsetpos.fsetpos_undoes_ungetc` | covered |
| "After an fsetpos() call, the next operation on an update stream may be either input or output" | `stdio_fsetpos.fsetpos_update_stream` | covered |
| "shall not change the setting of errno if successful" | `stdio_fsetpos.fsetpos_no_errno_change` | covered |
| "shall return 0 if it succeeds" | `stdio_fsetpos.fsetpos_basic` | covered |
| "shall return a non-zero value and set errno" on failure | `stdio_fsetpos.fsetpos_espipe` | covered |
| `errno` = `ESPIPE`: "The file descriptor underlying stream is associated with a pipe, FIFO, or socket" | `stdio_fsetpos.fsetpos_espipe` | covered |
| `errno` = `EBADF`: "The file descriptor underlying the stream file is not open for writing or the stream's buffer needed to be flushed and the file is not open" | — | not tested: triggering buffer flush on an invalidated fd is fragile and already covered by fseek_ferror |
| `errno` = `EAGAIN`: "The O_NONBLOCK flag is set for the file descriptor and the thread would be delayed in the write operation" | — | not tested: requires specific nonblocking write stall conditions |

## mkdtemp()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall create a directory with a unique name derived from template" | `stdlib_mkdtemp.mkdtemp_basic` | covered |
| "shall modify the contents of template by replacing six or more 'X' characters at the end of the pathname" | `stdlib_mkdtemp.mkdtemp_modifies_template` | covered |
| "the resulting pathname does not duplicate the name of an existing file" | `stdlib_mkdtemp.mkdtemp_unique` | covered |
| "create the new directory as if by a call to mkdir(pathname, S_IRWXU)" | `stdlib_mkdtemp.mkdtemp_basic` | covered |
| "shall return the value of template" on success | `stdlib_mkdtemp.mkdtemp_basic` | covered |
| "shall return a null pointer and shall set errno" on failure | `stdlib_mkdtemp.mkdtemp_einval` | covered |
| `errno` = `EINVAL`: "The string pointed to by template does not end in \"XXXXXX\"" | `stdlib_mkdtemp.mkdtemp_einval` | covered |
| `errno` = `ENOENT`: "A component of the path prefix specified by the template argument does not name an existing directory" | `stdlib_mkdtemp.mkdtemp_enoent` | covered |
| `errno` = `ENOTDIR`: "A component of the path prefix names an existing file that is neither a directory nor a symbolic link to a directory" | `stdlib_mkdtemp.mkdtemp_enotdir` | covered |
| `errno` = `ENAMETOOLONG`: "The length of a component of a pathname is longer than {NAME_MAX}" | `stdlib_mkdtemp.mkdtemp_enametoolong` | covered |
| `errno` = `EACCES`: "Search permission is denied on a component of the path prefix, or write permission is denied on the parent directory" | — | not tested: requires specific filesystem permission setup not portable across all test environments |
| `errno` = `ELOOP`: "A loop exists in symbolic links encountered during resolution of the path" | — | not tested: creating symlink loops is destructive to the test environment |
| `errno` = `EMLINK`: "The link count of the parent directory would exceed {LINK_MAX}" | — | not tested: cannot reliably exhaust link count with mandatory POSIX interfaces |
| `errno` = `ENOSPC`: "The file system does not contain enough space" | — | not tested: cannot reliably fill filesystem |
| `errno` = `EROFS`: "The parent directory resides on a read-only file system" | — | not tested: requires read-only filesystem mount |

## mkstemp()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall create a regular file with a unique name derived from template and return a file descriptor for the file open for reading and writing" | `stdlib_mkstemp.mkstemp_basic` | covered |
| "shall modify the contents of template by replacing six or more 'X' characters" | `stdlib_mkstemp.mkstemp_modifies_template` | covered |
| "the resulting pathname does not duplicate the name of an existing file" | `stdlib_mkstemp.mkstemp_unique` | covered |
| "as if by a call to open(pathname, O_RDWR|O_CREAT|O_EXCL, S_IRUSR|S_IWUSR)" | `stdlib_mkstemp.mkstemp_basic` | covered |
| fd is open for reading and writing | `stdlib_mkstemp.mkstemp_rdwr` | covered |
| "shall return an open file descriptor" on success | `stdlib_mkstemp.mkstemp_basic` | covered |
| "shall return -1 and shall set errno" on failure | `stdlib_mkstemp.mkstemp_einval` | covered |
| `errno` = `EINVAL` (from open): template not ending in XXXXXX | `stdlib_mkstemp.mkstemp_einval` | covered |
| `errno` = `ENOENT` (from open): path prefix does not exist | `stdlib_mkstemp.mkstemp_enoent` | covered |
| `errno` = `ENOTDIR` (from open): component of path prefix is not a directory | `stdlib_mkstemp.mkstemp_enotdir` | covered |
| `errno` = `ENAMETOOLONG` (from open): pathname component too long | `stdlib_mkstemp.mkstemp_enametoolong` | covered |

## mmap()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall establish a mapping between an address space of a process and a memory object" | `mman_mmap.mmap_basic_read` | covered |
| "If MAP_SHARED is specified, write references shall change the underlying object" | `mman_mmap.mmap_basic_write_shared` | covered |
| "If MAP_PRIVATE is specified, modifications to the mapped data by the calling process shall be visible only to the calling process and shall not change the underlying object" | `mman_mmap.mmap_basic_write_private` | covered |
| mapping at non-zero offset | `mman_mmap.mmap_offset` | covered |
| "shall return the address at which the mapping was placed (pa)" on success | `mman_mmap.mmap_basic_read` | covered |
| "shall return a value of MAP_FAILED and set errno" on failure | `mman_mmap.mmap_einval_zero_len` | covered |
| "No successful return from mmap() shall return the value MAP_FAILED" | `mman_mmap.mmap_not_null_return` | covered |
| "it never places a mapping at address 0" | `mman_mmap.mmap_not_null_return` | covered |
| "mmap() shall add an extra reference to the file... which is not removed by a subsequent close()" | `mman_mmap.mmap_fd_close_after_map` | covered |
| `errno` = `EINVAL`: "The value of len is zero" | `mman_mmap.mmap_einval_zero_len` | covered |
| `errno` = `EINVAL`: "The value of flags is invalid (neither MAP_PRIVATE nor MAP_SHARED is set)" | `mman_mmap.mmap_einval_no_map_flag` | covered |
| `errno` = `EBADF`: "The fildes argument is not a valid open file descriptor" | `mman_mmap.mmap_ebadf` | covered |
| `errno` = `EACCES`: "fildes is not open for read" / "fildes is not open for write and PROT_WRITE was specified for a MAP_SHARED type mapping" | `mman_mmap.mmap_eacces_write_on_rdonly` | covered |
| `errno` = `ENXIO`: "Addresses in the range [off,off+len) are invalid for the object specified by fildes" | `mman_mmap.mmap_enxio_invalid_offset` | covered (IGNORED on host: glibc allows mapping beyond file end) |
| `errno` = `ENODEV`: "fildes refers to a file whose type is not supported by mmap()" | — | not tested: no portable way to get an unsupported file type |
| `errno` = `ENOMEM`: "MAP_FIXED was specified, and the range exceeds that allowed for the address space" | — | not tested: MAP_FIXED usage is discouraged and implementation-defined |
| `errno` = `ENOTSUP`: "MAP_FIXED or MAP_PRIVATE was specified and the implementation does not support this functionality" | — | not tested: both are supported on Linux and Phoenix-RTOS |

## munmap()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall remove any mappings for those entire pages containing any part of the address space" | `mman_munmap.munmap_basic` | covered |
| "shall return 0" on success | `mman_munmap.munmap_basic` | covered |
| "shall return -1 and set errno" on failure | `mman_munmap.munmap_einval_zero_len` | covered |
| "If there are no mappings in the specified address range, then munmap() has no effect" | `mman_munmap.munmap_no_effect_unmapped` | covered |
| partial unmap of a multi-page mapping | `mman_munmap.munmap_partial` | covered |
| `errno` = `EINVAL`: "The len argument is 0" | `mman_munmap.munmap_einval_zero_len` | covered |
| `errno` = `EINVAL`: "Addresses in the range [addr,addr+len) are outside the valid range" | — | not tested: cannot portably determine an always-invalid address range |

## fdopendir()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall be equivalent to the opendir() function except that the directory is specified by a file descriptor rather than by a name" | `dirent_fdopendir.fdopendir_basic` | covered |
| returns a pointer to an object of type DIR on success | `dirent_fdopendir.fdopendir_basic` | covered |
| readdir on the returned DIR stream yields directory entries | `dirent_fdopendir.fdopendir_readdir` | covered |
| "Upon calling closedir() the file descriptor shall be closed" | `dirent_fdopendir.fdopendir_closedir_closes_fd` | covered |
| rewinddir works on fdopendir stream | `dirent_fdopendir.fdopendir_rewinddir` | covered |
| "shall return a null pointer and set errno" on failure | `dirent_fdopendir.fdopendir_ebadf` | covered |
| `errno` = `EBADF`: "The fd argument is not a valid file descriptor open for reading" | `dirent_fdopendir.fdopendir_ebadf` | covered |
| `errno` = `ENOTDIR`: "The descriptor fd is not associated with a directory" | `dirent_fdopendir.fdopendir_enotdir` | covered |

## seekdir()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall set the position of the next readdir() operation on the directory stream specified by dirp to the position specified by loc" | `dirent_seekdir_telldir.seekdir_restores_position` | covered |
| "The value of loc should have been returned from an earlier call to telldir() using the same directory stream" | `dirent_seekdir_telldir.seekdir_telldir_roundtrip` | covered |
| "The new position reverts to the one associated with the directory stream when telldir() was performed" | `dirent_seekdir_telldir.seekdir_multiple_positions` | covered |
| "shall not return a value" | — | covered implicitly (void return) |

## telldir()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall obtain the current location associated with the directory stream specified by dirp" | `dirent_seekdir_telldir.telldir_returns_position` | covered |
| "If the most recent operation on the directory stream was a seekdir(), the directory position returned from the telldir() shall be the same as that supplied as a loc argument for seekdir()" | `dirent_seekdir_telldir.seekdir_telldir_roundtrip` | covered |
| telldir after rewinddir returns same value as initial telldir | `dirent_seekdir_telldir.telldir_after_rewinddir` | covered |

## rand_r()

Tested in `stdlib/stdlib_random.c` (group `stdlib_rand_r`).

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "The rand_r() function shall compute a sequence of pseudo-random integers in the range [0, {RAND_MAX}]." | `stdlib_rand_r.rand_r_range` | covered |
| "If rand_r() is called with the same initial value for the object pointed to by seed and that object is not modified between successive returns and calls to rand_r(), the same sequence shall be generated." | `stdlib_rand_r.rand_r_reproducible` | covered |
| "The rand_r() function shall return a pseudo-random integer." | `stdlib_rand_r.rand_r_range` | covered |

## random()

Tested in `stdlib/stdlib_random.c` (group `stdlib_random`).

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "The random() function shall use a non-linear additive feedback random-number generator ... to return successive pseudo-random numbers in the range from 0 to 2^31-1." | `stdlib_random.random_range` | covered |
| "The random() function shall return the generated pseudo-random number." | `stdlib_random.random_range` | covered |
| "Like rand(), random() shall produce by default a sequence of numbers that can be duplicated by calling srandom() with 1 as the seed." | `stdlib_random.random_seed_one_reproducible` | covered |

## srandom()

Tested in `stdlib/stdlib_random.c` (group `stdlib_random`).

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "The srandom() function shall initialize the current state array using the value of seed." | `stdlib_random.random_reproducible` | covered |
| "The srandom() function shall not return a value." | — | not tested: void return enforced by the function prototype |

## ptsname()

Tested in `stdlib/stdlib_ptsname.c` (group `stdlib_ptsname`). Exercising ptsname()
requires a master pseudo-terminal from posix_openpt(), which is not implemented on
Phoenix; the test is guarded with `#ifndef __phoenix__` and reports `TEST_IGNORE`
on Phoenix targets.

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "The ptsname() function shall return the name of the slave pseudo-terminal device associated with a master pseudo-terminal device." | `stdlib_ptsname.ptsname_returns_slave_name` | covered |
| "Upon successful completion, ptsname() shall return a pointer to a string which is the name of the pseudo-terminal slave device." | `stdlib_ptsname.ptsname_returns_slave_name` | covered |
| "Upon failure, ptsname() shall return a null pointer and may set errno." | — | not tested: failure has only `may fail if:` clauses (EBADF, ENOTTY), so no portable input is guaranteed to fail |

## strndup()

Already covered — no new tests added to avoid redundancy. Tested in
`string/string_catdup.c` (group `string_dup`): `strndup_part`,
`strndup_full_string`, `strndup_zero_size`, `strndup_exceed`,
`strndup_specials_string`, `strndup_huge_string_part`, `strndup_huge_string_full`,
`strndup_ascii`, `strndup_extended_ascii`, `strndup_non_ascii`.

## strcasecmp(), strncasecmp()

Tested in `string/string_casecmp.c` (group `string_casecmp`).

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "The strcasecmp() ... functions shall compare, while ignoring differences in case, the string pointed to by s1 to the string pointed to by s2." | `string_casecmp.casecmp_equal_ignoring_case` | covered |
| "The strncasecmp() ... functions shall compare, while ignoring differences in case, not more than n bytes from the string pointed to by s1 to the string pointed to by s2." | `string_casecmp.ncasecmp_ignores_bytes_beyond_n` | covered |
| "When the LC_CTYPE category of the locale being used is from the POSIX locale, these functions shall behave as if the strings had been converted to lowercase and then a byte comparison performed." | `string_casecmp.casecmp_orders_by_char_value` | covered |
| "strcasecmp() ... shall return an integer greater than, equal to, or less than 0, if the string pointed to by s1 is, ignoring case, greater than, equal to, or less than the string pointed to by s2, respectively." | `string_casecmp.casecmp_orders_by_char_value` | covered |
| "strncasecmp() ... shall return an integer greater than, equal to, or less than 0, if the possibly null-terminated array pointed to by s1 is, ignoring case, greater than, equal to, or less than the possibly null-terminated array pointed to by s2, respectively." | `string_casecmp.ncasecmp_zero_length_is_equal` | covered |

## ffs()

Tested in `string/string_ffs.c` (group `string_ffs`).

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "The ffs() function shall find the first bit set (beginning with the least significant bit) in i, and return the index of that bit. Bits are numbered starting at one (the least significant bit)." | `string_ffs.ffs_returns_least_significant_index` | covered |
| "The ffs() function shall return the index of the first bit set." | `string_ffs.ffs_finds_high_and_sign_bit` | covered |
| "If i is 0, then ffs() shall return 0." | `string_ffs.ffs_zero_returns_zero` | covered |

## inet_addr()

Tested in `inet/inet_conv.c` (group `inet_addr`).

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "The inet_addr() function shall convert the string pointed to by cp, in the standard IPv4 dotted decimal notation, to an integer value suitable for use as an Internet address." | `inet_addr.addr_converts_dotted_decimal` | covered |
| "All Internet addresses shall be returned in network order (bytes ordered from left to right)." | `inet_addr.addr_converts_dotted_decimal` | covered |
| Values in the forms `a.b.c.d`, `a.b.c`, `a.b`, and `a` shall be interpreted as specified. | `inet_addr.addr_accepts_alternate_forms` | covered |
| "All numbers supplied as parts ... may be decimal, octal, or hexadecimal, as specified in the ISO C standard." | `inet_addr.addr_accepts_alternate_forms` | covered |
| "Upon successful completion, inet_addr() shall return the Internet address." | `inet_addr.addr_converts_dotted_decimal` | covered |
| "Otherwise, it shall return (in_addr_t)(-1)." | `inet_addr.addr_rejects_invalid` | covered |

## inet_ntoa()

Tested in `inet/inet_conv.c` (group `inet_ntoa`).

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "The inet_ntoa() function shall convert the Internet host address specified by in to a string in the Internet standard dot notation." | `inet_ntoa.ntoa_converts_to_dotted_decimal` | covered |
| "The inet_ntoa() function shall return a pointer to the network address in Internet standard dot notation." | `inet_ntoa.ntoa_converts_to_dotted_decimal` | covered |

## inet_pton()

Tested in `inet/inet_conv.c` (group `inet_pton`).

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "The inet_pton() function shall convert an address in its standard text presentation form into its numeric binary form." | `inet_pton.pton_ipv4_valid` | covered |
| "The inet_pton() function does not accept other formats (such as the octal numbers, hexadecimal numbers, and fewer than four numbers that inet_addr() accepts)." | `inet_pton.pton_ipv4_rejects_nonstandard` | covered |
| "If the af argument of inet_pton() is AF_INET6, the src string shall be in one of the ... standard IPv6 text forms." | `inet_pton.pton_ipv6_valid` | covered |
| "The inet_pton() function shall return 1 if the conversion succeeds, with the address pointed to by dst in network byte order." | `inet_pton.pton_ipv4_valid` | covered |
| "It shall return 0 if the input is not a valid IPv4 dotted-decimal string or a valid IPv6 address string." | `inet_pton.pton_ipv6_rejects_invalid` | covered |
| "or -1 with errno set to [EAFNOSUPPORT] if the af argument is unknown." | `inet_pton.pton_unknown_family` | covered |
| `errno` = `EAFNOSUPPORT`: "The af argument is invalid." | `inet_pton.pton_unknown_family` | covered |

## inet_ntop()

Tested in `inet/inet_conv.c` (group `inet_ntop`).

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "The inet_ntop() function shall convert a numeric address into a text string suitable for presentation." | `inet_ntop.ntop_ipv4` | covered |
| "The inet_ntop() function shall return a pointer to the buffer containing the text string if the conversion succeeds, and NULL otherwise, and set errno to indicate the error." | `inet_ntop.ntop_ipv4` / `inet_ntop.ntop_unknown_family` | covered |
| `errno` = `EAFNOSUPPORT`: "The af argument is invalid." | `inet_ntop.ntop_unknown_family` | covered |
| `errno` = `ENOSPC`: "The size of the inet_ntop() result buffer is inadequate." | `inet_ntop.ntop_buffer_too_small` | covered |

## ntohs()

Tested in `inet/inet_conv.c` (group `inet_byteorder`).

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "These functions shall convert 16-bit and 32-bit quantities between network byte order and host byte order." | `inet_byteorder.byteorder_ntohs_htons` | covered |
| "The ntohl() and ntohs() functions shall return the argument value converted from network to host byte order." | `inet_byteorder.byteorder_ntohs_htons` | covered |

## if_nametoindex()

Tested in `inet/inet_names.c` (group `inet_if`).

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "The if_nametoindex() function shall return the interface index corresponding to name ifname." | `inet_if.if_name_index_roundtrip` | covered |
| Return value: "The corresponding index if ifname is the name of an interface; otherwise, zero." | `inet_if.if_nametoindex_unknown_is_zero` | covered |

## if_indextoname()

Tested in `inet/inet_names.c` (group `inet_if`).

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "The if_indextoname() function shall map an interface index to its corresponding name." | `inet_if.if_name_index_roundtrip` | covered |
| "If ifindex is an interface index, then the function shall return the value supplied in ifname, which points to a buffer now containing the interface name." | `inet_if.if_name_index_roundtrip` | covered |
| "Otherwise, the function shall return a null pointer and set errno to indicate the error." | `inet_if.if_indextoname_unknown_fails` | covered |
| `errno` = `ENXIO`: "The interface does not exist." | `inet_if.if_indextoname_unknown_fails` | covered |

## gai_strerror()

Tested in `inet/inet_names.c` (group `inet_gai`).

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| For a listed EAI_* value "the function return value shall point to a string describing the error." | `inet_gai.gai_strerror_known_codes` | covered |
| "If the argument is not one of those values, the function shall return a pointer to a string whose contents indicate an unknown error." | `inet_gai.gai_strerror_unknown_code` | covered |
| "Upon successful completion, gai_strerror() shall return a pointer to an implementation-defined string." | `inet_gai.gai_strerror_known_codes` | covered |
