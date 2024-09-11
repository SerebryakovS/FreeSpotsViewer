
#ifndef MEMCACHED_H
#define MEMCACHED_H

#include <stdio.h>
#include <libmemcached/memcached.h>

typedef struct SensorData {
    uint8_t  Address;
    int8_t   Data;
	int8_t   InactivityCounter;
    uint32_t Timestamp;
    struct SensorData *NextSensor;
} SensorData;

void StoreSensorDataInMemcached(uint8_t ConcentratorId, SensorData *Data);
SensorData *ExtractSensorDataFromMemcached(uint8_t ConcentratorId);
int16_t GetSensorsCount(uint8_t ConcentratorId);
int16_t GetConcentratorsCount();

#endif

