/*
 * Phoenix-RTOS
 *
 * GRLIB multi driver tests
 *
 * Copyright 2023 Phoenix Systems
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#include <board_config.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/platform.h>
#if defined(__CPU_GR716)
#include <phoenix/arch/gr716.h>
#elif defined(__CPU_GR712RC)
#include <phoenix/arch/gr712rc.h>
#else
#error "Unsupported target"
#endif

#include <grlib-multi.h>

#include <unity_fixture.h>


#define TEST_GPIO_ID   id_gpio1
#define TEST_GPIO_PATH "/dev/gpio1"

#define TEST_GPIO_BASE      ((volatile uint32_t *)0x8030D000)
#define TEST_GPIO_PORT_OFFS 0
#define TEST_GPIO_DIR_OFFS  2

#define TEST_SPI_ID   id_spi0
#define TEST_SPI_PATH "/dev/spi0"

#define TEST_SPI_SCK  41
#define TEST_SPI_MISO 42
#define TEST_SPI_MOSI 43
#define TEST_SPI_CS   44

#define TEST_SPI_BUFFSZ_SMALL 8
#define TEST_SPI_BUFFSZ       16 /* Equal to SPI FIFO */
#define TEST_SPI_BUFFSZ_LARGE 41

#define TEST_SPI_IOMUX_OPT 0x7u

#define TEST_ADC_ID   id_adc0
#define TEST_ADC_PATH "/dev/adc0"

#define TEST_ADC_PIN 37

#define TEST_ADC_IOMUX_OPT 0x8u

#define TEST_SPI_SLOW_CLK 0
#define TEST_SPI_FAST_CLK 1

#define TEST_SPW_ID0 id_spw0
#define TEST_SPW_ID1 id_spw1

#define TEST_SPW_PATH0 "/dev/spw0"
#define TEST_SPW_PATH1 "/dev/spw1"

#define TEST_SPW_ADDR0 0x1
#define TEST_SPW_ADDR1 0x2


static struct {
	uint8_t txBuff[TEST_SPI_BUFFSZ_LARGE];
	uint8_t rxBuff[TEST_SPI_BUFFSZ_LARGE];
} test_common;


/* Helper functions */


static oid_t test_getOid(const char *path)
{
	oid_t oid;
	while (lookup(path, NULL, &oid) < 0) {
		usleep(10000);
	}

	return oid;
}


static void test_spiSetConfigFast(spi_t *spi, uint8_t byteOrder)
{
	spi->type = spi_config;
	spi->config.byteOrder = byteOrder;
	spi->config.mode = spi_mode0;
	spi->config.prescFactor = 1;
	spi->config.prescaler = 0;
	spi->config.div16 = 0;
}


static void test_spiSetConfigSlow(spi_t *spi, uint8_t byteOrder)
{
	spi->type = spi_config;
	spi->config.byteOrder = byteOrder;
	spi->config.mode = spi_mode0;
	spi->config.prescFactor = 0;
	spi->config.prescaler = 7;
	spi->config.div16 = 1;
}


static void test_spiSetTransaction(spi_t *spi, size_t len)
{
	spi->type = spi_transaction;
	spi->transaction.slaveMsk = 1;
	spi->transaction.len = len;
}


static void test_spiConfigureClk(int speed, uint8_t byteOrder)
{
	msg_t msg;
	oid_t oid = test_getOid(TEST_SPI_PATH);
	multi_i_t *idevctl = NULL;
	multi_o_t *odevctl = NULL;

	msg.type = mtDevCtl;
	msg.i.data = NULL;
	msg.i.size = 0;
	msg.o.data = NULL;
	msg.o.size = 0;

	idevctl = (multi_i_t *)msg.i.raw;
	idevctl->id = TEST_SPI_ID;

	if (speed == TEST_SPI_SLOW_CLK) {
		test_spiSetConfigSlow(&idevctl->spi, byteOrder);
	}
	else {
		test_spiSetConfigFast(&idevctl->spi, byteOrder);
	}

	TEST_ASSERT_EQUAL_INT(0, msgSend(oid.port, &msg));

	odevctl = (multi_o_t *)msg.o.raw;
	TEST_ASSERT_EQUAL_INT(0, odevctl->err);
}


static void test_spiTransaction(int bufsz)
{
	msg_t msg;
	oid_t oid = test_getOid(TEST_SPI_PATH);
	multi_i_t *idevctl = NULL;
	multi_o_t *odevctl = NULL;
	uint8_t *txBuff = test_common.txBuff;
	uint8_t *rxBuff = test_common.rxBuff;

	for (int i = 0; i < bufsz; i++) {
		txBuff[i] = i;
	}

	msg.type = mtDevCtl;
	msg.i.data = txBuff;
	msg.i.size = bufsz;
	msg.o.data = rxBuff;
	msg.o.size = bufsz;

	idevctl = (multi_i_t *)msg.i.raw;
	idevctl->id = TEST_SPI_ID;
	test_spiSetTransaction(&idevctl->spi, bufsz);

	TEST_ASSERT_EQUAL_INT(0, msgSend(oid.port, &msg));

	odevctl = (multi_o_t *)msg.o.raw;
	TEST_ASSERT_EQUAL_INT(0, odevctl->err);

	TEST_ASSERT_EQUAL_UINT8_ARRAY(txBuff, rxBuff, bufsz);
}


/* GPIO tests */


TEST_GROUP(test_gpio);


TEST_SETUP(test_gpio)
{
}


TEST_TEAR_DOWN(test_gpio)
{
}


TEST(test_gpio, gpioGetDir)
{
	msg_t msg;
	uint32_t dir;
	oid_t oid = test_getOid(TEST_GPIO_PATH);
	multi_i_t *idevctl = NULL;
	multi_o_t *odevctl = NULL;

	msg.type = mtDevCtl;
	msg.i.data = NULL;
	msg.i.size = 0;
	msg.o.data = NULL;
	msg.o.size = 0;

	idevctl = (multi_i_t *)msg.i.raw;
	idevctl->id = TEST_GPIO_ID;
	idevctl->gpio.type = gpio_getDir;

	TEST_ASSERT_EQUAL_INT(0, msgSend(oid.port, &msg));

	odevctl = (multi_o_t *)msg.o.raw;

	TEST_ASSERT_EQUAL_INT(0, odevctl->err);

	dir = *(TEST_GPIO_BASE + TEST_GPIO_DIR_OFFS);

	TEST_ASSERT_EQUAL(dir, odevctl->val);
}


TEST(test_gpio, gpioGetPort)
{
	msg_t msg;
	uint32_t port;
	oid_t oid = test_getOid(TEST_GPIO_PATH);
	multi_i_t *idevctl = NULL;
	multi_o_t *odevctl = NULL;

	msg.type = mtDevCtl;
	msg.i.data = NULL;
	msg.i.size = 0;
	msg.o.data = NULL;
	msg.o.size = 0;

	idevctl = (multi_i_t *)msg.i.raw;
	idevctl->id = TEST_GPIO_ID;
	idevctl->gpio.type = gpio_getPort;

	TEST_ASSERT_EQUAL_INT(0, msgSend(oid.port, &msg));

	odevctl = (multi_o_t *)msg.o.raw;

	TEST_ASSERT_EQUAL_INT(0, odevctl->err);

	port = *(TEST_GPIO_BASE + TEST_GPIO_PORT_OFFS);

	TEST_ASSERT_EQUAL(port, odevctl->val);
}


/* SPI tests */


TEST_GROUP(test_spiPins);


TEST_SETUP(test_spiPins)
{
}


TEST_TEAR_DOWN(test_spiPins)
{
}


TEST(test_spiPins, spiSetPins)
{
	msg_t msg;
	oid_t oid = test_getOid(TEST_SPI_PATH);
	multi_i_t *idevctl = NULL;
	multi_o_t *odevctl = NULL;

	msg.type = mtDevCtl;
	msg.i.data = NULL;
	msg.i.size = 0;
	msg.o.data = NULL;
	msg.o.size = 0;

	idevctl = (multi_i_t *)msg.i.raw;
	idevctl->id = TEST_SPI_ID;
	idevctl->spi.type = spi_setPins;
	idevctl->spi.pins.sck = TEST_SPI_SCK;
	idevctl->spi.pins.miso = TEST_SPI_MISO;
	idevctl->spi.pins.mosi = TEST_SPI_MOSI;
	idevctl->spi.pins.cs = TEST_SPI_CS;

	TEST_ASSERT_EQUAL_INT(0, msgSend(oid.port, &msg));

	odevctl = (multi_o_t *)msg.o.raw;

	TEST_ASSERT_EQUAL_INT(0, odevctl->err);
}


TEST_GROUP(test_spiMsbFast);


TEST_SETUP(test_spiMsbFast)
{
	memset(test_common.txBuff, 0, TEST_SPI_BUFFSZ_LARGE);
	memset(test_common.rxBuff, 0, TEST_SPI_BUFFSZ_LARGE);
}


TEST_TEAR_DOWN(test_spiMsbFast)
{
}


TEST(test_spiMsbFast, spiConfigureMsbFastClk)
{
	test_spiConfigureClk(TEST_SPI_FAST_CLK, spi_msb);
}


TEST(test_spiMsbFast, spiTransactionSmallerThanFifo)
{
	test_spiTransaction(TEST_SPI_BUFFSZ_SMALL);
}


TEST(test_spiMsbFast, spiTransactionEqualFifo)
{
	test_spiTransaction(TEST_SPI_BUFFSZ);
}


TEST(test_spiMsbFast, spiTransactionBiggerThanFifo)
{
	test_spiTransaction(TEST_SPI_BUFFSZ_LARGE);
}


TEST_GROUP(test_spiLsbFast);


TEST_SETUP(test_spiLsbFast)
{
	memset(test_common.txBuff, 0, TEST_SPI_BUFFSZ_LARGE);
	memset(test_common.rxBuff, 0, TEST_SPI_BUFFSZ_LARGE);
}


TEST_TEAR_DOWN(test_spiLsbFast)
{
}


TEST(test_spiLsbFast, spiConfigureLsbFastClk)
{
	test_spiConfigureClk(TEST_SPI_FAST_CLK, spi_lsb);
}


TEST(test_spiLsbFast, spiTransactionSmallerThanFifo)
{
	test_spiTransaction(TEST_SPI_BUFFSZ_SMALL);
}


TEST(test_spiLsbFast, spiTransactionEqualFifo)
{
	test_spiTransaction(TEST_SPI_BUFFSZ);
}


TEST(test_spiLsbFast, spiTransactionBiggerThanFifo)
{
	test_spiTransaction(TEST_SPI_BUFFSZ_LARGE);
}


TEST_GROUP(test_spiMsbSlow);


TEST_SETUP(test_spiMsbSlow)
{
	memset(test_common.txBuff, 0, TEST_SPI_BUFFSZ_LARGE);
	memset(test_common.rxBuff, 0, TEST_SPI_BUFFSZ_LARGE);
}


TEST_TEAR_DOWN(test_spiMsbSlow)
{
}


TEST(test_spiMsbSlow, spiConfigureMsbSlowClk)
{
	test_spiConfigureClk(TEST_SPI_SLOW_CLK, spi_msb);
}


TEST(test_spiMsbSlow, spiTransactionSmallerThanFifo)
{
	test_spiTransaction(TEST_SPI_BUFFSZ_SMALL);
}


TEST(test_spiMsbSlow, spiTransactionEqualFifo)
{
	test_spiTransaction(TEST_SPI_BUFFSZ);
}


TEST(test_spiMsbSlow, spiTransactionBiggerThanFifo)
{
	test_spiTransaction(TEST_SPI_BUFFSZ_LARGE);
}


TEST_GROUP(test_spiLsbSlow);


TEST_SETUP(test_spiLsbSlow)
{
	memset(test_common.txBuff, 0, TEST_SPI_BUFFSZ_LARGE);
	memset(test_common.rxBuff, 0, TEST_SPI_BUFFSZ_LARGE);
}


TEST_TEAR_DOWN(test_spiLsbSlow)
{
}


TEST(test_spiLsbSlow, spiConfigureLsbSlowClk)
{
	test_spiConfigureClk(TEST_SPI_SLOW_CLK, spi_lsb);
}


TEST(test_spiLsbSlow, spiTransactionSmallerThanFifo)
{
	test_spiTransaction(TEST_SPI_BUFFSZ_SMALL);
}


TEST(test_spiLsbSlow, spiTransactionEqualFifo)
{
	test_spiTransaction(TEST_SPI_BUFFSZ);
}


TEST(test_spiLsbSlow, spiTransactionBiggerThanFifo)
{
	test_spiTransaction(TEST_SPI_BUFFSZ_LARGE);
}


/* ADC tests */


TEST_GROUP(test_adc);


TEST_SETUP(test_adc)
{
}


TEST_TEAR_DOWN(test_adc)
{
}


TEST(test_adc, adcDefaultConfigConversion)
{
	msg_t msg;
	uint32_t adcVal;
	oid_t oid = test_getOid(TEST_ADC_PATH);
	platformctl_t pctl = {
		.action = pctl_set,
		.type = pctl_iomux,
		.iocfg = {
			.opt = TEST_ADC_IOMUX_OPT,
			.pin = TEST_ADC_PIN,
			.pulldn = 0,
			.pullup = 0,
		}
	};

	TEST_ASSERT_EQUAL_INT(0, platformctl(&pctl));

	msg.type = mtRead;
	msg.i.io.oid.id = TEST_ADC_ID;
	msg.i.data = NULL;
	msg.i.size = 0;
	msg.o.size = sizeof(uint32_t);
	msg.o.data = (void *)&adcVal;

	TEST_ASSERT_EQUAL_INT(0, msgSend(oid.port, &msg));

	TEST_ASSERT_EQUAL_INT(0, msg.o.io.err);
}


/* SpaceWire tests - assume physical loopback SPW0-SPW1 */


static unsigned int test_spwConfigureRx(const oid_t rxOid, const size_t nPackets)
{
	/* Configure RX on SPW0 */
	msg_t msg = {
		.type = mtDevCtl,
		.i = { .data = NULL, .size = 0 },
		.o = { .data = NULL, .size = 0 },
	};

	multi_i_t *idevctl = (multi_i_t *)msg.i.raw;
	multi_o_t *odevctl = (multi_o_t *)msg.o.raw;

	idevctl->id = TEST_SPW_ID0;
	idevctl->spw.type = spw_rxConfig;
	idevctl->spw.task.rxConfig.nPackets = nPackets;

	TEST_ASSERT_EQUAL_INT(0, msgSend(rxOid.port, &msg));
	TEST_ASSERT_EQUAL_INT(nPackets, odevctl->err);

	return odevctl->val;
}


static void test_spwTx(const oid_t txOid, uint8_t *txBuf, const size_t txBufsz, const size_t nPackets, bool async)
{
	msg_t msg = {
		.type = mtDevCtl,
		.i = { .data = txBuf, .size = txBufsz },
		.o = { .data = NULL, .size = 0 },
	};

	multi_i_t *idevctl = (multi_i_t *)msg.i.raw;
	multi_o_t *odevctl = (multi_o_t *)msg.o.raw;

	idevctl->id = TEST_SPW_ID1;
	idevctl->spw.type = spw_tx;
	idevctl->spw.task.tx.nPackets = nPackets;
	idevctl->spw.task.tx.async = async;

	TEST_ASSERT_EQUAL_INT(0, msgSend(txOid.port, &msg));
	TEST_ASSERT_EQUAL_INT(nPackets, odevctl->err);
}


static void test_spwRxRead(const oid_t rxOid, const unsigned int firstDesc, uint8_t *rxBuf, size_t rxBufsz, spw_rxPacket_t *packets, const size_t nPackets)
{
	msg_t msg = {
		.type = mtDevCtl,
		.i = { .data = NULL, .size = 0 },
		.o = { .data = rxBuf, .size = rxBufsz },
	};
	multi_i_t *idevctl = (multi_i_t *)msg.i.raw;
	multi_o_t *odevctl = (multi_o_t *)msg.o.raw;

	idevctl->id = TEST_SPW_ID0;
	idevctl->spw.type = spw_rx;
	idevctl->spw.task.rx.nPackets = nPackets;
	idevctl->spw.task.rx.firstDesc = firstDesc;

	TEST_ASSERT_EQUAL_INT(0, msgSend(rxOid.port, &msg));
	TEST_ASSERT_EQUAL_INT(nPackets, odevctl->err);

	for (size_t i = 0; i < nPackets; i++) {
		rxBuf += multi_spwDeserializeRxMsg(rxBuf, &packets[i]);
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
		size_t ret = multi_spwSerializeTxMsg(SPW_TX_FLG_HDR_LEN(hdrSz), dataSz, hdr, data, txBuf + size, txBufsz - size);
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

	for (size_t i = 0; i < nPackets; i++) {
		TEST_ASSERT_EQUAL(hdrSz + dataSz, packets[i].flags & SPW_RX_LEN_MSK);
		TEST_ASSERT_EQUAL_HEX8_ARRAY(hdr, packets[i].buf, hdrSz);
		TEST_ASSERT_EQUAL_HEX8_ARRAY(data, packets[i].buf + hdrSz, dataSz);
	}

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
	};
	multi_i_t *idevctl = (multi_i_t *)msg.i.raw;
	multi_o_t *odevctl = (multi_o_t *)msg.o.raw;

	idevctl->id = TEST_SPW_ID0;
	idevctl->spw.type = spw_config;
	idevctl->spw.task.config.node.addr = TEST_SPW_ADDR0;
	idevctl->spw.task.config.node.mask = 0x0;
	idevctl->spw.task.config.dma.addr = TEST_SPW_ADDR0;
	idevctl->spw.task.config.dma.mask = 0x0;

	TEST_ASSERT_EQUAL_INT(0, msgSend(oid.port, &msg));
	TEST_ASSERT_EQUAL_INT(0, odevctl->err);

	oid = test_getOid(TEST_SPW_PATH1);
	idevctl->id = TEST_SPW_ID1;
	idevctl->spw.type = spw_config;
	idevctl->spw.task.config.node.addr = TEST_SPW_ADDR1;
	idevctl->spw.task.config.node.mask = 0x0;
	idevctl->spw.task.config.dma.addr = TEST_SPW_ADDR1;
	idevctl->spw.task.config.dma.mask = 0x0;

	TEST_ASSERT_EQUAL_INT(0, msgSend(oid.port, &msg));
	TEST_ASSERT_EQUAL_INT(0, odevctl->err);
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


/* Test runner */


TEST_GROUP_RUNNER(test_gpio)
{
	RUN_TEST_CASE(test_gpio, gpioGetDir);
	RUN_TEST_CASE(test_gpio, gpioGetPort);
}

/* To test SPI without any external components, enable loopback in SPI driver! */
TEST_GROUP_RUNNER(test_spi)
{
	RUN_TEST_CASE(test_spiPins, spiSetPins);
	RUN_TEST_CASE(test_spiMsbFast, spiConfigureMsbFastClk);
	RUN_TEST_CASE(test_spiMsbFast, spiTransactionSmallerThanFifo);
	RUN_TEST_CASE(test_spiMsbFast, spiTransactionEqualFifo);
	RUN_TEST_CASE(test_spiMsbFast, spiTransactionBiggerThanFifo);
	RUN_TEST_CASE(test_spiLsbFast, spiConfigureLsbFastClk);
	RUN_TEST_CASE(test_spiLsbFast, spiTransactionSmallerThanFifo);
	RUN_TEST_CASE(test_spiLsbFast, spiTransactionEqualFifo);
	RUN_TEST_CASE(test_spiLsbFast, spiTransactionBiggerThanFifo);
	RUN_TEST_CASE(test_spiMsbSlow, spiConfigureMsbSlowClk);
	RUN_TEST_CASE(test_spiMsbSlow, spiTransactionSmallerThanFifo);
	RUN_TEST_CASE(test_spiMsbSlow, spiTransactionEqualFifo);
	RUN_TEST_CASE(test_spiMsbSlow, spiTransactionBiggerThanFifo);
	RUN_TEST_CASE(test_spiLsbSlow, spiConfigureLsbSlowClk);
	RUN_TEST_CASE(test_spiLsbSlow, spiTransactionSmallerThanFifo);
	RUN_TEST_CASE(test_spiLsbSlow, spiTransactionEqualFifo);
	RUN_TEST_CASE(test_spiLsbSlow, spiTransactionBiggerThanFifo);
}


TEST_GROUP_RUNNER(test_adc)
{
	RUN_TEST_CASE(test_adc, adcDefaultConfigConversion);
}


TEST_GROUP_RUNNER(test_spw)
{
	RUN_TEST_CASE(test_spw, spwSetAddress);
	RUN_TEST_CASE(test_spw, spwTxRxSinglePacketSync);
	RUN_TEST_CASE(test_spw, spwTxRxSinglePacketAsync);
	RUN_TEST_CASE(test_spw, spwTxRxMultiplePacketsSync);
	RUN_TEST_CASE(test_spw, spwTxRxMultiplePacketsAsync);
	RUN_TEST_CASE(test_spw, spwTxRxBigNumberOfPacketsSync);
	RUN_TEST_CASE(test_spw, spwTxRxBigNumberOfPacketsAsync);
}


void runner(void)
{
	/* GPIO tests were tailored to be run on GR716-MINI
	 * (because of many functions that given pin can have)
	 */
#if (GPIO_PORT_CNT > 0) && defined(__CPU_GR716)
	RUN_TEST_GROUP(test_gpio);
#endif
#if SPI_CNT > 0
	RUN_TEST_GROUP(test_spi);
#endif
#if ADC_CNT > 0
	RUN_TEST_GROUP(test_adc);
#endif
#if SPW_CNT > 1
	RUN_TEST_GROUP(test_spw);
#endif
}


int main(int argc, char *argv[])
{
	UnityMain(argc, (const char **)argv, runner);
	return 0;
}
