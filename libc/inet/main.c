/*
 * Phoenix-RTOS
 *
 * POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - <arpa/inet.h>
 *    - <net/if.h>
 *    - <netdb.h>
 *
 * Main entry point.
 *
 * Copyright 2026 Phoenix Systems
 * Author: Lukasz Kruszynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>

#include "unity_fixture.h"

/* no need for forward declarations, RUN_TEST_GROUP does it by itself */
void runner(void)
{
	RUN_TEST_GROUP(inet_addr);
	RUN_TEST_GROUP(inet_ntoa);
	RUN_TEST_GROUP(inet_pton);
	RUN_TEST_GROUP(inet_ntop);
	RUN_TEST_GROUP(inet_byteorder);
	RUN_TEST_GROUP(inet_if);
	RUN_TEST_GROUP(inet_gai);
	RUN_TEST_GROUP(inet_proto);
	RUN_TEST_GROUP(inet_getaddrinfo);
	RUN_TEST_GROUP(inet_getnameinfo);
}


int main(int argc, char *argv[])
{
	return (UnityMain(argc, (const char **)argv, runner) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
