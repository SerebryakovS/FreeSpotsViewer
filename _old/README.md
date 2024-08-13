## SpotTrack server API

### Discovery algorithm overview

For 12-byte UID we have range from 0 to 3060 for a sum of bytes:  255 * 12 = 3060;
Let's say maximum packet payload is 256 bytes, then: 256 * (1+8+1) / 115200 = 23[ms] * 30%(safety margin) = 30[ms], needs for tranmition;
The theoretical limit = 256 devices that 1/8th load each on RS-485 line;
Maximum delay for devices: 256 * 30[ms] = 7680[ms];

Server       Client
     <-------RTS
  CTS--------> 
     <-------PACK
  ACK-------->

### RS-485 API

#### get_status (server request)
```
[SpotTrack][TX]: {"uid":"34FF6E06504E393846420243","type":"get_status"}
[SpotTrack][RX]: [SpotTrack][RX]: {
        "uid":"34FF6E06504E393846420243",
        "threshold":1000,
        "measured_1":53,
        "measured_2":0,
        "is_parking_clear":false,
        "is_parked_set":false,
        "is_reserved":false,
        "is_free": false
}
```
#### set_status (client poll)
```
[SpotTrack][RX]: [SpotTrack][RX]: {
        "type" : "set_status",
        "uid":"34FF6E06504E393846420243",
        "is_free": false
}
[SpotTrack][TX]: {"uid":"34FF6E06504E393846420243","type":"ACK"}
```
#### ping
```
[SpotTrack][TX]: {"uid":"34FF6E06504E393846420243","type":"ping"}
[SpotTrack][RX]: {"uid":"34FF6E06504E393846420243"}
```
#### set_threshold
```
[SpotTrack][TX]: {"uid":"34FF6E06504E393846420243","type":"set_threshold","threshold":500}
[SpotTrack][RX]: {"uid":"34FF6E06504E393846420243"}
```
#### set_parked
```
[SpotTrack][TX]: {"uid":"34FF6E06504E393846420243","type":"set_parked","is_parked_set":true}
[SpotTrack][RX]: {"uid":"34FF6E06504E393846420243"}
```
#### set_reserved
```
[SpotTrack][TX]: {"uid":"34FF6E06504E393846420243","type":"set_reserved","is_reserved":true}
[SpotTrack][RX]: {"uid":"34FF6E06504E393846420243"}
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
    "is_parking_clear": false,
    "is_parked_set": false,
    "is_reserved" : false,
    "is_free": false
}
```
#### POST /set_parked
```
$ curl -X POST -d '{"uid":"34FF7206504E393854410243","is_parked_set":true}' http://localhost:16333/set_parked
{
  "uid": "34FF7206504E393854410243"
}
```
#### GET /free_spots_count
```
$ curl -X GET http://localhost:16333/free_spots_count
```
{
    "free_spots_count" : 2
}
#### GET /free_spots
```
$ curl -X GET http://localhost:16333/free_spots
```
{
    "list_devices" :[
        "34FF7206504E393854410243",
        "34FF6E06504E393817500343",
        "34FF6E06504E393846420243"
    ]
}
#### POST /set_reserved
```
$ curl -X POST -d '{"uid":"34FF7206504E393854410243","is_reserved":true}' http://localhost:16333/set_reserved
{
  "uid": "34FF7206504E393854410243"
}
```
#### POST /set_threshold
```
$ curl -X POST -d '{"uid":"34FF7206504E393854410243","threshold":300}' http://localhost:16333/set_threshold
{
  "uid": "34FF7206504E393854410243"
}
```

TODO:

1. is_parking_clear is not always filled
2. multiple equal packet's received from client
3. pretty print didn't finished
