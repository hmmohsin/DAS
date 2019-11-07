#!/bin/bash

home=$1
serversList="$home/CONFIG/servers.list"
scripts="$home/scripts"
while IFS= read -r line
do
        lineVar=($line)
        ip=${lineVar[1]}
	ssh -o StrictHostKeyChecking=no -n -f $ip "cd $scripts; ./dn_start.sh $home -s $ip -i $sid"
done < "$serversList"
