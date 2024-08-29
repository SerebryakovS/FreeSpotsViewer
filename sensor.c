
#include "spotrack.h"

UartModule _UartModuleA = {0};

void *SyncClientsHandler(void *Arguments) {
    UartModule *_UartModule = (UartModule *)Arguments;
    uint8_t ReadBuffer[10];
    struct timeval Timeout;
    Timeout.tv_sec  = 0;
    Timeout.tv_usec = SlotTimeUs;
    while (1) {
        SyncAndRead(_UartModule, ReadBuffer, &Timeout);
        StoreSensorDataInMemcached(GetSlaveId(), SensorsHead);
		ExtractSensorDataFromMemcached(1,64);
    };
    return NULL;
};
