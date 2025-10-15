/*
 * Phoenix-RTOS
 *
 * GRSPW2 driver tests
 *
 * Copyright 2025 Phoenix Systems
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#include <board_config.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/platform.h>
#include <arpa/inet.h>
#include <errno.h>

#include <libgrspw.h>
#include <unity_fixture.h>

#define TEST_SPW_ID0 id_spw0
#define TEST_SPW_ID1 id_spw1

#define TEST_SPW_PATH0 "/dev/spw0"
#define TEST_SPW_PATH1 "/dev/spw1"

#ifndef TEST_SPW_ADDR0
#define TEST_SPW_ADDR0 0x3
#endif

#ifndef TEST_SPW_ADDR1
#define TEST_SPW_ADDR1 0x4
#endif

#ifndef TEST_SPW_LOOPBACK
#define TEST_SPW_LOOPBACK 0
#endif

/* helper functions */


static oid_t test_getOid(const char *path)
{
	oid_t oid;
	while (lookup(path, NULL, &oid) < 0) {
		usleep(10000);
	}

	return oid;
}


static unsigned int test_spwConfigureRx(const oid_t rxOid, const size_t nPackets)
{
	/* Configure RX on SPW0 */
	msg_t msg = {
		.type = mtDevCtl,
		.i = { .data = NULL, .size = 0 },
		.o = { .data = NULL, .size = 0 },
		.oid.id = TEST_SPW_ID0,
		.oid.port = rxOid.port,
	};

	spw_t *idevctl = (spw_t *)msg.i.raw;
	spw_o_t *odevctl = (spw_o_t *)msg.o.raw;

	idevctl->type = spw_rxConfig;
	idevctl->task.rxConfig.nPackets = nPackets;

	TEST_ASSERT_EQUAL_INT(0, msgSend(rxOid.port, &msg));
	TEST_ASSERT_EQUAL_INT(nPackets, msg.o.err);

	return odevctl->val;
}


static void test_spwTx(const oid_t txOid, uint8_t *txBuf, const size_t txBufsz, const size_t nPackets, bool async)
{
	msg_t msg = {
		.type = mtDevCtl,
		.i = { .data = txBuf, .size = txBufsz },
		.o = { .data = NULL, .size = 0 },
		.oid.id = TEST_SPW_ID1,
		.oid.port = txOid.port,
	};

	spw_t *idevctl = (spw_t *)msg.i.raw;

	idevctl->type = spw_tx;
	idevctl->task.tx.nPackets = nPackets;
	idevctl->task.tx.async = async;

	TEST_ASSERT_EQUAL_INT(0, msgSend(txOid.port, &msg));
	TEST_ASSERT_EQUAL_INT(nPackets, msg.o.err);
}


static void test_spwRxRead(const oid_t rxOid, const unsigned int firstDesc, uint8_t *rxBuf, size_t rxBufsz, spw_rxPacket_t *packets, const size_t nPackets)
{
	msg_t msg = {
		.type = mtDevCtl,
		.i = { .data = NULL, .size = 0 },
		.o = { .data = rxBuf, .size = rxBufsz },
		.oid.id = TEST_SPW_ID0,
		.oid.port = rxOid.port,
	};
	spw_t *idevctl = (spw_t *)msg.i.raw;
	spw_o_t *odevctl = (spw_o_t *)msg.o.raw;

	idevctl->type = spw_rx;
	idevctl->task.rx.nPackets = nPackets;
	idevctl->task.rx.firstDesc = firstDesc;

	TEST_ASSERT_EQUAL_INT(0, msgSend(rxOid.port, &msg));
	TEST_ASSERT_EQUAL_INT(nPackets, odevctl->val);

	for (size_t i = 0; i < nPackets; i++) {
		rxBuf += spw_deserializeRxMsg(rxBuf, &packets[i]);
	}
}


static void test_spwRxTx(const size_t nPackets, bool async)
{
	oid_t rxOid = test_getOid(TEST_SPW_PATH0);
	unsigned int firstDesc = test_spwConfigureRx(rxOid, nPackets);

	oid_t txOid = test_getOid(TEST_SPW_PATH1);
	static const uint8_t hdr[] = { TEST_SPW_ADDR0, /* protocol ID */ 0x5 }, data[] = { 0x1, 0x2, 0x3, 0x4 };
	static const size_t hdrSz = sizeof(hdr), dataSz = sizeof(data);
	const size_t txBufsz = (SPW_TX_MIN_BUFSZ + hdrSz + dataSz) * nPackets;
	uint8_t *txBuf = malloc(txBufsz);
	TEST_ASSERT_NOT_NULL(txBuf);

	size_t size = 0;
	for (size_t i = 0; i < nPackets; i++) {
		size_t ret = spw_serializeTxMsg(SPW_TX_FLG_HDR_LEN(hdrSz), dataSz, hdr, data, txBuf + size, txBufsz - size);
		TEST_ASSERT_NOT_EQUAL(0, ret);
		size += ret;
	}
	test_spwTx(txOid, txBuf, size, nPackets, async);

	/* Receive packet */
	const size_t rxBufsz = (SPW_RX_MIN_BUFSZ + hdrSz + dataSz) * nPackets;
	uint8_t *rxBuf = malloc(rxBufsz);
	TEST_ASSERT_NOT_NULL(rxBuf);

	spw_rxPacket_t packets[nPackets];
	test_spwRxRead(rxOid, firstDesc, rxBuf, rxBufsz, packets, nPackets);

#if TEST_SPW_LOOPBACK
	for (size_t i = 0; i < nPackets; i++) {
		TEST_ASSERT_EQUAL(hdrSz + dataSz, packets[i].flags & SPW_RX_LEN_MSK);
		TEST_ASSERT_EQUAL_HEX8_ARRAY(hdr, packets[i].buf, hdrSz);
		TEST_ASSERT_EQUAL_HEX8_ARRAY(data, packets[i].buf + hdrSz, dataSz);
	}
#else
	for (size_t i = 0; i < nPackets; i++) {
		/* first byte of header (phy address) consumed by router */
		TEST_ASSERT_EQUAL((hdrSz + dataSz - 1), packets[i].flags & SPW_RX_LEN_MSK);
		TEST_ASSERT_EQUAL(hdr[1], packets[i].buf[0]);
		TEST_ASSERT_EQUAL_HEX8_ARRAY(data, packets[i].buf + hdrSz - 1, dataSz);
	}
#endif

	free(txBuf);
	free(rxBuf);
}


static void test_spwRxTimeout(uint32_t timeout)
{
	oid_t rxOid = test_getOid(TEST_SPW_PATH0);

	size_t nPackets = 1;
	unsigned int firstDesc = test_spwConfigureRx(rxOid, nPackets);

	const size_t rxBufsz = (SPW_RX_MIN_BUFSZ)*nPackets;
	uint8_t *rxBuf = malloc(rxBufsz);
	TEST_ASSERT_NOT_NULL(rxBuf);

	msg_t msg = {
		.type = mtDevCtl,
		.i = { .data = NULL, .size = 0 },
		.o = { .data = rxBuf, .size = rxBufsz },
		.oid.id = TEST_SPW_ID0,
		.oid.port = rxOid.port,
	};
	spw_t *idevctl = (spw_t *)msg.i.raw;
	spw_o_t *odevctl = (spw_o_t *)msg.o.raw;

	idevctl->type = spw_rx;
	idevctl->task.rx.nPackets = nPackets;
	idevctl->task.rx.firstDesc = firstDesc;
	idevctl->task.rx.timeoutUs = timeout;

	struct timeval start, end;
	gettimeofday(&start, NULL);

	int err = msgSend(rxOid.port, &msg);
	gettimeofday(&end, NULL);

	uint32_t diff = ((end.tv_sec - start.tv_sec) * 1e6) + (end.tv_usec - start.tv_usec);

	TEST_ASSERT_EQUAL_INT(0, err);

	TEST_ASSERT_GREATER_OR_EQUAL_INT(timeout, diff);
	TEST_ASSERT_LESS_OR_EQUAL_INT(timeout + 100000, diff);

	TEST_ASSERT_EQUAL_INT(-ETIME, msg.o.err);
	TEST_ASSERT_EQUAL_INT(0, odevctl->val);

	free(rxBuf);
}


static size_t test_spwRxReadTimeout(const oid_t rxOid, const unsigned int firstDesc, uint8_t *rxBuf, size_t rxBufsz, spw_rxPacket_t *packets, const size_t nPackets, uint32_t timeout)
{
	msg_t msg = {
		.type = mtDevCtl,
		.i = { .data = NULL, .size = 0 },
		.o = { .data = rxBuf, .size = rxBufsz },
		.oid.id = TEST_SPW_ID0,
		.oid.port = rxOid.port,
	};
	spw_t *idevctl = (spw_t *)msg.i.raw;
	spw_o_t *odevctl = (spw_o_t *)msg.o.raw;

	idevctl->type = spw_rx;
	idevctl->task.rx.nPackets = nPackets;
	idevctl->task.rx.firstDesc = firstDesc;
	idevctl->task.rx.timeoutUs = timeout;

	TEST_ASSERT_EQUAL_INT(0, msgSend(rxOid.port, &msg));

	for (size_t i = 0; i < odevctl->val; i++) {
		rxBuf += spw_deserializeRxMsg(rxBuf, &packets[i]);
	}

	return odevctl->val;
}


static void test_spwRxTxTimeout(size_t nPackets, size_t nLost, uint32_t timeoutUs)
{
	oid_t rxOid = test_getOid(TEST_SPW_PATH0);
	unsigned int firstDesc = test_spwConfigureRx(rxOid, nPackets);
	size_t nSent = (nPackets - nLost);

	oid_t txOid = test_getOid(TEST_SPW_PATH1);
	static const uint8_t hdr[] = { TEST_SPW_ADDR0, /* protocol ID */ 0x5 }, data[] = { 0x1, 0x2, 0x3, 0x4 };
	static const size_t hdrSz = sizeof(hdr), dataSz = sizeof(data);
	const size_t txBufsz = (SPW_TX_MIN_BUFSZ + hdrSz + dataSz) * nSent;
	uint8_t *txBuf = malloc(txBufsz);
	TEST_ASSERT_NOT_NULL(txBuf);

	size_t size = 0;
	for (size_t i = 0; i < nSent; i++) {
		size_t ret = spw_serializeTxMsg(SPW_TX_FLG_HDR_LEN(hdrSz), dataSz, hdr, data, txBuf + size, txBufsz - size);
		TEST_ASSERT_NOT_EQUAL(0, ret);
		size += ret;
	}
	test_spwTx(txOid, txBuf, size, nSent, true);

	/* Receive packet */
	const size_t rxBufsz = (SPW_RX_MIN_BUFSZ + hdrSz + dataSz) * nPackets;
	uint8_t *rxBuf = malloc(rxBufsz);
	TEST_ASSERT_NOT_NULL(rxBuf);

	spw_rxPacket_t packets[nPackets];
	size_t rPackets = test_spwRxReadTimeout(rxOid, firstDesc, rxBuf, rxBufsz, packets, nPackets, timeoutUs);

	TEST_ASSERT_EQUAL_INT(nSent, rPackets);

#if TEST_SPW_LOOPBACK
	for (size_t i = 0; i < nSent; i++) {
		TEST_ASSERT_EQUAL(hdrSz + dataSz, packets[i].flags & SPW_RX_LEN_MSK);
		TEST_ASSERT_EQUAL_HEX8_ARRAY(hdr, packets[i].buf, hdrSz);
		TEST_ASSERT_EQUAL_HEX8_ARRAY(data, packets[i].buf + hdrSz, dataSz);
	}
#else
	for (size_t i = 0; i < nSent; i++) {
		/* first byte of header (phy address) consumed by router */
		TEST_ASSERT_EQUAL((hdrSz + dataSz - 1), packets[i].flags & SPW_RX_LEN_MSK);
		TEST_ASSERT_EQUAL(hdr[1], packets[i].buf[0]);
		TEST_ASSERT_EQUAL_HEX8_ARRAY(data, packets[i].buf + hdrSz - 1, dataSz);
	}
#endif

	free(txBuf);
	free(rxBuf);
}


TEST_GROUP(test_spw);


TEST_SETUP(test_spw)
{
}


TEST_TEAR_DOWN(test_spw)
{
}


TEST(test_spw, spwSetAddress)
{
	oid_t oid = test_getOid(TEST_SPW_PATH0);
	msg_t msg = {
		.type = mtDevCtl,
		.i = { .data = NULL, .size = 0 },
		.o = { .data = NULL, .size = 0 },
		.oid.id = TEST_SPW_ID0,
		.oid.port = oid.port,
	};
	spw_t *ispwctl = (spw_t *)msg.i.raw;

	ispwctl->type = spw_config;
	ispwctl->task.config.node.addr = TEST_SPW_ADDR0;
	ispwctl->task.config.node.mask = 0xff;
	ispwctl->task.config.dma.addr = TEST_SPW_ADDR0;
	ispwctl->task.config.dma.mask = 0xff;

	TEST_ASSERT_EQUAL_INT(0, msgSend(oid.port, &msg));
	TEST_ASSERT_EQUAL_INT(0, msg.o.err);

	oid = test_getOid(TEST_SPW_PATH1);
	msg.oid.id = TEST_SPW_ID1;
	ispwctl->type = spw_config;
	ispwctl->task.config.node.addr = TEST_SPW_ADDR1;
	ispwctl->task.config.node.mask = 0xff;
	ispwctl->task.config.dma.addr = TEST_SPW_ADDR1;
	ispwctl->task.config.dma.mask = 0xff;

	TEST_ASSERT_EQUAL_INT(0, msgSend(oid.port, &msg));
	TEST_ASSERT_EQUAL_INT(0, msg.o.err);
}


TEST(test_spw, spwRxTimeout)
{
	test_spwRxTimeout(100000);
}


TEST(test_spw, spwTxRxSinglePacketSync)
{
	test_spwRxTx(1, false);
}


TEST(test_spw, spwTxRxSinglePacketAsync)
{
	test_spwRxTx(1, true);
}


TEST(test_spw, spwTxRxMultiplePacketsSync)
{
	test_spwRxTx(10, false);
}


TEST(test_spw, spwTxRxMultiplePacketsAsync)
{
	test_spwRxTx(10, true);
}

TEST(test_spw, spwTxRxBigNumberOfPacketsSync)
{
	/* Sync TX number of packets must not be bigger than SPW_TX_DESC_CNT */
	test_spwRxTx(64, false);
}


TEST(test_spw, spwTxRxBigNumberOfPacketsAsync)
{
	/* Async TX number of packets must not be bigger than SPW_RX_DESC_CNT */
	test_spwRxTx(128, true);
}


TEST(test_spw, spwTxRxTimeoutMultiplePackets)
{
	/* Lost number of packets must be smaller then TX number of packets */
	test_spwRxTxTimeout(128, 20, 100000);
}


TEST_GROUP_RUNNER(test_spw)
{
	RUN_TEST_CASE(test_spw, spwSetAddress);
	RUN_TEST_CASE(test_spw, spwRxTimeout);
	RUN_TEST_CASE(test_spw, spwTxRxSinglePacketSync);
	RUN_TEST_CASE(test_spw, spwTxRxSinglePacketAsync);
	RUN_TEST_CASE(test_spw, spwTxRxMultiplePacketsSync);
	RUN_TEST_CASE(test_spw, spwTxRxMultiplePacketsAsync);
	RUN_TEST_CASE(test_spw, spwTxRxBigNumberOfPacketsSync);
	RUN_TEST_CASE(test_spw, spwTxRxBigNumberOfPacketsAsync);
	RUN_TEST_CASE(test_spw, spwTxRxTimeoutMultiplePackets);
}


void runner(void)
{
	RUN_TEST_GROUP(test_spw);
}


int main(int argc, char *argv[])
{
	return (UnityMain(argc, (const char **)argv, runner) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
