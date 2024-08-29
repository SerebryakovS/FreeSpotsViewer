
#include "spotrack.h"

UartModule _UartModuleB = {0};

void *SyncConcentratorsHandler(void *Arguments) {
    UartModule *_UartModule = (UartModule *)Arguments;
    while (1) {
        if (IsMaster()) {
            digitalWrite(RS485_ROLE_LED_B, HIGH);
            if (!IsWebServerRunning()) {
                RunWebServer();
            };
            RunModbusMaster(_UartModule);
        } else {
            digitalWrite(RS485_ROLE_LED_B, LOW);
            if (IsWebServerRunning()) {
				StopWebServer();
            };
            RunModbusSlave(_UartModule);
        };
    };
};

