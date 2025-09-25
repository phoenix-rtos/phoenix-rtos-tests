/*
 * Phoenix-RTOS
 *
 * Message (PC version) tests group
 *
 * Copyright 2025 Phoenix Systems
 * Author: Mateusz Piasecki
 *
 *
 * %LICENSE%
 */

#undef NDEBUG

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <sys/wait.h>

#include <sys/msg.h>

#include <unity_fixture.h>


#define COLOR_DIM_CYAN "\033[2;96m"
#define COLOR_RESET    "\033[0m"

static pid_t server;
static bool verbose = false;


static int savedStdout = -1;

void silenceStdout(void)
{
	int devnull = open("/dev/null", O_WRONLY);
	if (devnull == -1) {
		perror("open");
		return;
	}

	savedStdout = dup(STDOUT_FILENO);
	if (savedStdout == -1) {
		perror("dup");
		close(devnull);
		return;
	}

	if (dup2(devnull, STDOUT_FILENO) == -1) {
		perror("dup2");
	}

	close(devnull);
}

void restoreStdout(void)
{
	if (savedStdout == -1) {
		return;
	}

	if (dup2(savedStdout, STDOUT_FILENO) == -1) {
		perror("dup2");
	}
	close(savedStdout);
	savedStdout = -1;
}


TEST_GROUP(msg_pc_tests);


TEST_SETUP(msg_pc_tests)
{
	if (!verbose) {
		silenceStdout();
	}
	else {
		printf(COLOR_DIM_CYAN);
	}
}


TEST_TEAR_DOWN(msg_pc_tests)
{
	if (!verbose) {
		restoreStdout();
	}
	else {
		printf(COLOR_RESET);
	}
}


void prepareMsg(msg_t *msg, uint32_t port)
{
	msg->oid = (oid_t) { port, 0 };
	msg->i.data = NULL;
	msg->i.size = 0;
	msg->o.data = NULL;
	msg->o.size = 0;
}


void *echoThread(void *arg)
{
	uint32_t *port = (uint32_t *)arg;
	msg_rid_t rid;
	msg_t msg;

	if (msgRecv(*port, &msg, &rid) < 0) {
		printf("Error in msgRecv occurred\n");
		return NULL;
	}
	if (msgRespond(*port, &msg, rid) < 0) {
		printf("Error in msgRespond occurred\n");
		return NULL;
	}

	return NULL;
}


void *echoThreadWithError(void *arg)
{
	uint32_t *port = (uint32_t *)arg;
	msg_rid_t rid;
	msg_t msg;

	if (msgRecv(*port, &msg, &rid) < 0) {
		printf("Error in msgRecv occurred\n");
		return NULL;
	}
	msg.o.err = 22;
	if (msgRespond(*port, &msg, rid) < 0) {
		printf("Error in msgRespond occurred\n");
		return NULL;
	}

	return NULL;
}


void *pingThread(void *arg)
{
	uint32_t *port = (uint32_t *)arg;
	msg_t msg;

	prepareMsg(&msg, *port);
	msgSend(*port, &msg);

	return NULL;
}


void *pingThreadWithAssertDataIDataO(void *arg)
{
	uint32_t *port = (uint32_t *)arg;
	msg_t msg;
	char data_i[128];
	char data_o[128];

	prepareMsg(&msg, *port);
	msg.i.data = data_i;
	msg.i.size = 128;
	msg.o.data = data_o;
	msg.o.size = 128;
	TEST_ASSERT(msgSend(*port, &msg) == 0);

	return NULL;
}


void *dataGreetThread(void *arg)
{
	uint32_t *port = (uint32_t *)arg;
	msg_t msg;
	char *data = "Hi there!";

	prepareMsg(&msg, *port);
	msg.i.data = data;
	msg.i.size = strlen(data) + 1;
	msgSend(*port, &msg);

	return NULL;
}


void *rawGreetThread(void *arg)
{
	uint32_t *port = (uint32_t *)arg;
	msg_t msg;
	char *data = "Hi there!";

	prepareMsg(&msg, *port);
	strcpy((char *)msg.i.raw, data);
	msgSend(*port, &msg);

	return NULL;
}


TEST(msg_pc_tests, lookup_nonexistent_port)
{
	oid_t oid;

	TEST_ASSERT(lookup("/nonexisting", NULL, &oid) < 0);
}


TEST(msg_pc_tests, msgSend_nonexistent_port)
{
	msg_t msg;

	prepareMsg(&msg, 99);
	TEST_ASSERT(msgSend(99, &msg) < 0);
}


TEST(msg_pc_tests, absolute_path_port_lookup_and_msgSend)
{
	uint32_t port;
	oid_t oid;
	msg_t msg;

	TEST_ASSERT(lookup("/testport", NULL, &oid) == 0);
	port = oid.port;

	prepareMsg(&msg, port);
	TEST_ASSERT(msgSend(port, &msg) == 0);
}


TEST(msg_pc_tests, relative_path_port_lookup_and_msgSend)
{
	uint32_t port;
	oid_t oid;
	msg_t msg;

	TEST_ASSERT(lookup("testport", NULL, &oid) == 0);
	port = oid.port;

	prepareMsg(&msg, port);
	TEST_ASSERT(msgSend(port, &msg) == 0);
}


TEST(msg_pc_tests, root_path_port_lookup_and_msgSend)
{
	uint32_t port;
	oid_t oid;
	msg_t msg;

	TEST_ASSERT(lookup("/", NULL, &oid) == 0);
	port = oid.port;

	prepareMsg(&msg, port);
	TEST_ASSERT(msgSend(port, &msg) == 0);
}


TEST(msg_pc_tests, portCreate_portDestroy)
{
	uint32_t port;

	TEST_ASSERT(portCreate(&port) == 0);
	portDestroy(port);
}

TEST(msg_pc_tests, portDestroy_while_msgRecv)
{
	uint32_t port;
	pthread_t thread;

	TEST_ASSERT(portCreate(&port) == 0);
	pthread_create(&thread, NULL, echoThread, &port);
	usleep(1000);
	portDestroy(port);
	pthread_join(thread, NULL);
}


TEST(msg_pc_tests, portCreate_portRegister_and_lookup)
{
	uint32_t port;
	oid_t oid = { 0, 25 };

	TEST_ASSERT(portCreate(&port) == 0);
	TEST_ASSERT(portRegister(port, "localport", &oid) == 0);
	oid.id = 0;
	TEST_ASSERT(lookup("localport", NULL, &oid) == 0);
	TEST_ASSERT_EQUAL_UINT64(25, oid.id);
	portDestroy(port);
}


TEST(msg_pc_tests, portCreate_portRegister_and_lookup_multiple_ports)
{
	uint32_t port1, port2, port3;
	oid_t oid;

	TEST_ASSERT(portCreate(&port1) == 0);
	TEST_ASSERT(portCreate(&port2) == 0);
	TEST_ASSERT(portCreate(&port3) == 0);

	TEST_ASSERT(portRegister(port1, "localport1", &oid) == 0);
	TEST_ASSERT(portRegister(port2, "localport2", &oid) == 0);
	TEST_ASSERT(portRegister(port3, "localport3", &oid) == 0);

	TEST_ASSERT(lookup("localport1", NULL, &oid) == 0);
	TEST_ASSERT(lookup("localport2", NULL, &oid) == 0);
	TEST_ASSERT(lookup("localport3", NULL, &oid) == 0);

	portDestroy(port1);
	portDestroy(port2);
	portDestroy(port3);
}


TEST(msg_pc_tests, portCreate_and_msgSend_without_registering)
{
	uint32_t port;
	msg_t msg;
	pthread_t thread;

	TEST_ASSERT(portCreate(&port) == 0);

	pthread_create(&thread, NULL, echoThread, &port);

	prepareMsg(&msg, port);
	TEST_ASSERT(msgSend(port, &msg) == 0);

	pthread_join(thread, NULL);
	portDestroy(port);
}

TEST(msg_pc_tests, msgSend_with_returned_error)
{
	uint32_t port;
	msg_t msg;
	pthread_t thread;

	TEST_ASSERT(portCreate(&port) == 0);

	pthread_create(&thread, NULL, echoThreadWithError, &port);

	prepareMsg(&msg, port);
	TEST_ASSERT(msgSend(port, &msg) == 0);

	TEST_ASSERT_EQUAL_INT(22, msg.o.err);

	pthread_join(thread, NULL);
	portDestroy(port);
}


TEST(msg_pc_tests, portCreate_and_msgSend_without_registering_multiple_ports)
{
	uint32_t port1, port2, port3;
	msg_t msg;
	pthread_t thread1, thread2, thread3;

	TEST_ASSERT(portCreate(&port1) == 0);
	TEST_ASSERT(portCreate(&port2) == 0);
	TEST_ASSERT(portCreate(&port3) == 0);

	pthread_create(&thread1, NULL, echoThread, &port1);
	pthread_create(&thread2, NULL, echoThread, &port2);
	pthread_create(&thread3, NULL, echoThread, &port3);

	prepareMsg(&msg, port1);
	TEST_ASSERT(msgSend(port1, &msg) == 0);

	prepareMsg(&msg, port2);
	TEST_ASSERT(msgSend(port2, &msg) == 0);

	prepareMsg(&msg, port3);
	TEST_ASSERT(msgSend(port3, &msg) == 0);

	pthread_join(thread1, NULL);
	pthread_join(thread2, NULL);
	pthread_join(thread3, NULL);

	portDestroy(port1);
	portDestroy(port2);
	portDestroy(port3);
}


TEST(msg_pc_tests, portCreate_partial_msgRecv_and_portDestroy)
{
	uint32_t port;
	msg_t msg;
	pthread_t thread[6];
	msg_rid_t rid;

	TEST_ASSERT(portCreate(&port) == 0);

	pthread_create(&thread[0], NULL, pingThread, &port);
	pthread_create(&thread[1], NULL, pingThread, &port);
	pthread_create(&thread[2], NULL, pingThread, &port);
	pthread_create(&thread[3], NULL, pingThread, &port);
	pthread_create(&thread[4], NULL, pingThread, &port);
	pthread_create(&thread[5], NULL, pingThread, &port);

	TEST_ASSERT(msgRecv(port, &msg, &rid) == 0);
	TEST_ASSERT(msgRecv(port, &msg, &rid) == 0);
	TEST_ASSERT(msgRecv(port, &msg, &rid) == 0);

	portDestroy(port);

	pthread_join(thread[0], NULL);
	pthread_join(thread[1], NULL);
	pthread_join(thread[2], NULL);
	pthread_join(thread[3], NULL);
	pthread_join(thread[4], NULL);
	pthread_join(thread[5], NULL);
}


TEST(msg_pc_tests, portCreate_msgRecv_and_msgRespond)
{
	uint32_t port;
	msg_t msg;
	pthread_t thread;
	msg_rid_t rid;

	TEST_ASSERT(portCreate(&port) == 0);

	pthread_create(&thread, NULL, pingThread, &port);

	TEST_ASSERT(msgRecv(port, &msg, &rid) == 0);

	TEST_ASSERT(msgRespond(port, &msg, rid) == 0);

	pthread_join(thread, NULL);

	portDestroy(port);
}


TEST(msg_pc_tests, msgRecv_nonexistent_port)
{
	msg_t msg;
	msg_rid_t rid;

	TEST_ASSERT(msgRecv(99, &msg, &rid) < 0);
}


TEST(msg_pc_tests, msgRespond_wrong_rid)
{
	uint32_t port;
	msg_t msg;
	pthread_t thread;
	msg_rid_t rid;

	TEST_ASSERT(portCreate(&port) == 0);

	pthread_create(&thread, NULL, pingThread, &port);

	TEST_ASSERT(msgRecv(port, &msg, &rid) == 0);

	TEST_ASSERT(msgRespond(port, &msg, (msg_rid_t)35271) < 0);

	portDestroy(port);

	pthread_join(thread, NULL);
}


TEST(msg_pc_tests, msgSend_with_data_o)
{
	uint32_t port;
	oid_t oid;
	msg_t msg;
	char data[32];

	TEST_ASSERT(lookup("greetport", NULL, &oid) == 0);
	port = oid.port;

	prepareMsg(&msg, port);
	msg.o.data = data;
	msg.o.size = 32;

	TEST_ASSERT(msgSend(port, &msg) == 0);

	TEST_ASSERT_EQUAL_STRING("Hello from greetport", data);
}


TEST(msg_pc_tests, msgRecv_with_data_i)
{
	uint32_t port;
	msg_t msg;
	msg_rid_t rid;
	pthread_t thread;

	TEST_ASSERT(portCreate(&port) == 0);

	pthread_create(&thread, NULL, dataGreetThread, &port);

	TEST_ASSERT(msgRecv(port, &msg, &rid) == 0);

	TEST_ASSERT_EQUAL_STRING("Hi there!", msg.i.data);

	msgRespond(port, &msg, rid);

	pthread_join(thread, NULL);

	portDestroy(port);
}


TEST(msg_pc_tests, msgSend_with_raw_o)
{
	uint32_t port;
	oid_t oid;
	msg_t msg;

	TEST_ASSERT(lookup("rawTinker", NULL, &oid) == 0);
	port = oid.port;

	prepareMsg(&msg, port);
	TEST_ASSERT(msgSend(port, &msg) == 0);

	TEST_ASSERT_EQUAL_STRING("Hello from rawTinker", msg.o.raw);
}


TEST(msg_pc_tests, msgRecv_with_raw_i)
{
	uint32_t port;
	msg_t msg;
	msg_rid_t rid;
	pthread_t thread;

	TEST_ASSERT(portCreate(&port) == 0);

	pthread_create(&thread, NULL, rawGreetThread, &port);

	TEST_ASSERT(msgRecv(port, &msg, &rid) == 0);

	TEST_ASSERT_EQUAL_STRING("Hi there!", msg.i.raw);

	msgRespond(port, &msg, rid);

	pthread_join(thread, NULL);

	portDestroy(port);
}


TEST(msg_pc_tests, dos_local_data_i)
{
	uint32_t port;
	msg_t msg;
	pthread_t thread;
	int i;
	char data_i[256];
	char data_o[256];

	TEST_ASSERT(portCreate(&port) == 0);

	for (i = 0; i < 1000; ++i) {
		pthread_create(&thread, NULL, echoThread, &port);
		prepareMsg(&msg, port);
		msg.i.data = data_i;
		msg.i.size = 256;
		msg.o.data = data_o;
		msg.o.size = 256;
		TEST_ASSERT(msgSend(port, &msg) == 0);
		pthread_join(thread, NULL);
	}

	portDestroy(port);
}


TEST(msg_pc_tests, dos_remote_data_o)
{
	uint32_t port;
	oid_t oid;
	msg_t msg;
	char data[32];
	int i;

	TEST_ASSERT(lookup("greetport", NULL, &oid) == 0);
	port = oid.port;

	for (i = 0; i < 10000; ++i) {
		prepareMsg(&msg, port);
		msg.o.data = data;
		msg.o.size = 32;

		TEST_ASSERT(msgSend(port, &msg) == 0);
	}
}


TEST(msg_pc_tests, ddos_remote_multiport)
{
	uint32_t port[3];
	oid_t oid;
	pthread_t thread[12];
	int i;

	TEST_ASSERT(lookup("/testport", NULL, &oid) == 0);
	port[0] = oid.port;
	TEST_ASSERT(lookup("rawTinker", NULL, &oid) == 0);
	port[1] = oid.port;
	TEST_ASSERT(lookup("greetport", NULL, &oid) == 0);
	port[2] = oid.port;

	for (i = 0; i < 100; ++i) {
		pthread_create(&thread[0], NULL, pingThreadWithAssertDataIDataO, &port[0]);
		pthread_create(&thread[1], NULL, pingThreadWithAssertDataIDataO, &port[1]);
		pthread_create(&thread[2], NULL, pingThreadWithAssertDataIDataO, &port[2]);
		pthread_create(&thread[3], NULL, pingThreadWithAssertDataIDataO, &port[0]);
		pthread_create(&thread[4], NULL, pingThreadWithAssertDataIDataO, &port[1]);
		pthread_create(&thread[5], NULL, pingThreadWithAssertDataIDataO, &port[2]);
		pthread_create(&thread[6], NULL, pingThreadWithAssertDataIDataO, &port[0]);
		pthread_create(&thread[7], NULL, pingThreadWithAssertDataIDataO, &port[0]);
		pthread_create(&thread[8], NULL, pingThreadWithAssertDataIDataO, &port[1]);
		pthread_create(&thread[9], NULL, pingThreadWithAssertDataIDataO, &port[1]);
		pthread_create(&thread[10], NULL, pingThreadWithAssertDataIDataO, &port[2]);
		pthread_create(&thread[11], NULL, pingThreadWithAssertDataIDataO, &port[2]);

		pthread_join(thread[0], NULL);
		pthread_join(thread[1], NULL);
		pthread_join(thread[2], NULL);
		pthread_join(thread[3], NULL);
		pthread_join(thread[4], NULL);
		pthread_join(thread[5], NULL);
		pthread_join(thread[6], NULL);
		pthread_join(thread[7], NULL);
		pthread_join(thread[8], NULL);
		pthread_join(thread[9], NULL);
		pthread_join(thread[10], NULL);
		pthread_join(thread[11], NULL);
	}
}


TEST_GROUP_RUNNER(msg_pc_tests)
{
	server = fork();
	if (server < 0) {
		fprintf(stderr, "fork failed!");
		exit(1);
	}

	if (server == 0) {
		silenceStdout();
		execlp("./msg-server", "msg-server", NULL);
	}
	else {
		usleep(500000);

		RUN_TEST_CASE(msg_pc_tests, lookup_nonexistent_port);
		RUN_TEST_CASE(msg_pc_tests, msgSend_nonexistent_port);
		RUN_TEST_CASE(msg_pc_tests, absolute_path_port_lookup_and_msgSend);
		RUN_TEST_CASE(msg_pc_tests, relative_path_port_lookup_and_msgSend);
		RUN_TEST_CASE(msg_pc_tests, root_path_port_lookup_and_msgSend);
		RUN_TEST_CASE(msg_pc_tests, portCreate_portDestroy);
		RUN_TEST_CASE(msg_pc_tests, portDestroy_while_msgRecv);
		RUN_TEST_CASE(msg_pc_tests, portCreate_portRegister_and_lookup);
		RUN_TEST_CASE(msg_pc_tests, portCreate_portRegister_and_lookup_multiple_ports);
		RUN_TEST_CASE(msg_pc_tests, portCreate_and_msgSend_without_registering);
		RUN_TEST_CASE(msg_pc_tests, msgSend_with_returned_error);
		RUN_TEST_CASE(msg_pc_tests, portCreate_and_msgSend_without_registering_multiple_ports);
		RUN_TEST_CASE(msg_pc_tests, portCreate_partial_msgRecv_and_portDestroy);
		RUN_TEST_CASE(msg_pc_tests, portCreate_msgRecv_and_msgRespond);
		RUN_TEST_CASE(msg_pc_tests, msgRecv_nonexistent_port);
		RUN_TEST_CASE(msg_pc_tests, msgRespond_wrong_rid);
		RUN_TEST_CASE(msg_pc_tests, msgSend_with_data_o);
		RUN_TEST_CASE(msg_pc_tests, msgRecv_with_data_i);
		RUN_TEST_CASE(msg_pc_tests, msgSend_with_raw_o);
		RUN_TEST_CASE(msg_pc_tests, msgRecv_with_raw_i);
		RUN_TEST_CASE(msg_pc_tests, dos_local_data_i);
		RUN_TEST_CASE(msg_pc_tests, dos_remote_data_o);
		RUN_TEST_CASE(msg_pc_tests, ddos_remote_multiport);

		if (kill(server, SIGINT) == 0) {
			waitpid(server, NULL, 0);
			printf("Test server exited gracefully\n");
		}
		else {
			kill(server, SIGKILL);
			printf("Test server had to be forcefully terminated!\n");
			exit(1);
		}
	}
}


void runner(void)
{
	RUN_TEST_GROUP(msg_pc_tests);
}


int main(int argc, char *argv[])
{
	for (int i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "-v") == 0) {
			verbose = true;
		}
	}
	return (UnityMain(argc, (const char **)argv, runner) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
