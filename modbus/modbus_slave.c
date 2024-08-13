
#include "modbus.h"

void RunModbusSlave(int32_t SlaveId, UartModule *_UartModule) {
    if (SlaveId < 0) {
        fprintf(stderr, "Failed to get correct device Id\n");
        return;
    };
    modbus_t *ModbusContext;
    modbus_mapping_t *MbMapping;
    uint8_t Query[MODBUS_RTU_MAX_ADU_LENGTH];
    ModbusContext = modbus_new_rtu(RS485_UART_PORT_B, 9600, 'N', 8, 1);
    if (ModbusContext == NULL) {
        fprintf(stderr, "Unable to create the libmodbus context\n");
        return;
    };
    if (modbus_set_slave(ModbusContext, SlaveId) == -1) {
        fprintf(stderr, "Failed to set slave ID: %s\n", modbus_strerror(errno));
        modbus_free(ModbusContext);
        return;
    };
    struct timeval Timeout;
    Timeout.tv_sec = 1;
    Timeout.tv_usec = 0;
    modbus_set_response_timeout(ModbusContext, &Timeout);
    modbus_set_byte_timeout(ModbusContext, &Timeout);
    MbMapping = modbus_mapping_new(0, 0, SENSOR_ZERO_ADDR + SENSORS_COUNT, 0);
    if (MbMapping == NULL) {
        fprintf(stderr, "Failed to allocate the mapping: %s\n", modbus_strerror(errno));
        modbus_free(ModbusContext);
        return;
    };
    if (modbus_connect(ModbusContext) == -1) {
        fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
        modbus_mapping_free(MbMapping);
        modbus_free(ModbusContext);
        return;
    };
    printf("Running as Modbus Slave with ID %d...\n", SlaveId);
    digitalWrite(_UartModule->EnablePin, LOW);
    while (!IsMaster()) {
        fd_set ReadFds;
        FD_ZERO(&ReadFds);
        int SocketFd = modbus_get_socket(ModbusContext);
        FD_SET(SocketFd, &ReadFds);
        int16_t SelectResult = select(SocketFd + 1, &ReadFds, NULL, NULL, &Timeout);
        if (SelectResult == -1) {
            perror("Select failed");
            break;
        } else if (SelectResult > 0 && FD_ISSET(SocketFd, &ReadFds)) {
            int RequestLength = modbus_receive(ModbusContext, Query);
            if (RequestLength > 0) {
                PrettyPrintModbusMessage(Query, RequestLength, "Master Request:");

                SensorData *CurrSensor = SensorsHead;
                uint8_t Idx = 0;
                while (CurrSensor != NULL && Idx < SENSORS_COUNT) {
                    if (CurrSensor->Data) {
                        MbMapping->tab_registers[SENSOR_ZERO_ADDR + Idx] = CurrSensor->Data;
                    }
                    CurrSensor = CurrSensor->NextSensor;
                    Idx++;
                }

                if (Idx == 0) {
                    for (uint8_t Idx = 0; Idx < SENSORS_COUNT; Idx++) {
                        MbMapping->tab_registers[SENSOR_ZERO_ADDR + Idx] = 1;
                    }
                }

                digitalWrite(_UartModule->EnablePin, HIGH);
                int ReplyLength = modbus_reply(ModbusContext, Query, RequestLength, MbMapping);
                PrettyPrintModbusMessage(Query, ReplyLength, "Slave Response:");
                tcdrain(modbus_get_socket(ModbusContext));
                digitalWrite(_UartModule->EnablePin, LOW);

            } else if (RequestLength == -1 && errno != ETIMEDOUT) {
                fprintf(stderr, "Modbus receive error: %s\n", modbus_strerror(errno));
            };
        };
    };
    modbus_mapping_free(MbMapping);
    modbus_close(ModbusContext);
    modbus_free(ModbusContext);
};
