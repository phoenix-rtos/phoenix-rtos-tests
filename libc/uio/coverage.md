# Coverage: uio module

## readv()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall be equivalent to read(), but shall place the input data into the iovcnt buffers specified by the members of the iov array" | `uio_readv.readv_single_iov` | covered |
| "each iovec entry specifies the base address and length of an area in memory where data should be placed" | `uio_readv.readv_multiple_iov_scatter` | covered |
| "readv() shall always fill an area completely before proceeding to the next" | `uio_readv.readv_fills_completely_before_next` | covered |
| "Upon successful completion, readv() shall return a non-negative integer indicating the number of bytes actually read" | `uio_readv.readv_single_iov` | covered |
| "Otherwise, the functions shall return -1 and set errno to indicate the error" | `uio_readv.readv_ebadf` | covered |
| "If the readv() function successfully reads some data ... the number of bytes actually read is returned" (partial/EOF) | `uio_readv.readv_partial_eof` | covered |
| "readv() shall return 0" when no data is available and EOF | `uio_readv.readv_eof_returns_zero` | covered |
| "iov_len member can be zero ... no data transferred for that entry" | `uio_readv.readv_zero_len_iov` | covered |
| Works on pipe file descriptors | `uio_readv.readv_pipe` | covered |
| [EBADF]: "The fildes argument is not a valid file descriptor open for reading" | `uio_readv.readv_ebadf` | covered |
| [EBADF]: write-only descriptor | `uio_readv.readv_ebadf_writeonly` | covered |
| [EINVAL]: "The sum of the iov_len values in the iov array overflowed an ssize_t" | — | not tested: requires allocating IOV_MAX entries with large lengths |

## writev()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall be equivalent to write(), but shall gather the output data from the iovcnt buffers specified by the members of the iov array" | `uio_writev.writev_single_iov` | covered |
| "the buffers are gathered in the order specified by the members of the iov array" | `uio_writev.writev_fills_areas_in_order` | covered |
| "each iovec entry specifies the base address and length of an area in memory from which data should be written" | `uio_writev.writev_multiple_iov` | covered |
| "Upon successful completion, writev() shall return the number of bytes actually written" | `uio_writev.writev_single_iov` | covered |
| "Otherwise, the functions shall return -1 and set errno to indicate the error" | `uio_writev.writev_ebadf` | covered |
| "iov_len member can be zero ... no data transferred for that entry" | `uio_writev.writev_zero_len_iov` | covered |
| "If all iov_len members are zero and the file is a regular file, writev() shall return 0" | `uio_writev.writev_all_zero_len_regular_file` | covered |
| "writev() is atomic with respect to each other and to reads" (pipe ≤ PIPE_BUF) | `uio_writev.writev_pipe_atomicity` | covered |
| [EBADF]: "The fildes argument is not a valid file descriptor open for writing" | `uio_writev.writev_ebadf` | covered |
| [EBADF]: read-only descriptor | `uio_writev.writev_ebadf_readonly` | covered |
| [EINVAL]: "The sum of the iov_len values in the iov array overflowed an ssize_t" | — | not tested: requires allocating IOV_MAX entries with large lengths |
