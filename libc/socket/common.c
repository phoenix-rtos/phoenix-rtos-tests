/*
 * Phoenix-RTOS
 *
 * test-libc-socket
 *
 * common part of socket tests
 *
 * Copyright 2021, 2024 Phoenix Systems
 * Author: Ziemowit Leszczynski, Adam Debek
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <sys/socket.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

#include "common.h"


ssize_t msg_send(int sock, void *buf, size_t len, int *fd, size_t fdcnt)
{
	struct msghdr msg;
	struct iovec iov;
	struct cmsghdr *cmsg;
	union {
		char buf[CMSG_SPACE(sizeof(int)) * MAX_FD_CNT];
		struct cmsghdr align;
	} u;

	memset(&msg, 0, sizeof(msg));
	iov.iov_base = buf;
	iov.iov_len = len;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	if (fd) {
		msg.msg_control = u.buf;
		msg.msg_controllen = CMSG_LEN(sizeof(int) * fdcnt);
		cmsg = CMSG_FIRSTHDR(&msg);
		cmsg->cmsg_level = SOL_SOCKET;
		cmsg->cmsg_type = SCM_RIGHTS;
		cmsg->cmsg_len = CMSG_LEN(sizeof(int) * fdcnt);
		memcpy(CMSG_DATA(cmsg), fd, sizeof(int) * fdcnt);
	}
	else {
		msg.msg_control = NULL;
		msg.msg_controllen = 0;
	}

	return sendmsg(sock, &msg, 0);
}


ssize_t msg_recv(int sock, void *buf, size_t len, int *fd, size_t *fdcnt)
{
	struct msghdr msg;
	struct iovec iov;
	struct cmsghdr *cmsg;
	union {
		char buf[CMSG_SPACE(sizeof(int)) * MAX_FD_CNT];
		struct cmsghdr align;
	} u;
	ssize_t n;

	memset(&msg, 0, sizeof(msg));
	iov.iov_base = buf;
	iov.iov_len = len;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = &u.buf;
	msg.msg_controllen = CMSG_LEN(sizeof(int) * MAX_FD_CNT);

	n = recvmsg(sock, &msg, 0);

	if (fdcnt)
		*fdcnt = 0;

	for (cmsg = CMSG_FIRSTHDR(&msg); cmsg; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
		if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS) {
			char *cmsg_data = (char *)CMSG_DATA(cmsg);
			char *cmsg_end = (char *)cmsg + cmsg->cmsg_len;
			size_t cnt = (cmsg_end - cmsg_data) / sizeof(int);

			if (fd) {
				memcpy(fd, cmsg_data, sizeof(int) * cnt);
				fd += cnt;
			}
			if (fdcnt)
				*fdcnt += cnt;
		}
		else
			return -1;
	}

	return n;
}


int set_nonblock(int fd, int enable)
{
	int flags;

	if ((flags = fcntl(fd, F_GETFL, 0)) < 0)
		return -1;

	if (enable)
		flags |= O_NONBLOCK;
	else
		flags &= ~O_NONBLOCK;

	if (fcntl(fd, F_SETFL, flags) < 0)
		return -1;

	return 0;
}


int open_files(int *fd, size_t cnt)
{
	size_t i;
	char buf[64];

	for (i = 0; i < cnt; ++i) {
		snprintf(buf, sizeof(buf), "/tmp/test_file_%zu", i);
		if ((fd[i] = open(buf, O_CREAT | O_RDWR, S_IFREG | 0666)) < 0)
			return -1;
	}

	return 0;
}


int close_files(int *fd, size_t cnt)
{
	size_t i;

	for (i = 0; i < cnt; ++i) {
		if (close(fd[i]) < 0)
			return -1;
	}

	return 0;
}


int write_files(int *fd, size_t cnt, char *data)
{
	size_t i;

	for (i = 0; i < cnt; ++i) {
		if (write(fd[i], data, 1 + i) != (1 + i))
			return -1;
	}

	return 0;
}


int read_files(int *fd, size_t cnt, char *data, char *buf)
{
	size_t i;

	for (i = 0; i < cnt; ++i) {
		if (lseek(fd[i], SEEK_SET, 0) != 0)
			return -1;
		if (read(fd[i], buf, 1 + i) != (1 + i))
			return -1;
		if (memcmp(data, buf, 1 + i) != 0)
			return -1;
	}

	return 0;
}


int stat_files(int *fd, size_t cnt, int exists)
{
	size_t i;
	struct stat statbuf;
	int ret;

	for (i = 0; i < cnt; ++i) {
		ret = fstat(fd[i], &statbuf);
		if (exists) {
			if (ret < 0)
				return -1;
		}
		else {
			if (ret != -1 || errno != EBADF)
				return -1;
		}
	}

	return 0;
}

/* Create directory unless it exists */
int createTmpIfMissing(int *isMissing)
{
	struct stat buffer;

	if (stat("/tmp", &buffer) != 0) {
		if (errno == ENOENT) {
			if (mkdir("/tmp", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0) {
				fprintf(stderr, "Creating /tmp directory by mkdir failed: %s\n", strerror(errno));
				return -1;
			}
			if (isMissing != NULL) {
				*isMissing = 1;
			}
			else {
				fprintf(stderr, "createTmpIfMissing argument is null\n");
				return -1;
			}
		}
		else {
			fprintf(stderr, "stat() on /tmp directory failed: %s\n", strerror(errno));
			return -1;
		}
	}

	return 0;
}
