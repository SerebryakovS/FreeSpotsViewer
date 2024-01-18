
#include "SpotTrack.h"


int main(int argc, char *argv[]) {
    if (argc == 1) {
        struct MHD_Daemon *Daemon = MHD_start_daemon(MHD_USE_INTERNAL_POLLING_THREAD, REST_PORT, 
                                                    NULL, NULL, &AnswerToWebRequest, NULL, 
                                                    MHD_OPTION_END);
        if (NULL == Daemon){ 
            return -EXIT_FAILURE;
        };
        printf("Server running on port %d\n", REST_PORT);

        int PipeFds[2];
        if (pipe(PipeFds) == -1) {
            return -EXIT_FAILURE; 
        };
        Rs485ReadFd  = UartFd;
        Rs485WriteFd = PipeFds[1];
        RunRs485Controller(Rs485ReadFd, Rs485WriteFd);
        MHD_stop_daemon(Daemon);
    } else if (argc > 1 && strcmp(argv[1], "debug") == 0) {
        RunRs485Controller(-1, -1);
    };
    return EXIT_SUCCESS;
}