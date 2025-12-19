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

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
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
			close(sock);
			return false;
		}

		if (ifr.ifr_flags & IFF_RUNNING) {
			close(sock);
			return true;
		}

		usleep(200000);  // 200 ms
	} while (--try);

	return false;
}


int init_connection_ips(char *ip, char *peer_ip)
{
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof addr;
	int peer;
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
		return -1;

	int opt = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	struct sockaddr_in sin = {
		.sin_family = AF_INET,
		.sin_port = htons(port),
		.sin_addr.s_addr = INADDR_ANY
	};

	if (bind(sock, (struct sockaddr *)&sin, sizeof sin) < 0) {
		close(sock);
		return -1;
	}

	if (listen(sock, 1) < 0) {
		close(sock);
		return -1;
	}

	SEND_CMD("Accept");
	peer = accept(sock, NULL, NULL);
	if (peer < 0) {
		close(sock);
		return -1;
	}

	if (getsockname(peer, (struct sockaddr *)&addr, &addrlen) < 0) {
		close(sock), close(peer);
		return -1;
	}

	strcpy(ip, inet_ntoa(addr.sin_addr));

	if (getpeername(peer, (struct sockaddr *)&addr, &addrlen) < 0) {
		close(sock), close(peer);
		return -1;
	}

	strcpy(peer_ip, inet_ntoa(addr.sin_addr));

	close(sock);
	close(peer);

	return 0;
}


int open_connection(const char *addr, const uint16_t port)
{
	struct sockaddr_in sin = {
		.sin_family = AF_INET,
		.sin_port = htons(port),
		.sin_addr.s_addr = inet_addr(addr)
	};

	int try = 5;
	do {
		int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (sock < 0)
			return -1;

		if (connect(sock, (struct sockaddr *)&sin, sizeof sin) == 0)
			return sock;

		close(sock);
		usleep(500000);  // 500 ms
	} while (--try);

	return -1;
}


int send_all(int sock, const uint8_t *buf, size_t len)
{
	size_t sent_total = 0;

	while (sent_total < len) {
		ssize_t sent = send(sock, buf + sent_total, len - sent_total, 0);
		if (sent <= 0)
			return -1;

		sent_total += (size_t)sent;
	}

	return 0;
}


int recv_all(int sock, uint8_t *buf, size_t len)
{
	size_t received_total = 0;

	while (received_total < len) {
		ssize_t r = recv(sock, buf + received_total, len - received_total, 0);
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


int sendmsg_all(int sock, struct iovec *iov, int iovcnt)
{
	struct msghdr msg = { 0 };
	msg.msg_iov = iov;
	msg.msg_iovlen = iovcnt;

	while (msg.msg_iovlen > 0) {
		ssize_t sent = sendmsg(sock, &msg, 0);
		if (sent <= 0)
			return -1;

		iov_advance(&msg.msg_iov, &msg.msg_iovlen, (size_t)sent);
	}

	return 0;
}


int recvmsg_all(int sock, struct iovec *iov, int iovcnt)
{
	struct msghdr msg = { 0 };
	msg.msg_iov = iov;
	msg.msg_iovlen = iovcnt;

	while (msg.msg_iovlen > 0) {
		ssize_t r = recvmsg(sock, &msg, 0);
		if (r <= 0)
			return -1;

		iov_advance(&msg.msg_iov, &msg.msg_iovlen, (size_t)r);
	}

	return 0;
}
