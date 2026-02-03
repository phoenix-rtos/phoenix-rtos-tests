/*
 * Phoenix-RTOS
 *
 * network tests common routines
 *
 * Copyright 2026 Phoenix Systems
 * Author: Adam Debek
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _NETWORK_COMMON_H_
#define _NETWORK_COMMON_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#define EVENT_RECV_EOF     (1 << 0)
#define EVENT_SEND_BLOCKED (1 << 1)

#define SEND_CMD(cmd) \
	do { \
		fprintf(cmd_stream, "NET: %s\n", cmd); \
		fflush(cmd_stream); \
	} while (0)

#define SEND_CMD_F(fmt, ...) \
	do { \
		fprintf(cmd_stream, "NET: " fmt "\n", ##__VA_ARGS__); \
		fflush(cmd_stream); \
	} while (0)

extern const uint16_t port;

bool interface_is_running(const char *iface);

int open_connection(const char *addr, const uint16_t port);

int setup_cmd_conn(void);

int init_connection_ips(int cmd_conn_fd, char *ip, char *peer_ip);

int send_all(int sock, const uint8_t *buf, size_t len, int flags);

int recv_all(int sock, uint8_t *buf, size_t len, int flags);

int sendmsg_all(int sock, struct iovec *iov, int iovcnt, int flags);

int recvmsg_all(int sock, struct iovec *iov, int iovcnt, int flags);

#endif
