/*
 * Phoenix-RTOS
 *
 * Tests for special ioctls:
 *   - SIOCIFCONF
 *   - SIOCETHTOOL
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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <sys/socket.h>

#include <phoenix/ethtool.h>

#include "unity_fixture.h"

#define ERR_MSG_LEN 64


static int fd = -1;
static struct ifconf ifc;
static struct ifconf current_ifc;
static struct ifreq current_ifr;


static inline int get_ifconf(int fd, struct ifconf *ifc)
{
	int err = ioctl(fd, SIOCGIFCONF, ifc);
	if (err < 0) {
		return -1;
	}

	ifc->ifc_req = malloc(ifc->ifc_len);
	if (ifc->ifc_req == NULL) {
		return -1;
	}

	err = ioctl(fd, SIOCGIFCONF, ifc);
	if (err < 0) {
		return -1;
	}

	return 0;
}


/* returns: 0 or errno on success, -1 on fail */
static inline int ethtool_ioctl(struct ifreq *ifr, void *ethtool_struct, uint32_t cmd, char *err_msg_buf)
{
	*((uint32_t *)ethtool_struct) = cmd;
	ifr->ifr_data = (void *)ethtool_struct;
	if (ioctl(fd, SIOCETHTOOL, ifr) < 0) {
		if (errno == ENXIO) {
			snprintf(err_msg_buf, ERR_MSG_LEN, "Interface '%.*s', not found", IFNAMSIZ, ifr->ifr_name);
			return -1;
		}
		if (errno == EOPNOTSUPP) {
			return EOPNOTSUPP;
		}
		snprintf(err_msg_buf, ERR_MSG_LEN, "Interface '%.*s': %s", IFNAMSIZ, ifr->ifr_name, strerror(errno));
		return -1;
	}
	return 0;
}


TEST_GROUP(test_ioctl_special);


TEST_SETUP(test_ioctl_special)
{
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
	TEST_ASSERT_EQUAL_MESSAGE(0, get_ifconf(fd, &current_ifc), strerror(errno));
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


TEST(test_ioctl_special, ethtool_gset)
{
	struct ethtool_cmd cmd = { 0 };
	char err_msg_buf[ERR_MSG_LEN] = { 0 };

	int err = ethtool_ioctl(&current_ifr, &cmd, ETHTOOL_GSET, err_msg_buf);
	if (err == EOPNOTSUPP) {
		TEST_IGNORE_MESSAGE("Operation not supported for this interface");
	}
	else if (err < 0) {
		TEST_FAIL_MESSAGE(err_msg_buf);
	}
}


TEST(test_ioctl_special, ethtool_sset)
{
	int err = 0;
	int last_port = -1;
	struct ethtool_cmd cmd = { 0 };
	char err_msg_buf[ERR_MSG_LEN] = { 0 };
	do {
		err = ethtool_ioctl(&current_ifr, &cmd, ETHTOOL_GSET, err_msg_buf);
		if (err == EOPNOTSUPP) {
			TEST_IGNORE_MESSAGE("Operation not supported for this interface");
		}
		else if (err < 0) {
			break;
		}

		last_port = cmd.port;
		cmd.port = 123;
		err = ethtool_ioctl(&current_ifr, &cmd, ETHTOOL_SSET, err_msg_buf);
		if (err == EOPNOTSUPP) {
			TEST_IGNORE_MESSAGE("Operation not supported for this interface");
		}
		else if (err < 0) {
			break;
		}
		TEST_ASSERT_EQUAL(123, cmd.port);

	} while (0);

	if (last_port != -1) {
		cmd.port = last_port;
		(void)ethtool_ioctl(&current_ifr, &cmd, ETHTOOL_SSET, err_msg_buf);
	}
	if (err < 0) {
		TEST_FAIL_MESSAGE(err_msg_buf);
	}
}


TEST(test_ioctl_special, ethtool_test)
{
	struct ethtool_test cmd = { 0 };
	char err_msg_buf[ERR_MSG_LEN] = { 0 };
	cmd.flags = ETH_TEST_FL_OFFLINE;

	int err = ethtool_ioctl(&current_ifr, &cmd, ETHTOOL_TEST, err_msg_buf);
	if (err == EOPNOTSUPP) {
		TEST_IGNORE_MESSAGE("Operation not supported for this interface");
	}
	else if (err < 0) {
		TEST_FAIL_MESSAGE(err_msg_buf);
	}
	TEST_ASSERT_EQUAL_MESSAGE(0, cmd.flags & ETH_TEST_FL_FAILED, "driver PHYSELFTEST failed");
}


TEST(test_ioctl_special, ethtool_gloopback)
{
	struct ethtool_value cmd = { 0 };
	char err_msg_buf[ERR_MSG_LEN] = { 0 };

	int err = ethtool_ioctl(&current_ifr, &cmd, ETHTOOL_GLOOPBACK, err_msg_buf);
	if (err == EOPNOTSUPP) {
		TEST_IGNORE_MESSAGE("Operation not supported for this interface");
	}
	else if (err < 0) {
		TEST_FAIL_MESSAGE(err_msg_buf);
	}
}


TEST(test_ioctl_special, ethtool_sloopback)
{
	int err = 0;
	int last_loopback = -1;
	struct ethtool_value cmd = { 0 };
	char err_msg_buf[ERR_MSG_LEN] = { 0 };

	do {
		err = ethtool_ioctl(&current_ifr, &cmd, ETHTOOL_GLOOPBACK, err_msg_buf);
		if (err == EOPNOTSUPP) {
			TEST_IGNORE_MESSAGE("Operation not supported for this interface");
		}
		else if (err < 0) {
			break;
		}
		last_loopback = cmd.data;
		uint32_t expected = (cmd.data != 0) ? ETH_PHY_LOOPBACK_DISABLED : ETH_PHY_LOOPBACK_ENABLED;

		cmd.data = expected;
		err = ethtool_ioctl(&current_ifr, &cmd, ETHTOOL_SLOOPBACK, err_msg_buf);
		if (err == EOPNOTSUPP) {
			TEST_IGNORE_MESSAGE("Operation not supported for this interface");
		}
		if (err < 0) {
			break;
		}

		if (cmd.data == ETH_PHY_LOOPBACK_SET_FAILED) {
			snprintf(err_msg_buf, ERR_MSG_LEN, "Interface %.*s: couldn't set loopback", IFNAMSIZ, current_ifr.ifr_name);
			err = -1;
			break;
		}

		cmd.data = 0;
		err = ethtool_ioctl(&current_ifr, &cmd, ETHTOOL_GLOOPBACK, err_msg_buf);
		if (err == EOPNOTSUPP) {
			TEST_IGNORE_MESSAGE("Operation not supported for this interface");
		}
		else if (err < 0) {
			break;
		}
		else if (cmd.data != expected) {
			snprintf(err_msg_buf, ERR_MSG_LEN, "Interface %.*s: loopback set incorrectly", IFNAMSIZ, current_ifr.ifr_name);
			err = -1;
			break;
		}
	} while (0);

	if (last_loopback != -1) {
		cmd.data = last_loopback;
		(void)ethtool_ioctl(&current_ifr, &cmd, ETHTOOL_SLOOPBACK, err_msg_buf);
	}

	if (err < 0) {
		TEST_FAIL_MESSAGE(err_msg_buf);
	}
}


TEST_GROUP_RUNNER(test_ioctl_special)
{
	RUN_TEST_CASE(test_ioctl_special, ifconf);
	RUN_TEST_CASE(test_ioctl_special, ifconf_not_enough_space);

	for (int i = 0; i < (ifc.ifc_len / sizeof(struct ifreq)); i++) {
		current_ifr = ifc.ifc_req[i];
		fprintf(stderr, "IF: %.*s\n", IFNAMSIZ, current_ifr.ifr_name);
		RUN_TEST_CASE(test_ioctl_special, ethtool_gset);
		RUN_TEST_CASE(test_ioctl_special, ethtool_sset);
		RUN_TEST_CASE(test_ioctl_special, ethtool_test);
		RUN_TEST_CASE(test_ioctl_special, ethtool_gloopback);
		RUN_TEST_CASE(test_ioctl_special, ethtool_sloopback);
	}
}


void runner(void)
{
	RUN_TEST_GROUP(test_ioctl_special);
}


int main(int argc, char *argv[])
{
	int err;
	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		perror("Couldn't open socket");
		return EXIT_FAILURE;
	}

	if (get_ifconf(fd, &ifc) < 0) {
		perror("Couldn't get ifconf");
		return EXIT_FAILURE;
	}

	err = UnityMain(argc, (const char **)argv, runner);
	if (ifc.ifc_req != NULL) {
		free(ifc.ifc_req);
	}
	if (fd != -1) {
		close(fd);
	}

	return (err == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
