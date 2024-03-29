
#include "SpotTrack.h"

Spot WorkingSpots[SPOTS_MAX_COUNT];

bool InitCacheWorkingSpots() {
    FILE *KnownDevices = fopen("KnownDevices", "r");
    if (!KnownDevices) {
        printf("[%s][ERR]: Could not open file: KnownDevices\n", PRINT_TAG);
        return false;
    }
    char DeviceUid[25]; int Idx = 0;
    for (int Idx=0; Idx < SPOTS_MAX_COUNT; Idx++){
        WorkingSpots[Idx].DeviceUid[0] = '\0';
        WorkingSpots[Idx].State = true;
    };
    while (fgets(DeviceUid, sizeof(DeviceUid), KnownDevices)) {
        DeviceUid[strcspn(DeviceUid, "\n")] = 0;
        if (strlen(DeviceUid) < 24){
            continue;
        };
        strncpy(WorkingSpots[Idx].DeviceUid, DeviceUid, sizeof(WorkingSpots[Idx].DeviceUid) - 1);
        printf("[%s][..]: Append device to list: %s\n", PRINT_TAG, WorkingSpots[Idx].DeviceUid); 
        Idx++;
        if (Idx >= sizeof(WorkingSpots) / sizeof(WorkingSpots[0])) {
            break;
        };
    };
    fclose(KnownDevices);
    return true;
};

int GetCacheFreeSpotsCount(){
    int FreeCounter = 0;
    for (int Idx = 0; Idx < SPOTS_MAX_COUNT && WorkingSpots[Idx].DeviceUid[0] != '\0'; Idx++) {
        if (WorkingSpots[Idx].State == true){
            FreeCounter++;
        };
    };
    return FreeCounter;
};

void SetCacheSpotState(char *DeviceUid, bool State){
    for (int Idx=0; Idx < SPOTS_MAX_COUNT; Idx++){
        if (strcmp(DeviceUid,WorkingSpots[Idx].DeviceUid) == 0){
            WorkingSpots[Idx].State = State;
        };
    };
};