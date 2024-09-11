
#include "sensor.h"

static uint8_t LADDR;

void ExtractLaddrEnv() {
    char *LaddrValue = getenv("LADDR");
    if (LaddrValue != NULL) {
        LADDR = atoi(LaddrValue);
    } else {
		LADDR = SENSOR_ZERO_ADDR + 1;
    };
};

void StoreLaddrEnv() {
    char ShellCommand[512];
    snprintf(ShellCommand, sizeof(ShellCommand),
             "grep -q '^export %s=' ~/.bashrc && "
             "sed -i 's/^export %s=.*/export %s=%d/' ~/.bashrc || "
             "echo 'export %s=%d' >> ~/.bashrc",
             "LADDR", "LADDR", "LADDR", LADDR, "LADDR", LADDR);
    system(ShellCommand);
    system("source ~/.bashrc");
};

void UpdateLaddr(uint8_t Address){
	if (Address == LADDR){
		LADDR++;
	} else if (Address > LADDR) {
		LADDR = ++Address;
	};
	StoreLaddrEnv();
};

uint8_t GetLaddr(){
	return LADDR;
};