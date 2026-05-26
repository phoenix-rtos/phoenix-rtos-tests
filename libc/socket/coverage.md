# Coverage: socket module

## socketpair()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall create an unbound pair of connected sockets in a specified domain, of a specified type" | `socket_api_socketpair.stream_success` | covered |
| "The two sockets shall be identical" (bidirectional) | `socket_api_socketpair.bidirectional_stream` | covered |
| "The file descriptors used in referencing the created sockets shall be returned in socket_vector[0] and socket_vector[1]" | `socket_api_socketpair.stream_success` | covered |
| "The file descriptors shall be allocated as described in File Descriptor Allocation" (lowest available) | `socket_api_socketpair.fd_allocation` | covered |
| SOCK_STREAM type | `socket_api_socketpair.stream_success` | covered |
| SOCK_DGRAM type | `socket_api_socketpair.dgram_success` | covered |
| SOCK_SEQPACKET type | `socket_api_socketpair.seqpacket_success` | covered |
| "Specifying a protocol of 0 causes socketpair() to use an unspecified default protocol appropriate for the requested socket type" | `socket_api_socketpair.protocol_zero_default` | covered |
| Bidirectional communication (SOCK_DGRAM) | `socket_api_socketpair.bidirectional_dgram` | covered |
| "Upon successful completion, this function shall return 0" | `socket_api_socketpair.stream_success` | covered |
| "otherwise, -1 shall be returned and errno set to indicate the error, no file descriptors shall be allocated, and the contents of socket_vector shall be left unmodified" | `socket_api_socketpair.vector_unmodified_on_error` | covered |
| [EAFNOSUPPORT]: "The implementation does not support the specified address family" | `socket_api_socketpair.eafnosupport_invalid_domain` | covered |
| [EOPNOTSUPP]: "The specified protocol does not permit creation of socket pairs" | `socket_api_socketpair.eopnotsupp_inet` | covered |
| [EPROTONOSUPPORT]: "The protocol is not supported by the address family, or the protocol is not supported by the implementation" | `socket_api_socketpair.eprotonosupport_invalid_protocol` | covered |
| [EPROTOTYPE]: "The socket type is not supported by the protocol" | `socket_api_socketpair.eprototype_invalid_type` | covered |
| [EMFILE]: "All, or all but one, of the file descriptors available to the process are currently open" | `socket_api_socketpair.emfile_resource_exhaustion` | covered |
| [ENFILE]: "No more file descriptors are available for the system" | — | not tested: system-wide limit cannot be portably triggered |
| [EACCES]: "The process does not have appropriate privileges" | — | not tested: cannot portably reduce privileges for socketpair |
| [ENOBUFS]: "Insufficient resources were available in the system to perform the operation" | — | not tested: cannot trigger portably |
| [ENOMEM]: "Insufficient memory was available to fulfill the request" | — | not tested: cannot trigger portably |

## shutdown()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall cause all or part of a full-duplex connection on the socket associated with the file descriptor socket to be shut down" | `socket_api_shutdown.shut_rdwr_disables_both` | covered |
| SHUT_RD: "Disables further receive operations" | `socket_api_shutdown.shut_rd_disables_recv` | covered |
| SHUT_WR: "Disables further send operations" | `socket_api_shutdown.shut_wr_disables_send` | covered |
| SHUT_RDWR: "Disables further send and receive operations" | `socket_api_shutdown.shut_rdwr_disables_both` | covered |
| SHUT_WR causes peer to read EOF after pending data | `socket_api_shutdown.shut_wr_peer_reads_eof` | covered |
| SHUT_RD does not affect peer's ability to receive from the other direction | `socket_api_shutdown.shut_rd_peer_can_still_send` | covered |
| Shutdown on SOCK_DGRAM connected socket | `socket_api_shutdown.dgram_shutdown_rdwr` | covered |
| "Upon successful completion, shutdown() shall return 0" | `socket_api_shutdown.return_zero_on_success` | covered |
| "otherwise, -1 shall be returned and errno set to indicate the error" | `socket_api_shutdown.ebadf_invalid_fd` | covered |
| [EBADF]: "The socket argument is not a valid file descriptor" | `socket_api_shutdown.ebadf_invalid_fd` | covered |
| [EBADF]: closed file descriptor | `socket_api_shutdown.ebadf_closed_fd` | covered |
| [EINVAL]: "The how argument is invalid" | `socket_api_shutdown.einval_invalid_how` | covered |
| [ENOTCONN]: "The socket is not connected" | `socket_api_shutdown.enotconn_unconnected` | covered |
| [ENOTSOCK]: "The socket argument does not refer to a socket" | `socket_api_shutdown.enotsock_not_socket` | covered |
| [ENOBUFS]: "Insufficient resources were available in the system to perform the operation" | — | not tested: cannot trigger portably |

## setsockopt()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall set the option specified by the option_name argument, at the protocol level specified by the level argument, to the value pointed to by the option_value argument" | `socket_api_setsockopt.so_reuseaddr` | covered |
| "To set options at the socket level, specify the level argument as SOL_SOCKET" | `socket_api_setsockopt.so_keepalive` | covered |
| SO_RCVBUF option | `socket_api_setsockopt.so_rcvbuf` | covered |
| SO_SNDBUF option | `socket_api_setsockopt.so_sndbuf` | covered |
| SO_RCVTIMEO option | `socket_api_setsockopt.so_rcvtimeo` | covered |
| SO_SNDTIMEO option | `socket_api_setsockopt.so_sndtimeo` | covered |
| SO_BROADCAST option | `socket_api_setsockopt.so_broadcast` | covered |
| SO_KEEPALIVE option (enable and disable) | `socket_api_setsockopt.disable_option` | covered |
| SO_LINGER option (struct linger) | `socket_api_setsockopt.so_linger` | covered |
| SO_DONTROUTE option | `socket_api_setsockopt.so_dontroute` | covered |
| SO_OOBINLINE option | `socket_api_setsockopt.so_oobinline` | covered |
| SO_RCVLOWAT option | `socket_api_setsockopt.so_rcvlowat` | covered |
| SO_SNDLOWAT option | `socket_api_setsockopt.so_sndlowat` | covered |
| SO_ACCEPTCONN is read-only | `socket_api_setsockopt.readonly_so_acceptconn` | covered |
| SO_ERROR is read-only | `socket_api_setsockopt.readonly_so_error` | covered |
| SO_TYPE is read-only | `socket_api_setsockopt.readonly_so_type` | covered |
| "Upon successful completion, setsockopt() shall return 0" | `socket_api_setsockopt.return_zero_on_success` | covered |
| "Otherwise, -1 shall be returned and errno set to indicate the error" | `socket_api_setsockopt.ebadf_invalid_fd` | covered |
| [EBADF]: "The socket argument is not a valid file descriptor" | `socket_api_setsockopt.ebadf_invalid_fd` | covered |
| [EINVAL]: "The specified option is invalid at the specified socket level or the socket has been shut down" | `socket_api_setsockopt.einval_after_shutdown` | covered |
| [EINVAL]: invalid option_len | `socket_api_setsockopt.einval_short_optlen` | covered |
| [ENOPROTOOPT]: "The option is not supported by the protocol" | `socket_api_setsockopt.enoprotoopt_invalid_option` | covered |
| [ENOTSOCK]: "The socket argument does not refer to a socket" | `socket_api_setsockopt.enotsock_not_socket` | covered |
| [EDOM]: "The send and receive timeout values are too big to fit into the timeout fields in the socket structure" | — | not tested: cannot determine implementation-specific maximum timeout value |
| [EISCONN]: "The socket is already connected, and a specified option cannot be set while the socket is connected" | — | not tested: no standard option that is restricted on connected AF_UNIX sockets |
| [ENOMEM]: "There was insufficient memory available for the operation to complete" | — | not tested: cannot trigger without manipulating system memory |
| [ENOBUFS]: "Insufficient resources are available in the system to complete the call" | — | not tested: cannot trigger portably |

## send()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall initiate transmission of a message from the specified socket to its peer" | `socket_api_send.basic_success` | covered |
| "shall send a message only when the socket is connected" | `socket_api_send.enotconn_unconnected` | covered |
| "If the socket is a connectionless-mode socket, the message shall be sent to the pre-specified peer address" | `socket_api_send.edestaddrreq_dgram_no_peer` | covered |
| MSG_NOSIGNAL: "Requests not to send the SIGPIPE signal if an attempt to send is made on a stream-oriented socket that is no longer connected. The [EPIPE] error shall still be returned." | `socket_api_send.msg_nosignal_flag` | covered |
| "Upon successful completion, send() shall return the number of bytes sent" | `socket_api_send.returns_bytes_sent` | covered |
| "Otherwise, -1 shall be returned and errno set to indicate the error" | `socket_api_send.ebadf_invalid_fd` | covered |
| [EAGAIN] or [EWOULDBLOCK]: "The socket's file descriptor is marked O_NONBLOCK and the requested operation would block" | `socket_api_send.ewouldblock_nonblocking` | covered |
| [EBADF]: "The socket argument is not a valid file descriptor" | `socket_api_send.ebadf_invalid_fd` | covered |
| [EDESTADDRREQ]: "The socket is not connection-mode and no peer address is set" | `socket_api_send.edestaddrreq_dgram_no_peer` | covered |
| [EINTR]: "A signal interrupted send() before any data was transmitted" | `socket_api_send.eintr_signal_interrupts` | covered |
| [EMSGSIZE]: "The message is too large to be sent all at once, as the socket requires" | `socket_api_send.emsgsize_dgram_too_large` | covered |
| [ENOTCONN]: "The socket is not connected" | `socket_api_send.enotconn_unconnected` | covered |
| [ENOTSOCK]: "The socket argument does not refer to a socket" | `socket_api_send.enotsock_not_socket` | covered |
| [EPIPE]: "The socket is shut down for writing, or the socket is connection-mode and is no longer connected" | `socket_api_send.epipe_after_shutdown_wr` | covered |
| APPLICATION USAGE: "if the flags argument is 0, the send() function is equivalent to write()" | `socket_api_send.equivalent_to_write_with_flags_zero` | covered |
| [ECONNRESET]: "A connection was forcibly closed by a peer" | — | not tested: cannot reliably trigger forcible connection reset on AF_UNIX |
| [EOPNOTSUPP]: "The socket argument is associated with a socket that does not support one or more of the values set in flags" | — | not tested: no portable unsupported flag value on AF_UNIX |

## sendto()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall send a message through a connection-mode or connectionless-mode socket" | `socket_api_sendto.basic_connected_stream` | covered |
| "If the socket is a connectionless-mode socket, the message shall be sent to the address specified by dest_addr" | `socket_api_sendto.dgram_with_address` | covered |
| "If the socket is connection-mode, dest_addr shall be ignored" | `socket_api_sendto.connection_mode_ignores_dest_addr` | covered |
| MSG_NOSIGNAL: "Requests not to send the SIGPIPE signal... The [EPIPE] error shall still be returned" | `socket_api_sendto.msg_nosignal_flag` | covered |
| "Upon successful completion, sendto() shall return the number of bytes sent" | `socket_api_sendto.returns_bytes_sent` | covered |
| "Otherwise, -1 shall be returned and errno set to indicate the error" | `socket_api_sendto.ebadf_invalid_fd` | covered |
| [EAGAIN] or [EWOULDBLOCK]: "The socket's file descriptor is marked O_NONBLOCK and the requested operation would block" | `socket_api_sendto.ewouldblock_nonblocking` | covered |
| [EBADF]: "The socket argument is not a valid file descriptor" | `socket_api_sendto.ebadf_invalid_fd` | covered |
| [EMSGSIZE]: "The message is too large to be sent all at once, as the socket requires" | `socket_api_sendto.emsgsize_dgram_too_large` | covered |
| [ENOTCONN]: "The socket is connection-mode but is not connected" | `socket_api_sendto.enotconn_stream_unconnected` | covered |
| [ENOTSOCK]: "The socket argument does not refer to a socket" | `socket_api_sendto.enotsock_not_socket` | covered |
| [EPIPE]: "The socket is shut down for writing, or the socket is connection-mode and is no longer connected" | `socket_api_sendto.epipe_after_shutdown_wr` | covered |
| AF_UNIX [ENOENT]: "A component of the pathname does not name an existing file or the pathname is an empty string" | `socket_api_sendto.enoent_unix_path_missing` | covered |
| [EAFNOSUPPORT]: "Addresses in the specified address family cannot be used with this socket" | — | not tested: requires mismatched address family |
| [ECONNRESET]: "A connection was forcibly closed by a peer" | — | not tested: cannot reliably trigger on AF_UNIX |
| [EINTR]: "A signal interrupted sendto() before any data was transmitted" | — | not tested: sendto on SOCK_DGRAM does not block; SOCK_STREAM uses same path as send() already tested |
| [EOPNOTSUPP]: "The socket argument is associated with a socket that does not support one or more of the values set in flags" | — | not tested: no portable unsupported flag value on AF_UNIX |

## sendmsg()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall send a message through a connection-mode or connectionless-mode socket" | `socket_api_sendmsg.basic_single_iov` | covered |
| "If the socket is connection-mode, the destination address in msghdr shall be ignored" | `socket_api_sendmsg.connection_mode_ignores_dest_addr` | covered |
| "The msg_flags member is ignored" | `socket_api_sendmsg.msg_flags_ignored` | covered |
| "The msg_iov and msg_iovlen fields of message specify zero or more buffers containing the data to be sent" | `socket_api_sendmsg.scatter_gather_multiple_iov` | covered |
| "The data from each storage area indicated by msg_iov is sent in turn" | `socket_api_sendmsg.scatter_gather_multiple_iov` | covered |
| Ancillary data (SCM_RIGHTS): "msg_control and msg_controllen members for ancillary data" | `socket_api_sendmsg.scm_rights_send_fd` | covered |
| MSG_NOSIGNAL: "Requests not to send the SIGPIPE signal... The [EPIPE] error shall still be returned" | `socket_api_sendmsg.msg_nosignal_flag` | covered |
| "Upon successful completion, sendmsg() shall return the number of bytes sent" | `socket_api_sendmsg.returns_bytes_sent` | covered |
| "Otherwise, -1 shall be returned and errno set to indicate the error" | `socket_api_sendmsg.ebadf_invalid_fd` | covered |
| [EAGAIN] or [EWOULDBLOCK]: "The socket's file descriptor is marked O_NONBLOCK and the requested operation would block" | `socket_api_sendmsg.ewouldblock_nonblocking` | covered |
| [EBADF]: "The socket argument is not a valid file descriptor" | `socket_api_sendmsg.ebadf_invalid_fd` | covered |
| [EINTR]: "A signal interrupted sendmsg() before any data was transmitted" | `socket_api_sendmsg.eintr_signal_interrupts` | covered |
| [EINVAL]: "The sum of the iov_len values overflows an ssize_t" or invalid cmsg_len | `socket_api_sendmsg.invalid_cmsg_len` | covered |
| [EMSGSIZE]: "the msg_iovlen member of the msghdr structure is less than or equal to 0 or is greater than {IOV_MAX}" | `socket_api_sendmsg.iovlen_exceeds_iov_max` | covered |
| msg_iovlen of 0 | `socket_api_sendmsg.iovlen_zero_or_negative` | covered |
| [EMSGSIZE]: "The message is too large to be sent all at once, as the socket requires" | `socket_api_sendmsg.emsgsize_dgram_too_large` | covered |
| [ENOTCONN]: "The socket is connection-mode but is not connected" | `socket_api_sendmsg.enotconn_unconnected` | covered |
| [ENOTSOCK]: "The socket argument does not refer to a socket" | `socket_api_sendmsg.enotsock_not_socket` | covered |
| [EPIPE]: "The socket is shut down for writing, or the socket is connection-mode and is no longer connected" | `socket_api_sendmsg.epipe_after_shutdown_wr` | covered |
| Connectionless-mode socket: "the message shall be sent to the address specified by msghdr" | `socket_api_sendmsg.dgram_with_dest_addr` | covered |
| [EAFNOSUPPORT]: "Addresses in the specified address family cannot be used with this socket" | — | not tested: requires mismatched address family which is hard to portably construct |
| [ECONNRESET]: "A connection was forcibly closed by a peer" | — | not tested: cannot reliably trigger on AF_UNIX |
| [EOPNOTSUPP]: "The socket argument is associated with a socket that does not support one or more of the values set in flags" | — | not tested: no portable unsupported flag value on AF_UNIX |

## getsockopt()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall retrieve the value for the option specified by the option_name argument for the socket specified by the socket argument" | `socket_api_getsockopt.basic_so_type_stream` | covered |
| "If the size of the option value is greater than option_len, the value stored in the object pointed to by the option_value argument shall be silently truncated" | `socket_api_getsockopt.truncation_small_optlen` | covered |
| "Otherwise, the object pointed to by the option_len argument shall be modified to indicate the actual length of the value" | `socket_api_getsockopt.optlen_updated` | covered |
| "To retrieve options at the socket level, specify the level argument as SOL_SOCKET" | `socket_api_getsockopt.basic_so_type_stream` | covered |
| SO_TYPE option (SOCK_STREAM) | `socket_api_getsockopt.basic_so_type_stream` | covered |
| SO_TYPE option (SOCK_DGRAM) | `socket_api_getsockopt.basic_so_type_dgram` | covered |
| SO_ERROR option | `socket_api_getsockopt.so_error_no_error` | covered |
| SO_RCVBUF option | `socket_api_getsockopt.so_rcvbuf` | covered |
| SO_SNDBUF option | `socket_api_getsockopt.so_sndbuf` | covered |
| SO_ACCEPTCONN option (not listening) | `socket_api_getsockopt.so_acceptconn_not_listening` | covered |
| SO_RCVTIMEO option (default value) | `socket_api_getsockopt.so_rcvtimeo_default` | covered |
| SO_SNDTIMEO option (default value) | `socket_api_getsockopt.so_sndtimeo_default` | covered |
| Retrieves value previously set by setsockopt() | `socket_api_getsockopt.retrieves_value_after_setsockopt` | covered |
| "Upon successful completion, getsockopt() shall return 0" | `socket_api_getsockopt.basic_so_type_stream` | covered |
| "otherwise, -1 shall be returned and errno set to indicate the error" | `socket_api_getsockopt.ebadf_invalid_fd` | covered |
| [EBADF]: "The socket argument is not a valid file descriptor" | `socket_api_getsockopt.ebadf_invalid_fd` | covered |
| [ENOPROTOOPT]: "The option is not supported by the protocol" | `socket_api_getsockopt.enoprotoopt_unsupported_option` | covered |
| [ENOTSOCK]: "The socket argument does not refer to a socket" | `socket_api_getsockopt.enotsock_regular_fd` | covered |
| [EINVAL]: "The specified option is invalid at the specified socket level" | `socket_api_getsockopt.enoprotoopt_unsupported_option` | covered |
| [ENOBUFS]: "Insufficient resources are available in the system to complete the function" | — | not tested: cannot trigger portably |

## listen()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall mark a connection-mode socket, specified by the socket argument, as accepting connections" | `socket_api_listen.basic_success` | covered |
| "The backlog argument provides a hint to the implementation which the implementation shall use to limit the number of outstanding connections in the socket's listen queue" | `socket_api_listen.basic_success` | covered |
| "Implementations shall support values of backlog up to SOMAXCONN" | `socket_api_listen.backlog_somaxconn` | covered |
| "If listen() is called with a backlog argument value that is less than 0, the function behaves as if it had been called with a backlog argument value of 0" | `socket_api_listen.backlog_negative` | covered |
| "A backlog argument of 0 may allow the socket to accept connections" | `socket_api_listen.backlog_zero` | covered |
| "If backlog exceeds this limit, the length of the listen queue is set to the limit" | `socket_api_listen.backlog_exceeds_somaxconn` | covered |
| "Normally, a larger backlog argument value shall result in a larger or equal length of the listen queue" | `socket_api_listen.backlog_somaxconn` | covered |
| Socket marked as accepting (SO_ACCEPTCONN) | `socket_api_listen.marks_socket_accepting` | covered |
| Calling listen twice on same socket | `socket_api_listen.listen_called_twice` | covered |
| "Upon successful completions, listen() shall return 0" | `socket_api_listen.basic_success` | covered |
| "otherwise, -1 shall be returned and errno set to indicate the error" | `socket_api_listen.ebadf_invalid_fd` | covered |
| [EBADF]: "The socket argument is not a valid file descriptor" | `socket_api_listen.ebadf_invalid_fd` | covered |
| [EINVAL]: "The socket is already connected" | `socket_api_listen.einval_already_connected` | covered |
| [ENOTSOCK]: "The socket argument does not refer to a socket" | `socket_api_listen.enotsock_pipe_fd` | covered |
| [EOPNOTSUPP]: "The socket protocol does not support listen()" | `socket_api_listen.eopnotsupp_dgram_socket` | covered |
| [EDESTADDRREQ]: "The socket is not bound to a local address, and the protocol does not support listening on an unbound socket" | — | not tested: AF_UNIX on Linux auto-binds or returns EINVAL rather than EDESTADDRREQ |
| [ENOBUFS]: "Insufficient resources are available in the system to complete the call" | — | not tested: cannot trigger portably |

## recv()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall receive a message from a connection-mode or connectionless-mode socket" | `socket_api_recv.basic_success` | covered |
| "shall return the length of the message written to the buffer pointed to by the buffer argument" | `socket_api_recv.returns_message_length` | covered |
| "For stream-based sockets, such as SOCK_STREAM, message boundaries shall be ignored" | `socket_api_recv.partial_read_stream` | covered |
| "In this case, data shall be returned to the user as soon as it becomes available, and no data shall be discarded" | `socket_api_recv.multiple_sends_single_recv` | covered |
| MSG_PEEK: "Peeks at an incoming message. The data is treated as unread and the next recv() or similar function shall still return this data" | `socket_api_recv.msg_peek` | covered |
| MSG_WAITALL: "On SOCK_STREAM sockets this requests that the function block until the full amount of data can be returned" | `socket_api_recv.msg_waitall` | covered |
| MSG_WAITALL: "The function may return the smaller amount of data if... the connection is terminated" | `socket_api_recv.msg_waitall` | covered |
| "If no messages are available at the socket and O_NONBLOCK is not set on the socket's file descriptor, recv() shall block until a message arrives" | — | not tested: would require multi-threading to verify blocking behaviour |
| "If no messages are available at the socket and O_NONBLOCK is set on the socket's file descriptor, recv() shall fail and set errno to [EAGAIN] or [EWOULDBLOCK]" | `socket_api_recv.eagain_nonblock_no_data` | covered |
| "If no messages are available to be received and the peer has performed an orderly shutdown, recv() shall return 0" | `socket_api_recv.orderly_shutdown_returns_zero` | covered |
| recv with length 0 | `socket_api_recv.zero_length_buffer` | covered |
| [EAGAIN] or [EWOULDBLOCK]: "The socket's file descriptor is marked O_NONBLOCK and no data is waiting to be received" | `socket_api_recv.eagain_nonblock_no_data` | covered |
| [EBADF]: "The socket argument is not a valid file descriptor" | `socket_api_recv.ebadf_invalid_fd` | covered |
| [ENOTCONN]: "A receive is attempted on a connection-mode socket that is not connected" | `socket_api_recv.enotconn_unconnected_stream` | covered |
| [ENOTSOCK]: "The socket argument does not refer to a socket" | `socket_api_recv.enotsock_pipe_fd` | covered |
| [ECONNRESET]: "A connection was forcibly closed by a peer" | — | not tested: cannot reliably trigger on AF_UNIX |
| [EINTR]: "The recv() function was interrupted by a signal that was caught, before any data was available" | — | not tested: requires multi-threading to block and then signal |
| [EINVAL]: "The MSG_OOB flag is set and no out-of-band data is available" | — | not tested: OOB data not supported on AF_UNIX |
| [EOPNOTSUPP]: "The specified flags are not supported for this socket type or protocol" | — | not tested: no portable unsupported flag value |
| [ETIMEDOUT]: "The connection timed out during connection establishment, or due to a transmission timeout on active connection" | — | not tested: cannot trigger on AF_UNIX sockets |

## recvfrom()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall receive a message from a connection-mode or connectionless-mode socket" | `socket_api_recvfrom.basic_success_null_address` | covered |
| "shall return the length of the message written to the buffer pointed to by the buffer argument" | `socket_api_recvfrom.returns_message_length` | covered |
| "It is normally used with connectionless-mode sockets because it permits the application to retrieve the source address of received data" | `socket_api_recvfrom.dgram_source_address` | covered |
| "For message-based sockets... the entire message shall be read in a single operation. If a message is too long to fit in the supplied buffer, and MSG_PEEK is not set in the flags argument, the excess bytes shall be discarded" | `socket_api_recvfrom.dgram_excess_bytes_discarded` | covered |
| "For stream-based sockets, such as SOCK_STREAM, message boundaries shall be ignored" | `socket_api_recvfrom.partial_read_stream` | covered |
| MSG_PEEK: "Peeks at an incoming message. The data is treated as unread and the next recvfrom() or similar function shall still return this data" | `socket_api_recvfrom.msg_peek` | covered |
| MSG_WAITALL: "On SOCK_STREAM sockets this requests that the function block until the full amount of data can be returned" | `socket_api_recvfrom.msg_waitall_stream` | covered |
| "If no messages are available at the socket and O_NONBLOCK is set on the socket's file descriptor, recvfrom() shall fail and set errno to [EAGAIN] or [EWOULDBLOCK]" | `socket_api_recvfrom.eagain_nonblock_no_data` | covered |
| "If no messages are available to be received and the peer has performed an orderly shutdown, recvfrom() shall return 0" | `socket_api_recvfrom.orderly_shutdown_returns_zero` | covered |
| "the source address of the received message shall be stored in the sockaddr structure pointed to by the address argument" | `socket_api_recvfrom.dgram_source_address` | covered |
| NULL address argument: recv-like behaviour | `socket_api_recvfrom.basic_success_null_address` | covered |
| [EAGAIN] or [EWOULDBLOCK]: "The socket's file descriptor is marked O_NONBLOCK and no data is waiting to be received" | `socket_api_recvfrom.eagain_nonblock_no_data` | covered |
| [EBADF]: "The socket argument is not a valid file descriptor" | `socket_api_recvfrom.ebadf_invalid_fd` | covered |
| [ENOTCONN]: "A receive is attempted on a connection-mode socket that is not connected" | `socket_api_recvfrom.enotconn_unconnected_stream` | covered |
| [ENOTSOCK]: "The socket argument does not refer to a socket" | `socket_api_recvfrom.enotsock_pipe_fd` | covered |
| [ECONNRESET]: "A connection was forcibly closed by a peer" | — | not tested: cannot reliably trigger on AF_UNIX |
| [EINTR]: "A signal interrupted recvfrom() before any data was available" | — | not tested: requires multi-threading to block and then signal |
| [EINVAL]: "The MSG_OOB flag is set and no out-of-band data is available" | — | not tested: OOB data not supported on AF_UNIX |
| [EOPNOTSUPP]: "The specified flags are not supported for this socket type" | — | not tested: no portable unsupported flag value |
| [ETIMEDOUT]: "The connection timed out during connection establishment, or due to a transmission timeout on active connection" | — | not tested: cannot trigger on AF_UNIX sockets |

## recvmsg()

| Requirement (POSIX verbatim) | Test case | Status |
|---|---|---|
| "shall receive a message from a connection-mode or connectionless-mode socket" | `socket_api_recvmsg.basic_success` | covered |
| "shall return the total length of the message" | `socket_api_recvmsg.returns_total_length` | covered |
| "The msg_iov and msg_iovlen fields are used to specify where the received data shall be stored... Each storage area indicated by msg_iov is filled with received data in turn" | `socket_api_recvmsg.scatter_multiple_iovecs` | covered |
| "the msg_name member may be a null pointer if the source address is not required" | `socket_api_recvmsg.msg_name_null` | covered |
| "if the socket is unconnected, the msg_name member points to a sockaddr structure in which the source address is to be stored" | `socket_api_recvmsg.dgram_source_address` | covered |
| MSG_PEEK: "Peeks at the incoming message" | `socket_api_recvmsg.msg_peek` | covered |
| MSG_WAITALL: "On SOCK_STREAM sockets this requests that the function block until the full amount of data can be returned" | `socket_api_recvmsg.msg_waitall_stream` | covered |
| "For message-based sockets... If a message is too long to fit in the supplied buffers, and MSG_PEEK is not set in the flags argument, the excess bytes shall be discarded, and MSG_TRUNC shall be set in the msg_flags member" | `socket_api_recvmsg.dgram_msg_trunc_flag` | covered |
| "If no messages are available at the socket and O_NONBLOCK is set on the socket's file descriptor, the recvmsg() function shall fail and set errno to [EAGAIN] or [EWOULDBLOCK]" | `socket_api_recvmsg.eagain_nonblock_no_data` | covered |
| "If no messages are available to be received and the peer has performed an orderly shutdown, recvmsg() shall return 0" | `socket_api_recvmsg.orderly_shutdown_returns_zero` | covered |
| [EAGAIN] or [EWOULDBLOCK]: "The socket's file descriptor is marked O_NONBLOCK and no data is waiting to be received" | `socket_api_recvmsg.eagain_nonblock_no_data` | covered |
| [EBADF]: "The socket argument is not a valid open file descriptor" | `socket_api_recvmsg.ebadf_invalid_fd` | covered |
| [ENOTCONN]: "A receive is attempted on a connection-mode socket that is not connected" | `socket_api_recvmsg.enotconn_unconnected_stream` | covered |
| [ENOTSOCK]: "The socket argument does not refer to a socket" | `socket_api_recvmsg.enotsock_pipe_fd` | covered |
| [ECONNRESET]: "A connection was forcibly closed by a peer" | — | not tested: cannot reliably trigger on AF_UNIX |
| [EINTR]: "This function was interrupted by a signal before any data was available" | — | not tested: requires multi-threading to block and then signal |
| [EINVAL]: "The sum of the iov_len values overflows a ssize_t, or the MSG_OOB flag is set and no out-of-band data is available" | — | not tested: iov_len overflow requires unreasonably large values; OOB not on AF_UNIX |
| [EMSGSIZE]: "The msg_iovlen member of the msghdr structure is less than or equal to 0, or is greater than {IOV_MAX}" | — | not tested: msg_iovlen=0 does not portably return EMSGSIZE on all implementations |
| [EOPNOTSUPP]: "The specified flags are not supported for this socket type" | — | not tested: no portable unsupported flag value |
| [ETIMEDOUT]: "The connection timed out during connection establishment, or due to a transmission timeout on active connection" | — | not tested: cannot trigger on AF_UNIX sockets |
