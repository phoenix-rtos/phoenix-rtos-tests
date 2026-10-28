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

#include <libgrspw-srv.h>
#include <unity_fixture.h>

#include <stdint.h>
#include <stddef.h>
#include <math.h>

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

/* defined in us */
#define TEST_RX_TIMEOUT           100000
#define TEST_RX_TIMEOUT_THRESHOLD 100000

#ifndef TEST_SPW_LOOPBACK
#define TEST_SPW_LOOPBACK 1
#endif

#define TEST_DATA_PAYLOAD_1KB (1024)
#define TEST_DATA_PAYLOAD_1MB (1024 * 1024)

/* helper functions */


static oid_t test_getOid(const char *path)
{
	oid_t oid;
	while (lookup(path, NULL, &oid) < 0) {
		usleep(10000);
	}

	return oid;
}


static uint8_t *test_spwAllocPatternBuffer(size_t size)
{
	uint8_t *buf = malloc(size);
	TEST_ASSERT_NOT_NULL(buf);

	for (size_t i = 0; i < size; i++) {
		buf[i] = (uint8_t)(i & 0xFF);
	}

	return buf;
}


static void test_spwCalcGrowingChunkSizes(size_t totalSize, size_t nPackets, double growthRate, size_t *outSizes)
{
	if (nPackets == 1) {
		outSizes[0] = totalSize;
		return;
	}

	double q = 1.0 + growthRate;
	double sumFactors = (pow(q, (double)nPackets) - 1.0) / (q - 1.0);
	double baseSize = (double)totalSize / sumFactors;

	size_t allocated = 0;
	for (size_t i = 0; i < nPackets - 1; i++) {
		size_t sz = (size_t)round(baseSize * pow(q, (double)i));
		if (sz == 0) {
			sz = 1;
		}
		outSizes[i] = sz;
		allocated += sz;
	}

	TEST_ASSERT_LESS_THAN(totalSize, allocated);
	outSizes[nPackets - 1] = totalSize - allocated;
}


static unsigned int test_spwConfigureRxOffLimits(const oid_t rxOid)
{
	const size_t nPackets = SPW_RX_DESC_CNT + 1;
	/* Configure RX on SPW0 */
	msg_t msg = {
		.type = mtDevCtl,
		.i = { .data = NULL, .size = 0 },
		.o = { .data = NULL, .size = 0 },
		.oid.id = TEST_SPW_ID0,
		.oid.port = rxOid.port,
	};

	spw_i_t *idevctl = (spw_i_t *)msg.i.raw;
	spw_o_t *odevctl = (spw_o_t *)msg.o.raw;

	idevctl->type = spw_rxConfig;
	idevctl->task.rxConfig.nPackets = nPackets;

	TEST_ASSERT_EQUAL_INT(0, msgSend(rxOid.port, &msg));
	TEST_ASSERT_LESS_THAN_INT(0, msg.o.err);

	return odevctl->val;
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

	spw_i_t *idevctl = (spw_i_t *)msg.i.raw;
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

	spw_i_t *idevctl = (spw_i_t *)msg.i.raw;

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
	spw_i_t *idevctl = (spw_i_t *)msg.i.raw;
	spw_o_t *odevctl = (spw_o_t *)msg.o.raw;

	idevctl->type = spw_rx;
	idevctl->task.rx.nPackets = nPackets;
	idevctl->task.rx.firstDesc = firstDesc;

	TEST_ASSERT_EQUAL_INT(0, msgSend(rxOid.port, &msg));
	TEST_ASSERT_EQUAL_INT(0, msg.o.err);
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

#if TEST_SPW_LOOPBACK
	/* Cable Loopback between 1st and 2nd port. Routering will be: AMBA -> 1 -> (loopback) -> 2 -> AMBA */
	static const uint8_t hdr[] = { 0x1, TEST_SPW_ADDR0, /* protocol ID */ 0x5 }, data[] = { 0x1, 0x2, 0x3, 0x4 };
#else
	static const uint8_t hdr[] = { TEST_SPW_ADDR0, /* protocol ID */ 0x5 }, data[] = { 0x1, 0x2, 0x3, 0x4 };
#endif

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
		/* two bytes of header (phy addresses) consumed by router */
		TEST_ASSERT_EQUAL(hdrSz - 2 + dataSz, packets[i].flags & SPW_RX_LEN_MSK);
		TEST_ASSERT_EQUAL(hdr[2], packets[i].buf[0]);
		TEST_ASSERT_EQUAL_HEX8_ARRAY(data, packets[i].buf + hdrSz - 2, dataSz);
	}
#else
	for (size_t i = 0; i < nPackets; i++) {
		/* first byte of header (phy address) consumed by router */
		TEST_ASSERT_EQUAL((hdrSz - 1 + dataSz), packets[i].flags & SPW_RX_LEN_MSK);
		TEST_ASSERT_EQUAL(hdr[1], packets[i].buf[0]);
		TEST_ASSERT_EQUAL_HEX8_ARRAY(data, packets[i].buf + hdrSz - 1, dataSz);
	}
#endif

	free(txBuf);
	free(rxBuf);
}


static void test_spwRxTx_1KB(const size_t nPackets, bool async)
{
	TEST_ASSERT_GREATER_THAN(0, nPackets);

	oid_t rxOid = test_getOid(TEST_SPW_PATH0);
	unsigned int firstDesc = test_spwConfigureRx(rxOid, nPackets);

	oid_t txOid = test_getOid(TEST_SPW_PATH1);

#if TEST_SPW_LOOPBACK
	/* Cable Loopback between 1st and 2nd port. Routering will be: AMBA -> 1 -> (loopback) -> 2 -> AMBA */
	static const uint8_t hdr[] = { 0x1, TEST_SPW_ADDR0, /* protocol ID */ 0x5 };
	const size_t consumedHdrBytes = 2;
#else
	static const uint8_t hdr[] = { TEST_SPW_ADDR0, /* protocol ID */ 0x5 };
	const size_t consumedHdrBytes = 1;
#endif

	const size_t hdrSz = sizeof(hdr);
	const size_t remainingHdrSz = hdrSz - consumedHdrBytes;

	const size_t totalDataSz = TEST_DATA_PAYLOAD_1KB;
	uint8_t *data = test_spwAllocPatternBuffer(totalDataSz);

	const size_t baseChunkSz = totalDataSz / nPackets;
	const size_t remainder = totalDataSz % nPackets;
	const size_t maxChunkSz = baseChunkSz + remainder;

	/* Bufor TX: zachowujemy oryginalną formułę (MIN_BUFSZ + hdrSz + maxChunkSz) * nPackets */
	const size_t txBufsz = (SPW_TX_MIN_BUFSZ + hdrSz + maxChunkSz) * nPackets;
	uint8_t *txBuf = malloc(txBufsz);
	TEST_ASSERT_NOT_NULL(txBuf);

	size_t size = 0;
	size_t dataOffset = 0;
	for (size_t i = 0; i < nPackets; i++) {
		const size_t currentChunkSz = baseChunkSz + ((i == nPackets - 1) ? remainder : 0);
		const uint8_t *chunkPtr = data + dataOffset;

		size_t ret = spw_serializeTxMsg(SPW_TX_FLG_HDR_LEN(hdrSz), currentChunkSz, hdr, chunkPtr,
				txBuf + size, txBufsz - size);
		TEST_ASSERT_NOT_EQUAL(0, ret);
		size += ret;
		dataOffset += currentChunkSz;
	}
	test_spwTx(txOid, txBuf, size, nPackets, async);

	/* Bufor RX: identycznie jak w oryginalnym teście */
	const size_t rxBufsz = (SPW_RX_MIN_BUFSZ + hdrSz + maxChunkSz) * nPackets;
	uint8_t *rxBuf = malloc(rxBufsz);
	TEST_ASSERT_NOT_NULL(rxBuf);

	spw_rxPacket_t packets[nPackets];
	test_spwRxRead(rxOid, firstDesc, rxBuf, rxBufsz, packets, nPackets);

	dataOffset = 0;
	for (size_t i = 0; i < nPackets; i++) {
		const size_t currentChunkSz = baseChunkSz + ((i == nPackets - 1) ? remainder : 0);
		const uint8_t *expectedData = data + dataOffset;

#if TEST_SPW_LOOPBACK
		/* two bytes of header (phy addresses) consumed by router */
		TEST_ASSERT_EQUAL(hdrSz - 2 + currentChunkSz, packets[i].flags & SPW_RX_LEN_MSK);
		TEST_ASSERT_EQUAL(hdr[2], packets[i].buf[0]);
		TEST_ASSERT_EQUAL_HEX8_ARRAY(expectedData, packets[i].buf + hdrSz - 2, currentChunkSz);
#else
		/* first byte of header (phy address) consumed by router */
		TEST_ASSERT_EQUAL(hdrSz - 1 + currentChunkSz, packets[i].flags & SPW_RX_LEN_MSK);
		TEST_ASSERT_EQUAL(hdr[1], packets[i].buf[0]);
		TEST_ASSERT_EQUAL_HEX8_ARRAY(expectedData, packets[i].buf + hdrSz - 1, currentChunkSz);
#endif

		dataOffset += currentChunkSz;
	}

	free(data);
	free(txBuf);
	free(rxBuf);
}


static void test_spwRxTx_1MB(const size_t nPackets, bool async)
{
	TEST_ASSERT_GREATER_THAN(0, nPackets);

	oid_t rxOid = test_getOid(TEST_SPW_PATH0);
	unsigned int firstDesc = test_spwConfigureRx(rxOid, nPackets);

	oid_t txOid = test_getOid(TEST_SPW_PATH1);

	const size_t totalDataSz = TEST_DATA_PAYLOAD_1KB * 10;
	uint8_t *data = test_spwAllocPatternBuffer(totalDataSz);

#if TEST_SPW_LOOPBACK
	/* Routing: AMBA -> Port 1 -> (loopback) -> Port 2 -> AMBA */
	static const uint8_t hdr[] = { 0x1, TEST_SPW_ADDR0, /* protocol ID */ 0x5 };
	const size_t consumedHdrBytes = 2;
#else
	static const uint8_t hdr[] = { TEST_SPW_ADDR0, /* protocol ID */ 0x5 };
	const size_t consumedHdrBytes = 1;
#endif

	const size_t hdrSz = sizeof(hdr);
	const size_t remainingHdrSz = hdrSz - consumedHdrBytes;

	const size_t txBufsz = (SPW_TX_MIN_BUFSZ + hdrSz) * nPackets + totalDataSz;
	uint8_t *txBuf = malloc(txBufsz);
	TEST_ASSERT_NOT_NULL(txBuf);

	const size_t baseChunkSz = totalDataSz / nPackets;
	const size_t remainder = totalDataSz % nPackets;

	size_t size = 0;
	size_t dataOffset = 0;
	for (size_t i = 0; i < nPackets; i++) {
		const size_t currentChunkSz = baseChunkSz + ((i == nPackets - 1) ? remainder : 0);
		const uint8_t *chunkPtr = data + dataOffset;

		size_t ret = spw_serializeTxMsg(SPW_TX_FLG_HDR_LEN(hdrSz), currentChunkSz, hdr, chunkPtr,
				txBuf + size, txBufsz - size);
		TEST_ASSERT_NOT_EQUAL(0, ret);
		size += ret;
		dataOffset += currentChunkSz;
	}
	test_spwTx(txOid, txBuf, size, nPackets, async);

	const size_t rxBufsz = (SPW_RX_MIN_BUFSZ + hdrSz) * nPackets + totalDataSz;
	uint8_t *rxBuf = malloc(rxBufsz);
	TEST_ASSERT_NOT_NULL(rxBuf);

	spw_rxPacket_t packets[nPackets];
	test_spwRxRead(rxOid, firstDesc, rxBuf, rxBufsz, packets, nPackets);

	dataOffset = 0;
	for (size_t i = 0; i < nPackets; i++) {
		const size_t currentChunkSz = baseChunkSz + ((i == nPackets - 1) ? remainder : 0);
		const uint8_t *expectedData = data + dataOffset;

		TEST_ASSERT_EQUAL(remainingHdrSz + currentChunkSz, packets[i].flags & SPW_RX_LEN_MSK);
		TEST_ASSERT_EQUAL(hdr[consumedHdrBytes], packets[i].buf[0]);
		TEST_ASSERT_EQUAL_HEX8_ARRAY(expectedData, packets[i].buf + remainingHdrSz, currentChunkSz);

		dataOffset += currentChunkSz;
	}

	free(data);
	free(txBuf);
	free(rxBuf);
}


static void test_spwRxTx_1KB_GrowingPackets(const size_t nPackets, bool async)
{
	TEST_ASSERT_GREATER_THAN(0, nPackets);

	const size_t totalDataSz = TEST_DATA_PAYLOAD_1KB;
	TEST_ASSERT_LESS_OR_EQUAL(totalDataSz, nPackets);

	size_t chunkSizes[nPackets];
	test_spwCalcGrowingChunkSizes(totalDataSz, nPackets, 0.20, chunkSizes);

	oid_t rxOid = test_getOid(TEST_SPW_PATH0);
	unsigned int firstDesc = test_spwConfigureRx(rxOid, nPackets);

	oid_t txOid = test_getOid(TEST_SPW_PATH1);

	uint8_t *data = test_spwAllocPatternBuffer(totalDataSz);

#if TEST_SPW_LOOPBACK
	/* Routing: AMBA -> Port 1 -> (loopback) -> Port 2 -> AMBA */
	static const uint8_t hdr[] = { 0x1, TEST_SPW_ADDR0, /* protocol ID */ 0x5 };
	const size_t consumedHdrBytes = 2;
#else
	static const uint8_t hdr[] = { TEST_SPW_ADDR0, /* protocol ID */ 0x5 };
	const size_t consumedHdrBytes = 1;
#endif

	const size_t hdrSz = sizeof(hdr);
	const size_t remainingHdrSz = hdrSz - consumedHdrBytes;

	const size_t txBufsz = (SPW_TX_MIN_BUFSZ + hdrSz) * nPackets + totalDataSz;
	uint8_t *txBuf = malloc(txBufsz);
	TEST_ASSERT_NOT_NULL(txBuf);

	size_t size = 0;
	size_t dataOffset = 0;
	for (size_t i = 0; i < nPackets; i++) {
		const size_t currentChunkSz = chunkSizes[i];
		const uint8_t *chunkPtr = data + dataOffset;

		size_t ret = spw_serializeTxMsg(SPW_TX_FLG_HDR_LEN(hdrSz), currentChunkSz, hdr, chunkPtr,
				txBuf + size, txBufsz - size);
		TEST_ASSERT_NOT_EQUAL(0, ret);
		size += ret;
		dataOffset += currentChunkSz;
	}
	test_spwTx(txOid, txBuf, size, nPackets, async);

	const size_t rxBufsz = (SPW_RX_MIN_BUFSZ + hdrSz) * nPackets + totalDataSz;
	uint8_t *rxBuf = malloc(rxBufsz);
	TEST_ASSERT_NOT_NULL(rxBuf);

	spw_rxPacket_t packets[nPackets];
	test_spwRxRead(rxOid, firstDesc, rxBuf, rxBufsz, packets, nPackets);

	dataOffset = 0;
	for (size_t i = 0; i < nPackets; i++) {
		const size_t currentChunkSz = chunkSizes[i];
		const uint8_t *expectedData = data + dataOffset;

		TEST_ASSERT_EQUAL(remainingHdrSz + currentChunkSz, packets[i].flags & SPW_RX_LEN_MSK);
		TEST_ASSERT_EQUAL(hdr[consumedHdrBytes], packets[i].buf[0]);
		TEST_ASSERT_EQUAL_HEX8_ARRAY(expectedData, packets[i].buf + remainingHdrSz, currentChunkSz);

		dataOffset += currentChunkSz;
	}

	free(data);
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
	spw_i_t *idevctl = (spw_i_t *)msg.i.raw;
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
	TEST_ASSERT_LESS_OR_EQUAL_INT(timeout + TEST_RX_TIMEOUT_THRESHOLD, diff);

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
	spw_i_t *idevctl = (spw_i_t *)msg.i.raw;
	spw_o_t *odevctl = (spw_o_t *)msg.o.raw;

	idevctl->type = spw_rx;
	idevctl->task.rx.nPackets = nPackets;
	idevctl->task.rx.firstDesc = firstDesc;
	idevctl->task.rx.timeoutUs = timeout;

	TEST_ASSERT_EQUAL_INT(0, msgSend(rxOid.port, &msg));
	TEST_ASSERT_EQUAL_INT(-ETIME, msg.o.err);

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

#if TEST_SPW_LOOPBACK
	/* Cable Loopback between 1st and 2nd port. Routering will be: AMBA -> 1 -> (external loopback) -> 2 -> AMBA */
	static const uint8_t hdr[] = { 0x1, TEST_SPW_ADDR0, /* protocol ID */ 0x5 }, data[] = { 0x1, 0x2, 0x3, 0x4 };
#else
	static const uint8_t hdr[] = { TEST_SPW_ADDR0, /* protocol ID */ 0x5 }, data[] = { 0x1, 0x2, 0x3, 0x4 };
#endif

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
		/* two bytes of header (phy addresses) consumed by router */
		TEST_ASSERT_EQUAL(hdrSz - 2 + dataSz, packets[i].flags & SPW_RX_LEN_MSK);
		TEST_ASSERT_EQUAL(hdr[2], packets[i].buf[0]);
		TEST_ASSERT_EQUAL_HEX8_ARRAY(data, packets[i].buf + hdrSz - 2, dataSz);
	}
#else
	for (size_t i = 0; i < nSent; i++) {
		/* first byte of header (phy address) consumed by router */
		TEST_ASSERT_EQUAL((hdrSz - 1 + dataSz), packets[i].flags & SPW_RX_LEN_MSK);
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
	spw_i_t *ispwctl = (spw_i_t *)msg.i.raw;

	ispwctl->type = spw_config;
	ispwctl->task.config.node.addr = TEST_SPW_ADDR0;
	ispwctl->task.config.node.mask = 0xff;
	ispwctl->task.config.dma.addr = TEST_SPW_ADDR0;
	ispwctl->task.config.dma.mask = 0xff;

	TEST_ASSERT_EQUAL_INT(0, msgSend(oid.port, &msg));
	TEST_ASSERT_EQUAL_INT(0, msg.o.err);

	oid = test_getOid(TEST_SPW_PATH1);
	msg.oid.id = TEST_SPW_ID1;
	msg.oid.port = oid.port;
	ispwctl->type = spw_config;
	ispwctl->task.config.node.addr = TEST_SPW_ADDR1;
	ispwctl->task.config.node.mask = 0xff;
	ispwctl->task.config.dma.addr = TEST_SPW_ADDR1;
	ispwctl->task.config.dma.mask = 0xff;

	TEST_ASSERT_EQUAL_INT(0, msgSend(oid.port, &msg));
	TEST_ASSERT_EQUAL_INT(0, msg.o.err);
}


TEST(test_spw, spwSetWrongPort)
{
	oid_t oid = test_getOid(TEST_SPW_PATH0);
	msg_t msg = {
		.type = mtDevCtl,
		.i = { .data = NULL, .size = 0 },
		.o = { .data = NULL, .size = 0 },
		.oid.id = 5,
		.oid.port = oid.port,
	};
	spw_i_t *ispwctl = (spw_i_t *)msg.i.raw;

	ispwctl->type = spw_config;
	ispwctl->task.config.node.addr = 0x22;
	ispwctl->task.config.node.mask = 0xff;
	ispwctl->task.config.dma.addr = 0x22;
	ispwctl->task.config.dma.mask = 0xff;

	TEST_ASSERT_EQUAL_INT(0, msgSend(oid.port, &msg));
	TEST_ASSERT_NOT_EQUAL_INT(0, msg.o.err);

	oid = test_getOid(TEST_SPW_PATH1);
	msg.oid.id = 7;
	ispwctl->type = spw_config;
	ispwctl->task.config.node.addr = 0x55;
	ispwctl->task.config.node.mask = 0xff;
	ispwctl->task.config.dma.addr = 0x55;
	ispwctl->task.config.dma.mask = 0xff;

	TEST_ASSERT_EQUAL_INT(0, msgSend(oid.port, &msg));
	TEST_ASSERT_NOT_EQUAL_INT(0, msg.o.err);
}


TEST(test_spw, spwTxRxInvalidAddressDrop)
{
	oid_t rxOid = test_getOid(TEST_SPW_PATH0);
	oid_t txOid = test_getOid(TEST_SPW_PATH1);

	const size_t nPackets = 1;
	unsigned int firstDesc = test_spwConfigureRx(rxOid, nPackets);

	uint8_t invalidAddr = (TEST_SPW_ADDR0 == 0xEE) ? 0xEF : 0xEE;
	static const uint8_t data[] = { 0xDE, 0xAD, 0xBE, 0xEF };

#if TEST_SPW_LOOPBACK
	const uint8_t hdr[] = { 0x1, invalidAddr, 0x5 };
#else
	const uint8_t hdr[] = { invalidAddr, 0x5 };
#endif

	const size_t hdrSz = sizeof(hdr);
	const size_t dataSz = sizeof(data);
	const size_t txBufsz = SPW_TX_MIN_BUFSZ + hdrSz + dataSz;

	uint8_t *txBuf = malloc(txBufsz);
	TEST_ASSERT_NOT_NULL(txBuf);

	size_t size = spw_serializeTxMsg(SPW_TX_FLG_HDR_LEN(hdrSz), dataSz, hdr, data, txBuf, txBufsz);
	TEST_ASSERT_NOT_EQUAL(0, size);

	test_spwTx(txOid, txBuf, size, nPackets, false);

	const size_t rxBufsz = SPW_RX_MIN_BUFSZ + hdrSz + dataSz;
	uint8_t *rxBuf = malloc(rxBufsz);
	TEST_ASSERT_NOT_NULL(rxBuf);

	spw_rxPacket_t packet;
	size_t receivedCount = test_spwRxReadTimeout(rxOid, firstDesc, rxBuf, rxBufsz, &packet, nPackets, TEST_RX_TIMEOUT);

	TEST_ASSERT_EQUAL_INT(0, receivedCount);

	free(txBuf);
	free(rxBuf);
}


TEST(test_spw, spwTxRxDataPatterns)
{
	oid_t rxOid = test_getOid(TEST_SPW_PATH0);
	oid_t txOid = test_getOid(TEST_SPW_PATH1);

	static const uint8_t patternAllZeros[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	static const uint8_t patternAllOnes[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
	static const uint8_t patternAltBits1[] = { 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55 };
	static const uint8_t patternAltBits2[] = { 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA };

	const uint8_t *patterns[] = { patternAllZeros, patternAllOnes, patternAltBits1, patternAltBits2 };
	const size_t patternCount = sizeof(patterns) / sizeof(patterns[0]);
	const size_t dataSz = sizeof(patternAllZeros);

#if TEST_SPW_LOOPBACK
	static const uint8_t hdr[] = { 0x1, TEST_SPW_ADDR0, 0x5 };
	const size_t consumedBytes = 2;
#else
	static const uint8_t hdr[] = { TEST_SPW_ADDR0, 0x5 };
	const size_t consumedBytes = 1;
#endif

	const size_t hdrSz = sizeof(hdr);

	for (size_t p = 0; p < patternCount; p++) {
		unsigned int firstDesc = test_spwConfigureRx(rxOid, 1);

		const size_t txBufsz = SPW_TX_MIN_BUFSZ + hdrSz + dataSz;
		uint8_t *txBuf = malloc(txBufsz);
		TEST_ASSERT_NOT_NULL(txBuf);

		size_t size = spw_serializeTxMsg(SPW_TX_FLG_HDR_LEN(hdrSz), dataSz, hdr, patterns[p], txBuf, txBufsz);
		TEST_ASSERT_NOT_EQUAL(0, size);

		test_spwTx(txOid, txBuf, size, 1, false);

		const size_t rxBufsz = SPW_RX_MIN_BUFSZ + hdrSz + dataSz;
		uint8_t *rxBuf = malloc(rxBufsz);
		TEST_ASSERT_NOT_NULL(rxBuf);

		spw_rxPacket_t packet;
		test_spwRxRead(rxOid, firstDesc, rxBuf, rxBufsz, &packet, 1);

		TEST_ASSERT_EQUAL(hdrSz - consumedBytes + dataSz, packet.flags & SPW_RX_LEN_MSK);
		TEST_ASSERT_EQUAL_HEX8_ARRAY(patterns[p], packet.buf + hdrSz - consumedBytes, dataSz);

		free(txBuf);
		free(rxBuf);
	}
}


TEST(test_spw, spwTxRxZeroLengthPayload)
{
	const size_t nPackets = 2;
	oid_t rxOid = test_getOid(TEST_SPW_PATH0);
	// unsigned int firstDesc = test_spwConfigureRx(rxOid, nPackets);

	oid_t txOid = test_getOid(TEST_SPW_PATH1);

#if TEST_SPW_LOOPBACK
	/* Cable Loopback between 1st and 2nd port. Routering will be: AMBA -> 1 -> (loopback) -> 2 -> AMBA */
	static const uint8_t hdr[] = { 0x1, TEST_SPW_ADDR0, /* protocol ID */ 0x5 };
#else
	static const uint8_t hdr[] = { TEST_SPW_ADDR0, /* protocol ID */ 0x5 };
#endif

	static const size_t hdrSz = sizeof(hdr), dataSz = 0;
	const size_t txBufsz = (SPW_TX_MIN_BUFSZ + hdrSz + dataSz) * nPackets;
	uint8_t *txBuf = malloc(txBufsz);
	TEST_ASSERT_NOT_NULL(txBuf);

	size_t size = 0;
	for (size_t i = 0; i < nPackets; i++) {
		size_t ret = spw_serializeTxMsg(SPW_TX_FLG_HDR_LEN(hdrSz), dataSz, hdr, NULL, txBuf + size, txBufsz - size);
		TEST_ASSERT_NOT_EQUAL(0, ret);
		size += ret;
	}
	test_spwTx(txOid, txBuf, size, nPackets, 0);

	free(txBuf);
}


TEST(test_spw, spwConfigureRxPacketsOffLimit)
{
	const size_t nPackets = SPW_RX_DESC_CNT + 1;
	oid_t rxOid = test_getOid(TEST_SPW_PATH0);
	test_spwConfigureRxOffLimits(rxOid);
}


TEST(test_spw, spwRxTimeout)
{
	test_spwRxTimeout(TEST_RX_TIMEOUT);
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
	test_spwRxTxTimeout(128, 20, TEST_RX_TIMEOUT);
}


TEST(test_spw, spwTxRxMultiplePacketsSync_1KB)
{
	test_spwRxTx_1KB(10, false);
}


TEST(test_spw, spwTxRxMultiplePacketsAsync_1KB)
{
	test_spwRxTx_1KB(10, true);
}

TEST(test_spw, spwTxRxBigNumberOfPacketsSync_1KB)
{
	/* Sync TX number of packets must not be bigger than SPW_TX_DESC_CNT */
	test_spwRxTx_1KB(64, false);
}


TEST(test_spw, spwTxRxBigNumberOfPacketsAsync_1KB)
{
	/* Async TX number of packets must not be bigger than SPW_RX_DESC_CNT */
	test_spwRxTx_1KB(128, true);
}


TEST(test_spw, spwTxRxMultiplePacketsSync_1MB)
{
	test_spwRxTx_1MB(24, false);
}


TEST(test_spw, spwTxRxMultiplePacketsAsync_1MB)
{
	test_spwRxTx_1MB(24, true);
}

TEST(test_spw, spwTxRxBigNumberOfPacketsSync_1MB)
{
	/* Sync TX number of packets must not be bigger than SPW_TX_DESC_CNT */
	test_spwRxTx_1MB(64, false);
}


TEST(test_spw, spwTxRxBigNumberOfPacketsAsync_1MB)
{
	/* Async TX number of packets must not be bigger than SPW_RX_DESC_CNT */
	test_spwRxTx_1MB(128, true);
}


TEST(test_spw, spwTxRxGrowingPacketsSync_1KB)
{
	test_spwRxTx_1KB_GrowingPackets(4, false);
}


TEST(test_spw, spwTxRxGrowingPacketsAsync_1KB)
{
	test_spwRxTx_1KB_GrowingPackets(4, true);
}


TEST(test_spw, spwTxRxManyGrowingPacketsSync_1KB)
{
	test_spwRxTx_1KB_GrowingPackets(16, false);
}


TEST(test_spw, spwTxRxManyGrowingPacketsAsync_1KB)
{
	test_spwRxTx_1KB_GrowingPackets(16, true);
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

	RUN_TEST_CASE(test_spw, spwConfigureRxPacketsOffLimit);
	RUN_TEST_CASE(test_spw, spwTxRxZeroLengthPayload);
	RUN_TEST_CASE(test_spw, spwTxRxDataPatterns);
	RUN_TEST_CASE(test_spw, spwTxRxInvalidAddressDrop);
	RUN_TEST_CASE(test_spw, spwSetWrongPort);

	RUN_TEST_CASE(test_spw, spwTxRxMultiplePacketsSync_1KB);
	RUN_TEST_CASE(test_spw, spwTxRxMultiplePacketsAsync_1KB);
	RUN_TEST_CASE(test_spw, spwTxRxBigNumberOfPacketsSync_1KB);
	RUN_TEST_CASE(test_spw, spwTxRxBigNumberOfPacketsAsync_1KB);

	RUN_TEST_CASE(test_spw, spwTxRxMultiplePacketsSync_1MB);
	RUN_TEST_CASE(test_spw, spwTxRxMultiplePacketsAsync_1MB);
	RUN_TEST_CASE(test_spw, spwTxRxBigNumberOfPacketsSync_1MB);
	RUN_TEST_CASE(test_spw, spwTxRxBigNumberOfPacketsAsync_1MB);

	RUN_TEST_CASE(test_spw, spwTxRxGrowingPacketsSync_1KB);
	RUN_TEST_CASE(test_spw, spwTxRxGrowingPacketsAsync_1KB);
	RUN_TEST_CASE(test_spw, spwTxRxManyGrowingPacketsSync_1KB);
	RUN_TEST_CASE(test_spw, spwTxRxManyGrowingPacketsAsync_1KB);
}


void runner(void)
{
	RUN_TEST_GROUP(test_spw);
}


int main(int argc, char *argv[])
{
	return (UnityMain(argc, (const char **)argv, runner) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
