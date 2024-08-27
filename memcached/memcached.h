
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
SensorData *ExtractSensorDataFromMemcached(uint8_t ConcentratorId, uint16_t TotalSensorsCount);
void GetItemsCount(int16_t *ConcentratorParam, int16_t *SensorsParam);
void StoreAliasInMemcached(const char *AliasType, uint8_t Id, const char *Alias);
char *RetrieveAliasFromMemcached(const char *AliasType, uint8_t Id);

#endif

