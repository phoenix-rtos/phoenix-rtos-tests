/*
 * Phoenix-RTOS
 *
 *    Phoenix-RTOS standard library functions tests
 *    HEADER:
 *    - ioctl.h
 *    TESTED:
 *    - ioctl()
 *
 * Copyright 2023 Phoenix Systems
 * Author: Adam Debek
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <phoenix/ioctl.h>
#include <sys/msg.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <posix/utils.h>
#include <unity_fixture.h>

static const int32_t test_expFlagVal = 0x12345678;
static uint32_t test_port;
static int test_fileDesc, test_devDesc;

typedef uint8_t test_ioctlBuf_t[1024];

#define MAX_FAIL       10
#define PATH_TF        "ioctl_testFile"
#define PATH_REG       "ioctl_testRegularFile"
#define DEV_IOCTL_TEST "/dev/ioctlTest"
/* T stands for test, do not confuse with tty */
#define TEST_GRP ('T')

#define TEST_IOCTL_SIG       _IOC(IOC_VOID, TEST_GRP, 0x01, 0)
#define TEST_IOCTL_IN_VAL    _IOC(IOC_VOID, TEST_GRP, 0x02, sizeof(int32_t))
#define TEST_IOCTL_IN        _IOC(IOC_IN, TEST_GRP, 0x03, sizeof(int32_t))
#define TEST_IOCTL_IN_BIG    _IOC(IOC_IN, TEST_GRP, 0x04, sizeof(test_ioctlBuf_t))
#define TEST_IOCTL_OUT       _IOC(IOC_OUT, TEST_GRP, 0x05, sizeof(int32_t))
#define TEST_IOCTL_OUT_BIG   _IOC(IOC_OUT, TEST_GRP, 0x06, sizeof(test_ioctlBuf_t))
#define TEST_IOCTL_INOUT     _IOC(IOC_INOUT, TEST_GRP, 0x07, sizeof(int32_t))
#define TEST_IOCTL_INOUT_BIG _IOC(IOC_INOUT, TEST_GRP, 0x08, sizeof(test_ioctlBuf_t))

static void *test_thread(void *arg)
{
	msg_t msg;
	msg_rid_t rid;
	int32_t out;
	test_ioctlBuf_t out2;
	int ret;

	while (1) {
		if ((ret = msgRecv(test_port, &msg, &rid)) < 0) {
			if (ret == -EINVAL) {
				break;
			}
			continue;
		}

		if (msg.type == mtDevCtl) {
			unsigned long request;
			int err = 0;

			const void *out_data = NULL;
			const void *in_data = ioctl_unpack(&msg, &request, NULL);

			if (lseek(test_fileDesc, 0, SEEK_SET) != 0) {
				TEST_MESSAGE("lseek failed");
			}

			switch (request) {
				case TEST_IOCTL_IN_VAL:
					if (write(test_fileDesc, &in_data, sizeof(int32_t)) != sizeof(int32_t)) {
						TEST_MESSAGE("write failed in TEST_IOCTL_IN_VAL request");
					}
					break;
				case TEST_IOCTL_SIG:
					if (write(test_fileDesc, &test_expFlagVal, sizeof(int32_t)) != sizeof(int32_t)) {
						TEST_MESSAGE("write failed in TEST_IOCTL_SIG request");
					}
					break;
				case TEST_IOCTL_IN:
					if (write(test_fileDesc, in_data, sizeof(int32_t)) != sizeof(int32_t)) {
						TEST_MESSAGE("write failed in TEST_IOCTL_IN request");
					}
					break;
				case TEST_IOCTL_IN_BIG:
					if (write(test_fileDesc, in_data, sizeof(test_ioctlBuf_t)) != sizeof(test_ioctlBuf_t)) {
						TEST_MESSAGE("write failed in TEST_IOCTL_IN_BIG request");
					}
					break;
				case TEST_IOCTL_OUT:
					out = 15;
					out_data = (const void *)(&out);
					break;
				case TEST_IOCTL_OUT_BIG:
					memset(out2, 5, sizeof(out2));
					out_data = (const void *)(out2);
					break;
				case TEST_IOCTL_INOUT:
					if (write(test_fileDesc, in_data, sizeof(int32_t)) != sizeof(int32_t)) {
						TEST_MESSAGE("write failed in TEST_IOCTL_INOUT request");
					}
					out = 18;
					out_data = (const void *)(&out);
					break;
				case TEST_IOCTL_INOUT_BIG:
					if (write(test_fileDesc, in_data, sizeof(test_ioctlBuf_t)) != sizeof(test_ioctlBuf_t)) {
						TEST_MESSAGE("write failed in TEST_IOCTL_INOUT_BIG request");
					}
					memset(out2, 8, sizeof(out2));
					out_data = (const void *)(out2);
					break;
				default:
					err = -1;
					break;
			}
			ioctl_setResponse(&msg, request, err, out_data);
		}
		msgRespond(test_port, &msg, rid);
	}
	return NULL;
}


TEST_GROUP(ioctl);


TEST_SETUP(ioctl)
{
}

TEST_TEAR_DOWN(ioctl)
{
}


TEST(ioctl, invalid_req)
{
	int ret = ioctl(test_devDesc, 0x1, NULL);
	TEST_ASSERT_EQUAL_INT(-1, ret);
}


TEST(ioctl, regular_file)
{
	/* try regular file, not special device file */
	int fdReg = open(PATH_REG, O_RDWR | O_CREAT | O_TRUNC, S_IFREG);
	TEST_ASSERT_NOT_EQUAL_INT(-1, fdReg);
	errno = 0;
	int ret = ioctl(fdReg, TEST_IOCTL_SIG, NULL);
	TEST_ASSERT_NOT_EQUAL_INT(0, ret);
	close(fdReg);
	remove(PATH_REG);
}


TEST(ioctl, not_valid_fd)
{
	errno = 0;
	int ret = ioctl(1234, 0, NULL);
	TEST_ASSERT_EQUAL_INT(-1, ret);
	TEST_ASSERT_EQUAL_INT(EBADF, errno);
}


TEST(ioctl, no_data)
{
	int32_t flag;
	int ret;

	/* Send data to driver by value */
	ret = ioctl(test_devDesc, TEST_IOCTL_SIG, NULL);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, lseek(test_fileDesc, 0, SEEK_SET));
	TEST_ASSERT_EQUAL_INT(sizeof(int32_t), read(test_fileDesc, &flag, sizeof(int32_t)));
	TEST_ASSERT_EQUAL_INT32(test_expFlagVal, flag);
}


TEST(ioctl, in_val)
{
	int32_t rdata = 0;
	int32_t dataIn = 14;
	int ret;

	/* Send data to driver by value */
	ret = ioctl(test_devDesc, TEST_IOCTL_IN_VAL, dataIn);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, lseek(test_fileDesc, 0, SEEK_SET));
	TEST_ASSERT_EQUAL_INT(sizeof(int32_t), read(test_fileDesc, &rdata, sizeof(int32_t)));
	TEST_ASSERT_EQUAL_INT32(dataIn, rdata);
}


TEST(ioctl, data_in)
{
	int32_t rdata = 0;
	int32_t dataIn = 20;
	int ret;

	/* Send data to driver by pointer */
	ret = ioctl(test_devDesc, TEST_IOCTL_IN, &dataIn);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, lseek(test_fileDesc, 0, SEEK_SET));
	TEST_ASSERT_EQUAL_INT(sizeof(int32_t), read(test_fileDesc, &rdata, sizeof(int32_t)));
	TEST_ASSERT_EQUAL_INT32(dataIn, rdata);
}


TEST(ioctl, data_in_big)
{
	/* Big data so that it is not copied inside message */
	test_ioctlBuf_t dataIn;
	test_ioctlBuf_t rdata;
	int ret;

	memset(dataIn, 3, sizeof(dataIn));
	memset(rdata, 0, sizeof(rdata));

	/* Send data to driver by pointer, data big enough to not be copied by ioctl_pack directly into message */
	ret = ioctl(test_devDesc, TEST_IOCTL_IN_BIG, dataIn);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, lseek(test_fileDesc, 0, SEEK_SET));
	TEST_ASSERT_EQUAL_INT(sizeof(rdata), read(test_fileDesc, rdata, sizeof(rdata)));
	TEST_ASSERT_EQUAL_MEMORY(dataIn, rdata, sizeof(rdata));
}


TEST(ioctl, data_out)
{
	int32_t dataOut = 0;
	int ret;

	/* Get data from driver */
	ret = ioctl(test_devDesc, TEST_IOCTL_OUT, &dataOut);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT32(15, dataOut);
}


TEST(ioctl, data_out_big)
{
	/* Big data so that it is not copied inside message */
	test_ioctlBuf_t expData;
	test_ioctlBuf_t dataOut;
	int ret;

	memset(expData, 5, sizeof(expData));
	memset(dataOut, 0, sizeof(dataOut));

	/* Get data from driver */
	ret = ioctl(test_devDesc, TEST_IOCTL_OUT_BIG, dataOut);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_MEMORY(expData, dataOut, sizeof(expData));
}


TEST(ioctl, data_inout)
{
	const int32_t dataIn = 17;
	int32_t rdata = 0;
	int32_t dataInout = dataIn;
	int ret;

	ret = ioctl(test_devDesc, TEST_IOCTL_INOUT, &dataInout);

	/* Get data sent to driver */
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, lseek(test_fileDesc, 0, SEEK_SET));
	TEST_ASSERT_EQUAL_INT(sizeof(int32_t), read(test_fileDesc, &rdata, sizeof(int32_t)));
	TEST_ASSERT_EQUAL_INT32(dataIn, rdata);

	/* Check data returned from driver */
	TEST_ASSERT_EQUAL_INT32(18, dataInout);
}


TEST(ioctl, data_inout_big)
{
	/* Big data so that it is not copied inside message */
	test_ioctlBuf_t dataInout;
	test_ioctlBuf_t dataIn;
	test_ioctlBuf_t dataOut;
	test_ioctlBuf_t rdata;
	int ret;

	memset(dataIn, 7, sizeof(dataIn));
	memset(dataOut, 8, sizeof(dataOut));
	memset(rdata, 0, sizeof(rdata));
	memcpy(dataInout, dataIn, sizeof(dataInout));

	/* Get data sent to driver */
	ret = ioctl(test_devDesc, TEST_IOCTL_INOUT_BIG, dataInout);
	TEST_ASSERT_EQUAL_INT(0, ret);
	TEST_ASSERT_EQUAL_INT(0, lseek(test_fileDesc, 0, SEEK_SET));
	TEST_ASSERT_EQUAL_INT(sizeof(rdata), read(test_fileDesc, rdata, sizeof(rdata)));
	TEST_ASSERT_EQUAL_MEMORY(dataIn, rdata, sizeof(rdata));

	/* Check data returned from driver */
	TEST_ASSERT_EQUAL_MEMORY(dataOut, dataInout, sizeof(dataOut));
}


TEST_GROUP_RUNNER(ioctl)
{
	/* main thread - run test cases */
	RUN_TEST_CASE(ioctl, invalid_req);
	RUN_TEST_CASE(ioctl, regular_file);
	RUN_TEST_CASE(ioctl, not_valid_fd);
	RUN_TEST_CASE(ioctl, no_data);
	RUN_TEST_CASE(ioctl, in_val);
	RUN_TEST_CASE(ioctl, data_in);
	RUN_TEST_CASE(ioctl, data_in_big);
	RUN_TEST_CASE(ioctl, data_out);
	RUN_TEST_CASE(ioctl, data_out_big);
	RUN_TEST_CASE(ioctl, data_inout);
	RUN_TEST_CASE(ioctl, data_inout_big);
}


void runner(void)
{
	RUN_TEST_GROUP(ioctl);
}


int main(int argc, char *argv[])
{
	pthread_t tid;
	pthread_attr_t attr;
	oid_t dev;
	int res, n = 0;

	res = pthread_attr_init(&attr);
	if (res != 0) {
		fprintf(stderr, "pthread_attr_init() error\n");
		return 1;
	}

	res = pthread_attr_setstacksize(&attr, 4096);
	if (res != 0) {
		fprintf(stderr, "pthread_attr_setstacksize error\n");
		return 1;
	}

	res = portCreate(&test_port);
	if (res != EOK) {
		fprintf(stderr, "Couldn't create port\n");
		return 1;
	}

	dev.port = test_port;
	dev.id = 0;

	res = create_dev(&dev, DEV_IOCTL_TEST);
	if (res != EOK) {
		fprintf(stderr, "Couldn't create device\n");
		return 1;
	}

	test_fileDesc = open(PATH_TF, O_RDWR | O_CREAT | O_TRUNC, S_IFREG);
	if (test_fileDesc < 0) {
		fprintf(stderr, "Couldn't open file\n");
		return 1;
	}

	res = pthread_create(&tid, &attr, test_thread, NULL);
	if (res != 0) {
		remove(PATH_TF);
		portDestroy(test_port);
		fprintf(stderr, "Couldn't create thread\n");
		return 1;
	}

	while ((test_devDesc = open(DEV_IOCTL_TEST, O_RDWR)) < 0 && n < MAX_FAIL) {
		usleep(10000);
		n++;
	}

	if (n == MAX_FAIL) {
		close(test_fileDesc);
		remove(PATH_TF);
		fprintf(stderr, "Can't open device file\n");
		return 1;
	}

	UnityMain(argc, (const char **)argv, runner);
	portDestroy(test_port);
	pthread_join(tid, NULL);

	close(test_devDesc);
	close(test_fileDesc);
	remove(PATH_TF);
	remove(DEV_IOCTL_TEST);

	return 0;
}
