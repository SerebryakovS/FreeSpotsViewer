
#ifndef MEMCACHED_H
#define MEMCACHED_H

#include <stdio.h>
#include <libmemcached/memcached.h>

typedef struct SensorData {
    uint8_t  Address;
    int8_t   Data;
    uint8_t  InactivityCounter;
    struct SensorData *NextSensor;
} SensorData;

void StoreSensorDataInMemcached(uint8_t concentratorId, SensorData *data);
SensorData *ExtractSensorDataFromMemcached(uint8_t concentratorId, uint16_t TotalSensorsCount);

#endif

