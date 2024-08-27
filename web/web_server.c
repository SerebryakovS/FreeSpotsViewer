
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

struct PostRequest {
    char* Data; 
	size_t Size;
};

char* FindValueForKey(const char* Key, const char* JsonObject) {
    char *p = strstr(JsonObject, Key);
    if (!p) return NULL;
    p += strlen(Key);
    while (*p == ' ' || *p == ':' || *p == '\"') ++p;
    return p;
};

static char* ExtractJsonValue(const char* Key, const char* JsonObject) {
    char *valueStart = FindValueForKey(Key, JsonObject);
    if (!valueStart) {
        return NULL;
    };
    char *valueEnd = strchr(valueStart, '\"');
    if (!valueEnd) {
        return NULL;
    };
    *valueEnd = '\0';
    return valueStart;
};

static int HandlePostRequest(struct MHD_Connection *Connection, const char* Url,
                             struct PostRequest *_PostRequest) {
    const char *ResponseStr = NULL;

    if (strcmp(Url, API_SET_SLAVE_ALIAS) == 0) {
        char *SlaveId = ExtractJsonValue("\"slave_id\"", _PostRequest->Data);
        char *SlaveAlias = ExtractJsonValue("\"slave_alias\"", _PostRequest->Data);
        if (!SlaveId || !SlaveAlias) {
            return MHD_HTTP_BAD_REQUEST;
        };
        ResponseStr = SetSlaveAlias(SlaveId, SlaveAlias);
    } else if (strcmp(Url, API_SET_ZONE_ALIAS) == 0) {
        char *ZoneAlias = ExtractJsonValue("\"zone_alias\"", _PostRequest->Data);
        if (!ZoneAlias) {
            return MHD_HTTP_BAD_REQUEST;
        };
        ResponseStr = SetZoneAlias(ZoneAlias);
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

static void OnRequestCompleted(void *Cls, struct MHD_Connection *Connection,
                               void **ConCls, enum MHD_RequestTerminationCode Toe) {
    struct PostRequest *_PostRequest = *ConCls;
    if (_PostRequest != NULL) {
        if (_PostRequest->Data != NULL) {
            free(_PostRequest->Data);
        };
        free(_PostRequest);
    };
    *ConCls = NULL;
};

static int WebRequestsHandler(void *cls, struct MHD_Connection *Connection,
                              const char *Url, const char *Method,
                              const char *Version, const char *UploadData,
                              size_t *UploadDataSize, void **ConCls) {
    if (strcmp(Method, "POST") == 0) {
        if (*ConCls == NULL) {
            struct PostRequest *_PostRequest = malloc(sizeof(struct PostRequest));
            if (_PostRequest == NULL) {
                return MHD_NO;
            }
            _PostRequest->Data = NULL;
            _PostRequest->Size = 0;
            *ConCls = _PostRequest;
            return MHD_YES;
        };
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

int32_t RunWebServer() {
    struct MHD_Daemon *Daemon = MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION, REST_PORT,
                                                 NULL, NULL, &WebRequestsHandler, NULL,
                                                 MHD_OPTION_NOTIFY_COMPLETED, OnRequestCompleted, NULL,
                                                 MHD_OPTION_END);
    if (NULL == Daemon) {
        return -EXIT_FAILURE;
    };
    printf("WebServer is running on port: %d\n", REST_PORT);
    MHD_stop_daemon(Daemon);
};
