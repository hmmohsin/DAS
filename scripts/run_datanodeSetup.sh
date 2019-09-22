#!/bin/bash

if [ $# -ne 3 ]; then
	print "Error: Missing arguments"
else
	path=$1
	sid=$2
	lid=$3
	spath=$path"/scripts"
	echo $path
	while [ $sid -le $lid ]; do
		ip="10.1.1."$sid
		echo $ip
		ssh -o StrictHostKeyChecking=no -n -f $ip "sh -c 'cd $spath; nohup ./datanodeSetup.sh $path > /dev/null 2>&1 &'"
		let sid=sid+1
	done
fi
