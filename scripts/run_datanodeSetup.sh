#!/bin/bash

home=$1
serversList="$home/CONFIG/servers.list"
clientsList="$home/CONFIG/clients.list"
scripts="$home/scripts"
while IFS= read -r line
do
        lineVar=($line)
        ip=${lineVar[1]}
	ssh -o StrictHostKeyChecking=no -n -f $ip "cd $scripts; nohup ./datanodeSetup.sh $home > /dev/null 2>&1 &"
done < "$serversList"

while IFS= read -r line
do
        lineVar=($line)
        ip=${lineVar[1]}
	ssh -o StrictHostKeyChecking=no -n -f $ip "cd $scripts; nohup ./datanodeSetup.sh $home > /dev/null 2>&1 &"
done < "$clientsList"
