#!/bin/bash

home=$1
path="$home/CONFIG/servers.list"
proxy="$home/HDFSProxy"
while IFS= read -r line
do
        lineVar=($line)
        ip=${lineVar[1]}
	ssh -o StrictHostKeyChecking=no -n -f $ip "cd $proxy; ./proxy_start.sh $ip $proxy"
done < "$path"
