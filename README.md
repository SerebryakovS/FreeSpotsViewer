## SpotTrack server API

### Discovery algorithm overview

For 12-byte UID we have range from 0 to 3060 for a sum of bytes:  255 * 12 = 3060;
Let's say maximum packet payload is 256 bytes, then: 256 * (1+8+1) / 115200 = 23[ms] * 30%(safety margin) = 30[ms], needs for tranmition;
The theoretical limit = 256 devices that 1/8th load each on RS-485 line;
Maximum delay for devices: 256 * 30[ms] = 7680[ms];




### Rest API

#### GET  /list_devices
response body:
```
{
    "list_devices" : [
        <device_uid_1>, <device_uid_2>, ...
    ]
}
```
#### GET  /get_status?device_uid=Str
response body:
```
{
    "threshold" : Int,
    "measured_1" : Int,
    "measured_2" : Int,
    "is_parking_clear" : Bool
}
```
#### POST /set_parked
request body:
```
{
    "device_uid" : "..."
}
```
response body:
```
{
    "is_parking_clear" : Bool
}
```