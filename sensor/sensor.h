
#ifndef SENSOR_H
#define SENSOR_H

#include "config.h"
#include "memcached.h"

#define SENSOR_ZERO_ADDR             1
#define DISPLAY_ADDR   SENSORS_COUNT-1
#define SENSOR_MAX_ADDR	DISPLAY_ADDR-1
#define SYNC_INTERVAL        1000000UL
#define MSG_START				  0xA0
#define MSG_END				      0xAF
#define CMD_SYNC				  0x91
#define CMD_DATA				  0x92
#define CMD_DISP				  0x93
#define CMD_SET_ID			      0x94

static const uint32_t SlotTimeUs = SYNC_INTERVAL / SENSORS_COUNT;

extern SensorData *SensorsHead;

void ExtractLaddrEnv();
void UpdateLaddr(uint8_t Address);
uint8_t GetLaddr();
uint8_t PopSensorIdsGap();
bool IsSensorAddressBusy(uint8_t SensorAddress);
uint16_t CalculateFreeSensorsCount( void );
void AddSensor(uint8_t SensorAddress, int8_t SensorValue);
void RemoveSensor(uint8_t SensorAddress);
void UpdateSensor(uint8_t SensorAddress, int8_t SensorValue);
void CheckInactiveSensors(uint8_t MaxInactivity);
void SyncAndRead(UartModule *_UartModule, uint8_t *ReadBuffer, struct timeval *Timeout);
void SensorsProtoHandler(UartModule *_UartModule, uint8_t SlotIdx, uint8_t *ReadBuffer, int16_t ReadCountBytes);

#endif
