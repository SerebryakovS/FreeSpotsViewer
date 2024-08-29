 
#include "memcached.h"

void StoreSensorDataInMemcached(uint8_t ConcentratorId, SensorData *Data) {
    memcached_st *Memc;
    memcached_return Rc;
    Memc = memcached_create(NULL);
    memcached_server_add(Memc, "localhost", 11211);
    char Key[50], Value[256];
    int SensorCount = 0;
    while (Data != NULL) {
        snprintf(Key, sizeof(Key), "concentrator_%d_sensor_%d", ConcentratorId, Data->Address);
        snprintf(Value, sizeof(Value), "data=%d,timestamp=%u", Data->Data, Data->Timestamp);
        Rc = memcached_set(Memc, Key, strlen(Key), Value, strlen(Value), (time_t)5, (uint32_t)0);
        if (Rc != MEMCACHED_SUCCESS) {
            fprintf(stderr, "Couldn't store sensor data in memcached: %s\n", memcached_strerror(Memc, Rc));
        };
        Data = Data->NextSensor;
        SensorCount++;
    };
    snprintf(Key, sizeof(Key), "concentrator_%d", ConcentratorId);
    snprintf(Value, sizeof(Value), "%d", SensorCount);
    Rc = memcached_set(Memc, Key, strlen(Key), Value, strlen(Value), (time_t)5, (uint32_t)0);
    if (Rc != MEMCACHED_SUCCESS) {
        fprintf(stderr, "Couldn't store sensor count in memcached: %s\n", memcached_strerror(Memc, Rc));
    };
    memcached_free(Memc);
};

SensorData *ExtractSensorDataFromMemcached(uint8_t ConcentratorId) {
    memcached_st *Memc;
    memcached_return Rc;
    Memc = memcached_create(NULL);
    memcached_server_add(Memc, "localhost", 11211);
    char Key[50], *RetrievedValue;
    size_t ValueLength;
    uint32_t Flags;
    SensorData *Head = NULL;
    SensorData *Current = NULL;
    for (int Idx = 0; Idx < 256; Idx++) {
        snprintf(Key, sizeof(Key), "concentrator_%d_sensor_%d", ConcentratorId, Idx);
        RetrievedValue = memcached_get(Memc, Key, strlen(Key), &ValueLength, &Flags, &Rc);
        if (Rc == MEMCACHED_SUCCESS && RetrievedValue != NULL) {
            SensorData *newSensor = (SensorData *)malloc(sizeof(SensorData));
            if (sscanf(RetrievedValue, "data=%hhu,timestamp=%u", &newSensor->Data, &newSensor->Timestamp) == 2) {
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

int16_t GetSensorsCount(uint8_t ConcentratorId) {
    memcached_st *Memc;
    memcached_return Rc;
    Memc = memcached_create(NULL);
    memcached_server_add(Memc, "localhost", 11211);
    char Key[50], *Value; size_t ValueLength;
    uint32_t Flags;
    int16_t SensorCount = -1;
    snprintf(Key, sizeof(Key), "concentrator_%d", ConcentratorId);
    Value = memcached_get(Memc, Key, strlen(Key), &ValueLength, &Flags, &Rc);
    if (Rc == MEMCACHED_SUCCESS && Value != NULL) {
        SensorCount = atoi(Value);
        free(Value);
    };
    memcached_free(Memc);
    return SensorCount;
};

int16_t GetConcentratorsCount() {
    memcached_st *Memc;
    memcached_return Rc;
    Memc = memcached_create(NULL);
    memcached_server_add(Memc, "localhost", 11211);
    char Key[50], *Value;
    size_t ValueLength;
    uint32_t Flags;
    int ConcentratorCount = 0;
    for (int Idx = 0; Idx < 256; Idx++) {
        snprintf(Key, sizeof(Key), "concentrator_%d", Idx);
        Value = memcached_get(Memc, Key, strlen(Key), &ValueLength, &Flags, &Rc);
        if (Rc == MEMCACHED_SUCCESS && Value != NULL) {
            ConcentratorCount++;
            free(Value);
        } else {
            break;
        };
    };
    memcached_free(Memc);
    return ConcentratorCount;
};
