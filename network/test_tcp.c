/*
 * Phoenix-RTOS
 *
 * tcp test
 *
 * Copyright 2026 Phoenix Systems
 * Author: Adam Debek
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include <unity_fixture.h>

#include "common.h"

#define MSS_SIZE 1460
#define ITER_CNT 25

int sock, cmd_sock;
FILE *cmd_stream;
char ip[INET_ADDRSTRLEN], peer_ip[INET_ADDRSTRLEN];
const uint16_t port = 50000;
uint8_t *rx_buf, *tx_buf;
bool peer_closed;


TEST_GROUP(tcp);


TEST_SETUP(tcp)
{
	peer_closed = false;
	SEND_CMD("Accept");
	sock = open_connection(peer_ip, port);
	if (sock < 0) {
		char msg[64];
		snprintf(msg, sizeof msg, "Failed to establish TCP connection: %s", strerror(errno));
		TEST_FAIL_MESSAGE(msg);
	}
}


TEST_TEAR_DOWN(tcp)
{
	if (!peer_closed) {
		SEND_CMD("Close");
	}

	if (sock >= 0) {
		close(sock);
		sock = -1;
	}
}


TEST(tcp, send_recv)
{
	TEST_ASSERT_NOT_NULL(rx_buf = (uint8_t *)malloc(MSS_SIZE));
	TEST_ASSERT_NOT_NULL(tx_buf = (uint8_t *)malloc(MSS_SIZE));

	int flag = 1;
	TEST_ASSERT_EQUAL(0, setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int)));

	for (int i = 0; i < ITER_CNT; i++) {
		size_t len = 1 + (size_t)(rand() % MSS_SIZE);

		for (size_t n = 0; n < len; n++) {
			tx_buf[n] = (uint8_t)(rand() % 256);
		}

		SEND_CMD_F("Receive %zu", len);
		TEST_ASSERT_EQUAL(len, send(sock, tx_buf, len, 0));

		SEND_CMD_F("Send received %zu", len);
		TEST_ASSERT_EQUAL(0, recv_all(sock, rx_buf, len, 0));

		TEST_ASSERT_EQUAL_MEMORY(tx_buf, rx_buf, len);
	}

	free(rx_buf);
	free(tx_buf);
}


TEST(tcp, send_recv_over_mss)
{
	size_t buf_size = 20 * 1024;

	TEST_ASSERT_NOT_NULL(rx_buf = (uint8_t *)malloc(buf_size));
	TEST_ASSERT_NOT_NULL(tx_buf = (uint8_t *)malloc(buf_size));

	for (int i = 0; i < ITER_CNT; i++) {
		for (size_t n = 0; n < buf_size; n++) {
			tx_buf[n] = (uint8_t)(rand() % 256);
		}

		SEND_CMD_F("Receive %zu", buf_size);
		TEST_ASSERT_EQUAL(0, send_all(sock, tx_buf, buf_size, 0));

		SEND_CMD_F("Send received %zu", buf_size);
		TEST_ASSERT_EQUAL(0, recv_all(sock, rx_buf, buf_size, 0));

		TEST_ASSERT_EQUAL_MEMORY(tx_buf, rx_buf, buf_size);
	}

	free(rx_buf);
	free(tx_buf);
}


TEST(tcp, send_recv_vectored)
{
	uint8_t *rx_buf[4], *tx_buf[4];
	size_t iov_len = MSS_SIZE / 4;

	int flag = 1;
	TEST_ASSERT_EQUAL(0, setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int)));

	for (int i = 0; i < 4; i++) {
		TEST_ASSERT_NOT_NULL(rx_buf[i] = (uint8_t *)malloc(iov_len));
		TEST_ASSERT_NOT_NULL(tx_buf[i] = (uint8_t *)malloc(iov_len));
	}

	struct iovec siov[4], riov[4];
	struct iovec siov_work[4], riov_work[4];

	for (int i = 0; i < 4; i++) {
		riov[i].iov_base = rx_buf[i];
		riov[i].iov_len = iov_len;

		siov[i].iov_base = tx_buf[i];
		siov[i].iov_len = iov_len;
	}

	for (int i = 0; i < ITER_CNT; i++) {
		for (int j = 0; j < 4; j++) {
			for (size_t n = 0; n < iov_len; n++) {
				tx_buf[j][n] = (uint8_t)(rand() % 256);
			}
		}

		memcpy(siov_work, siov, sizeof siov);
		memcpy(riov_work, riov, sizeof riov);

		SEND_CMD_F("Receive %zu", (size_t)MSS_SIZE);
		TEST_ASSERT_EQUAL(0, sendmsg_all(sock, siov_work, 4, 0));

		SEND_CMD_F("Send received %zu", (size_t)MSS_SIZE);
		TEST_ASSERT_EQUAL(0, recvmsg_all(sock, riov_work, 4, 0));

		TEST_ASSERT_EQUAL_MEMORY(tx_buf[0], rx_buf[0], iov_len);
		TEST_ASSERT_EQUAL_MEMORY(tx_buf[1], rx_buf[1], iov_len);
		TEST_ASSERT_EQUAL_MEMORY(tx_buf[2], rx_buf[2], iov_len);
		TEST_ASSERT_EQUAL_MEMORY(tx_buf[3], rx_buf[3], iov_len);
	}

	for (int i = 0; i < 4; i++) {
		free(rx_buf[i]);
		free(tx_buf[i]);
	}
}


TEST(tcp, send_recv_vectored_over_mss)
{
	uint8_t *rx_buf[4], *tx_buf[4];
	size_t iov_len = 4096;

	for (int i = 0; i < 4; i++) {
		TEST_ASSERT_NOT_NULL(rx_buf[i] = (uint8_t *)malloc(iov_len));
		TEST_ASSERT_NOT_NULL(tx_buf[i] = (uint8_t *)malloc(iov_len));
	}

	struct iovec siov[4], riov[4];
	struct iovec siov_work[4], riov_work[4];

	for (int i = 0; i < 4; i++) {
		riov[i].iov_base = rx_buf[i];
		riov[i].iov_len = iov_len;

		siov[i].iov_base = tx_buf[i];
		siov[i].iov_len = iov_len;
	}

	for (int i = 0; i < ITER_CNT; i++) {
		for (int j = 0; j < 4; j++) {
			for (size_t n = 0; n < iov_len; n++) {
				tx_buf[j][n] = (uint8_t)(rand() % 256);
			}
		}

		memcpy(siov_work, siov, sizeof siov);
		memcpy(riov_work, riov, sizeof riov);

		SEND_CMD_F("Receive %zu", 4 * iov_len);
		TEST_ASSERT_EQUAL(0, sendmsg_all(sock, siov_work, 4, 0));

		SEND_CMD_F("Send received %zu", 4 * iov_len);
		TEST_ASSERT_EQUAL(0, recvmsg_all(sock, riov_work, 4, 0));

		TEST_ASSERT_EQUAL_MEMORY(tx_buf[0], rx_buf[0], iov_len);
		TEST_ASSERT_EQUAL_MEMORY(tx_buf[1], rx_buf[1], iov_len);
		TEST_ASSERT_EQUAL_MEMORY(tx_buf[2], rx_buf[2], iov_len);
		TEST_ASSERT_EQUAL_MEMORY(tx_buf[3], rx_buf[3], iov_len);
	}

	for (int i = 0; i < 4; i++) {
		free(rx_buf[i]);
		free(tx_buf[i]);
	}
}


TEST(tcp, multi_send_single_recv)
{
	size_t len, len_total, pos;

	TEST_ASSERT_NOT_NULL(rx_buf = (uint8_t *)malloc(MSS_SIZE));
	TEST_ASSERT_NOT_NULL(tx_buf = (uint8_t *)malloc(MSS_SIZE));

	int flag = 1;
	TEST_ASSERT_EQUAL(0, setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int)));

	for (int i = 0; i < ITER_CNT; i++) {
		pos = len_total = 0;
		for (int j = 0; j < 4; j++) {
			len = 1 + (size_t)(rand() % (MSS_SIZE / 4));

			for (size_t n = 0; n < len; n++) {
				tx_buf[pos + n] = (uint8_t)(rand() % 256);
			}

			SEND_CMD_F("Receive %zu", len);
			TEST_ASSERT_EQUAL(len, send(sock, tx_buf + pos, len, 0));

			pos = len_total += len;
		}

		SEND_CMD_F("Send received %zu", len_total);
		TEST_ASSERT_EQUAL(0, recv_all(sock, rx_buf, len_total, 0));

		TEST_ASSERT_EQUAL_MEMORY(tx_buf, rx_buf, len_total);
	}

	free(rx_buf);
	free(tx_buf);
}


TEST(tcp, multi_send_single_recv_over_mss)
{
	size_t len, len_total, pos, buf_size = 20 * 1024;

	TEST_ASSERT_NOT_NULL(rx_buf = (uint8_t *)malloc(buf_size));
	TEST_ASSERT_NOT_NULL(tx_buf = (uint8_t *)malloc(buf_size));

	for (int i = 0; i < ITER_CNT; i++) {
		pos = len_total = 0;
		for (int j = 0; j < 4; j++) {
			len = 1 + (size_t)(rand() % (buf_size / 4));

			for (size_t n = 0; n < len; n++) {
				tx_buf[pos + n] = (uint8_t)(rand() % 256);
			}

			SEND_CMD_F("Receive %zu", len);
			TEST_ASSERT_EQUAL(0, send_all(sock, tx_buf + pos, len, 0));

			pos = len_total += len;
		}

		SEND_CMD_F("Send received %zu", len_total);
		TEST_ASSERT_EQUAL(0, recv_all(sock, rx_buf, len_total, 0));

		TEST_ASSERT_EQUAL_MEMORY(tx_buf, rx_buf, len_total);
	}

	free(rx_buf);
	free(tx_buf);
}


TEST(tcp, single_send_multi_recv)
{
	size_t len, len_total, remaining, pos;

	TEST_ASSERT_NOT_NULL(rx_buf = (uint8_t *)malloc(MSS_SIZE));
	TEST_ASSERT_NOT_NULL(tx_buf = (uint8_t *)malloc(MSS_SIZE));

	int flag = 1;
	TEST_ASSERT_EQUAL(0, setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int)));

	for (int i = 0; i < ITER_CNT; i++) {
		for (size_t n = 0; n < MSS_SIZE; n++) {
			tx_buf[n] = (uint8_t)(rand() % 256);
		}

		SEND_CMD_F("Receive %zu", (size_t)MSS_SIZE);
		TEST_ASSERT_EQUAL(MSS_SIZE, send(sock, tx_buf, MSS_SIZE, 0));

		pos = len_total = 0;
		for (int j = 0; j < 4; j++) {
			len = 1 + (size_t)(rand() % 128);

			SEND_CMD_F("Send received %zu", len);
			TEST_ASSERT_EQUAL(0, recv_all(sock, rx_buf + pos, len, 0));

			pos = len_total += len;
		}

		remaining = MSS_SIZE - len_total;
		SEND_CMD_F("Send received %zu", remaining);
		TEST_ASSERT_EQUAL(0, recv_all(sock, rx_buf + pos, remaining, 0));

		TEST_ASSERT_EQUAL_MEMORY(tx_buf, rx_buf, MSS_SIZE);
	}

	free(rx_buf);
	free(tx_buf);
}


TEST(tcp, single_send_multi_recv_over_mss)
{
	size_t len, len_total, remaining, pos, buf_size = 20 * 1024;

	TEST_ASSERT_NOT_NULL(rx_buf = (uint8_t *)malloc(buf_size));
	TEST_ASSERT_NOT_NULL(tx_buf = (uint8_t *)malloc(buf_size));

	for (int i = 0; i < ITER_CNT; i++) {
		for (size_t n = 0; n < buf_size; n++) {
			tx_buf[n] = (uint8_t)(rand() % 256);
		}

		SEND_CMD_F("Receive %zu", buf_size);
		TEST_ASSERT_EQUAL(0, send_all(sock, tx_buf, buf_size, 0));

		pos = len_total = 0;
		for (int j = 0; j < 4; j++) {
			len = 1 + (size_t)(rand() % (buf_size / 4));

			SEND_CMD_F("Send received %zu", len);
			TEST_ASSERT_EQUAL(0, recv_all(sock, rx_buf + pos, len, 0));

			pos = len_total += len;
		}

		remaining = buf_size - len_total;
		SEND_CMD_F("Send received %zu", remaining);
		TEST_ASSERT_EQUAL(0, recv_all(sock, rx_buf + pos, remaining, 0));

		TEST_ASSERT_EQUAL_MEMORY(tx_buf, rx_buf, buf_size);
	}

	free(rx_buf);
	free(tx_buf);
}


TEST(tcp, recv_timeout)
{
	uint8_t rx_buf[512];
	struct timeval start, end, timeout;

	timeout.tv_sec = 5;
	timeout.tv_usec = 0;

	TEST_ASSERT_EQUAL(0, setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout));

	gettimeofday(&start, NULL);

	TEST_ASSERT_EQUAL(-1, recv(sock, rx_buf, sizeof rx_buf, 0));

	gettimeofday(&end, NULL);

	double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;

	TEST_ASSERT_DOUBLE_WITHIN(1.0, 5.0, elapsed);

	TEST_ASSERT_TRUE_MESSAGE(
			errno == EWOULDBLOCK || errno == EAGAIN,
			"expected errno to be EWOULDBLOCK or EAGAIN");
}


TEST(tcp, fill_window_tx)
{
	uint8_t tx_buf[512];
	struct timeval start, end, timeout;

	timeout.tv_sec = 5;
	timeout.tv_usec = 0;

	TEST_ASSERT_EQUAL(0, setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof timeout));

	ssize_t ret = 1;
	do {
		TEST_ASSERT_GREATER_THAN(0, ret);
		gettimeofday(&start, NULL);
		ret = send(sock, tx_buf, sizeof tx_buf, 0);
		gettimeofday(&end, NULL);
	} while (ret > 0);

	double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;

	TEST_ASSERT_DOUBLE_WITHIN(1.5, 5.0, elapsed);

	TEST_ASSERT_TRUE_MESSAGE(
			errno == EWOULDBLOCK || errno == EAGAIN,
			"expected errno to be EWOULDBLOCK or EAGAIN");
}


TEST(tcp, fill_window_rx)
{
	char msg[64];
	int events;

	/* send until peer send buffer is full */
	SEND_CMD_F("Send %d", 1000000);

	sleep(3);
	SEND_CMD("Get events");

	TEST_ASSERT_NOT_NULL(fgets(msg, sizeof msg, cmd_stream));
	TEST_ASSERT_EQUAL(1, sscanf(msg, "NET: %d", &events));

	TEST_ASSERT_TRUE(events & EVENT_SEND_BLOCKED);
}


TEST(tcp, reuseaddr)
{
	struct sockaddr_in sin = {
		.sin_family = AF_INET,
		.sin_port = htons(12345),
		.sin_addr.s_addr = htonl(INADDR_LOOPBACK)
	};

	int useaddr_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	TEST_ASSERT_GREATER_OR_EQUAL(0, useaddr_sock);

	TEST_ASSERT_EQUAL(0, bind(useaddr_sock, (struct sockaddr *)&sin, sizeof sin));

	int reuseaddr_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	TEST_ASSERT_GREATER_OR_EQUAL(0, reuseaddr_sock);

	TEST_ASSERT_EQUAL(-1, bind(reuseaddr_sock, (struct sockaddr *)&sin, sizeof sin));
	TEST_ASSERT_EQUAL(EADDRINUSE, errno);

	close(useaddr_sock);

	int opt = 1;
	TEST_ASSERT_EQUAL(0, setsockopt(reuseaddr_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt));
	TEST_ASSERT_EQUAL(0, bind(reuseaddr_sock, (struct sockaddr *)&sin, sizeof sin));

	close(reuseaddr_sock);
}


TEST(tcp, peek)
{
	uint8_t rx_buf[128];
	uint8_t tx_buf[128];
	size_t len = sizeof tx_buf;

	struct timeval timeout = {
		.tv_sec = 5,
		.tv_usec = 0
	};

	TEST_ASSERT_EQUAL(0, setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout));

	for (size_t n = 0; n < len; n++) {
		tx_buf[n] = (uint8_t)(rand() % 256);
	}

	SEND_CMD_F("Receive %zu", len);
	TEST_ASSERT_EQUAL(len, send(sock, tx_buf, len, 0));

	SEND_CMD_F("Send received %zu", len);
	TEST_ASSERT_EQUAL(0, recv_all(sock, rx_buf, len, MSG_PEEK));
	TEST_ASSERT_EQUAL_MEMORY(tx_buf, rx_buf, len);

	SEND_CMD_F("Send received %zu", len);
	TEST_ASSERT_EQUAL(0, recv_all(sock, rx_buf, len, 0));
	TEST_ASSERT_EQUAL_MEMORY(tx_buf, rx_buf, len);
}


TEST(tcp, dont_wait)
{
	uint8_t rx_buf[128];

	TEST_ASSERT_EQUAL(-1, recv(sock, rx_buf, sizeof rx_buf, MSG_DONTWAIT));
	TEST_ASSERT_TRUE_MESSAGE(
			errno == EWOULDBLOCK || errno == EAGAIN,
			"expected errno to be EWOULDBLOCK or EAGAIN");
}


TEST(tcp, recv_closed_connection)
{
	uint8_t rx_buf[128];
	uint8_t tx_buf[128];
	size_t len = sizeof tx_buf;

	for (size_t n = 0; n < len; n++) {
		tx_buf[n] = (uint8_t)(rand() % 256);
	}

	SEND_CMD_F("Receive %zu", len);
	TEST_ASSERT_EQUAL(len, send(sock, tx_buf, len, 0));

	SEND_CMD_F("Send received %zu", len);
	TEST_ASSERT_EQUAL(0, recv_all(sock, rx_buf, len, MSG_PEEK));
	TEST_ASSERT_EQUAL_MEMORY(tx_buf, rx_buf, len);

	SEND_CMD("Close forcibly");
	peer_closed = true;
	sleep(1);

	TEST_ASSERT_EQUAL(0, recv_all(sock, rx_buf, len, 0));
	TEST_ASSERT_EQUAL_MEMORY(tx_buf, rx_buf, len);

	TEST_ASSERT_EQUAL(-1, recv(sock, rx_buf, sizeof rx_buf, 0));
	TEST_ASSERT_EQUAL(ECONNRESET, errno);
}


TEST(tcp, send_closed_connection)
{
	uint8_t tx_buf[128];
	size_t len = sizeof tx_buf;

	SEND_CMD("Close forcibly");
	peer_closed = true;
	sleep(1);

	TEST_ASSERT_EQUAL(-1, send(sock, tx_buf, len, 0));
	TEST_ASSERT_EQUAL(ECONNRESET, errno);
}


TEST(tcp, read_shutdown)
{
	uint8_t rx_buf[128];
	uint8_t tx_buf[128];
	size_t len = sizeof tx_buf;
	ssize_t ret;

	shutdown(sock, SHUT_RD);
	ret = recv_all(sock, rx_buf, len, 0);
	TEST_ASSERT_TRUE(ret == 0 || ret == -1);

	TEST_ASSERT_EQUAL(len, send(sock, tx_buf, len, 0));
}


TEST(tcp, write_shutdown)
{
	uint8_t rx_buf[128];
	uint8_t tx_buf[128];
	size_t len = sizeof tx_buf;
	char msg[64];
	int events;

	for (size_t n = 0; n < len; n++) {
		tx_buf[n] = (uint8_t)(rand() % 256);
	}

	SEND_CMD_F("Receive %zu", len);
	TEST_ASSERT_EQUAL(len, send(sock, tx_buf, len, 0));

	shutdown(sock, SHUT_WR);

	/* trigger EOF on peer side */
	SEND_CMD_F("Receive %zu", len);

	SEND_CMD_F("Send received %zu", len);
	TEST_ASSERT_EQUAL(0, recv_all(sock, rx_buf, len, 0));
	TEST_ASSERT_EQUAL_MEMORY(tx_buf, rx_buf, len);

	SEND_CMD("Get events");

	TEST_ASSERT_NOT_NULL(fgets(msg, sizeof msg, cmd_stream));
	TEST_ASSERT_EQUAL(1, sscanf(msg, "NET: %d", &events));

	TEST_ASSERT_TRUE(events & EVENT_RECV_EOF);
}


TEST(tcp, connect_unsuccessful)
{
	struct sockaddr_in sin;
	sin.sin_family = AF_INET;

	TEST_ASSERT_EQUAL(-1, connect(sock, (struct sockaddr *)&sin, sizeof sin));
	TEST_ASSERT_EQUAL(EISCONN, errno);

	close(sock), sock = -1;

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	TEST_ASSERT_GREATER_OR_EQUAL(0, sock);

	sin.sin_port = htons(port);
	sin.sin_addr.s_addr = inet_addr("8.8.8.8");

	TEST_ASSERT_EQUAL(-1, connect(sock, (struct sockaddr *)&sin, sizeof sin));
	TEST_ASSERT_EQUAL(EHOSTUNREACH, errno);

	sin.sin_port = htons(54321);
	sin.sin_addr.s_addr = inet_addr(peer_ip);

	TEST_ASSERT_EQUAL(-1, connect(sock, (struct sockaddr *)&sin, sizeof sin));
	TEST_ASSERT_EQUAL(ECONNRESET, errno);
}


TEST(tcp, multiple_connections)
{
	int connections[5];
	TEST_ASSERT_NOT_NULL(rx_buf = (uint8_t *)malloc(MSS_SIZE));
	TEST_ASSERT_NOT_NULL(tx_buf = (uint8_t *)malloc(MSS_SIZE));

	/* close unused test_setup connection */
	SEND_CMD("Close");
	peer_closed = true;

	for (int i = 0; i < 5; i++) {
		SEND_CMD("Accept");
		connections[i] = open_connection(peer_ip, port);
		TEST_ASSERT_GREATER_OR_EQUAL(0, connections[i]);
	}

	for (int i = 0; i < ITER_CNT; i++) {
		size_t len = 1 + (size_t)(rand() % MSS_SIZE);
		int idx = rand() % 5;

		for (size_t n = 0; n < len; n++) {
			tx_buf[n] = (uint8_t)(rand() % 256);
		}

		SEND_CMD_F("(%d) Receive %zu", idx, len);
		TEST_ASSERT_EQUAL(len, send(connections[idx], tx_buf, len, 0));

		SEND_CMD_F("(%d) Send received %zu", idx, len);
		TEST_ASSERT_EQUAL(0, recv_all(connections[idx], rx_buf, len, 0));

		TEST_ASSERT_EQUAL_MEMORY(tx_buf, rx_buf, len);
	}

	for (int i = 0; i < 5; i++) {
		SEND_CMD_F("(%d) Close", i);
		close(connections[i]);
	}

	free(rx_buf);
	free(tx_buf);
}


TEST(tcp, poll_timeout)
{
	struct timeval start, end;
	struct pollfd pfd;
	pfd.fd = sock;
	pfd.events = POLLIN;
	pfd.revents = 0;

	gettimeofday(&start, NULL);
	TEST_ASSERT_EQUAL(0, poll(&pfd, 1, 1000));
	TEST_ASSERT_EQUAL(0, pfd.revents);
	gettimeofday(&end, NULL);

	double elapsed_ms = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) / 1000;
	TEST_ASSERT_DOUBLE_WITHIN(250, 1000, elapsed_ms);
}


TEST(tcp, poll_accept)
{
	int listen_sock, peer_sock;
	struct sockaddr_in sin = {
		.sin_family = AF_INET,
		.sin_port = htons(port),
		.sin_addr.s_addr = inet_addr(ip)
	};

	/* close unused test_setup connection */
	SEND_CMD("Close");
	peer_closed = true;

	listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	TEST_ASSERT_GREATER_OR_EQUAL(0, listen_sock);

	int opt = 1;
	TEST_ASSERT_EQUAL(0, setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt));
	TEST_ASSERT_EQUAL(0, bind(listen_sock, (struct sockaddr *)&sin, sizeof sin));
	TEST_ASSERT_EQUAL(0, listen(listen_sock, 1));

	SEND_CMD("Connect");

	struct pollfd pfd;
	pfd.fd = listen_sock;
	pfd.events = POLLIN;
	pfd.revents = 0;

	TEST_ASSERT_EQUAL(1, poll(&pfd, 1, 5000));
	TEST_ASSERT_EQUAL(POLLIN, pfd.revents);

	peer_sock = accept(listen_sock, NULL, NULL);
	TEST_ASSERT_GREATER_OR_EQUAL(0, peer_sock);

	SEND_CMD("Close");
	close(listen_sock);
	close(peer_sock);
}


TEST(tcp, poll_rx_ready)
{
	uint8_t rx_buf[128];
	uint8_t tx_buf[128];
	size_t len = sizeof tx_buf;

	for (size_t n = 0; n < len; n++) {
		tx_buf[n] = (uint8_t)(rand() % 256);
	}

	SEND_CMD_F("Receive %zu", len);
	TEST_ASSERT_EQUAL(len, send(sock, tx_buf, len, 0));

	SEND_CMD_F("Send received %zu", len);

	struct pollfd pfd;
	pfd.fd = sock;
	pfd.events = POLLIN;
	pfd.revents = 0;

	TEST_ASSERT_EQUAL(1, poll(&pfd, 1, 1000));
	TEST_ASSERT_EQUAL(POLLIN, pfd.revents);

	TEST_ASSERT_EQUAL(0, recv_all(sock, rx_buf, len, 0));
	TEST_ASSERT_EQUAL_MEMORY(tx_buf, rx_buf, len);
}


TEST(tcp, poll_tx_ready)
{
	uint8_t tx_buf[128];
	size_t len = sizeof tx_buf;

	TEST_ASSERT_EQUAL(0, fcntl(sock, F_SETFL, O_NONBLOCK));
	ssize_t ret = 1;
	do {
		TEST_ASSERT_GREATER_THAN(0, ret);
		ret = send(sock, tx_buf, len, 0);
	} while (ret > 0);

	struct pollfd pfd;
	pfd.fd = sock;
	pfd.events = POLLOUT;
	pfd.revents = 0;

	TEST_ASSERT_EQUAL(0, poll(&pfd, 1, 0));
	TEST_ASSERT_EQUAL(0, pfd.revents);

	SEND_CMD_F("Send received %zu", len);

	TEST_ASSERT_EQUAL(1, poll(&pfd, 1, 5000));
	TEST_ASSERT_EQUAL(POLLOUT, pfd.revents);

	TEST_ASSERT_EQUAL(1, send(sock, tx_buf, 1, 0));
}


TEST(tcp, select_timeout)
{
	struct timeval start, end, timeout;
	fd_set rfds;

	FD_ZERO(&rfds);
	FD_SET(sock, &rfds);

	timeout.tv_sec = 1;
	timeout.tv_usec = 0;

	gettimeofday(&start, NULL);
	TEST_ASSERT_EQUAL(0, select(sock + 1, &rfds, NULL, NULL, &timeout));
	TEST_ASSERT_FALSE(FD_ISSET(sock, &rfds));
	gettimeofday(&end, NULL);

	double elapsed_ms = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) / 1000;
	TEST_ASSERT_DOUBLE_WITHIN(250, 1000, elapsed_ms);
}


TEST(tcp, select_accept)
{
	struct timeval timeout;
	int listen_sock, peer_sock;
	struct sockaddr_in sin = {
		.sin_family = AF_INET,
		.sin_port = htons(port),
		.sin_addr.s_addr = inet_addr(ip)
	};

	/* close unused test_setup connection */
	SEND_CMD("Close");
	peer_closed = true;

	listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	TEST_ASSERT_GREATER_OR_EQUAL(0, listen_sock);

	int opt = 1;
	TEST_ASSERT_EQUAL(0, setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt));
	TEST_ASSERT_EQUAL(0, bind(listen_sock, (struct sockaddr *)&sin, sizeof sin));
	TEST_ASSERT_EQUAL(0, listen(listen_sock, 1));

	SEND_CMD("Connect");

	fd_set rfds;
	FD_ZERO(&rfds);
	FD_SET(listen_sock, &rfds);

	timeout.tv_sec = 5;
	timeout.tv_usec = 0;

	TEST_ASSERT_EQUAL(1, select(listen_sock + 1, &rfds, NULL, NULL, &timeout));
	TEST_ASSERT_TRUE(FD_ISSET(listen_sock, &rfds));

	peer_sock = accept(listen_sock, NULL, NULL);
	TEST_ASSERT_GREATER_OR_EQUAL(0, peer_sock);

	SEND_CMD("Close");
	close(listen_sock);
	close(peer_sock);
}


TEST(tcp, select_rx_ready)
{
	struct timeval timeout;
	uint8_t rx_buf[128];
	uint8_t tx_buf[128];
	size_t len = sizeof tx_buf;

	for (size_t n = 0; n < len; n++) {
		tx_buf[n] = (uint8_t)(rand() % 256);
	}

	SEND_CMD_F("Receive %zu", len);
	TEST_ASSERT_EQUAL(len, send(sock, tx_buf, len, 0));

	SEND_CMD_F("Send received %zu", len);

	fd_set rfds;
	FD_ZERO(&rfds);
	FD_SET(sock, &rfds);

	timeout.tv_sec = 1;
	timeout.tv_usec = 0;

	TEST_ASSERT_EQUAL(1, select(sock + 1, &rfds, NULL, NULL, &timeout));
	TEST_ASSERT_TRUE(FD_ISSET(sock, &rfds));

	TEST_ASSERT_EQUAL(0, recv_all(sock, rx_buf, len, 0));
	TEST_ASSERT_EQUAL_MEMORY(tx_buf, rx_buf, len);
}


TEST(tcp, select_tx_ready)
{
	struct timeval timeout;
	uint8_t tx_buf[128];
	size_t len = sizeof tx_buf;

	TEST_ASSERT_EQUAL(0, fcntl(sock, F_SETFL, O_NONBLOCK));
	ssize_t ret = 1;
	do {
		TEST_ASSERT_GREATER_THAN(0, ret);
		ret = send(sock, tx_buf, len, 0);
	} while (ret > 0);

	fd_set wfds;
	FD_ZERO(&wfds);
	FD_SET(sock, &wfds);
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	TEST_ASSERT_EQUAL(0, select(sock + 1, NULL, &wfds, NULL, &timeout));
	TEST_ASSERT_FALSE(FD_ISSET(sock, &wfds));

	SEND_CMD_F("Send received %zu", len);

	FD_ZERO(&wfds);
	FD_SET(sock, &wfds);
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;

	TEST_ASSERT_EQUAL(1, select(sock + 1, NULL, &wfds, NULL, &timeout));
	TEST_ASSERT_TRUE(FD_ISSET(sock, &wfds));

	TEST_ASSERT_EQUAL(1, send(sock, tx_buf, 1, 0));
}


TEST_GROUP_RUNNER(tcp)
{
	RUN_TEST_CASE(tcp, send_recv);
	RUN_TEST_CASE(tcp, send_recv_over_mss);
	RUN_TEST_CASE(tcp, send_recv_vectored);
	RUN_TEST_CASE(tcp, send_recv_vectored_over_mss);
	RUN_TEST_CASE(tcp, multi_send_single_recv);
	RUN_TEST_CASE(tcp, multi_send_single_recv_over_mss);
	RUN_TEST_CASE(tcp, single_send_multi_recv);
	RUN_TEST_CASE(tcp, single_send_multi_recv_over_mss);
	RUN_TEST_CASE(tcp, recv_timeout);
	RUN_TEST_CASE(tcp, fill_window_tx);
	RUN_TEST_CASE(tcp, fill_window_rx);
	RUN_TEST_CASE(tcp, reuseaddr);
	RUN_TEST_CASE(tcp, peek);
	RUN_TEST_CASE(tcp, dont_wait);
	RUN_TEST_CASE(tcp, recv_closed_connection);
	RUN_TEST_CASE(tcp, send_closed_connection);
	RUN_TEST_CASE(tcp, read_shutdown);
	RUN_TEST_CASE(tcp, write_shutdown);
	RUN_TEST_CASE(tcp, connect_unsuccessful);
	RUN_TEST_CASE(tcp, multiple_connections);
	RUN_TEST_CASE(tcp, poll_timeout);
	RUN_TEST_CASE(tcp, poll_accept);
	RUN_TEST_CASE(tcp, poll_rx_ready);
	RUN_TEST_CASE(tcp, poll_tx_ready);
	RUN_TEST_CASE(tcp, select_timeout);
	RUN_TEST_CASE(tcp, select_accept);
	RUN_TEST_CASE(tcp, select_rx_ready);
	RUN_TEST_CASE(tcp, select_tx_ready);
}


void runner(void)
{
	RUN_TEST_GROUP(tcp);
}


int main(int argc, char **argv)
{
	char *iface = argv[1];

	if (!interface_is_running(iface)) {
		printf("Interface %s is down or no link detected. Check the cable connection\n", iface);
		return EXIT_FAILURE;
	}

	if ((cmd_sock = setup_cmd_conn()) < 0) {
		printf("Failed to create command connection\n");
		return EXIT_FAILURE;
	}

	if (init_connection_ips(cmd_sock, ip, peer_ip) < 0) {
		puts("Failed to initialize local and peer IP addresses");
		return EXIT_FAILURE;
	}

/*
 * Set to 1 to print command output to the console (stdout) instead of sending it through the socket.
 * Useful for debugging.
 */
#if 0
	/* make cmd_stream bidirectional (read/write) */
	cmd_stream = fdopen(STDOUT_FILENO, "r+");
#else
	cmd_stream = fdopen(cmd_sock, "r+");
#endif
	if (!cmd_stream) {
		perror("fdopen");
		return EXIT_FAILURE;
	}

	/* MSG_NOSIGNAL is not supported, therefore ignore SIGPIPE */
	signal(SIGPIPE, SIG_IGN);

	struct timeval tv;
	gettimeofday(&tv, NULL);
	srand(tv.tv_sec ^ tv.tv_usec);

	return (UnityMain(argc, (const char **)argv, runner) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
