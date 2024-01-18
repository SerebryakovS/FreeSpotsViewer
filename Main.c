
#include "SpotTrack.h"

// compilation: gcc Controller.c Server.c SpotTrack.h -o out -lgpiod

int main(int argc, char *argv[]) {
    if (argc == 1) {
        RunWebServer();
    } else if (argc > 1 && strcmp(argv[1], "debug") == 0) {
        RunRs485Controller(-1, -1);
    };
    return EXIT_SUCCESS;
}