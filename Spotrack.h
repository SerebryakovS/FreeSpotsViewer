
#ifndef SPOTRACK_H
#define SPOTRACK_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <wiringPi.h>
#include <sys/select.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/time.h>
#include <modbus.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>

#define SENSORS_COUNT               64
#define BSLAVES_COUNT               16
#define SENSOR_ZERO_ADDR          0x01
#define SYNC_INTERVAL        1000000UL
#define MSG_START                 0x02
#define MSG_END                   0x03
#define CMD_SYNC                  0x03
#define CMD_DATA                  0x82
#define CMD_ACK                   0x04
#define CMD_SET_ID                0x01
#define RS485_UART_PORT_A "/dev/ttyS1"
#define RS485_UART_PORT_B "/dev/ttyS2"
#define CONC_INET_IFACE         "eth0"
#define RS485_CTRL_PIN_A             4
#define RS485_CTRL_PIN_B             5
#define RS485_ROLE_LED_B            14
#define RS485_ROLE_PIN_B            10


typedef struct UartModule {
    char     PortId;
    uint16_t UartPortFd;
    uint8_t  EnablePin;
} UartModule;

typedef struct SensorData {
    uint8_t  Address;
    uint8_t  Data;
    uint8_t  InactivityCounter;
    struct SensorData *NextSensor;
} SensorData;

extern SensorData *SensorsHead;

SensorData OwnSensors[SENSORS_COUNT];

extern UartModule _UartModuleA, _UartModuleB;

void *SyncClientsHandler(void *Arguments);
void *SyncConcentratorsHandler(void *Arguments);

#endif
