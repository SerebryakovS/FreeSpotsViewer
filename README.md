## SpotTrack server API

### Discovery algorithm overview

For 12-byte UID we have range from 0 to 3060 for a sum of bytes:  255 * 12 = 3060;
Let's say maximum packet payload is 256 bytes, then: 256 * (1+8+1) / 115200 = 23[ms] * 30%(safety margin) = 30[ms], needs for tranmition;
The theoretical limit = 256 devices that 1/8th load each on RS-485 line;
Maximum delay for devices: 256 * 30[ms] = 7680[ms];


### RS-485 API

#### get_status
request body:
```
{
    "type" : "get_status"
    "uid" : "..."
}
```
response body:
```
{
    "uid" : "...",
    "threshold" : ...,
    "measured_1" : ...,
    "measured_2" : ...,
    "is_parking_clear" : true | false
}
```
example:
```
>{"type":"get_status", "uid":"34FF7206504E393854410243"}
{"type":"get_status", "uid":"34FF6E06504E393817500343"}

<
```
#### ping
request body:
```
{
    "type" : "ping"
    "uid" : "..."
}
```
#### set_working_mode
request body:
```
{
    "type" : "set_mode"
    "uid" : "...",
    "is_auto" : true | false
}
```
#### set_threshold
request body:
```
{
    "type" : "set_threshold"
    "uid" : "...",
    "threshold" : ...
}
```
#### set_parked
request body:
```
{
    "type" : "set_parked"
    "uid" : "...",
    "is_parked_set" : true | false
}
```
for all above requests, response body is:
```
{
    "uid" : "..."
}
```

### Rest API

#### GET  /list_devices
response body:
```
{
    "list_devices" : [
        "<device_uid_1>", "<device_uid_2>", ...
    ]
}
```
#### GET  /get_status?device_uid=...
response body:
```
{
    "threshold" : ...,
    "measured_1" : ...,
    "measured_2" : ...,
    "is_parking_clear" : true | false
}
```
#### POST /set_parked
request body:
```
{
    "device_uid" : "...",
    "value" : true | false
}
```
response body:
```
{
    "is_parking_clear" : true | false
}
```