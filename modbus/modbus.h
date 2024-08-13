 
#ifndef MODBUS_H
#define MODBUS_H

#include "config.h"

#include <wiringPi.h>
#include <stdio.h>
#include <time.h>
#include <modbus.h>
#include <sys/select.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/time.h>
#include <stdint.h>

void RunModbusSlave(int32_t SlaveId, UartModule *_UartModule);
void RunModbusMaster(UartModule *_UartModule);

void PrettyPrintModbusMessage(uint8_t *ModbusMessage, int Length, const char *MessageType);
uint8_t SendModbusRequest(UartModule *_UartModule, modbus_t *ModbusContext, uint8_t SlaveId,
                          uint8_t Function, uint8_t Address, uint8_t DataCount, uint8_t *RequestBody);

bool IsMaster();

#endif
