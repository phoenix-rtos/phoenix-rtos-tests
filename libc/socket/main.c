/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <sys/socket.h>
 * TESTED:
 *    - getsockopt()
 *    - listen()
 *    - recv()
 *    - recvfrom()
 *    - recvmsg()
 *    - send()
 *    - sendmsg()
 *    - sendto()
 *    - setsockopt()
 *    - shutdown()
 *    - socketpair()
 *
 * Copyright 2026 Phoenix Systems
 * Author: Lukasz Kruszynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "unity_fixture.h"

void runner(void)
{
	RUN_TEST_GROUP(socket_api_socketpair);
	RUN_TEST_GROUP(socket_api_shutdown);
	RUN_TEST_GROUP(socket_api_setsockopt);
	RUN_TEST_GROUP(socket_api_getsockopt);
	RUN_TEST_GROUP(socket_api_listen);
	RUN_TEST_GROUP(socket_api_send);
	RUN_TEST_GROUP(socket_api_sendto);
	RUN_TEST_GROUP(socket_api_sendmsg);
	RUN_TEST_GROUP(socket_api_recv);
	RUN_TEST_GROUP(socket_api_recvfrom);
	RUN_TEST_GROUP(socket_api_recvmsg);
}

int main(int argc, char *argv[])
{
	const char *var = "POSIXLY_CORRECT";

	if (setenv(var, "y", 1) != 0) {
		fprintf(stderr, "Setting %s environment variable failed: %s\n", var, strerror(errno));
		return 1;
	}

	return (UnityMain(argc, (const char **)argv, runner) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
