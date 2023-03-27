/*
 * Phoenix-Pilot
 *
 * Data from testing scenarios
 *
 * Copyright 2023 Phoenix Systems
 * Author: Piotr Nieciecki
 *
 * This file is part of Phoenix-Pilot software
 *
 * %LICENSE%
 */

#ifndef TEST_DATA_H
#define TEST_DATA_H

#include <libsensors.h>


const static sensor_event_t scenario1Event1 = { .timestamp = 0, .type = SENSOR_TYPE_BARO, .baro.pressure = 1000, .baro.temp = 293 };

const static sensor_event_t scenario2Event1 = { .timestamp = 0, .type = SENSOR_TYPE_BARO, .baro.pressure = 1020, .baro.temp = 350 };
const static sensor_event_t scenario2Event2 = { .timestamp = 100, .type = SENSOR_TYPE_ACCEL, .accels.accelX = 5, .accels.accelY = 10, .accels.accelZ = 15 };
const static sensor_event_t scenario2Event3 = { .timestamp = 200, .type = SENSOR_TYPE_BARO, .baro.pressure = 500, .baro.temp = 300 };

const static sensor_event_t scenario3Event1 = { .timestamp = 50, .type = SENSOR_TYPE_BARO, .baro.pressure = 1230, .baro.temp = 365 };
const static sensor_event_t scenario3Event2 = { .timestamp = 60, .type = SENSOR_TYPE_ACCEL, .accels.accelX = 50, .accels.accelY = 60, .accels.accelZ = 70 };
const static sensor_event_t scenario3Event3 = { .timestamp = 90, .type = SENSOR_TYPE_BARO, .baro.pressure = 5000, .baro.temp = 301 };


#endif
