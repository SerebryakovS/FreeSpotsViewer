
#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

#define SENSORS_COUNT               64
#define BSLAVES_COUNT               16

#define SENSOR_ZERO_ADDR          0x01
#define SENSOR_STATE_NONE            0
#define SENSOR_STATE_NOT_SET         1
#define SENSOR_STATE_SET             2

#define DISPLAY_ADDR              0x40
#define SYNC_INTERVAL        1000000UL
#define MSG_START                 0x02
#define MSG_END                   0x03
#define CMD_SYNC                  0x03
#define CMD_DATA                  0x82
#define CMD_DISP                  0x83
#define CMD_ACK                   0x04
#define CMD_SET_ID                0x01

#define RS485_UART_PORT_A "/dev/ttyS1"
#define RS485_CTRL_PIN_A             4
#define RS485_ROLE_LED_B            14

#define RS485_UART_PORT_B "/dev/ttyS2"
#define RS485_CTRL_PIN_B             5
#define RS485_ROLE_PIN_B            10

#define CONC_INET_IFACE         "eth0"

#define _FC_READ_HOLDING_REGISTERS    0x03

typedef struct SensorData {
    uint8_t  Address;
    uint8_t  Data;
    uint8_t  InactivityCounter;
    struct SensorData *NextSensor;
} SensorData;

extern SensorData *SensorsHead;

typedef struct {
    SensorData Sensors[SENSORS_COUNT];
    uint8_t SensorsCount;
} ConcentratorData;


typedef struct UartModule {
    char     PortId;
    int16_t  UartPortFd;
    uint8_t  EnablePin;
} UartModule;

extern UartModule _UartModuleA, _UartModuleB;

#endif
