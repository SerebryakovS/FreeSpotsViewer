
#ifndef SPOTRACK_H
#define SPOTRACK_H

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
#include <net/if.h>
#include <arpa/inet.h>

#include "modbus/modbus.h"
#include "sensor/sensor.h"

// #define TEST_MODE

extern UartModule _UartModuleA, _UartModuleB;

void *SyncClientsHandler(void *Arguments);
void *SyncConcentratorsHandler(void *Arguments);

#endif
