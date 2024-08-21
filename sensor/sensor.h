
#ifndef SENSOR_H
#define SENSOR_H

#include "config.h"
#include "memcached.h"

#define SENSOR_ZERO_ADDR             2
#define DISPLAY_ADDR   SENSORS_COUNT-1
#define SYNC_INTERVAL        1000000UL
#define MSG_START                 0x02
#define MSG_END                   0x03
#define CMD_SYNC                  0x03
#define CMD_DATA                  0x82
#define CMD_DISP                  0x83
#define CMD_ACK                   0x04
#define CMD_SET_ID                0x01

static const uint32_t SlotTimeUs = SYNC_INTERVAL / SENSORS_COUNT;

extern SensorData *SensorsHead;

uint8_t CalculateFreeSensors();
void AddSensor(uint8_t SensorAddress, int8_t SensorValue);
void RemoveSensor(uint8_t SensorAddress);
void UpdateSensor(uint8_t SensorAddress, int8_t SensorValue);
void CheckInactiveSensors(uint8_t MaxInactivity);
void SyncAndRead(UartModule *_UartModule, uint8_t *ReadBuffer, struct timeval *Timeout);
void SensorsProtoHandler(UartModule *_UartModule, uint8_t SlotIdx, uint8_t *ReadBuffer, int16_t ReadCountBytes);

#endif
