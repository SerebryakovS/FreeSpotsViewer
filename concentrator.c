
#include "spotrack.h"

UartModule _UartModuleB = {0};

void *SyncConcentratorsHandler(void *Arguments) {
    UartModule *_UartModule = (UartModule *)Arguments;
    while (1) {
        if (IsMaster()) {
            digitalWrite(RS485_ROLE_LED_B, HIGH);
            RunModbusMaster(_UartModule);
        } else {
            digitalWrite(RS485_ROLE_LED_B, LOW);
            RunModbusSlave(_UartModule);
        };
    };
};

