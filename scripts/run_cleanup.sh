#!/bin/bash

home=$1
serversList="$home/CONFIG/servers.list"
clientsList="$home/CONFIG/clients.list"
scripts="$home/scripts"
while IFS= read -r line
do
	lineVar=($line)
	ip=${lineVar[1]}
	ssh -o StrictHostKeyChecking=no -n -f $ip "cd $scripts; ./cleanup.sh $ip"
done < "$serversList"

while IFS= read -r line
do
	lineVar=($line)
	ip=${lineVar[1]}
	ssh -o StrictHostKeyChecking=no -n -f $ip "cd $scripts; ./cleanup.sh $ip"
done < "$clientsList"
