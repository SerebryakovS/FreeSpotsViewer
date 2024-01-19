## SpotTrack server API

### Discovery algorithm overview

For 12-byte UID we have range from 0 to 3060 for a sum of bytes:  255 * 12 = 3060;
Let's say maximum packet payload is 256 bytes, then: 256 * (1+8+1) / 115200 = 23[ms] * 30%(safety margin) = 30[ms], needs for tranmition;
The theoretical limit = 256 devices that 1/8th load each on RS-485 line;
Maximum delay for devices: 256 * 30[ms] = 7680[ms];


### RS-485 API

#### get_status
```
[SpotTrack][TX]: {"uid":"34FF7206504E393854410243","type":"get_status"}
[SpotTrack][RX]: {"uid":"34FF7206504E393854410243","threshold":1000,"measured_1":1230,"measured_2":   0,"is_parking_clear":true}
```
#### ping
```
[SpotTrack][TX]: {"uid":"34FF7206504E393854410243","type":"ping"}
[SpotTrack][RX]: {"uid":"34FF7206504E393854410243"}
```
#### set_working_mode
```
[SpotTrack][TX]: {"uid":"34FF7206504E393854410243","type":"set_working_mode","is_auto" : false}
[SpotTrack][RX]: {"uid":"34FF7206504E393854410243"}
```
#### set_threshold
```
[SpotTrack][TX]: {"uid":"34FF7206504E393854410243","type":"set_threshold","is_auto" : 500}
[SpotTrack][RX]: {"uid":"34FF7206504E393854410243"}
```
#### set_parked
```
[SpotTrack][TX]: {"uid":"34FF7206504E393854410243","type":"set_parked","is_parked_set" : true}
[SpotTrack][RX]: {"uid":"34FF7206504E393854410243"}
```

### Rest API

#### GET  /list_devices
```
$ curl -X GET http://localhost:16333/list_devices
{
    "list_devices" :[
        "34FF7206504E393854410243",
        "34FF6E06504E393817500343",
        "34FF6E06504E393846420243"
    ]
}
```
#### GET  /get_status?device_uid=...
```
$ curl -X GET http://localhost:16333/get_status?device_uid=34FF7206504E393854410243
{
    "uid": "34FF7206504E393854410243",
    "threshold": 1000,
    "measured_1": 26,
    "measured_2": 0,
    "is_parking_clear": false
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
    "device_uid" : "..."
}
```
