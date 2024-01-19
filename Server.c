
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

bool WriteToRs485(const char *command, char *ResponseBuffer, size_t ResponseBufferSize) {
    int BytesWrite = write(WebToRs485WriteFd, command, strlen(command));
    if (BytesWrite <= 0) {
        snprintf(ResponseBuffer, ResponseBufferSize, "{\"error\":\"WRITE_ERR\"}");
        return false;
    };
    return true;
};

bool ReadFromUart(char *ResponseBuffer, size_t ResponseBufferSize) {
    char TempBuffer[256];
    int BytesRead = read(UartFd, TempBuffer, sizeof(TempBuffer) - 1);
    if (BytesRead > 0) {
        TempBuffer[BytesRead] = '\0';
        return ExtractJson(TempBuffer, BytesRead, ResponseBuffer, ResponseBufferSize);
    } else if (BytesRead < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
        snprintf(ResponseBuffer, ResponseBufferSize, "{\"error\":\"READ_ERR\"}");
    } else {
        snprintf(ResponseBuffer, ResponseBufferSize, "{\"error\":\"NO_DATA\"}");
    };
    return false;
};

const char* GetDeviceStatus(const char* DeviceUid) {
    char Rs485Cmd[256];
    snprintf(Rs485Cmd, sizeof(Rs485Cmd), "{\"uid\":\"%s\",\"type\":\"get_status\"}\n", DeviceUid);
    int BytesWrite = write(WebToRs485WriteFd, Rs485Cmd, strlen(Rs485Cmd));
    if (BytesWrite <= 0) {
        snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"error\":\"WRITE_ERR\"}");
    } else {
        memset(WebResponseBuffer, 0, sizeof(WebResponseBuffer));
        char TempBuffer[256];
        int BytesRead = read(UartFd, TempBuffer, sizeof(TempBuffer) - 1);
        if (BytesRead > 0) {
            WebResponseBuffer[BytesRead] = '\0';
            if (ExtractJson(TempBuffer, BytesRead, WebResponseBuffer, RS485_BUFFER_LEN)){
                char PrettyJson[WEB_RESPONSE_SIZE];
                if (PrettyPrintJSON(WebResponseBuffer, PrettyJson, sizeof(PrettyJson)) == 0) {
                    return PrettyJson;
                } else {
                    snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"error\":\"PRINT_ERR\"}");
                };  
            } else {
                snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"error\":\"PARSE_ERR\"}");
            };
        } else if (BytesRead < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"error\":\"READ_ERR\"}");
        } else {
            snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"error\":\"NO_DATA\"}");
        };
    };
    return WebResponseBuffer;
};

const char* SetDeviceParked(const char* DeviceUid, const char* IsParkedSet) {
    char Command[256];
    snprintf(Command, sizeof(Command), "{\"uid\":\"%s\",\"type\":\"set_parked\",\"is_parked_set\":%s}\n", DeviceUid, IsParkedSet);
    char WebResponseBuffer[WEB_RESPONSE_SIZE];
    if (WriteToRs485(Command, WebResponseBuffer, sizeof(WebResponseBuffer))) {
        if (ReadFromUart(WebResponseBuffer, sizeof(WebResponseBuffer))) {
            char PrettyJson[WEB_RESPONSE_SIZE];
            if (PrettyPrintJSON(WebResponseBuffer, PrettyJson, sizeof(PrettyJson)) == 0) {
                return PrettyJson;
            } else {
                snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"error\":\"PRINT_ERR\"}");
            };
        };
    };
    return WebResponseBuffer;
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
    if (strcmp(Url, "/set_parked") == 0) {
        const char* DeviceUid = MHD_lookup_connection_value(Connection, MHD_POSTDATA_KIND, "uid");
        const char* IsParkedSet = MHD_lookup_connection_value(Connection, MHD_POSTDATA_KIND, "is_parked_set");
        if (DeviceUid != NULL && IsParkedSet != NULL) {
            const char* ResponseStr = SetDeviceParked(DeviceUid, IsParkedSet); 
            struct MHD_Response *Response = MHD_create_response_from_buffer(strlen(ResponseStr),
                                                                            (void*)ResponseStr, 
                                                                            MHD_RESPMEM_MUST_COPY);
            int ReturnValue = MHD_queue_response(Connection, MHD_HTTP_OK, Response);
            MHD_destroy_response(Response);
            return ReturnValue;
        } else {
            const char* ErrorResponse = "{\"error\":\"Missing parameters\"}";
            struct MHD_Response *Response = MHD_create_response_from_buffer(strlen(ErrorResponse),
                                                                            (void*)ErrorResponse, 
                                                                            MHD_RESPMEM_MUST_COPY);
            MHD_queue_response(Connection, MHD_HTTP_BAD_REQUEST, Response);
            MHD_destroy_response(Response);
            return MHD_HTTP_BAD_REQUEST;
        };
    };
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

int RunWebServer(){
        struct MHD_Daemon *Daemon = MHD_start_daemon(MHD_USE_INTERNAL_POLLING_THREAD, REST_PORT, 
                                                     NULL, NULL, &AnswerToWebRequest, NULL, 
                                                     MHD_OPTION_END);
        if (NULL == Daemon){ 
            return -EXIT_FAILURE;
        };
        printf("[%s][RX]: Server running on port: %d\n", PRINT_TAG, REST_PORT);
        RunRs485Controller();
        MHD_stop_daemon(Daemon);
}