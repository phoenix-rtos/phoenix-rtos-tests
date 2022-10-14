/*
 * Phoenix-RTOS
 *
 * phoenix-rtos-tests
 *
 * socket tests
 *
 * Copyright 2021 Phoenix Systems
 * Author: Ziemowit Leszczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "unity_fixture.h"


#define MAX_FD_CNT        16
#define CLOSE_LOOP_CNT    100
#define SENDMSG_LOOP_CNT  100
#define FORK_LOOP_CNT     100
#define MAX_TRANSFER_CNT  (1024 * 16)
#define TRANSFER_LOOP_CNT 100

static char data[1024];
static char buf[1024];

/* ignore warning when using sendmsg/recvmsg, we know it's not fully supported */
#pragma GCC diagnostic ignored "-Wattribute-warning"

ssize_t unix_msg_send(int sock, void *buf, size_t len, int *fd, size_t fdcnt)
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


ssize_t unix_msg_recv(int sock, void *buf, size_t len, int *fd, size_t *fdcnt)
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


ssize_t unix_dgram_socket(char *name)
{
	int fd;
	struct sockaddr_un addr = { 0 };

	unlink(name);

	if ((fd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0)
		return -1;

	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, name);

	if (bind(fd, (struct sockaddr *)&addr, SUN_LEN(&addr)) < 0) {
		close(fd);
		return -1;
	}

	return fd;
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
		if ((fd[i] = open(buf, O_CREAT | O_RDWR, 0666)) < 0)
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


int unlink_files(size_t cnt)
{
	size_t i;
	char buf[64];

	for (i = 0; i < cnt; ++i) {
		snprintf(buf, sizeof(buf), "/tmp/test_file_%zu", i);
		if (unlink(buf) < 0)
			return -1;
	}

	return 0;
}


int write_files(int *fd, size_t cnt)
{
	size_t i;

	for (i = 0; i < cnt; ++i) {
		if (write(fd[i], data, 1 + i) != (1 + i))
			return -1;
	}

	return 0;
}


int read_files(int *fd, size_t cnt)
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


TEST_GROUP(test_unix_socket);


TEST_SETUP(test_unix_socket)
{
	unsigned int i;

	srandom(time(NULL));

	for (i = 0; i < sizeof(data); i++) {
		data[i] = rand();
	}
}


TEST_TEAR_DOWN(test_unix_socket)
{
}


TEST(test_unix_socket, unix_zero_len_send)
{
	int fd[3];
	struct msghdr msg;
	struct iovec iov;
	struct cmsghdr *cmsg;
	union {
		char buf[CMSG_SPACE(sizeof(int)) * 3];
		struct cmsghdr align;
	} u;
	ssize_t n;

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, fd) < 0)
		FAIL("socketpair");

	/* write */
	{
		n = write(fd[0], NULL, 0);
		TEST_ASSERT(n == 0);

		n = write(fd[0], data, 0);
		TEST_ASSERT(n == 0);
	}

	/* writev */
	{
		n = writev(fd[0], NULL, 0);
		TEST_ASSERT(n == -1);
		TEST_ASSERT(errno == EINVAL);

		n = writev(fd[0], &iov, 0);
		TEST_ASSERT(n == -1);
		TEST_ASSERT(errno == EINVAL);

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

		memset(&msg, 0, sizeof(msg));
		msg.msg_iov = &iov;
		msg.msg_iovlen = 0;
		msg.msg_control = u.buf;
		msg.msg_controllen = CMSG_LEN(sizeof(int) * 2);
		cmsg = CMSG_FIRSTHDR(&msg);
		cmsg->cmsg_level = SOL_SOCKET;
		cmsg->cmsg_type = SCM_RIGHTS;
		cmsg->cmsg_len = CMSG_LEN(sizeof(int) * 2);
		memcpy(CMSG_DATA(cmsg), fd, sizeof(int) * 2);
		n = sendmsg(fd[0], &msg, 0);
		TEST_ASSERT(n == 0);

		fd[2] = 33333; /* should be bad descriptor */
		memset(&msg, 0, sizeof(msg));
		msg.msg_iov = &iov;
		msg.msg_iovlen = 0;
		msg.msg_control = u.buf;
		msg.msg_controllen = CMSG_LEN(sizeof(int) * 3);
		cmsg = CMSG_FIRSTHDR(&msg);
		cmsg->cmsg_level = SOL_SOCKET;
		cmsg->cmsg_type = SCM_RIGHTS;
		cmsg->cmsg_len = CMSG_LEN(sizeof(int) * 3);
		memcpy(CMSG_DATA(cmsg), fd, sizeof(int) * 3);
		/* NOTE: control data should be validated in any case */
		n = sendmsg(fd[0], &msg, 0);
		TEST_ASSERT(n == -1);
		TEST_ASSERT(errno == EBADF);
	}

	close(fd[0]);
	close(fd[1]);
}


TEST(test_unix_socket, unix_zero_len_recv)
{
	int fd[2];
	struct msghdr msg;
	struct iovec iov;
	ssize_t n;

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, fd) < 0)
		FAIL("socketpair");

	/* NOTE: receiving should block on zero len hence we use O_NONBLOCK or MSG_DONTWAIT below */

	/* read */
	{
		if (set_nonblock(fd[1], 1) < 0)
			FAIL("set_nonblock");

		n = read(fd[1], NULL, 0);
		TEST_ASSERT(n == -1);
		TEST_ASSERT(errno == EAGAIN);

		if (set_nonblock(fd[1], 0) < 0)
			FAIL("set_nonblock");
	}

	/* readv */
	{
		if (set_nonblock(fd[1], 1) < 0)
			FAIL("set_nonblock");

		n = readv(fd[1], NULL, 0);
		TEST_ASSERT(n == -1);
		TEST_ASSERT(errno == EINVAL);

		n = readv(fd[1], &iov, 0);
		TEST_ASSERT(n == -1);
		TEST_ASSERT(errno == EINVAL);

		iov.iov_base = NULL;
		iov.iov_len = 0;
		n = readv(fd[1], &iov, 1);
		TEST_ASSERT(n == -1);
		TEST_ASSERT(errno == EAGAIN);

		if (set_nonblock(fd[1], 0) < 0)
			FAIL("set_nonblock");
	}

	/* recv */
	n = recv(fd[1], NULL, 0, MSG_DONTWAIT);
	TEST_ASSERT(n == -1);
	TEST_ASSERT(errno == EAGAIN);

	/* recvfrom */
	n = recvfrom(fd[1], NULL, 0, MSG_DONTWAIT, NULL, 0);
	TEST_ASSERT(n == -1);
	TEST_ASSERT(errno == EAGAIN);

	/* recvmsg */
	{
		memset(&msg, 0, sizeof(msg));
		msg.msg_iov = NULL;
		msg.msg_iovlen = 0;
		n = recvmsg(fd[1], &msg, MSG_DONTWAIT);
		TEST_ASSERT(n == -1);
		TEST_ASSERT(errno == EAGAIN);

		memset(&msg, 0, sizeof(msg));
		iov.iov_base = NULL;
		iov.iov_len = 0;
		msg.msg_iov = &iov;
		msg.msg_iovlen = 1;
		n = recvmsg(fd[1], &msg, MSG_DONTWAIT);
		TEST_ASSERT(n == -1);
		TEST_ASSERT(errno == EAGAIN);
	}

	close(fd[0]);
	close(fd[1]);
}


TEST(test_unix_socket, unix_close)
{
	unsigned int i;
	int fd[2];
	ssize_t n;

	for (i = 0; i < CLOSE_LOOP_CNT; ++i) {
		if (socketpair(AF_UNIX, SOCK_STREAM, 0, fd) < 0)
			FAIL("socketpair");

		n = close(fd[0]);
		TEST_ASSERT(n == 0);
		n = close(fd[1]);
		TEST_ASSERT(n == 0);
	}
	// TODO: check memory leak

	for (i = 0; i < CLOSE_LOOP_CNT; ++i) {
		if ((fd[0] = unix_dgram_socket("/tmp/test_close")) < 0)
			FAIL("unix_dgram_socket");

		n = close(fd[0]);
		TEST_ASSERT(n == 0);
	}
	// TODO: check memory leak

	for (i = 0; i < CLOSE_LOOP_CNT; ++i) {
		int sfd, rfd;
		size_t rfdcnt;

		if ((sfd = unix_dgram_socket("/tmp/test_close")) < 0)
			FAIL("unix_dgram_socket");

		if (socketpair(AF_UNIX, SOCK_STREAM, 0, fd) < 0)
			FAIL("socketpair");

		n = unix_msg_send(fd[0], data, 1, &sfd, 1);
		TEST_ASSERT(n == 1);

		n = unix_msg_recv(fd[1], buf, sizeof(buf), &rfd, &rfdcnt);
		TEST_ASSERT(n == 1);
		TEST_ASSERT(rfdcnt == 1);

		n = close(rfd);
		TEST_ASSERT(n == 0);
		n = close(sfd);
		TEST_ASSERT(n == 0);
		n = close(fd[0]);
		TEST_ASSERT(n == 0);
		n = close(fd[1]);
		TEST_ASSERT(n == 0);
	}
	// TODO: check memory leak
}


void unix_msg_data_only(int type)
{
	unsigned int i;
	int fd[2];
	ssize_t n, m;
	size_t fdcnt;

	if (socketpair(AF_UNIX, type | SOCK_NONBLOCK, 0, fd) < 0)
		FAIL("socketpair");

	for (i = 0; i < SENDMSG_LOOP_CNT; ++i) {
		m = 1 + rand() % sizeof(data);

		n = unix_msg_send(fd[0], data, m, NULL, 0);
		TEST_ASSERT(n == m);

		n = unix_msg_recv(fd[1], buf, sizeof(buf), NULL, &fdcnt);
		TEST_ASSERT(n == m);
		TEST_ASSERT(fdcnt == 0);
		TEST_ASSERT(memcmp(data, buf, n) == 0);
	}

	close(fd[0]);
	close(fd[1]);
}

TEST(test_unix_socket, unix_msg_data_only)
{
	unix_msg_data_only(SOCK_STREAM);
	unix_msg_data_only(SOCK_DGRAM);
}


void unix_msg_data_and_fd(int type)
{
	unsigned int i;
	int fd[2];
	int sfd[MAX_FD_CNT];
	int rfd[MAX_FD_CNT];
	ssize_t n, m;
	size_t sfdcnt, rfdcnt;

	if (socketpair(AF_UNIX, type | SOCK_NONBLOCK, 0, fd) < 0)
		FAIL("socketpair");

	for (i = 0; i < SENDMSG_LOOP_CNT; ++i) {
		sfdcnt = rand() % (MAX_FD_CNT + 1);

		if (open_files(sfd, sfdcnt) < 0)
			FAIL("open_files");

		m = 1 + rand() % sizeof(data);

		n = unix_msg_send(fd[0], data, m, sfd, sfdcnt);
		TEST_ASSERT(n == m);

		if (close_files(sfd, sfdcnt) < 0)
			FAIL("close_files");

		n = unix_msg_recv(fd[1], buf, sizeof(buf), rfd, &rfdcnt);
		TEST_ASSERT(n == m);
		TEST_ASSERT(rfdcnt == sfdcnt);
		TEST_ASSERT(memcmp(data, buf, n) == 0);

		if (close_files(rfd, rfdcnt) < 0)
			FAIL("close_files");

		if (stat_files(sfd, sfdcnt, 0) < 0)
			FAIL("stat_files");

		if (stat_files(rfd, rfdcnt, 0) < 0)
			FAIL("stat_files");

		if (unlink_files(rfdcnt) < 0)
			FAIL("unlink_files");
	}

	close(fd[0]);
	close(fd[1]);
}


TEST(test_unix_socket, unix_msg_data_and_fd)
{
	unix_msg_data_and_fd(SOCK_STREAM);
	unix_msg_data_and_fd(SOCK_DGRAM);
}


void unix_msg_fork(int type)
{
	int fd[2];
	pid_t pid;
	size_t sfdcnt, rfdcnt;

	sfdcnt = rand() % (MAX_FD_CNT + 1);

	if (socketpair(AF_UNIX, type, 0, fd) < 0)
		FAIL("socketpair");

	if ((pid = fork()) < 0)
		FAIL("fork");

	if (pid) {
		int sfd[MAX_FD_CNT];
		ssize_t n;
		int status;

		if (open_files(sfd, sfdcnt) < 0)
			FAIL("open_files");

		if (write_files(sfd, sfdcnt) < 0)
			FAIL("write_files");

		n = unix_msg_send(fd[0], data, 1, sfd, sfdcnt);
		TEST_ASSERT(n == 1);

		if (close_files(sfd, sfdcnt) < 0)
			FAIL("close_files");

		if (waitpid(pid, &status, 0) < 0)
			FAIL("waitpid");

		TEST_ASSERT(WIFEXITED(status));
		TEST_ASSERT(WEXITSTATUS(status) == 0);

		if (stat_files(sfd, sfdcnt, 0) < 0)
			FAIL("stat_files");

		if (unlink_files(sfdcnt) < 0)
			FAIL("unlink_files");

		close(fd[0]);
		close(fd[1]);
	}
	else {
		int rfd[MAX_FD_CNT];
		ssize_t n;

		n = unix_msg_recv(fd[1], buf, sizeof(buf), rfd, &rfdcnt);
		if (n != 1 || rfdcnt != sfdcnt)
			exit(1);

		if (read_files(rfd, rfdcnt) < 0)
			FAIL("read_files");

		if (close_files(rfd, rfdcnt) < 0)
			exit(2);

		if (stat_files(rfd, rfdcnt, 0) < 0)
			FAIL("stat_files");

		exit(0);
	}
}


TEST(test_unix_socket, unix_msg_fork)
{
	unsigned int i;

	for (i = 0; i < FORK_LOOP_CNT; ++i) {
		unix_msg_fork(SOCK_STREAM);
		unix_msg_fork(SOCK_DGRAM);
	}
}


int unix_data_cmp(char *buf, size_t pos, size_t len)
{
	size_t i;

	for (i = 0; i < len; ++i) {
		if (buf[i] != data[(pos + i) % sizeof(data)])
			return 1;
	}

	return 0;
}


void unix_transfer(int type)
{
	int fd[2];
	pid_t pid;
	size_t tot_len;

	tot_len = 1 + rand() % MAX_TRANSFER_CNT;

	if (socketpair(AF_UNIX, type | SOCK_NONBLOCK, 0, fd) < 0)
		FAIL("socketpair");

	if ((pid = fork()) < 0)
		FAIL("fork");

	if (pid) {
		size_t max_len, len, pos = 0;
		ssize_t n;
		int status;

		while (tot_len > 0) {
			max_len = sizeof(data) - pos;
			if (tot_len < max_len)
				max_len = tot_len;
			len = 1 + rand() % max_len;
			n = send(fd[0], data + pos, len, 0);
			TEST_ASSERT(n > 0 || errno == EAGAIN);
			if (n > 0) {
				tot_len -= n;
				pos = (pos + n) % sizeof(data);
			}
		}

		if (waitpid(pid, &status, 0) < 0)
			FAIL("waitpid");

		TEST_ASSERT(WIFEXITED(status));
		TEST_ASSERT(WEXITSTATUS(status) == 0);

		close(fd[0]);
		close(fd[1]);
	}
	else {
		size_t pos = 0;
		ssize_t n;

		while (tot_len > 0) {
			n = recv(fd[1], buf, sizeof(buf), 0);
			TEST_ASSERT(n > 0 || errno == EAGAIN);
			if (n > 0) {
				TEST_ASSERT(unix_data_cmp(buf, pos, n) == 0);
				tot_len -= n;
				pos = (pos + n) % sizeof(data);
			}
		}

		exit(0);
	}
}


TEST(test_unix_socket, unix_transfer)
{
	unsigned int i;

	for (i = 0; i < TRANSFER_LOOP_CNT; ++i) {
		unix_transfer(SOCK_STREAM);
		unix_transfer(SOCK_DGRAM);
	}
}


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
		n = writev(fd[0], NULL, 0);
		TEST_ASSERT(n == -1);
		TEST_ASSERT(errno == EINVAL);

		n = writev(fd[0], &iov, 0);
		TEST_ASSERT(n == -1);
		TEST_ASSERT(errno == EINVAL);

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


TEST_GROUP_RUNNER(test_unix_socket)
{
	RUN_TEST_CASE(test_unix_socket, unix_zero_len_send);
	RUN_TEST_CASE(test_unix_socket, unix_zero_len_recv);
	RUN_TEST_CASE(test_unix_socket, unix_close);
	RUN_TEST_CASE(test_unix_socket, unix_msg_data_only);
	RUN_TEST_CASE(test_unix_socket, unix_msg_data_and_fd);
	RUN_TEST_CASE(test_unix_socket, unix_msg_fork);
	RUN_TEST_CASE(test_unix_socket, unix_transfer);
}


TEST_GROUP_RUNNER(test_inet_socket)
{
	RUN_TEST_CASE(test_inet_socket, inet_zero_len_send);
}


void runner(void)
{
	RUN_TEST_GROUP(test_unix_socket);
	RUN_TEST_GROUP(test_inet_socket);
}


int main(int argc, char *argv[])
{
	UnityMain(argc, (const char **)argv, runner);
	return 0;
}

/* restore -Wattribute-warning */
#pragma GCC diagnostic pop
