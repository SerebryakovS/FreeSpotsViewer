
#include "SpotTrack.h"

// compilation:  gcc Main.c Controller.c Server.c Json.c Cache.c SpotTrack.h -o out -lgpiod -lmicrohttpd

int WebToRs485RecvPipe[2], WebToRs485SendPipe[2], UartFd;

int main(int argc, char *argv[]) {
    InitCacheWorkingSpots();
    InitPipe(WebToRs485RecvPipe);
    InitPipe(WebToRs485SendPipe);
    if (argc == 1) {
        return RunWebServer();
    } else if (argc > 1 && strcmp(argv[1], "debug") == 0) {
        return RunRs485Controller(DEBUG_MODE);
    };
    return EXIT_SUCCESS;
};