
#ifndef SPOTRACK_H
#define SPOTRACK_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>

#include <sys/ioctl.h>

#include "config.h"

// #define TEST_MODE

SensorData OwnSensors[SENSORS_COUNT];

void *SyncClientsHandler(void *Arguments);
void *SyncConcentratorsHandler(void *Arguments);
extern uint8_t CalculateFreeSpaces();
#ifdef TEST_MODE
extern void SimulateSensorData();
#endif
#endif
