 
#include "memcached.h"

void StoreSensorDataInMemcached(uint8_t concentratorId, SensorData *data) {
    memcached_st *memc;
    memcached_return rc;
    memc = memcached_create(NULL);
    memcached_server_add(memc, "localhost", 11211);

    char key[50];
    char value[256];

    while (data != NULL) {
        // Create a key based on concentrator ID and sensor address
        snprintf(key, sizeof(key), "concentrator_%d_sensor_%d", concentratorId, data->Address);
        snprintf(value, sizeof(value), "data=%d,inactivity=%d", data->Data, data->InactivityCounter);

        // Store the sensor data in memcached
        rc = memcached_set(memc, key, strlen(key), value, strlen(value), (time_t)0, (uint32_t)0);
        if (rc != MEMCACHED_SUCCESS) {
            fprintf(stderr, "Couldn't store sensor data in memcached: %s\n", memcached_strerror(memc, rc));
        }
        data = data->NextSensor;
    }
    memcached_free(memc);
}

SensorData *ExtractSensorDataFromMemcached(uint8_t concentratorId, uint16_t TotalSensorsCount) {
    // Initialize a memcached connection
    memcached_st *memc;
    memcached_return rc;
    memc = memcached_create(NULL);
    memcached_server_add(memc, "localhost", 11211);

    char key[50];
    char *retrieved_value;
    size_t value_length;
    uint32_t flags;
    SensorData *head = NULL;
    SensorData *current = NULL;

    for (int i = 0; i < TotalSensorsCount; i++) {
        snprintf(key, sizeof(key), "concentrator_%d_sensor_%d", concentratorId, i);

        retrieved_value = memcached_get(memc, key, strlen(key), &value_length, &flags, &rc);
        if (rc == MEMCACHED_SUCCESS && retrieved_value != NULL) {
            SensorData *newSensor = (SensorData *)malloc(sizeof(SensorData));
            if (sscanf(retrieved_value, "data=%hhu,inactivity=%hhu", &newSensor->Data, &newSensor->InactivityCounter) == 2) {
                newSensor->Address = i;
                newSensor->NextSensor = NULL;

                if (head == NULL) {
                    head = newSensor;
                    current = head;
                } else {
                    current->NextSensor = newSensor;
                    current = current->NextSensor;
                }
            }
            free(retrieved_value);
        }
    }
    memcached_free(memc);
    return head;
}
