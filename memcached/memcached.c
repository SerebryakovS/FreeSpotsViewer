 
#include "memcached.h"

void StoreSensorDataInMemcached(uint8_t ConcentratorId, SensorData *Data) {
    memcached_st *Memc;
    memcached_return Rc;
    Memc = memcached_create(NULL);
    memcached_server_add(Memc, "localhost", 11211);
    char Key[50], Value[256];
    while (Data != NULL) {
        snprintf(Key, sizeof(Key), "concentrator_%d_sensor_%d", ConcentratorId, Data->Address);
        snprintf(Value, sizeof(Value), "data=%d,inactivity=%d", Data->Data, Data->InactivityCounter);
        Rc = memcached_set(Memc, Key, strlen(Key), Value, strlen(Value), (time_t)10, (uint32_t)0);
        if (Rc != MEMCACHED_SUCCESS) {
            fprintf(stderr, "Couldn't store sensor data in memcached: %s\n", memcached_strerror(Memc, Rc));
        };
        Data = Data->NextSensor;
    };
    memcached_free(Memc);
};

SensorData *ExtractSensorDataFromMemcached(uint8_t ConcentratorId, uint16_t TotalSensorsCount) {
    memcached_st *Memc;
    memcached_return Rc;
    Memc = memcached_create(NULL);
    memcached_server_add(Memc, "localhost", 11211);
    char Key[50], *RetrievedValue;
    size_t ValueLength;
    uint32_t Flags;
    SensorData *Head = NULL;
    SensorData *Current = NULL;
    for (int Idx = 0; Idx < TotalSensorsCount; Idx++) {
        snprintf(Key, sizeof(Key), "concentrator_%d_sensor_%d", ConcentratorId, Idx);
        RetrievedValue = memcached_get(Memc, Key, strlen(Key), &ValueLength, &Flags, &Rc);
        if (Rc == MEMCACHED_SUCCESS && RetrievedValue != NULL) {
            SensorData *newSensor = (SensorData *)malloc(sizeof(SensorData));
            if (sscanf(RetrievedValue, "data=%hhu,inactivity=%hhu", &newSensor->Data, &newSensor->InactivityCounter) == 2) {
                newSensor->Address = Idx;
                newSensor->NextSensor = NULL;
                if (Head == NULL) {
                    Head = newSensor;
                    Current = Head;
                } else {
                    Current->NextSensor = newSensor;
                    Current = Current->NextSensor;
                };
            };
            free(RetrievedValue);
        };
    };
    memcached_free(Memc);
    return Head;
};
