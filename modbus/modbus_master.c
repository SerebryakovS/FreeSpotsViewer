 
#include "modbus.h"

void RunModbusMaster(UartModule *_UartModule) {
    modbus_t *ModbusContext;
    uint8_t ModbusRequest[MODBUS_RTU_MAX_ADU_LENGTH];
    uint8_t ModbusResponse[MODBUS_RTU_MAX_ADU_LENGTH];
    ModbusContext = modbus_new_rtu(RS485_UART_PORT_B, 9600, 'N', 8, 1);
    if (ModbusContext == NULL) {
        return;
    };
    // modbus_set_debug(ModbusContext, TRUE);
    if (modbus_connect(ModbusContext) == -1) {
        modbus_free(ModbusContext);
        return;
    };
    struct timeval Timeout;
    Timeout.tv_sec = 0;
    Timeout.tv_usec = 150000;
    modbus_set_response_timeout(ModbusContext, &Timeout);
    printf("B-Line Master running...\n");
    digitalWrite(_UartModule->EnablePin, LOW);
    while (IsMaster()) {
        for (uint8_t SlaveId = 1; SlaveId <= BSLAVES_COUNT; ++SlaveId) {
            memset(ModbusResponse, 0, sizeof(ModbusResponse));
            if (modbus_set_slave(ModbusContext, SlaveId) == -1) {
                continue;
            };
            int ReadCountBytes = -1;
            int8_t SendCountBytes = SendModbusRequest(
                _UartModule, ModbusContext, SlaveId, _FC_READ_HOLDING_REGISTERS, 0x0000, SENSORS_COUNT, ModbusRequest);
            if (SendCountBytes == -1) {
                continue;
            };
            ReadCountBytes = modbus_receive_confirmation(ModbusContext, ModbusResponse);
            if (ReadCountBytes > 0) {
                uint8_t ByteCount = ModbusResponse[2];
                SensorData *IncomingData = malloc(sizeof(SensorData) * SENSORS_COUNT);
                for (uint8_t Idx = 0; Idx < 64 && Idx * 2 < ByteCount; ++Idx) {
                    IncomingData[Idx].Address = Idx;
                    IncomingData[Idx].Data = (ModbusResponse[3 + Idx * 2] << 8) | ModbusResponse[4 + Idx * 2];
                    IncomingData[Idx].InactivityCounter = 0;
                    IncomingData[Idx].NextSensor = (Idx < SENSORS_COUNT - 1) ? &IncomingData[Idx + 1] : NULL;
                };
                StoreSensorDataInMemcached(SlaveId, IncomingData);
                free(IncomingData);
            };
        };
    };
    modbus_close(ModbusContext);
    modbus_free(ModbusContext);
};
