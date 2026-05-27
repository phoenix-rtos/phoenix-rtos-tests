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
