
#include "SpotTrack.h"

// compilation: gcc Main.c Controller.c Server.c SpotTrack.h -o out -lgpiod -lmicrohttpd

int WebToRs485ReadFd, WebToRs485WriteFd, UartFd;

int main(int argc, char *argv[]) {
    UartFd = SetupUart();
    if (UartFd < 0) {
        return -EXIT_FAILURE;
    };
    if (argc == 1) {
        int PipeFds[2];
        if (pipe(PipeFds) == -1) {
            return -EXIT_FAILURE; 
        };
        WebToRs485ReadFd  = PipeFds[0];
        WebToRs485WriteFd = PipeFds[1];
        return RunWebServer();
    } else if (argc > 1 && strcmp(argv[1], "debug") == 0) {
        WebToRs485ReadFd  = -1;
        return RunRs485Controller();
    };
    return EXIT_SUCCESS;
}