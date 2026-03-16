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


TEST(msg_test_common, msg_errnos)
{
	msg_t msg = { 0 };
	TEST_ASSERT_EQUAL_INT(-EINVAL, msgSend(111, &msg));
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

	if (WIFEXITED(status)) {
		TEST_ASSERT_EQUAL_INT(0, WEXITSTATUS(status));
	}

	kill(pid, SIGKILL);
}


TEST(msg_test_common, msg_transfer)
{
	const char *dev_path = "/dev/msg_transfer";

	int exp_err = 42;
	int exp_label = mtWrite;

	pid_t pid;

#define BUF1_STR "abcd"
#define BUF2_STR "efghij"

	if ((pid = safe_fork()) == 0) {
		uint32_t port = 0;
		if (setup_port_dev(dev_path, &port) < 0) {
			exit(3);
		}

		msg_t msg = { 0 };
		msg_rid_t reply;

		char buf[6] = BUF2_STR;

		msg.type = exp_label + 5;

		while (1) {
			if (msgRecv(port, &msg, &reply) < 0) {
				exit(1);
			}
			if (exp_label != msg.type) {
				exit(2);
			}

			if (strncmp((char *)msg.i.data, BUF1_STR, sizeof(BUF1_STR)) != 0) {
				exit(11);
			}

			memcpy(msg.o.data, buf, sizeof(buf));

			msg.o.err = exp_err;
			if (msgRespond(port, &msg, reply) < 0) {
				exit(4);
			}
		}

		// TODO
		// while (1) {
		// 	if (msgRespondAndRecv(port, &msg, &reply) < 0) {
		// 		exit(msg.o.err == -EINTR ? 0 : 5);
		// 	}
		// 	if (exp_label != msg.type) {
		// 		exit(6);
		// 	}
		//     memcpy(msg.o.data, buf, sizeof(buf));
		// 	msg.o.err = exp_err;
		// }
	}
	else {
		oid_t oid;
		while (lookup(dev_path, NULL, &oid) < 0) {
			usleep(100 * 1000);
		}
		uint32_t port = oid.port;
		msg_t msg = { 0 };

		char buf[5] = BUF1_STR;
		char obuf[6];

		for (int retry = 0; retry < 100; retry++) {
			msg.o.err = exp_err + 5;
			msg.type = exp_label;
			msg.i.data = &buf;
			msg.i.size = sizeof(buf);

			msg.o.data = &obuf;
			msg.o.size = sizeof(obuf);
			if (msgSend(port, &msg) == EOK) {
				TEST_ASSERT_EQUAL_INT(exp_err, msg.o.err);
				TEST_ASSERT_EQUAL_INT(0, strncmp((char *)msg.o.data, BUF2_STR, sizeof(BUF2_STR)));
			}
			else {
				assert_child_exit(pid);
				FAIL("should not fail");
			}
		}

		assert_child_exit(pid);
	}
}


TEST(msg_test_common, msg_recv_interruptible)
{
	const char *dev_path = "/dev/msg_recv_interruptible";

	pid_t pid;

	if ((pid = safe_fork()) != 0) {
		uint32_t port = 0;

		TEST_ASSERT_GREATER_OR_EQUAL_INT(0, setup_port_dev(dev_path, &port));

		msg_t msg = { 0 };
		msg_rid_t reply;

		msg.o.err = 0;
		while (msgRecv(port, &msg, &reply) < 0) {
			if (msg.o.err != EOK) {
				TEST_ASSERT_EQUAL_INT(-EINTR, msg.o.err);
			}
			else {
				TEST_ASSERT_EQUAL_INT(42, msg.o.pulse);
				break;
			}
		}

		if (waitpid(pid, NULL, 0) < 0) {
			FAIL("waitpid");
		}
	}
	else {
		oid_t oid;
		while (lookup(dev_path, NULL, &oid) < 0) {
			usleep(100 * 1000);
		}
		usleep(100 * 1000);

		msgPulse(oid.port, 42);

		exit(0);
	}
}


TEST_GROUP_RUNNER(msg_test_common)
{
	RUN_TEST_CASE(msg_test_common, msg_errnos);
	RUN_TEST_CASE(msg_test_common, msg_port_setup);
	RUN_TEST_CASE(msg_test_common, msg_transfer);
	RUN_TEST_CASE(msg_test_common, msg_recv_interruptible);
}


void runner(void)
{
	RUN_TEST_GROUP(msg_test_common);
}


int main(int argc, char *argv[])
{
	return (UnityMain(argc, (const char **)argv, runner) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
