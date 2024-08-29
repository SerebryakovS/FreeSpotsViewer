
#include "web.h"

static int HandleGetRequest(struct MHD_Connection *Connection, const char* Url) {
    const char* ResponseStr = NULL;
    if (strncmp(Url, API_GET_SLAVES, strlen(API_GET_SLAVES)) == 0) {
        ResponseStr = GetSlaves();
    } else if (strncmp(Url, API_GET_SLAVE_SENSORS, strlen(API_GET_SLAVE_SENSORS)) == 0) {
        const char* SlaveId = MHD_lookup_connection_value(Connection, MHD_GET_ARGUMENT_KIND, "slave_id");
        if (SlaveId != NULL) {
            ResponseStr = GetSlaveSensors(SlaveId);
        } else {
            ResponseStr = "{\"error\":\"Missing slave_id parameter\"}\n";
        };
    } else {
        ResponseStr = "{\"error\":\"Unknown endpoint\"}\n";
    };
    if (ResponseStr == NULL) {
        ResponseStr = "{\"error\":\"Internal Server Error\"}\n";
    };
    struct MHD_Response *Response = MHD_create_response_from_buffer(strlen(ResponseStr),
                                                                    (void*)ResponseStr,
                                                                    MHD_RESPMEM_MUST_COPY);
    int ReturnValue = MHD_queue_response(Connection, MHD_HTTP_OK, Response);
    MHD_destroy_response(Response);
    return ReturnValue;
};

static int WebRequestsHandler(void *cls, struct MHD_Connection *Connection,
                              const char *Url, const char *Method,
                              const char *Version, const char *UploadData,
                              size_t *UploadDataSize, void **ConCls) {
	if (strcmp(Method, "GET") == 0) {
        return HandleGetRequest(Connection, Url);
    };
    return MHD_NO;
};

static struct MHD_Daemon *Daemon = NULL;
static bool WebServerRunFlag = false;

int32_t RunWebServer() {
    if (!WebServerRunFlag) {
        Daemon = MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION, REST_PORT,
                                  NULL, NULL, &WebRequestsHandler, NULL,
                                  MHD_OPTION_NOTIFY_COMPLETED, NULL, NULL,
                                  MHD_OPTION_END);
        if (Daemon != NULL) {
            WebServerRunFlag = true;
            printf("WebServer started on port: %d\n", REST_PORT);
        } else {
            fprintf(stderr, "Failed to start WebServer\n");
        };
    };
};

void StopWebServer(void) {
    if (WebServerRunFlag && Daemon != NULL) {
        MHD_stop_daemon(Daemon);
        WebServerRunFlag = false;
        printf("WebServer stopped.\n");
    };
};

bool IsWebServerRunning(void) {
    return WebServerRunFlag;
};