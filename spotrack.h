
#ifndef SPOTRACK_H
#define SPOTRACK_H

#include "config/config.h"
#include "modbus/modbus.h"
#include "sensor/sensor.h"

extern UartModule _UartModuleA, _UartModuleB;

void *SyncClientsHandler(void *Arguments);
void *SyncConcentratorsHandler(void *Arguments);

#endif
