#!/bin/bash

curl -X POST -d '{"uid":"34FF7206504E393854410243","is_parked_set":true}' http://localhost:16333/set_parked
sleep 1
curl -X POST -d '{"uid":"34FF6E06504E393817500343","is_parked_set":true}' http://localhost:16333/set_parked
sleep 1
curl -X POST -d '{"uid":"34FF6E06504E393846420243","is_parked_set":true}' http://localhost:16333/set_parked
sleep 1
curl -X POST -d '{"uid":"34FF6C06504E393835420243","is_parked_set":true}' http://localhost:16333/set_parked
sleep 1
curl -X POST -d '{"uid":"34FF6D06504E393826420243","is_parked_set":true}' http://localhost:16333/set_parked
sleep 1
curl -X GET http://localhost:16333/free_spots_count
sleep 1

curl -X POST -d '{"uid":"34FF7206504E393854410243","is_parked_set":false}' http://localhost:16333/set_parked
sleep 1
curl -X POST -d '{"uid":"34FF6E06504E393817500343","is_parked_set":false}' http://localhost:16333/set_parked
sleep 1
curl -X POST -d '{"uid":"34FF6E06504E393846420243","is_parked_set":false}' http://localhost:16333/set_parked
sleep 1
curl -X POST -d '{"uid":"34FF6C06504E393835420243","is_parked_set":false}' http://localhost:16333/set_parked
sleep 1
curl -X POST -d '{"uid":"34FF6D06504E393826420243","is_parked_set":false}' http://localhost:16333/set_parked
sleep 1
curl -X GET http://localhost:16333/free_spots_count
sleep 1

	
