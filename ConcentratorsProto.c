 
#include "Spotrack.h"

UartModule _UartModuleB = {0};

typedef struct {
    SensorData Sensors[SENSORS_COUNT];
    uint8_t SensorsCount;
} ConcentratorData;

uint8_t GetSlaveId(){
    return 0x03;
};

bool IsMaster() {
    return digitalRead(RS485_ROLE_PIN_B) == 0;
};

void RunModbusSlave(uint8_t SlaveId, UartModule *_UartModule) {
    modbus_t *ModbusContext;
    modbus_mapping_t *MbMapping;
    uint8_t Query[MODBUS_RTU_MAX_ADU_LENGTH];
    ModbusContext = modbus_new_rtu(RS485_UART_PORT_B, 9600, 'N', 8, 1);
    modbus_set_slave(ModbusContext, SlaveId);
    MbMapping = modbus_mapping_new(0, 0, SENSORS_COUNT, 0);
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
    printf("Slave run\n");
    while (!IsMaster()) {
        int RequestLength = modbus_receive(ModbusContext, Query);
        if (RequestLength > 0) {
            SensorData *CurrSensor = SensorsHead;
            uint8_t Idx = 0;
            while (CurrSensor != NULL && Idx < SENSORS_COUNT) {
                MbMapping->tab_registers[Idx] = CurrSensor->Data;
                CurrSensor = CurrSensor->NextSensor;
                Idx++;
            };
            digitalWrite(_UartModule->EnablePin, HIGH);
            modbus_reply(ModbusContext, Query, RequestLength, MbMapping);
            digitalWrite(_UartModule->EnablePin, LOW);
        } else if (RequestLength == -1) {
            break;
        };
    };
    modbus_mapping_free(MbMapping);
    modbus_close(ModbusContext);
    modbus_free(ModbusContext);
};

void RunModbusMaster(UartModule *_UartModule) {
    modbus_t *ModbusContext;
    ConcentratorData Slaves[BSLAVES_COUNT] = {0};
    uint16_t TabReg[SENSORS_COUNT];
    ModbusContext = modbus_new_rtu(RS485_UART_PORT_B, 9600, 'N', 8, 1);
    if (modbus_connect(ModbusContext) == -1) {
        fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
        modbus_free(ModbusContext);
        return;
    };
    printf("Master run\n");
    while (IsMaster()) {
        for (uint8_t SlaveId = 1; SlaveId <= BSLAVES_COUNT; ++SlaveId) {
            if (modbus_set_slave(ModbusContext, SlaveId) == -1) {
                fprintf(stderr, "Failed to set slave ID: %s\n", modbus_strerror(errno));
                continue;
            };
            digitalWrite(_UartModule->EnablePin, HIGH);
            int RequestLength = modbus_read_registers(ModbusContext, SENSOR_ZERO_ADDR, SENSORS_COUNT, TabReg);
            digitalWrite(_UartModule->EnablePin, LOW);
            if (RequestLength == -1) {
                fprintf(stderr, "Read failed for slave %d: %s\n", SlaveId, modbus_strerror(errno));
            } else {
                Slaves[SlaveId - 1].SensorsCount = RequestLength;
                for (uint8_t Idx = 0; Idx < RequestLength; ++Idx) {
                    Slaves[SlaveId - 1].Sensors[Idx].Address = SENSOR_ZERO_ADDR + Idx;
                    Slaves[SlaveId - 1].Sensors[Idx].Data = TabReg[Idx];
                };
            };
        };
        usleep(100000);
    };
    modbus_close(ModbusContext);
    modbus_free(ModbusContext);
};

void *SyncConcentratorsHandler(void *Arguments) {
    UartModule *_UartModule = (UartModule *)Arguments;
    while (1) {
        if (IsMaster()) {
            digitalWrite(RS485_ROLE_LED_B, HIGH);
            RunModbusMaster(_UartModule);
        } else {
            digitalWrite(RS485_ROLE_LED_B, LOW);
            RunModbusSlave(GetSlaveId(), _UartModule);
        };
    };
};

