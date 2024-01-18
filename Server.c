
#include "SpotTrack.h"
#include <microhttpd.h>

char WebResponseBuffer[WEB_RESPONSE_SIZE];

const char* GetListOfDevices() {
    FILE *KnownDevices = fopen("KnownDevices.json", "r");
    if (!KnownDevices) {
        snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"error\":\"FILE_ERR\"}");
        return WebResponseBuffer;
    };
    memset(WebResponseBuffer, 0, sizeof(WebResponseBuffer));
    size_t BytesRead = fread(WebResponseBuffer, 1, WEB_RESPONSE_SIZE - 1, KnownDevices);
    fclose(KnownDevices);
    if (BytesRead == 0) {
        snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"error\":\"READ_ERR\"}");
        return WebResponseBuffer;
    };
    WebResponseBuffer[BytesRead] = '\0';
    return WebResponseBuffer;
};

const char* GetDeviceStatus(const char* DeviceUid) {
    char Rs485Cmd[256];
    snprintf(Rs485Cmd, sizeof(Rs485Cmd), "{\"uid\":\"%s\",\"type\":\"get_status\"}", DeviceUid);
    write(Rs485WriteFd, Rs485Cmd, strlen(Rs485Cmd));
    sleep(1);
    memset(WebResponseBuffer, 0, sizeof(WebResponseBuffer));
    int BytesRead = read(Rs485ReadFd, WebResponseBuffer, sizeof(WebResponseBuffer) - 1);
    if (BytesRead > 0) {
        WebResponseBuffer[BytesRead] = '\0';
        return WebResponseBuffer;
    } else if (BytesRead < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
        snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"error\":\"READ_ERR\"}");
    } else {
        snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"error\":\"NO_DATA\"}");
    };
    return WebResponseBuffer;
}

const char* SetDeviceParked(const char* DeviceUid) {
    return "{\"is_parking_clear\":false}";
};

static int HandleGetRequest(const struct MHD_Connection *Connection, const char* Url) {
    const char* ResponseStr;
    if (strcmp(Url, "/list_devices") == 0) {
        ResponseStr = GetListOfDevices();
    } else if (strncmp(Url, "/get_status", strlen("/get_status")) == 0) {
        const char* DeviceUid = MHD_lookup_connection_value(Connection, MHD_GET_ARGUMENT_KIND, "device_uid");
        ResponseStr = GetDeviceStatus(DeviceUid);
    } else {
        ResponseStr = "{\"error\":\"Unknown endpoint\"}";
    };
    struct MHD_Response *Response = MHD_create_response_from_buffer(strlen(ResponseStr),
                                                                    (void*)ResponseStr, 
                                                                    MHD_RESPMEM_MUST_COPY);
    int ReturnValue = MHD_queue_response(Connection, MHD_HTTP_OK, Response);
    MHD_destroy_response(Response);
    return ReturnValue;
};

static int HandlePostRequest(const struct MHD_Connection *Connection, const char* Url) {
    const char* ResponseStr = SetDeviceParked(NULL); 
    struct MHD_Response *Response = MHD_create_response_from_buffer(strlen(ResponseStr),
                                                                    (void*)ResponseStr, 
                                                                    MHD_RESPMEM_MUST_COPY);
    int ReturnValue = MHD_queue_response(Connection, MHD_HTTP_OK, Response);
    MHD_destroy_response(Response);
    return ReturnValue;
};

static int AnswerToWebRequest(void *Cls, struct MHD_Connection *Connection,
                              const char *Url, const char *Method,
                              const char *Version, const char *UploadData,
                              size_t *UploadDataSize, void **ConCls) {
    if (strcmp(Method, "GET") == 0) {
        return HandleGetRequest(Connection, Url);
    } else if (strcmp(Method, "POST") == 0) {
        return HandlePostRequest(Connection, Url);
    };
    return MHD_NO;
};

void RunWebServer(){
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
}