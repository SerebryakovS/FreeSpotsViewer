
#include "sensor.h"

//////////////////////////////////////////////////////////////////////////////////

SensorData *SensorsHead = NULL;
static uint32_t CurrentSensorsCount = 0;

void AddSensor(uint8_t SensorAddress, int8_t SensorValue) {
    SensorData *NewSensor = (SensorData *)malloc(sizeof(SensorData));
    if (!NewSensor) {
        perror("Failed to allocate memory for new sensor");
        return;
    };
    NewSensor->Address = SensorAddress;
    NewSensor->Data = SensorValue;
    NewSensor->InactivityCounter = 0;
    NewSensor->NextSensor = SensorsHead;
    SensorsHead = NewSensor;
    CurrentSensorsCount++;
};

void RemoveSensor(uint8_t SensorAddress) {
    SensorData *CurrSensor = SensorsHead;
    SensorData *PrevSensor = NULL;
    while (CurrSensor != NULL) {
        if (CurrSensor->Address == SensorAddress) {
            if (PrevSensor == NULL) {
                SensorsHead = CurrSensor->NextSensor;
            } else {
                PrevSensor->NextSensor = CurrSensor->NextSensor;
            };
            free(CurrSensor);
            CurrentSensorsCount--;
            return;
        };
        PrevSensor = CurrSensor;
        CurrSensor = CurrSensor->NextSensor;
    };
};

void UpdateSensor(uint8_t SensorAddress, int8_t SensorValue) {
    SensorData *CurrSensor = SensorsHead;
    while (CurrSensor != NULL) {
        if (CurrSensor->Address == SensorAddress) {
            CurrSensor->Data = SensorValue;
            CurrSensor->InactivityCounter = 0;
            return;
        };
        CurrSensor = CurrSensor->NextSensor;
    };
    AddSensor(SensorAddress, SensorValue);
};

void CheckInactiveSensors(uint8_t MaxInactivity) {
    SensorData *CurrSensor = SensorsHead;
    SensorData *PrevSensor = NULL;
    while (CurrSensor != NULL) {
        if (CurrSensor->InactivityCounter >= MaxInactivity) {
            if (PrevSensor == NULL) {
                SensorsHead = CurrSensor->NextSensor;
            } else {
                PrevSensor->NextSensor = CurrSensor->NextSensor;
            };
            SensorData *ToDelete = CurrSensor;
            CurrSensor = CurrSensor->NextSensor;
            free(ToDelete);
        } else {
            CurrSensor->InactivityCounter++;
            PrevSensor = CurrSensor;
            CurrSensor = CurrSensor->NextSensor;
        };
    };
};

uint8_t CalculateFreeSensors() {
    uint8_t FreeSpaces = 0;
    SensorData *CurrSensor = SensorsHead;
    while (CurrSensor != NULL) {
        if (CurrSensor->Data == 0) {
            FreeSpaces++;
        };
        CurrSensor = CurrSensor->NextSensor;
    };
    return FreeSpaces;
};

//////////////////////////////////////////////////////////////////////////////////
