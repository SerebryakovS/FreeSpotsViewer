
#include "SpotTrack.h"

// compilation: gcc Controller.c Server.c SpotTrack.h -o out -lgpiod -lmicrohttpd

int main(int argc, char *argv[]) {
    UartFd = SetupUart();
    if (argc == 1) {
        int PipeFds[2];
        if (pipe(PipeFds) == -1) {
            return -EXIT_FAILURE; 
        };
        Rs485ReadFd  = UartFd;
        Rs485WriteFd = PipeFds[1];
        RunWebServer();
    } else if (argc > 1 && strcmp(argv[1], "debug") == 0) {
        Rs485ReadFd = -1;
        Rs485WriteFd = -1;
        RunRs485Controller();
    };
    return EXIT_SUCCESS;
}