/*
 * Phoenix-RTOS
 *
 * phoenix-rtos-tests
 *
 * Copyright 2025 Phoenix Systems
 * Author: Andrzej Tlomak
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/threads.h>
#include <sys/msg.h>
#include <unistd.h>
#include <libgrspw.h>
#include <stdio.h>
#include <errno.h>
#include <board_config.h>


/* clang-format off */
#define LOG(fmt, ...)       printf("spwperf: " fmt "\n", ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) fprintf(stderr, "spwperf: %s: " fmt "\n", __func__, ##__VA_ARGS__)
/* clang-format on */


#define SPWPERF_PRIO    1
#define SPWPERF_STACKSZ 4096

/* timeout 3s */
#define SPWPERF_RX_TIMEOUT 1000000 * 3

#define SPWPERF_C_PATH "/dev/spw0"
#define SPWPERF_C_ID   id_spw0
#define SPWPERF_C_ADDR 0x3

#define SPWPERF_S_PATH "/dev/spw1"
#define SPWPERF_S_ID   id_spw1
#define SPWPERF_S_ADDR 0x4


#define SPWPERF_TEST_NPACKETS 128u
#define SPWPERF_TEST_HDRSZ    2u
#define SPWPERF_TEST_DATASZ   (SPW_MAX_PACKET_LEN - SPW_RX_MIN_BUFSZ - SPWPERF_TEST_HDRSZ)
#define SPWPERF_TEST_ACKSZ    1u
#define SPWPERF_TEST_ACKID    0x1u


static struct {
	oid_t clientOid;
	oid_t serverOid;
	time_t ttime; /* test timeout in seconds */
	char stack[SPWPERF_STACKSZ];
} spwperf_common;


/* helper functions */


static oid_t spwperf_getOid(const char *path)
{
	oid_t oid;
	while (lookup(path, NULL, &oid) < 0) {
		usleep(10000);
	}

	return oid;
}


/* tx */


static int spwperf_spwTx(const oid_t txOid, uint8_t *txBuf, const size_t txBufsz, const size_t nPackets, bool async)
{
	msg_t msg = {
		.type = mtDevCtl,
		.i = { .data = txBuf, .size = txBufsz },
		.o = { .data = NULL, .size = 0 },
		.oid.id = txOid.id,
		.oid.port = txOid.port,
	};

	spw_t *idevctl = (spw_t *)msg.i.raw;

	idevctl->type = spw_tx;
	idevctl->task.tx.nPackets = nPackets;
	idevctl->task.tx.async = async;

	if (msgSend(txOid.port, &msg) != 0) {
		LOG_ERROR("msgSend returned error");
		return -1;
	}

	if (nPackets != msg.o.err) {
		LOG_ERROR("mismatch nPackets %d", msg.o.err);
		return -1;
	}

	return nPackets;
}


/* rx */


static unsigned int spwperf_spwConfigureRx(const oid_t rxOid, const size_t nPackets)
{
	/* Configure RX on SPW0 */
	msg_t msg = {
		.type = mtDevCtl,
		.i = { .data = NULL, .size = 0 },
		.o = { .data = NULL, .size = 0 },
		.oid.id = rxOid.id,
		.oid.port = rxOid.port,
	};

	spw_t *idevctl = (spw_t *)msg.i.raw;
	spw_o_t *odevctl = (spw_o_t *)msg.o.raw;

	idevctl->type = spw_rxConfig;
	idevctl->task.rxConfig.nPackets = nPackets;

	if (msgSend(rxOid.port, &msg) != 0) {
		LOG_ERROR("msgSend returned error");
		return -1;
	}

	if (nPackets != msg.o.err) {
		LOG_ERROR("mismatch nPackets");
		return -1;
	}

	return odevctl->val;
}


static int spwperf_spwRxRead(const oid_t rxOid, const unsigned int firstDesc, uint8_t *rxBuf, size_t rxBufsz, spw_rxPacket_t *packets, const size_t nPackets)
{
	msg_t msg = {
		.type = mtDevCtl,
		.i = { .data = NULL, .size = 0 },
		.o = { .data = rxBuf, .size = rxBufsz },
		.oid.id = rxOid.id,
		.oid.port = rxOid.port,
	};
	spw_t *idevctl = (spw_t *)msg.i.raw;
	spw_o_t *odevctl = (spw_o_t *)msg.o.raw;

	idevctl->type = spw_rx;
	idevctl->task.rx.nPackets = nPackets;
	idevctl->task.rx.firstDesc = firstDesc;
	idevctl->task.rx.timeoutUs = SPWPERF_RX_TIMEOUT;

	msgSend(rxOid.port, &msg);

	if (msg.o.err != 0) {
		LOG_ERROR("rx error: %d", msg.o.err);
		return -1;
	}

	for (size_t i = 0; i < nPackets; i++) {
		rxBuf += spw_deserializeRxMsg(rxBuf, &packets[i]);
	}

	return odevctl->val;
}


/* client/server */


static void spwperf_sendAck(const oid_t txOid, uint8_t addr, uint8_t ackPackets)
{
	const uint8_t hdr[SPWPERF_TEST_HDRSZ] = { addr, SPWPERF_TEST_ACKID };
	const uint8_t data[SPWPERF_TEST_ACKSZ] = { ackPackets };

	static const size_t hdrSz = sizeof(hdr), dataSz = sizeof(data);
	const size_t txBufsz = (SPW_TX_MIN_BUFSZ + hdrSz + dataSz);

	uint8_t *txBuf = malloc(txBufsz);
	size_t size = spw_serializeTxMsg(SPW_TX_FLG_HDR_LEN(hdrSz), dataSz, hdr, data, txBuf, txBufsz);

	spwperf_spwTx(txOid, txBuf, size, 1, true);

	free(txBuf);
}


static int spwperf_waitAck(const oid_t rxOid)
{
	unsigned int firstDesc = spwperf_spwConfigureRx(rxOid, 1);

	/* protocol id size = 1 */
	const size_t rxBufsz = (SPW_RX_MIN_BUFSZ + (SPWPERF_TEST_HDRSZ - 1) + SPWPERF_TEST_ACKSZ);
	uint8_t *rxBuf = malloc(rxBufsz);

	spw_rxPacket_t packet;
	int rx_cnt = spwperf_spwRxRead(rxOid, firstDesc, rxBuf, rxBufsz, &packet, 1);

	size_t len = (packet.flags & SPW_RX_LEN_MSK) - 1;

	if ((rx_cnt < 0) || (len != SPWPERF_TEST_ACKSZ) || (packet.buf[0] != SPWPERF_TEST_ACKID)) {
		LOG_ERROR("Malformed ACK");
		free(rxBuf);
		return -1;
	}
	int ackPackets = packet.buf[1];

	free(rxBuf);
	return ackPackets;
}


static void spwperf_clientThread(void *arg)
{
	(void)arg;

	size_t nPackets = SPWPERF_TEST_NPACKETS;
	const uint8_t hdr[] = { SPWPERF_S_ADDR, /* protocol ID */ 0x5 };
	uint8_t *data = malloc(SPWPERF_TEST_DATASZ);
	const size_t hdrSz = sizeof(hdr), dataSz = sizeof(data);

	const size_t txBufsz = (SPW_TX_MIN_BUFSZ + hdrSz + dataSz) * nPackets;
	uint8_t *txBuf = malloc(txBufsz);

	size_t size = 0;
	for (size_t i = 0; i < nPackets; i++) {
		size_t ret = spw_serializeTxMsg(SPW_TX_FLG_HDR_LEN(hdrSz), dataSz, hdr, data, txBuf + size, txBufsz - size);
		size += ret;
	}
	/* wait for server thread */
	sleep(1);

	for (;;) {
		spwperf_spwTx(spwperf_common.clientOid, txBuf, size, nPackets, true);

		int ack = spwperf_waitAck(spwperf_common.clientOid);
		if (ack != nPackets) {
			LOG_ERROR("Mismatch ack: %d", ack);
			sleep(1);
		}
	}

	free(txBuf);
	free(data);
}


static void spwperf_serverThread(void *arg)
{
	size_t nPackets = SPWPERF_TEST_NPACKETS;
	uint32_t sumB = 0;

	const size_t rxBufsz = (SPW_RX_MIN_BUFSZ + SPWPERF_TEST_HDRSZ + SPWPERF_TEST_DATASZ) * nPackets;
	uint8_t *rxBuf = malloc(rxBufsz);

	struct timeval start;
	struct timeval end;

	gettimeofday(&start, NULL);

	while ((time(NULL) - start.tv_sec) <= spwperf_common.ttime) {
		unsigned int firstDesc = spwperf_spwConfigureRx(spwperf_common.serverOid, nPackets);


		spw_rxPacket_t packets[nPackets];
		int rx_cnt = spwperf_spwRxRead(spwperf_common.serverOid, firstDesc, rxBuf, rxBufsz, packets, nPackets);
		size_t len = packets->flags & SPW_RX_LEN_MSK;

		sumB += (rx_cnt * len);
		LOG("Received %u bytes", sumB);


		spwperf_sendAck(spwperf_common.serverOid, SPWPERF_C_ADDR, rx_cnt);
	}

	gettimeofday(&end, NULL);
	double elapsed = (end.tv_sec - start.tv_sec) + ((end.tv_usec - start.tv_usec) / 1e6);

	LOG("Test finished. Performance: %.2f Mb/s", (double)(sumB * 8.0) / (elapsed * 1e6));
	free(rxBuf);
}


static int spwperf_setAddress(const oid_t oid, const uint8_t addr)
{
	msg_t msg = {
		.type = mtDevCtl,
		.i = { .data = NULL, .size = 0 },
		.o = { .data = NULL, .size = 0 },
		.oid.id = oid.id,
		.oid.port = oid.port,
	};
	spw_t *ispwctl = (spw_t *)msg.i.raw;

	ispwctl->type = spw_config;
	ispwctl->task.config.node.addr = addr;
	ispwctl->task.config.node.mask = 0xff;
	ispwctl->task.config.dma.addr = addr;
	ispwctl->task.config.dma.mask = 0xff;

	if (msgSend(oid.port, &msg) != 0) {
		return -1;
	}
	if (msg.o.err != 0) {
		return -1;
	}

	return 0;
}


static void spwperf_usage(const char *progname)
{
	printf("Usage: %s [options]\n", progname);
	printf("Options:\n");
	printf("\t-n <id> - spwrtr core id\n");
}


int main(int argc, char **argv)
{
	/* default test time = 5s */
	time_t ttime = 5;
	int c;
	if (argc > 1) {
		do {
			c = getopt(argc, argv, "t:");
			switch (c) {
				case 't':
					ttime = atoi(optarg);
					break;

				case -1:
					break;

				default:
					spwperf_usage(argv[0]);
					return EXIT_FAILURE;
			}
		} while (c != -1);
	}

	if (ttime < 0) {
		return -EINVAL;
	}

	spwperf_common.clientOid = spwperf_getOid(SPWPERF_C_PATH);
	spwperf_common.serverOid = spwperf_getOid(SPWPERF_S_PATH);
	spwperf_common.ttime = ttime;

	int err;
	err = spwperf_setAddress(spwperf_common.clientOid, SPWPERF_C_ADDR);
	if (err != 0) {
		LOG_ERROR("Failed to set address oid: %lu", spwperf_common.clientOid.id);
	}

	err = spwperf_setAddress(spwperf_common.serverOid, SPWPERF_S_ADDR);
	if (err != 0) {
		LOG_ERROR("Failed to set address oid: %lu", spwperf_common.clientOid.id);
	}

	err = beginthread(spwperf_clientThread, SPWPERF_PRIO, spwperf_common.stack, SPWPERF_STACKSZ, NULL);
	if (err < 0) {
		LOG_ERROR("Failed to start client thread");
		return -1;
	}

	priority(SPWPERF_PRIO);
	spwperf_serverThread(NULL);

	return 0;
}
