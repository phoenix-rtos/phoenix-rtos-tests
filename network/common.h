/*
 * Phoenix-RTOS
 *
 * network tests common header
 *
 * Copyright 2024 Phoenix Systems
 * Author: Adam Debek
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <stdint.h>

#ifndef _TEST_NETWORK_COMMON_H
#define _TEST_NETWORK_COMMON_H

#define sendall(_sockfd, _buffer, _length, _flags) \
	do { \
		int ret; \
		ssize_t n = 0; \
		size_t total = 0; \
		size_t left = _length; \
		size_t send_len = _length; \
		struct pollfd fds[1]; \
		fds[0].fd = _sockfd; \
		fds[0].events = POLLOUT; \
		fds[0].revents = 0; \
		while (n < _length && (ret = poll(fds, 1, 5000))) { \
			if (fds[0].revents & POLLOUT) { \
				n = send(_sockfd, _buffer + total, send_len, _flags); \
				if (n > 0) { \
					total += n; \
					left -= n; \
					if (left < send_len) { \
						send_len = left; \
					} \
				} \
				else if (n < 0 && errno == EPIPE) { \
					TEST_FAIL_MESSAGE("sendall: host closed connection"); \
				} \
				else if (n < 0 && errno == EMSGSIZE) { \
					send_len /= 2; \
					continue; \
				} \
				else if (n < 0) { \
					char err_msg[64]; \
					sprintf(err_msg, "sendall: %s", strerror(errno)); \
					TEST_FAIL_MESSAGE(err_msg); \
				} \
			} \
			else { \
				TEST_FAIL_MESSAGE("sendall: host hangup"); \
			} \
		} \
		if (ret == 0) { \
			TEST_FAIL_MESSAGE("sendall: poll timeout"); \
		} \
		else if (n != _length) { \
			TEST_FAIL_MESSAGE("sendall: didn't send all bytes"); \
		} \
	} while (0)


#define recvall(_sockfd, _buffer, _length, _flags) \
	do { \
		int ret; \
		ssize_t r = 0; \
		size_t total = 0; \
		struct pollfd fds[1]; \
		fds[0].fd = _sockfd; \
		fds[0].events = POLLIN; \
		fds[0].revents = 0; \
		while (r < _length && (ret = poll(fds, 1, 5000))) { \
			if (fds[0].revents & POLLIN) { \
				r = recv(_sockfd, _buffer, _length, _flags); \
				if (r > 0) { \
					total += r; \
				} \
				else if (r == 0) { \
					TEST_FAIL_MESSAGE("recvall: host closed connection"); \
				} \
				else if (r < 0) { \
					char err_msg[64]; \
					sprintf(err_msg, "recvall: %s", strerror(errno)); \
					TEST_FAIL_MESSAGE(err_msg); \
				} \
			} \
			else { \
				TEST_FAIL_MESSAGE("recvall: host hangup"); \
			} \
		} \
		if (ret == 0) { \
			TEST_FAIL_MESSAGE("recvall: poll timeout"); \
		} \
		else if (total != _length) { \
			TEST_FAIL_MESSAGE("recvall: didn't receive all bytes"); \
		} \
	} while (0)


#define sendall_child(_sockfd, _buffer, _length, _flags) \
	do { \
		int ret; \
		ssize_t n = 0; \
		size_t total = 0; \
		size_t left = _length; \
		size_t send_len = _length; \
		struct pollfd fds[1]; \
		fds[0].fd = _sockfd; \
		fds[0].events = POLLOUT; \
		fds[0].revents = 0; \
		while (n < _length && (ret = poll(fds, 1, 5000))) { \
			if (fds[0].revents & POLLOUT) { \
				n = send(_sockfd, _buffer + total, send_len, _flags); \
				if (n > 0) { \
					total += n; \
					left -= n; \
					if (left < send_len) { \
						send_len = left; \
					} \
				} \
				else if (n < 0 && errno == EPIPE) { \
					exit(2); \
				} \
				else if (n < 0 && errno == EMSGSIZE) { \
					send_len /= 2; \
					continue; \
				} \
				else if (n < 0) { \
					exit(3); \
				} \
			} \
			else { \
				exit(4); \
			} \
		} \
		if (ret == 0) { \
			exit(5); \
		} \
		else if (n != _length) { \
			exit(6); \
		} \
	} while (0)


#define recvall_child(_sockfd, _buffer, _length, _flags) \
	do { \
		int ret; \
		ssize_t r = 0; \
		size_t total = 0; \
		struct pollfd fds[1]; \
		fds[0].fd = _sockfd; \
		fds[0].events = POLLIN; \
		fds[0].revents = 0; \
		while (r < _length && (ret = poll(fds, 1, 5000))) { \
			if (fds[0].revents & POLLIN) { \
				r = recv(_sockfd, _buffer, _length, _flags); \
				if (r > 0) { \
					total += r; \
				} \
				else if (r == 0) { \
					exit(7); \
				} \
				else if (r < 0) { \
					exit(8); \
				} \
			} \
			else { \
				exit(9); \
			} \
		} \
		if (ret == 0) { \
			exit(10); \
		} \
		else if (total != _length) { \
			exit(11); \
		} \
	} while (0)


#define get_host_response(_syncfd, _buffer) \
	do { \
		ssize_t r; \
		r = recv(_syncfd, _buffer, sizeof _buffer, MSG_WAITALL); \
		if (r == 0) { \
			fprintf(stderr, "Sync socket: host closed connection\n"); \
			close(_syncfd); \
			exit(1); \
		} \
		else if (r < 0) { \
			perror("Sync socket"); \
			close(_syncfd); \
			exit(1); \
		} \
		else if (target_failed_flag == 0) { \
			host_response_flag = 1; \
			if (strcmp(_buffer, "success") != 0) { \
				TEST_FAIL_MESSAGE(_buffer); \
			} \
		} \
		else if (target_failed_flag == 1) { \
			if (strcmp(_buffer, "success") != 0) { \
				fprintf(stderr, "%s\n", _buffer); \
			} \
		} \
	} while (0)


#define print_child_error_msg(_val) \
	do { \
		char err_msg[64]; \
		switch (_val) { \
			case 1: \
				TEST_FAIL_MESSAGE("Failed to setup connection"); \
			case 2: \
				TEST_FAIL_MESSAGE("sendall_child: host closed connection"); \
			case 3: \
				sprintf(err_msg, "sendall_child: %s", strerror(errno)); \
				TEST_FAIL_MESSAGE(err_msg); \
			case 4: \
				TEST_FAIL_MESSAGE("sendall_child: host hangup"); \
			case 5: \
				TEST_FAIL_MESSAGE("sendall_child: poll timeout"); \
			case 6: \
				TEST_FAIL_MESSAGE("sendall_child: didn't send all bytes"); \
			case 7: \
				TEST_FAIL_MESSAGE("recvall_child: host closed connection"); \
			case 8: \
				sprintf(err_msg, "recvall_child: %s", strerror(errno)); \
				TEST_FAIL_MESSAGE(err_msg); \
			case 9: \
				TEST_FAIL_MESSAGE("recvall_child: host hangup"); \
			case 10: \
				TEST_FAIL_MESSAGE("recvall_child: poll timeout"); \
			case 11: \
				TEST_FAIL_MESSAGE("recvall_child: didn't receive all bytes"); \
			default: \
				TEST_FAIL_MESSAGE("Child returned invalid value"); \
		} \
	} while (0)


/* If sport is 0, source port will be assigned by kernel */
int create_con(const char *daddr, uint16_t dport);

/* Wait for running interface */
int wait_if_running(void);

#endif /* TEST_NETWORK_COMMON_H */
