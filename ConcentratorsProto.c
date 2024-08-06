
#include "Spotrack.h"

UartModule _UartModuleB = {0};

typedef struct {
    SensorData Sensors[SENSORS_COUNT];
    uint8_t SensorsCount;
} ConcentratorData;

int32_t GetSlaveId(/*const char *Interface*/) {
    // int32_t SocketFd;
    // struct ifreq Ifr;
    // unsigned char MacAddress[6];
    // SocketFd = socket(AF_INET, SOCK_DGRAM, 0);
    // if (SocketFd == -1) {
    //     return -EXIT_FAILURE;
    // };
    // strncpy(Ifr.ifr_name, Interface, IFNAMSIZ - 1);
    // if (ioctl(SocketFd, SIOCGIFHWADDR, &Ifr) == -1) {
    //     close(SocketFd);
    //     return -EXIT_FAILURE;
    // };
    // close(SocketFd);
    // memcpy(MacAddress, Ifr.ifr_hwaddr.sa_data, 6);
    // int32_t DeviceId = (MacAddress[2] << 24) | (MacAddress[3] << 16) | (MacAddress[4] << 8) | MacAddress[5];
    // printf("Unique Device ID: %d\n", DeviceId);
    // return DeviceId;
    return 0x03;
};

bool IsMaster() {
    return digitalRead(RS485_ROLE_PIN_B) == 0;
};

void RunModbusSlave(int32_t SlaveId, UartModule *_UartModule) {
    if (SlaveId < 0) {
        fprintf(stderr, "Failed to get correct device Id\n");
        return;
    }
    modbus_t *ModbusContext;
    modbus_mapping_t *MbMapping;
    uint8_t Query[MODBUS_RTU_MAX_ADU_LENGTH];

    ModbusContext = modbus_new_rtu(RS485_UART_PORT_B, 9600, 'N', 8, 1);
    if (ModbusContext == NULL) {
        fprintf(stderr, "Unable to create the libmodbus context\n");
        return;
    }
    if (modbus_set_slave(ModbusContext, SlaveId) == -1) {
        fprintf(stderr, "Failed to set slave ID: %s\n", modbus_strerror(errno));
        modbus_free(ModbusContext);
        return;
    }
    struct timeval Timeout;
    Timeout.tv_sec = 0;
    Timeout.tv_usec = 500000;  // 500 ms
    modbus_set_response_timeout(ModbusContext, &Timeout);
    modbus_set_byte_timeout(ModbusContext, &Timeout);

    MbMapping = modbus_mapping_new(0, 0, SENSORS_COUNT, 0);
    if (MbMapping == NULL) {
        fprintf(stderr, "Failed to allocate the mapping: %s\n", modbus_strerror(errno));
        modbus_free(ModbusContext);
        return;
    }

    if (modbus_connect(ModbusContext) == -1) {
        fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
        modbus_mapping_free(MbMapping);
        modbus_free(ModbusContext);
        return;
    }

    printf("Running as Modbus Slave with ID %d...\n", SlaveId);
    digitalWrite(_UartModule->EnablePin, LOW);

    while (!IsMaster()) {
        fd_set ReadFds;
        FD_ZERO(&ReadFds);
        int SocketFd = modbus_get_socket(ModbusContext);
        FD_SET(SocketFd, &ReadFds);
        int16_t SelectResult = select(SocketFd + 1, &ReadFds, NULL, NULL, &Timeout);

        if (SelectResult == -1) {
            break;
        } else if (SelectResult > 0 && FD_ISSET(SocketFd, &ReadFds)) {
            int RequestLength = modbus_receive(ModbusContext, Query);
            if (RequestLength > 0) {
                SensorData *CurrSensor = SensorsHead;
                uint8_t Idx = 0;
                while (CurrSensor != NULL && Idx < SENSORS_COUNT) {
                    if (CurrSensor->Data) { // Only consider active sensors
                        MbMapping->tab_registers[Idx] = CurrSensor->Data;
                    }
                    CurrSensor = CurrSensor->NextSensor;
                    Idx++;
                }
                digitalWrite(_UartModule->EnablePin, HIGH);
                modbus_reply(ModbusContext, Query, RequestLength, MbMapping);
                digitalWrite(_UartModule->EnablePin, LOW);
            } else if (RequestLength == -1 && errno != ETIMEDOUT) {
                fprintf(stderr, "Modbus receive error: %s\n", modbus_strerror(errno));
                break;
            }
        }
        usleep(100000);
    }

    modbus_mapping_free(MbMapping);
    modbus_close(ModbusContext);
    modbus_free(ModbusContext);
}

void RunModbusMaster(UartModule *_UartModule) {
    modbus_t *ModbusContext;
    ConcentratorData Slaves[BSLAVES_COUNT] = {0};
    uint16_t TabReg[SENSORS_COUNT];
    uint8_t request[MODBUS_RTU_MAX_ADU_LENGTH];
    uint8_t response[MODBUS_RTU_MAX_ADU_LENGTH];

    ModbusContext = modbus_new_rtu(RS485_UART_PORT_B, 9600, 'N', 8, 1);
    if (ModbusContext == NULL) {
        fprintf(stderr, "Unable to create the libmodbus context\n");
        return;
    }
    if (modbus_connect(ModbusContext) == -1) {
        fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
        modbus_free(ModbusContext);
        return;
    }
    printf("B-Line Master running...\n");

    while (IsMaster()) {
        for (uint8_t SlaveId = 1; SlaveId <= BSLAVES_COUNT; ++SlaveId) {
            if (modbus_set_slave(ModbusContext, SlaveId) == -1) {
                fprintf(stderr, "Failed to set slave ID: %s\n", modbus_strerror(errno));
                continue;
            }

            // Enable TX mode
            digitalWrite(_UartModule->EnablePin, HIGH);

            // Manually construct a read request for the slave
            int req_length = modbus_read_registers(ModbusContext, SENSOR_ZERO_ADDR, SENSORS_COUNT, TabReg);

            // Switch to RX mode
            digitalWrite(_UartModule->EnablePin, LOW);

            if (req_length == -1) {
                fprintf(stderr, "Failed to prepare request for slave %d: %s\n", SlaveId, modbus_strerror(errno));
                continue;
            }

            // Receive response
            int receive_length = modbus_receive_confirmation(ModbusContext, response);

            if (receive_length == -1) {
                fprintf(stderr, "Receive failed for slave %d: %s\n", SlaveId, modbus_strerror(errno));
            } else {
                // Process the response data
                for (uint8_t Idx = 0; Idx < SENSORS_COUNT; ++Idx) {
                    Slaves[SlaveId - 1].Sensors[Idx].Address = SENSOR_ZERO_ADDR + Idx;
                    Slaves[SlaveId - 1].Sensors[Idx].Data = TabReg[Idx];
                }
            }
        }
        usleep(100000);
    }

    modbus_close(ModbusContext);
    modbus_free(ModbusContext);
}


void *SyncConcentratorsHandler(void *Arguments) {
    UartModule *_UartModule = (UartModule *)Arguments;
    while (1) {
        if (IsMaster()) {
            digitalWrite(RS485_ROLE_LED_B, HIGH);
            RunModbusMaster(_UartModule);
        } else {
            digitalWrite(RS485_ROLE_LED_B, LOW);
            RunModbusSlave(GetSlaveId(/*CONC_INET_IFACE*/), _UartModule);
        };
    };
};

