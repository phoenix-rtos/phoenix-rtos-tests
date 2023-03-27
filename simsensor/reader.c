/*
 * Phoenix-Pilot
 *
 * Unit tests for simsensor reader
 *
 * Copyright 2023 Phoenix Systems
 * Author: Piotr Nieciecki
 *
 * This file is part of Phoenix-Pilot software
 *
 * %LICENSE%
 */

#include <unity_fixture.h>

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

#include <simsensor_reader.h>

#include "test_data.h"

#define TEST_FILES_DIR "usr/test/simsensor/"
#define QUEUE_SZ 10
#define MAX_TIMESTAMP_DIFF 100000 /* in microseconds */

#define TEST_ASSERT_EQUAL_BARO_EVENT(expected, actual) \
    TEST_ASSERT_EQUAL(SENSOR_TYPE_BARO, (actual).type); \
    TEST_ASSERT_EQUAL((expected).timestamp, (actual).timestamp); \
    TEST_ASSERT_EQUAL_UINT32((expected).baro.pressure, (actual).baro.pressure); \
    TEST_ASSERT_EQUAL_UINT32((expected).baro.temp, (actual).baro.tmp);


static simsens_reader_t reader;
static int readerInit;

static event_queue_t queue;
static int queueInit;

static sensor_event_t event;

static time_t actTime;
static time_t firstTimestamp;


time_t getTime(void)
{
    struct timeval tv;
    gettimeofday(&tv,NULL);

    return 1000000 * tv.tv_sec + tv.tv_usec;
}


TEST_GROUP(group_sim_reader);


TEST_SETUP(group_sim_reader)
{
    readerInit = -1;

    queueInit = eventQueue_init(&queue, QUEUE_SZ);
    TEST_ASSERT_EQUAL(0, queueInit);

    actTime = getTime();
}


TEST_TEAR_DOWN(group_sim_reader)
{
    if (readerInit == 0) {
        reader_close(&reader);
        readerInit = -1;
    }

    if (queueInit == 0) {
        eventQueue_free(&queue);
        queueInit = -1;
    }
}


TEST(group_sim_reader, correct_init)
{
    readerInit = reader_open(&reader, TEST_FILES_DIR"scenario1.csv", SENSOR_TYPE_BARO, 100);
    TEST_ASSERT_EQUAL(0, readerInit);
}


TEST(group_sim_reader, correct_parse)
{
    readerInit = reader_open(&reader, TEST_FILES_DIR"scenario1.csv", SENSOR_TYPE_BARO, 100);
    TEST_ASSERT_EQUAL(0, readerInit);
    
    TEST_ASSERT_EQUAL(0, reader_read(&reader, &queue));

    TEST_ASSERT_EQUAL(0, eventQueue_dequeue(&queue, &event));
    TEST_ASSERT_EQUAL(SENSOR_TYPE_BARO, event.type);
    TEST_ASSERT_INT64_WITHIN(MAX_TIMESTAMP_DIFF, actTime, event.timestamp);
    TEST_ASSERT_EQUAL(1000, event.baro.pressure);
    TEST_ASSERT_EQUAL(293, event.baro.temp);

    TEST_ASSERT_NOT_EQUAL(0, eventQueue_dequeue(&queue, &event));
}


TEST(group_sim_reader, ignore_other_sensor_entry)
{
    readerInit = reader_open(&reader, TEST_FILES_DIR"scenario2.csv", SENSOR_TYPE_BARO, 1000);
    TEST_ASSERT_EQUAL(0, readerInit);
    
    TEST_ASSERT_EQUAL(0, reader_read(&reader, &queue));

    TEST_ASSERT_EQUAL(0, eventQueue_dequeue(&queue, &event));
    TEST_ASSERT_EQUAL(SENSOR_TYPE_BARO, event.type);
    TEST_ASSERT_INT64_WITHIN(MAX_TIMESTAMP_DIFF, actTime, event.timestamp);
    TEST_ASSERT_EQUAL(1020, event.baro.pressure);
    TEST_ASSERT_EQUAL(350, event.baro.temp);

    firstTimestamp = event.timestamp;

    TEST_ASSERT_EQUAL(0, eventQueue_dequeue(&queue, &event));
    TEST_ASSERT_EQUAL(SENSOR_TYPE_BARO, event.type);
    TEST_ASSERT_EQUAL(firstTimestamp + 200, event.timestamp);
    TEST_ASSERT_EQUAL(500, event.baro.pressure);
    TEST_ASSERT_EQUAL(300, event.baro.temp);

    TEST_ASSERT_NOT_EQUAL(0, eventQueue_dequeue(&queue, &event));
}


TEST(group_sim_reader, parse_multiple_header_at_once)
{
    readerInit = reader_open(&reader, TEST_FILES_DIR"scenario2.csv", SENSOR_TYPE_BARO|SENSOR_TYPE_ACCEL, 1000);
    TEST_ASSERT_EQUAL(0, readerInit);
    
    TEST_ASSERT_EQUAL(0, reader_read(&reader, &queue));

    TEST_ASSERT_EQUAL(0, eventQueue_dequeue(&queue, &event));
    TEST_ASSERT_EQUAL(SENSOR_TYPE_BARO, event.type);
    TEST_ASSERT_INT64_WITHIN(MAX_TIMESTAMP_DIFF, actTime, event.timestamp);
    TEST_ASSERT_EQUAL(1020, event.baro.pressure);
    TEST_ASSERT_EQUAL(350, event.baro.temp);

    firstTimestamp = event.timestamp;

    TEST_ASSERT_EQUAL(0, eventQueue_dequeue(&queue, &event));
    TEST_ASSERT_EQUAL(SENSOR_TYPE_ACCEL, event.type);
    TEST_ASSERT_EQUAL(firstTimestamp + 100, event.timestamp);
    TEST_ASSERT_EQUAL(5, event.accels.accelX);
    TEST_ASSERT_EQUAL(10, event.accels.accelY);
    TEST_ASSERT_EQUAL(15, event.accels.accelZ);

    TEST_ASSERT_EQUAL(0, eventQueue_dequeue(&queue, &event));
    TEST_ASSERT_EQUAL(SENSOR_TYPE_BARO, event.type);
    TEST_ASSERT_EQUAL(firstTimestamp + 200, event.timestamp);
    TEST_ASSERT_EQUAL(500, event.baro.pressure);
    TEST_ASSERT_EQUAL(300, event.baro.temp);

    TEST_ASSERT_NOT_EQUAL(0, eventQueue_dequeue(&queue, &event));
}


TEST(group_sim_reader, time_horizon_check)
{
    readerInit = reader_open(&reader, TEST_FILES_DIR"scenario2.csv", SENSOR_TYPE_BARO, 100);
    TEST_ASSERT_EQUAL(0, readerInit);
    
    TEST_ASSERT_EQUAL(0, reader_read(&reader, &queue));

    TEST_ASSERT_EQUAL(0, eventQueue_dequeue(&queue, &event));
    TEST_ASSERT_EQUAL(SENSOR_TYPE_BARO, event.type);
    TEST_ASSERT_INT64_WITHIN(MAX_TIMESTAMP_DIFF, actTime, event.timestamp);
    TEST_ASSERT_EQUAL(1020, event.baro.pressure);
    TEST_ASSERT_EQUAL(350, event.baro.temp);

    TEST_ASSERT_NOT_EQUAL(0, eventQueue_dequeue(&queue, &event));
}


TEST(group_sim_reader, multiple_invocations_returns_later_events)
{
    readerInit = reader_open(&reader, TEST_FILES_DIR"scenario2.csv", SENSOR_TYPE_BARO, 100);
    TEST_ASSERT_EQUAL(0, readerInit);
    
    TEST_ASSERT_EQUAL(0, reader_read(&reader, &queue));

    TEST_ASSERT_EQUAL(0, eventQueue_dequeue(&queue, &event));
    TEST_ASSERT_EQUAL(SENSOR_TYPE_BARO, event.type);
    TEST_ASSERT_INT64_WITHIN(MAX_TIMESTAMP_DIFF, actTime, event.timestamp);
    TEST_ASSERT_EQUAL(1020, event.baro.pressure);
    TEST_ASSERT_EQUAL(350, event.baro.temp);
    TEST_ASSERT_NOT_EQUAL(0, eventQueue_dequeue(&queue, &event));

    firstTimestamp = event.timestamp;

    TEST_ASSERT_EQUAL(0, reader_read(&reader, &queue));

    TEST_ASSERT_EQUAL(0, eventQueue_dequeue(&queue, &event));
    TEST_ASSERT_EQUAL(SENSOR_TYPE_BARO, event.type);
    TEST_ASSERT_EQUAL(firstTimestamp + 200, event.timestamp);
    TEST_ASSERT_EQUAL(500, event.baro.pressure);
    TEST_ASSERT_EQUAL(300, event.baro.temp);
    TEST_ASSERT_NOT_EQUAL(0, eventQueue_dequeue(&queue, &event));

    TEST_ASSERT_NOT_EQUAL(0, reader_read(&reader, &queue));
}


TEST(group_sim_reader, looping)
{
    int i;
    time_t exp_timestamp = actTime;

    readerInit = reader_open(&reader, TEST_FILES_DIR"scenario3.csv", SENSOR_TYPE_BARO, 10000);
    TEST_ASSERT_EQUAL(0, readerInit);
    
    TEST_ASSERT_EQUAL(0, reader_read(&reader, &queue));

    TEST_ASSERT_EQUAL(0, eventQueue_dequeue(&queue, &event));
    TEST_ASSERT_EQUAL(SENSOR_TYPE_BARO, event.type);
    TEST_ASSERT_INT64_WITHIN(MAX_TIMESTAMP_DIFF, exp_timestamp, event.timestamp);
    TEST_ASSERT_EQUAL(1230, event.baro.pressure);
    TEST_ASSERT_EQUAL(365, event.baro.temp);

    exp_timestamp = event.timestamp;

    for (i = 0; i < QUEUE_SZ/2 - 1; i++) {
        exp_timestamp += 40;

        TEST_ASSERT_EQUAL(0, eventQueue_dequeue(&queue, &event));
        TEST_ASSERT_EQUAL(SENSOR_TYPE_BARO, event.type);
        TEST_ASSERT_EQUAL(exp_timestamp, event.timestamp);
        TEST_ASSERT_EQUAL(5000, event.baro.pressure);
        TEST_ASSERT_EQUAL(301, event.baro.temp);

        TEST_ASSERT_EQUAL(0, eventQueue_dequeue(&queue, &event));
        TEST_ASSERT_EQUAL(SENSOR_TYPE_BARO, event.type);
        TEST_ASSERT_EQUAL(exp_timestamp, event.timestamp);
        TEST_ASSERT_EQUAL(1230, event.baro.pressure);
        TEST_ASSERT_EQUAL(365, event.baro.temp);
    }

    exp_timestamp += 40;

    TEST_ASSERT_EQUAL(0, eventQueue_dequeue(&queue, &event));
    TEST_ASSERT_EQUAL(SENSOR_TYPE_BARO, event.type);
    TEST_ASSERT_EQUAL(exp_timestamp, event.timestamp);
    TEST_ASSERT_EQUAL(5000, event.baro.pressure);
    TEST_ASSERT_EQUAL(301, event.baro.temp);

    TEST_ASSERT_NOT_EQUAL(0, eventQueue_dequeue(&queue, &event));
}


TEST(group_sim_reader, looping_stop_with_time_horizon)
{
    int i;
    time_t exp_timestamp = actTime;

    readerInit = reader_open(&reader, TEST_FILES_DIR"scenario3.csv", SENSOR_TYPE_BARO, 150);
    TEST_ASSERT_EQUAL(0, readerInit);
    
    TEST_ASSERT_EQUAL(0, reader_read(&reader, &queue));

    TEST_ASSERT_EQUAL(0, eventQueue_dequeue(&queue, &event));
    TEST_ASSERT_EQUAL(SENSOR_TYPE_BARO, event.type);
    TEST_ASSERT_INT64_WITHIN(MAX_TIMESTAMP_DIFF, exp_timestamp, event.timestamp);
    TEST_ASSERT_EQUAL(1230, event.baro.pressure);
    TEST_ASSERT_EQUAL(365, event.baro.temp);

    exp_timestamp = event.timestamp;

    for (i = 0; i < 3; i++) {
        exp_timestamp += 40;

        TEST_ASSERT_EQUAL(0, eventQueue_dequeue(&queue, &event));
        TEST_ASSERT_EQUAL(SENSOR_TYPE_BARO, event.type);
        TEST_ASSERT_EQUAL(exp_timestamp, event.timestamp);
        TEST_ASSERT_EQUAL(5000, event.baro.pressure);
        TEST_ASSERT_EQUAL(301, event.baro.temp);

        TEST_ASSERT_EQUAL(0, eventQueue_dequeue(&queue, &event));
        TEST_ASSERT_EQUAL(SENSOR_TYPE_BARO, event.type);
        TEST_ASSERT_EQUAL(exp_timestamp, event.timestamp);
        TEST_ASSERT_EQUAL(1230, event.baro.pressure);
        TEST_ASSERT_EQUAL(365, event.baro.temp);
    }

    TEST_ASSERT_NOT_EQUAL(0, eventQueue_dequeue(&queue, &event));
}


TEST(group_sim_reader, looping_stop_with_time_horizon_multiple_times)
{
    int i;
    time_t exp_timestamp = actTime;

    readerInit = reader_open(&reader, TEST_FILES_DIR"scenario3.csv", SENSOR_TYPE_BARO, 150);
    TEST_ASSERT_EQUAL(0, readerInit);
    
    TEST_ASSERT_EQUAL(0, reader_read(&reader, &queue));

    TEST_ASSERT_EQUAL(0, eventQueue_dequeue(&queue, &event));
    TEST_ASSERT_EQUAL(SENSOR_TYPE_BARO, event.type);
    TEST_ASSERT_INT64_WITHIN(MAX_TIMESTAMP_DIFF, exp_timestamp, event.timestamp);
    TEST_ASSERT_EQUAL(1230, event.baro.pressure);
    TEST_ASSERT_EQUAL(365, event.baro.temp);

    exp_timestamp = event.timestamp;

    for (i = 0; i < 3; i++) {
        exp_timestamp += 40;

        TEST_ASSERT_EQUAL(0, eventQueue_dequeue(&queue, &event));
        TEST_ASSERT_EQUAL(SENSOR_TYPE_BARO, event.type);
        TEST_ASSERT_EQUAL(exp_timestamp, event.timestamp);
        TEST_ASSERT_EQUAL(5000, event.baro.pressure);
        TEST_ASSERT_EQUAL(301, event.baro.temp);

        TEST_ASSERT_EQUAL(0, eventQueue_dequeue(&queue, &event));
        TEST_ASSERT_EQUAL(SENSOR_TYPE_BARO, event.type);
        TEST_ASSERT_EQUAL(exp_timestamp, event.timestamp);
        TEST_ASSERT_EQUAL(1230, event.baro.pressure);
        TEST_ASSERT_EQUAL(365, event.baro.temp);
    }

    TEST_ASSERT_NOT_EQUAL(0, eventQueue_dequeue(&queue, &event));

    TEST_ASSERT_EQUAL(0, reader_read(&reader, &queue));

    for (i = 0; i < 4; i++) {
        exp_timestamp += 40;

        TEST_ASSERT_EQUAL(0, eventQueue_dequeue(&queue, &event));
        TEST_ASSERT_EQUAL(SENSOR_TYPE_BARO, event.type);
        TEST_ASSERT_EQUAL(exp_timestamp, event.timestamp);
        TEST_ASSERT_EQUAL(5000, event.baro.pressure);
        TEST_ASSERT_EQUAL(301, event.baro.temp);

        TEST_ASSERT_EQUAL(0, eventQueue_dequeue(&queue, &event));
        TEST_ASSERT_EQUAL(SENSOR_TYPE_BARO, event.type);
        TEST_ASSERT_EQUAL(exp_timestamp, event.timestamp);
        TEST_ASSERT_EQUAL(1230, event.baro.pressure);
        TEST_ASSERT_EQUAL(365, event.baro.temp);
    }

    TEST_ASSERT_NOT_EQUAL(0, eventQueue_dequeue(&queue, &event));
}


TEST_GROUP_RUNNER(group_sim_reader)
{
	RUN_TEST_CASE(group_sim_reader, correct_init);
    RUN_TEST_CASE(group_sim_reader, correct_parse);
    RUN_TEST_CASE(group_sim_reader, ignore_other_sensor_entry);
    RUN_TEST_CASE(group_sim_reader, parse_multiple_header_at_once);
    RUN_TEST_CASE(group_sim_reader, time_horizon_check);
    RUN_TEST_CASE(group_sim_reader, multiple_invocations_returns_later_events);
    RUN_TEST_CASE(group_sim_reader, looping);
    RUN_TEST_CASE(group_sim_reader, looping_stop_with_time_horizon);
    RUN_TEST_CASE(group_sim_reader, looping_stop_with_time_horizon_multiple_times);
}
