
#include "SpotTrack.h"

// compilation: gcc Main.c Controller.c Server.c SpotTrack.h -o out -lgpiod -lmicrohttpd

int Rs485ReadFd, Rs485WriteFd, UartFd;

int main(int argc, char *argv[]) {
    UartFd = SetupUart();
    if (argc == 1) {
        int PipeFds[2];
        if (pipe(PipeFds) == -1) {
            return -EXIT_FAILURE; 
        };
        Rs485ReadFd  = UartFd;
        Rs485WriteFd = PipeFds[1];
        return RunWebServer();
    } else if (argc > 1 && strcmp(argv[1], "debug") == 0) {
        Rs485ReadFd = -1;
        Rs485WriteFd = -1;
        return RunRs485Controller();
    };
    return EXIT_SUCCESS;
}