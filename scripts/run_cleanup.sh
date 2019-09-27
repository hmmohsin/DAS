#!/bin/bash

sid=2
lid=12
path=$(pwd)
echo $path
while [ $sid -le $lid ]; do
	ip="10.1.1."$sid
	ssh -o StrictHostKeyChecking=no -n -f $ip "cd $path; ./cleanup.sh $ip"
	let sid=sid+1
done
