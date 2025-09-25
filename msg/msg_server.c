/*
 * Phoenix-RTOS
 *
 * Message (PC version) tests utility
 *
 * Copyright 2025 Phoenix Systems
 * Author: Mateusz Piasecki
 *
 *
 * %LICENSE%
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>

#include <sys/msg.h>


void *echoPortThread(void *arg)
{
	char *name = (char *)arg;
	msg_rid_t rid;
	uint32_t port;
	oid_t oid;
	msg_t msg;

	portCreate(&port);

	oid = (oid_t) { port, 1 };
	portRegister(port, name, &oid);

	while (1) {
		if (msgRecv(port, &msg, &rid) < 0) {
			printf("Error in msgRecv occurred\n");
			continue;
		}

		if (msgRespond(port, &msg, rid) < 0) {
			printf("Error in msgRespond occurred\n");
			continue;
		}
	}

	return NULL;
}


void *dataGreetPortThread(void *arg)
{
	char *name = (char *)arg;
	char *message;
	char *greeting = "Hello from ";
	size_t messageSize;
	msg_rid_t rid;
	uint32_t port;
	oid_t oid;
	msg_t msg;

	messageSize = strlen(greeting) + strlen(name) + 1;
	message = malloc(messageSize);

	if (message == NULL) {
		perror("malloc");
		exit(1);
	}

	strcpy(message, greeting);
	strcpy(message + strlen(greeting), name);

	portCreate(&port);

	oid = (oid_t) { port, 1 };
	portRegister(port, name, &oid);

	while (1) {
		if (msgRecv(port, &msg, &rid) < 0) {
			printf("Error in msgRecv occurred\n");
			continue;
		}

		if (msg.o.size >= messageSize) {
			strcpy(msg.o.data, message);
		}

		if (msgRespond(port, &msg, rid) < 0) {
			printf("Error in msgRespond occurred\n");
			continue;
		}
	}

	return NULL;
}

void *rawGreetPortThread(void *arg)
{
	char *name = (char *)arg;
	char *message;
	char *greeting = "Hello from ";
	size_t messageSize;
	msg_rid_t rid;
	uint32_t port;
	oid_t oid;
	msg_t msg;

	messageSize = strlen(greeting) + strlen(name) + 1;
	message = malloc(messageSize);

	if (message == NULL) {
		perror("malloc");
		exit(1);
	}

	strcpy(message, greeting);
	strcpy(message + strlen(greeting), name);

	portCreate(&port);

	oid = (oid_t) { port, 1 };
	portRegister(port, name, &oid);

	while (1) {
		if (msgRecv(port, &msg, &rid) < 0) {
			printf("Error in msgRecv occurred\n");
			continue;
		}

		strcpy((char *)msg.o.raw, message);

		if (msgRespond(port, &msg, rid) < 0) {
			printf("Error in msgRespond occurred\n");
			continue;
		}
	}

	return NULL;
}


int main(void)
{
	pthread_t thread[5];

	setbuf(stdout, NULL);

	pthread_create(&thread[0], NULL, echoPortThread, "/testport");
	pthread_create(&thread[1], NULL, echoPortThread, "testport");
	pthread_create(&thread[2], NULL, echoPortThread, "/");
	pthread_create(&thread[3], NULL, dataGreetPortThread, "greetport");
	pthread_create(&thread[3], NULL, rawGreetPortThread, "rawTinker");

	pthread_join(thread[0], NULL);
	pthread_join(thread[1], NULL);
	pthread_join(thread[2], NULL);
	pthread_join(thread[3], NULL);
	pthread_join(thread[4], NULL);

	return 0;
}
