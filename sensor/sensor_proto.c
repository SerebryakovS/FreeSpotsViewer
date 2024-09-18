
#include "sensor.h"

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

void BinaryToHex(const uint8_t *BinaryPacket, size_t BinaryPacketLength, char *HexStringOutput) {
    const char HexChars[] = "0123456789ABCDEF";
    for (size_t Idx = 0; Idx < BinaryPacketLength; ++Idx) {
        HexStringOutput[Idx * 2] = HexChars[(BinaryPacket[Idx] >> 4) & 0x0F];
        HexStringOutput[Idx * 2 + 1] = HexChars[BinaryPacket[Idx] & 0x0F];
    };
    HexStringOutput[BinaryPacketLength * 2] = '\0';
};

void SendToSensor(UartModule *_UartModule, uint8_t Address, uint8_t Command, const uint8_t *Data, size_t DataLength) {
    uint8_t Buffer[6];
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

void ReadFromSensor(UartModule *_UartModule, uint8_t *ReadBuffer, struct timeval *Timeout, uint8_t SlotIdx) {
    if (_UartModule->UartPortFd < 0) {
        fprintf(stderr, "Invalid UART file descriptor\n");
        return;
    };
	uint16_t FreeSpaces = 0;
	int16_t ReadCountBytes = 0;
    struct timeval StartTime, EndTime;
	gettimeofday(&StartTime, NULL);
	if (SlotIdx == SENSORS_COUNT - 1){
		if (IsMaster()) {
			FreeSpaces = CalculateTotalFreeSensorsCount();
		} else {
			FreeSpaces = CalculateFreeSensorsCount();
		};
		SendToSensor(_UartModule, DISPLAY_ADDR, CMD_DISP, &FreeSpaces, 2);
	} else {
		fd_set ReadFds;
		FD_ZERO(&ReadFds);
		FD_SET(_UartModule->UartPortFd, &ReadFds);
		int16_t SelectResult = select(_UartModule->UartPortFd + 1, &ReadFds, NULL, NULL, Timeout);
		if (SelectResult > 0) {
			ReadCountBytes = read(_UartModule->UartPortFd, ReadBuffer, 6);
			if (ReadCountBytes < 0) {
				perror("Unable to read UART port");
			};
		};
	};
	gettimeofday(&EndTime, NULL);
	uint32_t ElapsedUsRd = EndTime.tv_usec - StartTime.tv_usec;
	uint32_t ElapsedUsRw = ElapsedUsRd;
	if (SlotIdx != SENSORS_COUNT - 1 && ReadCountBytes >= 5) {
		char HexBuffer[ReadCountBytes * 2 + 1];
		BinaryToHex(ReadBuffer, ReadCountBytes, HexBuffer);
		SensorsProtoHandler(_UartModule, SlotIdx, ReadBuffer, ReadCountBytes);
		gettimeofday(&EndTime, NULL);
		ElapsedUsRw = EndTime.tv_usec - StartTime.tv_usec;
		fprintf(stdout, "SlotIdx: %d, Received packet: %s, ElapsedUsRd: %d, ElapsedUsRw: %d\n", SlotIdx, HexBuffer, ElapsedUsRd, ElapsedUsRw);        
    };
	if (SlotIdx != SENSORS_COUNT - 1){
		if ( ElapsedUsRw < SlotTimeUs ) {
			usleep(SlotTimeUs - ElapsedUsRw);
		};
	} else {
		if ( ElapsedUsRd < SlotTimeUs ) {
			usleep(SlotTimeUs - ElapsedUsRd);
		};
	};
};

void SyncAndRead(UartModule *_UartModule, uint8_t *ReadBuffer, struct timeval *Timeout) {
    const uint8_t Data[] = {GetLaddr()};	
    SendToSensor(_UartModule, 0xFF, CMD_SYNC, Data, 1);
    for (int SlotIdx = 0; SlotIdx < SENSORS_COUNT; ++SlotIdx) {
		ReadFromSensor(_UartModule, ReadBuffer, Timeout, SlotIdx);
    };
    CheckInactiveSensors(5);
};

void SensorsProtoHandler(UartModule *_UartModule, uint8_t SlotIdx, uint8_t *ReadBuffer, int16_t ReadCountBytes) {
    if (ReadBuffer[0] == MSG_START && ReadBuffer[ReadCountBytes - 1] == MSG_END) {
        uint8_t Address = ReadBuffer[1];
		if (Address != SlotIdx) {
			fprintf(stderr, "SlotIdx and DeviceId mismatch\n");
			tcflush(_UartModule->UartPortFd, TCIFLUSH);
			return;
		};
        uint8_t Command = ReadBuffer[2];
        uint8_t Checksum = ReadBuffer[ReadCountBytes - 2];
        uint8_t CalculatedChecksum = CalculateChecksum(ReadBuffer, ReadCountBytes - 2);
        if (Checksum == CalculatedChecksum) {
            switch (Command) {
                case CMD_DATA:
					UpdateLaddr(Address);
					uint8_t SensorValue = ReadBuffer[3];
                    UpdateSensor(Address, SensorValue);
                    break;
				default:
					break;
            };
        } else {
            fprintf(stderr, "Checksum error\n");
        };
    } else {
        fprintf(stderr, "Message format error\n");
		tcflush(_UartModule->UartPortFd, TCIFLUSH);
    };
};
