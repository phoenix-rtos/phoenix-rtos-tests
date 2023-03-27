/*
 * Phoenix-Pilot
 *
 * Unit tests for event_queue
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
#include <time.h>

#include <event_queue.h>


#define QUEUE_SZ 10


static event_queue_t queue;
static int queueInit;


/* ##############################################################################
 * -------------------------        minimal setup       -------------------------
 * ############################################################################## */


TEST_GROUP(group_event_queue_minimal_setup);


TEST_SETUP(group_event_queue_minimal_setup)
{
    queueInit = -1;
}


TEST_TEAR_DOWN(group_event_queue_minimal_setup)
{
    if (queueInit == 0) {
        eventQueue_free(&queue);
    }
}


TEST(group_event_queue_minimal_setup, correct_init)
{
    queueInit = eventQueue_init(&queue, QUEUE_SZ);
    TEST_ASSERT_EQUAL(0, queueInit);
}


/* ##############################################################################
 * ------------------------------        std       ------------------------------
 * ############################################################################## */


TEST_GROUP(group_event_queue);


TEST_SETUP(group_event_queue)
{
    queueInit = eventQueue_init(&queue, QUEUE_SZ);
    TEST_ASSERT_EQUAL(0, queueInit);
}


TEST_TEAR_DOWN(group_event_queue)
{
    if (queueInit == 0) {
        eventQueue_free(&queue);
    }
}


TEST(group_event_queue, add_elem)
{
    sensor_event_t event = {};

    TEST_ASSERT_EQUAL(0, eventQueue_enqueue(&queue, &event));
}


TEST(group_event_queue, add_as_many_items_as_queue_size)
{
    sensor_event_t event = {};
    int i;

    for (i = 0; i < QUEUE_SZ; i++) {
        TEST_ASSERT_EQUAL(0, eventQueue_enqueue(&queue, &event));
    }
}


TEST(group_event_queue, fails_when_more_items_added_than_queue_size)
{
    sensor_event_t event = {};
    int i;

    for (i = 0; i < QUEUE_SZ; i++) {
        TEST_ASSERT_EQUAL(0, eventQueue_enqueue(&queue, &event));
    }

    TEST_ASSERT_NOT_EQUAL(0, eventQueue_enqueue(&queue, &event));
}


TEST(group_event_queue, remove_without_adding)
{
    sensor_event_t event;

    TEST_ASSERT_NOT_EQUAL(0, eventQueue_dequeue(&queue, &event));
}


TEST(group_event_queue, add_one_item_remove_one_item)
{
    sensor_type_t type = SENSOR_TYPE_BARO;
    time_t timestamp = 123;
    
    sensor_event_t event_to_add = { .type = type, .timestamp = timestamp };
    sensor_event_t event_from_queue = {};

    TEST_ASSERT_EQUAL(0, eventQueue_enqueue(&queue, &event_to_add));
    TEST_ASSERT_EQUAL(0, eventQueue_dequeue(&queue, &event_from_queue));
    
    TEST_ASSERT_EQUAL(event_to_add.type, event_from_queue.type);
    TEST_ASSERT_EQUAL(event_to_add.timestamp, event_from_queue.timestamp);
}


TEST(group_event_queue, remove_more_than_in_queue)
{
    sensor_type_t type1 = SENSOR_TYPE_BARO, type2 = SENSOR_TYPE_GPS;
    time_t time1 = 100, time2 = 200;

    sensor_event_t event1 = { .type = type1, .timestamp = time1 };
    sensor_event_t event2 = { .type = type2, .timestamp = time2 };
    sensor_event_t event = {};

    TEST_ASSERT_EQUAL(0, eventQueue_enqueue(&queue, &event1));
    TEST_ASSERT_EQUAL(0, eventQueue_enqueue(&queue, &event2));

    TEST_ASSERT_EQUAL(0, eventQueue_dequeue(&queue, &event));
    TEST_ASSERT_EQUAL(event1.type, event.type);
    TEST_ASSERT_EQUAL(event1.timestamp, event.timestamp);

    TEST_ASSERT_EQUAL(0, eventQueue_dequeue(&queue, &event));
    TEST_ASSERT_EQUAL(event2.type, event.type);
    TEST_ASSERT_EQUAL(event2.timestamp, event.timestamp);

    TEST_ASSERT_NOT_EQUAL(0, eventQueue_dequeue(&queue, &event));
}


TEST(group_event_queue, cyclic_buffer_test)
{
    sensor_type_t type = SENSOR_TYPE_BARO;
    time_t timestamp = 123;
    int i;
    
    sensor_event_t event_to_add = { .type = type, .timestamp = timestamp };
    sensor_event_t event = {};

    for (i = 0; i < QUEUE_SZ; i++) {
        TEST_ASSERT_EQUAL(0, eventQueue_enqueue(&queue, &event_to_add));
    }

    for (i = 0; i < QUEUE_SZ/2; i++) {
        TEST_ASSERT_EQUAL(0, eventQueue_dequeue(&queue, &event));
    }

    for (i = 0; i < QUEUE_SZ/2; i++) {
        TEST_ASSERT_EQUAL(0, eventQueue_enqueue(&queue, &event_to_add));
    }

    TEST_ASSERT_NOT_EQUAL(0, eventQueue_enqueue(&queue, &event_to_add));
}


TEST_GROUP_RUNNER(group_event_queue)
{
	RUN_TEST_CASE(group_event_queue_minimal_setup, correct_init);

    RUN_TEST_CASE(group_event_queue, add_elem);
    RUN_TEST_CASE(group_event_queue, add_as_many_items_as_queue_size);
    RUN_TEST_CASE(group_event_queue, fails_when_more_items_added_than_queue_size);

    RUN_TEST_CASE(group_event_queue, remove_without_adding);
    RUN_TEST_CASE(group_event_queue, add_one_item_remove_one_item);
    RUN_TEST_CASE(group_event_queue, remove_more_than_in_queue);
    RUN_TEST_CASE(group_event_queue, cyclic_buffer_test);
}
