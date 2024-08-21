
#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
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
#include <modbus.h>

#define SENSORS_COUNT               64
#define BSLAVES_COUNT               16

#define RS485_UART_PORT_A "/dev/ttyS1"
#define RS485_CTRL_PIN_A             4
#define RS485_ROLE_LED_B            14

#define RS485_UART_PORT_B "/dev/ttyS2"
#define RS485_CTRL_PIN_B             5
#define RS485_ROLE_PIN_B            10

#define CONC_INET_IFACE         "eth0"

typedef struct UartModule {
    char     PortId;
    int16_t  UartPortFd;
    uint8_t  EnablePin;
} UartModule;

#endif
