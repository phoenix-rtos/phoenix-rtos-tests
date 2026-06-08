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

#include "unity_fixture.h"
#include "common.h"


void setup_msg(struct msghdr *msg, struct iovec *iov, void *buf, size_t buflen, void *cbuf, size_t cbuflen)
{
	memset(msg, 0, sizeof(*msg));
	iov->iov_base = buf;
	iov->iov_len = buflen;
	msg->msg_iov = iov;
	msg->msg_iovlen = 1;
	msg->msg_control = cbuf;
	msg->msg_controllen = cbuflen;
}


void pack_fds(struct msghdr *msg, int *fd, size_t fdcnt)
{
	struct cmsghdr *cmsg;
	if (fd != NULL) {
		cmsg = CMSG_FIRSTHDR(msg);
		cmsg->cmsg_level = SOL_SOCKET;
		cmsg->cmsg_type = SCM_RIGHTS;
		cmsg->cmsg_len = CMSG_LEN(sizeof(int) * fdcnt);
		memcpy(CMSG_DATA(cmsg), fd, sizeof(int) * fdcnt);
		msg->msg_controllen = CMSG_LEN(sizeof(int) * fdcnt);
	}
	else {
		msg->msg_control = NULL;
		msg->msg_controllen = 0;
	}
}


int unpack_fds(struct msghdr *msg, int *fd, size_t *fdcnt)
{
	struct cmsghdr *cmsg;
	size_t count = 0;

	for (cmsg = CMSG_FIRSTHDR(msg); cmsg; cmsg = CMSG_NXTHDR(msg, cmsg)) {
		if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS) {
			char *cmsg_data = (char *)CMSG_DATA(cmsg);
			char *cmsg_end = (char *)cmsg + cmsg->cmsg_len;
			size_t cnt = (cmsg_end - cmsg_data) / sizeof(int);

			if (count + cnt <= *fdcnt) {
				count += cnt;
			}
			else {
				break;
			}

			if (fd != NULL) {
				memcpy(fd, cmsg_data, sizeof(int) * cnt);
				fd += cnt;
			}
		}
		else {
			for (int i = 0; i < count; i++) {
				close(fd[i]);
				fd[i] = -1;
			}
			return -1;
		}
	}
	*fdcnt = count;
	return 0;
}


ssize_t msg_send(int sock, void *buf, size_t len, int *fd, size_t fdcnt)
{
	struct msghdr msg;
	struct iovec iov;
	union {
		char buf[CMSG_SPACE(sizeof(int)) * MAX_FD_CNT];
		struct cmsghdr align;
	} u;

	setup_msg(&msg, &iov, buf, len, u.buf, sizeof(u));
	pack_fds(&msg, fd, fdcnt);
	return sendmsg(sock, &msg, 0);
}


ssize_t msg_recv(int sock, void *buf, size_t len, int *fd, size_t *fdcnt)
{
	struct msghdr msg;
	struct iovec iov;
	union {
		char buf[CMSG_SPACE(sizeof(int)) * MAX_FD_CNT];
		struct cmsghdr align;
	} u;
	ssize_t n;

	setup_msg(&msg, &iov, buf, len, u.buf, sizeof(u));
	n = recvmsg(sock, &msg, 0);
	if (n < 0) {
		return -1;
	}

	*fdcnt = MAX_FD_CNT;
	if (unpack_fds(&msg, fd, fdcnt) < 0) {
		return -1;
	}

	return n;
}


int get_flags(int fd)
{
	return fcntl(fd, F_GETFL, 0);
}


int get_fd_flags(int fd)
{
	return fcntl(fd, F_GETFD, 0);
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
		snprintf(buf, sizeof(buf), TEST_FILE_PATH_TEMPLATE, i);
		if ((fd[i] = open(buf, O_CREAT | O_RDWR, 0666)) < 0)
			return -1;
	}

	return 0;
}


int open_files_with_flags(int *fd, int *flags, size_t cnt)
{
	size_t i;
	char buf[64];

	for (i = 0; i < cnt; ++i) {
		snprintf(buf, sizeof(buf), TEST_FILE_PATH_TEMPLATE, i);
		if ((fd[i] = open(buf, O_CREAT | flags[i], 0666)) < 0)
			return -1;
	}

	return 0;
}


int close_files(int *fd, size_t cnt)
{
	size_t i;

	for (i = 0; i < cnt; ++i) {
		if (fd[i] >= 0) {
			if (close(fd[i]) < 0) {
				return -1;
			}
			fd[i] = -1;
		}
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
		if (lseek(fd[i], 0, SEEK_SET) != 0)
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
