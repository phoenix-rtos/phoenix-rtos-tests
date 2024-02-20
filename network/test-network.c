/*
 * Phoenix-RTOS
 *
 * test-network-client
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
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unity_fixture.h>

int8_t send_data[128];
int8_t recv_data[128];
char *peer_ip;


TEST_GROUP(test_network);


TEST_SETUP(test_network)
{
}


TEST_TEAR_DOWN(test_network)
{
}


TEST(test_network, basic)
{
	int fd;
	size_t n, r, i;
	struct sockaddr_in si;

	memset(&si, 0, sizeof si);
	si.sin_family = AF_INET;
	si.sin_addr.s_addr = inet_addr(peer_ip);
	si.sin_port = htons(1025);

	fd = socket(AF_INET, SOCK_STREAM, 0);
	TEST_ASSERT(fd > 0);

	if (connect(fd, (struct sockaddr *)&si, sizeof(si)) < 0) {
		perror("connect");
		FAIL("connect");
	}

	r = read(fd, recv_data, sizeof(recv_data));
	if (r < 0 && errno == EPIPE) {
		FAIL("peer closed connection");
	}
	TEST_ASSERT(r == sizeof(recv_data));

	srandom(time(NULL));

	for (i = 0; i < sizeof(send_data); i++) {
		send_data[i] = recv_data[i] + 1;
	}

	n = send(fd, send_data, sizeof(send_data), MSG_NOSIGNAL);
	if (n < 0 && errno == EPIPE) {
		FAIL("peer closed connection");
	}
	TEST_ASSERT(n == sizeof(send_data));

	close(fd);
}


TEST_GROUP_RUNNER(test_network)
{
	RUN_TEST_CASE(test_network, basic);
}

void runner(void)
{
	RUN_TEST_GROUP(test_network);
}


int main(int argc, char *argv[])
{
	if (argc == 2 && argv[1] != NULL) {
		peer_ip = argv[1];
	}
	else {
		fprintf(stderr, "Usage: %s <ip>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	UnityMain(argc, (const char **)argv, runner);

	return 0;
}
