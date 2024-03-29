/*
 * Phoenix-RTOS
 *
 * test-libc-socket
 *
 * inet socket tests
 *
 * Copyright 2021, 2024 Phoenix Systems
 * Author: Ziemowit Leszczynski, Adam Debek
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/uio.h>
#include <sys/wait.h>

#include "common.h"
#include "unity_fixture.h"

char data[DATA_SIZE];
char buf[DATA_SIZE];


TEST_GROUP(test_inet_socket);


TEST_SETUP(test_inet_socket)
{
}


TEST_TEAR_DOWN(test_inet_socket)
{
}


TEST(test_inet_socket, inet_zero_len_send)
{
	int fd[3];
	struct sockaddr_in addr = { 0 };
	struct msghdr msg;
	struct iovec iov;
	ssize_t n;

	if ((fd[0] = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		FAIL("socket");
	if ((fd[1] = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		FAIL("socket");

	addr.sin_family = AF_INET;
	addr.sin_port = 0;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	if (bind(fd[0], (struct sockaddr *)&addr, sizeof(addr)) < 0)
		FAIL("bind");

	addr.sin_family = AF_INET;
	addr.sin_port = htons(30000);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	if (bind(fd[1], (struct sockaddr *)&addr, sizeof(addr)) < 0)
		FAIL("bind");

	if (connect(fd[0], (struct sockaddr *)&addr, sizeof(addr)) < 0)
		FAIL("connect");

	/* write */
	{
		n = write(fd[0], NULL, 0);
		TEST_ASSERT(n == 0);

		n = write(fd[0], data, 0);
		TEST_ASSERT(n == 0);
	}

	/* writev */
	{
#ifdef __phoenix__
		n = writev(fd[0], NULL, 0);
		TEST_ASSERT(n == -1);
		TEST_ASSERT(errno == EINVAL);

		n = writev(fd[0], &iov, 0);
		TEST_ASSERT(n == -1);
		TEST_ASSERT(errno == EINVAL);
#else
		n = writev(fd[0], NULL, 0);
		TEST_ASSERT(n == 0);
		TEST_ASSERT(errno == 0);

		n = writev(fd[0], &iov, 0);
		TEST_ASSERT(n == 0);
		TEST_ASSERT(errno == 0);
#endif
		iov.iov_base = NULL;
		iov.iov_len = 0;
		n = writev(fd[0], &iov, 1);
		TEST_ASSERT(n == 0);

		iov.iov_base = data;
		iov.iov_len = 0;
		n = writev(fd[0], &iov, 1);
		TEST_ASSERT(n == 0);
	}

	/* send */
	{
		n = send(fd[0], NULL, 0, 0);
		TEST_ASSERT(n == 0);

		n = send(fd[0], data, 0, 0);
		TEST_ASSERT(n == 0);
	}

	/* sendto */
	{
		n = sendto(fd[0], NULL, 0, 0, NULL, 0);
		TEST_ASSERT(n == 0);

		n = sendto(fd[0], data, 0, 0, NULL, 0);
		TEST_ASSERT(n == 0);
	}

	/* sendmsg */
	{
		memset(&msg, 0, sizeof(msg));
		msg.msg_iov = NULL;
		msg.msg_iovlen = 0;
		n = sendmsg(fd[0], &msg, 0);
		TEST_ASSERT(n == 0);

		memset(&msg, 0, sizeof(msg));
		iov.iov_base = NULL;
		iov.iov_len = 0;
		msg.msg_iov = &iov;
		msg.msg_iovlen = 1;
		n = sendmsg(fd[0], &msg, 0);
		TEST_ASSERT(n == 0);
	}

	close(fd[0]);
	close(fd[1]);
}


TEST_GROUP_RUNNER(test_inet_socket)
{
	RUN_TEST_CASE(test_inet_socket, inet_zero_len_send);
}

void runner(void)
{
	RUN_TEST_GROUP(test_inet_socket);
}


int main(int argc, char *argv[])
{
	return (UnityMain(argc, (const char **)argv, runner) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
