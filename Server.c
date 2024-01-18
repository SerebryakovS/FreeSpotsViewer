
#include "SpotTrack.h"
#include <microhttpd.h>

const char* GetListOfDevices() {
    return "{\"list_devices\":[\"device_uid_1\",\"device_uid_2\"]}";
};

const char* GetDeviceStatus(const char* DeviceUid) {
    return "{\"threshold\":10,\"measured_1\":20,\"measured_2\":30,\"is_parking_clear\":true}";
};

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

static int AnswerToConnection(void *Cls, struct MHD_Connection *Connection,
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