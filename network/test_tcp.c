/*
 * Phoenix-RTOS
 *
 * tcp test
 *
 * Copyright 2025 Phoenix Systems
 * Author: Adam Debek
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <unity_fixture.h>

#include "common.h"

#define MSS_SIZE 1460

int sock;
char ip[INET_ADDRSTRLEN], peer_ip[INET_ADDRSTRLEN];
const uint16_t port = 50000;
uint8_t *rx_buf, *tx_buf;


TEST_GROUP(tcp);


TEST_SETUP(tcp)
{
	SEND_CMD("Connect");
	sock = open_connection(peer_ip, port);
	if (sock < 0)
		TEST_FAIL_MESSAGE("Failed to establish TCP connection");
}


TEST_TEAR_DOWN(tcp)
{
	SEND_CMD("Close");
	close(sock);
}


TEST(tcp, send_recv)
{
	TEST_ASSERT_NOT_NULL(rx_buf = (uint8_t *)malloc(MSS_SIZE));
	TEST_ASSERT_NOT_NULL(tx_buf = (uint8_t *)malloc(MSS_SIZE));

	for (int i = 0; i < 100; i++) {
		size_t len = 1 + (size_t)(rand() % MSS_SIZE);

		for (size_t n = 0; n < len; n++) {
			tx_buf[n] = (uint8_t)(rand() % 256);
		}

		SEND_CMD_F("Transmit %zu", len);
		TEST_ASSERT_EQUAL(len, send(sock, tx_buf, len, 0));

		SEND_CMD_F("Receive %zu", len);
		TEST_ASSERT_EQUAL(0, recv_all(sock, rx_buf, len));

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

	for (int i = 0; i < 100; i++) {
		for (size_t n = 0; n < buf_size; n++) {
			tx_buf[n] = (uint8_t)(rand() % 256);
		}

		SEND_CMD_F("Transmit %zu", buf_size);
		TEST_ASSERT_EQUAL(0, send_all(sock, tx_buf, buf_size));

		SEND_CMD_F("Receive %zu", buf_size);
		TEST_ASSERT_EQUAL(0, recv_all(sock, rx_buf, buf_size));

		TEST_ASSERT_EQUAL_MEMORY(tx_buf, rx_buf, buf_size);
	}

	free(rx_buf);
	free(tx_buf);
}


TEST(tcp, send_recv_vectored)
{
	uint8_t *rx_buf[4], *tx_buf[4];
	size_t iov_len = MSS_SIZE / 4;

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

	for (int i = 0; i < 100; i++) {
		for (int j = 0; j < 4; j++) {
			for (size_t n = 0; n < iov_len; n++) {
				tx_buf[j][n] = (uint8_t)(rand() % 256);
			}
		}

		memcpy(siov_work, siov, sizeof siov);
		memcpy(riov_work, riov, sizeof riov);

		SEND_CMD_F("Transmit %zu", (size_t)MSS_SIZE);
		TEST_ASSERT_EQUAL(0, sendmsg_all(sock, siov_work, 4));

		SEND_CMD_F("Receive %zu", (size_t)MSS_SIZE);
		TEST_ASSERT_EQUAL(0, recvmsg_all(sock, riov_work, 4));

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
		siov[i].iov_base = tx_buf[i];
		siov[i].iov_len = iov_len;

		riov[i].iov_base = rx_buf[i];
		riov[i].iov_len = iov_len;
	}

	for (int i = 0; i < 100; i++) {
		for (int j = 0; j < 4; j++) {
			for (size_t n = 0; n < iov_len; n++) {
				tx_buf[j][n] = (uint8_t)(rand() % 256);
			}
		}

		memcpy(siov_work, siov, sizeof siov);
		memcpy(riov_work, riov, sizeof riov);

		SEND_CMD_F("Transmit %zu", 4 * iov_len);
		TEST_ASSERT_EQUAL(0, sendmsg_all(sock, siov_work, 4));

		SEND_CMD_F("Receive %zu", 4 * iov_len);
		TEST_ASSERT_EQUAL(0, recvmsg_all(sock, riov_work, 4));

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

	for (int i = 0; i < 100; i++) {
		pos = len_total = 0;
		for (int j = 0; j < 4; j++) {
			len = 1 + (size_t)(rand() % (MSS_SIZE / 4));

			for (size_t n = 0; n < len; n++) {
				tx_buf[pos + n] = (uint8_t)(rand() % 256);
			}

			SEND_CMD_F("Transmit %zu", len);
			TEST_ASSERT_EQUAL(len, send(sock, tx_buf + pos, len, 0));

			pos = len_total += len;
		}

		SEND_CMD_F("Receive %zu", len_total);
		TEST_ASSERT_EQUAL(0, recv_all(sock, rx_buf, len_total));

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

	for (int i = 0; i < 100; i++) {
		pos = len_total = 0;
		for (int j = 0; j < 4; j++) {
			len = 1 + (size_t)(rand() % (buf_size / 4));

			for (size_t n = 0; n < len; n++) {
				tx_buf[pos + n] = (uint8_t)(rand() % 256);
			}

			SEND_CMD_F("Transmit %zu", len);
			TEST_ASSERT_EQUAL(0, send_all(sock, tx_buf + pos, len));

			pos = len_total += len;
		}

		SEND_CMD_F("Receive %zu", len_total);
		TEST_ASSERT_EQUAL(0, recv_all(sock, rx_buf, len_total));

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

	for (int i = 0; i < 100; i++) {
		for (size_t n = 0; n < MSS_SIZE; n++) {
			tx_buf[n] = (uint8_t)(rand() % 256);
		}

		SEND_CMD_F("Transmit %zu", (size_t)MSS_SIZE);
		TEST_ASSERT_EQUAL(MSS_SIZE, send(sock, tx_buf, MSS_SIZE, 0));

		pos = len_total = 0;
		for (int j = 0; j < 4; j++) {
			len = 1 + (size_t)(rand() % 128);

			SEND_CMD_F("Receive %zu", len);
			TEST_ASSERT_EQUAL(0, recv_all(sock, rx_buf + pos, len));

			pos = len_total += len;
		}

		remaining = MSS_SIZE - len_total;
		SEND_CMD_F("Receive %zu", remaining);
		TEST_ASSERT_EQUAL(0, recv_all(sock, rx_buf + pos, remaining));

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

	for (int i = 0; i < 100; i++) {
		for (size_t n = 0; n < buf_size; n++) {
			tx_buf[n] = (uint8_t)(rand() % 256);
		}

		SEND_CMD_F("Transmit %zu", buf_size);
		TEST_ASSERT_EQUAL(0, send_all(sock, tx_buf, buf_size));

		pos = len_total = 0;
		for (int j = 0; j < 4; j++) {
			len = 1 + (size_t)(rand() % (buf_size / 4));

			SEND_CMD_F("Receive %zu", len);
			TEST_ASSERT_EQUAL(0, recv_all(sock, rx_buf + pos, len));

			pos = len_total += len;
		}

		remaining = buf_size - len_total;
		SEND_CMD_F("Receive %zu", remaining);
		TEST_ASSERT_EQUAL(0, recv_all(sock, rx_buf + pos, remaining));

		TEST_ASSERT_EQUAL_MEMORY(tx_buf, rx_buf, buf_size);
	}

	free(rx_buf);
	free(tx_buf);
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

	if (init_connection_ips(ip, peer_ip) < 0) {
		puts("Failed to initialize local and peer IP addresses");
		return EXIT_FAILURE;
	}

	/* MSG_NOSIGNAL is not supported, therefore ignore SIGPIPE */
	signal(SIGPIPE, SIG_IGN);

	struct timeval tv;
	gettimeofday(&tv, NULL);
	srand(tv.tv_sec ^ tv.tv_usec);

	return (UnityMain(argc, (const char **)argv, runner) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
