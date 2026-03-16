/*
 * Phoenix-RTOS
 *
 * test-sys-msg
 *
 * Tests for rendezvous message passing
 *
 * Copyright 2026 Phoenix Systems
 * Author: Adam Greloch
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include <posix/utils.h>

#include <sys/threads.h>
#include <sys/msg.h>

#include <unity_fixture.h>


TEST_GROUP(msg_test_common);


static pid_t safe_fork(void)
{
	pid_t pid;
	fflush(stdout);
	if ((pid = fork()) < 0) {
		if (errno == ENOSYS) {
			TEST_IGNORE_MESSAGE("fork syscall not supported");
		}
		else {
			FAIL("fork");
		}
	}
	return pid;
}


TEST_SETUP(msg_test_common)
{
}


TEST_TEAR_DOWN(msg_test_common)
{
}


TEST(msg_test_common, init_buf)
{
	TEST_ASSERT_NOT_EQUAL(NULL, msgInitBuf());
}


TEST(msg_test_common, msg_errnos)
{
	TEST_ASSERT_EQUAL_INT(-EINVAL, msgCall(111));
}


static int setup_port_dev(const char *name, uint32_t *rport)
{
	uint32_t port;
	int err;

	err = portCreate(&port);
	if (err < 0) {
		return err;
	}

	oid_t oid;
	oid.port = port;
	oid.id = 0;

	if (rport != NULL) {
		*rport = port;
	}

	return create_dev(&oid, name);
}


TEST(msg_test_common, msg_port_setup)
{
	TEST_ASSERT_GREATER_OR_EQUAL_INT(0, setup_port_dev("/dev/haha", NULL));
}

void assert_child_exit(pid_t pid)
{
	/* TODO: replace with SIGINT */
	int status;
	usleep(1000);

	if (waitpid(pid, &status, WNOHANG) < 0) {
		FAIL("waitpid");
	}

	kill(pid, SIGKILL);

	// TEST_ASSERT(WIFEXITED(status));
	// TEST_ASSERT(WEXITSTATUS(status) == 0);
}


TEST(msg_test_common, msg_transfer)
{
	const char *dev_path = "/dev/msg_transfer";

	int exp_err = 42;
	int exp_label = mtWrite;

	pid_t pid;

	if ((pid = safe_fork()) == 0) {
		uint32_t port = 0;
		if (setup_port_dev(dev_path, &port) < 0) {
			exit(3);
		}

		msgBuf_t *buf = msgInitBuf();
		void *reply;

		buf->label = exp_label + 5;

		reply = msgRecv2(port);
		if (reply == NULL) {
			exit(1);
		}
		if (exp_label != buf->label) {
			exit(2);
		}

		buf->err = exp_err;
		if (msgRespond2(port, reply) < 0) {
			exit(4);
		}

		while (1) {
			if (msgRespondAndRecv(port) == NULL) {
				exit(buf->err == -EINTR ? 0 : 5);
			}
			if (exp_label != buf->label) {
				exit(6);
			}
			buf->err = exp_err;
		}
	}
	else {
		oid_t oid;
		while (lookup(dev_path, NULL, &oid) < 0) {
			usleep(100 * 1000);
		}
		uint32_t port = oid.port;
		msgBuf_t *buf = msgInitBuf();

		for (int retry = 0; retry < 6; retry++) {
			buf->err = exp_err + 5;
			buf->label = exp_label;
			TEST_ASSERT_EQUAL_INT(0, msgCall(port));
			TEST_ASSERT_EQUAL_INT(exp_err, buf->err);
		}

		assert_child_exit(pid);
	}
}


TEST(msg_test_common, msg_respond_while_in_respond_and_recv)
{
	const char *dev_path = "/dev/msg_respond_while_in_respond_and_recv";

	pid_t pid;

	if ((pid = safe_fork()) == 0) {
		uint32_t port = 0;

		if (setup_port_dev(dev_path, &port) < 0) {
			exit(3);
		}

		msgBuf_t *buf = msgInitBuf();

		while (1) {
			if (msgRespondAndRecv(port) == NULL) {
				exit(buf->err == -EINTR ? 0 : 1);
			}
			buf->err = 0;

			if (msgRespond2(port, NULL) != -EINVAL) {
				exit(2);
			}
		}
	}
	else {
		oid_t oid;
		while (lookup(dev_path, NULL, &oid) < 0) {
			usleep(100 * 1000);
		}
		uint32_t port = oid.port;
		(void)msgInitBuf();

		TEST_ASSERT_EQUAL_INT(0, msgCall(port));

		assert_child_exit(pid);
	}
}


TEST_GROUP_RUNNER(msg_test_common)
{
	RUN_TEST_CASE(msg_test_common, init_buf);
	RUN_TEST_CASE(msg_test_common, msg_errnos);
	RUN_TEST_CASE(msg_test_common, msg_port_setup);
	RUN_TEST_CASE(msg_test_common, msg_transfer);
	RUN_TEST_CASE(msg_test_common, msg_respond_while_in_respond_and_recv);
}


void runner(void)
{
	RUN_TEST_GROUP(msg_test_common);
}


int main(int argc, char *argv[])
{
	return (UnityMain(argc, (const char **)argv, runner) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
