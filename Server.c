
#include "SpotTrack.h"
#include <microhttpd.h>

char WebResponseBuffer[WEB_RESPONSE_SIZE];

struct PostRequest {
    char* Data; size_t Size;
};
//
const char* GetListOfDevices() {
    const char *FormatStart = "{\n    \"list_devices\" :[\n";
    const char *FormatEnd = "\n    ]\n}";
    const char *FormatDevice = "        \"%s\"";
    memset(WebResponseBuffer, 0, WEB_RESPONSE_SIZE);
    strncat(WebResponseBuffer, FormatStart, WEB_RESPONSE_SIZE - strlen(WebResponseBuffer) - 1);
    for (int Idx = 0; Idx < SPOTS_MAX_COUNT && WorkingSpots[Idx].DeviceUid[0] != '\0'; Idx++) {
        char DeviceEntry[30];
        snprintf(DeviceEntry, sizeof(DeviceEntry), FormatDevice, WorkingSpots[Idx].DeviceUid);
        strncat(WebResponseBuffer, DeviceEntry, WEB_RESPONSE_SIZE - strlen(WebResponseBuffer) - 1);
        if (Idx < SPOTS_MAX_COUNT - 1 && WorkingSpots[Idx + 1].DeviceUid[0] != '\0') {
            strncat(WebResponseBuffer, ",\n", WEB_RESPONSE_SIZE - strlen(WebResponseBuffer) - 1);
        };
    };
    strncat(WebResponseBuffer, FormatEnd, WEB_RESPONSE_SIZE - strlen(WebResponseBuffer) - 1);
    return WebResponseBuffer;
};
//
const char* GetFreeSpotsCount() {
    int FreeSpotsCount = GetCacheFreeSpotsCount();
    snprintf(WebResponseBuffer, WEB_RESPONSE_SIZE, "{\n    \"free_spots_count\" : %d\n}", FreeSpotsCount);
    return WebResponseBuffer;
};
//
bool StrToBool(const char *StrBool) {
    if (strcmp(StrBool, "true") == 0) {
        return true;
    } else if (strcmp(StrBool, "false") == 0) {
        return false;
    };
    return false; 
};
//
const char* GetDeviceStatus(const char* DeviceUid) {
    char Rs485Cmd[256];
    snprintf(Rs485Cmd, sizeof(Rs485Cmd), "{\"uid\":\"%s\",\"type\":\"get_status\"}\n", DeviceUid);
    if (Rs485MakeIO(Rs485Cmd, WebResponseBuffer, sizeof(WebResponseBuffer))){
        const char *IsParkedSetKey = "\"is_parked_set\":";
        char *IsFound = strstr(jsonStr, IsParkedSetKey);
        if (IsFound) {
            IsFound += strlen(IsParkedSetKey);
            char StrBoolValue[6];
            if (sscanf(IsFound, " %5s", StrBoolValue) == 1) {
                SetCacheSpotState(DeviceUid, StrToBool(StrBoolValue));
            };
        };
    };
    return WebResponseBuffer;
};
//
const char* SetDeviceParked(const char* DeviceUid, const char* IsParkedSet) {
    char Rs485Cmd[256];
    snprintf(Rs485Cmd, sizeof(Rs485Cmd), "{\"uid\":\"%s\",\"type\":\"set_parked\",\"is_parked_set\":%s}\n", DeviceUid, IsParkedSet);
    if (Rs485MakeIO(Rs485Cmd, WebResponseBuffer, sizeof(WebResponseBuffer))){
        SetCacheSpotState(DeviceUid, StrToBool(IsParkedSet));
    };
    return WebResponseBuffer;
};
//
static int HandleGetRequest(const struct MHD_Connection *Connection, const char* Url) {
    const char* ResponseStr = NULL;
    if (strcmp(Url, "/list_devices") == 0) {
        ResponseStr = GetListOfDevices();
    } else if (strncmp(Url, "/get_status", strlen("/get_status")) == 0) {
        const char* DeviceUid = MHD_lookup_connection_value(Connection, MHD_GET_ARGUMENT_KIND, "device_uid");
        ResponseStr = GetDeviceStatus(DeviceUid);
    } if (strncmp(Url, "/free_spots_count", strlen("/free_spots_count")) == 0) {
        ResponseStr = GetFreeSpotsCount();
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
//
char* FindValueForKey(const char* key, const char* JsonObject) {
    char *p = strstr(JsonObject, key);
    if (!p) return NULL;
    p += strlen(key);
    while (*p == ' ' || *p == ':' || *p == '\"') ++p;
    return p;
};
//
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
};
//
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
//
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
    } else if (strcmp(Method, "GET") == 0) {
        return HandleGetRequest(Connection, Url);
    };
    return MHD_NO;
};
//
int RunWebServer(){
        struct MHD_Daemon *Daemon = MHD_start_daemon(MHD_USE_INTERNAL_POLLING_THREAD, REST_PORT,
                                                     NULL, NULL, &AnswerToWebRequest, NULL,
                                                     MHD_OPTION_NOTIFY_COMPLETED, RequestCompleted, NULL,
                                                     MHD_OPTION_END);
        if (NULL == Daemon){ 
            return -EXIT_FAILURE;
        };
        printf("[%s][RX]: Server running on port: %d\n", PRINT_TAG, REST_PORT);
        RunRs485Controller();
        MHD_stop_daemon(Daemon);
};
