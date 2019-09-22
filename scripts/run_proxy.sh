#!/bin/bash

if [ $# -ne 3 ]; then
	print "Error: Missing Arguments.."
else
	path=$1"/HDFSProxy"
	sid=$2
	lid=$3
	echo $path
	while [ $sid -le $lid ]; do
		ip="10.1.1."$sid
		ssh -o StrictHostKeyChecking=no -n -f $ip "cd $path; ./proxy_start.sh $ip $path"
		
		let sid=sid+1
	done
fi
