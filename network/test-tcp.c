/*
 * Phoenix-RTOS
 *
 * test-tcp (target side part)
 *
 * network tests
 *
 * Copyright 2024 Phoenix Systems
 * Author: Adam Debek
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <unity_fixture.h>
#include "common.h"


uint8_t send_data[128];
uint8_t recv_data[128];
uint8_t rand_data[128];
char host_response[128];
int host_response_flag;
int target_failed_flag;
char *host_ip;
char *target_ip;
uint16_t host_port, target_port;
int sockfd, syncfd;


TEST_GROUP(test_tcp);


TEST_SETUP(test_tcp)
{
	target_failed_flag = 0;
	host_response_flag = 0;
	memset(host_response, 0, sizeof host_response);
	sockfd = create_con(host_ip, host_port);
	if (sockfd < 0) {
		TEST_FAIL_MESSAGE("Testcase connection creation failed");
	}
}


TEST_TEAR_DOWN(test_tcp)
{
	/* If response hadn't been got yet, test failed due to target fail */
	if (host_response_flag == 0) {
		/* Close testcase socket first to make sure host will not get blocked */
		close(sockfd);
		target_failed_flag = 1;
		get_host_response(syncfd, host_response);
	}
}


TEST(test_tcp, basic)
{
	uint8_t recv_buf[128];
	uint8_t expected[128];
	unsigned int i;

	recvall(sockfd, recv_data, sizeof recv_data, MSG_WAITALL);
	memcpy(recv_buf, recv_data, sizeof recv_buf);

	srand(time(NULL));
	for (i = 0; i < sizeof(send_data); i++) {
		rand_data[i] = rand() % 128;
		send_data[i] = recv_data[i] + rand_data[i];
	}

	sendall(sockfd, send_data, sizeof send_data, MSG_NOSIGNAL);

	recvall(sockfd, recv_data, sizeof recv_data, MSG_WAITALL);

	for (i = 0; i < sizeof(recv_data); i++) {
		expected[i] = send_data[i] - recv_data[i];
	}

	TEST_ASSERT_EQUAL_MEMORY(expected, recv_buf, sizeof rand_data);

	close(sockfd);
	get_host_response(syncfd, host_response);
}


TEST(test_tcp, big_data)
{
	size_t data_size = 24 * 1024;
	uint8_t *send_data_big = (uint8_t *)malloc(data_size);
	uint8_t *recv_data_big = (uint8_t *)malloc(data_size);
	uint8_t *rand_data_big = (uint8_t *)malloc(data_size);
	uint8_t *recv_buf_big = (uint8_t *)malloc(data_size);
	uint8_t *expected_big = (uint8_t *)malloc(data_size);

	if (send_data_big == NULL || recv_data_big == NULL || rand_data_big == NULL ||
		recv_buf_big == NULL || expected_big == NULL) {
		TEST_FAIL_MESSAGE("Not enough memory");
	}

	recvall(sockfd, recv_data_big, data_size, MSG_WAITALL);
	memcpy(recv_buf_big, recv_data_big, data_size);

	srand(time(NULL));
	for (size_t i = 0; i < data_size; i++) {
		rand_data_big[i] = rand() % 128;
		send_data_big[i] = recv_data_big[i] + rand_data_big[i];
	}

	sendall(sockfd, send_data_big, data_size, MSG_NOSIGNAL);

	recvall(sockfd, recv_data_big, data_size, MSG_WAITALL);

	for (size_t i = 0; i < data_size; i++) {
		expected_big[i] = send_data_big[i] - recv_data_big[i];
	}

	TEST_ASSERT_EQUAL_MEMORY(expected_big, recv_buf_big, data_size);

	free(send_data_big);
	free(recv_data_big);
	free(rand_data_big);
	free(recv_buf_big);
	free(expected_big);
	close(sockfd);
	get_host_response(syncfd, host_response);
}


TEST(test_tcp, send_after_close)
{
	ssize_t n;
	int ret;

	ret = close(sockfd);
	TEST_ASSERT(ret == 0);

	n = send(sockfd, send_data, sizeof send_data, MSG_NOSIGNAL);
	TEST_ASSERT(n < 0);

	get_host_response(syncfd, host_response);
}


TEST(test_tcp, recv_remaining_data)
{
	/* Wait until host close connection and read remaining data */
	ssize_t r;
	size_t data_size = 24 * 1024;
	uint8_t *recv_data_big = (uint8_t *)malloc(data_size);

	if (recv_data_big == NULL) {
		TEST_FAIL_MESSAGE("Not enough memory");
	}

	recvall(sockfd, recv_data_big, data_size, MSG_WAITALL);

	r = recv(sockfd, recv_data_big, data_size, MSG_WAITALL);
	TEST_ASSERT(r == 0);

	free(recv_data_big);
	close(sockfd);
	get_host_response(syncfd, host_response);
}


TEST(test_tcp, simultaneous_clients)
{
#define num 20
	pid_t proc[num];
	unsigned int i, j;
	int ret;

	for (i = 0; i < num; i++) {
		if ((proc[i] = fork()) < 0) {
			for (j = 0; j < i; j++) {
				wait(NULL);
			}
			TEST_FAIL_MESSAGE("fork failed");
		}
		else if (proc[i] == 0) {
			uint8_t recv_buf[128];
			uint8_t expected[128];
			unsigned int i, j;
			int confd;
			int iters = 30;

			confd = create_con(host_ip, host_port);
			if (confd < 0) {
				exit(1);
			}

			srand(time(NULL));
			for (j = 0; j < iters; j++) {
				recvall_child(confd, recv_data, sizeof recv_data, MSG_WAITALL);
				memcpy(recv_buf, recv_data, sizeof recv_buf);

				srand(time(NULL));
				for (i = 0; i < sizeof(send_data); i++) {
					rand_data[i] = rand() % 128;
					send_data[i] = recv_data[i] + rand_data[i];
				}

				sendall_child(confd, send_data, sizeof send_data, MSG_NOSIGNAL);

				recvall_child(confd, recv_data, sizeof recv_data, MSG_WAITALL);

				for (i = 0; i < sizeof(recv_data); i++) {
					expected[i] = send_data[i] - recv_data[i];
				}

				TEST_ASSERT_EQUAL_MEMORY(expected, recv_buf, sizeof rand_data);

				usleep((rand() % 200) * 1000);
			}
			exit(EXIT_SUCCESS);
		}
	}

	for (i = 0; i < num; i++) {
		wait(&ret);
		if (WEXITSTATUS(ret) != EXIT_SUCCESS) {
			print_child_error_msg(WEXITSTATUS(ret));
		}
	}

	close(sockfd);
	get_host_response(syncfd, host_response);
}


TEST(test_tcp, accept_connections)
{
	/* Issue with poll */
	int listenfd, ret;
	int confd[200];
	struct pollfd fds[1];
	struct sockaddr_in addr;
	socklen_t len = sizeof addr;
	int reuse = 1;
	unsigned int i, j;
	char errmsg[64];

	listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	TEST_ASSERT(listenfd > 0);

	ret = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof reuse);
	TEST_ASSERT(ret == 0);

	memset(&addr, 0, sizeof addr);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(target_ip);
	addr.sin_port = htons(target_port);

	ret = bind(listenfd, (struct sockaddr *)&addr, sizeof addr);
	TEST_ASSERT(ret == 0);

	ret = listen(listenfd, 0);
	TEST_ASSERT(ret == 0);

	fds[0].fd = listenfd;
	fds[0].events = POLLIN;
	fds[0].revents = 0;

	for (i = 0; i < 200; i++) {
		ret = poll(fds, 1, 5000);

		if (ret != 1 && !(fds[0].revents & POLLIN)) {
			for (j = 0; j < i; j++) {
				close(confd[j]);
			}
			sprintf(errmsg, "polling %uth connection failed", i + 1);
			TEST_FAIL_MESSAGE(errmsg);
		}

		confd[i] = accept(listenfd, (struct sockaddr *)&addr, &len);
		if (confd[i] < 0) {
			for (j = 0; j < i; j++) {
				close(confd[j]);
			}
			sprintf(errmsg, "accepting %uth connection failed", i + 1);
			TEST_FAIL_MESSAGE(errmsg);
		}
	}

	close(listenfd);
	for (i = 0; i < 200; i++) {
		close(confd[i]);
	}

	close(sockfd);
	get_host_response(syncfd, host_response);
}


/* Function used in testcases which only check if wrong packets are dropped */
void assert_packet_dropped(void)
{
	int ret;
	struct pollfd fds[1];

	fds[0].fd = sockfd;
	fds[0].events = POLLIN;
	fds[0].revents = 0;
	ret = poll(fds, 1, 3000);

	/* Assert poll timed out */
	TEST_ASSERT(ret == 0);
	TEST_ASSERT(fds[0].revents == 0);

	/* Send anything to check if session is not desynchronized */
	sendall(sockfd, send_data, sizeof send_data, MSG_NOSIGNAL);
}


TEST(test_tcp, receive_rst)
{
	/* Receive packet with RST flag on, after that packets should be dropped */
	int ret;
	struct pollfd fds[1];
	ssize_t r;
	uint8_t dummy_buf[16];

	fds[0].fd = sockfd;
	fds[0].events = POLLIN | POLLHUP;
	fds[0].revents = 0;
	ret = poll(fds, 1, 3000);

	/* Assert poll timed out */
	TEST_ASSERT(ret == 1);
	/* Only POLLIN is received, I think POLLHUP should also */
	TEST_ASSERT(fds[0].revents == POLLIN);

	errno = 0;
	r = recv(sockfd, dummy_buf, sizeof dummy_buf, 0);
	TEST_ASSERT(r == -1);
	TEST_ASSERT(errno == ECONNRESET);

	close(sockfd);
	get_host_response(syncfd, host_response);
}


TEST(test_tcp, wrong_src_port)
{
	/* Host sends segment with source port, packet should be dropped */
	assert_packet_dropped();
	close(sockfd);
	get_host_response(syncfd, host_response);
}


TEST(test_tcp, wrong_dest_port)
{
	/* Host sends segment with wrong destination port, packet should be dropped */
	assert_packet_dropped();
	close(sockfd);
	get_host_response(syncfd, host_response);
}


TEST(test_tcp, wrong_seq)
{
	/* Host sends segment with wrong sequential number, packet should be dropped */
	assert_packet_dropped();
	close(sockfd);
	get_host_response(syncfd, host_response);
}


TEST(test_tcp, wrong_ack)
{
	/* Packets with ack out of window are accepted ISSUE?? */
	assert_packet_dropped();
	close(sockfd);
	get_host_response(syncfd, host_response);
}


TEST(test_tcp, wrong_chk_sum)
{
	/* Host sends segment with wrong checksum, packet should be dropped */
	assert_packet_dropped();
	close(sockfd);
	get_host_response(syncfd, host_response);
}


TEST_GROUP_RUNNER(test_tcp)
{
	/* Wait until interface will be in running state */
	if (wait_if_running() < 0) {
		fprintf(stderr, "Interface en1 is not running");
		exit(1);
	}
	/* Create sync socket */
	syncfd = create_con(host_ip, host_port);
	if (syncfd < 0) {
		fprintf(stderr, "Setting sync connection failed\n");
		exit(1);
	}

	RUN_TEST_CASE(test_tcp, basic);
	RUN_TEST_CASE(test_tcp, big_data);
	RUN_TEST_CASE(test_tcp, accept_connections);
	RUN_TEST_CASE(test_tcp, send_after_close);
	RUN_TEST_CASE(test_tcp, recv_remaining_data);
	RUN_TEST_CASE(test_tcp, simultaneous_clients);
	RUN_TEST_CASE(test_tcp, receive_rst);
	RUN_TEST_CASE(test_tcp, wrong_src_port);
	RUN_TEST_CASE(test_tcp, wrong_dest_port);
	RUN_TEST_CASE(test_tcp, wrong_seq);
	RUN_TEST_CASE(test_tcp, wrong_chk_sum);
	// RUN_TEST_CASE(test_tcp, wrong_ack);

	/* Close sync socket */
	close(syncfd);
}


void runner(void)
{
	RUN_TEST_GROUP(test_tcp);
}


int main(int argc, char **argv)
{
	if (argc == 3 && argv[1] != NULL && argv[2] != NULL) {
		host_ip = strtok(argv[1], ":");
		host_port = (uint16_t)atoi(strtok(NULL, ""));
		target_ip = strtok(argv[2], ":");
		target_port = (uint16_t)atoi(strtok(NULL, ""));
	}
	else {
		fprintf(stderr, "Usage: %s <host_ip:host_port> <target_ip:target_port>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	return (UnityMain(argc, (const char **)argv, runner) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
