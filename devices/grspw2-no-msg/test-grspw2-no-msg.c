/*
 * Phoenix-RTOS
 *
 * GRSPW2 driver tests
 *
 * Copyright 2026 Phoenix Systems
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
#include <libgrspw.h>
#include <unity_fixture.h>
#include <time.h>

#include <stdint.h>
#include <stddef.h>

#include <trace.h>
#include <sys/stat.h>
#include <inttypes.h>

#include <sys/mman.h>
#include <sys/threads.h>
#include <stdatomic.h>


#define TEST_SPW_ID0 0
#define TEST_SPW_ID1 1

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

#define LINK_FREQUENCY 50

#define STR_HELPER(x) #x
#define STR(x)        STR_HELPER(x)

#define TRACE_ENABLE 1
#define MEASURE_TIME 1
#define DATA_CHECK 1


/* helper functions */

int dev0 = TEST_SPW_ID0;
int dev1 = TEST_SPW_ID1;

spw_dev_t *spw0;
spw_dev_t *spw1;


extern uintptr_t va2pa(void *va);


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


static inline uint64_t get_time_us(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}


static inline unsigned int test_spwConfigureRx(const size_t nPackets)
{
    _Alignas(size_t) unsigned char raw[64] = { 0 };
    spw_o_t *odevctl = (spw_o_t *)raw;

    int res = spw_rxConfigure(spw0, (size_t *)odevctl, nPackets);
    if (res < 0) {
        TEST_FAIL_MESSAGE("spw_rxConfigure failed");
        exit(EXIT_FAILURE);
    }

    return odevctl->val;
}


static inline void test_spwTx(uint8_t *txBuf, const size_t txBufsz, const size_t nPackets, bool async)
{
    const spw_tx_t task_tx = {
	.nPackets = nPackets,
	.async = async
    };

    int res = spw_transmit(spw1, txBuf, txBufsz, &task_tx);
    if (res < 0) {
        TEST_FAIL_MESSAGE("spw_transmit returned error");
        exit(EXIT_FAILURE);
    }
}


static inline void test_spwRxRead(const unsigned int firstDesc, uint8_t *rxBuf, size_t rxBufsz, spw_rxPacket_t *packets, const size_t nPackets)
{
    const spw_rx_t task_rx = {
        .firstDesc = firstDesc,
        .nPackets = nPackets,
        .timeoutUs = TEST_RX_TIMEOUT
    };

    _Alignas(size_t) unsigned char raw[64] = { 0 };
    spw_o_t *odevctl = (spw_o_t *)raw;

    int res = spw_rxRead(spw0, rxBuf, rxBufsz, (size_t *)odevctl, &task_rx);
    if (res != 0) {
        TEST_FAIL_MESSAGE("spw_rxRead returned error");
        exit(EXIT_FAILURE);
    }

    if (odevctl->val != nPackets) {
        TEST_FAIL_MESSAGE("Received packets count mismatch");
        exit(EXIT_FAILURE);
    }

    for (size_t i = 0; i < nPackets; i++) {
        size_t consumed = spw_deserializeRxMsg(rxBuf, &packets[i]);
        if (consumed == 0) {
            TEST_FAIL_MESSAGE("spw_deserializeRxMsg failed to parse packet");
            exit(EXIT_FAILURE);
        }
        rxBuf += consumed;
    }
}


static void test_spwRxTx_1MB(const size_t nPackets, bool async)
{
    TEST_ASSERT_GREATER_THAN(0, nPackets);

    const size_t totalDataSz = TEST_DATA_PAYLOAD_1MB;
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

    // const size_t TRANSMISSION_SZ = 10 * 1024;
    const size_t TRANSMISSION_SZ = 16 * 4096; // 16 * 4kB

    const size_t txBufsz = (SPW_TX_MIN_BUFSZ + hdrSz) * nPackets + TRANSMISSION_SZ;
    uint8_t *txBuf = malloc(txBufsz);
    TEST_ASSERT_NOT_NULL(txBuf);

    const size_t rxBufsz = (SPW_RX_MIN_BUFSZ + hdrSz) * nPackets + TRANSMISSION_SZ;
    uint8_t *rxBuf = malloc(rxBufsz);
    TEST_ASSERT_NOT_NULL(rxBuf);

    spw_rxPacket_t packets[nPackets];

    uint64_t txTimeUs = 0;
    uint64_t rxTimeUs = 0;
    uint64_t tStart = get_time_us();

    for (size_t sentBytes = 0; sentBytes < totalDataSz; sentBytes += TRANSMISSION_SZ) {
        const size_t currentTransmissionSz = (sentBytes + TRANSMISSION_SZ <= totalDataSz) ? TRANSMISSION_SZ : (totalDataSz - sentBytes);

        #if defined (TRACE_ENABLE) && (TRACE_ENABLE == 1)
            bool doTrace = (sentBytes == 0) ? 1 : 0; 
            trace_ctx_t traceCtx;

            if (doTrace) {
                trace_init(&traceCtx, true);
                trace_start(&traceCtx);
            }
        #endif /* TRACE_ENABLE */
        
        unsigned int firstDesc = test_spwConfigureRx(nPackets);

        const size_t baseChunkSz = currentTransmissionSz / nPackets;
        const size_t remainder = currentTransmissionSz % nPackets;

        size_t txPayloadLen = 0;
        size_t chunkOffset = 0;

        for (size_t i = 0; i < nPackets; i++) {
            const size_t currentChunkSz = baseChunkSz + ((i == nPackets - 1) ? remainder : 0);
            const uint8_t *chunkPtr = data + sentBytes + chunkOffset;

            size_t ret = spw_serializeTxMsg(SPW_TX_FLG_HDR_LEN(hdrSz), currentChunkSz, hdr, chunkPtr,
                    txBuf + txPayloadLen, txBufsz - txPayloadLen);
            TEST_ASSERT_NOT_EQUAL(0, ret);

            txPayloadLen += ret;
            chunkOffset += currentChunkSz;
        }

        #if defined (MEASURE_TIME) && (MEASURE_TIME == 1)
            uint64_t tTxStart = get_time_us();
            test_spwTx(txBuf, txPayloadLen, nPackets, async);
            uint64_t tTxEnd = get_time_us();
            txTimeUs += (tTxEnd - tTxStart);

            uint64_t tRxStart = get_time_us();
            test_spwRxRead(firstDesc, rxBuf, rxBufsz, packets, nPackets);
            uint64_t tRxEnd = get_time_us();
            rxTimeUs += (tRxEnd - tRxStart);
        #else
            test_spwTx(txBuf, txPayloadLen, nPackets, async);
            test_spwRxRead(firstDesc, rxBuf, rxBufsz, packets, nPackets);
        #endif /* MEASURE_TIME */

        #if defined (TRACE_ENABLE) && (TRACE_ENABLE == 1)
			if (doTrace) {
				trace_stopAndGather(&traceCtx, 65536, "/var/trace_spw");

				static const char * const traceFiles[] = {
					"/var/trace_spw/channel_meta0",
					"/var/trace_spw/channel_meta1",
					"/var/trace_spw/channel_event0",
					"/var/trace_spw/channel_event1"
				};

				for (size_t fi = 0; fi < sizeof(traceFiles) / sizeof(traceFiles[0]); fi++) {
					FILE *f = fopen(traceFiles[fi], "rb");
					if (f == NULL) {
						continue;
					}

					fseek(f, 0, SEEK_END);
					long sz = ftell(f);
					fseek(f, 0, SEEK_SET);

					if (sz > 0) {
						void *dumpBuf = malloc(sz);
						if (dumpBuf != NULL) {
							fread(dumpBuf, 1, sz, f);
							uintptr_t paddr = va2pa(dumpBuf);
							printf("\n[GRMON DUMP READY] %s | addr: 0x%" PRIxPTR " | size: %ld\n",
								traceFiles[fi], paddr, sz);
						}
					}
					fclose(f);
				}
			}
        #endif /* TRACE_ENABLE */

        #if defined (DATA_CHECK) && (DATA_CHECK == 1)
            chunkOffset = 0;
            for (size_t i = 0; i < nPackets; i++) {
                const size_t currentChunkSz = baseChunkSz + ((i == nPackets - 1) ? remainder : 0);
                const uint8_t *expectedData = data + sentBytes + chunkOffset;

                TEST_ASSERT_EQUAL(remainingHdrSz + currentChunkSz, packets[i].flags & SPW_RX_LEN_MSK);
                TEST_ASSERT_EQUAL(hdr[consumedHdrBytes], packets[i].buf[0]);
                TEST_ASSERT_EQUAL_HEX8_ARRAY(expectedData, packets[i].buf + remainingHdrSz, currentChunkSz);

                chunkOffset += currentChunkSz;
            }
        #endif /* DATA_CHECK */
    }

    uint64_t tEnd = get_time_us();
    uint64_t totalTimeUs = tEnd - tStart;

    char msg[160];

    /*  */
    if (totalTimeUs > 0) {
        double totalSec = (double)totalTimeUs / 1000000.0;
        double totalMBs = ((double)totalDataSz / (1024.0 * 1024.0)) / totalSec;
        double totalMbits = ((double)totalDataSz * 8.0 / 1000000.0) / totalSec;

        snprintf(msg, sizeof(msg),
                "[SpaceWire Total Timing] Start: %llu us | End: %llu us",
                (unsigned long long)tStart, (unsigned long long)tEnd);
        UNITY_PRINT_EOL();
        UnityPrint(msg);

        snprintf(msg, sizeof(msg),
                "[SpaceWire Total Rate] Time: %llu us (%.3f s) | %.2f MiB/s | %.2f Mbit/s",
                (unsigned long long)totalTimeUs, totalSec, totalMBs, totalMbits);
        UNITY_PRINT_EOL();
        UnityPrint(msg);
        UNITY_PRINT_EOL();
    }

    #if defined (MEASURE_TIME) && (MEASURE_TIME == 1)
        uint64_t pureTransferTimeUs = txTimeUs + rxTimeUs;
        /* (TX + RX) */
        if (pureTransferTimeUs > 0) {
            double pureSec = (double)pureTransferTimeUs / 1000000.0;
            double pureMBs = ((double)totalDataSz / (1024.0 * 1024.0)) / pureSec;
            double pureMbits = ((double)totalDataSz * 8.0 / 1000000.0) / pureSec;

            snprintf(msg, sizeof(msg),
                    "[SpaceWire Pure Transfer] Time: %llu us (%.3f s) | %.2f MiB/s | %.2f Mbit/s",
                    (unsigned long long)pureTransferTimeUs, pureSec, pureMBs, pureMbits);
            UNITY_PRINT_EOL();
            UnityPrint(msg);
        }

        /*  TX */
        if (txTimeUs > 0) {
            double txSec = (double)txTimeUs / 1000000.0;
            double txMBs = ((double)totalDataSz / (1024.0 * 1024.0)) / txSec;
            double txMbits = ((double)totalDataSz * 8.0 / 1000000.0) / txSec;

            snprintf(msg, sizeof(msg),
                    "[SpaceWire TX Only] Time: %llu us (%.3f s) | %.2f MiB/s | %.2f Mbit/s",
                    (unsigned long long)txTimeUs, txSec, txMBs, txMbits);
            UNITY_PRINT_EOL();
            UnityPrint(msg);
        }

        /* RX */
        if (rxTimeUs > 0) {
            double rxSec = (double)rxTimeUs / 1000000.0;
            double rxMBs = ((double)totalDataSz / (1024.0 * 1024.0)) / rxSec;
            double rxMbits = ((double)totalDataSz * 8.0 / 1000000.0) / rxSec;

            snprintf(msg, sizeof(msg),
                    "[SpaceWire RX Only] Time: %llu us (%.3f s) | %.2f MiB/s | %.2f Mbit/s",
                    (unsigned long long)rxTimeUs, rxSec, rxMBs, rxMbits);
            UNITY_PRINT_EOL();
            UnityPrint(msg);
            UNITY_PRINT_EOL();
        }
    #endif /* MEASURE_TIME */

    free(data);
    free(txBuf);
    free(rxBuf);

    spw_dumpTrace();
}


/* MULTI THREAD */

typedef struct {
	handle_t lock;
	handle_t startCond;
	int readyCnt;
	bool go;
	size_t configuredBatches;

	/* TX */
	uint8_t *txBuf;
	size_t *txBatchLen;
	size_t batchPackets;
	size_t totalBatches;
	bool async;

	uint64_t txDurationUs;
	int txErr;

	/* RX */
	uint8_t *rxBuf;
	size_t *rxBatchLen;
	spw_rxPacket_t *packets;

	uint64_t rxDurationUs;
	int rxErr;

	/* debugging variables */
	volatile int txDone, rxDone;
	volatile size_t txBatch, RxBatch;
	volatile int txStage, RxStage;
} spw_streamCtx_t;


/* Local GRSPW2 register offsets & TX descriptor bits for diagnostics */
#define TEST_SPW_STATUS    1
#define TEST_DMA_CTRL      8
#define TEST_DMA_TX_DESC   10

#define TEST_TX_DESC_LERR  (1U << 15)
#define TEST_TX_DESC_IE    (1U << 14)
#define TEST_TX_DESC_WR    (1U << 13)
#define TEST_TX_DESC_EN    (1U << 12)


static void spw_txDump(spw_dev_t *tx)
{
	if (tx == NULL) {
		return;
	}

	uint32_t dmaCtrl = tx->vbase[TEST_DMA_CTRL];
	uint32_t spwStatus = tx->vbase[TEST_SPW_STATUS];
	uint32_t dmaTxDescPa = tx->vbase[TEST_DMA_TX_DESC];

	uintptr_t basePa = va2pa((void *)tx->txDesc);
	size_t hwIdx = SPW_TX_DESC_CNT;

	if ((dmaTxDescPa >= (uint32_t)basePa) &&
			(dmaTxDescPa < (uint32_t)(basePa + SPW_TX_DESC_CNT * sizeof(spw_txDesc_t)))) {
		hwIdx = (dmaTxDescPa - (uint32_t)basePa) / sizeof(spw_txDesc_t);
	}

	printf("\n[SPW TX DUMP] DMA_CTRL=0x%08" PRIx32 " | SPW_STATUS=0x%08" PRIx32 " | DMA_TX_DESC=0x%08" PRIx32 " (base=0x%08" PRIx32 ", hw_idx=",
			dmaCtrl, spwStatus, dmaTxDescPa, (uint32_t)basePa);
	if (hwIdx < SPW_TX_DESC_CNT) {
		printf("%zu)\n", hwIdx);
	}
	else {
		printf("invalid)\n");
	}

	for (size_t i = 0; i < SPW_TX_DESC_CNT; i++) {
		uint32_t ctrl = tx->txDesc[i].ctrl;
		if ((ctrl & (TEST_TX_DESC_EN | TEST_TX_DESC_LERR)) != 0) {
			printf("  txDesc[%zu]%s: ctrl=0x%08" PRIx32 " (EN=%u, LERR=%u, WR=%u, IE=%u) len=%" PRIu32 "\n",
					i,
					(i == hwIdx) ? " <-- HW" : "",
					ctrl,
					(ctrl & TEST_TX_DESC_EN) ? 1U : 0U,
					(ctrl & TEST_TX_DESC_LERR) ? 1U : 0U,
					(ctrl & TEST_TX_DESC_WR) ? 1U : 0U,
					(ctrl & TEST_TX_DESC_IE) ? 1U : 0U,
					tx->txDesc[i].packetLen);
		}
	}
}


static void spw_streamDiagDump(spw_streamCtx_t *ctx, spw_dev_t *tx)
{
	printf("\n[SPW STREAM DIAG] txDone=%d (batch=%zu/%zu, stage=%d, err=%d) | rxDone=%d (batch=%zu/%zu, stage=%d, err=%d)\n",
			ctx->txDone, ctx->txBatch, ctx->totalBatches, ctx->txStage, ctx->txErr,
			ctx->rxDone, ctx->RxBatch, ctx->totalBatches, ctx->RxStage, ctx->rxErr);
	fflush(stdout);
	spw_txDump(tx);
	fflush(stdout);
}


static void spw_txStreamWorker(void *arg)
{
	spw_streamCtx_t *ctx = (spw_streamCtx_t *)arg;

	mutexLock(ctx->lock);
	ctx->readyCnt++;
	condBroadcast(ctx->startCond);
	while (!ctx->go) {
		condWait(ctx->startCond, ctx->lock, 0);
	}
	mutexUnlock(ctx->lock);

	uint64_t t0 = get_time_us();
	uint8_t *ptr = ctx->txBuf;

	for (size_t b = 0; b < ctx->totalBatches; b++) {
		ctx->txBatch = b;
		ctx->txStage = 0;

		mutexLock(ctx->lock);
		while ((ctx->configuredBatches <= b) && (ctx->rxErr == 0)) {
			condWait(ctx->startCond, ctx->lock, 0);
		}
		int rxErr = ctx->rxErr;
		mutexUnlock(ctx->lock);

		if (rxErr != 0) {
			break;
		}

		const spw_tx_t task_tx = {
			.nPackets = ctx->batchPackets,
			.async = ctx->async
		};

		ctx->txStage = 1;
		int res = spw_transmit(spw1, ptr, ctx->txBatchLen[b], &task_tx);
		if (res < 0) {
			mutexLock(ctx->lock);
			ctx->txErr = res;
			condBroadcast(ctx->startCond);
			mutexUnlock(ctx->lock);
			break;
		}

		ctx->txStage = 2;
		ptr += ctx->txBatchLen[b];
	}

	ctx->txDurationUs = get_time_us() - t0;
	ctx->txDone = 1;
	endthread();
}


static void spw_rxStreamWorker(void *arg)
{
	spw_streamCtx_t *ctx = (spw_streamCtx_t *)arg;

	_Alignas(size_t) unsigned char raw[64] = { 0 };
	spw_o_t *odevctl = (spw_o_t *)raw;

	if (spw_rxConfigure(spw0, (size_t *)odevctl, ctx->batchPackets) < 0) {
		ctx->rxErr = -1;
		spw_streamDiagDump(ctx, spw1);
		endthread();
	}
	unsigned int firstDesc = odevctl->val;

	mutexLock(ctx->lock);
	ctx->configuredBatches = 1;
	ctx->readyCnt++;
	condBroadcast(ctx->startCond);
	while (!ctx->go) {
		condWait(ctx->startCond, ctx->lock, 0);
	}
	mutexUnlock(ctx->lock);

	uint64_t t0 = get_time_us();
	uint8_t *rxPtr = ctx->rxBuf;
	spw_rxPacket_t *pktPtr = ctx->packets;

	for (size_t b = 0; b < ctx->totalBatches; b++) {
		ctx->RxBatch = b;

		const spw_rx_t task_rx = {
			.firstDesc = firstDesc,
			.nPackets = ctx->batchPackets,
			.timeoutUs = TEST_RX_TIMEOUT
		};
		ctx->RxStage = 1;
		int res = spw_rxRead(spw0, rxPtr, ctx->rxBatchLen[b], (size_t *)odevctl, &task_rx);
		if ((res != 0) || (odevctl->val != ctx->batchPackets)) {
			mutexLock(ctx->lock);
			ctx->rxErr = (res != 0) ? res : -3;
			condBroadcast(ctx->startCond);
			mutexUnlock(ctx->lock);
			spw_streamDiagDump(ctx, spw1);
			break;
		}

		/* configure next batch of descriptors and enable TX */
		unsigned int nextDesc = 0;
		ctx->RxStage = 2;
		if (b + 1 < ctx->totalBatches) {
			if (spw_rxConfigure(spw0, (size_t *)odevctl, ctx->batchPackets) < 0) {
				mutexLock(ctx->lock);
				ctx->rxErr = -2;
				condBroadcast(ctx->startCond);
				mutexUnlock(ctx->lock);
				spw_streamDiagDump(ctx, spw1);
				break;
			}
			nextDesc = odevctl->val;

			mutexLock(ctx->lock);
			ctx->configuredBatches++;
			condBroadcast(ctx->startCond);
			mutexUnlock(ctx->lock);
		}

		/* deserialization of current batch in parallel with transfer of the next batch */
		ctx->RxStage = 3;
		uint8_t *parsePtr = rxPtr;
		for (size_t i = 0; i < ctx->batchPackets; i++) {
			size_t consumed = spw_deserializeRxMsg(parsePtr, &pktPtr[i]);
			if (consumed == 0) {
				mutexLock(ctx->lock);
				ctx->rxErr = -4;
				condBroadcast(ctx->startCond);
				mutexUnlock(ctx->lock);
				spw_streamDiagDump(ctx, spw1);
				break;
			}
			parsePtr += consumed;
		}
		if (ctx->rxErr != 0) {
			break;
		}

		firstDesc = nextDesc;
		rxPtr += ctx->rxBatchLen[b];
		pktPtr += ctx->batchPackets;
	}

	ctx->rxDurationUs = get_time_us() - t0;
	ctx->rxDone = 1;
	endthread();
}

static void test_spwRxTxStream_1MB(const size_t nPackets, bool async)
{
	TEST_ASSERT_GREATER_THAN(0, nPackets);
	TEST_ASSERT_LESS_OR_EQUAL(SPW_RX_DESC_CNT / 2, nPackets);

	const size_t totalDataSz = TEST_DATA_PAYLOAD_1MB;
	const size_t TRANSMISSION_SZ = 10 * 1024; // 16 * 4096;
	const size_t totalBatches = (totalDataSz + TRANSMISSION_SZ - 1) / TRANSMISSION_SZ;
	const size_t totalPackets = totalBatches * nPackets;

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

	uint8_t *data = test_spwAllocPatternBuffer(totalDataSz);

	const size_t txBufsz = (SPW_TX_MIN_BUFSZ + hdrSz) * totalPackets + totalDataSz;
	uint8_t *txBuf = malloc(txBufsz);
	TEST_ASSERT_NOT_NULL(txBuf);

	const size_t rxBufsz = (SPW_RX_MIN_BUFSZ + remainingHdrSz) * totalPackets + totalDataSz;
	uint8_t *rxBuf = malloc(rxBufsz);
	TEST_ASSERT_NOT_NULL(rxBuf);

	spw_rxPacket_t *packets = malloc(sizeof(spw_rxPacket_t) * totalPackets);
	TEST_ASSERT_NOT_NULL(packets);

	size_t *txBatchLen = malloc(sizeof(size_t) * totalBatches);
	size_t *rxBatchLen = malloc(sizeof(size_t) * totalBatches);
	TEST_ASSERT_NOT_NULL(txBatchLen);
	TEST_ASSERT_NOT_NULL(rxBatchLen);

	size_t txOffset = 0;
	size_t sentBytes = 0;
	for (size_t b = 0; b < totalBatches; b++) {
		const size_t currentTransmissionSz = (sentBytes + TRANSMISSION_SZ <= totalDataSz) ? TRANSMISSION_SZ : (totalDataSz - sentBytes);
		const size_t baseChunkSz = currentTransmissionSz / nPackets;
		const size_t remainder = currentTransmissionSz % nPackets;

		size_t batchTxBytes = 0;
		size_t chunkOffset = 0;

		for (size_t i = 0; i < nPackets; i++) {
			const size_t currentChunkSz = baseChunkSz + ((i == nPackets - 1) ? remainder : 0);
			size_t ret = spw_serializeTxMsg(SPW_TX_FLG_HDR_LEN(hdrSz), currentChunkSz, hdr,
					data + sentBytes + chunkOffset, txBuf + txOffset + batchTxBytes,
					txBufsz - txOffset - batchTxBytes);
			TEST_ASSERT_NOT_EQUAL(0, ret);

			batchTxBytes += ret;
			chunkOffset += currentChunkSz;
		}

		txBatchLen[b] = batchTxBytes;
		rxBatchLen[b] = (SPW_RX_MIN_BUFSZ + remainingHdrSz) * nPackets + currentTransmissionSz;

		txOffset += batchTxBytes;
		sentBytes += currentTransmissionSz;
	}

	spw_streamCtx_t ctx = {
		.readyCnt = 0,
		.go = false,
		.txBuf = txBuf,
		.txBatchLen = txBatchLen,
		.batchPackets = nPackets,
		.totalBatches = totalBatches,
		.async = async,
		.txDurationUs = 0,
		.txErr = 0,
		.rxBuf = rxBuf,
		.rxBatchLen = rxBatchLen,
		.packets = packets,
		.rxDurationUs = 0,
		.rxErr = 0,
		.txDone = 0,
		.rxDone = 0,
		.txBatch = 0,
		.RxBatch = 0,
		.txStage = 0,
		.RxStage = 0,
        .configuredBatches = 0,
	};

	TEST_ASSERT_EQUAL(0, mutexCreate(&ctx.lock));
	TEST_ASSERT_EQUAL(0, condCreate(&ctx.startCond));

	static char txStack[8192] __attribute__((aligned(16)));
	static char rxStack[8192] __attribute__((aligned(16)));

    handle_t txTid, rxTid;
	TEST_ASSERT_EQUAL(0, beginthreadex(spw_txStreamWorker, 4, txStack, sizeof(txStack), &ctx, &txTid));
	TEST_ASSERT_EQUAL(0, beginthreadex(spw_rxStreamWorker, 4, rxStack, sizeof(rxStack), &ctx, &rxTid));

	mutexLock(ctx.lock);
	while (ctx.readyCnt < 2) {
		condWait(ctx.startCond, ctx.lock, 0);
	}

	uint64_t tStart = get_time_us();
	ctx.go = true;
	condBroadcast(ctx.startCond);
	mutexUnlock(ctx.lock);

	TEST_ASSERT_GREATER_OR_EQUAL(0, threadJoin(txTid, 2000000));
	TEST_ASSERT_GREATER_OR_EQUAL(0, threadJoin(rxTid, 2000000));

	uint64_t totalStreamUs = get_time_us() - tStart;

	TEST_ASSERT_EQUAL_INT(0, ctx.txErr);
	TEST_ASSERT_EQUAL_INT(0, ctx.rxErr);

#if defined(DATA_CHECK) && (DATA_CHECK == 1)
	sentBytes = 0;
	size_t pktIdx = 0;
	for (size_t b = 0; b < totalBatches; b++) {
		const size_t currentTransmissionSz = (sentBytes + TRANSMISSION_SZ <= totalDataSz) ? TRANSMISSION_SZ : (totalDataSz - sentBytes);
		const size_t baseChunkSz = currentTransmissionSz / nPackets;
		const size_t remainder = currentTransmissionSz % nPackets;

		size_t chunkOffset = 0;
		for (size_t i = 0; i < nPackets; i++) {
			const size_t currentChunkSz = baseChunkSz + ((i == nPackets - 1) ? remainder : 0);
			const uint8_t *expectedData = data + sentBytes + chunkOffset;

			TEST_ASSERT_EQUAL(remainingHdrSz + currentChunkSz, packets[pktIdx].flags & SPW_RX_LEN_MSK);
			TEST_ASSERT_EQUAL(hdr[consumedHdrBytes], packets[pktIdx].buf[0]);
			TEST_ASSERT_EQUAL_HEX8_ARRAY(expectedData, packets[pktIdx].buf + remainingHdrSz, currentChunkSz);

			chunkOffset += currentChunkSz;
			pktIdx++;
		}
		sentBytes += currentTransmissionSz;
	}
#endif

	char msg[180];
	if (totalStreamUs > 0) {
		double sec = (double)totalStreamUs / 1000000.0;
		double mibPerSec = ((double)totalDataSz / (1024.0 * 1024.0)) / sec;
		double mbitsPerSec = ((double)totalDataSz * 8.0 / 1000000.0) / sec;

		snprintf(msg, sizeof(msg),
				"[SpaceWire Stream TX+RX] nPackets=%zu | Total: %llu us (%.3f s) | %.2f MiB/s | %.2f Mbit/s (TX: %llu us, RX: %llu us)",
				nPackets, (unsigned long long)totalStreamUs, sec, mibPerSec, mbitsPerSec,
				(unsigned long long)ctx.txDurationUs, (unsigned long long)ctx.rxDurationUs);
		UNITY_PRINT_EOL();
		UnityPrint(msg);
		UNITY_PRINT_EOL();
	}

	resourceDestroy(ctx.startCond);
	resourceDestroy(ctx.lock);
	free(rxBatchLen);
	free(txBatchLen);
	free(packets);
	free(rxBuf);
	free(txBuf);
	free(data);
}

static inline void test_timeMemcpy()
{
    size_t bufSz = TEST_DATA_PAYLOAD_1MB;

    uint8_t *src = mmap(NULL, bufSz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_UNCACHED, -1, 0);
    uint8_t *dst = mmap(NULL, bufSz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_UNCACHED, -1, 0);

    while (bufSz >= 64 * 1024) {
        src = malloc(bufSz);
        if (src != NULL) {
            dst = malloc(bufSz);
            if (dst != NULL) {
                break;
            }
            free(src);
            src = NULL;
        }
        bufSz /= 2;
    }

    TEST_ASSERT_NOT_NULL_MESSAGE(src, "Failed to allocate memory for memcpy benchmark");
    TEST_ASSERT_NOT_NULL_MESSAGE(dst, "Failed to allocate dst memory for memcpy benchmark");

    for (size_t i = 0; i < bufSz; i++) {
        src[i] = (uint8_t)(i & 0xFF);
        dst[i] = 0;
    }

    const size_t totalBytes = TEST_DATA_PAYLOAD_1MB;
    const size_t iterations = totalBytes / bufSz;

    asm volatile("" : : "r"(src), "r"(dst) : "memory");

    uint64_t tStart = get_time_us();

    for (size_t i = 0; i < iterations; i++) {
        memcpy(dst, src, bufSz);
        asm volatile("" : : "r"(dst) : "memory");
    }

    uint64_t tEnd = get_time_us();
    uint64_t elapsedUs = tEnd - tStart;

    TEST_ASSERT_EQUAL_HEX8_ARRAY(src, dst, bufSz);

    char msg[160];
    if (elapsedUs > 0) {
        double sec = (double)elapsedUs / 1000000.0;
        double mibPerSec = ((double)totalBytes / (1024.0 * 1024.0)) / sec;
        double mbitsPerSec = ((double)totalBytes * 8.0 / 1000000.0) / sec;

        snprintf(msg, sizeof(msg),
                "[RAM Memcpy] Total: %zu B (chunk: %zu B) | Time: %llu us (%.6f s) | %.2f MiB/s | %.2f Mbit/s",
                totalBytes, bufSz, (unsigned long long)elapsedUs, sec, mibPerSec, mbitsPerSec);
    }
    else {
        snprintf(msg, sizeof(msg),
                "[RAM Memcpy] Total: %zu B (chunk: %zu B) | Time: < 1 us",
                totalBytes, bufSz);
    }

    UNITY_PRINT_EOL();
    UnityPrint(msg);
    UNITY_PRINT_EOL();

    munmap(src, bufSz);
    munmap(dst, bufSz);

}


TEST_GROUP(test_spw_nm);


TEST_SETUP(test_spw_nm)
{
}


TEST_TEAR_DOWN(test_spw_nm)
{
}


TEST(test_spw_nm, spwSetAddress)
{
	/* rx */
	const spw_config_t cfg_rx = {
		.node.addr = TEST_SPW_ADDR0,
		.node.mask = 0xff,
		.dma.addr = TEST_SPW_ADDR0,
		.dma.mask = 0xff,
		.dma.flags = SPW_DMA_CFG_NS
	};

    int res = spw_configure(spw0, &cfg_rx);
    if (res < 0) {
        TEST_FAIL_MESSAGE("spw_rxConfigure failed");
        exit(EXIT_FAILURE);
    }

	/* tx */
	const spw_config_t cfg_tx = {
		.node.addr = TEST_SPW_ADDR1,
		.node.mask = 0xff,
		.dma.addr = TEST_SPW_ADDR1,
		.dma.mask = 0xff,
		.dma.flags = SPW_DMA_CFG_NS
	};

    res = spw_configure(spw1, &cfg_tx);
    if (res < 0) {
        TEST_FAIL_MESSAGE("spw_txConfigure failed");
        exit(EXIT_FAILURE);
    }

	TEST_ASSERT_EQUAL(0, res);
}


TEST(test_spw_nm, spwTxRxMultiplePacketsSync_1MB)
{
	test_spwRxTx_1MB(16, false);
}


TEST(test_spw_nm, spwTxRxMultiplePacketsAsync_1MB)
{
	test_spwRxTx_1MB(16, true);
}

TEST(test_spw_nm, spwTxRxBigNumberOfPacketsSync_1MB)
{
	/* Sync TX number of packets must not be bigger than SPW_TX_DESC_CNT */
	test_spwRxTx_1MB(64, false);
}


TEST(test_spw_nm, spwTxRxBigNumberOfPacketsAsync_1MB)
{
	/* Async TX number of packets must not be bigger than SPW_RX_DESC_CNT */
	test_spwRxTx_1MB(128, true);
}


TEST(test_spw_nm, spw_memcpy)
{
    test_timeMemcpy();
}


TEST(test_spw_nm, spwTxRxMultiThreadAsync_1MB)
{
    test_spwRxTxStream_1MB(24, 1);
}


TEST(test_spw_nm, spwTxRxMultiThreadSync_1MB)
{
    test_spwRxTxStream_1MB(24, 0);
}


TEST_GROUP_RUNNER(test_spw_nm)
{
	RUN_TEST_CASE(test_spw_nm, spwSetAddress);
	RUN_TEST_CASE(test_spw_nm, spwTxRxMultiplePacketsSync_1MB);
	RUN_TEST_CASE(test_spw_nm, spwTxRxMultiplePacketsAsync_1MB);
    RUN_TEST_CASE(test_spw_nm, spwTxRxMultiThreadAsync_1MB);
    RUN_TEST_CASE(test_spw_nm, spwTxRxMultiThreadSync_1MB);
    RUN_TEST_CASE(test_spw_nm, spw_memcpy);
}


void runner(void)
{
	RUN_TEST_GROUP(test_spw_nm);
}


int main(int argc, char *argv[])
{
    mkdir("/var", 0777);
    mkdir("/var/trace_spw", 0777);
    
	spwsrv_init();

	spw0 = spwsrv_getDev(dev0);
	spw1 = spwsrv_getDev(dev1);

	UNITY_PRINT_EOL();
	UnityPrint("Executing tests with LINK_FREQUENCY = " STR(LINK_FREQUENCY) " MHz");
	UNITY_PRINT_EOL();

	return (UnityMain(argc, (const char **)argv, runner) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}