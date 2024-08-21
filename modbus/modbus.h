 
#ifndef MODBUS_H
#define MODBUS_H

#include "config.h"
#include "memcached.h"

#define _FC_READ_HOLDING_REGISTERS    0x03

void RunModbusSlave(UartModule *_UartModule);
void RunModbusMaster(UartModule *_UartModule);

void PrettyPrintModbusMessage(uint8_t *ModbusMessage, int Length, const char *MessageType);
uint8_t SendModbusRequest(UartModule *_UartModule, modbus_t *ModbusContext, uint8_t SlaveId,
                          uint8_t Function, uint8_t Address, uint8_t DataCount, uint8_t *RequestBody);

int32_t GetSlaveId();
bool IsMaster();
#endif
