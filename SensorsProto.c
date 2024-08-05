
#include "Spotrack.h"

UartModule _UartModuleA = {0};

void BinaryToHex(const uint8_t *bin, size_t bin_len, char *hex) {
    const char hex_chars[] = "0123456789ABCDEF";
    for (size_t i = 0; i < bin_len; ++i) {
        hex[i * 2] = hex_chars[(bin[i] >> 4) & 0x0F];
        hex[i * 2 + 1] = hex_chars[bin[i] & 0x0F];
    };
    hex[bin_len * 2] = '\0'; 
};

const uint32_t SlotTimeUs = SYNC_INTERVAL / SENSORS_COUNT;

uint8_t LastRegisteredAddress = 0x01;

static inline uint8_t CalculateChecksum(const uint8_t *Data, size_t Length) {
    uint8_t Checksum = 0;
    for (size_t Idx = 0; Idx < Length; ++Idx) {
        Checksum ^= Data[Idx];
    };
    return Checksum;
};

void ConstructMessage(uint8_t *Buffer, uint8_t Address, uint8_t Command, const uint8_t *Data, size_t DataLength) {
    Buffer[0] = MSG_START;
    Buffer[1] = Address;
    Buffer[2] = Command;
    memcpy(&Buffer[3], Data, DataLength);
    Buffer[3 + DataLength] = CalculateChecksum(Buffer, 3 + DataLength);
    Buffer[4 + DataLength] = MSG_END;
};

void UartWrite(UartModule *_UartModule, uint8_t Address, uint8_t Command, const uint8_t *Data, size_t DataLength) {
    uint8_t Buffer[10];
    ConstructMessage(Buffer, Address, Command, Data, DataLength);
    size_t MessageLength = 5 + DataLength;
    digitalWrite(_UartModule->EnablePin, HIGH);
    int16_t BytesWritten = write(_UartModule->UartPortFd, Buffer, MessageLength);
    if (BytesWritten < 0) {
        perror("Unable to write UART port");
    };
    tcdrain(_UartModule->UartPortFd);
    digitalWrite(_UartModule->EnablePin, LOW);
};

void UartRead(UartModule *_UartModule, uint8_t *ReadBuffer, struct timeval *Timeout, uint8_t SlotIdx) {
    if (_UartModule->UartPortFd < 0) {
        fprintf(stderr, "Invalid UART file descriptor\n");
        return;
    };
    fd_set ReadFds; FD_ZERO(&ReadFds); FD_SET(_UartModule->UartPortFd, &ReadFds);
    int16_t SelectResult = select(_UartModule->UartPortFd + 1, &ReadFds, NULL, NULL, Timeout);
    if (SelectResult > 0) {
        int16_t ReadCountBytes = read(_UartModule->UartPortFd, ReadBuffer, 10);
        //printf("ReadCountBytes = %d, Buffer:%s\n", ReadCountBytes, ReadBuffer);
        if (ReadCountBytes < 0) {
            perror("Unable to read UART port");
        } else if (ReadCountBytes > 0) {
            char HexBuffer[ReadCountBytes * 2 + 1];
            BinaryToHex(ReadBuffer, ReadCountBytes, HexBuffer);
            fprintf(stdout,"SlotIdx: %d, Received packet: %s\n", SlotIdx, HexBuffer);
            if (ReadBuffer[0] == MSG_START && ReadBuffer[ReadCountBytes - 1] == MSG_END) {
                uint8_t Address = ReadBuffer[1];
                uint8_t Command = ReadBuffer[2];
                uint8_t Checksum = ReadBuffer[ReadCountBytes - 2];
                uint8_t CalculatedChecksum = CalculateChecksum(ReadBuffer, ReadCountBytes - 2);
                if (Checksum == CalculatedChecksum) {
                    switch (Command) {
                        case CMD_DATA:
                            if (Address == SENSOR_ZERO_ADDR) {
                                uint8_t NewAddress = LastRegisteredAddress++;
                                UartWrite(_UartModule, SENSOR_ZERO_ADDR, CMD_SET_ID, &NewAddress, 1);
                            } else {
                                OwnSensors[Address - 1].Address = Address;
                                OwnSensors[Address - 1].Data = ReadBuffer[3];
                            };
                            UartWrite(_UartModule, Address, CMD_ACK, NULL, 0);
                            break;
                    };
                } else {
                    fprintf(stderr, "Checksum error\n");
                };
            } else {
                fprintf(stderr, "Message format error\n");
            };
        };
    };
};

void SyncAndRead(UartModule *_UartModule, uint8_t *ReadBuffer, struct timeval *Timeout) {
    const uint8_t Data[] = {0}; // No additional data for SYNC
    struct timeval StartTime, EndTime;
    uint32_t ElapsedUs;
    UartWrite(_UartModule, 0xFF, CMD_SYNC, Data, 0);
    for (int Idx = 0; Idx < SENSORS_COUNT; ++Idx) {
        gettimeofday(&StartTime, NULL);
        UartRead(_UartModule, ReadBuffer, Timeout, Idx);
        gettimeofday(&EndTime, NULL);
        ElapsedUs = (EndTime.tv_sec - StartTime.tv_sec) * 1000000 + (EndTime.tv_usec - StartTime.tv_usec);
        if ( ElapsedUs < SlotTimeUs ) {
            usleep(SlotTimeUs - ElapsedUs);
        };
    };
};

void *SyncClientsHandler(void *Arguments) {
    UartModule *_UartModule = (UartModule *)Arguments;
    uint8_t ReadBuffer[10];
    struct timeval Timeout;
    Timeout.tv_sec  = 0;
    Timeout.tv_usec = SlotTimeUs;
    while (1) {
        SyncAndRead(_UartModule, ReadBuffer, &Timeout);
    };
    return NULL;
};

void GetSensorsStatus( void ) {
    uint8_t freeSpotsCount = 0;
    for (int Idx = 0; Idx < SENSORS_COUNT; ++Idx) {
        if (OwnSensors[Idx].Data == 1) {
            freeSpotsCount++;
        };
    };
    printf("Busy Spots Count: %d\n", freeSpotsCount);
};

void *SensorHandler(void *Arguments) {
    UartModule *SensorUart = (UartModule *)Arguments;
    uint8_t ReadBuffer[10];
    uint8_t SlaveId = 0x00;
    uint8_t DataValue = 1;
    uint8_t SyncReceived = 0;
    while (1) {
        int16_t ReadCountBytes = read(SensorUart->UartPortFd, ReadBuffer, sizeof(ReadBuffer));
        if (ReadCountBytes > 0) {
            if (ReadBuffer[0] == MSG_START && ReadBuffer[ReadCountBytes - 1] == MSG_END) {
                uint8_t Command = ReadBuffer[2];
                uint8_t Checksum = ReadBuffer[ReadCountBytes - 2];
                uint8_t CalculatedChecksum = CalculateChecksum(ReadBuffer, ReadCountBytes - 2);
                if (Checksum == CalculatedChecksum) {
                    switch (Command) {
                        case CMD_SYNC:
                            //printf("CMD_SYNC\n");
                            SyncReceived = 1;
                            break;
                        case CMD_SET_ID:
                            //printf("CMD_SET_ID\n");
                            SlaveId = ReadBuffer[3];
                            break;
                        case CMD_ACK:
                            //printf("CMD_ACK\n");
                            break;
                    };
                };
            };
        };
        if (SyncReceived) {
            SyncReceived = 0;
            usleep(SlaveId * SlotTimeUs);
            UartWrite(SensorUart, SlaveId, CMD_DATA, &DataValue, 1);
        };
    };
    return NULL;
};


