
#include "config.h"

uint16_t GetSlaveId() {
	uint16_t Id = 0x00;
	Id |= (digitalRead(RS485_ADDR_PIN_B_1) ^ 0x01) << 0;
	Id |= (digitalRead(RS485_ADDR_PIN_B_2) ^ 0x01) << 1;
	Id |= (digitalRead(RS485_ADDR_PIN_B_3) ^ 0x01) << 2;
	Id |= (digitalRead(RS485_ADDR_PIN_B_4) ^ 0x01) << 3;
	Id |= (digitalRead(RS485_ADDR_PIN_B_5) ^ 0x01) << 4;
	return Id;
};

bool IsMaster() {
	bool ReturnRole = false;
	if (GetSlaveId() == 0x00){
		ReturnRole = true;
	};
	return ReturnRole;
};