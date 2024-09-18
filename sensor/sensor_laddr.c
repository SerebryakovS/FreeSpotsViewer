
#include "sensor.h"

static uint8_t LADDR;

void ExtractLaddrEnv() {
    char *LaddrValue = getenv("LADDR");
    if (LaddrValue != NULL) {
        LADDR = atoi(LaddrValue);
    } else {
		LADDR = SENSOR_ZERO_ADDR + 1;
    };
	printf("Current LADDR=%d\n", LADDR);
};

void StoreLaddrEnv() {
    char ShellCommand[512];
    snprintf(ShellCommand, sizeof(ShellCommand),
             "grep -q '^export %s=' ~/.bashrc && "
             "sed -i 's/^export %s=.*/export %s=%d/' ~/.bashrc || "
             "echo 'export %s=%d' >> ~/.bashrc",
             "LADDR", "LADDR", "LADDR", LADDR, "LADDR", LADDR);
    system(ShellCommand);
};

void UpdateLaddr(uint8_t Address){
	if (Address == LADDR){
		if (++LADDR >= SENSOR_MAX_ADDR){
			uint8_t GapId;
			do {
				GapId = PopSensorIdsGap();
			} while (GapId > SENSOR_ZERO_ADDR && GapId < SENSOR_MAX_ADDR && IsSensorAddressBusy(GapId));
			if (GapId > SENSOR_ZERO_ADDR && GapId < SENSOR_MAX_ADDR) {
				LADDR = GapId;
			};
		};
	} else if (Address > LADDR) {
		LADDR = ++Address;
	};
	StoreLaddrEnv();
};

uint8_t GetLaddr(){
	return LADDR;
};
