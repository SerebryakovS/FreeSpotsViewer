 
#include "modbus.h"

int32_t GetSlaveId() {
    return 0x01;
};

void PrettyPrintModbusMessage(uint8_t *ModbusMessage, int Length, const char *MessageType) {
    printf("%s Message:\n", MessageType);
    printf("Raw Bytes: ");
    for (uint16_t Idx = 0; Idx < Length; Idx++) {
        printf("%02X ", ModbusMessage[Idx]);
    };
    printf("\n");
    if (Length >= 8) {
        printf("Slave ID: %d\n", ModbusMessage[0]);
        printf("Function Code: %d\n", ModbusMessage[1]);
        printf("Address: 0x%02X%02X\n", ModbusMessage[2], ModbusMessage[3]);
        printf("Quantity: %d\n", (ModbusMessage[4] << 8) | ModbusMessage[5]);
        printf("CRC: 0x%02X%02X\n", ModbusMessage[6], ModbusMessage[7]);
    } else if (Length >= 5) {
        printf("Slave ID: %d\n", ModbusMessage[0]);
        printf("Function Code: %d\n", ModbusMessage[1]);
        printf("Byte Count: %d\n", ModbusMessage[2]);
        for (uint16_t Idx = 3; Idx < Length - 2; Idx++) {
            printf("Data[%d]: 0x%02X\n", Idx - 3, ModbusMessage[Idx]);
        };
        printf("CRC: 0x%02X%02X\n", ModbusMessage[Length - 2], ModbusMessage[Length - 1]);
    } else {
        printf("Message length is too short for a valid Modbus message.\n");
    };
    printf("\n");
};

uint16_t CalculateCRC(uint8_t *IncomingData, int Length) {
    uint16_t CRC = 0xFFFF;
    for (uint16_t Position = 0; Position < Length; Position++) {
        CRC ^= (uint16_t)IncomingData[Position];
        for (uint8_t Idx = 0; Idx < 8; Idx++) {
            if ((CRC & 0x0001) != 0) {
                CRC >>= 1;
                CRC ^= 0xA001;
            } else {
                CRC >>= 1;
            };
        };
    };
    return CRC;
};

uint8_t SendModbusRequest(UartModule *_UartModule, modbus_t *ModbusContext, uint8_t SlaveId, uint8_t Function, uint8_t Address, uint8_t DataCount, uint8_t *ModbusRequest) {
    ModbusRequest[0] = SlaveId;
    ModbusRequest[1] = Function;
    ModbusRequest[2] = (Address >> 8) & 0xFF;
    ModbusRequest[3] = Address & 0xFF;
    ModbusRequest[4] = (DataCount >> 8) & 0xFF;
    ModbusRequest[5] = DataCount & 0xFF;
    int RequestLength = 6;
    uint16_t CRC = CalculateCRC(ModbusRequest, RequestLength);
    ModbusRequest[RequestLength++] = CRC & 0xFF;
    ModbusRequest[RequestLength++] = (CRC >> 8) & 0xFF;
    digitalWrite(_UartModule->EnablePin, HIGH);
    int16_t SendCountBytes = write(modbus_get_socket(ModbusContext), ModbusRequest, RequestLength);
    tcdrain(modbus_get_socket(ModbusContext));
    digitalWrite(_UartModule->EnablePin, LOW);
    if (SendCountBytes != RequestLength) {
        fprintf(stderr, "[send_modbus_request]: Failed to send request: %s\n", strerror(errno));
        return 0;
    };
    return 1;
};

bool IsMaster() {
    return digitalRead(RS485_ROLE_PIN_B) == 0;
};
