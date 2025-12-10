/*
 * Phoenix-RTOS
 *
 * network tests common routines
 *
 * Copyright 2025 Phoenix Systems
 * Author: Adam Debek
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _LIBPHOENIX_MATH_COMMON_H_
#define _LIBPHOENIX_MATH_COMMON_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#define SEND_CMD(cmd) \
	do { \
		printf("NET: %s\n", cmd); \
	} while (0)

#define SEND_CMD_F(fmt, ...) \
	do { \
		printf("NET: " fmt "\n", ##__VA_ARGS__); \
	} while (0)

extern const uint16_t port;

bool interface_is_running(const char *iface);

int open_connection(const char *addr, const uint16_t port);

int init_connection_ips(char *ip, char *peer_ip);

int send_all(int sock, const uint8_t *buf, size_t len);

int recv_all(int sock, uint8_t *buf, size_t len);

int sendmsg_all(int sock, struct iovec *iov, int iovcnt);

int recvmsg_all(int sock, struct iovec *iov, int iovcnt);

#endif
