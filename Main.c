
#include "SpotTrack.h"


int main() {
    struct MHD_Daemon *Daemon = MHD_start_daemon(MHD_USE_INTERNAL_POLLING_THREAD, REST_PORT, 
                                                 NULL, NULL, &AnswerToConnection, NULL, 
                                                 MHD_OPTION_END);
    if (NULL == Daemon){ 
        return -EXIT_FAILURE;
    };
    printf("Server running on port %d\n", REST_PORT);
    getchar();
    MHD_stop_daemon(Daemon);
    return EXIT_SUCCESS;
}