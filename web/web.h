
#ifndef SENSOR_H
#define SENSOR_H

#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <microhttpd.h>

#define WEB_RESPONSE_SIZE  4096
#define REST_PORT         12441

#define API_GET_SLAVES 		  "/get_slaves"
#define API_GET_SLAVE_SENSORS "/get_slave_sensors"
#define API_SET_SLAVE_ALIAS   "/set_slave_alias"
#define API_SET_ZONE_ALIAS	  "/set_zone_alias"

const char *GetSlaves(void);
const char *GetSlaveSensors(const char *SlaveId);
const char *SetSlaveAlias(const char *SlaveId, const char *SlaveAlias);
const char *SetZoneAlias(const char *ZoneAlias);

int32_t RunWebServer( void );

#endif
