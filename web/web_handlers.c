
#include "web.h"
#include "memcached.h"

char WebResponseBuffer[WEB_RESPONSE_SIZE];

void AppendToResponseBuffer(const char *Format, ...) {
    va_list Arguments;
    va_start(Arguments, Format);
    size_t CurrLength = strlen(WebResponseBuffer);
    size_t AvailSpace = WEB_RESPONSE_SIZE - CurrLength - 1;
    vsnprintf(WebResponseBuffer + CurrLength, AvailSpace, Format, Arguments);
    va_end(Arguments);
};

const char *GetSlaves(void) {
    memset(WebResponseBuffer, 0, sizeof(WebResponseBuffer));
    int16_t TotalConcentrators = 0;
    GetItemsCount(&TotalConcentrators, NULL);
    AppendToResponseBuffer("{\"slaves\":[");
    for (int16_t Idx = 0; Idx < TotalConcentrators; Idx++) {
        int16_t TotalSensors = 0;
        GetItemsCount(&Idx, &TotalSensors);
        char *Alias = RetrieveAliasFromMemcached("concentrator", Idx);
        AppendToResponseBuffer("{\"slave_id\":%d,\"slave_alias\":\"%s\",\"total_sensors_count\":%d}", 
							   Idx, Alias ? Alias : "", TotalSensors);
        if (Alias) {
            free(Alias);
        };
        if (Idx < TotalConcentrators - 1) {
            AppendToResponseBuffer(",");
        };
    };
    AppendToResponseBuffer("]}");
    return WebResponseBuffer;
}

const char *GetSlaveSensors(const char *SlaveId) {
    memset(WebResponseBuffer, 0, sizeof(WebResponseBuffer));
    int16_t ConcentratorId = atoi(SlaveId);
    int16_t TotalSensors = 0;
    GetItemsCount(&ConcentratorId, &TotalSensors);
    SensorData *Sensors = ExtractSensorDataFromMemcached(ConcentratorId, TotalSensors);
    AppendToResponseBuffer("{\"slave_id\":%d,\"sensors\":[", ConcentratorId);
    SensorData *Current = Sensors;
    while (Current != NULL) {
        AppendToResponseBuffer("{\"sensor_id\":%d,\"sensor_state\":%d,\"last_update\":%d}", 
							   Current->Address, Current->Data, Current->Timestamp);
        if (Current->NextSensor != NULL) {
            AppendToResponseBuffer(",");
        };
        Current = Current->NextSensor;
    };
    AppendToResponseBuffer("]}");
    Current = Sensors;
    while (Current != NULL) {
        SensorData *Next = Current->NextSensor;
        free(Current);
        Current = Next;
    };
    return WebResponseBuffer;
};

const char *SetSlaveAlias(const char *SlaveId, const char *SlaveAlias) {
    memset(WebResponseBuffer, 0, sizeof(WebResponseBuffer));
    int16_t ConcentratorId = atoi(SlaveId);
    StoreAliasInMemcached("concentrator", ConcentratorId, SlaveAlias);
    snprintf(WebResponseBuffer, sizeof(WebResponseBuffer), "{\"success\":\"true\"}");
    return WebResponseBuffer;
};

const char *SetZoneAlias(const char *ZoneAlias) {
    memset(WebResponseBuffer, 0, sizeof(WebResponseBuffer));
    StoreAliasInMemcached("zone", UINT8_MAX, ZoneAlias);
    snprintf(WebResponseBuffer, sizeof(WebResponseBuffer), "{\"success\":\"true\"}");
    return WebResponseBuffer;
};