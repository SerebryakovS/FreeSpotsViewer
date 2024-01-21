
#include "SpotTrack.h"
#include <microhttpd.h>

char WebResponseBuffer[WEB_RESPONSE_SIZE];

struct PostRequest {
    char* Data; size_t Size;
};

// const char* GetListOfDevices() {
//     FILE *KnownDevices = fopen("KnownDevices.JsonObject", "r");
//     if (!KnownDevices) {
//         snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"error\":\"FILE_ERR\"}\n");
//         return WebResponseBuffer;
//     };
//     memset(WebResponseBuffer, 0, sizeof(WebResponseBuffer));
//     size_t BytesRead = fread(WebResponseBuffer, 1, WEB_RESPONSE_SIZE - 1, KnownDevices);
//     fclose(KnownDevices);
//     if (BytesRead == 0) {
//         snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"error\":\"READ_ERR\"}\n");
//         return WebResponseBuffer;
//     };
//     WebResponseBuffer[BytesRead] = '\0';
//     return WebResponseBuffer;
// };
//
// bool WriteToRs485(const char *command, char *ResponseBuffer, size_t ResponseBufferSize) {
//     int BytesWrite = write(WebToRs485WriteFd, command, strlen(command));
//     if (BytesWrite <= 0) {
//         snprintf(ResponseBuffer, ResponseBufferSize, "{\"error\":\"WRITE_ERR\"}\n");
//         return false;
//     };
//     return true;
// };
//
// bool ReadFromUart(char *ResponseBuffer, size_t ResponseBufferSize) {
//     char TempBuffer[256];
//     int BytesRead = read(UartFd, TempBuffer, sizeof(TempBuffer) - 1);
//     if (BytesRead > 0) {
//         TempBuffer[BytesRead] = '\0';
//         return ExtractJson(TempBuffer, BytesRead, ResponseBuffer, ResponseBufferSize);
//     } else if (BytesRead < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
//         snprintf(ResponseBuffer, ResponseBufferSize, "{\"error\":\"READ_ERR\"}\n");
//     } else {
//         snprintf(ResponseBuffer, ResponseBufferSize, "{\"error\":\"NO_DATA\"}\n");
//     };
//     return false;
// };
//
// const char* GetDeviceStatus(const char* DeviceUid) {
//     char Rs485Cmd[256];
//     snprintf(Rs485Cmd, sizeof(Rs485Cmd), "{\"uid\":\"%s\",\"type\":\"get_status\"}\n", DeviceUid);
//     int BytesWrite = write(WebToRs485WriteFd, Rs485Cmd, strlen(Rs485Cmd));
//     if (BytesWrite <= 0) {
//         snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"error\":\"WRITE_ERR\"}\n");
//     } else {
//         memset(WebResponseBuffer, 0, sizeof(WebResponseBuffer));
//         char TempBuffer[256];
//         int BytesRead = read(UartFd, TempBuffer, sizeof(TempBuffer) - 1);
//         if (BytesRead > 0) {
//             WebResponseBuffer[BytesRead] = '\0';
//             if (ExtractJson(TempBuffer, BytesRead, WebResponseBuffer, RS485_BUFFER_LEN)){
//                 char PrettyJson[WEB_RESPONSE_SIZE];
//                 if (PrettyPrintJSON(WebResponseBuffer, PrettyJson, sizeof(PrettyJson)) == 0) {
//                     return PrettyJson;
//                 } else {
//                     snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"error\":\"PRINT_ERR\"}\n");
//                 };
//             } else {
//                 snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"error\":\"PARSE_ERR\"}\n");
//             };
//         } else if (BytesRead < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
//             snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"error\":\"READ_ERR\"}\n");
//         } else {
//             snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"error\":\"NO_DATA\"}\n");
//         };
//     };
//     return WebResponseBuffer;
// };

const char* SetDeviceParked(const char* DeviceUid, const char* IsParkedSet) {
    char Rs485Cmd[256];
    snprintf(Rs485Cmd, sizeof(Rs485Cmd), "{\"uid\":\"%s\",\"type\":\"set_parked\",\"is_parked_set\":%s}\n", DeviceUid, IsParkedSet);
    // if (WriteToRs485(Rs485Cmd, WebResponseBuffer, sizeof(WebResponseBuffer))) {
    //     if (ReadFromUart(WebResponseBuffer, sizeof(WebResponseBuffer))) {
    //         char PrettyJson[WEB_RESPONSE_SIZE];
    //         if (PrettyPrintJSON(WebResponseBuffer, PrettyJson, sizeof(PrettyJson)) == 0) {
    //             return PrettyJson;
    //         } else {
    //             snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\"error\":\"PRINT_ERR\"}\n");
    //         };
    //     };
    // };
    sprintf(WebResponseBuffer, "%s",Rs485Cmd);
    return WebResponseBuffer;
};

// static int HandleGetRequest(const struct MHD_Connection *Connection, const char* Url) {
//     const char* ResponseStr = NULL;
//     if (strcmp(Url, "/list_devices") == 0) {
//         ResponseStr = GetListOfDevices();
//     } else if (strncmp(Url, "/get_status", strlen("/get_status")) == 0) {
//         const char* DeviceUid = MHD_lookup_connection_value(Connection, MHD_GET_ARGUMENT_KIND, "device_uid");
//         ResponseStr = GetDeviceStatus(DeviceUid);
//     } else {
//         ResponseStr = "{\"error\":\"Unknown endpoint\"}\n";
//     };
    // if (ResponseStr == NULL) {
    //     ResponseStr = "{\"error\":\"Internal Server Error\"}\n";
    // };
//     struct MHD_Response *Response = MHD_create_response_from_buffer(strlen(ResponseStr),
//                                                                     (void*)ResponseStr,
//                                                                     MHD_RESPMEM_MUST_COPY);
//     int ReturnValue = MHD_queue_response(Connection, MHD_HTTP_OK, Response);
//     MHD_destroy_response(Response);
//     return ReturnValue;
// };


char* FindValueForKey(const char* key, const char* JsonObject) {
    char *p = strstr(JsonObject, key);
    if (!p) return NULL;
    p += strlen(key);
    while (*p == ' ' || *p == ':' || *p == '\"') ++p;
    return p;
};
static int HandlePostRequest(const struct MHD_Connection *Connection, const char* Url,
                             struct PostRequest *_PostRequest) {
    const char* ResponseStr = NULL;
    if (strcmp(Url, "/set_parked") == 0) {
        const char *JsonObject = _PostRequest->Data;
        const char *UidKey = "\"uid\""; const char *ParkedKey = "\"is_parked_set\"";
        char DeviceUid[25];
        bool IsParkedSet = false;

        char *UidStart = FindValueForKey(UidKey, JsonObject);
        char *ParkedStart = FindValueForKey(ParkedKey, JsonObject);
        if (!UidStart || !ParkedStart) {
            return MHD_HTTP_BAD_REQUEST;
        };
        char *UidEnd = strchr(UidStart, '\"');
        if (!UidEnd || UidEnd - UidStart >= sizeof(DeviceUid)) {
            return MHD_HTTP_BAD_REQUEST;
        };
        strncpy(DeviceUid, UidStart, UidEnd - UidStart);
        DeviceUid[UidEnd - UidStart] = '\0';
        IsParkedSet = strstr(ParkedStart, "true") != NULL;
        ResponseStr = SetDeviceParked(DeviceUid, IsParkedSet ? "true" : "false");
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
}


static void RequestCompleted(void *Cls, struct MHD_Connection *Connection,
                              void **ConCls, enum MHD_RequestTerminationCode Toe){
    struct PostRequest *_PostRequest = *ConCls;
    if (_PostRequest != NULL) {
        if (_PostRequest->Data != NULL){
            free(_PostRequest->Data);
        };
        free(_PostRequest);
    };
    *ConCls = NULL;
};

static int AnswerToWebRequest(void *cls, struct MHD_Connection *Connection,
                              const char *Url, const char *Method,
                              const char *Version, const char *UploadData,
                              size_t *UploadDataSize, void **ConCls) {
    if (strcmp(Method, "POST") == 0) {
        if (*ConCls == NULL) {
            struct PostRequest *_PostRequest = malloc(sizeof(struct PostRequest));
            if (_PostRequest == NULL){
                return MHD_NO;
            };
            _PostRequest->Data = NULL;
            _PostRequest->Size = 0;
            *ConCls = _PostRequest;
            return MHD_YES;
        }
        struct PostRequest *_PostRequest = *ConCls;
        if (*UploadDataSize != 0) {
            _PostRequest->Data = realloc(_PostRequest->Data, _PostRequest->Size + *UploadDataSize + 1);
            if (_PostRequest->Data == NULL)
                return MHD_NO;
            memcpy(_PostRequest->Data + _PostRequest->Size, UploadData, *UploadDataSize);
            _PostRequest->Size += *UploadDataSize;
            _PostRequest->Data[_PostRequest->Size] = '\0';
            *UploadDataSize = 0;
            return MHD_YES;
        } else if (_PostRequest->Data) {
            return HandlePostRequest(Connection, Url, _PostRequest);
        };
    } /*else if (strcmp(Method, "GET") == 0) {
        return HandleGetRequest(Connection, Url);
    };*/
    return MHD_NO;
};

int RunWebServer(){
        struct MHD_Daemon *Daemon = MHD_start_daemon(MHD_USE_INTERNAL_POLLING_THREAD, REST_PORT,
                                                     NULL, NULL, &AnswerToWebRequest, NULL,
                                                     MHD_OPTION_NOTIFY_COMPLETED, RequestCompleted, NULL,
                                                     MHD_OPTION_END);
        if (NULL == Daemon){ 
            return -EXIT_FAILURE;
        };
        printf("[%s][RX]: Server running on port: %d\n", PRINT_TAG, REST_PORT);
        //RunRs485Controller();
        getchar();
        MHD_stop_daemon(Daemon);
};

int main(){
    RunWebServer();
};
