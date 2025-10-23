/*
 * Phoenix-RTOS
 *
 * network tests common routines
 *
 * Copyright 2024 Phoenix Systems
 * Author: Adam Debek
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <unity_fixture.h>
#include "common.h"


int create_con(const char *daddr, uint16_t dport)
{
	int try = 10;
	int sockfd;
	struct sockaddr_in dest_addr;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		return -1;
	}

	memset(&dest_addr, 0, sizeof dest_addr);
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_addr.s_addr = inet_addr(daddr);
	dest_addr.sin_port = htons(dport);

	while (connect(sockfd, (struct sockaddr *)&dest_addr, sizeof dest_addr) < 0) {
		if (try-- > 0) {
			close(sockfd);
			if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
				perror("socket");
				return -1;
			}
			sleep(1);
		}
		else {
			close(sockfd);
			return -1;
		}
	}

	return sockfd;
}


int wait_if_running(void)
{
	struct ifreq ioctlInterface;
	short interfaceFlags;
	char *ifname = "en1";
	int try = 300;
	int ret;

	strcpy(ioctlInterface.ifr_name, ifname);

	int sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd < 0) {
		perror("socket");
		return -1;
	}

	while (try-- > 0) {
		ret = ioctl(sd, SIOCGIFFLAGS, &ioctlInterface);
		if (ret < 0) {
			/* Unable to obtain flags */
			perror("ioctl(SIOCGIFFLAGS)");
			return ret;
		}

		interfaceFlags = ioctlInterface.ifr_flags;
		if (interfaceFlags & IFF_RUNNING) {
			return 0;
		}

		usleep(10000);
	}

	/* Interface not running */
	return -1;
}
