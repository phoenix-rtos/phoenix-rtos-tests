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

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "common.h"

bool interface_is_running(const char *iface)
{
	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0)
		return false;

	struct ifreq ifr;
	strncpy(ifr.ifr_name, iface, IFNAMSIZ - 1);
	ifr.ifr_name[IFNAMSIZ - 1] = '\0';

	int try = 20;
	do {
		if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0) {
			perror("ioctl");
			close(sock);
			return false;
		}

		if (ifr.ifr_flags & IFF_RUNNING) {
			close(sock);
			return true;
		}

		usleep(200000);  // 200 ms
	} while (--try);

	close(sock);
	return false;
}


int init_connection_ips(int cmd_conn_fd, char *ip, char *peer_ip)
{
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof addr;

	if (getsockname(cmd_conn_fd, (struct sockaddr *)&addr, &addrlen) < 0) {
		perror("getsockname");
		close(cmd_conn_fd);
		return -1;
	}

	strcpy(ip, inet_ntoa(addr.sin_addr));

	if (getpeername(cmd_conn_fd, (struct sockaddr *)&addr, &addrlen) < 0) {
		perror("getpeername");
		close(cmd_conn_fd);
		return -1;
	}

	strcpy(peer_ip, inet_ntoa(addr.sin_addr));

	return 0;
}


int setup_cmd_conn(void)
{
	int peer;
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
		return -1;

	int opt = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	struct sockaddr_in sin = {
		.sin_family = AF_INET,
		.sin_port = htons(port),
		.sin_addr.s_addr = htonl(INADDR_ANY)
	};

	if (bind(sock, (struct sockaddr *)&sin, sizeof sin) < 0) {
		perror("bind");
		close(sock);
		return -1;
	}

	if (listen(sock, 1) < 0) {
		perror("listen");
		close(sock);
		return -1;
	}

	/* network harness should initiate a connection */
	peer = accept(sock, NULL, NULL);
	if (peer < 0) {
		perror("accept");
		close(sock);
		return -1;
	}

	close(sock);
	return peer;
}


int open_connection(const char *addr, const uint16_t port)
{
	struct sockaddr_in sin = {
		.sin_family = AF_INET,
		.sin_port = htons(port),
		.sin_addr.s_addr = inet_addr(addr)
	};

	int saved_errno = 0;
	int try = 5;
	do {
		int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (sock < 0)
			return -1;

		if (connect(sock, (struct sockaddr *)&sin, sizeof sin) == 0)
			return sock;

		saved_errno = errno;
		close(sock);
		usleep(500000);  // 500 ms
	} while (--try);

	errno = saved_errno;
	return -1;
}


int send_all(int sock, const uint8_t *buf, size_t len, int flags)
{
	size_t sent_total = 0;

	while (sent_total < len) {
		ssize_t sent = send(sock, buf + sent_total, len - sent_total, flags);
		if (sent <= 0)
			return -1;

		sent_total += (size_t)sent;
	}

	return 0;
}


int recv_all(int sock, uint8_t *buf, size_t len, int flags)
{
	size_t received_total = 0;

	while (received_total < len) {
		ssize_t r = recv(sock, buf + received_total, len - received_total, flags);
		if (r <= 0)
			return -1;

		received_total += (size_t)r;
	}

	return 0;
}


static void iov_advance(struct iovec **iov, int *iovcnt, size_t n)
{
	while (*iovcnt > 0 && n > 0) {
		if (n >= (*iov)->iov_len) {
			n -= (*iov)->iov_len;
			(*iov)++;
			(*iovcnt)--;
		}
		else {
			(*iov)->iov_base = (uint8_t *)(*iov)->iov_base + n;
			(*iov)->iov_len -= n;
			n = 0;
		}
	}
}


int sendmsg_all(int sock, struct iovec *iov, int iovcnt, int flags)
{
	struct msghdr msg = { 0 };
	msg.msg_iov = iov;
	msg.msg_iovlen = iovcnt;

	while (msg.msg_iovlen > 0) {
		ssize_t sent = sendmsg(sock, &msg, flags);
		if (sent <= 0)
			return -1;

		iov_advance(&msg.msg_iov, &msg.msg_iovlen, (size_t)sent);
	}

	return 0;
}


int recvmsg_all(int sock, struct iovec *iov, int iovcnt, int flags)
{
	struct msghdr msg = { 0 };
	msg.msg_iov = iov;
	msg.msg_iovlen = iovcnt;

	while (msg.msg_iovlen > 0) {
		ssize_t r = recvmsg(sock, &msg, flags);
		if (r <= 0)
			return -1;

		iov_advance(&msg.msg_iov, &msg.msg_iovlen, (size_t)r);
	}

	return 0;
}
