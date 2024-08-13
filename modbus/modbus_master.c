 
#include "modbus.h"

void RunModbusMaster(UartModule *_UartModule) {
    modbus_t *ModbusContext;
    ConcentratorData Slaves[BSLAVES_COUNT] = {0};
    uint8_t ModbusRequest[MODBUS_RTU_MAX_ADU_LENGTH];
    uint8_t ModbusResponse[MODBUS_RTU_MAX_ADU_LENGTH];

    ModbusContext = modbus_new_rtu(RS485_UART_PORT_B, 9600, 'N', 8, 1);
    if (ModbusContext == NULL) {
        fprintf(stderr, "Unable to create the libmodbus context\n");
        return;
    };
    if (modbus_connect(ModbusContext) == -1) {
        fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
        modbus_free(ModbusContext);
        return;
    };
    struct timeval Timeout;
    Timeout.tv_sec = 0;
    Timeout.tv_usec = 1000000;
    modbus_set_response_timeout(ModbusContext, &Timeout);
    printf("B-Line Master running...\n");
    while (IsMaster()) {
        uint8_t SlaveId = 1;
        // for (uint8_t SlaveId = 1; SlaveId <= BSLAVES_COUNT; ++SlaveId) {
            if (modbus_set_slave(ModbusContext, SlaveId) == -1) {
                fprintf(stderr, "Failed to set slave ID: %s\n", modbus_strerror(errno));
                continue;
            };
            int ReadCountBytes = -1, RetryCount = 0;
            while (RetryCount < 3 && ReadCountBytes == -1) {
                int8_t SendCountBytes = SendModbusRequest(
                    _UartModule, ModbusContext, SlaveId, _FC_READ_HOLDING_REGISTERS, SENSOR_ZERO_ADDR, SENSORS_COUNT, ModbusRequest);
                if (SendCountBytes == -1) {
                    RetryCount++;
                    continue;
                }
                PrettyPrintModbusMessage(ModbusRequest, 8, "Master Request");
                ReadCountBytes = modbus_receive_confirmation(ModbusContext, ModbusResponse);
                if (ReadCountBytes == -1) {
                    fprintf(stderr, "Receive failed for slave %d: %s\n", SlaveId, modbus_strerror(errno));
                    RetryCount++;
                } else {
                    PrettyPrintModbusMessage(ModbusResponse, ReadCountBytes, "Slave Response");

                    if (ModbusResponse[1] != (_FC_READ_HOLDING_REGISTERS | 0x80)) {
                        uint8_t ByteCount = ModbusResponse[2];
                        for (uint8_t Idx = 0; Idx < SENSORS_COUNT && Idx * 2 < ByteCount; ++Idx) {
                            Slaves[SlaveId - 1].Sensors[Idx].Address = SENSOR_ZERO_ADDR + Idx;
                            Slaves[SlaveId - 1].Sensors[Idx].Data = (ModbusResponse[3 + Idx * 2] << 8) | ModbusResponse[4 + Idx * 2];
                        }
                    } else {
                        uint8_t ExceptionCode = ModbusResponse[2];
                        fprintf(stderr, "Received exception response from slave %d: %02X\n", SlaveId, ExceptionCode);
                    };
                    break;
                };
            };
        // };
    };
    modbus_close(ModbusContext);
    modbus_free(ModbusContext);
};
