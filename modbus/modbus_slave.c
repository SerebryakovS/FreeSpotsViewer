
#include "modbus.h"

void RunModbusSlave(UartModule *_UartModule) {
    int32_t SlaveId = GetSlaveId();
    if (SlaveId < 0) {
        return;
    };
    modbus_t *ModbusContext;
    modbus_mapping_t *MbMapping;
    uint8_t Query[MODBUS_RTU_MAX_ADU_LENGTH];
    ModbusContext = modbus_new_rtu(RS485_UART_PORT_B, 9600, 'N', 8, 1);
    if (ModbusContext == NULL) {
        return;
    };
    //modbus_set_debug(ModbusContext, TRUE);
    if (modbus_set_slave(ModbusContext, SlaveId) == -1) {
        modbus_free(ModbusContext);
        return;
    };
    struct timeval Timeout;
    Timeout.tv_sec  = 3;
    Timeout.tv_usec = 0;
    modbus_set_response_timeout(ModbusContext, &Timeout);
    modbus_set_byte_timeout(ModbusContext, &Timeout);
    MbMapping = modbus_mapping_new(0, 0, 0x0100, 0);
    if (MbMapping == NULL) {
        modbus_free(ModbusContext);
        return;
    };
    if (modbus_connect(ModbusContext) == -1) {
        modbus_mapping_free(MbMapping);
        modbus_free(ModbusContext);
        return;
    };
	uint8_t SensorAvailable[SENSORS_COUNT] = {0};
    printf("Running as Modbus Slave with ID %d...\n", SlaveId);
    digitalWrite(_UartModule->EnablePin, LOW);
    while (!IsMaster()) {
        fd_set ReadFds;
        FD_ZERO(&ReadFds);
        int SocketFd = modbus_get_socket(ModbusContext);
        FD_SET(SocketFd, &ReadFds);
        int16_t SelectResult = select(SocketFd + 1, &ReadFds, NULL, NULL, &Timeout);
        memset(Query, 0, sizeof(Query));
        if (SelectResult == -1) {
            break;
        } else if (SelectResult > 0 && FD_ISSET(SocketFd, &ReadFds)) {
            int RequestLength = modbus_receive(ModbusContext, Query);
            if (RequestLength > 0) {
                uint16_t start_address = (Query[2] << 8) + Query[3];
                uint16_t quantity = (Query[4] << 8) + Query[5];
                if ((start_address + quantity) > 0x0100 || quantity > 64) {
                    continue;
                };
				memset(MbMapping->tab_registers, 0xFF, SENSORS_COUNT * sizeof(uint16_t));
                memset(SensorAvailable, 0, SENSORS_COUNT);
                SensorData *CurrSensor = ExtractSensorDataFromMemcached(SlaveId, SENSORS_COUNT);
                uint8_t Idx = 0;
                while (CurrSensor != NULL) {
                    MbMapping->tab_registers[CurrSensor->Address] = CurrSensor->Data;
                    SensorAvailable[CurrSensor->Address] = 1;
                    CurrSensor = CurrSensor->NextSensor;
                };
                for (uint8_t Idx = 0; Idx < SENSORS_COUNT; Idx++) {
                    if (!SensorAvailable[Idx]) {
                        MbMapping->tab_registers[Idx] = 0xFF;
                    };
                };
                digitalWrite(_UartModule->EnablePin, HIGH);
                modbus_reply(ModbusContext, Query, RequestLength, MbMapping);
                tcdrain(modbus_get_socket(ModbusContext));
                digitalWrite(_UartModule->EnablePin, LOW);
            } else {
                if (errno == EMBBADCRC) {
                    tcflush(modbus_get_socket(ModbusContext), TCIFLUSH);
                };
            };
        };
    };
    modbus_mapping_free(MbMapping);
    modbus_close(ModbusContext);
    modbus_free(ModbusContext);
};
