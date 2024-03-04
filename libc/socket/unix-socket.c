/*
 * Phoenix-RTOS
 *
 * test-libc-socket
 *
 * unix socket tests
 *
 * Copyright 2021, 2024 Phoenix Systems
 * Author: Ziemowit Leszczynski, Adam Debek
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
#include <signal.h>
#include <poll.h>
#include <sys/uio.h>
#include <sys/wait.h>

#include "common.h"
#include "unity_fixture.h"


char data[DATA_SIZE];
char buf[DATA_SIZE];


ssize_t unix_named_socket(int type, const char *name)
{
	int fd;
	struct sockaddr_un addr = { 0 };

	unlink(name);

	if ((fd = socket(AF_UNIX, type, 0)) < 0)
		return -1;

	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, name);

	if (bind(fd, (struct sockaddr *)&addr, SUN_LEN(&addr)) < 0) {
		close(fd);
		return -1;
	}

	return fd;
}


int unix_connect(int fd, const char *name)
{
	struct sockaddr_un addr = { 0 };

	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, name);

	return connect(fd, (struct sockaddr *)&addr, SUN_LEN(&addr));
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


TEST_GROUP(test_unix_socket);


TEST_SETUP(test_unix_socket)
{
	size_t i;

	srandom(time(NULL));

	for (i = 0; i < sizeof(data); i++) {
		data[i] = rand();
	}
}


TEST_TEAR_DOWN(test_unix_socket)
{
}


TEST(test_unix_socket, zero_len_send)
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

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, fd) < 0) {
		FAIL("socketpair");
	}

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


TEST(test_unix_socket, zero_len_recv)
{
	int fd[2];
	struct msghdr msg;
	struct iovec iov;
	ssize_t n;

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, fd) < 0)
		FAIL("socketpair");

		/* NOTE: receiving should block on zero len hence we use O_NONBLOCK or MSG_DONTWAIT below */

#if 0
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

	/* readv - fails */
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
#endif

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


TEST(test_unix_socket, close)
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
		if ((fd[0] = unix_named_socket(SOCK_DGRAM, "/tmp/test_close")) < 0)
			FAIL("unix_named_socket");

		n = close(fd[0]);
		TEST_ASSERT(n == 0);
	}
	// TODO: check memory leak

	for (i = 0; i < CLOSE_LOOP_CNT; ++i) {
		int sfd, rfd;
		size_t rfdcnt;

		if ((sfd = unix_named_socket(SOCK_DGRAM, "/tmp/test_close")) < 0)
			FAIL("unix_named_socket");

		if (socketpair(AF_UNIX, SOCK_STREAM, 0, fd) < 0)
			FAIL("socketpair");

		n = msg_send(fd[0], data, 1, &sfd, 1);
		TEST_ASSERT(n == 1);

		n = msg_recv(fd[1], buf, sizeof(buf), &rfd, &rfdcnt);
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
	ssize_t n, m, r, sum = 0;
	size_t fdcnt = 0;

	if (socketpair(AF_UNIX, type | SOCK_NONBLOCK, 0, fd) < 0)
		FAIL("socketpair");

	for (i = 0; i < SENDMSG_LOOP_CNT; ++i, sum = 0) {
		m = 1 + rand() % sizeof(data);

		while (sum != m) {
			errno = 0;
			n = msg_send(fd[0], data, m - sum, NULL, fdcnt);
			if (n < 0) {
				TEST_ASSERT(errno = EMSGSIZE);
				break;
			}
			TEST_ASSERT(n >= 0 && errno == 0);

			r = msg_recv(fd[1], buf, sizeof(buf), NULL, &fdcnt);
			TEST_ASSERT(n == r);
			TEST_ASSERT(fdcnt == 0);
			TEST_ASSERT(memcmp(data, buf, n) == 0);

			sum += n;
		}
	}

	close(fd[0]);
	close(fd[1]);
}


TEST(test_unix_socket, msg_data_only)
{
	unix_msg_data_only(SOCK_STREAM);
	unix_msg_data_only(SOCK_DGRAM);
}


void unix_msg_data_and_fd(int type)
{
	int i;
	int fd[2];
	int sfd[MAX_FD_CNT];
	int rfd[MAX_FD_CNT];
	ssize_t n, m, r, sum = 0;
	size_t sfdcnt, rfdcnt;

	if (socketpair(AF_UNIX, type | SOCK_NONBLOCK, 0, fd) < 0)
		FAIL("socketpair");

	for (i = 0; i < SENDMSG_LOOP_CNT; ++i) {
		m = 1 + rand() % DATA_SIZE;
		sfdcnt = rand() % (MAX_FD_CNT + 1);

		if (open_files(sfd, sfdcnt) < 0)
			FAIL("open_files");

		while (sum != m) {
			errno = 0;
			n = msg_send(fd[0], data, m - sum, sfd, sfdcnt);
			if (n < 0) {
				TEST_ASSERT(errno = EMSGSIZE);

				if (close_files(sfd, sfdcnt) < 0)
					FAIL("close_files");
				if (unlink_files(sfdcnt) < 0)
					FAIL("unlink_files");

				break;
			}
			TEST_ASSERT(n >= 0 && errno == 0);

			if (close_files(sfd, sfdcnt) < 0)
				FAIL("close_files");

			r = msg_recv(fd[1], buf, sizeof(buf), rfd, &rfdcnt);
			TEST_ASSERT(n == r);
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

			sum += n;
		}
	}

	close(fd[0]);
	close(fd[1]);
}


TEST(test_unix_socket, stream_sock_data_and_fd)
{
	unix_msg_data_and_fd(SOCK_STREAM);
}


TEST(test_unix_socket, dgram_sock_data_and_fd)
{
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

	if ((pid = fork()) < 0) {
		if (errno == ENOSYS) {
			TEST_IGNORE_MESSAGE("fork syscall not supported");
		}
		else {
			FAIL("fork");
		}
	}

	if (pid) {
		int sfd[MAX_FD_CNT];
		ssize_t n;
		int status;

		if (open_files(sfd, sfdcnt) < 0)
			FAIL("open_files");

		if (write_files(sfd, sfdcnt, data) < 0)
			FAIL("write_files");

		n = msg_send(fd[0], data, 1, sfd, sfdcnt);
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

		n = msg_recv(fd[1], buf, sizeof(buf), rfd, &rfdcnt);
		if (n != 1 || rfdcnt != sfdcnt)
			exit(1);

		if (read_files(rfd, rfdcnt, data, buf) < 0)
			FAIL("read_files");

		if (close_files(rfd, rfdcnt) < 0)
			exit(2);

		if (stat_files(rfd, rfdcnt, 0) < 0)
			FAIL("stat_files");

		exit(0);
	}
}


TEST(test_unix_socket, stream_sock_msg_fork)
{
	unsigned int i;

	for (i = 0; i < FORK_LOOP_CNT; ++i) {
		unix_msg_fork(SOCK_STREAM);
	}
}


TEST(test_unix_socket, dgram_sock_msg_fork)
{
	unsigned int i;

	for (i = 0; i < FORK_LOOP_CNT; ++i) {
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

	if ((pid = fork()) < 0) {
		if (errno == ENOSYS) {
			TEST_IGNORE_MESSAGE("fork syscall not supported");
		}
		else {
			FAIL("fork");
		}
	}

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


TEST(test_unix_socket, transfer)
{
	unsigned int i;

	for (i = 0; i < TRANSFER_LOOP_CNT; ++i) {
		unix_transfer(SOCK_STREAM);
		unix_transfer(SOCK_DGRAM);
	}
}


void unix_close_connected(int type)
{
	int fd[2];

	if (socketpair(AF_UNIX, type, 0, fd) < 0)
		FAIL("socketpair");

	close(fd[0]);
	close(fd[1]);
}


TEST(test_unix_socket, close_connected)
{
	unsigned int i;

	for (i = 0; i < CONNECTED_LOOP_CNT; ++i) {
		unix_close_connected(SOCK_STREAM);
		unix_close_connected(SOCK_DGRAM);
		unix_close_connected(SOCK_SEQPACKET);
	}
}


volatile int got_epipe;

void sighandler(int sig)
{
	got_epipe = 1;
}


void unix_send_after_close(int type, int epipe, int err)
{
	int fd[2];
	ssize_t n;

	signal(SIGPIPE, sighandler);

	if (socketpair(AF_UNIX, type, 0, fd) < 0)
		FAIL("socketpair");

	close(fd[1]);

	got_epipe = 0;
	n = send(fd[0], data, sizeof(data), 0);
	TEST_ASSERT(got_epipe == epipe);
	TEST_ASSERT(n == -1);
	TEST_ASSERT(errno == err);

	got_epipe = 0;
	n = send(fd[0], data, sizeof(data), 0);
	TEST_ASSERT(got_epipe == epipe);
	TEST_ASSERT(n == -1);

	close(fd[0]);

	signal(SIGPIPE, SIG_DFL);
}


TEST(test_unix_socket, send_after_close)
{
	unsigned int i;

	for (i = 0; i < CONNECTED_LOOP_CNT; ++i) {
		unix_send_after_close(SOCK_STREAM, 1, EPIPE);
#ifdef __phoenix__
		unix_send_after_close(SOCK_DGRAM, 0, ECONNREFUSED);
		unix_send_after_close(SOCK_SEQPACKET, 1, EPIPE);
#endif
	}
}


void unix_recv_after_close(int type)
{
	int fd[2];
	ssize_t n;

	if (socketpair(AF_UNIX, type, 0, fd) < 0)
		FAIL("socketpair");

	close(fd[1]);

	n = recv(fd[0], buf, sizeof(buf), 0);
	TEST_ASSERT(n == 0); /* EOS */

	close(fd[0]);

	if (socketpair(AF_UNIX, type, 0, fd) < 0)
		FAIL("socketpair");

	n = send(fd[1], data, sizeof(data), 0);
	TEST_ASSERT(n == sizeof(data));

	close(fd[1]);

	n = recv(fd[0], buf, sizeof(buf), 0);
	TEST_ASSERT(n == sizeof(buf));

	n = recv(fd[0], buf, sizeof(buf), 0);
	TEST_ASSERT(n == 0); /* EOS */

	close(fd[0]);
}


TEST(test_unix_socket, recv_after_close)
{
	unsigned int i;

	for (i = 0; i < CONNECTED_LOOP_CNT; ++i) {
		unix_recv_after_close(SOCK_STREAM);
		unix_recv_after_close(SOCK_SEQPACKET);
	}
}


void unix_connect_after_close(int type)
{
	int fd[3];
	int rv;

	if (socketpair(AF_UNIX, type, 0, fd) < 0)
		FAIL("socketpair");

	close(fd[1]);

	if ((fd[2] = unix_named_socket(SOCK_DGRAM, "/tmp/test_connect_after_close")) < 0)
		FAIL("unix_named_socket(SOCK_DGRAM, ");

	rv = unix_connect(fd[0], "/tmp/test_connect_after_close");
	TEST_ASSERT(rv == -1);
	/* EPROTOTYPE??? */
	// TEST_ASSERT(errno == EISCONN);

	close(fd[0]);
}


TEST(test_unix_socket, connect_after_close)
{
	unsigned int i;

	for (i = 0; i < CONNECTED_LOOP_CNT; ++i) {
		unix_connect_after_close(SOCK_STREAM);
		unix_connect_after_close(SOCK_SEQPACKET);
	}
}


void unix_poll(int type)
{
	int fd[2];
	struct pollfd fds[2];
	struct timespec ts[2];
	int rv, ms;

	fds[0].fd = 11111;
	fds[1].fd = 22222;
	fds[0].events = 0;
	fds[1].events = 0;
	fds[0].revents = 0;
	fds[1].revents = 0;
	rv = poll(fds, 2, 0);
	TEST_ASSERT(rv == 2);
	TEST_ASSERT(fds[0].revents == POLLNVAL);
	TEST_ASSERT(fds[1].revents == POLLNVAL);

	if (socketpair(AF_UNIX, type, 0, fd) < 0)
		FAIL("socketpair");

	fds[0].fd = fd[0];
	fds[1].fd = fd[1];

	clock_gettime(CLOCK_REALTIME, &ts[0]);
	fds[0].events = POLLIN;
	fds[1].events = POLLIN;
	fds[0].revents = 0;
	fds[1].revents = 0;
	rv = poll(fds, 2, 300);
	clock_gettime(CLOCK_REALTIME, &ts[1]);
	ms = (ts[1].tv_sec - ts[0].tv_sec) * 1000 + (ts[1].tv_nsec - ts[0].tv_nsec) / 1000000;
	TEST_ASSERT(rv == 0);
	TEST_ASSERT(fds[0].revents == 0);
	TEST_ASSERT(fds[1].revents == 0);
	TEST_ASSERT(ms < 310);
	TEST_ASSERT(ms > 290);

	clock_gettime(CLOCK_REALTIME, &ts[0]);
	fds[0].events = POLLIN | POLLOUT;
	fds[1].events = POLLIN | POLLOUT;
	fds[0].revents = 0;
	fds[1].revents = 0;
	rv = poll(fds, 2, 1000);
	clock_gettime(CLOCK_REALTIME, &ts[1]);
	ms = (ts[1].tv_sec - ts[0].tv_sec) * 1000 + (ts[1].tv_nsec - ts[0].tv_nsec) / 1000000;
	TEST_ASSERT(rv == 2);
	TEST_ASSERT(fds[0].revents == POLLOUT);
	TEST_ASSERT(fds[1].revents == POLLOUT);
	TEST_ASSERT(ms <= 1);

	send(fd[0], data, sizeof(data), 0);
	send(fd[1], data, sizeof(data), 0);

	clock_gettime(CLOCK_REALTIME, &ts[0]);
	fds[0].events = POLLIN;
	fds[1].events = POLLIN;
	fds[0].revents = 0;
	fds[1].revents = 0;
	rv = poll(fds, 2, 1000);
	clock_gettime(CLOCK_REALTIME, &ts[1]);
	ms = (ts[1].tv_sec - ts[0].tv_sec) * 1000 + (ts[1].tv_nsec - ts[0].tv_nsec) / 1000000;
	TEST_ASSERT(rv == 2);
	TEST_ASSERT(fds[0].revents == POLLIN);
	TEST_ASSERT(fds[1].revents == POLLIN);
	TEST_ASSERT(ms <= 1);

	close(fd[0]);
	close(fd[1]);
}


TEST(test_unix_socket, poll)
{
	unix_poll(SOCK_STREAM);
	unix_poll(SOCK_DGRAM);
	unix_poll(SOCK_SEQPACKET);
}


TEST_GROUP_RUNNER(test_unix_socket)
{
	RUN_TEST_CASE(test_unix_socket, zero_len_send);
	RUN_TEST_CASE(test_unix_socket, zero_len_recv);
	RUN_TEST_CASE(test_unix_socket, close);
	RUN_TEST_CASE(test_unix_socket, msg_data_only);
	RUN_TEST_CASE(test_unix_socket, stream_sock_data_and_fd);
	RUN_TEST_CASE(test_unix_socket, dgram_sock_data_and_fd);
	RUN_TEST_CASE(test_unix_socket, stream_sock_msg_fork);
	RUN_TEST_CASE(test_unix_socket, dgram_sock_msg_fork);
	RUN_TEST_CASE(test_unix_socket, transfer);
	RUN_TEST_CASE(test_unix_socket, close_connected);
	RUN_TEST_CASE(test_unix_socket, send_after_close);
	RUN_TEST_CASE(test_unix_socket, recv_after_close);
	RUN_TEST_CASE(test_unix_socket, connect_after_close);
	RUN_TEST_CASE(test_unix_socket, poll);
}

void runner(void)
{
	RUN_TEST_GROUP(test_unix_socket);
}


int main(int argc, char *argv[])
{
	/* Assume /tmp dir is missing */
	int isMissing = 0;

	if (createTmpIfMissing(&isMissing) < 0) {
		exit(EXIT_FAILURE);
	}

	UnityMain(argc, (const char **)argv, runner);

	if (isMissing) {
		rmdir("/tmp");
	}

	return 0;
}
