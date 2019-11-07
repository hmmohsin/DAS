#!/bin/bash

home=$1
serversList="$home/CONFIG/servers.list"
clientsList="$home/CONFIG/clients.list"
scripts="$home/scripts"
while IFS= read -r line; do
	lineVar=($line)
	ip=${lineVar[1]}
	ssh -o StrictHostKeyChecking=no $ip "exit"	
done < "$serversList"

while IFS= read -r line; do
	lineVar=($line)
	ip=${lineVar[1]}
	ssh -o StrictHostKeyChecking=no $ip "exit"
done < "$clientsList"
ssh -o StrictHostKeyChecking=no hadoopMaster "exit"
ssh -o StrictHostKeyChecking=no 0.0.0.0 "exit"
