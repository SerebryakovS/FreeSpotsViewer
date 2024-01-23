#!/bin/bash

VievFreeSpots(){
    JsonResponse=$(curl -X GET http://localhost:16333/free_spots_count);
    FreeSpotsCount=$(echo $JsonResponse | jq '.free_spots_count');
    curl http://10.66.100.197/text/__"$FreeSpotsCount"__
}


curl -X POST -d '{"uid":"34FF7206504E393854410243","is_parked_set":true}' http://localhost:16333/set_parked
sleep 1 && VievFreeSpots
curl -X POST -d '{"uid":"34FF6E06504E393817500343","is_parked_set":true}' http://localhost:16333/set_parked
sleep 1 && VievFreeSpots
curl -X POST -d '{"uid":"34FF6E06504E393846420243","is_parked_set":true}' http://localhost:16333/set_parked
sleep 1 && VievFreeSpots
curl -X POST -d '{"uid":"34FF6C06504E393835420243","is_parked_set":true}' http://localhost:16333/set_parked
sleep 1 && VievFreeSpots
curl -X POST -d '{"uid":"34FF6D06504E393826420243","is_parked_set":true}' http://localhost:16333/set_parked
sleep 1 && VievFreeSpots


curl -X POST -d '{"uid":"34FF7206504E393854410243","is_parked_set":false}' http://localhost:16333/set_parked
sleep 1 && VievFreeSpots
curl -X POST -d '{"uid":"34FF6E06504E393817500343","is_parked_set":false}' http://localhost:16333/set_parked
sleep 1 && VievFreeSpots
curl -X POST -d '{"uid":"34FF6E06504E393846420243","is_parked_set":false}' http://localhost:16333/set_parked
sleep 1 && VievFreeSpots
curl -X POST -d '{"uid":"34FF6C06504E393835420243","is_parked_set":false}' http://localhost:16333/set_parked
sleep 1 && VievFreeSpots
curl -X POST -d '{"uid":"34FF6D06504E393826420243","is_parked_set":false}'
sleep 1 && VievFreeSpots

echo '{ "free_spots_count" : 5 }' | jq '.free_spots_count'
