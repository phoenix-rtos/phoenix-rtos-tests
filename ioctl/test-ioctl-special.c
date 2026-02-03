/*
 * Phoenix-RTOS
 *
 * Tests for special ioctls:
 *   - SIOCIFCONF
 *
 * (special - passed structure has a pointer
 * to arbitrary memory -> needs flattening
 * in userspace)
 *
 * Copyright 2025 Phoenix Systems
 * Author: Julian Uziembło
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>

#include <sys/socket.h>
#include <net/if.h>
#include <arpa/inet.h>

#include "unity_fixture.h"

#define GET_IFADDRS(fd, ifc_ptr) \
	do { \
		TEST_ASSERT_EQUAL_MESSAGE(0, ioctl(fd, SIOCGIFCONF, (ifc_ptr)), strerror(errno)); \
		(ifc_ptr)->ifc_req = malloc((ifc_ptr)->ifc_len); \
		TEST_ASSERT_NOT_NULL_MESSAGE((ifc_ptr)->ifc_req, strerror(errno)); \
		TEST_ASSERT_EQUAL_MESSAGE(0, ioctl(fd, SIOCGIFCONF, (ifc_ptr)), strerror(errno)); \
	} while (0)


static int fd = -1;
static struct ifconf current_ifc;


TEST_GROUP(test_ioctl_special);


TEST_SETUP(test_ioctl_special)
{
	fd = socket(AF_INET, SOCK_STREAM, 0);
	TEST_ASSERT_GREATER_OR_EQUAL_MESSAGE(0, fd, strerror(errno));
}


TEST_TEAR_DOWN(test_ioctl_special)
{
	if (current_ifc.ifc_req != NULL) {
		free(current_ifc.ifc_req);
		memset(&current_ifc, 0, sizeof(current_ifc));
	}
}


TEST(test_ioctl_special, ifconf)
{
	GET_IFADDRS(fd, &current_ifc);
}


TEST(test_ioctl_special, ifconf_not_enough_space)
{
	struct ifreq ifr = { 0 };
	struct ifconf ifc = {
		.ifc_req = &ifr,
		.ifc_len = sizeof(ifr),
	};

	int res = ioctl(fd, SIOCGIFCONF, &ifc);
	TEST_ASSERT_EQUAL_MESSAGE(0, res, strerror(errno));
	TEST_ASSERT_EQUAL(sizeof(ifr), ifc.ifc_len);

	/* ifr_name should be 3 characters in lwip.
	   if net stack is ever changed - this should change too */
	TEST_ASSERT_EQUAL(3, strnlen(ifc.ifc_req->ifr_name, IFNAMSIZ));
}


TEST_GROUP_RUNNER(test_ioctl_special)
{
	RUN_TEST_CASE(test_ioctl_special, ifconf);
	RUN_TEST_CASE(test_ioctl_special, ifconf_not_enough_space);
}


void runner(void)
{
	RUN_TEST_GROUP(test_ioctl_special);
}


int main(int argc, char *argv[])
{
	return (UnityMain(argc, (const char **)argv, runner) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
