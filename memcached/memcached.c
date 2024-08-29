 
#include "memcached.h"

void StoreSensorDataInMemcached(uint8_t ConcentratorId, SensorData *Data) {
    memcached_st *Memc;
    memcached_return Rc;
    Memc = memcached_create(NULL);
    memcached_server_add(Memc, "localhost", 11211);
    char Key[50], Value[256];
    while (Data != NULL) {
        snprintf(Key, sizeof(Key), "concentrator_%d_sensor_%d", ConcentratorId, Data->Address);
		snprintf(Value, sizeof(Value), "data=%d,timestamp=%u", Data->Data, Data->Timestamp);
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
    for (int Idx = 2; Idx < TotalSensorsCount; Idx++) {
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

void GetItemsCount(int16_t *ConcentratorParam, int16_t *SensorsParam) {
    if (ConcentratorParam == NULL) {
        return;
    };
    memcached_st *Memc = memcached_create(NULL);
    memcached_server_add(Memc, "localhost", 11211);
    char Key[50]; uint32_t Flags; size_t ValueLength;
    memcached_return Rc;
    char *Value;
    int16_t Id = 2;
    int16_t *CountParam = (SensorsParam == NULL) ? ConcentratorParam : SensorsParam;
    const char *KeyFormat = (SensorsParam == NULL) ? "concentrator_%d_sensor_2" : "concentrator_%d_sensor_%d";
    *CountParam = 0;
    while (1) {
        if (SensorsParam == NULL) {
            snprintf(Key, sizeof(Key), KeyFormat, Id);
        } else {
            snprintf(Key, sizeof(Key), KeyFormat, *ConcentratorParam, Id);
        };
        Value = memcached_get(Memc, Key, strlen(Key), &ValueLength, &Flags, &Rc);
        if (Rc == MEMCACHED_SUCCESS && Value != NULL) {
			printf("%s\n",Key);
            (*CountParam)++;
            free(Value);
        } else {
            break;
        };
        Id++;
    };
    memcached_free(Memc);
};

void StoreAliasInMemcached(const char *AliasType, uint8_t Id, const char *Alias) {
    memcached_st *Memc = memcached_create(NULL);
    memcached_server_add(Memc, "localhost", 11211);
    char Key[50];
    if (Id != UINT8_MAX) {
        snprintf(Key, sizeof(Key), "%s_%d_alias", AliasType, Id);
    } else {
        snprintf(Key, sizeof(Key), "%s_alias", AliasType);
    };
    memcached_return Rc = memcached_set(Memc, Key, strlen(Key), Alias, strlen(Alias), (time_t)0, (uint32_t)0);
    if (Rc != MEMCACHED_SUCCESS) {
        fprintf(stderr, "Couldn't store %s alias in memcached: %s\n", AliasType, memcached_strerror(Memc, Rc));
    };
    memcached_free(Memc);
};

char *RetrieveAliasFromMemcached(const char *AliasType, uint8_t Id) {
    memcached_st *Memc = memcached_create(NULL);
    memcached_server_add(Memc, "localhost", 11211);
    char Key[50];
    if (Id != UINT8_MAX) {
        snprintf(Key, sizeof(Key), "%s_%d_alias", AliasType, Id);
    } else {
        snprintf(Key, sizeof(Key), "%s_alias", AliasType);
    };
    uint32_t Flags;
    size_t ValueLength;
    memcached_return Rc;
    char *Value = memcached_get(Memc, Key, strlen(Key), &ValueLength, &Flags, &Rc);
    memcached_free(Memc);
    return (Rc == MEMCACHED_SUCCESS && Value != NULL) ? Value : NULL;
};